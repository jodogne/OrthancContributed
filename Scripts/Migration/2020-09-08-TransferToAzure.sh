#!/bin/bash
##
## Contribution by Steve Hawes
##
## It is meant to migrate orthanc files from a FileSystem storage to
## an Azure Blob Storage such that it can be used by the Orthanc Azure Blob Storage plugin.
## The script adapts the file structure and naming from hierarchical (FileSystem)
## to flat (Blob storage)
##

usage() {
  echo -e "Usage:\t$0 -s <source-path> -a <storage-account-name> -c <container-name> -r"
  echo -e "Where: "
  echo -e "  -s\tthe source directory to copy from, usually the root of the Orthanc storage area"
  echo -e "  -a\tthe Azure storage account name"
  echo -e "  -c\tthe Azure storage container name - this container MUST already exist"
  echo -e "  -r\toptional paramater to indicate that the specified directory should be copied recursively"
  exit 1
}

if ! command -v azcopy &> /dev/null
then
    echo -e "The Azure command line tool azcopy is required for this script to function and it was not found on your system. Please see https://docs.microsoft.com/en-us/azure/storage/common/storage-use-azcopy-v10 to download and install it"
    exit
fi

RECURSIVE="-maxdepth 1"

while getopts 's:a:c:r' opt
do
  case ${opt} in
    s) SOURCE=$OPTARG ;;
    a) ACCOUNTNAME=$OPTARG ;;
    c) CONTAINERNAME=$OPTARG ;;
    r) RECURSIVE='' ;;
  esac
done

if [ -z "${SOURCE}" ] || [ -z "${ACCOUNTNAME}" ] || [ -z "${CONTAINERNAME}" ]
then
  usage
fi

azcopy login

if [ "$?" == 0 ] 
then
  for a in $(find ${SOURCE} ${RECURSIVE} -type f -print)
  do
    case $(file -b --mime-type $a) in
      application/dicom) EXTENSION=.dcm ;;
      text/plain) EXTENSION=.json ;;
      *) EXTENSION="" ;;
    esac
    BLOBFILE=$(basename $a)${EXTENSION}
    azcopy copy $a https://${ACCOUNTNAME}.blob.core.windows.net/${CONTAINERNAME}/${BLOBFILE}
    if [ $? -gt 0 ]
    then
      exit 1
    fi
  done
fi



azcopy logout
