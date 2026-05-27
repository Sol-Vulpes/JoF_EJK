#!/bin/bash
echo "Building JoF EternalJK for Linux..."

# Detect architecture
if [[ "$(uname -m)" == "x86_64" ]]; then
    ARCH="x86_64"
    BUILD_DIR="build-linux64"
else
    ARCH="i386"
    BUILD_DIR="build-linux32"
fi

echo "Detected architecture: $ARCH"
echo "Build directory: $BUILD_DIR"

if [ ! -d "$BUILD_DIR" ]; then
    mkdir "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure with CMake
cmake -DCMAKE_BUILD_TYPE=Release ..
if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    cd ..
    exit 1
fi

# Build
make -j$(nproc)
if [ $? -ne 0 ]; then
    echo "Build failed!"
    cd ..
    exit 1
fi

echo "Build completed successfully! Files are in $BUILD_DIR"
cd ..