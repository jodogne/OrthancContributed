echo off
echo Stop and restart Orthanc service. RUN AS ADMINISTRATOR!

net stop orthanc
if %errorlevel% == 2 echo Access Denied - Could not stop service
if %errorlevel% == 0 echo Service stopped successfully

net start orthanc
if %errorlevel% == 2 echo Access Denied - Could not start service
if %errorlevel% == 0 echo Service started successfully
