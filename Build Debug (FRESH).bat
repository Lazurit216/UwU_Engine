@echo off
echo ====================================================
echo    UwU Engine - Building Debug (FRESH)
echo ====================================================

if not exist "build" mkdir build

cd build

echo [1/2] Configuring CMake...
cmake .. -G "Visual Studio 18 2026" -A x64 --fresh
echo [2/2] Building Debug...
cmake --build . --config Debug --verbose 

echo.
echo ====================================================
echo    Build finished successfully!
echo.
echo    Final output:
echo    UwU_Engine\bin\Debug-x64\Sandbox\Sandbox.exe
echo    UwU_Engine\bin\Debug-x64\Sandbox\UwU_Engine.dll
echo.
echo    Intermediate files are in:
echo    UwU_Engine\bin-int\Debug-x64\
echo ====================================================

pause
