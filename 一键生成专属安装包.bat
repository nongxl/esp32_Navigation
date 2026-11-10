@echo off
setlocal
title esp32_Navigation Quick Signer
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\Quick-Sign.ps1" -Language zh
echo.
pause
