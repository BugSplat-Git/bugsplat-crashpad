# Create third_party directory if it doesn't exist
if (-not (Test-Path "third_party")) {
    New-Item -ItemType Directory -Path "third_party" | Out-Null
}
Set-Location -Path "third_party"

# Check if crashpad directory already exists
if (-not (Test-Path "crashpad")) {
    Write-Host "Fetching Crashpad using depot_tools..."
    fetch crashpad
    Set-Location -Path "crashpad"
} else {
    Write-Host "Crashpad already exists, updating..."
    Set-Location -Path "crashpad"
    git checkout dialog-2 # TODO BG WIP branch on https://github.com/BugSplat-Git/crashpad
    git pull
    gclient sync
}

# Generate build files with GN for Debug configuration
Write-Host "Generating Debug build files with GN..."
gn gen out/win-debug --args="extra_cflags=\`"/MDd\`" is_debug=true" 

# Generate build files with GN for Release configuration
Write-Host "Generating Release build files with GN..."
gn gen out/win-release --args="extra_cflags=\`"/MD\`" is_debug=false" 

# Build Debug
Write-Host "Building Debug with Ninja..."
ninja -C out/win-debug

# Build Release
Write-Host "Building Release with Ninja..."
ninja -C out/win-release

Write-Host "Crashpad Debug and Release builds complete."

# Return to the original directory
Set-Location -Path "../../" 