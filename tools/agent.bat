@echo off
setlocal EnableExtensions
rem Build, run the default automation suite, and write agent.ok when both succeed.
rem
rem Usage:
rem   tools\agent.bat
rem
rem Deletes agent.ok in the repo root first. A failing build or test leaves the
rem marker absent and returns that script's exit code. The marker is gitignored.
rem The first line is Passed. The second line is the commit, or unknown.

set "SCRIPT_DIR=%~dp0"
set "ROOT=%SCRIPT_DIR%.."
for %%I in ("%ROOT%") do set "ROOT=%%~fI"
set "MARKER=%ROOT%\agent.ok"

if exist "%MARKER%" del /f /q "%MARKER%"

call "%SCRIPT_DIR%build.bat"
set "ERR=%ERRORLEVEL%"
if not "%ERR%"=="0" exit /b %ERR%

call "%SCRIPT_DIR%test.bat"
set "ERR=%ERRORLEVEL%"
if not "%ERR%"=="0" exit /b %ERR%

set "SHA=unknown"
set "SHAFILE=%TEMP%\solidcore1-agent-sha.txt"
if exist "%SHAFILE%" del /f /q "%SHAFILE%"
git -C "%ROOT%" rev-parse HEAD > "%SHAFILE%" 2>nul
if exist "%SHAFILE%" (
  for /f "usebackq delims=" %%H in ("%SHAFILE%") do set "SHA=%%H"
  del /f /q "%SHAFILE%"
)
if not defined SHA set "SHA=unknown"

> "%MARKER%" echo Passed
>> "%MARKER%" echo %SHA%
echo Marker: %MARKER%
echo Commit: %SHA%
exit /b 0
