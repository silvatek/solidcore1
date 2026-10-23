@echo off
setlocal EnableExtensions EnableDelayedExpansion
rem Write /Game/World/WorldChart as JSON.
rem
rem Usage:
rem   tools\dump-chart.bat
rem   tools\dump-chart.bat C:\temp\WorldChart.json
rem
rem The file lists the painted cells, then each mark and each road.
rem Row and column count from 1 at the top-left.
rem positionCm is the cell centre stored on the asset. groundCm is where the
rem World chart mode draws that mark, on the terrain centred at the origin.
rem
rem Launches only when ue.lock is missing or its word is Free. Then writes Dump,
rem runs the commandlet, and writes Free. Any other word leaves the lock unchanged
rem and does not start.
rem
rem Optional env:
rem   UE_ROOT   Engine root (default: C:\Program Files\Epic Games\UE_5.8)
rem   PROJECT   Path to .uproject (default: this repo's SolidCore1.uproject)

if not defined UE_ROOT set "UE_ROOT=C:\Program Files\Epic Games\UE_5.8"

set "SCRIPT_DIR=%~dp0"
if not defined PROJECT set "PROJECT=%SCRIPT_DIR%..\SolidCore1.uproject"
for %%I in ("%PROJECT%") do set "PROJECT=%%~fI"

set "LOCK=%SCRIPT_DIR%..\ue.lock"
for %%I in ("%LOCK%") do set "LOCK=%%~fI"

set "OUT=%~1"
if "%OUT%"=="" set "OUT=%SCRIPT_DIR%..\Saved\WorldChart.json"
for %%I in ("%OUT%") do set "OUT=%%~fI"

set "EDITOR=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
if not exist "%EDITOR%" (
  echo ERROR: UnrealEditor-Cmd not found at:
  echo   %EDITOR%
  echo Set UE_ROOT to your engine install and retry.
  exit /b 1
)
if not exist "%PROJECT%" (
  echo ERROR: Project not found at:
  echo   %PROJECT%
  echo Set PROJECT to your SolidCore1.uproject and retry.
  exit /b 1
)

call :LockIsFree
if errorlevel 1 exit /b 1

> "%LOCK%" echo Dump
echo Lock: %LOCK%
echo Word: Dump
echo Dumping the world chart
echo Project: %PROJECT%
echo Engine:  %UE_ROOT%
echo Out:     %OUT%
echo.

if exist "%OUT%" del /f /q "%OUT%"

"%EDITOR%" "%PROJECT%" -run=SolidDumpWorldChart -unattended -nop4 -nosplash -nosound -NullRHI -out="%OUT%"
set "ERR=%ERRORLEVEL%"

> "%LOCK%" echo Free
echo Word: Free
rem The editor process exits 1 when an unrelated startup line is logged as an error.
rem The dump itself succeeded when it wrote the file.
if exist "%OUT%" (
  echo Wrote %OUT%
  exit /b 0
)
echo Dump failed ^(%ERR%^).
exit /b 1

:LockIsFree
setlocal EnableExtensions EnableDelayedExpansion
if not exist "%LOCK%" exit /b 0
set "LOCKWORD="
for /f "usebackq tokens=*" %%W in ("%LOCK%") do (
  if not defined LOCKWORD set "LOCKWORD=%%W"
)
if not defined LOCKWORD goto LockBusy
:LockTrim
if "!LOCKWORD:~-1!"==" " set "LOCKWORD=!LOCKWORD:~0,-1!" & goto LockTrim
if "!LOCKWORD:~-1!"=="	" set "LOCKWORD=!LOCKWORD:~0,-1!" & goto LockTrim
if not defined LOCKWORD goto LockBusy
if "!LOCKWORD!"=="Free" exit /b 0
:LockBusy
echo ERROR: ue.lock is not Free. Not dumping the chart.
echo Lock: %LOCK%
echo Word: "!LOCKWORD!"
exit /b 1
