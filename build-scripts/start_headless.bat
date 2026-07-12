@echo off
setlocal EnableExtensions
title EternalJK Headless (console / chat only)

REM ==========================================================================
REM  Launch EternalJK headless: no window, no GPU, just the console.
REM  Uses the rd-null renderer so the full client runs with minimal impact.
REM
REM  Usage:
REM     start_headless.bat                 - launch, then type "connect <ip>"
REM     start_headless.bat 1.2.3.4:29070   - launch and auto-connect
REM
REM  Once running, type Quake console commands in the window it opens:
REM     connect <ip:port>     say hello     tell <client> hi     quit
REM ==========================================================================

for /f "tokens=*" %%i in ('call "%~dp0gamedata-path.bat"') do set "GAMEDATA=%%i"
if not defined GAMEDATA goto :failed

set "EXE=eternaljk.x86_64.exe"
if not exist "%GAMEDATA%\%EXE%" goto :noexe

REM One flag enables headless mode (null renderer, no sound, capped fps, own
REM console window). It is NOT archived, so it never affects normal play.
set "ARGS=+set com_headless 1"

REM Optional first argument = server to auto-connect to.
if not "%~1"=="" set "ARGS=%ARGS% +connect %~1"

if exist "%GAMEDATA%\rd-null_x86_64.dll" goto :launch
echo.
echo   WARNING: rd-null_x86_64.dll is not next to the exe, so the game will
echo   fall back to the GL renderer and open a normal window.
echo   Deploy a build first:  sol_one_script.bat deploy
echo.

:launch
echo Starting EternalJK headless... a separate console window will open;
echo type your commands there (connect / say / tell / quit).
echo.
REM "start" detaches the game: it keeps running after this window closes.
start "" /D "%GAMEDATA%" "%GAMEDATA%\%EXE%" %ARGS%
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
