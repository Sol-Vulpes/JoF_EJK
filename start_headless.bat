@echo off
setlocal
title EternalJK Headless (console / chat only)

REM ==========================================================================
REM  Launch EternalJK headless: no window, no GPU, just the console.
REM  Uses the rd-null renderer so the full client runs with minimal impact.
REM
REM  Usage:
REM     start_headless.bat                 - launch, then type "connect <ip>"
REM     start_headless.bat 1.2.3.4:29070   - launch and auto-connect
REM
REM  Once running, type Quake console commands right here in this window:
REM     connect <ip:port>     say hello     tell <client> hi     quit
REM ==========================================================================

REM --- Where the game lives (folder holding eternaljk.x86_64.exe). Edit if needed.
set "GAMEDATA=C:\Program Files (x86)\Steam\steamapps\common\Jedi Academy\GameData"
set "EXE=eternaljk.x86_64.exe"

REM --- Headless cvars: null renderer, no audio device, low fps to stay near-idle.
set "ARGS=+set cl_renderer rd-null +set s_initsound 0 +set com_maxfps 30"

REM --- Optional first argument = server to auto-connect to.
if not "%~1"=="" set "ARGS=%ARGS% +connect %~1"

cd /d "%GAMEDATA%"
if not exist "%EXE%" (
    echo.
    echo   ERROR: %EXE% not found in:
    echo     "%GAMEDATA%"
    echo   Edit the GAMEDATA line in this .bat to point at your GameData folder.
    echo.
    pause
    exit /b 1
)
if not exist "rd-null_x86_64.dll" (
    echo.
    echo   WARNING: rd-null_x86_64.dll not found next to the exe.
    echo   Build it and run copy_build_to_steam.bat, or the game will fall back
    echo   to the GL renderer and open a normal window.
    echo.
)

echo Starting EternalJK headless...  (type "quit" to exit)
echo.

REM /wait /b keeps the game attached to THIS console window (shared stdin/stdout)
REM so you can read chat and type commands here.
start "" /wait /b "%EXE%" %ARGS%

endlocal
