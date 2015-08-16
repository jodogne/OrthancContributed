#!/bin/bash
#
# Here is a backup script for PostgreSQL on UBUNTU. Please see the inline 
# comments for implementation:
#
# Orthanc backup script for PostgreSQL pg_backup.bash
# Based on code by "Fhilip and Pascal"
# http://www.defitek.com/blog/2010/01/06/a-simple-yet-effective-postgresql-backup-script/
# modified/fixed by J.Webster for Orthanc backup
#
# Crontab entry should besimilar to the below. Backup triggers at 30 min past midnight
# It must run under an account that has superuser privs to the database
# crontab will send output to the indicated
# 30 0 * * * /Orthanc/scripts/pg_backup.bash >> /Orthanc/scripts/dbbackup.log 2>&1
#
# Files should be placed at /Orthanc/scripts and execute privilage added
# e.g. chmod +x pg_backup.bash
#
logfile=/Orthanc/scripts/dbbackup.log     # => This must be the same as the crontab entry
backup_dir=/data/backups/postgres-backup/ # => location to put backup
backup_date=`date +%y%m%d%H%M`            # => string to append to the name of the backup files
number_of_days=7                          # => number of days to keep copies of databases

# Get list of databases to back up, but exclude templates and postgres d'base
#
databases=`psql -lqt | grep -vE '^ +(template[0-9]+|postgres)? *\|' | cut -d'|' -f1| sed -e 's/ //g' -e '/^$/d'`

# log the starting time
echo [`date`] PostgreSQL backup START `date`

# Backup each database
for i in $databases; do
outfile=$backup_dir$i-pg_dump.$backup_date.gz
echo -n " –> Dumping $i to $outfile "
pg_dump -Fc $i | gzip > $outfile
outfilesize=`ls -lh $outfile | awk '{ print $5 }'`
echo  " ($outfilesize) "
done

# Delete old backups and trim the backup log file
echo " –> Deleteing backups older than $number_of_days days "
find $backup_dir -type f -prune -mtime +$number_of_days -exec rm -f {} \;
# finish time
echo [`date`] PostgreSQL backup END `date`
# Trim the log file
# Ensure this is the same name as the contab
tail -150 $logfile > trimmed.log
mv trimmed.log $logfile
echo "Logfile trimmed" >>$logfile
exit
