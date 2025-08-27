# Check if depot_tools is in PATH
try {
    Get-Command fetch -ErrorAction Stop | Out-Null
} catch {
    Write-Error "ERROR: depot_tools not found in PATH"
    Write-Host "Please install depot_tools and add it to your PATH:"
    Write-Host "https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html"
    exit 1
}

# First build Crashpad
Write-Host "Building Crashpad..."
& "$PSScriptRoot\build_crashpad_windows_msvc.ps1"

# Create build directory if it doesn't exist
Write-Host "Building MyCMakeCrasher..."
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}
Set-Location -Path "build"

# Configure with Debug mode for better symbol generation
Write-Host "Configuring with Debug symbols for better crash reporting..."
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build Debug configuration
cmake --build . --config Debug

Write-Host "Build complete. Run the application with: .\build\Debug\MyCMakeCrasher.exe"

# Return to root directory
Set-Location -Path ".." 