# JoF EternalJK Client
[![Website](https://img.shields.io/badge/website-jofacademy-brightgreen.svg)](https://jofacademy.eu/)

This is the JoF EternalJK Client which focuses on providing a safe client option for JoF Members, friends and others to use.

It was based on an [![Fork](https://img.shields.io/badge/repository-EternalJK-brightgreen.svg)](https://github.com/eternalcodes/EternalJK) version given to us by [Bucky](https://github.com/Bucky21659) with the promise of not sharing some code that was protected by the TECH definition. This is the version of the repo with all of that TECH code removed, and will be where continued development will happen.

## License

[![License](https://img.shields.io/github/license/eternalcodes/EternalJK.svg)](https://github.com/eternalcodes/EternalJK/blob/master/LICENSE.txt)

OpenJK is licensed under GPLv2 as free software. You are free to use, modify and redistribute OpenJK following the terms in LICENSE.txt.

## Build Instructions

For detailed build instructions across all platforms (Windows 32/64-bit, Linux, macOS, cross-compilation), see [BUILD.md](BUILD.md).

**Custom CMake Path**: If you use a specific CMake version, see [CMAKE_PATH_README.md](CMAKE_PATH_README.md) for configuration options.

### Requirements & Supported Compilers

The code targets the **C++11** standard and requires **CMake 3.10 or later**. Our release binaries (see [`.github/workflows/build.yml`](.github/workflows/build.yml)) are produced with the toolchains below, so these are the recommended/tested versions:

| Platform | Compiler | Tested version | Notes |
|----------|----------|----------------|-------|
| Windows  | MSVC | Visual Studio 2022 (v143) | VS 2015 (v140) and later also work |
| Linux (x86_64) | GCC | **GCC 11** (Ubuntu 22.04) | GCC 9+ should work; Clang also supported |
| Linux (x86, 32-bit) | GCC | **GCC 11** via `gcc-multilib` / `g++-multilib` | uses `linux-i686` toolchain file |
| Linux → Windows (cross) | MinGW-w64 GCC | mingw-w64 (e.g. GCC 10+) | via `x86_64-w64-mingw32` / `i686-w64-mingw32` toolchain files |
| macOS (x86_64 / arm64) | Apple Clang | Xcode (macOS 15 runners) | |

On Debian/Ubuntu you can install the Linux toolchain with:

```bash
# 64-bit
sudo apt-get install build-essential cmake libsdl2-dev libopenal-dev zlib1g-dev libpng-dev libjpeg-dev

# 32-bit (adds the multilib GCC)
sudo apt-get install gcc-multilib g++-multilib libjpeg-dev:i386 libpng-dev:i386 zlib1g-dev:i386 libsdl2-dev:i386
```

### Easy Build Scripts

Use the provided build scripts for quick setup:

**Windows:**
```batch
# Interactive menu (recommended for beginners)
build-cross-platform.bat

# Direct builds
build-win64.bat    # 64-bit Windows
build-win32.bat    # 32-bit Windows

# Advanced configuration
build-config.bat
```

**PowerShell (cross-platform):**
```powershell
# 64-bit Windows (default)
.\build.ps1

# 32-bit Windows
.\build.ps1 -Target win32

# Linux (requires WSL on Windows)
.\build.ps1 -Target linux
```

**Linux:**
```bash
./build-linux.sh
```

### Manual CMake Build

**Windows:**
```batch
# 64-bit (default)
cmake -G "Visual Studio 16 2019" -A x64 .
cmake --build . --config Release

# 32-bit
cmake -G "Visual Studio 16 2019" -A Win32 .
cmake --build . --config Release
```

**Linux:**
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

See [BUILD_SCRIPTS_README.md](BUILD_SCRIPTS_README.md) for detailed script usage.

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
* [lumayaa](https://github.com/lumayaa)
* [Sol-Vulpes](https://github.com/Sol-Vulpes)
* [Clix (looZ149)](https://github.com/looZ149)

### External Contributors
* [bucky](https://github.com/Bucky21659) - special thanks for giving us the starting point of this client.
* [eternal](https://github.com/eternalcodes)
* [loda](https://github.com/videoP)
* [Sunny](https://github.com/JKSunny) - for the Vulkan renderer version we package with our releases: [![Fork](https://img.shields.io/badge/repository-EternalJK-brightgreen.svg)](https://github.com/JKSunny/EternalJK)
* [Tayst](https://github.com/taysta) - for changes contributed to Sunny's EJK which we sourced from.
