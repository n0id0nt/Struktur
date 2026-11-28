@echo off
cd /d "%~dp0.."
for %%d in (build-windows build-windows-debug build-web build-nogame build-tests) do (
    if exist "%%d" (
        rmdir /s /q "%%d"
        echo Removed %%d
    )
)
pause
