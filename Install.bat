@echo off
title Voodoo Vince Free Camera - Install
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0files\setup.ps1" %*
echo.
pause
