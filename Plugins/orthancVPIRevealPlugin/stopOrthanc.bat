echo off
echo Stop Orthanc service. RUN AS ADMINISTRATOR!

net stop orthanc
if %errorlevel% == 2 echo Access Denied - Could not stop service
if %errorlevel% == 0 echo Service stopped successfully

echo Press a key
pause
