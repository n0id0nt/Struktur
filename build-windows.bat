@echo off
echo Building for Windows Desktop...

if not exist "build-windows" mkdir build-windows
cd build-windows

cmake .. -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

cmake --build . --config Release
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
echo.

cmake --install . --prefix ./Dist --config Release
if %errorlevel% neq 0 (
    echo Install failed!
    pause
    exit /b 1
)

echo.
echo Executable: build-windows\Dist\Struktur.exe
echo.

Dist\Struktur.exe

cd ..
pause