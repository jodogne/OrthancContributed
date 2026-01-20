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


#if GOOGLE_STORAGE_PLUGIN==1
#  include "../Google/GoogleStoragePlugin.h"
#  define StoragePluginFactory GoogleStoragePluginFactory
#elif AZURE_STORAGE_PLUGIN==1
#  include "../Azure/AzureBlobStoragePlugin.h"
#  define StoragePluginFactory AzureBlobStoragePluginFactory
#elif AWS_STORAGE_PLUGIN==1
#  include "../Aws/AwsS3StoragePlugin.h"
#  define StoragePluginFactory AwsS3StoragePluginFactory
#else
#  pragma message(error  "define a plugin")
#endif

#include <string.h>
#include <stdio.h>
#include <string>

#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include "../Common/EncryptionHelpers.h"
#include "../Common/EncryptionConfigurator.h"
#include "../Common/FileSystemStorage.h"
#include "../Common/PathGenerator.h"

#include <Logging.h>
#include <SystemToolbox.h>
#include "MoveStorageJob.h"
#include "StoragePlugin.h"
#include <Toolbox.h>

#ifdef _WIN32
// This is a hotfix for: https://orthanc.uclouvain.be/hg/orthanc/rev/e4d9a872998f
#  undef ORTHANC_PLUGINS_API
#  define ORTHANC_PLUGINS_API __declspec(dllexport)
#endif


static std::unique_ptr<IStorage> primaryStorage;
static std::unique_ptr<IStorage> secondaryStorage;

static std::unique_ptr<EncryptionHelpers> crypto;
static bool cryptoEnabled = false;
static std::string fileSystemRootPath;
static std::string objectsRootPath;
static std::string hybridModeNameForLogs = "";
static bool useCustomNamingScheme = false;

typedef enum
{
  HybridMode_WriteToFileSystem,       // write to disk, try to read first from disk and then, from object-storage
  HybridMode_WriteToObjectStorage,    // write to object storage, try to read first from object storage and then, from disk
  HybridMode_WriteRedundant,          // write to BOTH disk and object-storage, read from disk first then object-storage
  HybridMode_Disabled                 // read and write only from/to object-storage
} HybridMode;  

static HybridMode hybridMode = HybridMode_Disabled;

static bool IsReadFromDisk()
{
  return hybridMode != HybridMode_Disabled;
}

static bool IsHybridModeEnabled()
{
  return hybridMode != HybridMode_Disabled;
}

typedef void LogErrorFunction(const std::string& message);

static void LogErrorAsWarning(const std::string& message)
{
  LOG(WARNING) << message;
}

static void LogErrorAsError(const std::string& message)
{
  LOG(ERROR) << message;
}



static OrthancPluginErrorCode StorageCreate(const char* uuid,
                                            const void* content,
                                            int64_t size,
                                            OrthancPluginContentType type)
{
  try
  {
    Orthanc::Toolbox::ElapsedTimer timer;
    LOG(INFO) << primaryStorage->GetNameForLogs() << ": creating attachment " << uuid
              << " of type " << boost::lexical_cast<std::string>(type);
    std::unique_ptr<IStorage::IWriter> writer(primaryStorage->GetWriterForObject(uuid, type, cryptoEnabled));

    std::string encryptedData;
    if (cryptoEnabled)
    {
      try
      {
        crypto->Encrypt(encryptedData, (const char*)content, size);
      }
      catch (EncryptionException& ex)
      {
        LOG(ERROR) << primaryStorage->GetNameForLogs() << ": error while encrypting object " << uuid << ": " << ex.what();
        return OrthancPluginErrorCode_StorageAreaPlugin;
      }

      writer->Write(encryptedData.data(), encryptedData.size());
    }
    else
    {
      // No encryption: write directly from source pointer, avoiding memory copy
      writer->Write(reinterpret_cast<const char*>(content), size);
    }
    LOG(INFO) << primaryStorage->GetNameForLogs() << ": created attachment " << uuid
              << " (" << timer.GetHumanTransferSpeed(true, size) << ")";

    // WriteRedundant: also write to secondary storage - MUST succeed for redundancy guarantee
    if (hybridMode == HybridMode_WriteRedundant && secondaryStorage.get() != nullptr)
    {
      try
      {
        LOG(INFO) << secondaryStorage->GetNameForLogs() << ": creating redundant copy of attachment " << uuid;
        std::unique_ptr<IStorage::IWriter> secondaryWriter(secondaryStorage->GetWriterForObject(uuid, type, cryptoEnabled));
        if (cryptoEnabled)
        {
          secondaryWriter->Write(encryptedData.data(), encryptedData.size());
        }
        else
        {
          secondaryWriter->Write(reinterpret_cast<const char*>(content), size);
        }
        LOG(INFO) << secondaryStorage->GetNameForLogs() << ": created redundant copy of attachment " << uuid;
      }
      catch (StoragePluginException& ex)
      {
        // Redundancy mode requires BOTH writes to succeed
        // Primary file is kept (data not lost) but operation fails so Orthanc knows
        LOG(ERROR) << secondaryStorage->GetNameForLogs() << ": FAILED to create redundant copy of object " << uuid << ": " << ex.what();
        LOG(ERROR) << "WriteRedundant mode: secondary write failed. Primary file exists at " << primaryStorage->GetNameForLogs()
                   << " but redundancy is NOT guaranteed. Manual intervention required.";
        return OrthancPluginErrorCode_StorageAreaPlugin;
      }
    }
  }
  catch (StoragePluginException& ex)
  {
    LOG(ERROR) << primaryStorage->GetNameForLogs() << ": error while creating object " << uuid << ": " << ex.what();
    return OrthancPluginErrorCode_StorageAreaPlugin;
  }

  return OrthancPluginErrorCode_Success;
}


