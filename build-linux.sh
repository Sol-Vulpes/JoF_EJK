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

# Force the asset pk3s to rebuild so new/changed sounds and textures get packed.
# The CMake zip step declares no input deps, so it keeps a stale archive on
# incremental builds; deleting them makes the build regenerate from assets/.
rm -f codemp/jofclient-assets.pk3 codemp/japro-assets.pk3

# Build
make -j$(nproc)
if [ $? -ne 0 ]; then
    echo "Build failed!"
    cd ..
    exit 1
fi

# Redeploy the freshly built pk3s next to the exe (the engine's own copy step is
# skipped when it doesn't relink, e.g. when only assets or cgame changed).
if [ -f codemp/jofclient-assets.pk3 ]; then
    mkdir -p EternalJK
    cp -f codemp/jofclient-assets.pk3 EternalJK/
    [ -f codemp/japro-assets.pk3 ] && cp -f codemp/japro-assets.pk3 EternalJK/
fi

echo "Build completed successfully! Files are in $BUILD_DIR"
cd ..