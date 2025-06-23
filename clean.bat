@echo off
echo Cleaning build directories...

if exist "build-windows" (
    rmdir /s /q "build-windows"
    echo Removed build-windows directory
)

if exist "build-web" (
    rmdir /s /q "build-web"
    echo Removed build-web directory
)

echo.
echo Clean completed!
pause