static OrthancPluginErrorCode StorageReadRange(IStorage* storage,
                                               LogErrorFunction logErrorFunction,
                                               OrthancPluginMemoryBuffer64* target, // Memory buffer where to store the content of the range.  The memory buffer is allocated and freed by Orthanc. The length of the range of interest corresponds to the size of this buffer.
                                               const char* uuid,
                                               OrthancPluginContentType type,
                                               uint64_t rangeStart)
{
  assert(!cryptoEnabled);

  try
  {
    Orthanc::Toolbox::ElapsedTimer timer;
    LOG(INFO) << storage->GetNameForLogs() << ": reading range of attachment " << uuid
              << " of type " << boost::lexical_cast<std::string>(type);
    
    std::unique_ptr<IStorage::IReader> reader(storage->GetReaderForObject(uuid, type, cryptoEnabled));
    reader->ReadRange(reinterpret_cast<char*>(target->data), target->size, rangeStart);
    
    LOG(INFO) << storage->GetNameForLogs() << ": read range of attachment " << uuid
              << " (" << timer.GetHumanTransferSpeed(true, target->size) << ")";
    return OrthancPluginErrorCode_Success;
  }
  catch (StoragePluginException& ex)
  {
    logErrorFunction(std::string(StoragePluginFactory::GetStoragePluginName()) + ": error while reading object " + std::string(uuid) + ": " + std::string(ex.what()));
    return OrthancPluginErrorCode_StorageAreaPlugin;
  }
}

static OrthancPluginErrorCode StorageReadRange(OrthancPluginMemoryBuffer64* target, // Memory buffer where to store the content of the range.  The memory buffer is allocated and freed by Orthanc. The length of the range of interest corresponds to the size of this buffer.
                                               const char* uuid,
                                               OrthancPluginContentType type,
                                               uint64_t rangeStart)
{
  OrthancPluginErrorCode res = StorageReadRange(primaryStorage.get(),
                                                (IsHybridModeEnabled() ? LogErrorAsWarning : LogErrorAsError), // log errors as warning on first try
                                                target,
                                                uuid,
                                                type,
                                                rangeStart);

  if (res != OrthancPluginErrorCode_Success && IsHybridModeEnabled())
  {
    res = StorageReadRange(secondaryStorage.get(),
                           LogErrorAsError, // log errors as errors on second try
                           target,
                           uuid,
                           type,
                           rangeStart);
  }
  return res;
}



static OrthancPluginErrorCode StorageReadWhole(IStorage* storage,
                                               LogErrorFunction logErrorFunction,
                                               OrthancPluginMemoryBuffer64* target, // Memory buffer where to store the content of the file. It must be allocated by the plugin using OrthancPluginCreateMemoryBuffer64(). The core of Orthanc will free it.
                                               const char* uuid,
                                               OrthancPluginContentType type)
{
  try
  {
    Orthanc::Toolbox::ElapsedTimer timer;
    LOG(INFO) << storage->GetNameForLogs() << ": reading whole attachment " << uuid
              << " of type " << boost::lexical_cast<std::string>(type);
    std::unique_ptr<IStorage::IReader> reader(storage->GetReaderForObject(uuid, type, cryptoEnabled));

    size_t fileSize = reader->GetSize();
    size_t size;

    if (cryptoEnabled)
    {
      size = fileSize - crypto->OVERHEAD_SIZE;
    }
    else
    {
      size = fileSize;
    }

    if (size <= 0)
    {
      logErrorFunction(storage->GetNameForLogs() + ": error while reading object " + std::string(uuid) + ", size of file is too small: " + boost::lexical_cast<std::string>(fileSize) + " bytes");
      return OrthancPluginErrorCode_StorageAreaPlugin;
    }

    if (OrthancPluginCreateMemoryBuffer64(OrthancPlugins::GetGlobalContext(), target, size) != OrthancPluginErrorCode_Success)
    {
      logErrorFunction(storage->GetNameForLogs() + ": error while reading object " + std::string(uuid) + ", cannot allocate memory of size " + boost::lexical_cast<std::string>(size) + " bytes");
      return OrthancPluginErrorCode_StorageAreaPlugin;
    }

    if (cryptoEnabled)
    {
      std::vector<char> encrypted(fileSize);
      reader->ReadWhole(encrypted.data(), fileSize);

      try
      {
        crypto->Decrypt(reinterpret_cast<char*>(target->data), encrypted.data(), fileSize);
      }
      catch (EncryptionException& ex)
      {
        logErrorFunction(storage->GetNameForLogs() + ": error while decrypting object " + std::string(uuid) + ": " + ex.what());
        return OrthancPluginErrorCode_StorageAreaPlugin;
      }
    }
    else
    {
      reader->ReadWhole(reinterpret_cast<char*>(target->data), fileSize);
    }

    LOG(INFO) << storage->GetNameForLogs() << ": read whole attachment " << uuid
              << " (" << timer.GetHumanTransferSpeed(true, fileSize) << ")";
  }
  catch (StoragePluginException& ex)
  {
    logErrorFunction(storage->GetNameForLogs() + ": error while reading object " + std::string(uuid) + ": " + ex.what());
    return OrthancPluginErrorCode_StorageAreaPlugin;
  }

  return OrthancPluginErrorCode_Success;
}

static OrthancPluginErrorCode StorageReadWhole(OrthancPluginMemoryBuffer64* target, // Memory buffer where to store the content of the file. It must be allocated by the plugin using OrthancPluginCreateMemoryBuffer64(). The core of Orthanc will free it.
                                               const char* uuid,
                                               OrthancPluginContentType type)
{
  OrthancPluginErrorCode res = StorageReadWhole(primaryStorage.get(),
                                                (IsHybridModeEnabled() ? LogErrorAsWarning : LogErrorAsError), // log errors as warning on first try
                                                target,
                                                uuid,
                                                type);

  if (res != OrthancPluginErrorCode_Success && IsHybridModeEnabled())
  {
    res = StorageReadWhole(secondaryStorage.get(),
                           LogErrorAsError, // log errors as errors on second try
                           target,
                           uuid,
                           type);
  }
  return res;
}

