@echo off
setlocal

set SKSE64_ROOT=%~dp0\..\dep\skse64

echo Unpack and merge SKSE64...

"%~dp0\7za.exe" -aos x "%SKSE64_ROOT%\skse64.7z" -o"%SKSE64_ROOT%"
if errorlevel 1 (
    echo Unable to unpack Boost, exiting...
    exit /b 1
)

