@echo off
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "tools\build.ps1"
echo.
echo [build.bat] exit code = %errorlevel%
echo.
pause