static OrthancPluginErrorCode StorageReadWholeLegacy(void** content,
                                                     int64_t* size,
                                                     const char* uuid,
                                                     OrthancPluginContentType type)
{
  OrthancPluginMemoryBuffer64 buffer;
  OrthancPluginErrorCode result = StorageReadWhole(&buffer, uuid, type); // will allocate OrthancPluginMemoryBuffer64

  if (result == OrthancPluginErrorCode_Success)
  {
    *size = buffer.size;
    *content = buffer.data; // orthanc will free the buffer (we don't have to delete it ourselves)
  }

  return result;
}


static OrthancPluginErrorCode StorageRemove(IStorage* storage,
                                            LogErrorFunction logErrorFunction,
                                            const char* uuid,
                                            OrthancPluginContentType type)
{
  try
  {
    LOG(INFO) << storage->GetNameForLogs() << ": deleting attachment " << uuid
              << " of type " << boost::lexical_cast<std::string>(type);
    storage->DeleteObject(uuid, type, cryptoEnabled);

    // For WriteRedundant, we need to signal to also delete from secondary
    // For other hybrid modes, only signal if this is the primary storage
    if ((storage == primaryStorage.get()) && IsHybridModeEnabled() &&
        hybridMode != HybridMode_WriteRedundant)
    {
      // not 100% sure the file has been deleted, try the secondary plugin
      return OrthancPluginErrorCode_StorageAreaPlugin;
    }

    return OrthancPluginErrorCode_Success;
  }
  catch (StoragePluginException& ex)
  {
    logErrorFunction(std::string(StoragePluginFactory::GetStoragePluginName()) + ": error while deleting object " + std::string(uuid) + ": " + std::string(ex.what()));
    return OrthancPluginErrorCode_StorageAreaPlugin;
  }
}

static OrthancPluginErrorCode StorageRemove(const char* uuid,
                                            OrthancPluginContentType type)
{
  OrthancPluginErrorCode res = StorageRemove(primaryStorage.get(),
                                             (IsHybridModeEnabled() ? LogErrorAsWarning : LogErrorAsError), // log errors as warning on first try
                                             uuid,
                                             type);

  // For WriteRedundant: MUST also delete from secondary (file exists in both)
  if (hybridMode == HybridMode_WriteRedundant && secondaryStorage.get() != nullptr)
  {
    try
    {
      LOG(INFO) << secondaryStorage->GetNameForLogs() << ": deleting redundant copy of attachment " << uuid;
      secondaryStorage->DeleteObject(uuid, type, cryptoEnabled);
      LOG(INFO) << secondaryStorage->GetNameForLogs() << ": deleted redundant copy of attachment " << uuid;
    }
    catch (StoragePluginException& ex)
    {
      // Redundancy mode requires BOTH deletes to succeed
      LOG(ERROR) << secondaryStorage->GetNameForLogs() << ": FAILED to delete redundant copy of object " << uuid << ": " << ex.what();
      LOG(ERROR) << "WriteRedundant mode: secondary delete failed. Primary file deleted but secondary may still exist. Manual cleanup required.";
      return OrthancPluginErrorCode_StorageAreaPlugin;
    }
  }
  // For other hybrid modes: only try secondary as fallback if primary failed
  else if (res != OrthancPluginErrorCode_Success && IsHybridModeEnabled())
  {
    res = StorageRemove(secondaryStorage.get(),
                        LogErrorAsError, // log errors as errors on second try
                        uuid,
                        type);
  }

  return res;
}


// ============================================================================
// StorageArea3 API callbacks (with customData and dicomInstance support)
// ============================================================================

