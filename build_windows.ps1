# Audio Cleaner - Windows Build Script with OpenCL Support
# Run this script in PowerShell as Administrator

param(
    [switch]$InstallDependencies = $false,
    [switch]$EnableOpenCL = $true,
    [switch]$BuildOnly = $false,
    [switch]$DisableOpenCL = $false
)

Write-Host "=== Audio Cleaner Windows Build Script ===" -ForegroundColor Cyan
Write-Host ""

# Check if running as Administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin -and $InstallDependencies) {
    Write-Host "ERROR: Installing dependencies requires Administrator privileges" -ForegroundColor Red
    Write-Host "Please run PowerShell as Administrator or use -BuildOnly flag" -ForegroundColor Yellow
    exit 1
}

# Install dependencies if requested
if ($InstallDependencies) {
    Write-Host "Installing build dependencies..." -ForegroundColor Yellow
    
    # Check for Chocolatey
    if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
        Write-Host "Installing Chocolatey..." -ForegroundColor Yellow
        Set-ExecutionPolicy Bypass -Scope Process -Force
        [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
        iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
    }
    
    # Install build tools
    Write-Host "Installing CMake and build tools..." -ForegroundColor Yellow
    choco install -y cmake git
    
    # Check for CUDA/OpenCL
    if ($EnableOpenCL) {
        Write-Host "Checking for NVIDIA CUDA Toolkit..." -ForegroundColor Yellow
        if (-not (Test-Path "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA")) {
            Write-Host "CUDA Toolkit not found. Please install from:" -ForegroundColor Yellow
            Write-Host "https://developer.nvidia.com/cuda-downloads" -ForegroundColor Cyan
            Write-Host ""
            Write-Host "Or install via Chocolatey:" -ForegroundColor Yellow
            Write-Host "choco install -y cuda" -ForegroundColor Cyan
            $response = Read-Host "Continue without CUDA? (y/n)"
            if ($response -ne 'y') {
                exit 1
            }
            $EnableOpenCL = $false
        }
    }
    
    # Install vcpkg for audio libraries
    if (-not (Test-Path "C:\vcpkg")) {
        Write-Host "Installing vcpkg..." -ForegroundColor Yellow
        cd C:\
        git clone https://github.com/Microsoft/vcpkg.git
        cd vcpkg
        .\bootstrap-vcpkg.bat
        
        Write-Host "Installing audio libraries..." -ForegroundColor Yellow
        .\vcpkg install libmp3lame:x64-windows
        .\vcpkg install libvorbis:x64-windows
        .\vcpkg install libogg:x64-windows
        .\vcpkg install ffmpeg:x64-windows
    }
}

# Navigate to project directory
$projectDir = Split-Path -Parent $MyInvocation.MyCommand.Path
cd $projectDir

Write-Host "Project directory: $projectDir" -ForegroundColor Cyan
Write-Host ""

# Create build directory
$buildDir = "build-windows"
if (Test-Path $buildDir) {
    Write-Host "Cleaning existing build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $buildDir
}

Write-Host "Creating build directory..." -ForegroundColor Yellow
New-Item -ItemType Directory -Path $buildDir | Out-Null
cd $buildDir

# Configure CMake
Write-Host ""
Write-Host "Configuring CMake..." -ForegroundColor Yellow
$cmakeArgs = @()

# Detect and set generator
$generator = $null
if (Get-Command "ninja" -ErrorAction SilentlyContinue) {
    $generator = "Ninja"
    Write-Host "Using Ninja build system" -ForegroundColor Cyan
} elseif (Test-Path "C:\Program Files\Microsoft Visual Studio\2022\*\Common7\IDE\devenv.exe") {
    $generator = "Visual Studio 17 2022"
    Write-Host "Using Visual Studio 2022" -ForegroundColor Cyan
} elseif (Test-Path "C:\Program Files (x86)\Microsoft Visual Studio\2019\*\Common7\IDE\devenv.exe") {
    $generator = "Visual Studio 16 2019"
    Write-Host "Using Visual Studio 2019" -ForegroundColor Cyan
} else {
    Write-Host "WARNING: No suitable build system found. Trying MinGW..." -ForegroundColor Yellow
    $generator = "MinGW Makefiles"
}

