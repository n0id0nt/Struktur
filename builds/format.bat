@echo off
setlocal enabledelayedexpansion

rem Formats all .cpp/.h/.hpp files under src/ using the repo's .clang-format config.
rem Usage:
rem   format.bat            - format files in place
rem   format.bat --check    - report files that aren't correctly formatted, don't modify anything (exit code 1 if any)

set "SCRIPT_DIR=%~dp0"
rem %%~fI resolves the ".." away to a clean absolute path - needed so EXCLUDE_DIR below is a plain substring of
rem the fully-resolved paths the for /r loop produces, which never contain "..".
for %%I in ("%SCRIPT_DIR%..") do set "ROOT_DIR=%%~fI"
set "SRC_DIR=%ROOT_DIR%\src"
set "EXCLUDE_DIR=%SRC_DIR%\WrenBindings\Bindings\Generated"

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
for /r "%SRC_DIR%" %%F in (*.cpp *.h *.hpp) do (
    set "FILE=%%F"
    echo !FILE! | findstr /I /C:"%EXCLUDE_DIR%" >nul
    if errorlevel 1 (
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
