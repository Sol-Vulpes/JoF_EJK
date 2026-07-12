@echo off
setlocal

REM ===========================================================================
REM  copy_build_to_steam.bat
REM  Copies the finished 64-bit build into your Jedi Academy GameData folder.
REM  Only files newer than the ones already there are copied (robocopy /XO).
REM
REM  Build first, or just use sol_one_script.bat to do build+copy+launch.
REM
REM  Pass "nopause" to skip the prompt at the end (sol_one_script.bat does this).
REM  The GameData folder is resolved by gamedata-path.bat.
REM ===========================================================================

REM Repo root is one level up from this script.
for %%I in ("%~dp0..") do set "ROOT=%%~fI"
set "SRC=%ROOT%\build64temp\Release"
set "PK3SRC=%ROOT%\build64temp\codemp"
set "NOPAUSE=%~1"

for /f "tokens=*" %%i in ('call "%~dp0gamedata-path.bat"') do set "GAMEDATA=%%i"
if not defined GAMEDATA goto :failed

set "ETERNALJK=%GAMEDATA%\EternalJK"

REM Note: don't echo an unquoted path inside a ( ) block - it is expanded while
REM the block is parsed, so a ")" in the path would close the block early.
if not exist "%SRC%\eternaljk.x86_64.exe" goto :nobuild

echo Copying main executables/DLLs to GameData ^(only if newer^)...
robocopy "%SRC%" "%GAMEDATA%" eternaljk.x86_64.exe rd-eternaljk_x86_64.dll rd-rend2-etjk_x86_64.dll rd-null_x86_64.dll eternaljkded.x86_64.exe /XO /R:1 /W:1
if errorlevel 8 goto :copyfailed

echo.
echo Copying EternalJK files to GameData\EternalJK ^(only if newer^)...
robocopy "%SRC%" "%ETERNALJK%" cgamex86_64.dll jampgamex86_64.dll uix86_64.dll compact_glsl.exe /XO /R:1 /W:1
if errorlevel 8 goto :copyfailed

echo.
echo Copying asset pk3s to GameData\EternalJK ^(only if newer^)...
robocopy "%PK3SRC%" "%ETERNALJK%" jofclient-assets.pk3 japro-assets.pk3 /XO /R:1 /W:1
if errorlevel 8 goto :copyfailed

echo.
echo Done.
REM robocopy uses 0-7 for success; normalise so callers can trust errorlevel.
if /i not "%NOPAUSE%"=="nopause" pause
exit /b 0

:nobuild
echo No build found at:
echo     %SRC%
echo Build it first:  sol_one_script.bat build
goto :failed

:copyfailed
echo.
echo Copy failed ^(robocopy exit %errorlevel%^). Is the game still running and
echo holding the exe/DLLs open? Close it and try again.
goto :failed

:failed
if /i not "%NOPAUSE%"=="nopause" pause
exit /b 1
