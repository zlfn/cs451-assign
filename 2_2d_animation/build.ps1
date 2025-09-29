# Build script for CSED451 C++20 modules project (Windows)
# Usage: .\build.ps1 [clean]

param(
    [string]$Action = ""
)

$ErrorActionPreference = "Stop"

$BUILD_DIR = "build"
$ROOT_DIR = ".."

# Function to print colored messages
function Write-Info {
    param([string]$Message)
    Write-Host "[INFO] $Message" -ForegroundColor Green
}

function Write-Warn {
    param([string]$Message)
    Write-Host "[WARN] $Message" -ForegroundColor Yellow
}

function Write-Error {
    param([string]$Message)
    Write-Host "[ERROR] $Message" -ForegroundColor Red
}

# Check if we have required tools
function Test-Dependencies {
    if (-not (Get-Command "cmake" -ErrorAction SilentlyContinue)) {
        Write-Error "CMake is not installed or not in PATH"
        exit 1
    }
}

# Setup Visual Studio environment
function Initialize-VSEnvironment {
    $vsPaths = @(
        "C:\Program Files\Microsoft Visual Studio\2022\Community",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise"
    )

    foreach ($vsPath in $vsPaths) {
        $devShellPath = Join-Path $vsPath "Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
        if (Test-Path $devShellPath) {
            Write-Info "Setting up Visual Studio 2022 environment..."
            Import-Module $devShellPath
            Enter-VsDevShell -VsInstallPath $vsPath -DevCmdArguments "-arch=amd64 -host_arch=amd64"
            return $true
        }
    }

    Write-Error "Visual Studio 2022 not found in expected locations"
    return $false
}

# Clean build directory if requested
if ($Action -eq "clean") {
    Write-Info "Cleaning build directory..."
    if (Test-Path $ROOT_DIR\$BUILD_DIR) {
        Remove-Item -Recurse -Force $ROOT_DIR\$BUILD_DIR
    }
}

# Check dependencies
Test-Dependencies

# Initialize VS environment
if (-not (Initialize-VSEnvironment)) {
    exit 1
}

# Navigate to root directory for CMake configuration
Set-Location $ROOT_DIR

# Create build directory at root level if it doesn't exist
if (-not (Test-Path $BUILD_DIR)) {
    Write-Info "Creating build directory..."
    New-Item -ItemType Directory -Path $BUILD_DIR | Out-Null
}

Set-Location $BUILD_DIR

# Configure with CMake using Visual Studio generator
Write-Info "Configuring with CMake (Visual Studio 2022 + C++20 modules)..."
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..

# Build the project
Write-Info "Building project (Release configuration)..."
cmake --build . --config Release

Write-Info "Build completed successfully!"
Write-Info "Executables are in: $BUILD_DIR\bin\Release\"
Write-Info "Compile commands: $BUILD_DIR\compile_commands.json"

# Add win-x64-msvc/bin to PATH for DLLs
$dllPath = Resolve-Path "..\win-x64-msvc\bin"
$env:Path = $env:Path + ";$dllPath"

Write-Info "DLL path added to environment: $dllPath"
Write-Info "You can now run executables from the bin\Release directory"
