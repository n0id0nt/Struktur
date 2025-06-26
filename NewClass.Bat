@echo off
setlocal enabledelayedexpansion

if "%~1"=="" (
    echo Usage: new-class.bat ClassName [namespace]
    echo Example: new-class.bat Player
    echo Example: new-class.bat Enemy Game
    pause
    exit /b 1
)

set "CLASS_NAME=%~1"
set "NAMESPACE=%~2"

:: Convert class name to lowercase for filename
set "FILENAME="
for /l %%i in (0,1,100) do (
    call set "char=%%CLASS_NAME:~%%i,1%%"
    if "!char!"=="" goto :done_filename
    
    :: Check if character is uppercase
    echo !char!| findstr /r "^[A-Z]$" >nul
    if !errorlevel! equ 0 (
        :: Convert to lowercase and add underscore if not first character
        if not "!FILENAME!"=="" set "FILENAME=!FILENAME!_"
        call :tolower !char! lowercase_char
        set "FILENAME=!FILENAME!!lowercase_char!"
    ) else (
        set "FILENAME=!FILENAME!!char!"
    )
)

:done_filename
set "HEADER_FILE=src\!FILENAME!.h"
set "SOURCE_FILE=src\!FILENAME!.cpp"

echo Creating new C++ class: %CLASS_NAME%
echo Header file: %HEADER_FILE%
echo Source file: %SOURCE_FILE%

:: Create header file
echo Creating header file...
(
echo #pragma once
echo.
if not "%NAMESPACE%"=="" (
    echo namespace %NAMESPACE% {
    echo.
)
echo class %CLASS_NAME% {
echo public:
echo     %CLASS_NAME%^(^);
echo     ~%CLASS_NAME%^(^);
echo.
echo     void Update^(^);
echo     void Draw^(^);
echo.
echo private:
echo     // Add your member variables here
echo };
if not "%NAMESPACE%"=="" (
    echo.
    echo } // namespace %NAMESPACE%
)
) > "%HEADER_FILE%"

:: Create source file
echo Creating source file...
(
echo #include "%FILENAME%.h"
echo #include "raylib.h"
echo.
if not "%NAMESPACE%"=="" (
    echo using namespace %NAMESPACE%;
    echo.
)
echo %CLASS_NAME%::%CLASS_NAME%^(^) {
echo     // Constructor implementation
echo }
echo.
echo %CLASS_NAME%::~%CLASS_NAME%^(^) {
echo     // Destructor implementation
echo }
echo.
echo void %CLASS_NAME%::Update^(^) {
echo     // Update logic here
echo }
echo.
echo void %CLASS_NAME%::Draw^(^) {
echo     // Drawing logic here
echo }
) > "%SOURCE_FILE%"

:: Update CMakeLists.txt
echo Updating CMakeLists.txt...
set "TEMP_FILE=CMakeLists_temp.txt"
set "FOUND_SOURCES=0"

(
for /f "delims=" %%a in (CMakeLists.txt) do (
    set "line=%%a"
    echo !line!
    
    :: Check if this is the SOURCES section
    echo !line! | findstr /c:"set(SOURCES" >nul
    if !errorlevel! equ 0 set "FOUND_SOURCES=1"
    
    :: If we're in sources section and find the closing parenthesis, add our files
    if "!FOUND_SOURCES!"=="1" (
        echo !line! | findstr /c:")" >nul
        if !errorlevel! equ 0 (
            if not "!line!"=="set(SOURCES" (
                echo     src/!FILENAME!.h
                echo     src/!FILENAME!.cpp
                set "FOUND_SOURCES=0"
            )
        )
    )
)
) > "%TEMP_FILE%"

move "%TEMP_FILE%" CMakeLists.txt

echo.
echo ✅ Successfully created class %CLASS_NAME%!
echo.
echo Files created:
echo   - %HEADER_FILE%
echo   - %SOURCE_FILE%
echo.
echo CMakeLists.txt has been updated to include the new files.
echo.
echo Don't forget to:
echo 1. #include "%FILENAME%.h" in your main.cpp or other files
if not "%NAMESPACE%"=="" echo 2. Use %NAMESPACE%::%CLASS_NAME% to access the class
echo 3. Rebuild your project
echo.
pause
goto :eof

:tolower
set "%~2="
set "str=%~1"
for %%i in (A B C D E F G H I J K L M N O P Q R S T U V W X Y Z) do (
    call set "str=%%str:%%i=%%i%%"
)
for %%i in (a b c d e f g h i j k l m n o p q r s t u v w x y z) do (
    call set "str=%%str:%%i=%%i%%"
)
call set "str=%%str:A=a%%"
call set "str=%%str:B=b%%"
call set "str=%%str:C=c%%"
call set "str=%%str:D=d%%"
call set "str=%%str:E=e%%"
call set "str=%%str:F=f%%"
call set "str=%%str:G=g%%"
call set "str=%%str:H=h%%"
call set "str=%%str:I=i%%"
call set "str=%%str:J=j%%"
call set "str=%%str:K=k%%"
call set "str=%%str:L=l%%"
call set "str=%%str:M=m%%"
call set "str=%%str:N=n%%"
call set "str=%%str:O=o%%"
call set "str=%%str:P=p%%"
call set "str=%%str:Q=q%%"
call set "str=%%str:R=r%%"
call set "str=%%str:S=s%%"
call set "str=%%str:T=t%%"
call set "str=%%str:U=u%%"
call set "str=%%str:V=v%%"
call set "str=%%str:W=w%%"
call set "str=%%str:X=x%%"
call set "str=%%str:Y=y%%"
call set "str=%%str:Z=z%%"
set "%~2=%str%"
goto :eof
```
```batch
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