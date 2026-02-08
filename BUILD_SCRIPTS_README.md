# JoF EternalJK Build Scripts

This directory contains various build scripts to make building JoF EternalJK easier across different platforms.

## Windows Batch Scripts

### `build-win64.bat`
Builds JoF EternalJK for Windows 64-bit.
```batch
build-win64.bat
```

### `build-win32.bat`
Builds JoF EternalJK for Windows 32-bit.
```batch
build-win32.bat
```

### `build-cross-platform.bat`
Interactive menu for choosing build targets (Windows 64-bit, 32-bit, or Linux).
```batch
build-cross-platform.bat
```

### `build-config.bat`
Advanced interactive build configuration tool with multiple options.
```batch
build-config.bat
```

## PowerShell Script

### `build.ps1`
Cross-platform PowerShell build script with parameters.

```powershell
# Build Windows 64-bit (default)
.\build.ps1

# Build Windows 32-bit
.\build.ps1 -Target win32

# Build with Debug configuration
.\build.ps1 -Configuration Debug

# Clean all build directories
.\build.ps1 -Target clean

# Show build status
.\build.ps1 -Target status
```

Parameters:
- `-Target`: `win64`, `win32`, `linux`, `clean`, `status`
- `-Configuration`: `Release`, `Debug`, `RelWithDebInfo`
- `-Generator`: Visual Studio generator name

## Linux Scripts

### `build-linux.sh`
Builds JoF EternalJK for Linux (detects architecture automatically).
```bash
./build-linux.sh
```

## Configuration

### `cmake-path.txt`
Specify a custom CMake executable path. Uncomment and modify the path as needed:
```
# Example for Windows
C:\Program Files\CMake\bin\cmake.exe

# Example for Linux/Mac
/usr/local/bin/cmake
```

Alternatively, set the `CMAKE_PATH` environment variable.

### `build-config.ini`
Configuration file for customizing build settings. Edit this file to change default options.

## Quick Reference

| Target | Windows Batch | PowerShell | Linux |
|--------|---------------|------------|-------|
| 64-bit Windows | `build-win64.bat` | `.\build.ps1` | N/A |
| 32-bit Windows | `build-win32.bat` | `.\build.ps1 -Target win32` | N/A |
| Linux | N/A | `.\build.ps1 -Target linux` | `./build-linux.sh` |
| Clean builds | N/A | `.\build.ps1 -Target clean` | `rm -rf build*` |
| Show status | N/A | `.\build.ps1 -Target status` | `ls -la build*/` |

## Prerequisites

### Windows
- Visual Studio 2015 or later
- CMake 3.1+
- PowerShell (for PowerShell scripts)

### Linux
- CMake 3.1+
- GCC/Clang
- Required libraries (SDL2, OpenAL, etc.) or use bundled versions

## Troubleshooting

### Visual Studio Not Found
The build scripts automatically detect available Visual Studio versions. If you get errors:
1. Install Visual Studio with C++ build tools (2015 or later)
2. Install Visual Studio Build Tools if you don't want the full IDE
3. Ensure Visual Studio is properly registered with Windows
4. Try running the scripts as Administrator

**Automatic Detection Order:**
- Visual Studio 2022 (preferred)
- Visual Studio 2019
- Visual Studio 2017
- Visual Studio 2015 (minimum supported)

If no Visual Studio is detected, you can:
- Install Visual Studio Community (free)
- Use alternative generators like Ninja (requires additional setup)

### CMake Errors
- Ensure CMake is accessible via PATH, cmake-path.txt, or CMAKE_PATH environment variable
- Clear build directories: `.\build.ps1 -Target clean`
- Try different generator
- Check cmake-path.txt for correct path syntax

### Linux Dependencies
```bash
# Ubuntu/Debian
sudo apt-get install cmake build-essential libsdl2-dev libopenal-dev zlib1g-dev libpng-dev libjpeg-dev

# Fedora/CentOS
sudo dnf install cmake gcc-c++ SDL2-devel openal-soft-devel zlib-devel libpng-devel libjpeg-devel
```

### Permission Errors
On Linux/Mac, make scripts executable:
```bash
chmod +x *.sh
```

## Advanced Usage

### Custom CMake Options
Edit `build-config.ini` or use command line:

```batch
cmake -DBuildMPRend2=ON -DBuildDiscordRichPresence=OFF -DUseInternalSDL2=ON ..
```

### Cross-compilation
For building Windows binaries on Linux, use the CMake toolchain files:
```bash
cmake -DCMAKE_TOOLCHAIN_FILE=CMakeModules/Toolchains/x86_64-w64-mingw32.cmake ..
```

