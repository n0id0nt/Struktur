@echo off
cd /d "%~dp0.."
cmake --build build-windows-debug --config Debug --parallel
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)
echo.
echo Executable: build-windows-debug\Debug\SevenGods.exe
echo.
rem Run from the game folder: the editor keeps its layout files in the folder it starts in.
build-windows-debug\Debug\SevenGods.exe
pause
