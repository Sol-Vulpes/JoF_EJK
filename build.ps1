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

function Test-Prerequisites {
    # Check for CMake
    try {
        $cmakeVersion = & cmake --version 2>$null | Select-String -Pattern "cmake version" | ForEach-Object { $_.Line -replace "cmake version ", "" }
        Write-Host "✓ CMake found: $cmakeVersion" -ForegroundColor Green
    } catch {
        Write-Error "✗ CMake not found. Please install CMake 3.1 or later."
        exit 1
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

function Invoke-Win64Build {
    Write-Host "Building for Windows 64-bit ($Configuration)..." -ForegroundColor Yellow

    $buildDir = "build-win64-$Configuration"
    if (!(Test-Path $buildDir)) {
        New-Item -ItemType Directory -Path $buildDir | Out-Null
    }

    Push-Location $buildDir

    try {
        # Configure
        Write-Host "Running CMake configuration..."
        & cmake -G $Generator -A x64 -DCMAKE_BUILD_TYPE=$Configuration ..

        if ($LASTEXITCODE -ne 0) {
            throw "CMake configuration failed"
        }

        # Build
        Write-Host "Building..."
        & cmake --build . --config $Configuration

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
    Write-Host "Building for Windows 32-bit ($Configuration)..." -ForegroundColor Yellow

    $buildDir = "build-win32-$Configuration"
    if (!(Test-Path $buildDir)) {
        New-Item -ItemType Directory -Path $buildDir | Out-Null
    }

    Push-Location $buildDir

    try {
        # Configure
        Write-Host "Running CMake configuration..."
        & cmake -G $Generator -A Win32 -DCMAKE_BUILD_TYPE=$Configuration ..

        if ($LASTEXITCODE -ne 0) {
            throw "CMake configuration failed"
        }

        # Build
        Write-Host "Building..."
        & cmake --build . --config $Configuration

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
Test-Prerequisites

switch ($Target) {
    "win64" { Invoke-Win64Build }
    "win32" { Invoke-Win32Build }
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