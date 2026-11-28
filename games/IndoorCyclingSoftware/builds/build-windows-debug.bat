@echo off
cd /d "%~dp0.."
echo Building Indoor Cycling Software (Debug, with the editor)...
cmake -S . -B build-windows-debug -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)
cmake --build build-windows-debug --config Debug --parallel
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)
echo.
echo Executable: build-windows-debug\Debug\IndoorCyclingSoftware.exe
echo.
rem Run from the game folder: the editor keeps its layout files in the folder it starts in.
build-windows-debug\Debug\IndoorCyclingSoftware.exe
pause