static OrthancPluginErrorCode StorageCreate3(OrthancPluginMemoryBuffer* customData,
                                             const char* uuid,
                                             const void* content,
                                             uint64_t size,
                                             OrthancPluginContentType type,
                                             OrthancPluginCompressionType compressionType,
                                             const OrthancPluginDicomInstance* dicomInstance)
{
  try
  {
    Orthanc::Toolbox::ElapsedTimer timer;

    // Extract DICOM tags if available
    Json::Value tags;
    std::string customPath;

    if (dicomInstance != NULL && useCustomNamingScheme)
    {
      OrthancPlugins::DicomInstance dicom(dicomInstance);
      dicom.GetSimplifiedJson(tags);

      // Generate custom path from tags
      customPath = PathGenerator::GetPathFromTags(tags, uuid, type, cryptoEnabled);
    }

    LOG(INFO) << primaryStorage->GetNameForLogs() << ": creating attachment " << uuid
              << " of type " << boost::lexical_cast<std::string>(type)
              << (customPath.empty() ? "" : " (custom path: " + customPath + ")");

    std::unique_ptr<IStorage::IWriter> writer(primaryStorage->GetWriterForObject(uuid, type, cryptoEnabled, customPath));

    std::string encryptedData;
    if (cryptoEnabled)
    {
      try
      {
        crypto->Encrypt(encryptedData, (const char*)content, size);
      }
      catch (EncryptionException& ex)
      {
        LOG(ERROR) << primaryStorage->GetNameForLogs() << ": error while encrypting object " << uuid << ": " << ex.what();
        return OrthancPluginErrorCode_StorageAreaPlugin;
      }

      writer->Write(encryptedData.data(), encryptedData.size());
    }
    else
    {
      // No encryption: write directly from source pointer, avoiding memory copy
      writer->Write(reinterpret_cast<const char*>(content), size);
    }

    LOG(INFO) << primaryStorage->GetNameForLogs() << ": created attachment " << uuid
              << " (" << timer.GetHumanTransferSpeed(true, size) << ")";

    // WriteRedundant: also write to secondary storage - MUST succeed for redundancy guarantee
    if (hybridMode == HybridMode_WriteRedundant && secondaryStorage.get() != nullptr)
    {
      try
      {
        LOG(INFO) << secondaryStorage->GetNameForLogs() << ": creating redundant copy of attachment " << uuid
                  << (customPath.empty() ? "" : " (custom path: " + customPath + ")");
        std::unique_ptr<IStorage::IWriter> secondaryWriter(secondaryStorage->GetWriterForObject(uuid, type, cryptoEnabled, customPath));
        if (cryptoEnabled)
        {
          secondaryWriter->Write(encryptedData.data(), encryptedData.size());
        }
        else
        {
          secondaryWriter->Write(reinterpret_cast<const char*>(content), size);
        }
        LOG(INFO) << secondaryStorage->GetNameForLogs() << ": created redundant copy of attachment " << uuid;
      }
      catch (StoragePluginException& ex)
      {
        // Redundancy mode requires BOTH writes to succeed
        // Primary file is kept (data not lost) but operation fails so Orthanc knows
        LOG(ERROR) << secondaryStorage->GetNameForLogs() << ": FAILED to create redundant copy of object " << uuid << ": " << ex.what();
        LOG(ERROR) << "WriteRedundant mode: secondary write failed. Primary file exists at " << primaryStorage->GetNameForLogs()
                   << " but redundancy is NOT guaranteed. Manual intervention required.";
        return OrthancPluginErrorCode_StorageAreaPlugin;
      }
    }

    // Store the custom path in customData so we can retrieve it during read/delete
    // Only allocate if customPath is non-empty to avoid zero-size buffer issues
    if (!customPath.empty())
    {
      if (OrthancPluginCreateMemoryBuffer(OrthancPlugins::GetGlobalContext(), customData, customPath.size()) != OrthancPluginErrorCode_Success)
      {
        LOG(ERROR) << primaryStorage->GetNameForLogs() << ": error allocating customData for " << uuid;
        // Clean up orphaned files on allocation failure
        try
        {
          primaryStorage->DeleteObject(uuid, type, cryptoEnabled, customPath);
          if (hybridMode == HybridMode_WriteRedundant && secondaryStorage.get() != nullptr)
          {
            secondaryStorage->DeleteObject(uuid, type, cryptoEnabled, customPath);
          }
        }
        catch (...)
        {
          LOG(WARNING) << "Failed to clean up orphaned files after customData allocation failure for " << uuid;
        }
        return OrthancPluginErrorCode_StorageAreaPlugin;
      }
      memcpy(customData->data, customPath.data(), customPath.size());
    }
    else
    {
      // Empty customPath - set customData to empty (size 0, data NULL)
      customData->size = 0;
      customData->data = NULL;
    }
  }
  catch (StoragePluginException& ex)
  {
    LOG(ERROR) << primaryStorage->GetNameForLogs() << ": error while creating object " << uuid << ": " << ex.what();
    return OrthancPluginErrorCode_StorageAreaPlugin;
  }

  return OrthancPluginErrorCode_Success;
}


static OrthancPluginErrorCode StorageReadRange3(OrthancPluginMemoryBuffer64* target,
                                                const char* uuid,
                                                OrthancPluginContentType type,
                                                uint64_t rangeStart,
                                                const void* customData,
                                                uint32_t customDataSize)
{
  // Extract custom path from customData
  std::string customPath;
  if (customData != NULL && customDataSize > 0)
  {
    customPath = std::string(reinterpret_cast<const char*>(customData), customDataSize);
  }

  // For encrypted files, we must read the whole file and decrypt, then extract the range
  if (cryptoEnabled)
  {
    try
    {
      Orthanc::Toolbox::ElapsedTimer timer;
      LOG(INFO) << primaryStorage->GetNameForLogs() << ": reading encrypted attachment " << uuid
                << " (range " << rangeStart << "+" << target->size << ")";

      std::unique_ptr<IStorage::IReader> reader(primaryStorage->GetReaderForObject(uuid, type, cryptoEnabled, customPath));
      size_t encryptedSize = reader->GetSize();
      std::string encryptedData(encryptedSize, '\0');
      reader->ReadWhole(&encryptedData[0], encryptedSize);

      std::string decryptedData;
      try
      {
        crypto->Decrypt(decryptedData, encryptedData);
      }
      catch (EncryptionException& ex)
      {
        LOG(ERROR) << primaryStorage->GetNameForLogs() << ": error while decrypting object " << uuid << ": " << ex.what();
        return OrthancPluginErrorCode_StorageAreaPlugin;
      }

      // Validate range is within decrypted data
      if (rangeStart + target->size > decryptedData.size())
      {
        LOG(ERROR) << primaryStorage->GetNameForLogs() << ": requested range exceeds file size for " << uuid;
        return OrthancPluginErrorCode_StorageAreaPlugin;
      }

      memcpy(target->data, decryptedData.data() + rangeStart, target->size);

      LOG(INFO) << primaryStorage->GetNameForLogs() << ": read encrypted attachment " << uuid
                << " (" << timer.GetHumanTransferSpeed(true, target->size) << ")";
      return OrthancPluginErrorCode_Success;
    }
    catch (StoragePluginException& ex)
    {
      // Try secondary storage if hybrid mode enabled
      if (IsHybridModeEnabled())
      {
        try
        {
          std::unique_ptr<IStorage::IReader> reader(secondaryStorage->GetReaderForObject(uuid, type, cryptoEnabled, customPath));
          size_t encryptedSize = reader->GetSize();
          std::string encryptedData(encryptedSize, '\0');
          reader->ReadWhole(&encryptedData[0], encryptedSize);

          std::string decryptedData;
          crypto->Decrypt(decryptedData, encryptedData);

          if (rangeStart + target->size > decryptedData.size())
          {
            LOG(ERROR) << secondaryStorage->GetNameForLogs() << ": requested range exceeds file size for " << uuid;
            return OrthancPluginErrorCode_StorageAreaPlugin;
          }

          memcpy(target->data, decryptedData.data() + rangeStart, target->size);
          return OrthancPluginErrorCode_Success;
        }
        catch (StoragePluginException& ex2)
        {
          LOG(ERROR) << StoragePluginFactory::GetStoragePluginName() << ": error while reading object " << uuid << ": " << ex2.what();
        }
        catch (EncryptionException& ex2)
        {
          LOG(ERROR) << secondaryStorage->GetNameForLogs() << ": error while decrypting object " << uuid << ": " << ex2.what();
        }
      }
      else
      {
        LOG(ERROR) << StoragePluginFactory::GetStoragePluginName() << ": error while reading object " << uuid << ": " << ex.what();
      }
      return OrthancPluginErrorCode_StorageAreaPlugin;
    }
  }

  // Non-encrypted: direct range read
  try
  {
    Orthanc::Toolbox::ElapsedTimer timer;
    LOG(INFO) << primaryStorage->GetNameForLogs() << ": reading range of attachment " << uuid
              << " of type " << boost::lexical_cast<std::string>(type);

    std::unique_ptr<IStorage::IReader> reader(primaryStorage->GetReaderForObject(uuid, type, cryptoEnabled, customPath));
    reader->ReadRange(reinterpret_cast<char*>(target->data), target->size, rangeStart);

    LOG(INFO) << primaryStorage->GetNameForLogs() << ": read range of attachment " << uuid
              << " (" << timer.GetHumanTransferSpeed(true, target->size) << ")";
    return OrthancPluginErrorCode_Success;
  }
  catch (StoragePluginException& ex)
  {
    // Try secondary storage if hybrid mode enabled
    if (IsHybridModeEnabled())
    {
      try
      {
        std::unique_ptr<IStorage::IReader> reader(secondaryStorage->GetReaderForObject(uuid, type, cryptoEnabled, customPath));
        reader->ReadRange(reinterpret_cast<char*>(target->data), target->size, rangeStart);
        return OrthancPluginErrorCode_Success;
      }
      catch (StoragePluginException& ex2)
      {
        LOG(ERROR) << StoragePluginFactory::GetStoragePluginName() << ": error while reading object " << uuid << ": " << ex2.what();
      }
    }
    else
    {
      LOG(ERROR) << StoragePluginFactory::GetStoragePluginName() << ": error while reading object " << uuid << ": " << ex.what();
    }
    return OrthancPluginErrorCode_StorageAreaPlugin;
  }
}


