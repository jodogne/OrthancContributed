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


#include "BaseStorage.h"

#include <Logging.h>

#include <boost/filesystem/fstream.hpp>

boost::filesystem::path BaseStorage::GetOrthancFileSystemPath(const std::string& uuid, const std::string& fileSystemRootPath)
{
  boost::filesystem::path path = fileSystemRootPath;

  path /= std::string(&uuid[0], &uuid[2]);
  path /= std::string(&uuid[2], &uuid[4]);
  path /= uuid;

  path.make_preferred();

  return path;
}


std::string BaseStorage::GetPath(const char* uuid, OrthancPluginContentType type, bool encryptionEnabled, bool useAlternateExtension)
{
  if (enableLegacyStorageStructure_)
  {
    return GetOrthancFileSystemPath(uuid, rootPath_).string();
  }
  else
  {
    boost::filesystem::path path = rootPath_;
    std::string filename = std::string(uuid);

    if (type == OrthancPluginContentType_Dicom)
    {
      filename += ".dcm";
    }
    else if (type == OrthancPluginContentType_DicomAsJson)
    {
      filename += ".json";
    }
    else if (type == OrthancPluginContentType_DicomUntilPixelData && !useAlternateExtension)  // for some time, header files were saved with .unk (happened with S3 storage plugin between version 1.2.0 and 1.3.0 - 21.5.1 and 21.6.2)
    {
      filename += ".dcm.head";
    }
    else
    {
      filename += ".unk";
    }

    if (encryptionEnabled)
    {
      filename += ".enc";
    }
    path /= filename;

    return path.string();
  }
}

std::string BaseStorage::GetPathWithCustom(const char* uuid, OrthancPluginContentType type, bool encryptionEnabled, const std::string& customPath)
{
  if (!customPath.empty())
  {
    // Use the custom path (already includes rootPath from PathGenerator)
    boost::filesystem::path path = rootPath_;
    path /= customPath;
    return path.string();
  }
  else
  {
    // Fall back to UUID-based path
    return GetPath(uuid, type, encryptionEnabled, false);
  }
}

bool BaseStorage::ReadCommonConfiguration(bool& enableLegacyStorageStructure, bool& storageContainsUnknownFiles, const OrthancPlugins::OrthancConfiguration& pluginSection)
{
  std::string storageStructure = pluginSection.GetStringValue("StorageStructure", "flat");
  if (storageStructure == "flat")
  {
    enableLegacyStorageStructure = false;
  }
  else
  {
    enableLegacyStorageStructure = true;
    if (storageStructure != "legacy")
    {
      LOG(ERROR) << "ObjectStorage/StorageStructure configuration invalid value: " << storageStructure << ", allowed values are 'flat' and 'legacy'";
      return false;
    }
  }

  storageContainsUnknownFiles = pluginSection.GetBooleanValue("EnableLegacyUnknownFiles", false);

  return true;
}
