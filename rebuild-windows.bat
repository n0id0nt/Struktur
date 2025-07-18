@echo off
echo Rebuilding for Windows Desktop...

if not exist "build-windows" mkdir build-windows
cd build-windows

cmake --build . --config Release
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
echo Executable: build-windows\Release\Struktur.exe
echo.

Release\Struktur.exe

cd ..
pause