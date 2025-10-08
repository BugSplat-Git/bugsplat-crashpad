param(
    [Parameter(Mandatory=$true)][string]$database,
    [Parameter(Mandatory=$true)][string]$appName,
    [Parameter(Mandatory=$true)][string]$version,
    [Parameter(Mandatory=$true)][string]$symbolsDir,
    [Parameter(Mandatory=$true)][string]$clientId,
    [Parameter(Mandatory=$true)][string]$clientSecret
)

$rootPath = Split-Path -Parent $PSScriptRoot
$toolsDir = Join-Path $rootPath "tools\windows"
$symbolUploader = Join-Path $toolsDir "symbol-upload-windows.exe"

# Create tools directory if it doesn't exist
if (-not (Test-Path $toolsDir)) {
    Write-Host "Creating tools directory: $toolsDir"
    New-Item -ItemType Directory -Path $toolsDir -Force | Out-Null
}

# Download the symbol uploader if it doesn't exist
if (-not (Test-Path $symbolUploader)) {
    Write-Host "Downloading symbol-upload-windows.exe..."
    try {
        Invoke-WebRequest -Uri "https://app.bugsplat.com/download/symbol-upload-windows.exe" -OutFile $symbolUploader
        if (-not (Test-Path $symbolUploader)) {
            Write-Error "Failed to download symbol-upload-windows.exe"
            exit 1
        }
        Write-Host "Successfully downloaded symbol-upload-windows.exe"
    }
    catch {
        Write-Error "Failed to download symbol-upload-windows.exe: $_"
        exit 1
    }
}

Write-Host "Uploading symbols from $symbolsDir"

& $symbolUploader -b $database -a $appName -v $version -d $symbolsDir -f "**/{MyCMakeCrasher,crash}.pdb" -i $clientId -s $clientSecret -m

if ($LASTEXITCODE -ne 0) {
    Write-Error "Symbol upload failed with exit code $LASTEXITCODE"
    exit $LASTEXITCODE
}

Write-Host "Symbol upload completed successfully" 