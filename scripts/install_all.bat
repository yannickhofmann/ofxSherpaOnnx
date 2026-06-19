@echo off
setlocal
set SCRIPT_DIR=%~dp0
py -3 "%SCRIPT_DIR%install_all.py" %*
if errorlevel 1 exit /b %errorlevel%
