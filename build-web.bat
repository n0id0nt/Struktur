@echo off
echo Building for Web (WASM)...

call %EMSDK%\emsdk_env
if %errorlevel% neq 0 (
    echo Warning: Failed to activate Emscripten environment
    echo Continuing anyway...
)

:: Check if Emscripten is available
where emcc >nul 2>nul
if %errorlevel% neq 0 (
    echo Emscripten not found! Please install and activate Emscripten SDK.
    echo Visit: https://emscripten.org/docs/getting_started/downloads.html
    pause
    exit /b 1
)

if not exist "build-web" mkdir build-web
cd build-web

echo.
echo Create project
echo.
call emcmake cmake .. -DPLATFORM=Web -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=%EMSDK%/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake -G Ninja
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo Build project
echo.
call cmake --build .
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
echo Web files: build-web\RayLibGame.html
echo Running a local web server in build-web directory
echo Example: python -m http.server 8000
echo.

start "" http://www.localhost:8000/Struktur.html
call python -m http.server 8000

cd ..

pause