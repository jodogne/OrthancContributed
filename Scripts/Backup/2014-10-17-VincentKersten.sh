#!/bin/bash

##
## Contribution by Vincent Kersten <vincent1234567@gmail.com>
## (2014-10-17) on the Google Group of Orthanc Users.
##
## It is meant for use in cron, but you can also run in by
## hand. Giving it any argument will give debugging info
## (e.g. './orthanc-backup.sh debug')
##


[ $1 ] && set -xv

# This tool does its best to shorten downtime on the database backup
# First we sync all, then we stop orthanc, sync once more, start again

# ORG - location of orthanc db-v3 or orthanc db-v4 folder - no trailing slash
# "StorageDirectory" in the orthanc.json configuration file.
ORG="/var/lib/orthanc/db-v4"

# BU destination folder for backup.  - no trailing slash
BU="/mnt/spare-os/var/lib/orthanc/db-v4"

# MAILTO spam me me me!
MAILTO=mailme@thisplace.com

#------------------------------------------------------#

# send reports via mail
function report() {
	echo "$1" | mail -s "$(hostname):$0 reports: $1" "$MAILTO"
}

function syncfolders() {
rsync -aW --delete "$ORG" "$BU" || report "Sync failed"
}

# create backup dir
mkdir -p "$BU"

syncfolders

# stop orthanc
service orthanc stop 
if [ "$?" != "0" ]; then
	report "NO clean backup was made; could not stop orthanc"
	exit 1
fi

syncfolders 

service orthanc start 

service orthanc status >/dev/null || report "Orthanc was not started"

exit 0



#########################################################

# Additional stuff

## Make a rotating 7 day backup
## (this would become /mnt/spare-os/var/orthanc/orthanc-Friday.tgz)
# tar cfvz $(dirname "$BU")/orthanc-$(date +%A).tgz "$BU" 

## process all journals and tidy up DB
# cd "$ORG"
# sqlite3 index "PRAGMA auto_vacuum;"

## make a copy of the database with the backup API
# cd "$ORG"
# sqlite3 index ".backup \"$BU/index\" "



