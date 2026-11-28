@echo off
cd /d "%~dp0.."
cmake --build build-windows --config Release --parallel
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)
echo.
echo Executable: build-windows\Release\SevenGods.exe (build-windows.bat packages it with its assets)
pause
