param(
    [Parameter(Mandatory=$false)]
    [ValidateSet("win64", "win32", "linux", "clean", "status")]
    [string]$Target = "win64",

    [Parameter(Mandatory=$false)]
    [ValidateSet("Release", "Debug", "RelWithDebInfo")]
    [string]$Configuration = "Release",

    [Parameter(Mandatory=$false)]
    [string]$Generator = "Visual Studio 16 2019"
)

$ErrorActionPreference = "Stop"

function Write-Header {
    Write-Host "JoF EternalJK Build Script" -ForegroundColor Cyan
    Write-Host "=========================" -ForegroundColor Cyan
    Write-Host ""
}

function Get-CMakeCommand {
    # Try to get cmake path from multiple sources
    $cmakePath = $null

    # 1. Check cmake-path.txt file
    if (Test-Path "cmake-path.txt") {
        $content = Get-Content "cmake-path.txt" | Where-Object { $_ -notmatch "^#" -and $_.Trim() -ne "" }
        if ($content) {
            $cmakePath = $content[0].Trim()
        }
    }

    # 2. Check CMAKE_PATH environment variable
    if (!$cmakePath -and $env:CMAKE_PATH) {
        $cmakePath = $env:CMAKE_PATH
    }

    # 3. Default to cmake in PATH
    if (!$cmakePath) {
        $cmakePath = "cmake"
    }

    return $cmakePath
}

function Test-Prerequisites {
    $cmakeCmd = Get-CMakeCommand

    # Check for CMake
    try {
        $cmakeVersion = & $cmakeCmd --version 2>$null | Select-String -Pattern "cmake version" | ForEach-Object { $_.Line -replace "cmake version ", "" }
        Write-Host "✓ CMake found: $cmakeVersion ($cmakeCmd)" -ForegroundColor Green
    } catch {
        Write-Error "✗ CMake not found at: $cmakeCmd"
        Write-Host "Please either:" -ForegroundColor Yellow
        Write-Host "1. Add cmake to your PATH" -ForegroundColor Yellow
        Write-Host "2. Set CMAKE_PATH environment variable" -ForegroundColor Yellow
        Write-Host "3. Edit cmake-path.txt with the full path to cmake.exe" -ForegroundColor Yellow
        exit 1
    }

    return $cmakeCmd
}

    # Check for Visual Studio (if building for Windows)
    if ($Target -like "win*") {
        $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
        if (Test-Path $vswhere) {
            $vsPath = & $vswhere -latest -property installationPath
            Write-Host "✓ Visual Studio found at: $vsPath" -ForegroundColor Green
        } else {
            Write-Warning "Visual Studio not found via vswhere. Build may still work."
        }
    }
}

function Get-BestVSGenerator {
    $generators = & $cmakeCmd --help 2>$null | Select-String -Pattern "Visual Studio" | ForEach-Object {
        $_.Line.Trim() -replace '\[arch\]', '' -replace '.*= ', ''
    }

    # Prefer newer versions first
    $vs2022 = $generators | Where-Object { $_ -like "*2022*" }
    $vs2019 = $generators | Where-Object { $_ -like "*2019*" }
    $vs2017 = $generators | Where-Object { $_ -like "*2017*" }
    $vs2015 = $generators | Where-Object { $_ -like "*2015*" }

    if ($vs2022) { return $vs2022 }
    if ($vs2019) { return $vs2019 }
    if ($vs2017) { return $vs2017 }
    if ($vs2015) { return $vs2015 }

    throw "No compatible Visual Studio generator found. Please install Visual Studio 2015 or later."
}

function Invoke-Win64Build {
    param([string]$CMakeCmd)

    Write-Host "Building for Windows 64-bit ($Configuration)..." -ForegroundColor Yellow

    try {
        $vsGenerator = Get-BestVSGenerator
        Write-Host "Using generator: $vsGenerator" -ForegroundColor Cyan
    } catch {
        Write-Error $_.Exception.Message
        return
    }

    $buildDir = "build-win64-$Configuration"
    if (!(Test-Path $buildDir)) {
        New-Item -ItemType Directory -Path $buildDir | Out-Null
    }

    Push-Location $buildDir

    try {
        # Clean previous CMake cache if it exists
        if (Test-Path "CMakeCache.txt") {
            Write-Host "Cleaning previous CMake cache..."
            Remove-Item "CMakeFiles" -Recurse -Force -ErrorAction SilentlyContinue
            Remove-Item "CMakeCache.txt" -Force -ErrorAction SilentlyContinue
        }

        # Configure
        Write-Host "Running CMake configuration..."
        & $CMakeCmd -G $vsGenerator -A x64 -DCMAKE_BUILD_TYPE=$Configuration ..

        if ($LASTEXITCODE -ne 0) {
            throw "CMake configuration failed"
        }

        # Build
        Write-Host "Building..."
        & $CMakeCmd --build . --config $Configuration

        if ($LASTEXITCODE -ne 0) {
            throw "Build failed"
        }

        Write-Host "✓ Build completed successfully!" -ForegroundColor Green
        Write-Host "Output files are in: $buildDir\$Configuration\" -ForegroundColor Cyan

    } finally {
        Pop-Location
    }
}

