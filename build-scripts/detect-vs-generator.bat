@echo off
REM Visual Studio Generator Detection
REM Echoes the newest installed Visual Studio generator, and nothing else:
REM     for /f "tokens=*" %%i in ('call detect-vs-generator.bat') do set "GEN=%%i"
REM
REM Progress lines go to stderr. They still show up in the console, but the
REM caller's for /f only ever sees the generator name on stdout.

setlocal EnableExtensions EnableDelayedExpansion

for /f "tokens=*" %%i in ('call "%~dp0cmake-path.bat"') do set "CMAKE_CMD=%%i"
if not defined CMAKE_CMD exit /b 1

echo Detecting available Visual Studio generators... 1>&2

REM Newest first: the first hit wins.
call :probe "Visual Studio 17 2022"
call :probe "Visual Studio 16 2019"
call :probe "Visual Studio 15 2017"
call :probe "Visual Studio 14 2015"

if not defined BEST goto :none

echo [*] Selected: %BEST% 1>&2
echo %BEST%
exit /b 0

:probe
if defined BEST goto :eof
"%CMAKE_CMD%" --help 2>nul | findstr /c:"%~1" >nul 2>&1
if errorlevel 1 goto :eof
set "BEST=%~1"
echo [+] Found %~1 1>&2
goto :eof

:none
echo [ERROR] No compatible Visual Studio generator found. 1>&2
echo Install Visual Studio 2015 or later, or the Visual Studio Build Tools. 1>&2
exit /b 1
