@echo off
cd /d "%~dp0.."
rem Build for desktop once first: the web build embeds the shaders that compiles.
call %EMSDK%\emsdk_env
where emcc >nul 2>nul
if %errorlevel% neq 0 (
    echo Emscripten not found! Please install and activate the Emscripten SDK.
    pause
    exit /b 1
)
call emcmake cmake -S . -B build-web -G Ninja -DPLATFORM=Web -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_TOOLCHAIN_FILE=%EMSDK%/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)
call cmake --build build-web
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)
start "" http://localhost:8000/SevenGods.html
python -m http.server 8000 --directory build-web
pause
