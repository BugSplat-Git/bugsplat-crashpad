# Script to upload symbols to BugSplat for Windows

# Get script directory and root directory
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Split-Path -Parent $scriptDir

# Check if .env file exists
$envFile = Join-Path $rootDir ".env"
if (Test-Path $envFile) {
    Write-Host "Loading environment from .env file..."
    Get-Content $envFile | ForEach-Object {
        if ($_ -match '^\s*([^=\s]+)\s*=\s*(.*)$') {
            $key = $matches[1].Trim()
            $value = $matches[2].Trim()

            # Remove surrounding quotes if present
            if ($value -match '^"(.*)"$' -or $value -match "^'(.*)'$") {
                $value = $matches[1]
            }

            Set-Item -Path "Env:$key" -Value $value
        }
    }
} else {
    Write-Host "Warning: .env file not found. Make sure you have set BUGSPLAT_CLIENT_ID and BUGSPLAT_CLIENT_SECRET environment variables."
}

# Check if credentials are set
if (-not $env:BUGSPLAT_CLIENT_ID) {
    Write-Error "Error: BUGSPLAT_CLIENT_ID not found. Please set BUGSPLAT_CLIENT_ID environment variable or create a .env file."
    exit 1
}

if (-not $env:BUGSPLAT_CLIENT_SECRET) {
    Write-Error "Error: BUGSPLAT_CLIENT_SECRET not found. Please set BUGSPLAT_CLIENT_SECRET environment variable or create a .env file."
    exit 1
}

# Determine build directory and ensure we're looking in the Debug subdirectory
$buildDir = Join-Path $rootDir "build"
$debugDir = Join-Path $buildDir "Debug"
if (-not (Test-Path $debugDir)) {
    Write-Error "Error: Debug directory not found. Please build the project in Debug configuration first."
    exit 1
}

# Extract configuration from main.h
$mainH = Join-Path $rootDir "main.h"
if (-not (Test-Path $mainH)) {
    Write-Error "Error: main.h not found."
    exit 1
}

# Extract values from main.h
$mainContent = Get-Content $mainH
$database = ($mainContent | Select-String '#define BUGSPLAT_DATABASE' | ForEach-Object { if ($_.Line -match '"([^"]+)"') { $matches[1] } })
$appName = ($mainContent | Select-String '#define BUGSPLAT_APP_NAME' | ForEach-Object { if ($_.Line -match '"([^"]+)"') { $matches[1] } })
$appVersion = ($mainContent | Select-String '#define BUGSPLAT_APP_VERSION' | ForEach-Object { if ($_.Line -match '"([^"]+)"') { $matches[1] } })

if (-not $database) {
    Write-Error "Error: Could not extract BUGSPLAT_DATABASE from main.h."
    exit 1
}
if (-not $appName) {
    Write-Error "Error: Could not extract BUGSPLAT_APP_NAME from main.h."
    exit 1
}
if (-not $appVersion) {
    Write-Error "Error: Could not extract BUGSPLAT_APP_VERSION from main.h."
    exit 1
}

Write-Host "Database: $database"
Write-Host "App Name: $appName"
Write-Host "Version: $appVersion"

# Execute the PowerShell script for Windows symbol upload
Write-Host "Running Windows symbol upload script..."
$symbolUploadScript = Join-Path $scriptDir "symbol_upload_windows.ps1"
& $symbolUploadScript -database $database -appName $appName -version $appVersion -symbolsDir $debugDir -clientId $env:BUGSPLAT_CLIENT_ID -clientSecret $env:BUGSPLAT_CLIENT_SECRET

if ($LASTEXITCODE -ne 0) {
    Write-Error "Symbol upload failed. Please check your credentials and network connection."
    exit $LASTEXITCODE
} else {
    Write-Host "Symbol upload completed successfully."
} 