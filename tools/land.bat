@echo off
setlocal EnableExtensions
rem Land one buildable branch onto main.
rem
rem Usage:
rem   tools\land.bat
rem
rem SOLID_AGENT_ID is the lock holder. The script fetches even when the
rem lock word is PIE, Editor, or Dump, and it does not rebase, build, or
rem test while that word is set. ue.yield reserves the next landing.

if "%SOLID_AGENT_ID%"=="" (
  echo SOLID_AGENT_ID is not set.
  exit /b 1
)
set "PYTHONDONTWRITEBYTECODE=1"
python "%~dp0land.py"
exit /b %ERRORLEVEL%
