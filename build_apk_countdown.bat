@echo off
setlocal
title esp32_Navigation Countdown
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Build-Countdown.ps1"
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ======================================================================
    echo  [Build Failed] ExitCode: %ERRORLEVEL%
    echo ======================================================================
)
echo.
pause