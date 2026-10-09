@echo off
setlocal
rem Run SolidCore1 automation tests headlessly (UE 5.8).
rem
rem Usage:
rem   tools\run_automation_tests.bat
rem   tools\run_automation_tests.bat SolidCore1.Fog
rem
rem Optional env:
rem   UE_ROOT   Engine root (default: C:\Program Files\Epic Games\UE_5.8)
rem   PROJECT   Path to .uproject (default: repo SolidCore1.uproject)

if not defined UE_ROOT set "UE_ROOT=C:\Program Files\Epic Games\UE_5.8"

set "SCRIPT_DIR=%~dp0"
if not defined PROJECT set "PROJECT=%SCRIPT_DIR%..\SolidCore1.uproject"

set "FILTER=%~1"
if "%FILTER%"=="" set "FILTER=SolidCore1"

set "EDITOR=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
if not exist "%EDITOR%" (
  echo ERROR: UnrealEditor-Cmd not found at:
  echo   %EDITOR%
  echo Set UE_ROOT to your engine install and retry.
  exit /b 1
)

echo Running automation filter: %FILTER%
echo Project: %PROJECT%
"%EDITOR%" "%PROJECT%" -NullRHI -unattended -nop4 -nosound -nosplash -ExecCmds="Automation RunTests %FILTER%; Quit" -Log
set "ERR=%ERRORLEVEL%"
echo Exit code: %ERR%
exit /b %ERR%
