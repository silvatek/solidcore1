@echo off
setlocal EnableExtensions EnableDelayedExpansion
rem Check whether Fab assets this project expects are on disk.
rem Recorded paths in tools\fab-assets.json are tried first.
rem A miss falls back to a search, and a find is written back.
rem
rem Usage:
rem   tools\fab_doctor.bat

set "SCRIPT_DIR=%~dp0"
set "ROOT=%SCRIPT_DIR%.."

echo SolidCore1 Fab doctor
echo Project: %ROOT%
echo.
echo Listing IDs (copy/paste)
powershell -NoProfile -Command "$j = Get-Content -Raw -LiteralPath '%SCRIPT_DIR%fab-assets.json' | ConvertFrom-Json; foreach ($a in $j.assets) { if ($a.listingUuid) { $a.listingUuid } }"
if errorlevel 1 (
  echo ca4ba583-8d90-4069-b51f-50e694530b2f
  echo 94bfee39-8d7d-409c-89c9-40433550ee3a
)
echo.

set "MISSING=0"

echo [required] Viking  (recorded path, else Content\Viking or Content\Fab)
set "VIKING_MESH="
set "VIKING_HOW=search"
call :GetField viking SK_Viking.uasset
if defined FIELD_VALUE if exist "%ROOT%\!FIELD_VALUE:/=\!" (
  set "VIKING_MESH=%ROOT%\!FIELD_VALUE:/=\!"
  set "VIKING_HOW=recorded"
)
if not defined VIKING_MESH (
  if exist "%ROOT%\Content\Viking\Mesh\SK_Viking.uasset" set "VIKING_MESH=%ROOT%\Content\Viking\Mesh\SK_Viking.uasset"
)
if not defined VIKING_MESH (
  for /r "%ROOT%\Content\Fab" %%F in (SK_Viking.uasset) do (
    if exist "%%F" set "VIKING_MESH=%%F"
  )
)
if defined VIKING_MESH (
  echo   OK  !VIKING_MESH:%ROOT%\=!  (!VIKING_HOW!)
  call :RememberFile viking SK_Viking.uasset "!VIKING_MESH!"
) else (
  echo   MISSING  SK_Viking.uasset
  set "MISSING=1"
)

for %%A in (idle1 walk run jump) do (
  set "CLIP="
  set "CLIP_HOW=search"
  call :GetField viking "Anim_Viking_%%A.uasset"
  if defined FIELD_VALUE if exist "%ROOT%\!FIELD_VALUE:/=\!" (
    set "CLIP=%ROOT%\!FIELD_VALUE:/=\!"
    set "CLIP_HOW=recorded"
  )
  if not defined CLIP (
    if exist "%ROOT%\Content\Viking\Animations\Anim_Viking_%%A.uasset" set "CLIP=%ROOT%\Content\Viking\Animations\Anim_Viking_%%A.uasset"
  )
  if not defined CLIP (
    for /r "%ROOT%\Content\Fab" %%F in (Anim_Viking_%%A.uasset) do (
      if exist "%%F" set "CLIP=%%F"
    )
  )
  if defined CLIP (
    echo   OK  !CLIP:%ROOT%\=!  (!CLIP_HOW!)
    call :RememberFile viking "Anim_Viking_%%A.uasset" "!CLIP!"
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
set "GRASS_HOW=search"
call :GetField grass resolvedPath
if defined FIELD_VALUE if exist "%ROOT%\!FIELD_VALUE:/=\!" (
  set "GRASS=%ROOT%\!FIELD_VALUE:/=\!"
  set "GRASS_HOW=recorded"
)
if not defined GRASS (
  call :GetField grass resolvedFolder
  if defined FIELD_VALUE if exist "%ROOT%\!FIELD_VALUE:/=\!\" (
    call :FindGrassIn "%ROOT%\!FIELD_VALUE:/=\!"
    if defined FOUND_GRASS (
      set "GRASS=!FOUND_GRASS!"
      set "GRASS_HOW=recorded folder"
    )
  )
)
if not defined GRASS (
  if exist "%ROOT%\Content\Fab" (
    call :FindGrassIn "%ROOT%\Content\Fab"
    if defined FOUND_GRASS set "GRASS=!FOUND_GRASS!"
  )
)
if defined GRASS (
  echo   OK  !GRASS:%ROOT%\=!  (!GRASS_HOW!)
  call :RememberFile grass resolvedPath "!GRASS!"
  for %%P in ("!GRASS!") do call :RememberFile grass resolvedFolder "%%~dpP"
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
echo   No folder moves. Recorded paths in tools\fab-assets.json are tried first.
echo   A missing recorded path falls back to a search, and a find is written back.
echo   Manifest: tools\fab-assets.json
echo.

if "%MISSING%"=="1" (
  echo RESULT: required Fab assets are missing.
  exit /b 1
)
echo RESULT: required Fab assets are present.
exit /b 0

:GetField
set "FIELD_VALUE="
set "FIELD_TMP=%TEMP%\sc1-fab-get.txt"
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%fab_manifest.ps1" -Action get -Id "%~1" -Field "%~2" -OutFile "%FIELD_TMP%"
if errorlevel 1 (
  echo   manifest read failed for %~1 %~2
  exit /b 0
)
if exist "%FIELD_TMP%" set /p FIELD_VALUE=<"%FIELD_TMP%"
exit /b 0

:RememberFile
call :ProjectRelative "%~3"
call :GetField "%~1" "%~2"
if /i "!REL_PATH!"=="!FIELD_VALUE!" exit /b 0
call :PutField "%~1" "%~2" "!REL_PATH!"
if errorlevel 1 (
  echo   manifest update failed for %~1 %~2
  exit /b 0
)
echo   recorded !REL_PATH!
exit /b 0

:PutField
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%fab_manifest.ps1" -Action set -Id "%~1" -Field "%~2" -Value "%~3"
exit /b !ERRORLEVEL!

:ProjectRelative
set "REL_PATH=%~f1"
for %%I in ("%ROOT%") do set "ROOT_ABS=%%~fI"
set "REL_PATH=!REL_PATH:%ROOT_ABS%\=!"
set "REL_PATH=!REL_PATH:\=/!"
if "!REL_PATH:~0,1!"=="/" set "REL_PATH=!REL_PATH:~1!"
if "!REL_PATH:~-1!"=="/" set "REL_PATH=!REL_PATH:~0,-1!"
exit /b 0

:FirstSk
set "FOUND_SK="
if not exist "%~1\" exit /b 0
pushd "%~1"
for /r %%F in (SK_*.uasset) do (
  if not defined FOUND_SK set "FOUND_SK=%%F"
)
popd
exit /b 0

:FindGrassIn
set "FOUND_GRASS="
if not exist "%~1\" exit /b 0
for /r "%~1" %%F in (Mat_025_grass.uasset) do (
  echo %%F | findstr /i /c:"StaticMeshes" >nul
  if errorlevel 1 if not defined FOUND_GRASS set "FOUND_GRASS=%%F"
)
exit /b 0