static OrthancPluginErrorCode StorageRemove3(const char* uuid,
                                             OrthancPluginContentType type,
                                             const void* customData,
                                             uint32_t customDataSize)
{
  // Extract custom path from customData
  std::string customPath;
  if (customData != NULL && customDataSize > 0)
  {
    customPath = std::string(reinterpret_cast<const char*>(customData), customDataSize);
  }

  try
  {
    LOG(INFO) << primaryStorage->GetNameForLogs() << ": deleting attachment " << uuid
              << " of type " << boost::lexical_cast<std::string>(type);

    primaryStorage->DeleteObject(uuid, type, cryptoEnabled, customPath);
    LOG(INFO) << primaryStorage->GetNameForLogs() << ": deleted attachment " << uuid;

    // Handle secondary storage deletion based on mode
    if (hybridMode == HybridMode_WriteRedundant && secondaryStorage.get() != nullptr)
    {
      // WriteRedundant: BOTH deletes must succeed
      try
      {
        LOG(INFO) << secondaryStorage->GetNameForLogs() << ": deleting redundant copy of attachment " << uuid;
        secondaryStorage->DeleteObject(uuid, type, cryptoEnabled, customPath);
        LOG(INFO) << secondaryStorage->GetNameForLogs() << ": deleted redundant copy of attachment " << uuid;
      }
      catch (StoragePluginException& ex)
      {
        LOG(ERROR) << secondaryStorage->GetNameForLogs() << ": FAILED to delete redundant copy of object " << uuid << ": " << ex.what();
        LOG(ERROR) << "WriteRedundant mode: secondary delete failed. Primary file deleted but secondary may still exist. Manual cleanup required.";
        return OrthancPluginErrorCode_StorageAreaPlugin;
      }
    }
    else if (IsHybridModeEnabled() && secondaryStorage.get() != nullptr)
    {
      // Other hybrid modes: best-effort delete from secondary
      try
      {
        secondaryStorage->DeleteObject(uuid, type, cryptoEnabled, customPath);
      }
      catch (...)
      {
        // Ignore errors from secondary storage in non-redundant modes
      }
    }

    return OrthancPluginErrorCode_Success;
  }
  catch (StoragePluginException& ex)
  {
    LOG(ERROR) << primaryStorage->GetNameForLogs() << ": error while deleting object " << uuid << ": " << ex.what();

    if (IsHybridModeEnabled() && hybridMode != HybridMode_WriteRedundant)
    {
      // Non-redundant hybrid modes: try secondary storage as fallback
      try
      {
        secondaryStorage->DeleteObject(uuid, type, cryptoEnabled, customPath);
        return OrthancPluginErrorCode_Success;
      }
      catch (StoragePluginException& ex2)
      {
        LOG(ERROR) << StoragePluginFactory::GetStoragePluginName() << ": error while deleting object from secondary " << uuid << ": " << ex2.what();
      }
    }
    return OrthancPluginErrorCode_StorageAreaPlugin;
  }
}


static MoveStorageJob* CreateMoveStorageJob(const std::string& targetStorage, const std::vector<std::string>& instances, const Json::Value& resourcesForJobContent)
{
  std::unique_ptr<MoveStorageJob> job(new MoveStorageJob(targetStorage, instances, resourcesForJobContent, cryptoEnabled));

  if (hybridMode == HybridMode_WriteToFileSystem)
  {
    // WriteToFileSystem: primary=filesystem, secondary=object-storage
    job->SetStorages(primaryStorage.get(), secondaryStorage.get());
  }
  else if (hybridMode == HybridMode_WriteRedundant)
  {
    // WriteRedundant: primary=filesystem, secondary=object-storage (same as WriteToFileSystem)
    job->SetStorages(primaryStorage.get(), secondaryStorage.get());
  }
  else
  {
    // WriteToObjectStorage: primary=object-storage, secondary=filesystem
    job->SetStorages(secondaryStorage.get(), primaryStorage.get());
  }

  return job.release();
}


