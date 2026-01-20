# Orthanc Object Storage Plugin with NamingScheme and WriteRedundant Support

 This is a modified version of the official `orthanc-object-storage`  plugin that adds:

 1. **Custom NamingScheme** - Store DICOM files using variable based paths
    -resultingly adds StorageArea3 API support to the plugin
 2. **WriteRedundant HybridMode** - Write files to BOTH local filesystem AND object storage for redundancy
 3. Adds these features while maintaining backwards compatibility. Custom path only supported on Orthanc >= 1.12.8
    -utilizes version check validation to ensure compatibility


 ## Features

 ### NamingScheme

 Configure custom storage paths using DICOM tags:

 ```json
 {
   "AwsS3Storage": {
     "NamingScheme": "{ResponsibleOrganization}/{StudyDate}/{SOPInstanceUID}{.ext}"
   }
 }
 ```

 Supported placeholders:
 - `{PatientID}`, `{PatientName}`, `{PatientBirthDate}`, `{PatientSex}`
 - `{StudyInstanceUID}`, `{StudyDate}`, `{StudyID}`, `{StudyDescription}`, `{AccessionNumber}`
 - `{SeriesInstanceUID}`, `{SeriesDate}`, `{SeriesDescription}`, `{Modality}`
 - `{SOPInstanceUID}`, `{InstanceNumber}`
 - `{ResponsibleOrganization}` (DICOM tag 0010,2299)
 - `{UUID}` - Orthanc's attachment UUID
 - `{.ext}` - Appropriate file extension (.dcm, .json, .dcm.head, .dcm.enc)
 - `{split(StudyDate)}` - Formats date as YYYY/MM/DD
 - ANY DICOM tag hex notation: `{0010,2299}`, `{0008,0020}`, etc.

 ### WriteRedundant HybridMode

 Store files to both local filesystem AND object storage:

 ```json
 {
   "AwsS3Storage": {
     "HybridMode": "WriteRedundant"
   }
 }
 ```

 This mode:
 - Writes to BOTH filesystem and object storage on every store
 - Reads from filesystem first (faster), falls back to object storage
 - Deletes from both storages
 - Fails write/delete operation if either write fails (strict redundancy)

 ## Requirements

 - Orthanc >= 1.12.8 (for StorageArea3 API with custom path support)
 - Original orthanc-object-storage build dependencies

 ## Configuration Example

 ```json
 {
   "Plugins": ["/path/to/libOrthancAwsS3Storage.so"],
   "AwsS3Storage": {
     "BucketName": "my-dicom-bucket",
     "Region": "us-west-004",
     "Endpoint": "s3.us-west-004.backblazeb2.com",
     "AccessKey": "your-access-key",
     "SecretKey": "your-secret-key",
     "VirtualAddressing": false,
     "HybridMode": "WriteRedundant",
     "NamingScheme": "{ResponsibleOrganization}/{StudyDate}/{SOPInstanceUID}{.ext}",
     "MaxPathLength": 900,
     "EnableLegacyStorageStructure": false,
     "StorageEncryption": {
       "Enable": false
     }
   }
 }
 ```

 ## Building

 Apply these modified files to the official `orthanc-object-storage` repository and build as normal:

 ```bash
 hg clone https://orthanc.uclouvain.be/hg/orthanc-object-storage/
 # Copy modified files from this contribution
 cd orthanc-object-storage
 mkdir build-aws && cd build-aws
 cmake ../Aws -DSTATIC_BUILD=ON -DALLOW_DOWNLOADS=ON -DCMAKE_BUILD_TYPE=Release
 make -j4
 ```

 ## Files Modified

 From the base `orthanc-object-storage` repository:

 - `Common/StoragePlugin.cpp` - Added StorageArea3 API, NamingScheme, WriteRedundant mode
 - `Common/IStorage.h` - Added custom path method signatures
 - `Common/BaseStorage.h/cpp` - Added GetPathWithCustom helper
 - `Common/FileSystemStorage.h/cpp` - Added custom path support for HybridMode
 - `Common/PathGenerator.h/cpp` - NEW: Generates paths from DICOM tags
 - `Aws/AwsS3StoragePlugin.cpp` - Added custom path support

 ## License

 GPLv3 - Same as Orthanc

 ## Author

 R.G. Douglas
