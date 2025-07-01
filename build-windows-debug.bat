@echo off
echo Building for Windows Desktop...

if not exist "build-windows-debug" mkdir build-windows-debug
cd build-windows-debug

cmake .. -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

cmake --build . --config Debug
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
echo Executable: build-windows-debug\Debug\Struktur.exe
echo.

Debug\Struktur.exe

cd ..
pause