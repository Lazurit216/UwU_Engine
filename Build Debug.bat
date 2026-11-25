@echo off
setlocal
pushd "%~dp0" || exit /b 1

echo ====================================================
echo    UwU Engine - Building Debug
echo ====================================================

set "BUILD_DIR=build\Debug"
set "TRACY=OFF"
set "OUTPUT_VARIANT=Debug-x64"

choice /C YN /N /M "Enable Tracy profiling? [Y/N] "
if errorlevel 3 goto :failed
if errorlevel 2 goto :configure
if not errorlevel 1 goto :failed

set "BUILD_DIR=build\Debug-Tracy"
set "TRACY=ON"
set "OUTPUT_VARIANT=Debug-x64-Tracy"

:configure

echo [1/2] Configuring CMake (Tracy: %TRACY%)...
cmake -S . -B "%BUILD_DIR%" -G "Visual Studio 18 2026" -A x64 -DUWU_ENABLE_TRACY=%TRACY%
if errorlevel 1 goto :failed

echo [2/2] Building Debug...
cmake --build "%BUILD_DIR%" --config Debug --verbose
if errorlevel 1 goto :failed

echo.
echo ====================================================
echo    Build finished successfully!
echo.
echo    Visual Studio solution:
echo    "%CD%\%BUILD_DIR%\UwU_Engine.slnx"
echo.
echo    Final output:
echo    "%CD%\bin\%OUTPUT_VARIANT%\Sandbox\Sandbox.exe"
echo    "%CD%\bin\%OUTPUT_VARIANT%\Sandbox\UwU_Engine.dll"
echo.
echo    Intermediate files are in:
echo    "%CD%\bin-int\%OUTPUT_VARIANT%"
echo ====================================================
popd
pause
exit /b 0

:failed
echo.
echo    Build failed. See the errors above.
popd
pause
exit /b 1
