@echo off
setlocal EnableExtensions
rem Release ue.lock, or clear a dead agent id.
rem
rem Usage:
rem   tools\free-ue.bat
rem   tools\free-ue.bat <id>
rem
rem With an id, write Free only when the word is that id.
rem With no id, clear a dead agent id when UnrealEditor.exe,
rem UnrealEditor-Cmd.exe, and UnrealBuildTool.exe are not running.
rem Free, PIE, Editor, and Dump are left unchanged.

set "SCRIPT_DIR=%~dp0"
set "LOCKPY=%SCRIPT_DIR%ue_lock.py"
if "%~1"=="" (
  python "%LOCKPY%" clear-dead
) else (
  python "%LOCKPY%" release "%~1"
)
exit /b %ERRORLEVEL%
