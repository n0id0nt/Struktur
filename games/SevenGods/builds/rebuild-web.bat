@echo off
cd /d "%~dp0.."
call %EMSDK%\emsdk_env
where emcc >nul 2>nul
if %errorlevel% neq 0 (
    echo Emscripten not found! Please install and activate the Emscripten SDK.
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
