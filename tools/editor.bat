@echo off
setlocal EnableExtensions
rem Launch the SolidCore1 Unreal Editor and hold ue.lock until the editor exits.
rem
rem Launches only when ue.lock is missing or its word is Free. Then writes Editor,
rem starts Unreal Editor without Play In Editor, waits until UnrealEditor.exe has
rem exited, and writes Free. Any other word leaves the lock unchanged and does
rem not start the editor.
rem
rem Usage:
rem   tools\editor.bat
rem   tools\editor.bat -log
rem
rem Opens /Game/ThirdPerson/Lvl_ThirdPerson and switches to the World chart mode.
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

set "LOCKPY=%SCRIPT_DIR%ue_lock.py"

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

python "%LOCKPY%" acquire Editor --session
if errorlevel 1 exit /b 1

echo Launching the Unreal Editor
echo Project: %PROJECT%
echo Engine:  %UE_ROOT%
echo.

"%EDITOR%" "%PROJECT%" /Game/ThirdPerson/Lvl_ThirdPerson -SolidWorldChart %*
set "ERR=%ERRORLEVEL%"

echo Editor launch returned ^(%ERR%^). Waiting until UnrealEditor.exe has exited.

:WaitForEditor
tasklist /FI "IMAGENAME eq UnrealEditor.exe" /NH 2>nul | find /I "UnrealEditor.exe" >nul
if errorlevel 1 goto EditorClosed
ping -n 3 127.0.0.1 >nul
goto WaitForEditor

:EditorClosed
python "%LOCKPY%" release Editor
if errorlevel 1 if "%ERR%"=="0" set "ERR=1"
exit /b %ERR%
