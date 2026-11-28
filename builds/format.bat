@echo off
setlocal enabledelayedexpansion

rem Formats the engine's and the games' .cpp/.h/.hpp files using the repo's .clang-format config.
rem Usage:
rem   format.bat            - format files in place
rem   format.bat --check    - report files that aren't correctly formatted, don't modify anything (exit code 1 if any)

set "SCRIPT_DIR=%~dp0"
rem %%~fI resolves the ".." away to a clean absolute path.
for %%I in ("%SCRIPT_DIR%..") do set "ROOT_DIR=%%~fI"

rem Prefer clang-format on PATH; fall back to the copy bundled with VS2022's LLVM tools.
set "CLANG_FORMAT=clang-format"
where clang-format >nul 2>nul
if errorlevel 1 (
    set "CLANG_FORMAT=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-format.exe"
    if not exist "!CLANG_FORMAT!" (
        echo clang-format not found on PATH or at the expected VS2022 install location.
        echo Install LLVM/clang-format, or edit CLANG_FORMAT at the top of this script.
        exit /b 1
    )
)

set "MODE=-i"
set "CHECKING=0"
if /I "%~1"=="--check" (
    set "MODE=--dry-run --Werror"
    set "CHECKING=1"
)
if /I "%~1"=="-check" (
    set "MODE=--dry-run --Werror"
    set "CHECKING=1"
)

echo Using "!CLANG_FORMAT!"
echo.

set "FAIL=0"
for %%D in ("%ROOT_DIR%\src" "%ROOT_DIR%\tests" "%ROOT_DIR%\games\IndoorCyclingSoftware\src" "%ROOT_DIR%\games\IndoorCyclingSoftware\tests" "%ROOT_DIR%\games\SevenGods\src") do (
    for /r "%%~D" %%F in (*.cpp *.h *.hpp) do (
        "!CLANG_FORMAT!" %MODE% "%%F"
        if errorlevel 1 set "FAIL=1"
    )
)

echo.
if "!CHECKING!"=="1" (
    if "!FAIL!"=="1" (
        echo One or more files are not correctly formatted. Run format.bat without --check to fix them.
        exit /b 1
    )
    echo All files are correctly formatted.
) else (
    echo Formatting complete.
)

endlocal
