@echo off
cd /d "%~dp0.."
echo Building for Windows Desktop...

:: ============================================================
:: Select project assets folder via dialog
:: ============================================================
echo Selecting assets folder...
for /f "delims=" %%i in ('powershell -Command "Add-Type -AssemblyName System.Windows.Forms; $f = New-Object System.Windows.Forms.FolderBrowserDialog; $f.Description = 'Select Assets Folder to Pack'; $f.SelectedPath = (Get-Location).Path; if ($f.ShowDialog() -eq 'OK') { $f.SelectedPath }"') do set ASSETS_FOLDER=%%i

if "%ASSETS_FOLDER%"=="" (
    echo No folder selected, using default assets folder...
    set ASSETS_FOLDER=%CD%\assets
)

echo Assets folder: %ASSETS_FOLDER%
echo.

:: ============================================================
:: Build
:: ============================================================
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

:: ============================================================
:: Pack assets into data.pak
:: ============================================================
echo Packing assets into data.pak...

:: Use PowerShell to zip the selected assets folder
powershell -Command "Compress-Archive -Path '%ASSETS_FOLDER%\*' -DestinationPath 'Dist\data.zip' -Force"
if %errorlevel% neq 0 (
    echo Asset packing failed!
    pause
    exit /b 1
)

:: Rename .zip to .pak
rename "Dist\data.zip" "data.pak"
if %errorlevel% neq 0 (
    echo Failed to rename pak file!
    pause
    exit /b 1
)

echo Assets packed successfully into game_data.pak
echo.
echo Executable : build-windows\Dist\Struktur.exe
echo Assets     : build-windows\Dist\data.pak
echo.

Dist\Struktur.exe

cd ..
pause