# CMake Path Configuration

The JoF EternalJK build system now supports multiple ways to specify which CMake executable to use, allowing you to use specific CMake versions or installations.

## Configuration Methods

### 1. cmake-path.txt File (Recommended)

Edit the `cmake-path.txt` file in the project root:

```txt
# Windows example
C:\Program Files\CMake\bin\cmake.exe

# Linux example
/usr/local/bin/cmake

# Default (uses PATH)
cmake
```

The file supports comments (lines starting with #) and will use the first non-comment, non-empty line.

### 2. CMAKE_PATH Environment Variable

Set the environment variable before running build scripts:

**Windows:**
```batch
set CMAKE_PATH=C:\Program Files\CMake\bin\cmake.exe
build-win64.bat
```

**PowerShell:**
```powershell
$env:CMAKE_PATH = "C:\Program Files\CMake\bin\cmake.exe"
.\build.ps1
```

**Linux:**
```bash
export CMAKE_PATH=/usr/local/bin/cmake
./build-linux.sh
```

### 3. System PATH (Default)

If neither of the above is configured, the scripts will use `cmake` from your system PATH.

## Priority Order

The scripts check for CMake paths in this order:
1. `cmake-path.txt` file (first valid line)
2. `CMAKE_PATH` environment variable
3. `cmake` in system PATH

## Examples

### Using a Specific CMake Version

If you have multiple CMake versions installed:

**Windows:**
```
C:\Program Files\CMake 3.28\bin\cmake.exe
```

**Linux:**
```
/opt/cmake-3.28/bin/cmake
```

### Using CMake from Visual Studio

If you want to use the CMake that comes with Visual Studio:

```
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe
```

### Using Chocolatey/NuGet CMake

```
C:\ProgramData\Chocolatey\bin\cmake.exe
```

## Testing Your Configuration

### Windows
```batch
call cmake-path.bat
call detect-vs-generator.bat
```

### PowerShell
```powershell
.\build.ps1 -Target status
```

## Visual Studio Detection

The build scripts automatically detect available Visual Studio installations and use the newest version available. The detection order is:

1. Visual Studio 2022
2. Visual Studio 2019
3. Visual Studio 2017
4. Visual Studio 2015

If no Visual Studio is detected, the scripts will provide helpful error messages and installation suggestions.

This will show which CMake executable is being used.

## Troubleshooting

### "CMake not found" Error
- Check that your path in `cmake-path.txt` exists
- Verify the CMake executable is at that location
- Try using forward slashes (/) instead of backslashes (\) in paths
- Ensure the path doesn't contain spaces (use quotes if needed)

### Scripts Still Use Wrong CMake
- Clear any `CMAKE_PATH` environment variable
- Ensure `cmake-path.txt` has the correct path as the first non-comment line
- Restart your command prompt/PowerShell session

### Permission Issues
- Ensure you have read access to the CMake executable
- On Linux/Mac, ensure the CMake binary has execute permissions

## Advanced Usage

### Per-Build Configuration
You can create different cmake-path files for different build types:

```
cmake-path-release.txt  # For release builds
cmake-path-debug.txt    # For debug builds
```

Then copy the appropriate file to `cmake-path.txt` before building.

### CI/CD Integration
For automated builds, set the `CMAKE_PATH` environment variable in your CI configuration.