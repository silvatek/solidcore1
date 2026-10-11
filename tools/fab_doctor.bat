@echo off
setlocal EnableExtensions EnableDelayedExpansion
rem Check whether Fab assets this project expects are on disk.
rem C++ finds Viking/grass by asset name under Content\Viking or Content\Fab.
rem Companion meshes are SK_ assets inside Medieval_Assassin_Girl and
rem Elf_Blacksmith anywhere under Content. No folder move after Add to Project.
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
  echo 2986b290-90ed-4a0e-a891-cacf442033f9
  echo d01abf8c-9798-4bcb-8617-2d1d560d318a
)
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

echo [required] Assassin companion mesh  (folder Medieval_Assassin_Girl)
set "ASSASSIN_DIR="
for /f "delims=" %%D in ('dir /b /s /ad "%ROOT%\Content" 2^>nul ^| findstr /i /l /e /c:"\Medieval_Assassin_Girl"') do (
  if not defined ASSASSIN_DIR set "ASSASSIN_DIR=%%D"
)
set "ASSASSIN="
if defined ASSASSIN_DIR (
  pushd "!ASSASSIN_DIR!"
  for /r %%F in (SK_*.uasset) do (
    if not defined ASSASSIN set "ASSASSIN=%%F"
  )
  popd
)
if defined ASSASSIN (
  echo   OK  !ASSASSIN:%ROOT%\=!
) else if defined ASSASSIN_DIR (
  echo   MISSING  SK_*.uasset in !ASSASSIN_DIR:%ROOT%\=!
  set "MISSING=1"
) else (
  echo   MISSING  Content folder Medieval_Assassin_Girl
  set "MISSING=1"
)
echo   uuid:    2986b290-90ed-4a0e-a891-cacf442033f9
echo   listing: https://www.fab.com/listings/2986b290-90ed-4a0e-a891-cacf442033f9
echo.

echo [required] Blacksmith companion mesh  (folder Elf_Blacksmith)
set "BLACKSMITH_DIR="
for /f "delims=" %%D in ('dir /b /s /ad "%ROOT%\Content" 2^>nul ^| findstr /i /l /e /c:"\Elf_Blacksmith"') do (
  if not defined BLACKSMITH_DIR set "BLACKSMITH_DIR=%%D"
)
set "BLACKSMITH="
if defined BLACKSMITH_DIR (
  pushd "!BLACKSMITH_DIR!"
  for /r %%F in (SK_*.uasset) do (
    if not defined BLACKSMITH set "BLACKSMITH=%%F"
  )
  popd
)
if defined BLACKSMITH (
  echo   OK  !BLACKSMITH:%ROOT%\=!
) else if defined BLACKSMITH_DIR (
  echo   MISSING  SK_*.uasset in !BLACKSMITH_DIR:%ROOT%\=!
  set "MISSING=1"
) else (
  echo   MISSING  Content folder Elf_Blacksmith
  set "MISSING=1"
)
echo   uuid:    d01abf8c-9798-4bcb-8617-2d1d560d318a
echo   listing: https://www.fab.com/listings/d01abf8c-9798-4bcb-8617-2d1d560d318a
echo.

echo Restore
echo   1. Epic Games Launcher -^> Unreal Engine -^> Fab. Sign in.
echo   2. Open each listing. Add to My Library if needed.
echo   3. Add to Project -^> this SolidCore1.uproject.
echo   No folder moves. Viking and grass are found by asset name under /Game/Viking or /Game/Fab.
echo   Companion meshes are the SK_ mesh inside Medieval_Assassin_Girl and Elf_Blacksmith anywhere under Content.
echo   Manifest: tools\fab-assets.json
echo.

if "%MISSING%"=="1" (
  echo RESULT: required Fab assets are missing.
  exit /b 1
)
echo RESULT: required Fab assets are present.
exit /b 0
