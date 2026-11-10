@echo off
setlocal
title esp32_Navigation
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Build-Personal.ps1"
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ======================================================================
    echo  [Build Failed] ExitCode: %ERRORLEVEL%
    echo ======================================================================
)
echo.
pause