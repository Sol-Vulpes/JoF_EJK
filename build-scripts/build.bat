@echo off
setlocal EnableExtensions

REM ===========================================================================
REM  build.bat [x64|Win32]      (default: x64)
REM
REM  Incremental by design: CMake is only configured when the build directory
REM  doesn't exist yet, so a rebuild only recompiles the sources you changed.
REM  Nothing here forces a full rebuild.
REM
REM  Called by sol_one_script.bat; works standalone too.
REM ===========================================================================

REM Repo root is one level up from this script, whatever the current directory is.
for %%I in ("%~dp0..") do set "ROOT=%%~fI"

set "ARCH=%~1"
if not defined ARCH set "ARCH=x64"

if /i "%ARCH%"=="x64"   set "BUILD_DIR=%ROOT%\build64temp"
if /i "%ARCH%"=="Win32" set "BUILD_DIR=%ROOT%\build32temp"
if not defined BUILD_DIR (
    echo Unknown architecture "%ARCH%" - expected x64 or Win32.
    exit /b 1
)

echo Building JoF EternalJK for Windows ^(%ARCH%^)...

REM Resolve cmake and the newest installed Visual Studio generator.
for /f "tokens=*" %%i in ('call "%~dp0cmake-path.bat"') do set "CMAKE_CMD=%%i"
if not defined CMAKE_CMD exit /b 1

for /f "tokens=*" %%i in ('call "%~dp0detect-vs-generator.bat"') do set "VS_GENERATOR=%%i"
if not defined VS_GENERATOR (
    echo Failed to detect a Visual Studio generator.
    exit /b 1
)
echo Using generator: %VS_GENERATOR%

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
pushd "%BUILD_DIR%" || exit /b 1

REM Configure only on a fresh build dir; otherwise go straight to an
REM incremental build, the same as hitting Build in Visual Studio.
if exist "CMakeCache.txt" goto :compile

"%CMAKE_CMD%" -G "%VS_GENERATOR%" -A %ARCH% -DCMAKE_BUILD_TYPE=Release "%ROOT%"
if errorlevel 1 goto :configfailed

:compile
REM Force the asset pk3s to repack so new/changed sounds and textures get in.
REM The CMake zip step declares no input dependencies, so it would otherwise
REM keep a stale archive forever. Costs ~0.3s; everything else stays incremental.
if exist "codemp\jofclient-assets.pk3" del /q "codemp\jofclient-assets.pk3"
if exist "codemp\japro-assets.pk3" del /q "codemp\japro-assets.pk3"

"%CMAKE_CMD%" --build . --config Release
if errorlevel 1 goto :buildfailed

REM Put the fresh pk3s next to the exe. The engine's own copy step is skipped
REM when it doesn't relink (e.g. only assets or cgame changed), so do it here.
if not exist "codemp\jofclient-assets.pk3" goto :done
if not exist "Release\EternalJK" mkdir "Release\EternalJK"
copy /y "codemp\jofclient-assets.pk3" "Release\EternalJK\" >nul
if exist "codemp\japro-assets.pk3" copy /y "codemp\japro-assets.pk3" "Release\EternalJK\" >nul

:done
popd
echo Build OK - output in %BUILD_DIR%\Release\
exit /b 0

:configfailed
popd
echo.
echo CMake configuration failed. Check that Visual Studio and the C++ build
echo tools are installed, then delete "%BUILD_DIR%" and try again.
exit /b 1

:buildfailed
popd
echo.
echo Build failed.
exit /b 1