static void AddResourceForJobContent(Json::Value& resourcesForJobContent /* out */, Orthanc::ResourceType resourceType, const std::string& resourceId)
{
  const char* resourceGroup = Orthanc::GetResourceTypeText(resourceType, true, true);

  if (!resourcesForJobContent.isMember(resourceGroup))
  {
    resourcesForJobContent[resourceGroup] = Json::arrayValue;
  }
  
  resourcesForJobContent[resourceGroup].append(resourceId);
}

void MoveStorage(OrthancPluginRestOutput* output,
                 const char* /*url*/,
                 const OrthancPluginHttpRequest* request)
{
  OrthancPluginContext* context = OrthancPlugins::GetGlobalContext();

  if (request->method != OrthancPluginHttpMethod_Post)
  {
    OrthancPluginSendMethodNotAllowed(context, output, "POST");
    return;
  }

  Json::Value requestPayload;

  if (!OrthancPlugins::ReadJson(requestPayload, request->body, request->bodySize))
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat, "A JSON payload was expected");
  }

  std::vector<std::string> instances;
  Json::Value resourcesForJobContent;

  if (requestPayload.type() != Json::objectValue ||
      !requestPayload.isMember(KEY_RESOURCES) ||
      requestPayload[KEY_RESOURCES].type() != Json::arrayValue)
  {
    throw Orthanc::OrthancException(
      Orthanc::ErrorCode_BadFileFormat,
      "A request to the move-storage endpoint must provide a JSON object "
      "with the field \"" + std::string(KEY_RESOURCES) + 
      "\" containing an array of resources to be sent");
  }

  if (!requestPayload.isMember(KEY_TARGET_STORAGE)
      || requestPayload[KEY_TARGET_STORAGE].type() != Json::stringValue
      || (requestPayload[KEY_TARGET_STORAGE].asString() != STORAGE_TYPE_FILE_SYSTEM && requestPayload[KEY_TARGET_STORAGE].asString() != STORAGE_TYPE_OBJECT_STORAGE))
  {
    throw Orthanc::OrthancException(
      Orthanc::ErrorCode_BadFileFormat,
      "A request to the move-storage endpoint must provide a JSON object "
      "with the field \"" + std::string(KEY_TARGET_STORAGE) + 
      "\" set to \"" + std::string(STORAGE_TYPE_FILE_SYSTEM) + "\" or \"" + std::string(STORAGE_TYPE_OBJECT_STORAGE) +  "\"");
  }

  const std::string& targetStorage = requestPayload[KEY_TARGET_STORAGE].asString();
  const Json::Value& resources = requestPayload[KEY_RESOURCES];

  // Extract information about all the child instances
  for (Json::Value::ArrayIndex i = 0; i < resources.size(); i++)
  {
    if (resources[i].type() != Json::stringValue)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    std::string resource = resources[i].asString();
    if (resource.empty())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_UnknownResource);
    }

    // Test whether this resource is an instance
    Json::Value tmpResource;
    Json::Value tmpInstances;
    if (OrthancPlugins::RestApiGet(tmpResource, "/instances/" + resource, false))
    {
      instances.push_back(resource);
      AddResourceForJobContent(resourcesForJobContent, Orthanc::ResourceType_Instance, resource);
    }
    // This was not an instance, successively try with series/studies/patients
    else if ((OrthancPlugins::RestApiGet(tmpResource, "/series/" + resource, false) &&
              OrthancPlugins::RestApiGet(tmpInstances, "/series/" + resource + "/instances", false)) ||
             (OrthancPlugins::RestApiGet(tmpResource, "/studies/" + resource, false) &&
              OrthancPlugins::RestApiGet(tmpInstances, "/studies/" + resource + "/instances", false)) ||
             (OrthancPlugins::RestApiGet(tmpResource, "/patients/" + resource, false) &&
              OrthancPlugins::RestApiGet(tmpInstances, "/patients/" + resource + "/instances", false)))
    {
      if (tmpInstances.type() != Json::arrayValue)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

      AddResourceForJobContent(resourcesForJobContent, Orthanc::StringToResourceType(tmpResource["Type"].asString().c_str()), resource);

      for (Json::Value::ArrayIndex j = 0; j < tmpInstances.size(); j++)
      {
        instances.push_back(tmpInstances[j]["ID"].asString());
      }
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_UnknownResource);
    }   
  }

  LOG(INFO) << "Moving " << instances.size() << " instances to " << targetStorage;

  std::unique_ptr<MoveStorageJob> job(CreateMoveStorageJob(targetStorage, instances, resourcesForJobContent));

  OrthancPlugins::OrthancJob::SubmitFromRestApiPost(output, requestPayload, job.release());
}

