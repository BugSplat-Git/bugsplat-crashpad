# Build Crashpad at a pinned commit with GN + depot_tools on Windows.
#
#   build.ps1 -Work <dir> -Commit <sha> -Out <name> -ArgsGn "<args.gn contents>" -Targets "client ..."
#
# depot_tools' launchers are .bat files. Each one runs in its own cmd so a launcher that ends with
# a bare `exit` cannot swallow the rest of the script, and every exit code is checked. GN args go
# through args.gn instead of shell quoting.
param(
  [Parameter(Mandatory)] [string] $Work,
  [Parameter(Mandatory)] [string] $Commit,
  [Parameter(Mandatory)] [string] $Out,
  [Parameter(Mandatory)] [string] $ArgsGn,
  [Parameter(Mandatory)] [string] $Targets
)
$ErrorActionPreference = "Stop"

function Invoke-Native([string]$Cmd) {
  Write-Host ">> $Cmd"
  cmd /c $Cmd
  if ($LASTEXITCODE -ne 0) { throw "failed ($LASTEXITCODE): $Cmd" }
}

$env:DEPOT_TOOLS_WIN_TOOLCHAIN = "0"
$env:DEPOT_TOOLS_UPDATE = "0"
git config --global depot-tools.allowGlobalGitConfig false

New-Item -ItemType Directory -Force $Work | Out-Null
$Work = (Resolve-Path $Work).Path
if (-not (Test-Path "$Work\depot_tools")) {
  Invoke-Native "git clone --depth 1 https://chromium.googlesource.com/chromium/tools/depot_tools.git $Work\depot_tools"
}
$env:PATH = "$Work\depot_tools;$env:PATH"
Invoke-Native "call gclient.bat --version"

if (-not (Test-Path "$Work\crashpad")) {
  Push-Location $Work
  try { Invoke-Native "call fetch.bat --no-history crashpad" } finally { Pop-Location }
}
Set-Location "$Work\crashpad"
Invoke-Native "git fetch --depth 1 origin $Commit"
Invoke-Native "git checkout -q $Commit"
Invoke-Native "call gclient.bat sync"

New-Item -ItemType Directory -Force "out\$Out" | Out-Null
# One assignment per line; GN accepts them on one line too but this reads better in the log.
$lines = $ArgsGn -split '\s+(?=[A-Za-z_][A-Za-z0-9_]*=)'
Set-Content -Path "out\$Out\args.gn" -Value ($lines -join "`n")
Write-Host "--- out\$Out\args.gn"; Get-Content "out\$Out\args.gn"; Write-Host "---"
Invoke-Native "call gn.bat gen out/$Out"
Invoke-Native "call ninja.bat -C out/$Out $Targets"
if (-not (Test-Path "out\$Out\obj\client\client.lib")) { throw "client.lib was not produced" }
