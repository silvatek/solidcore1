@echo off
setlocal EnableExtensions
rem Reserve the next build and test, or clear that reservation.
rem
rem Usage:
rem   tools\yield-ue.bat
rem   tools\yield-ue.bat clear
rem
rem tools\pie.bat, tools\editor.bat, and tools\dump-chart.bat ignore this.
rem A build or test that has already started is left alone.

set "SCRIPT_DIR=%~dp0"
set "LOCKPY=%SCRIPT_DIR%ue_lock.py"
if /I "%~1"=="clear" (
  python "%LOCKPY%" yield-clear
) else (
  python "%LOCKPY%" yield
)
exit /b %ERRORLEVEL%
