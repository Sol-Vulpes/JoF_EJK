@echo off
echo JoF EternalJK Cross-Platform Build Script
echo ==========================================
echo.
echo Choose your build target:
echo [1] Windows 64-bit
echo [2] Windows 32-bit
echo [3] Linux (requires WSL or Linux environment)
echo [4] Show available Visual Studio generators
echo.
set /p choice="Enter your choice (1-4): "

if "%choice%"=="1" goto win64
if "%choice%"=="2" goto win32
if "%choice%"=="3" goto linux
if "%choice%"=="4" goto generators
echo Invalid choice. Exiting.
exit /b 1

:win64
echo Building for Windows 64-bit...
REM Get cmake path
for /f "tokens=*" %%i in ('call cmake-path.bat') do set CMAKE_CMD=%%i
if %errorlevel% neq 0 goto end

REM Detect available Visual Studio generator
for /f "tokens=*" %%i in ('call detect-vs-generator.bat') do set VS_GENERATOR=%%i
if %errorlevel% neq 0 goto end

echo Using generator: %VS_GENERATOR%

if not exist build-win64 mkdir build-win64
cd build-win64

REM Clean previous CMake cache if generator changed
if exist CMakeCache.txt (
    echo Cleaning previous CMake cache...
    rmdir /s /q CMakeFiles 2>nul
    del CMakeCache.txt 2>nul
)

"%CMAKE_CMD%" -G "%VS_GENERATOR%" -A x64 -DCMAKE_BUILD_TYPE=Release ..
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    cd ..
    goto end
)

"%CMAKE_CMD%" --build . --config Release
if %errorlevel% neq 0 (
    echo Build failed!
    cd ..
    goto end
)

echo Build completed successfully! Files are in build-win64\Release\
cd ..
goto end

:win32
echo Building for Windows 32-bit...
REM Get cmake path
for /f "tokens=*" %%i in ('call cmake-path.bat') do set CMAKE_CMD=%%i
if %errorlevel% neq 0 goto end

REM Detect available Visual Studio generator
for /f "tokens=*" %%i in ('call detect-vs-generator.bat') do set VS_GENERATOR=%%i
if %errorlevel% neq 0 goto end

echo Using generator: %VS_GENERATOR%

if not exist build-win32 mkdir build-win32
cd build-win32

REM Clean previous CMake cache if generator changed
if exist CMakeCache.txt (
    echo Cleaning previous CMake cache...
    rmdir /s /q CMakeFiles 2>nul
    del CMakeCache.txt 2>nul
)

"%CMAKE_CMD%" -G "%VS_GENERATOR%" -A Win32 -DCMAKE_BUILD_TYPE=Release ..
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    cd ..
    goto end
)

"%CMAKE_CMD%" --build . --config Release
if %errorlevel% neq 0 (
    echo Build failed!
    cd ..
    goto end
)

echo Build completed successfully! Files are in build-win32\Release\
cd ..
goto end

:linux
echo Building for Linux (using WSL or native Linux)...
if exist "C:\Windows\System32\bash.exe" (
    echo Using WSL...
    bash -c "./build-linux.sh"
) else (
    echo WSL not found. Please run this script on Linux or use WSL.
    echo You can also use: wsl ./build-linux.sh
)
goto end

:generators
echo.
echo Available Visual Studio Generators:
echo - Visual Studio 16 2019 (for VS 2019)
echo - Visual Studio 17 2022 (for VS 2022)
echo - Visual Studio 15 2017 (for VS 2017)
echo - Visual Studio 14 2015 (for VS 2015)
echo.
echo Example usage:
echo cmake -G "Visual Studio 16 2019" -A x64 .
echo.
pause
goto end

:end
echo.
echo Build process completed.
pause