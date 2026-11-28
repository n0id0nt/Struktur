@echo off
cd /d "%~dp0.."
echo Building Seven Gods (Release)...
cmake -S . -B build-windows -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)
cmake --build build-windows --config Release --parallel
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)
cmake --install build-windows --prefix build-windows\Dist --config Release
if %errorlevel% neq 0 (
    echo Install failed!
    pause
    exit /b 1
)
echo Packing assets into data.pak...
if exist "build-windows\Dist\data.pak" del "build-windows\Dist\data.pak"
powershell -Command "Compress-Archive -Path 'assets\*' -DestinationPath 'build-windows\Dist\data.zip' -Force"
if %errorlevel% neq 0 (
    echo Asset packing failed!
    pause
    exit /b 1
)
rename "build-windows\Dist\data.zip" "data.pak"
echo.
echo Executable : build-windows\Dist\SevenGods.exe
echo Assets     : build-windows\Dist\data.pak
echo.
cd build-windows\Dist
.\SevenGods.exe
pause
