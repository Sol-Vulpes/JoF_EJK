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
call build-win64.bat
goto end

:win32
echo Building for Windows 32-bit...
call build-win32.bat
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