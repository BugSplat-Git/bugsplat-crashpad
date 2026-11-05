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
    git checkout main
    git pull
    gclient sync
}

# CRASHPAD_COMMIT pins the build to one commit (the monthly workflow builds every platform from
# the same one). Unset, the build uses the head of main.
if ($env:CRASHPAD_COMMIT) {
    Write-Host "Checking out Crashpad $env:CRASHPAD_COMMIT..."
    git checkout -q $env:CRASHPAD_COMMIT
    if ($LASTEXITCODE -ne 0) { throw "git checkout $env:CRASHPAD_COMMIT failed" }
    gclient sync
    if ($LASTEXITCODE -ne 0) { throw "gclient sync failed" }
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