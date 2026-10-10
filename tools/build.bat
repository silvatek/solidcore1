@echo off
setlocal EnableExtensions
rem Build SolidCore1Editor Win64 Development with Unreal Build Tool.
rem
rem Usage:
rem   tools\build.bat
rem
rem Optional env:
rem   UE_ROOT   Engine root (default: C:\Program Files\Epic Games\UE_5.8)
rem   PROJECT   Path to .uproject (default: this repo's SolidCore1.uproject)

if not defined UE_ROOT set "UE_ROOT=C:\Program Files\Epic Games\UE_5.8"

set "SCRIPT_DIR=%~dp0"
if not defined PROJECT set "PROJECT=%SCRIPT_DIR%..\SolidCore1.uproject"
for %%I in ("%PROJECT%") do set "PROJECT=%%~fI"

set "UBT=%UE_ROOT%\Engine\Build\BatchFiles\Build.bat"
if not exist "%UBT%" (
  echo ERROR: Unreal Build Tool not found at:
  echo   %UBT%
  echo Set UE_ROOT to your engine install and retry.
  exit /b 1
)
if not exist "%PROJECT%" (
  echo ERROR: Project not found at:
  echo   %PROJECT%
  echo Set PROJECT to your SolidCore1.uproject and retry.
  exit /b 1
)

echo Building SolidCore1Editor Win64 Development
echo Project: %PROJECT%
echo Engine:  %UE_ROOT%
echo.

call "%UBT%" SolidCore1Editor Win64 Development "-Project=%PROJECT%" -WaitMutex
exit /b %ERRORLEVEL%
