@echo off
setlocal EnableExtensions EnableDelayedExpansion
rem Check whether Fab assets this project expects are on disk, and print
rem how to restore them from Fab in the Epic Games Launcher.
rem
rem Usage:
rem   tools\fab_doctor.bat

set "SCRIPT_DIR=%~dp0"
set "ROOT=%SCRIPT_DIR%.."

echo SolidCore1 Fab doctor
echo Project: %ROOT%
echo.

set "MISSING=0"

echo [required] Viking
if exist "%ROOT%\Content\Viking\Mesh\SK_Viking.uasset" (
  echo   OK  Content\Viking\Mesh\SK_Viking.uasset
) else (
  echo   MISSING  Content\Viking\Mesh\SK_Viking.uasset
  set "MISSING=1"
)
for %%A in (idle1 walk run jump) do (
  if exist "%ROOT%\Content\Viking\Animations\Anim_Viking_%%A.uasset" (
    echo   OK  Content\Viking\Animations\Anim_Viking_%%A.uasset
  ) else (
    echo   MISSING  Content\Viking\Animations\Anim_Viking_%%A.uasset
    set "MISSING=1"
  )
)
echo   listing: https://www.fab.com/listings/ca4ba583-8d90-4069-b51f-50e694530b2f
echo.

echo [optional] Grass Mat_025_grass
set "GRASS="
for /r "%ROOT%\Content\Fab" %%F in (Mat_025_grass.uasset) do (
  if exist "%%F" set "GRASS=%%F"
)
if defined GRASS (
  echo   OK  !GRASS:%ROOT%\=!
) else (
  echo   missing  Content\Fab\...\Mat_025_grass.uasset
  echo   (terrain falls back to FlatCol)
)
echo   listing: https://www.fab.com/listings/94bfee39-8d7d-409c-89c9-40433550ee3a
echo.

echo Restore via Fab in Launcher
echo   1. Epic Games Launcher -^> Unreal Engine -^> Fab. Sign in.
echo   2. Gear icon: default format = Unreal Engine, export target = Unreal Engine 5.8.
echo   3. Open each listing above. Add to My Library if needed.
echo   4. Native UE packs do NOT batch-download or batch-export.
echo      Per listing: Add to Project -^> this SolidCore1.uproject.
echo   5. If Viking lands under Content\Fab\, move it to Content\Viking\
echo      so /Game/Viking/Mesh/SK_Viking resolves.
echo   Docs: https://dev.epicgames.com/documentation/en-us/fab/exporting-assets-from-fab-in-launcher
echo   Manifest: tools\fab-assets.json
echo.

if "%MISSING%"=="1" (
  echo RESULT: required Viking assets are missing.
  exit /b 1
)
echo RESULT: required Viking assets are present.
exit /b 0
