@echo off
setlocal
title esp32_Navigation Export QuickPack
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\package_release_easypack.ps1"
echo.
pause
