@echo off
REM CMake Path Resolver
REM This script determines which cmake executable to use

setlocal enabledelayedexpansion

REM Default cmake command
set CMAKE_CMD=cmake

REM Check for cmake-path.txt file
if exist "cmake-path.txt" (
    REM Read the file and extract the first non-comment line
    for /f "tokens=*" %%i in (cmake-path.txt) do (
        REM Skip comment lines
        echo %%i | findstr "^#" >nul
        if !errorlevel! neq 0 (
            REM Skip empty lines
            if not "%%i"=="" (
                REM Remove any trailing/leading spaces
                for /f "tokens=*" %%j in ("%%i") do set CMAKE_CMD=%%j
                goto :found_path
            )
        )
    )
)

:found_path
REM Check if CMAKE_PATH environment variable is set
if defined CMAKE_PATH (
    set CMAKE_CMD=%CMAKE_PATH%
)

REM Verify cmake exists
"%CMAKE_CMD%" --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: CMake not found at: %CMAKE_CMD%
    echo.
    echo Please either:
    echo 1. Add cmake to your PATH
    echo 2. Set CMAKE_PATH environment variable
    echo 3. Edit cmake-path.txt with the full path to cmake.exe
    echo.
    echo Current CMAKE_CMD: %CMAKE_CMD%
    exit /b 1
)

REM Output the cmake command for use by other scripts
echo %CMAKE_CMD%