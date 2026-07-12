@echo off
setlocal EnableExtensions

REM ===========================================================================
REM  sol_one_script.bat - the one Windows script for JoF EternalJK.
REM
REM  Double-click it, or from a terminal:
REM
REM    sol_one_script.bat              build + deploy + launch  (the usual one)
REM    sol_one_script.bat build        build only (64-bit)
REM    sol_one_script.bat build32      build only (32-bit)
REM    sol_one_script.bat deploy       copy the existing build into GameData
REM    sol_one_script.bat run          launch the game (no build, no deploy)
REM    sol_one_script.bat headless     launch headless (rd-null, console only)
REM    sol_one_script.bat vulkan       launch with the Vulkan renderer
REM    sol_one_script.bat vs           generate a Visual Studio solution
REM    sol_one_script.bat clean        delete the build directories
REM    sol_one_script.bat help
REM
REM  Anything after the command is passed straight to the game:
REM    sol_one_script.bat run +connect 1.2.3.4
REM    sol_one_script.bat all +set cg_autoHeal 1
REM
REM  Builds are incremental - only sources you actually changed get recompiled.
REM  The game is launched detached: close this window and it keeps running.
REM
REM  The pieces live in build-scripts\ and still work on their own.
REM ===========================================================================

set "SCRIPTS=%~dp0build-scripts"

set "CMD=%~1"
if not defined CMD set "CMD=all"

REM Everything after the command is forwarded to the game.
set "GAMEARGS="
:collect
shift
if "%~1"=="" goto :dispatch
set "GAMEARGS=%GAMEARGS% %1"
goto :collect

:dispatch
if /i "%CMD%"=="all"      goto :all
if /i "%CMD%"=="build"    goto :build
if /i "%CMD%"=="build32"  goto :build32
if /i "%CMD%"=="deploy"   goto :deploy
if /i "%CMD%"=="run"      goto :run
if /i "%CMD%"=="headless" goto :headless
if /i "%CMD%"=="vulkan"   goto :vulkan
if /i "%CMD%"=="vs"       goto :vs
if /i "%CMD%"=="clean"    goto :clean
if /i "%CMD%"=="help"     goto :help
if /i "%CMD%"=="--help"   goto :help
if /i "%CMD%"=="-h"       goto :help

echo Unknown command: %CMD%
echo.
goto :help


:all
echo === [1/3] Build ===
call "%SCRIPTS%\build.bat" x64
if errorlevel 1 goto :failed
echo.
echo === [2/3] Deploy ===
call "%SCRIPTS%\copy_build_to_steam.bat" nopause
if errorlevel 1 goto :failed
echo.
echo === [3/3] Launch ===
goto :launch


:build
call "%SCRIPTS%\build.bat" x64
if errorlevel 1 goto :failed
exit /b 0


:build32
call "%SCRIPTS%\build.bat" Win32
if errorlevel 1 goto :failed
exit /b 0


:deploy
call "%SCRIPTS%\copy_build_to_steam.bat" nopause
if errorlevel 1 goto :failed
exit /b 0


:run
goto :launch


:launch
for /f "tokens=*" %%i in ('call "%SCRIPTS%\gamedata-path.bat"') do set "GAMEDATA=%%i"
if not defined GAMEDATA goto :failed
if not exist "%GAMEDATA%\eternaljk.x86_64.exe" goto :noexe

REM "start" launches the game in its own process with no console attached, so
REM it survives this window closing. Do NOT add /B - that would tie it to this
REM console and it would die with the terminal.
start "" /D "%GAMEDATA%" "%GAMEDATA%\eternaljk.x86_64.exe" +set fs_game EternalJK%GAMEARGS%
echo Launched. This window can be closed - the game keeps running.
exit /b 0


:headless
call "%SCRIPTS%\start_headless.bat" %GAMEARGS%
exit /b %errorlevel%


:vulkan
call "%SCRIPTS%\start_vulkan.bat" %GAMEARGS%
exit /b %errorlevel%


:vs
call "%SCRIPTS%\CreateVisualStudio2022Projects.bat"
exit /b %errorlevel%


:clean
echo Removing build directories...
if exist "%~dp0build64temp" rmdir /s /q "%~dp0build64temp"
if exist "%~dp0build32temp" rmdir /s /q "%~dp0build32temp"
echo Done. The next build will reconfigure CMake from scratch.
echo (The Visual Studio solution in build\ was left alone.)
exit /b 0


:noexe
echo.
echo eternaljk.x86_64.exe is not in your GameData folder yet.
echo Deploy a build first:  sol_one_script.bat deploy
echo.
goto :failed


:help
echo.
echo   sol_one_script.bat [command] [extra game args]
echo.
echo     (none)     build + deploy + launch
echo     build      build only (64-bit)
echo     build32    build only (32-bit)
echo     deploy     copy the existing build into GameData
echo     run        launch the game
echo     headless   launch headless (console only, no GPU)
echo     vulkan     launch with the Vulkan renderer
echo     vs         generate a Visual Studio solution
echo     clean      delete build64temp\ and build32temp\
echo.
echo   Extra arguments go to the game:
echo     sol_one_script.bat run +connect 1.2.3.4
echo.
exit /b 0


:failed
echo.
echo *** Failed. The game was not launched. ***
pause
exit /b 1
