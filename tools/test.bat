@echo off
setlocal EnableExtensions EnableDelayedExpansion
rem Run SolidCore1 automation tests headlessly (UE 5.8) and print pass/fail counts.
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

set "LOGDIR=%TEMP%\solidcore1-automation"
if not exist "%LOGDIR%" mkdir "%LOGDIR%"
set "LOGFILE=%LOGDIR%\automation.log"
set "FAILLIST=%LOGDIR%\failed-tests.txt"
if exist "%LOGFILE%" del /f /q "%LOGFILE%"
if exist "%FAILLIST%" del /f /q "%FAILLIST%"

echo Running automation filter: %FILTER%
echo Project: %PROJECT%
echo Log: %LOGFILE%
echo.

"%EDITOR%" "%PROJECT%" -NullRHI -unattended -nop4 -nosound -nosplash -ABSLOG="%LOGFILE%" -ExecCmds="Automation RunTests %FILTER%; Quit"
set "ERR=%ERRORLEVEL%"

set "PASSED=0"
set "FAILED=0"
set "RAN=0"

if exist "%LOGFILE%" (
  rem UE AutomationController lines look like:
  rem   Test Completed. Result={Success} Name={...} Path={...}
  rem   Test Completed. Result={Fail} Name={...} Path={...}
  for /f %%C in ('findstr /R /C:"Result={Success}" "%LOGFILE%" 2^>nul ^| find /C /V ""') do set "PASSED=%%C"
  for /f %%C in ('findstr /R /C:"Result={Fail}" "%LOGFILE%" 2^>nul ^| find /C /V ""') do set "FAILED=%%C"
  set /a RAN=PASSED+FAILED

  rem Robust Path/Name extraction via PowerShell (cmd *substitution mangles braces).
  if not "!FAILED!"=="0" (
    powershell -NoProfile -Command ^
      "$p='%LOGFILE%';" ^
      "Get-Content -LiteralPath $p | Where-Object { $_ -match 'Result=\{Fail\}' } | ForEach-Object {" ^
      "  if ($_ -match 'Path=\{([^}]+)\}') { $matches[1] }" ^
      "  elseif ($_ -match 'Name=\{([^}]+)\}') { $matches[1] }" ^
      "  else { $_ }" ^
      "} | ForEach-Object { $_.Trim() } | Where-Object { $_ } | Set-Content -LiteralPath '%FAILLIST%'"
  )
) else (
  echo WARNING: automation log not found at "%LOGFILE%"
)

echo.
echo ===== SolidCore1 automation summary =====
echo Filter : %FILTER%
echo Ran    : !RAN!
echo Passed : !PASSED!
echo Failed : !FAILED!
echo Exit   : %ERR%
echo Log    : %LOGFILE%
if exist "%FAILLIST%" (
  echo.
  echo Failed tests:
  for /f "usebackq delims=" %%F in ("%FAILLIST%") do echo   - %%F
)
echo =========================================

if not "!FAILED!"=="0" if "%ERR%"=="0" set "ERR=1"

exit /b %ERR%
