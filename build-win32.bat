@echo off
echo Building JoF EternalJK for Windows 32-bit...

if not exist build-win32 mkdir build-win32
cd build-win32

cmake -G "Visual Studio 16 2019" -A Win32 -DCMAKE_BUILD_TYPE=Release ..
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    cd ..
    exit /b 1
)

cmake --build . --config Release
if %errorlevel% neq 0 (
    echo Build failed!
    cd ..
    exit /b 1
)

echo Build completed successfully! Files are in build-win32\Release\
cd ..