OrthancPluginJob* JobUnserializer(const char* jobType,
                                  const char* serialized)
{
  if (jobType == NULL ||
      serialized == NULL)
  {
    return NULL;
  }

  std::string type(jobType);

  if (type != JOB_TYPE_MOVE_STORAGE)
  {
    return NULL;
  }

  try
  {
    std::string tmp(serialized);

    Json::Value source;
    if (Orthanc::Toolbox::ReadJson(source, tmp))
    {
      std::unique_ptr<OrthancPlugins::OrthancJob> job;

      if (type == JOB_TYPE_MOVE_STORAGE)
      {
        std::vector<std::string> instances;

        for (size_t i = 0; i < source[KEY_INSTANCES].size(); ++i)
        {
          instances.push_back(source[KEY_INSTANCES][static_cast<int>(i)].asString());
        }

        job.reset(CreateMoveStorageJob(source[KEY_TARGET_STORAGE].asString(), instances, source[KEY_CONTENT]));
      }

      if (job.get() == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
      else
      {
        return OrthancPlugins::OrthancJob::Create(job.release());
      }
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "Error while unserializing a job from the " << StoragePluginFactory::GetStoragePluginName() << " plugin: "
               << e.What();
    return NULL;
  }
  catch (...)
  {
    LOG(ERROR) << "Error while unserializing a job from the " << StoragePluginFactory::GetStoragePluginName() << " plugin";
    return NULL;
  }
}


extern "C"
{
  ORTHANC_PLUGINS_API int32_t OrthancPluginInitialize(OrthancPluginContext* context)
  {
    OrthancPlugins::SetGlobalContext(context);

#if ORTHANC_FRAMEWORK_VERSION_IS_ABOVE(1, 12, 4)
    Orthanc::Logging::InitializePluginContext(context, StoragePluginFactory::GetStoragePluginName());
#elif ORTHANC_FRAMEWORK_VERSION_IS_ABOVE(1, 7, 2)
    Orthanc::Logging::InitializePluginContext(context);
#else
    Orthanc::Logging::Initialize(context);
#endif

    /* Check the version of the Orthanc core */
    if (OrthancPluginCheckVersion(context) == 0)
    {
      char info[1024];
      sprintf(info, "Your version of Orthanc (%s) must be above %d.%d.%d to run this plugin",
              context->orthancVersion,
              ORTHANC_PLUGINS_MINIMAL_MAJOR_NUMBER,
              ORTHANC_PLUGINS_MINIMAL_MINOR_NUMBER,
              ORTHANC_PLUGINS_MINIMAL_REVISION_NUMBER);
      LOG(ERROR) << info;
      return -1;
    }

    Orthanc::InitializeFramework("", false);

    OrthancPlugins::OrthancConfiguration orthancConfig;

    LOG(WARNING) << StoragePluginFactory::GetStoragePluginName() << " plugin is initializing";
    OrthancPlugins::SetDescription(StoragePluginFactory::GetStoragePluginName(), StoragePluginFactory::GetStorageDescription());

    try
    {
      const char* pluginSectionName = StoragePluginFactory::GetConfigurationSectionName();
      static const char* const ENCRYPTION_SECTION = "StorageEncryption";

      if (!orthancConfig.IsSection(pluginSectionName))
      {
        LOG(WARNING) << StoragePluginFactory::GetStoragePluginName() << ": no \"" << pluginSectionName
                     << "\" section found in configuration, plugin is disabled";
        return 0;
      }

      OrthancPlugins::OrthancConfiguration pluginSection;
      orthancConfig.GetSection(pluginSection, pluginSectionName);

      bool migrationFromFileSystemEnabled = pluginSection.GetBooleanValue("MigrationFromFileSystemEnabled", false);
      std::string hybridModeString = pluginSection.GetStringValue("HybridMode", "Disabled");

      if (migrationFromFileSystemEnabled && hybridModeString == "Disabled")
      {
        hybridMode = HybridMode_WriteToObjectStorage;
        LOG(WARNING) << StoragePluginFactory::GetStoragePluginName() << ": 'MigrationFromFileSystemEnabled' configuration is deprecated, use 'HybridMode': 'WriteToObjectStorage' instead";
      }
      else if (hybridModeString == "WriteToObjectStorage")
      {
        hybridMode = HybridMode_WriteToObjectStorage;
        LOG(WARNING) << StoragePluginFactory::GetStoragePluginName() << ": WriteToObjectStorage HybridMode is enabled: writing to object-storage, try reading first from object-storage and, then, from file system";
      }
      else if (hybridModeString == "WriteToFileSystem")
      {
        hybridMode = HybridMode_WriteToFileSystem;
        LOG(WARNING) << StoragePluginFactory::GetStoragePluginName() << ": WriteToFileSystem HybridMode is enabled: writing to file system, try reading first from file system and, then, from object-storage";
      }
      else if (hybridModeString == "WriteRedundant")
      {
        hybridMode = HybridMode_WriteRedundant;
        LOG(WARNING) << StoragePluginFactory::GetStoragePluginName() << ": WriteRedundant HybridMode is enabled: writing to BOTH file system and object-storage, reading from file system first then object-storage";
      }
      else if (hybridModeString == "Disabled")
      {
        hybridMode = HybridMode_Disabled;
        LOG(WARNING) << StoragePluginFactory::GetStoragePluginName() << ": HybridMode is disabled: writing to object-storage and reading only from object-storage";
      }
      else
      {
        LOG(ERROR) << StoragePluginFactory::GetStoragePluginName() << ": Invalid HybridMode value: '" << hybridModeString
                   << "'. Valid options are: Disabled, WriteToFileSystem, WriteToObjectStorage, WriteRedundant";
        return -1;
      }

      if (IsReadFromDisk())
      {
        fileSystemRootPath = orthancConfig.GetStringValue("StorageDirectory", "OrthancStorageNotDefined");
        LOG(WARNING) << StoragePluginFactory::GetStoragePluginName() << ": HybridMode: reading from file system is enabled, source: " << fileSystemRootPath;
      }

      objectsRootPath = pluginSection.GetStringValue("RootPath", std::string());

      if (objectsRootPath.size() >= 1 && objectsRootPath[0] == '/')
      {
        LOG(ERROR) << StoragePluginFactory::GetStoragePluginName() << ": The RootPath shall not start with a '/': " << objectsRootPath;
        return -1;
      }

      std::string objecstStoragePluginName = StoragePluginFactory::GetStoragePluginName();
      if (hybridMode == HybridMode_WriteToFileSystem)
      {
        objecstStoragePluginName += " (Secondary: object-storage)";
      }
      else if (hybridMode == HybridMode_WriteToObjectStorage)
      {
        objecstStoragePluginName += " (Primary: object-storage)";
      }
      else if (hybridMode == HybridMode_WriteRedundant)
      {
        objecstStoragePluginName += " (Redundant: object-storage)";
      }

      std::unique_ptr<IStorage> objectStoragePlugin(StoragePluginFactory::CreateStorage(objecstStoragePluginName, orthancConfig));

      if (objectStoragePlugin.get() == nullptr)
      {
        return -1;
      }

      objectStoragePlugin->SetRootPath(objectsRootPath);

      std::unique_ptr<IStorage> fileSystemStoragePlugin;
      if (IsHybridModeEnabled())
      {
        bool fsync = orthancConfig.GetBooleanValue("SyncStorageArea", true);

        std::string filesystemStoragePluginName = StoragePluginFactory::GetStoragePluginName();
        if (hybridMode == HybridMode_WriteToFileSystem)
        {
          filesystemStoragePluginName += " (Primary: file-system)";
        }
        else if (hybridMode == HybridMode_WriteToObjectStorage)
        {
          filesystemStoragePluginName += " (Secondary: file-system)";
        }
        else if (hybridMode == HybridMode_WriteRedundant)
        {
          filesystemStoragePluginName += " (Redundant: file-system)";
        }

        fileSystemStoragePlugin.reset(new FileSystemStoragePlugin(filesystemStoragePluginName, fileSystemRootPath, fsync));
      }

      if (hybridMode == HybridMode_Disabled || hybridMode == HybridMode_WriteToObjectStorage)
      {
        primaryStorage.reset(objectStoragePlugin.release());

        if (hybridMode == HybridMode_WriteToObjectStorage)
        {
          secondaryStorage.reset(fileSystemStoragePlugin.release());
        }
      }
      else if (hybridMode == HybridMode_WriteToFileSystem)
      {
        primaryStorage.reset(fileSystemStoragePlugin.release());
        secondaryStorage.reset(objectStoragePlugin.release());
      }
      else if (hybridMode == HybridMode_WriteRedundant)
      {
        // WriteRedundant: filesystem is primary (for fast reads), object-storage is secondary
        // We write to BOTH, read from filesystem first
        primaryStorage.reset(fileSystemStoragePlugin.release());
        secondaryStorage.reset(objectStoragePlugin.release());
      }

      if (pluginSection.IsSection(ENCRYPTION_SECTION))
      {
        OrthancPlugins::OrthancConfiguration cryptoSection;
        pluginSection.GetSection(cryptoSection, ENCRYPTION_SECTION);

        crypto.reset(EncryptionConfigurator::CreateEncryptionHelpers(cryptoSection));
        cryptoEnabled = crypto.get() != nullptr;
      }

      if (cryptoEnabled)
      {
        LOG(WARNING) << StoragePluginFactory::GetStoragePluginName() << ": client-side encryption is enabled";
      }
      else
      {
        LOG(WARNING) << StoragePluginFactory::GetStoragePluginName() << ": client-side encryption is disabled";
      }

      // Parse NamingScheme configuration
      std::string namingScheme = pluginSection.GetStringValue("NamingScheme", "");
      if (!namingScheme.empty() && namingScheme != "flat" && namingScheme != "legacy")
      {
        PathGenerator::SetNamingScheme(namingScheme);
        useCustomNamingScheme = true;
        LOG(WARNING) << StoragePluginFactory::GetStoragePluginName() << ": Custom NamingScheme enabled: " << namingScheme;

        if (cryptoEnabled)
        {
          LOG(WARNING) << StoragePluginFactory::GetStoragePluginName() << ": Note: Encryption is enabled, range reads will not be supported";
        }

        // Parse MaxPathLength - used to validate generated paths don't exceed provider limits
        // See PathGenerator.h for known provider limits (most S3-compatible: 1024 bytes)
        unsigned int maxPathLength = pluginSection.GetUnsignedIntegerValue("MaxPathLength", 0);
        if (maxPathLength > 0)
        {
          PathGenerator::SetMaxPathLength(maxPathLength);
          LOG(WARNING) << StoragePluginFactory::GetStoragePluginName()
                       << ": MaxPathLength validation enabled: " << maxPathLength << " bytes";
        }
      }


      if (IsHybridModeEnabled())
      {
        OrthancPlugins::RegisterRestCallback<MoveStorage>("/move-storage", true);
        OrthancPluginRegisterJobsUnserializer(context, JobUnserializer);
      }

      // Register storage area callbacks
      if (useCustomNamingScheme)
      {
        // Check Orthanc version - StorageArea3 requires 1.12.8+
        if (!OrthancPlugins::CheckMinimalOrthancVersion(1, 12, 8))
        {
          LOG(ERROR) << StoragePluginFactory::GetStoragePluginName()
                     << ": NamingScheme requires Orthanc 1.12.8 or higher. "
                     << "Please upgrade Orthanc or remove the NamingScheme configuration.";
          return -1;
        }

        // Use StorageArea3 API for custom naming scheme support
        LOG(WARNING) << StoragePluginFactory::GetStoragePluginName() << ": Using StorageArea3 API for custom path support (requires Orthanc >= 1.12.8)";
        OrthancPluginRegisterStorageArea3(context, StorageCreate3, StorageReadRange3, StorageRemove3);
      }
      else if (cryptoEnabled)
      {
        // with encrypted file, we can not support ReadRange.  Therefore, we register the old interface
        OrthancPluginRegisterStorageArea(context, StorageCreate, StorageReadWholeLegacy, StorageRemove);
      }
      else
      {
        OrthancPluginRegisterStorageArea2(context, StorageCreate, StorageReadWhole, StorageReadRange, StorageRemove);
      }
    }
    catch (Orthanc::OrthancException& e)
    {
      LOG(ERROR) << "Exception while creating the object storage plugin: " << e.What();
      return -1;
    }

    return 0;
  }


  ORTHANC_PLUGINS_API void OrthancPluginFinalize()
  {
    LOG(WARNING) << StoragePluginFactory::GetStoragePluginName() << " plugin is finalizing";
    primaryStorage.reset();
    secondaryStorage.reset();
    Orthanc::FinalizeFramework();
  }


  ORTHANC_PLUGINS_API const char* OrthancPluginGetName()
  {
    return StoragePluginFactory::GetStoragePluginName();
  }


  ORTHANC_PLUGINS_API const char* OrthancPluginGetVersion()
  {
    return PLUGIN_VERSION;
  }
}

