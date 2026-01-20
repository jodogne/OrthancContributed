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

#pragma once

#include <orthanc/OrthancCPlugin.h>
#include <json/value.h>
#include <string>

/**
 * PathGenerator - Generates custom storage paths from DICOM tags
 *
 * Known S3-compatible provider path length limits (as of 2025):
 *   - AWS S3:           1024 bytes
 *   - Backblaze B2:     1024 bytes
 *   - Google Cloud:     1024 bytes
 *   - Azure Blob:       1024 bytes
 *   - MinIO:            Varies by backend filesystem
 *   - Wasabi:           1024 bytes
 *   - DigitalOcean:     1024 bytes
 *
 * Recommendation: Set MaxPathLength to 900 to leave margin for RootPath prefix.
 * These limits may change - check your provider's current documentation.
 */
class PathGenerator
{
public:
  static void SetNamingScheme(const std::string& namingScheme);
  static void SetMaxPathLength(size_t maxLength);
  static bool IsDefaultNamingScheme();
  static std::string GetPathFromTags(const Json::Value& tags,
                                     const char* uuid,
                                     OrthancPluginContentType type,
                                     bool encryptionEnabled);
  static std::string GetExtension(OrthancPluginContentType type, bool encryptionEnabled);
};
