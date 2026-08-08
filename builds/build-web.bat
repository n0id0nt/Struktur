@echo off
cd /d "%~dp0.."
echo Building for Web (WASM)...

call %EMSDK%\emsdk_env
if %errorlevel% neq 0 (
    echo Warning: Failed to activate Emscripten environment
)

where emcc >nul 2>nul
if %errorlevel% neq 0 (
    echo Emscripten not found! Please install and activate Emscripten SDK.
    pause
    exit /b 1
)

:: ============================================================
:: Select project assets folder via dialog
:: ============================================================
echo Selecting assets folder...
for /f "delims=" %%i in ('powershell -Command "Add-Type -AssemblyName System.Windows.Forms; $f = New-Object System.Windows.Forms.FolderBrowserDialog; $f.Description = 'Select Assets Folder'; $f.SelectedPath = (Get-Location).Path; if ($f.ShowDialog() -eq 'OK') { $f.SelectedPath }"') do set ASSETS_FOLDER=%%i

if "%ASSETS_FOLDER%"=="" (
    echo No folder selected, using default assets folder...
    set ASSETS_FOLDER=%CD%\assets
)

echo Assets folder: %ASSETS_FOLDER%
echo.

:: ============================================================
:: Select shell.html via dialog
:: ============================================================
echo Selecting shell.html...
for /f "delims=" %%i in ('powershell -Command "Add-Type -AssemblyName System.Windows.Forms; $f = New-Object System.Windows.Forms.OpenFileDialog; $f.Title = 'Select Emscripten Shell HTML'; $f.Filter = 'HTML files (*.html)|*.html'; $f.InitialDirectory = (Get-Location).Path; if ($f.ShowDialog() -eq 'OK') { $f.FileName }"') do set SHELL_FILE=%%i

if "%SHELL_FILE%"=="" (
    echo No shell file selected, using default web/shell.html...
    set SHELL_FILE=%CD%\web\shell.html
)

echo Shell file: %SHELL_FILE%
echo.

:: ============================================================
:: Build
:: ============================================================
if not exist "build-web" mkdir build-web
cd build-web

echo Create project
call emcmake cmake .. ^
    -DPLATFORM=Web ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_TOOLCHAIN_FILE=%EMSDK%/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake ^
    -DPROJECT_ASSETS_FOLDER="%ASSETS_FOLDER%" ^
    -DWEB_SHELL_FILE="%SHELL_FILE%" ^
    -G Ninja
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

echo Build project
call cmake --build .
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
echo.

start "" http://www.localhost:8000/Struktur.html
call python -m http.server 8000

cd ..
pause