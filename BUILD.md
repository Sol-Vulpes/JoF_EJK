# JoF EternalJK Build Guide

This guide provides instructions for building JoF EternalJK for different platforms and architectures.

## Table of Contents
- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Windows Builds](#windows-builds)
- [Linux Builds](#linux-builds)
- [Build Options](#build-options)
- [Troubleshooting](#troubleshooting)

## Prerequisites

### Windows
- **Visual Studio 2015 or later** (2017, 2019, or 2022 recommended)
- **CMake 3.1 or later**
- **Git** (for cloning and version info)

### CMake Path Configuration
The build scripts support custom CMake installations. You can specify your CMake path in several ways:

1. **cmake-path.txt file**: Edit this file with your CMake executable path
2. **CMAKE_PATH environment variable**: Set this variable to your CMake path
3. **PATH**: Add CMake to your system PATH (default behavior)

### Linux
- **CMake 3.1 or later**
- **GCC or Clang compiler**
- **Make or Ninja**
- **Git**
- **Required libraries**: SDL2, OpenAL, zlib, libpng, libjpeg (or use bundled versions)

### Cross-compilation (Optional)
For cross-compiling Windows binaries on Linux:
- **mingw-w64** toolchain
- **mingw-w64-gcc** and **mingw-w64-g++**

## Quick Start

### Windows (Visual Studio)
```batch
# Clone the repository
git clone https://github.com/Milamber0/JoF_EJK.git
cd JoF_EJK

# Generate Visual Studio project (64-bit by default)
cmake -G "Visual Studio 16 2019" -A x64 .

# Build
cmake --build . --config Release
```

### Linux (Native)
```bash
# Clone the repository
git clone https://github.com/Milamber0/JoF_EJK.git
cd JoF_EJK

# Configure and build
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Windows Builds

### Visual Studio (Recommended)

#### 64-bit Build
```batch
# Automatic detection (recommended)
.\build-win64.bat

# Manual specification
cmake -G "Visual Studio 16 2019" -A x64 .
cmake --build . --config Release
```

#### 32-bit Build
```batch
# Automatic detection (recommended)
.\build-win32.bat

# Manual specification
cmake -G "Visual Studio 16 2019" -A Win32 .
cmake --build . --config Release
```

### Automated Build Scripts

Use the provided batch scripts in `scripts/builds/`:

```batch
# Full build process (generate, build, install, package)
call scripts\builds\all.bat
```

### Visual Studio Version Matrix

| Visual Studio Version | Generator | Platform Toolset |
|----------------------|-----------|------------------|
| 2015 | `"Visual Studio 14 2015"` | v140 |
| 2017 | `"Visual Studio 15 2017"` | v141 |
| 2019 | `"Visual Studio 16 2019"` | v142 |
| 2022 | `"Visual Studio 17 2022"` | v143 |

### Easy Architecture Switch Scripts

#### build-win64.bat
```batch
@echo off
if not exist build mkdir build
cd build
cmake -G "Visual Studio 16 2019" -A x64 ..
cmake --build . --config Release
cd ..
```

#### build-win32.bat
```batch
@echo off
if not exist build32 mkdir build32
cd build32
cmake -G "Visual Studio 16 2019" -A Win32 ..
cmake --build . --config Release
cd ..
```

## Linux Builds

### Native Linux Build (64-bit)
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Native Linux Build (32-bit)
```bash
mkdir build32 && cd build32
cmake -DCMAKE_TOOLCHAIN_FILE=../CMakeModules/Toolchains/linux-i686.cmake ..
make -j$(nproc)
```

### Cross-compilation (Linux → Windows)

#### 64-bit Windows from Linux
```bash
mkdir build-win64 && cd build-win64
cmake -DCMAKE_TOOLCHAIN_FILE=../CMakeModules/Toolchains/x86_64-w64-mingw32.cmake ..
make -j$(nproc)
```

#### 32-bit Windows from Linux
```bash
mkdir build-win32 && cd build-win32
cmake -DCMAKE_TOOLCHAIN_FILE=../CMakeModules/Toolchains/i686-w64-mingw32.cmake ..
make -j$(nproc)
```

### Intel Compiler Builds
```bash
# 64-bit with Intel Compiler
mkdir build-intel && cd build-intel
cmake -DCMAKE_TOOLCHAIN_FILE=../CMakeModules/Toolchains/linux-icc.cmake ..
make -j$(nproc)

# 32-bit with Intel Compiler
mkdir build-intel32 && cd build-intel32
cmake -DCMAKE_TOOLCHAIN_FILE=../CMakeModules/Toolchains/linux-i686-icc.cmake ..
make -j$(nproc)
```

## Build Options

### CMake Configuration Options

| Option | Default | Description |
|--------|---------|-------------|
| `BuildMPEngine` | ON | Build the main MP client (eternaljk.x86.exe) |
| `BuildMPRdVanilla` | ON | Build the default renderer |
| `BuildMPDed` | ON | Build the dedicated server |
| `BuildMPGame` | ON | Build the server-side gamecode |
| `BuildMPCGame` | ON | Build the client-side gamecode |
| `BuildMPUI` | ON | Build the UI code |
| `BuildMPRend2` | ON | Build the experimental rend2 renderer |
| `BuildPortableVersion` | ON | Build portable version |
| `BuildDiscordRichPresence` | ON | Enable Discord Rich Presence |
| `UseInternalOpenAL` | Auto | Use bundled OpenAL |
| `UseInternalZlib` | Auto | Use bundled zlib |
| `UseInternalPNG` | Auto | Use bundled libpng |
| `UseInternalJPEG` | Auto | Use bundled libjpeg |
| `UseInternalSDL2` | Auto | Use bundled SDL2 |

### Examples

```bash
# Minimal client build (no server components)
cmake -DBuildMPDed=OFF -DBuildMPGame=OFF ..

# Development build with all features
cmake -DBuildMPRend2=ON -DBuildDiscordRichPresence=ON ..

# Cross-platform build using bundled libraries
cmake -DUseInternalOpenAL=ON -DUseInternalSDL2=ON -DUseInternalZlib=ON ..
```

## Output Files

After building, you'll find these files in the build directory:

### Windows
- `eternaljk.x86.exe` - Main client (32-bit) or `eternaljk.x86_64.exe` (64-bit)
- `eternaljkded.x86.exe` - Dedicated server
- `rd-eternaljk_x86.dll` - Default renderer
- `rd-rend2-etjk_x86.dll` - Rend2 renderer (if enabled)
- `jampgamex86.dll` - Server-side gamecode
- `cgamex86.dll` - Client-side gamecode
- `uix86.dll` - UI code

### Linux
- `eternaljk.x86_64` - Main client
- `eternaljkded.x86_64` - Dedicated server
- `rd-eternaljk_x86_64.so` - Default renderer
- `jampgamex86_64.so` - Server-side gamecode
- `cgamex86_64.so` - Client-side gamecode
- `ui_x86_64.so` - UI code

## Troubleshooting

### Common Issues

#### CMake Errors
- Ensure you're using CMake 3.1 or later
- Clear the build directory and try again: `rm -rf build* && mkdir build && cd build`

#### Visual Studio Issues
- Use the correct generator for your Visual Studio version
- Ensure you have the C++ build tools installed

#### Linux Dependencies
```bash
# Ubuntu/Debian
sudo apt-get install cmake build-essential libsdl2-dev libopenal-dev zlib1g-dev libpng-dev libjpeg-dev

# Fedora/CentOS
sudo dnf install cmake gcc-c++ SDL2-devel openal-soft-devel zlib-devel libpng-devel libjpeg-devel

# Arch Linux
sudo pacman -S cmake gcc sdl2 openal zlib libpng libjpeg-turbo
```

#### Cross-compilation Issues
- Install mingw-w64: `sudo apt-get install gcc-mingw-w64 g++-mingw-w64`
- Ensure toolchain files are correctly referenced

#### Build Failures
- Check that all submodules are initialized: `git submodule update --init --recursive`
- Try building in a clean directory
- Check compiler versions and ensure they're supported

### Getting Help
- Check existing issues on GitHub
- Join the JoF Discord for community support
- Review the CMake output for specific error messages

## Advanced Configuration

### Custom Build Directory
```bash
mkdir mybuild && cd mybuild
cmake -DCMAKE_INSTALL_PREFIX=../install ..
make install
```

### Debug Builds
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

### Release with Debug Info
```bash
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make
```

### Ninja Build System
```bash
cmake -GNinja ..
ninja
```

This build system provides flexibility for development across multiple platforms while maintaining consistent output.</content>
</xai:function_call<parameter name="contents"># JoF EternalJK Client

[![Website](https://img.shields.io/badge/website-jofacademy-brightgreen.svg)](https://jofacademy.eu/)

This is the JoF EternalJK Client which focuses on providing a safe client option for JoF Members, friends and others to use.

It was based on an [EternalJK](https://github.com/eternalcodes/EternalJK) version given to us by [Bucky](https://github.com/Bucky21659) with the promise of not sharing some code that was protected by the TECH definition. This is the version of the repo with all of that TECH code removed, and will be where continued development will happen.

## Build Instructions

For detailed build instructions across all platforms (Windows 32/64-bit, Linux, cross-compilation), see [BUILD.md](BUILD.md).

### Quick Windows Build
```batch
# 64-bit (default)
cmake -G "Visual Studio 16 2019" -A x64 .
cmake --build . --config Release

# 32-bit
cmake -G "Visual Studio 16 2019" -A Win32 .
cmake --build . --config Release
```

### Quick Linux Build
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## License

[OpenJK](https://github.com/eternalcodes/EternalJK/blob/master/LICENSE.txt) is licensed under GPLv2 as free software. You are free to use, modify and redistribute OpenJK following the terms in LICENSE.txt.

## For players

Installing and running EternalJK:

1. [Download the latest release](https://github.com/Milamber0/JoF_EJK/releases).
2. Extract the file into the Jedi Academy `GameData` folder. For Steam users, this will be in `<Steam Folder>/steamapps/common/Jedi Academy/GameData/`.
3. Run eternaljk.x86.exe (Rename to jamp.exe for better steam support)

## Credits

### JoF Maintainers/Contributors
* [Milamber](https://github.com/Milamber0)
* Daniel
* [Jediman](https://github.com/Jediman9973)

### External Contributors
* [bucky](https://github.com/Bucky21659) - special thanks for giving us the starting point of this client.
* [eternal](https://github.com/eternalcodes)
* [loda](https://github.com/videoP)
* [Sunny](https://github.com/JKSunny) - for the Vulkan renderer version we package with our releases: [EternalJK](https://github.com/JKSunny/EternalJK)