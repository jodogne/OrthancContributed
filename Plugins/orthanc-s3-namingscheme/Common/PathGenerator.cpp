/**
 * Cloud storage plugins for Orthanc
 * Copyright (C) 2020-2023 Osimis S.A., Belgium
 * Copyright (C) 2024-2026 Orthanc Team SRL, Belgium
 * Copyright (C) 2021-2026 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/

#include "PathGenerator.h"

#include <Logging.h>
#include <OrthancException.h>
#include <Toolbox.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#include <iomanip>
#include <regex>
#include <mutex>

static std::string namingScheme_;
static size_t maxPathLength_ = 0;  // 0 means no limit

// DICOM tag keyword to hex mapping
static std::map<std::string, std::string> dicomTagKeywords_;
static std::once_flag dicomTagKeywordsInitFlag_;

static void InitializeTagKeywords()
{
  std::call_once(dicomTagKeywordsInitFlag_, []() {
    // Patient level
    dicomTagKeywords_["PatientID"] = "0010,0020";
    dicomTagKeywords_["PatientName"] = "0010,0010";
    dicomTagKeywords_["PatientBirthDate"] = "0010,0030";
    dicomTagKeywords_["PatientSex"] = "0010,0040";
    dicomTagKeywords_["ResponsibleOrganization"] = "0010,2299";

    // Study level
    dicomTagKeywords_["StudyInstanceUID"] = "0020,000D";
    dicomTagKeywords_["StudyDate"] = "0008,0020";
    dicomTagKeywords_["StudyTime"] = "0008,0030";
    dicomTagKeywords_["StudyID"] = "0020,0010";
    dicomTagKeywords_["StudyDescription"] = "0008,1030";
    dicomTagKeywords_["AccessionNumber"] = "0008,0050";
    dicomTagKeywords_["ReferringPhysicianName"] = "0008,0090";

    // Series level
    dicomTagKeywords_["SeriesInstanceUID"] = "0020,000E";
    dicomTagKeywords_["SeriesDate"] = "0008,0021";
    dicomTagKeywords_["SeriesDescription"] = "0008,103E";
    dicomTagKeywords_["SeriesNumber"] = "0020,0011";
    dicomTagKeywords_["Modality"] = "0008,0060";

    // Instance level
    dicomTagKeywords_["SOPInstanceUID"] = "0008,0018";
    dicomTagKeywords_["SOPClassUID"] = "0008,0016";
    dicomTagKeywords_["InstanceNumber"] = "0020,0013";
  });
}

static std::string GetTagValueFromJson(const Json::Value& tags, const std::string& tagKey)
{
  // First try direct access (simplified JSON uses keywords)
  if (tags.isMember(tagKey) && tags[tagKey].isString())
  {
    return tags[tagKey].asString();
  }

  // If it's an integer value (like InstanceNumber), convert to string
  if (tags.isMember(tagKey) && tags[tagKey].isInt())
  {
    return boost::lexical_cast<std::string>(tags[tagKey].asInt());
  }

  return "";
}

static std::string GetTagValueByHex(const Json::Value& tags, const std::string& hexTag)
{
  // hexTag format: "0010,2299" or "0010,0020"
  // Simplified JSON uses keyword names, so we need to reverse lookup
  InitializeTagKeywords();

  for (const auto& pair : dicomTagKeywords_)
  {
    if (pair.second == hexTag)
    {
      return GetTagValueFromJson(tags, pair.first);
    }
  }

  return "";
}

static std::string SanitizeForPath(const std::string& value)
{
  // Remove or replace characters that are invalid in paths/S3 keys
  std::string result;
  result.reserve(value.size());

  for (char c : value)
  {
    if (c == '/' || c == '\\' || c == ':' || c == '*' ||
        c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
    {
      result += '_';
    }
    else if (c >= 32 && c < 127)  // Only printable ASCII
    {
      result += c;
    }
    else
    {
      result += '_';
    }
  }

  // Trim trailing spaces and dots (problematic on Windows/some systems)
  while (!result.empty() && (result.back() == ' ' || result.back() == '.'))
  {
    result.pop_back();
  }

  return result.empty() ? "UNKNOWN" : result;
}

void PathGenerator::SetNamingScheme(const std::string& namingScheme)
{
  namingScheme_ = namingScheme;

  if (!namingScheme_.empty() && namingScheme_ != "flat" && namingScheme_ != "legacy")
  {
    LOG(WARNING) << "PathGenerator: Using custom naming scheme: " << namingScheme_;

    // Validate that scheme contains UUID or SOPInstanceUID for uniqueness
    if (namingScheme_.find("{UUID}") == std::string::npos &&
        namingScheme_.find("{SOPInstanceUID}") == std::string::npos)
    {
      LOG(WARNING) << "PathGenerator: Custom naming scheme should contain {UUID} or {SOPInstanceUID} for uniqueness";
    }
  }
}

void PathGenerator::SetMaxPathLength(size_t maxLength)
{
  maxPathLength_ = maxLength;

  if (maxLength > 0)
  {
    LOG(WARNING) << "PathGenerator: Maximum path length set to " << maxLength << " bytes";
  }
}

bool PathGenerator::IsDefaultNamingScheme()
{
  return namingScheme_.empty() || namingScheme_ == "flat" || namingScheme_ == "legacy";
}

std::string PathGenerator::GetExtension(OrthancPluginContentType type, bool encryptionEnabled)
{
  std::string extension;

  switch (type)
  {
    case OrthancPluginContentType_Dicom:
      extension = ".dcm";
      break;
    case OrthancPluginContentType_DicomAsJson:
      extension = ".json";
      break;
    case OrthancPluginContentType_DicomUntilPixelData:
      extension = ".dcm.head";
      break;
    default:
      extension = ".unk";
      break;
  }

  if (encryptionEnabled)
  {
    extension += ".enc";
  }

  return extension;
}

std::string PathGenerator::GetPathFromTags(const Json::Value& tags,
                                           const char* uuid,
                                           OrthancPluginContentType type,
                                           bool encryptionEnabled)
{
  if (IsDefaultNamingScheme())
  {
    // Return empty to indicate default path generation should be used
    return "";
  }

  // If tags are null/empty, we cannot generate a custom path
  // This happens for non-DICOM attachments when tags weren't provided
  // In that case, fall back to default path generation
  if (tags.isNull() || tags.empty())
  {
    LOG(INFO) << "PathGenerator: No tags available for attachment " << uuid
              << ", using default path";
    return "";
  }

  InitializeTagKeywords();

  std::string path = namingScheme_;

  // Replace {UUID}
  boost::replace_all(path, "{UUID}", uuid);

  // Replace {.ext} with appropriate extension for this content type
  boost::replace_all(path, "{.ext}", GetExtension(type, encryptionEnabled));

  // Replace DICOM tag placeholders in hex format {XXXX,YYYY}
  // Static regex - compiled once for performance
  static const std::regex hexTagRegex("\\{([0-9A-Fa-f]{4}),([0-9A-Fa-f]{4})\\}");
  std::smatch match;
  std::string temp = path;

  while (std::regex_search(temp, match, hexTagRegex))
  {
    std::string fullMatch = match[0].str();
    std::string hexTag = match[1].str() + "," + match[2].str();

    // Convert to lowercase for comparison
    std::transform(hexTag.begin(), hexTag.end(), hexTag.begin(), ::tolower);

    std::string value = GetTagValueByHex(tags, hexTag);
    if (value.empty())
    {
      value = "UNKNOWN";
    }

    boost::replace_all(path, fullMatch, SanitizeForPath(value));
    temp = match.suffix().str();
  }

  // Replace keyword-based placeholders
  for (const auto& pair : dicomTagKeywords_)
  {
    std::string placeholder = "{" + pair.first + "}";
    if (path.find(placeholder) != std::string::npos)
    {
      std::string value = GetTagValueFromJson(tags, pair.first);
      if (value.empty())
      {
        value = "NO_" + pair.first;
      }
      boost::replace_all(path, placeholder, SanitizeForPath(value));
    }
  }

  // Handle special formatted dates like {split(StudyDate)} -> YYYY/MM/DD
  if (path.find("{split(StudyDate)}") != std::string::npos)
  {
    std::string date = GetTagValueFromJson(tags, "StudyDate");
    if (date.size() == 8)
    {
      std::string formatted = date.substr(0, 4) + "/" + date.substr(4, 2) + "/" + date.substr(6, 2);
      boost::replace_all(path, "{split(StudyDate)}", formatted);
    }
    else
    {
      boost::replace_all(path, "{split(StudyDate)}", "NO_DATE");
    }
  }

  // Clean up any double slashes
  while (path.find("//") != std::string::npos)
  {
    boost::replace_all(path, "//", "/");
  }

  // Remove leading slash if present
  if (!path.empty() && path[0] == '/')
  {
    path = path.substr(1);
  }

  // Validate path length if configured
  if (maxPathLength_ > 0 && path.size() > maxPathLength_)
  {
    LOG(ERROR) << "PathGenerator: Generated path exceeds maximum length ("
               << path.size() << " > " << maxPathLength_ << " bytes): " << path;
    throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange,
      "Generated storage path exceeds MaxPathLength limit (" +
      boost::lexical_cast<std::string>(path.size()) + " > " +
      boost::lexical_cast<std::string>(maxPathLength_) + " bytes)");
  }

  LOG(INFO) << "PathGenerator: Generated path for " << uuid << ": " << path;

  return path;
}
