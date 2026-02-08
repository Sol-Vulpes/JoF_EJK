@echo off
echo Building JoF EternalJK for Windows 64-bit...

if not exist build-win64 mkdir build-win64
cd build-win64

cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Release ..
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

echo Build completed successfully! Files are in build-win64\Release\
cd ..