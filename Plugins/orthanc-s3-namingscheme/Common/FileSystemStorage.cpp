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

#include "FileSystemStorage.h"
#include "BaseStorage.h"

#include <Logging.h>
#include <SystemToolbox.h>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

namespace fs = boost::filesystem;

void FileSystemStoragePlugin::FileSystemWriter::Write(const char* data, size_t size)
{
  Orthanc::SystemToolbox::MakeDirectory(path_.parent_path().string());

  Orthanc::SystemToolbox::WriteFile(reinterpret_cast<const void*>(data), size, path_.string(), fsync_);
}

size_t FileSystemStoragePlugin::FileSystemReader::GetSize()
{
  if (!Orthanc::SystemToolbox::IsRegularFile(path_.string()))
  {
    throw StoragePluginException(std::string("The path does not point to a regular file: ") + path_.string());
  }

  try
  {
    fs::ifstream f;
    f.open(path_.string(), std::ifstream::in | std::ifstream::binary);
    if (!f.good())
    {
      throw StoragePluginException(std::string("The path does not point to a regular file: ") + path_.string());
    }

    f.seekg(0, std::ios::end);
    std::streamsize fileSize = f.tellg();
    f.seekg(0, std::ios::beg);

    f.close();

    return fileSize;
  }
  catch (...)
  {
    throw StoragePluginException(std::string("Unexpected error while reading: ") + path_.string());
  }

}

void FileSystemStoragePlugin::FileSystemReader::ReadWhole(char* data, size_t size)
{
  ReadRange(data, size, 0);
}

void FileSystemStoragePlugin::FileSystemReader::ReadRange(char* data, size_t size, size_t fromOffset)
{
  if (!Orthanc::SystemToolbox::IsRegularFile(path_.string()))
  {
    throw StoragePluginException(std::string("The path does not point to a regular file: ") + path_.string());
  }

  try
  {
    fs::ifstream f;
    f.open(path_.string(), std::ifstream::in | std::ifstream::binary);
    if (!f.good())
    {
      throw StoragePluginException(std::string("The path does not point to a regular file: ") + path_.string());
    }

    f.seekg(fromOffset, std::ios::beg);

    f.read(reinterpret_cast<char*>(data), size);

    f.close();
  }
  catch (...)
  {
    throw StoragePluginException(std::string("Unexpected error while reading: ") + path_.string());
  }
}



IStorage::IWriter* FileSystemStoragePlugin::GetWriterForObject(const char* uuid, OrthancPluginContentType type, bool encryptionEnabled)
{
  return new FileSystemWriter(BaseStorage::GetOrthancFileSystemPath(uuid, fileSystemRootPath_), fsync_);
}

IStorage::IReader* FileSystemStoragePlugin::GetReaderForObject(const char* uuid, OrthancPluginContentType type, bool encryptionEnabled)
{
  return new FileSystemReader(BaseStorage::GetOrthancFileSystemPath(uuid, fileSystemRootPath_));
}

void FileSystemStoragePlugin::DeleteObject(const char* uuid, OrthancPluginContentType type, bool encryptionEnabled)
{
  try
  {
    namespace fs = boost::filesystem;

    fs::path path = BaseStorage::GetOrthancFileSystemPath(uuid, fileSystemRootPath_);

    try
    {
      fs::remove(path.string());
    }
    catch (...)
    {
      // Ignore the error
    }

    // Remove the two parent directories, ignoring the error code if
    // these directories are not empty

    try
    {
      boost::system::error_code err;
      fs::remove(path.parent_path().string(), err);
      fs::remove(path.parent_path().parent_path().string(), err);
    }
    catch (...)
    {
      // Ignore the error
    }
  }
  catch(Orthanc::OrthancException& e)
  {
    LOG(ERROR) << GetNameForLogs() << ": error while deleting object " << uuid << ": " << e.What();
  }

}

bool FileSystemStoragePlugin::FileExists(const std::string& uuid, OrthancPluginContentType type, bool encryptionEnabled)
{
  namespace fs = boost::filesystem;

  fs::path path = BaseStorage::GetOrthancFileSystemPath(uuid, fileSystemRootPath_);

  return Orthanc::SystemToolbox::IsRegularFile(path.string());
}

// Custom path support implementations

IStorage::IWriter* FileSystemStoragePlugin::GetWriterForObject(const char* uuid, OrthancPluginContentType type, bool encryptionEnabled, const std::string& customPath)
{
  if (!customPath.empty())
  {
    fs::path path = fileSystemRootPath_;
    path /= customPath;
    LOG(INFO) << GetNameForLogs() << ": Writing to custom path: " << path.string();
    return new FileSystemWriter(path, fsync_);
  }
  else
  {
    return GetWriterForObject(uuid, type, encryptionEnabled);
  }
}

IStorage::IReader* FileSystemStoragePlugin::GetReaderForObject(const char* uuid, OrthancPluginContentType type, bool encryptionEnabled, const std::string& customPath)
{
  if (!customPath.empty())
  {
    fs::path path = fileSystemRootPath_;
    path /= customPath;
    LOG(INFO) << GetNameForLogs() << ": Reading from custom path: " << path.string();
    return new FileSystemReader(path);
  }
  else
  {
    return GetReaderForObject(uuid, type, encryptionEnabled);
  }
}

void FileSystemStoragePlugin::DeleteObject(const char* uuid, OrthancPluginContentType type, bool encryptionEnabled, const std::string& customPath)
{
  try
  {
    namespace fs = boost::filesystem;

    fs::path path;
    if (!customPath.empty())
    {
      path = fileSystemRootPath_;
      path /= customPath;
      LOG(INFO) << GetNameForLogs() << ": Deleting from custom path: " << path.string();
    }
    else
    {
      path = BaseStorage::GetOrthancFileSystemPath(uuid, fileSystemRootPath_);
    }

    try
    {
      fs::remove(path.string());
    }
    catch (...)
    {
      // Ignore the error
    }

    // Remove empty parent directories
    try
    {
      boost::system::error_code err;
      fs::path parent = path.parent_path();

      // Remove empty directories up the tree (up to 5 levels for custom paths)
      for (int i = 0; i < 5 && !parent.empty() && parent != fileSystemRootPath_; ++i)
      {
        if (fs::is_empty(parent))
        {
          fs::remove(parent, err);
          parent = parent.parent_path();
        }
        else
        {
          break;
        }
      }
    }
    catch (...)
    {
      // Ignore the error
    }
  }
  catch(Orthanc::OrthancException& e)
  {
    LOG(ERROR) << GetNameForLogs() << ": error while deleting object " << uuid << ": " << e.What();
  }
}
