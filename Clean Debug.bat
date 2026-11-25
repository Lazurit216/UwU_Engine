@echo off
setlocal
pushd "%~dp0" || exit /b 1

echo ====================================================
echo    UwU Engine - Cleaning Debug
echo ====================================================

set "CLEAN_FAILED=0"
for %%D in ("build\Debug" "build\Debug-Tracy" "build\Release" "build\Release-Tracy" "build" "build\tracy") do (
    call :clean_folder "%%~D"
    if errorlevel 1 set "CLEAN_FAILED=1"
)

echo.
if "%CLEAN_FAILED%"=="0" (
    echo    Clean finished.
) else (
    echo    Clean failed. See the errors above.
)
popd
pause
exit /b %CLEAN_FAILED%

:clean_folder
if not exist "%~1\CMakeCache.txt" exit /b 0
echo Cleaning Debug in "%~1"...
cmake --build "%~1" --config Debug --target clean
exit /b %ERRORLEVEL%
