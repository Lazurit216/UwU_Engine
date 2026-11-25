@echo off
setlocal

echo ====================================================
echo    UwU Engine - Cleaning Build
echo ====================================================

choice /C DR /N /M "Configuration to clean: [D] Debug, [R] Release "
if errorlevel 3 exit /b 1
if errorlevel 2 goto :release
if not errorlevel 1 exit /b 1

call "%~dp0Clean Debug.bat"
exit /b %ERRORLEVEL%

:release
call "%~dp0Clean Release.bat"
exit /b %ERRORLEVEL%
