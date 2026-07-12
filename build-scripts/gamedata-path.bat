@echo off
REM GameData Path Resolver
REM Echoes the Jedi Academy GameData folder so other scripts can consume it:
REM     for /f "tokens=*" %%i in ('call gamedata-path.bat') do set "GAMEDATA=%%i"
REM
REM Resolution order:
REM   1. GAMEDATA environment variable
REM        setx GAMEDATA "D:\Games\Jedi Academy\GameData"
REM   2. gamedata-path.txt sitting next to this script (first non-comment line)
REM   3. The usual Steam install locations
REM
REM Candidates are probed through the :try subroutine rather than a for-loop
REM inside an if-block: cmd mis-parses the "(x86)" in "Program Files (x86)" when
REM that list is nested inside another parenthesised block.

setlocal enabledelayedexpansion

set "FOUND="

REM --- 1. Environment variable wins. ---
if defined GAMEDATA set "FOUND=%GAMEDATA%"
if defined FOUND goto :check

REM --- 2. gamedata-path.txt (first non-comment, non-empty line). ---
if not exist "%~dp0gamedata-path.txt" goto :probe
for /f "usebackq tokens=* delims=" %%i in ("%~dp0gamedata-path.txt") do (
    if not defined FOUND (
        echo %%i | findstr /b "#" >nul
        if !errorlevel! neq 0 set "FOUND=%%i"
    )
)
if defined FOUND goto :check

REM --- 3. Common Steam locations. ---
:probe
call :try "C:\Program Files (x86)\Steam\steamapps\common\Jedi Academy\GameData"
call :try "C:\Program Files\Steam\steamapps\common\Jedi Academy\GameData"
call :try "D:\SteamLibrary\steamapps\common\Jedi Academy\GameData"
call :try "E:\SteamLibrary\steamapps\common\Jedi Academy\GameData"
call :try "S:\SteamLibrary\steamapps\common\Jedi Academy\GameData"
call :try "D:\Steam\steamapps\common\Jedi Academy\GameData"

REM Nothing below uses a parenthesised block: %FOUND% is expanded while the block
REM is parsed, so the ")" in "Program Files (x86)" would close the block early.
:check
if not defined FOUND goto :notfound
if not exist "%FOUND%" goto :missing

echo %FOUND%
exit /b 0

REM Errors go to stderr so callers capturing stdout never mistake them for a path.
:notfound
echo ERROR: Could not find your Jedi Academy GameData folder. 1>&2
echo Set it once with:  setx GAMEDATA "D:\Games\Jedi Academy\GameData" 1>&2
echo ...or put the path in gamedata-path.txt next to this script. 1>&2
exit /b 1

:missing
echo ERROR: GameData folder does not exist: %FOUND% 1>&2
exit /b 1

:try
if defined FOUND goto :eof
if exist "%~1" set "FOUND=%~1"
goto :eof
