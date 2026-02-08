@echo off
echo JoF EternalJK Build Configuration Tool
echo ======================================
echo.
echo This tool helps you configure and build JoF EternalJK
echo.

:menu
echo Choose a build configuration:
echo [1] Quick 64-bit Release (recommended)
echo [2] Quick 32-bit Release
echo [3] Custom CMake configuration
echo [4] Clean build directories
echo [5] Show build status
echo [6] Exit
echo.
set /p choice="Enter your choice (1-6): "

if "%choice%"=="1" goto quick64
if "%choice%"=="2" goto quick32
if "%choice%"=="3" goto custom
if "%choice%"=="4" goto clean
if "%choice%"=="5" goto status
if "%choice%"=="6" goto exit
echo Invalid choice. Try again.
goto menu

:quick64
echo Setting up 64-bit Release build...
if not exist build-release-x64 mkdir build-release-x64
cd build-release-x64
cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Release ..
if %errorlevel% neq 0 goto cmake_error
cmake --build . --config Release
if %errorlevel% neq 0 goto build_error
echo Build completed successfully!
cd ..
goto menu

:quick32
echo Setting up 32-bit Release build...
if not exist build-release-x86 mkdir build-release-x86
cd build-release-x86
cmake -G "Visual Studio 16 2019" -A Win32 -DCMAKE_BUILD_TYPE=Release ..
if %errorlevel% neq 0 goto cmake_error
cmake --build . --config Release
if %errorlevel% neq 0 goto build_error
echo Build completed successfully!
cd ..
goto menu

:custom
echo.
echo Custom CMake Configuration
echo ==========================
echo.
echo Common options:
echo -DBuildMPRend2=ON/OFF          (experimental renderer)
echo -DBuildDiscordRichPresence=ON/OFF  (Discord integration)
echo -DBuildPortableVersion=ON/OFF      (portable mode)
echo -DUseInternalSDL2=ON/OFF           (use bundled SDL2)
echo.
echo Example: cmake -G "Visual Studio 16 2019" -A x64 -DBuildMPRend2=ON .
echo.
set /p cmake_cmd="Enter your custom cmake command: "
%cmake_cmd%
if %errorlevel% neq 0 (
    echo CMake configuration failed!
) else (
    echo CMake configuration completed.
    echo Run 'cmake --build . --config Release' to build.
)
goto menu

:clean
echo Cleaning build directories...
for /d %%i in (build*) do rmdir /s /q "%%i" 2>nul
echo Build directories cleaned.
goto menu

:status
echo.
echo Build Status:
echo =============
if exist build-release-x64 (
    echo [+] 64-bit Release build directory exists
    if exist build-release-x64\Release\eternaljk.x86_64.exe (
        echo [+] 64-bit executable found
    ) else (
        echo [-] 64-bit executable not found
    )
) else (
    echo [-] 64-bit Release build directory not found
)

if exist build-release-x86 (
    echo [+] 32-bit Release build directory exists
    if exist build-release-x86\Release\eternaljk.x86.exe (
        echo [+] 32-bit executable found
    ) else (
        echo [-] 32-bit executable not found
    )
) else (
    echo [-] 32-bit Release build directory not found
)

if exist build-win64 (
    echo [+] Windows 64-bit build directory exists
) else (
    echo [-] Windows 64-bit build directory not found
)

if exist build-win32 (
    echo [+] Windows 32-bit build directory exists
) else (
    echo [-] Windows 32-bit build directory not found
)
echo.
pause
goto menu

:cmake_error
echo.
echo ERROR: CMake configuration failed!
echo Please check that you have:
echo - Visual Studio installed
echo - CMake in your PATH
echo - Correct generator name
cd ..
goto menu

:build_error
echo.
echo ERROR: Build failed!
echo Check the output above for error details.
cd ..
goto menu

:exit
echo Goodbye!
exit /b 0