function Invoke-Win32Build {
    param([string]$CMakeCmd)

    Write-Host "Building for Windows 32-bit ($Configuration)..." -ForegroundColor Yellow

    try {
        $vsGenerator = Get-BestVSGenerator
        Write-Host "Using generator: $vsGenerator" -ForegroundColor Cyan
    } catch {
        Write-Error $_.Exception.Message
        return
    }

    $buildDir = "build-win32-$Configuration"
    if (!(Test-Path $buildDir)) {
        New-Item -ItemType Directory -Path $buildDir | Out-Null
    }

    Push-Location $buildDir

    try {
        # Clean previous CMake cache if it exists
        if (Test-Path "CMakeCache.txt") {
            Write-Host "Cleaning previous CMake cache..."
            Remove-Item "CMakeFiles" -Recurse -Force -ErrorAction SilentlyContinue
            Remove-Item "CMakeCache.txt" -Force -ErrorAction SilentlyContinue
        }

        # Configure
        Write-Host "Running CMake configuration..."
        & $CMakeCmd -G $vsGenerator -A Win32 -DCMAKE_BUILD_TYPE=$Configuration ..

        if ($LASTEXITCODE -ne 0) {
            throw "CMake configuration failed"
        }

        # Build
        Write-Host "Building..."
        & $CMakeCmd --build . --config $Configuration

        if ($LASTEXITCODE -ne 0) {
            throw "Build failed"
        }

        Write-Host "✓ Build completed successfully!" -ForegroundColor Green
        Write-Host "Output files are in: $buildDir\$Configuration\" -ForegroundColor Cyan

    } finally {
        Pop-Location
    }
}

function Invoke-LinuxBuild {
    Write-Host "Building for Linux..." -ForegroundColor Yellow

    # Check if we're on Linux or have WSL
    $isLinux = $env:OS -notlike "*Windows*"
    $hasWsl = Test-Path "C:\Windows\System32\bash.exe"

    if ($isLinux) {
        Write-Host "Running native Linux build..."
        & ./build-linux.sh
    } elseif ($hasWsl) {
        Write-Host "Running Linux build via WSL..."
        & bash -c "./build-linux.sh"
    } else {
        Write-Error "Linux build requires either native Linux or WSL."
        exit 1
    }
}

function Clear-BuildDirectories {
    Write-Host "Cleaning build directories..." -ForegroundColor Yellow

    Get-ChildItem -Directory -Filter "build*" | ForEach-Object {
        Write-Host "Removing $($_.Name)..."
        Remove-Item $_.FullName -Recurse -Force
    }

    Write-Host "✓ Build directories cleaned." -ForegroundColor Green
}

function Show-Status {
    Write-Host "Build Status:" -ForegroundColor Cyan
    Write-Host "=============" -ForegroundColor Cyan

    $buildDirs = Get-ChildItem -Directory -Filter "build*"

    if ($buildDirs.Count -eq 0) {
        Write-Host "No build directories found." -ForegroundColor Yellow
        return
    }

    foreach ($dir in $buildDirs) {
        Write-Host "$($dir.Name):" -ForegroundColor White

        # Check for executables
        $exe64 = Join-Path $dir.FullName "Release\eternaljk.x86_64.exe"
        $exe32 = Join-Path $dir.FullName "Release\eternaljk.x86.exe"

        if (Test-Path $exe64) {
            Write-Host "  ✓ 64-bit executable found" -ForegroundColor Green
        } elseif (Test-Path $exe32) {
            Write-Host "  ✓ 32-bit executable found" -ForegroundColor Green
        } else {
            Write-Host "  - No executable found" -ForegroundColor Red
        }
    }
}

# Main execution
Write-Header
$cmakeCmd = Test-Prerequisites

switch ($Target) {
    "win64" { Invoke-Win64Build -CMakeCmd $cmakeCmd }
    "win32" { Invoke-Win32Build -CMakeCmd $cmakeCmd }
    "linux" { Invoke-LinuxBuild }
    "clean" { Clear-BuildDirectories }
    "status" { Show-Status }
    default {
        Write-Error "Invalid target: $Target"
        exit 1
    }
}

Write-Host ""
Write-Host "Build script completed." -ForegroundColor Cyan