if ($generator) {
    $cmakeArgs += "-G"
    $cmakeArgs += $generator
}

if ($DisableOpenCL) {
    Write-Host "OpenCL support: DISABLED (forced)" -ForegroundColor Yellow
    $cmakeArgs += "-DENABLE_OPENCL=OFF"
} elseif ($EnableOpenCL) {
    Write-Host "OpenCL support: ENABLED" -ForegroundColor Green
    $cmakeArgs += "-DENABLE_OPENCL=ON"
} else {
    Write-Host "OpenCL support: DISABLED" -ForegroundColor Yellow
    $cmakeArgs += "-DENABLE_OPENCL=OFF"
}

# Add vcpkg toolchain if available
if (Test-Path "C:\vcpkg\scripts\buildsystems\vcpkg.cmake") {
    $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
}

$cmakeArgs += ".."

Write-Host "CMake command: cmake $($cmakeArgs -join ' ')" -ForegroundColor Cyan
& cmake $cmakeArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "ERROR: CMake configuration failed" -ForegroundColor Red
    exit 1
}

# Build
Write-Host ""
Write-Host "Building Audio Cleaner..." -ForegroundColor Yellow
cmake --build . --config Release

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "ERROR: Build failed" -ForegroundColor Red
    exit 1
}

# Success
Write-Host ""
Write-Host "=== Build Successful ===" -ForegroundColor Green
Write-Host ""
Write-Host "Executable location: $buildDir\Release\audio_cleaner.exe" -ForegroundColor Cyan
Write-Host ""

# Check for OpenCL
if ($EnableOpenCL) {
    Write-Host "Checking OpenCL availability..." -ForegroundColor Yellow
    
    # Try to detect OpenCL platforms
    $openclCheck = @"
#include <CL/cl.h>
#include <iostream>
int main() {
    cl_uint numPlatforms;
    clGetPlatformIDs(0, NULL, &numPlatforms);
    std::cout << "OpenCL Platforms: " << numPlatforms << std::endl;
    return 0;
}
"@
    
    Write-Host "OpenCL support compiled in. Test with --force-gpu flag." -ForegroundColor Green
}

# Display usage
Write-Host ""
Write-Host "=== Usage Examples ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Basic usage:" -ForegroundColor Yellow
Write-Host "  .\Release\audio_cleaner.exe -i input.wav -o output.mp3" -ForegroundColor White
Write-Host ""
Write-Host "Force GPU acceleration:" -ForegroundColor Yellow
Write-Host "  .\Release\audio_cleaner.exe -i input.wav -o output.mp3 --force-gpu" -ForegroundColor White
Write-Host ""
Write-Host "Check backend selection:" -ForegroundColor Yellow
Write-Host "  .\Release\audio_cleaner.exe -i input.wav -o output.mp3 --backend auto" -ForegroundColor White
Write-Host ""

# Offer to run test
if (-not $BuildOnly) {
    Write-Host "Would you like to run a test? (y/n): " -ForegroundColor Yellow -NoNewline
    $runTest = Read-Host
    
    if ($runTest -eq 'y') {
        Write-Host ""
        Write-Host "Enter input WAV file path: " -ForegroundColor Yellow -NoNewline
        $inputFile = Read-Host
        
        if (Test-Path $inputFile) {
            $outputFile = [System.IO.Path]::ChangeExtension($inputFile, ".mp3")
            $outputFile = [System.IO.Path]::GetFileNameWithoutExtension($inputFile) + "_cleaned.mp3"
            
            Write-Host ""
            Write-Host "Processing: $inputFile" -ForegroundColor Cyan
            Write-Host "Output: $outputFile" -ForegroundColor Cyan
            Write-Host ""
            
            .\Release\audio_cleaner.exe -i $inputFile -o $outputFile --force-gpu
            
            if ($LASTEXITCODE -eq 0) {
                Write-Host ""
                Write-Host "Processing complete! Output saved to: $outputFile" -ForegroundColor Green
            } else {
                Write-Host ""
                Write-Host "Processing failed with exit code: $LASTEXITCODE" -ForegroundColor Red
            }
        } else {
            Write-Host "File not found: $inputFile" -ForegroundColor Red
        }
    }
}

Write-Host ""
Write-Host "Done!" -ForegroundColor Green
