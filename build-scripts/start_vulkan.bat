@echo off
setlocal EnableExtensions
title EternalJK (Vulkan)

REM ==========================================================================
REM  Launch EternalJK with the Vulkan renderer.
REM
REM  Also use this to un-stick the renderer: cl_renderer is an archived cvar,
REM  so whatever you pass here becomes your saved default. If a previous
REM  headless launch left cl_renderer on "rd-null", this puts it back to a
REM  real windowed renderer.
REM
REM  Anything you pass is forwarded to the game:
REM      start_vulkan.bat +connect 1.2.3.4
REM ==========================================================================

for /f "tokens=*" %%i in ('call "%~dp0gamedata-path.bat"') do set "GAMEDATA=%%i"
if not defined GAMEDATA goto :failed

set "EXE=eternaljk.x86_64.exe"
if not exist "%GAMEDATA%\%EXE%" goto :noexe

REM Force the Vulkan renderer, overriding the archived cl_renderer value.
REM "start" detaches the game: it keeps running after this window closes.
start "" /D "%GAMEDATA%" "%GAMEDATA%\%EXE%" +set cl_renderer rd-vulkan %*
exit /b 0

:noexe
echo.
echo   ERROR: %EXE% not found in:
echo     %GAMEDATA%
echo   Deploy a build first:  sol_one_script.bat deploy
echo.
pause
exit /b 1

:failed
pause
exit /b 1
