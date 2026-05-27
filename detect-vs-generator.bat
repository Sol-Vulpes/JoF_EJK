@echo off
REM Visual Studio Generator Detection Script
REM This script detects the highest available Visual Studio generator

setlocal enabledelayedexpansion

REM Check for available Visual Studio generators by testing cmake
set "VS2022_FOUND="
set "VS2019_FOUND="
set "VS2017_FOUND="
set "VS2015_FOUND="

echo Detecting available Visual Studio generators...

REM Test VS2022
cmake --help 2>nul | findstr "Visual Studio 17 2022" >nul 2>&1
if %errorlevel% equ 0 (
    echo [+] Visual Studio 17 2022 found
    set "VS2022_FOUND=Visual Studio 17 2022"
)

REM Test VS2019
cmake --help 2>nul | findstr "Visual Studio 16 2019" >nul 2>&1
if %errorlevel% equ 0 (
    echo [+] Visual Studio 16 2019 found
    set "VS2019_FOUND=Visual Studio 16 2019"
)

REM Test VS2017
cmake --help 2>nul | findstr "Visual Studio 15 2017" >nul 2>&1
if %errorlevel% equ 0 (
    echo [+] Visual Studio 15 2017 found
    set "VS2017_FOUND=Visual Studio 15 2017"
)

REM Test VS2015
cmake --help 2>nul | findstr "Visual Studio 14 2015" >nul 2>&1
if %errorlevel% equ 0 (
    echo [+] Visual Studio 14 2015 found
    set "VS2015_FOUND=Visual Studio 14 2015"
)

REM Determine the best generator (highest version first)
if defined VS2022_FOUND (
    set "BEST_GENERATOR=!VS2022_FOUND!"
    echo [*] Selected: !BEST_GENERATOR! (newest available)
    goto :found_generator
)

if defined VS2019_FOUND (
    set "BEST_GENERATOR=!VS2019_FOUND!"
    echo [*] Selected: !BEST_GENERATOR!
    goto :found_generator
)

if defined VS2017_FOUND (
    set "BEST_GENERATOR=!VS2017_FOUND!"
    echo [*] Selected: !BEST_GENERATOR!
    goto :found_generator
)

if defined VS2015_FOUND (
    set "BEST_GENERATOR=!VS2015_FOUND!"
    echo [*] Selected: !BEST_GENERATOR! (oldest supported)
    goto :found_generator
)

REM No generators found
echo [ERROR] No compatible Visual Studio generator found!
echo.
echo Please install one of the following:
echo - Visual Studio 2015 or later
echo - Visual Studio Build Tools
echo.
echo Alternatively, you can use Ninja or other generators.
echo.
exit /b 1

:found_generator

REM Output the selected generator for use by other scripts
echo !BEST_GENERATOR!