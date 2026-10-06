@echo off
title Voodoo Vince Free Camera - Uninstall
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0files\setup.ps1" -Uninstall %*
echo.
pause
