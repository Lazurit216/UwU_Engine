@echo off
echo ====================================================
echo    UwU Engine - Cleaning Build
echo ====================================================

if exist "build" (
    cd build
    echo Removing compiled files...
    cmake --build . --target clean --config Debug
    cd ..
) else (
    echo build folder not found
)

if exist "bin" (
    echo Removing bin directory...
    rmdir /s /q bin
)

if exist "bin-int" (
    echo Removing bin-int directory...
    rmdir /s /q bin-int
)

echo.
echo ====================================================
echo    Clean finished!
echo ====================================================

pause