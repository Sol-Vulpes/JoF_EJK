@echo off
echo Building JoF EternalJK for Windows 64-bit...

REM Get cmake path
for /f "tokens=*" %%i in ('call cmake-path.bat') do set CMAKE_CMD=%%i
if %errorlevel% neq 0 exit /b 1

REM Detect available Visual Studio generator
for /f "tokens=*" %%i in ('call detect-vs-generator.bat') do set VS_GENERATOR=%%i
if %errorlevel% neq 0 (
    echo Failed to detect Visual Studio generator!
    exit /b 1
)

echo Using generator: %VS_GENERATOR%

REM Use unique build directory to avoid conflicts
set BUILD_DIR=build64temp
echo Using build directory: %BUILD_DIR%

REM Only run CMake configuration when the build dir isn't set up yet.
REM If it already exists, skip straight to an incremental build (like VS's Build button).
if exist "%BUILD_DIR%\CMakeCache.txt" (
    echo Build directory already configured, skipping CMake configuration.
    cd "%BUILD_DIR%"
    if errorlevel 1 (
        echo Failed to change to directory %BUILD_DIR%
        exit /b 1
    )
    echo Now in directory: %CD%
) else (
    if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
    if errorlevel 1 (
        echo Failed to create directory %BUILD_DIR%
        exit /b 1
    )
    cd "%BUILD_DIR%"
    if errorlevel 1 (
        echo Failed to change to directory %BUILD_DIR%
        exit /b 1
    )
    echo Now in directory: %CD%

    "%CMAKE_CMD%" -G "%VS_GENERATOR%" -A x64 -DCMAKE_BUILD_TYPE=Release ..
    if errorlevel 1 (
        echo CMake configuration failed!
        echo.
        echo Troubleshooting:
        echo 1. Ensure Visual Studio is properly installed
        echo 2. Try running as Administrator
        echo 3. Check that you have the C++ build tools installed
        echo 4. Try using a different generator manually
        echo.
        cd ..
        exit /b 1
    )
)

REM Force the asset pk3s to rebuild so new/changed sounds and textures get packed.
REM The CMake zip step declares no input deps, so it keeps a stale archive on
REM incremental builds; deleting them makes the build regenerate from assets\.
if exist "codemp\jofclient-assets.pk3" del /q "codemp\jofclient-assets.pk3"
if exist "codemp\japro-assets.pk3" del /q "codemp\japro-assets.pk3"

"%CMAKE_CMD%" --build . --config Release
if %errorlevel% neq 0 (
    echo Build failed!
    cd ..
    exit /b 1
)

REM Redeploy the freshly built pk3s next to the exe (the engine's own copy step
REM is skipped when it doesn't relink, e.g. when only assets or cgame changed).
if exist "codemp\jofclient-assets.pk3" (
    if not exist "Release\EternalJK" mkdir "Release\EternalJK"
    copy /y "codemp\jofclient-assets.pk3" "Release\EternalJK\" >nul
    if exist "codemp\japro-assets.pk3" copy /y "codemp\japro-assets.pk3" "Release\EternalJK\" >nul
)

echo Build completed successfully! Files are in %BUILD_DIR%\Release\
cd ..