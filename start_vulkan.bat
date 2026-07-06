@echo off
setlocal
title EternalJK (Vulkan)

REM ==========================================================================
REM  Launch EternalJK normally with the Vulkan renderer.
REM
REM  Also use this to un-stick the renderer: cl_renderer is an archived cvar,
REM  so whatever you pass here becomes your saved default. If a previous
REM  headless launch left cl_renderer on "rd-null", this overwrites it back to
REM  a real windowed renderer.
REM ==========================================================================

set "GAMEDATA=C:\Program Files (x86)\Steam\steamapps\common\Jedi Academy\GameData"
set "EXE=eternaljk.x86_64.exe"

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

REM Force the Vulkan renderer (overrides the archived cl_renderer value).
start "" "%EXE%" +set cl_renderer rd-vulkan

endlocal
