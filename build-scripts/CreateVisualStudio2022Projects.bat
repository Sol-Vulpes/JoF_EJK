@echo off
setlocal EnableExtensions

REM ===========================================================================
REM  Generate a Visual Studio 2022 solution in build\ for browsing and
REM  debugging in the IDE.
REM
REM  This is NOT the command-line build - that's build.bat, which uses its own
REM  build64temp\ directory, so the two never clash.
REM ===========================================================================

for %%I in ("%~dp0..") do set "ROOT=%%~fI"

for /f "tokens=*" %%i in ('call "%~dp0cmake-path.bat"') do set "CMAKE_CMD=%%i"
if not defined CMAKE_CMD goto :failed

if not exist "%ROOT%\build" mkdir "%ROOT%\build"
pushd "%ROOT%\build" || goto :failed

REM x64: this project ships 64-bit. (The old script generated Win32 here,
REM which produced a solution that didn't match what we actually build.)
"%CMAKE_CMD%" -G "Visual Studio 17 2022" -A x64 -D CMAKE_INSTALL_PREFIX="%ROOT%/install" "%ROOT%"
set "RC=%errorlevel%"
popd
if not "%RC%"=="0" goto :failed

echo.
echo Solution generated in: %ROOT%\build
pause
exit /b 0

:failed
echo.
echo Failed to generate the Visual Studio solution.
pause
exit /b 1
