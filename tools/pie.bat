@echo off
setlocal EnableExtensions
rem Launch SolidCore1 in Play In Editor and hold ue.lock until the editor exits.
rem
rem Writes the word PIE into ue.lock at the repo root, starts Unreal Editor with
rem -immersive (the switch that opens the editor and starts PIE), waits until
rem UnrealEditor.exe has exited, then writes the word Free.
rem
rem Usage:
rem   tools\pie.bat
rem   tools\pie.bat -log
rem
rem Optional env:
rem   UE_ROOT   Engine root (default: C:\Program Files\Epic Games\UE_5.8)
rem   PROJECT   Path to .uproject (default: this repo's SolidCore1.uproject)
rem
rem Extra arguments are passed through to UnrealEditor.exe.

if not defined UE_ROOT set "UE_ROOT=C:\Program Files\Epic Games\UE_5.8"

set "SCRIPT_DIR=%~dp0"
if not defined PROJECT set "PROJECT=%SCRIPT_DIR%..\SolidCore1.uproject"
for %%I in ("%PROJECT%") do set "PROJECT=%%~fI"

set "LOCK=%SCRIPT_DIR%..\ue.lock"
for %%I in ("%LOCK%") do set "LOCK=%%~fI"

set "EDITOR=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe"
if not exist "%EDITOR%" (
  echo ERROR: UnrealEditor not found at:
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

(echo PIE) > "%LOCK%"
echo Lock: %LOCK%
echo Word: PIE
echo Launching Play In Editor ^(-immersive^)
echo Project: %PROJECT%
echo Engine:  %UE_ROOT%
echo.

"%EDITOR%" "%PROJECT%" -immersive %*
set "ERR=%ERRORLEVEL%"

echo Editor launch returned ^(%ERR%^). Waiting until UnrealEditor.exe has exited.

:WaitForEditor
tasklist /FI "IMAGENAME eq UnrealEditor.exe" /NH 2>nul | find /I "UnrealEditor.exe" >nul
if errorlevel 1 goto EditorClosed
ping -n 3 127.0.0.1 >nul
goto WaitForEditor

:EditorClosed
(echo Free) > "%LOCK%"
echo Word: Free
exit /b %ERR%