See [BUILD.md](BUILD.md) for complete build instructions.</content>
</xai:function_call<parameter name="contents"># JoF EternalJK Build Scripts

This directory contains various build scripts to make building JoF EternalJK easier across different platforms.

## Windows Batch Scripts

### `build-win64.bat`
Builds JoF EternalJK for Windows 64-bit.
```batch
build-win64.bat
```

### `build-win32.bat`
Builds JoF EternalJK for Windows 32-bit.
```batch
build-win32.bat
```

### `build-cross-platform.bat`
Interactive menu for choosing build targets (Windows 64-bit, 32-bit, or Linux).
```batch
build-cross-platform.bat
```

### `build-config.bat`
Advanced interactive build configuration tool with multiple options.
```batch
build-config.bat
```

## PowerShell Script

### `build.ps1`
Cross-platform PowerShell build script with parameters.

```powershell
# Build Windows 64-bit (default)
.\build.ps1

# Build Windows 32-bit
.\build.ps1 -Target win32

# Build with Debug configuration
.\build.ps1 -Configuration Debug

# Clean all build directories
.\build.ps1 -Target clean

# Show build status
.\build.ps1 -Target status
```

Parameters:
- `-Target`: `win64`, `win32`, `linux`, `clean`, `status`
- `-Configuration`: `Release`, `Debug`, `RelWithDebInfo`
- `-Generator`: Visual Studio generator name

## Linux Scripts

### `build-linux.sh`
Builds JoF EternalJK for Linux (detects architecture automatically).
```bash
./build-linux.sh
```

## Configuration

### `cmake-path.txt`
Specify a custom CMake executable path. Uncomment and modify the path as needed:
```
# Example for Windows
C:\Program Files\CMake\bin\cmake.exe

# Example for Linux/Mac
/usr/local/bin/cmake
```

Alternatively, set the `CMAKE_PATH` environment variable.

### `build-config.ini`
Configuration file for customizing build settings. Edit this file to change default options.

## Quick Reference

| Target | Windows Batch | PowerShell | Linux |
|--------|---------------|------------|-------|
| 64-bit Windows | `build-win64.bat` | `.\build.ps1` | N/A |
| 32-bit Windows | `build-win32.bat` | `.\build.ps1 -Target win32` | N/A |
| Linux | N/A | `.\build.ps1 -Target linux` | `./build-linux.sh` |
| Clean builds | N/A | `.\build.ps1 -Target clean` | `rm -rf build*` |
| Show status | N/A | `.\build.ps1 -Target status` | `ls -la build*/` |

## Prerequisites

### Windows
- Visual Studio 2015 or later
- CMake 3.1+
- PowerShell (for PowerShell scripts)

### Linux
- CMake 3.1+
- GCC/Clang
- Required libraries (SDL2, OpenAL, etc.) or use bundled versions

## Troubleshooting

### Visual Studio Not Found
The build scripts automatically detect available Visual Studio versions. If you get errors:
1. Install Visual Studio with C++ build tools (2015 or later)
2. Install Visual Studio Build Tools if you don't want the full IDE
3. Ensure Visual Studio is properly registered with Windows
4. Try running the scripts as Administrator

**Automatic Detection Order:**
- Visual Studio 2022 (preferred)
- Visual Studio 2019
- Visual Studio 2017
- Visual Studio 2015 (minimum supported)

If no Visual Studio is detected, you can:
- Install Visual Studio Community (free)
- Use alternative generators like Ninja (requires additional setup)

### CMake Errors
- Ensure CMake is accessible via PATH, cmake-path.txt, or CMAKE_PATH environment variable
- Clear build directories: `.\build.ps1 -Target clean`
- Try different generator
- Check cmake-path.txt for correct path syntax

### Linux Dependencies
```bash
# Ubuntu/Debian
sudo apt-get install cmake build-essential libsdl2-dev libopenal-dev zlib1g-dev libpng-dev libjpeg-dev

# Fedora/CentOS
sudo dnf install cmake gcc-c++ SDL2-devel openal-soft-devel zlib-devel libpng-devel libjpeg-devel
```

### Permission Errors
On Linux/Mac, make scripts executable:
```bash
chmod +x *.sh
```

## Advanced Usage

### Custom CMake Options
Edit `build-config.ini` or use command line:

```batch
cmake -DBuildMPRend2=ON -DBuildDiscordRichPresence=OFF -DUseInternalSDL2=ON ..
```

### Cross-compilation
For building Windows binaries on Linux, use the CMake toolchain files:
```bash
cmake -DCMAKE_TOOLCHAIN_FILE=CMakeModules/Toolchains/x86_64-w64-mingw32.cmake ..
```

See [BUILD.md](BUILD.md) for complete build instructions.