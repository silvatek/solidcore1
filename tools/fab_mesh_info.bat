@echo off
setlocal EnableExtensions
rem List meshes and animations in a downloaded Fab pack.
rem Does not change tools\fab-assets.json.
rem
rem Usage:
rem   tools\fab_mesh_info.bat Viking
rem   tools\fab_mesh_info.bat Content\Viking

set "SCRIPT_DIR=%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%fab_mesh_info.ps1" %*
exit /b %ERRORLEVEL%
