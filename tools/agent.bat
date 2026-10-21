@echo off
setlocal EnableExtensions
rem Start the solidcore1-ue worker. When it exits 0, write agent.ok in the repo root.
rem
rem Usage:
rem   tools\agent.bat
rem
rem The marker is gitignored and stays on this machine. The line in the file is
rem Passed. A non-zero exit leaves any existing marker unchanged.

set "SCRIPT_DIR=%~dp0"
set "ROOT=%SCRIPT_DIR%.."
for %%I in ("%ROOT%") do set "ROOT=%%~fI"
set "MARKER=%ROOT%\agent.ok"

call agent worker start --name "solidcore1-ue" --worker-dir Dev\solidcore1
set "ERR=%ERRORLEVEL%"
if not "%ERR%"=="0" exit /b %ERR%

> "%MARKER%" echo Passed
echo Marker: %MARKER%
exit /b 0
