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
#include <OrthancPluginCppWrapper.h>

class StoragePluginException : public std::runtime_error
{
public:
  explicit StoragePluginException(const std::string& what)
    : std::runtime_error(what)
  {
  }
};




// each "plugin" must also provide these global methods
//class XXXStoragePluginFactory
//{
//public:
//  const char* GetStoragePluginName();
//  IStorage* CreateStorage(const OrthancPlugins::OrthancConfiguration& orthancConfig);
//};

class IStorage : public boost::noncopyable
{
public:
  class IWriter
  {
  public:
    IWriter()
    {}

    virtual ~IWriter() {}
    virtual void Write(const char* data, size_t size) = 0;
  };

  class IReader
  {
  public:
    IReader()
    {}

    virtual ~IReader() {}
    virtual size_t GetSize() = 0;
    virtual void ReadWhole(char* data, size_t size) = 0;
    virtual void ReadRange(char* data, size_t size, size_t fromOffset) = 0;
  };

  std::string nameForLogs_;
public:
  IStorage(const std::string& nameForLogs):
    nameForLogs_(nameForLogs)
  {}

  virtual ~IStorage()
  {
  }

  virtual void SetRootPath(const std::string& rootPath) = 0;

  // Legacy methods (for backward compatibility)
  virtual IWriter* GetWriterForObject(const char* uuid, OrthancPluginContentType type, bool encryptionEnabled) = 0;
  virtual IReader* GetReaderForObject(const char* uuid, OrthancPluginContentType type, bool encryptionEnabled) = 0;
  virtual void DeleteObject(const char* uuid, OrthancPluginContentType type, bool encryptionEnabled) = 0;

  // New methods with custom path support
  virtual IWriter* GetWriterForObject(const char* uuid, OrthancPluginContentType type, bool encryptionEnabled, const std::string& customPath)
  {
    // Default implementation ignores customPath for backward compatibility
    return GetWriterForObject(uuid, type, encryptionEnabled);
  }

  virtual IReader* GetReaderForObject(const char* uuid, OrthancPluginContentType type, bool encryptionEnabled, const std::string& customPath)
  {
    // Default implementation ignores customPath for backward compatibility
    return GetReaderForObject(uuid, type, encryptionEnabled);
  }

  virtual void DeleteObject(const char* uuid, OrthancPluginContentType type, bool encryptionEnabled, const std::string& customPath)
  {
    // Default implementation ignores customPath for backward compatibility
    DeleteObject(uuid, type, encryptionEnabled);
  }

  const std::string& GetNameForLogs() {return nameForLogs_;}

  virtual bool HasFileExists() = 0;
  virtual bool FileExists(const std::string& uuid, OrthancPluginContentType type, bool encryptionEnabled) {return false;}
};
