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

#include "IStorage.h"
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;


class FileSystemStoragePlugin: public IStorage
{
public:
  class FileSystemWriter: public IStorage::IWriter
  {
    const fs::path path_;
    bool fsync_;
  public:
    FileSystemWriter(const fs::path& path, bool fsync)
    : path_(path),
      fsync_(fsync)
    {}

    virtual ~FileSystemWriter() {}
    virtual void Write(const char* data, size_t size) ORTHANC_OVERRIDE;
  };

  class FileSystemReader: public IStorage::IReader
  {
    const fs::path path_;
  public:
    explicit FileSystemReader(const fs::path& path)
    : path_(path)
    {}

    virtual ~FileSystemReader() {}
    virtual size_t GetSize() ORTHANC_OVERRIDE;
    virtual void ReadWhole(char* data, size_t size) ORTHANC_OVERRIDE;
    virtual void ReadRange(char* data, size_t size, size_t fromOffset) ORTHANC_OVERRIDE;
  };

  std::string fileSystemRootPath_;
  bool fsync_;
public:
  FileSystemStoragePlugin(const std::string& nameForLogs, const std::string& fileSystemRootPath, bool fsync)
  : IStorage(nameForLogs),
    fileSystemRootPath_(fileSystemRootPath),
    fsync_(fsync)
  {}

  virtual void SetRootPath(const std::string& rootPath) ORTHANC_OVERRIDE {}

  virtual IStorage::IWriter* GetWriterForObject(const char* uuid, OrthancPluginContentType type, bool encryptionEnabled) ORTHANC_OVERRIDE;
  virtual IStorage::IReader* GetReaderForObject(const char* uuid, OrthancPluginContentType type, bool encryptionEnabled) ORTHANC_OVERRIDE;
  virtual void DeleteObject(const char* uuid, OrthancPluginContentType type, bool encryptionEnabled) ORTHANC_OVERRIDE;

  // Custom path support overrides
  virtual IStorage::IWriter* GetWriterForObject(const char* uuid, OrthancPluginContentType type, bool encryptionEnabled, const std::string& customPath) ORTHANC_OVERRIDE;
  virtual IStorage::IReader* GetReaderForObject(const char* uuid, OrthancPluginContentType type, bool encryptionEnabled, const std::string& customPath) ORTHANC_OVERRIDE;
  virtual void DeleteObject(const char* uuid, OrthancPluginContentType type, bool encryptionEnabled, const std::string& customPath) ORTHANC_OVERRIDE;

  virtual bool HasFileExists() ORTHANC_OVERRIDE {return true;};
  virtual bool FileExists(const std::string& uuid, OrthancPluginContentType type, bool encryptionEnabled) ORTHANC_OVERRIDE;
};
