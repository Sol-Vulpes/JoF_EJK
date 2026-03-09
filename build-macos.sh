#!/usr/bin/env bash

echo "Building JoF EternalJK for macOS..."

# Ensure we're running on macOS
if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "This script is intended to be run on macOS."
  exit 1
fi

# Determine repository root (directory of this script)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

# Detect architecture
ARCH="$(uname -m)"
case "$ARCH" in
  arm64)
    BUILD_DIR="build-macos-arm64"
    ;;
  x86_64)
    BUILD_DIR="build-macos-x86_64"
    ;;
  *)
    echo "Unsupported architecture: $ARCH"
    exit 1
    ;;
esac

echo "Detected architecture: $ARCH"
echo "Build directory: $BUILD_DIR"

# Resolve CMake command
CMAKE_CMD="${CMAKE_PATH:-cmake}"

# If cmake-path.txt exists, use its first non-comment, non-empty line
if [[ -f "${SCRIPT_DIR}/cmake-path.txt" ]]; then
  while IFS= read -r line; do
    # Trim leading/trailing whitespace
    line="${line#"${line%%[![:space:]]*}"}"
    line="${line%"${line##*[![:space:]]}"}"

    # Skip empty or comment lines
    [[ -z "$line" ]] && continue
    [[ "$line" == \#* ]] && continue

    CMAKE_CMD="$line"
    break
  done < "${SCRIPT_DIR}/cmake-path.txt"
fi

# Verify CMake exists
if ! "$CMAKE_CMD" --version >/dev/null 2>&1; then
  echo "ERROR: CMake not found at: $CMAKE_CMD"
  echo
  echo "Please either:"
  echo "  1. Add cmake to your PATH"
  echo "  2. Set the CMAKE_PATH environment variable"
  echo "  3. Edit cmake-path.txt with the full path to the cmake executable"
  echo
  echo "Current CMAKE_CMD: $CMAKE_CMD"
  exit 1
fi

# Create build directory if needed
if [[ ! -d "$BUILD_DIR" ]]; then
  mkdir "$BUILD_DIR" || {
    echo "Failed to create build directory: $BUILD_DIR"
    exit 1
  }
fi

cd "$BUILD_DIR" || {
  echo "Failed to enter build directory: $BUILD_DIR"
  exit 1
}

# Configure with CMake
"$CMAKE_CMD" -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="$ARCH" ..
if [[ $? -ne 0 ]]; then
  echo "CMake configuration failed!"
  cd "$SCRIPT_DIR" || exit 1
  exit 1
fi

# Build
"$CMAKE_CMD" --build . --config Release
if [[ $? -ne 0 ]]; then
  echo "Build failed!"
  cd "$SCRIPT_DIR" || exit 1
  exit 1
fi

echo "Build completed successfully! Files are in $BUILD_DIR"
cd "$SCRIPT_DIR" || exit 1

