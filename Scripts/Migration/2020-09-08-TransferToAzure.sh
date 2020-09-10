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
  echo -e "Usage:\t$0 -s <source-path> -a <storage-account-name> -c <container-name> [-t <SASToken>] [-l <main|sub|off>]"
  echo -e "Where: "
  echo -e "  -s\tthe source directory to copy from, usually the root of the Orthanc storage area"
  echo -e "  -a\tthe Azure storage account name"
  echo -e "  -c\tthe Azure storage container name - this container MUST already exist"
  echo -e "  -l\toptional parameter to specify if a local copy of the files should be made and then transferred as a batch. Using this option with main or sub specified will make local copies of the files with the correct file extension and then transfer them in a batch so you must have enough space in your /tmp partition to hold a copy of the largest subdirectory to be copied"
  echo -e "\t<main>\t-\tbatch the files by grouping them according to the highest level directory within the source path (requires most free space)"
  echo -e "\t<sub>\t-\tbatch the files by grouping them according to the lowest level directory within the source path (requires least free space)"
  echo -e "\t<off>\t-\tdo not batch the files and transfer them individually (requires no free space, default value)"
  echo -e "  -t\toptional parameter giving a SAS token for the Azure storage - this removes the need to login via browser"
  exit 1
}

if ! command -v azcopy &> /dev/null
then
    echo -e "The Azure command line tool azcopy is required for this script to function and it was not found on your system. Please see https://docs.microsoft.com/en-us/azure/storage/common/storage-use-azcopy-v10 to download and install it"
    exit
fi

RECURSIVE=
SASTOKEN=
LOCALCOPY=false

while getopts 'l:s:a:c:t:' opt
do
  case ${opt} in
    s) SOURCE=$OPTARG ;;
    a) ACCOUNTNAME=$OPTARG ;;
    c) CONTAINERNAME=$OPTARG ;;
    r) RECURSIVE='' ;;
    t) SASTOKEN=$OPTARG ;;
    l) 
       case ${OPTARG} in
         main) LOCALCOPY=true
               RECURSIVE="-maxdepth 0"
               ;;
         sub) LOCALCOPY=true
              RECURSIVE=""
              ;;
         off) LOCALCOPY=false
              RECURSIVE=""
              ;;
         *) usage ;;
       esac
       ;;
    *) usage ;;
  esac
done

if [ -z "${SOURCE}" ] || [ -z "${ACCOUNTNAME}" ] || [ -z "${CONTAINERNAME}" ]
then
  usage
fi

if [ -z "${SASTOKEN}" ]
then
  azcopy login
fi

if [ "$?" == 0 ] 
then
  if [ "${LOCALCOPY}" == true ]
  then
    for dir in  $(find ${SOURCE} ${RECURSIVE} -type d -print | sort)
    do
      echo "Starting local copy of ${dir}"
      TMPDIR=$(mktemp -d -t az-XXXXXXXXXX)
      for a in $(find ${dir} -type f -print | sort)
      do
        case $(file -b --mime-type $a) in
          application/dicom) EXTENSION=.dcm ;;
          text/plain) EXTENSION=.json ;;
          *) EXTENSION="" ;;
        esac
        BLOBFILE=$(basename $a)${EXTENSION}
        cp $a ${TMPDIR}/${BLOBFILE}
        if [ $? -gt 0 ]
        then
          exit 1
        fi
      done
      echo "Local copy complete"
      azcopy copy "${TMPDIR}/*" https://${ACCOUNTNAME}.blob.core.windows.net/${CONTAINERNAME}/${SASTOKEN} --overwrite false --recursive
      if [ $? -gt 0 ]
      then
        exit 1
      fi
      rm -rf ${TMPDIR}
    done
  else
    for a in $(find ${SOURCE} ${RECURSIVE} -type f -print | sort)
    do
      case $(file -b --mime-type $a) in
        application/dicom) EXTENSION=.dcm ;;
        text/plain) EXTENSION=.json ;;
        *) EXTENSION="" ;;
      esac
      BLOBFILE=$(basename $a)${EXTENSION}
      echo azcopy copy $a https://${ACCOUNTNAME}.blob.core.windows.net/${CONTAINERNAME}/${BLOBFILE}${SASTOKEN} --overwrite false
      if [ $? -gt 0 ]
      then
        exit 1
      fi
    done
  fi
fi

if [ -z "${SASTOKEN}" ]
then
  azcopy logout
fi
