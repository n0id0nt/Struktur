@echo off
echo Rebuilding for Windows Desktop...

if not exist "build-windows-debug" mkdir build-windows-debug
cd build-windows-debug

cmake --build . --config Debug
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
echo Executable: build-windows\Debug\Struktur.exe
echo.

Debug\Struktur.exe

cd ..
pause