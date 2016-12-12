echo Copy configuration, stop and restart Orthanc service. RUN AS ADMINISTRATOR!

copy "C:\Program Files (x86)\Orthanc-VPI plugin\bin\Configuration.json" C:\Orthanc\

call "C:\Program Files (x86)\Orthanc-VPI plugin\bin\restartOrthanc.bat"

echo off
set RULE_NAME="Orthanc-1.1.0-Release"
set PROGRAM_NAME="C:\Program Files (x86)\Orthanc\Orthanc Server\Orthanc-1.1.0-Release.exe"

netsh advfirewall firewall show rule name=%RULE_NAME% >nul
if not ERRORLEVEL 1 (
    echo Rule %RULE_NAME% already exists.
) else (
    echo Rule %RULE_NAME% does not exist. Creating...
    netsh advfirewall firewall add rule name=%RULE_NAME% dir=in action=allow program=%PROGRAM_NAME% enable=yes profile=private
)

pause
