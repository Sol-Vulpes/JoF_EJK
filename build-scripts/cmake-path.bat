@echo off
REM CMake Path Resolver
REM Echoes the cmake command to use, and nothing else, so callers can do:
REM     for /f "tokens=*" %%i in ('call cmake-path.bat') do set "CMAKE_CMD=%%i"
REM
REM Resolution order:
REM   1. CMAKE_PATH environment variable
REM   2. cmake-path.txt next to this script (first non-comment, non-empty line)
REM   3. "cmake" from PATH
REM
REM Errors go to stderr: anything on stdout is taken by the caller as the path.

setlocal EnableExtensions EnableDelayedExpansion

set "CMAKE_CMD=cmake"
set "FROM_FILE="

REM 2. cmake-path.txt (looked up next to this script, not the current directory).
if not exist "%~dp0cmake-path.txt" goto :envvar
for /f "usebackq tokens=* delims=" %%i in ("%~dp0cmake-path.txt") do (
    if not defined FROM_FILE (
        echo %%i | findstr /b "#" >nul
        if !errorlevel! neq 0 if not "%%i"=="" set "FROM_FILE=%%i"
    )
)
if defined FROM_FILE set "CMAKE_CMD=%FROM_FILE%"

REM 1. The environment variable wins over the file.
:envvar
if defined CMAKE_PATH set "CMAKE_CMD=%CMAKE_PATH%"

"%CMAKE_CMD%" --version >nul 2>&1
if errorlevel 1 goto :notfound

echo %CMAKE_CMD%
exit /b 0

:notfound
echo ERROR: CMake not found at: %CMAKE_CMD% 1>&2
echo Fix this by doing one of: 1>&2
echo   - add cmake to your PATH 1>&2
echo   - set the CMAKE_PATH environment variable 1>&2
echo   - put the full path to cmake.exe in build-scripts\cmake-path.txt 1>&2
exit /b 1
