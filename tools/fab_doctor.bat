@echo off
setlocal EnableExtensions EnableDelayedExpansion
rem Check whether Fab assets this project expects are on disk.
rem C++ finds Viking/grass by asset name under Content\Viking or Content\Fab —
rem no manual folder move after Launcher Add to Project.
rem
rem Usage:
rem   tools\fab_doctor.bat

set "SCRIPT_DIR=%~dp0"
set "ROOT=%SCRIPT_DIR%.."

echo SolidCore1 Fab doctor
echo Project: %ROOT%
echo.
echo Listing IDs (copy/paste)
echo ca4ba583-8d90-4069-b51f-50e694530b2f
echo 94bfee39-8d7d-409c-89c9-40433550ee3a
echo.

set "MISSING=0"

echo [required] Viking  (Content\Viking or Content\Fab, name SK_Viking)
set "VIKING_MESH="
if exist "%ROOT%\Content\Viking\Mesh\SK_Viking.uasset" set "VIKING_MESH=%ROOT%\Content\Viking\Mesh\SK_Viking.uasset"
if not defined VIKING_MESH (
  for /r "%ROOT%\Content\Fab" %%F in (SK_Viking.uasset) do (
    if exist "%%F" set "VIKING_MESH=%%F"
  )
)
if defined VIKING_MESH (
  echo   OK  !VIKING_MESH:%ROOT%\=!
) else (
  echo   MISSING  SK_Viking.uasset
  set "MISSING=1"
)

for %%A in (idle1 walk run jump) do (
  set "CLIP="
  if exist "%ROOT%\Content\Viking\Animations\Anim_Viking_%%A.uasset" set "CLIP=%ROOT%\Content\Viking\Animations\Anim_Viking_%%A.uasset"
  if not defined CLIP (
    for /r "%ROOT%\Content\Fab" %%F in (Anim_Viking_%%A.uasset) do (
      if exist "%%F" set "CLIP=%%F"
    )
  )
  if defined CLIP (
    echo   OK  !CLIP:%ROOT%\=!
  ) else (
    echo   MISSING  Anim_Viking_%%A.uasset
    set "MISSING=1"
  )
)
echo   uuid:    ca4ba583-8d90-4069-b51f-50e694530b2f
echo   listing: https://www.fab.com/listings/ca4ba583-8d90-4069-b51f-50e694530b2f
echo.

echo [required] Grass Mat_025_grass
set "GRASS="
if exist "%ROOT%\Content\Fab" (
  for /r "%ROOT%\Content\Fab" %%F in (Mat_025_grass.uasset) do (
    if exist "%%F" (
      echo %%F | findstr /i /c:"StaticMeshes" >nul
      if errorlevel 1 set "GRASS=%%F"
    )
  )
)
if defined GRASS (
  echo   OK  !GRASS:%ROOT%\=!
) else (
  echo   MISSING  Mat_025_grass.uasset
  set "MISSING=1"
)
echo   uuid:    94bfee39-8d7d-409c-89c9-40433550ee3a
echo   listing: https://www.fab.com/listings/94bfee39-8d7d-409c-89c9-40433550ee3a
echo.

echo Restore
echo   1. Epic Games Launcher -^> Unreal Engine -^> Fab. Sign in.
echo   2. Open each listing. Add to My Library if needed.
echo   3. Add to Project -^> this SolidCore1.uproject.
echo   No folder moves. Code searches /Game/Viking then /Game/Fab by asset name.
echo   Manifest: tools\fab-assets.json
echo.

if "%MISSING%"=="1" (
  echo RESULT: required Fab assets are missing.
  exit /b 1
)
echo RESULT: required Fab assets are present.
exit /b 0
