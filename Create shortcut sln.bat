@echo off
setlocal
pushd "%~dp0" || exit /b 1

echo ====================================================
echo    UwU Engine - Create Visual Studio Shortcut
echo ====================================================
echo    1. Debug
echo    2. Debug with Tracy
echo    3. Release
echo    4. Release with Tracy
echo.

choice /C 1234 /N /M "Choose a solution [1-4]: "
if errorlevel 5 goto :failed
if not errorlevel 1 goto :failed

if errorlevel 4 (
    set "BUILD_DIR=build\Release-Tracy"
) else if errorlevel 3 (
    set "BUILD_DIR=build\Release"
) else if errorlevel 2 (
    set "BUILD_DIR=build\Debug-Tracy"
) else (
    set "BUILD_DIR=build\Debug"
)

set "UWU_SHORTCUT_TARGET=%CD%\%BUILD_DIR%\UwU_Engine.slnx"
if not exist "%UWU_SHORTCUT_TARGET%" (
    echo Solution not found: "%UWU_SHORTCUT_TARGET%"
    echo Run the matching build script first.
    goto :failed
)

powershell.exe -NoProfile -Command "$ErrorActionPreference = 'Stop'; $shell = New-Object -ComObject WScript.Shell; $link = $shell.CreateShortcut((Join-Path (Get-Location).Path 'UwU_Engine.slnx.lnk')); $link.TargetPath = $env:UWU_SHORTCUT_TARGET; $link.WorkingDirectory = Split-Path -Parent $env:UWU_SHORTCUT_TARGET; $link.Save()"
if errorlevel 1 goto :failed

echo Shortcut updated: UwU_Engine.slnx.lnk
echo Target: "%UWU_SHORTCUT_TARGET%"
popd
pause
exit /b 0

:failed
popd
pause
exit /b 1
