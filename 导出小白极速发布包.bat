@echo off
setlocal
cd /d "%~dp0"
chcp 65001 >nul 2>&1

powershell -NoProfile -ExecutionPolicy Bypass -File "tools\package_release_easypack.ps1"

echo.
echo ======================================================================
echo Press any key to exit / 请按任意键退出 . . .
pause >nul
