# Builds Crashpad at -Commit into <Work>\crashpad\out\<Out> with depot_tools at
# $env:DEPOT_TOOLS_COMMIT. depot_tools' launchers are .bat files, so each runs in its own cmd: a
# launcher ending in a bare `exit` can't end this script, and every exit code is checked.
#
#   build.ps1 -Work <dir> -Commit <sha> -Out <name> -ArgsGn "<args.gn contents>" -Targets "client ..."
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

if (-not $env:DEPOT_TOOLS_COMMIT) { throw "set DEPOT_TOOLS_COMMIT to the depot_tools commit to build with" }
$env:DEPOT_TOOLS_WIN_TOOLCHAIN = "0"
git config --global depot-tools.allowGlobalGitConfig false

New-Item -ItemType Directory -Force $Work | Out-Null
$Work = (Resolve-Path $Work).Path
$DepotTools = "$Work\depot_tools"
if (-not (Test-Path $DepotTools)) {
  Invoke-Native "git init -q $DepotTools"
  Invoke-Native "git -C $DepotTools remote add origin https://chromium.googlesource.com/chromium/tools/depot_tools.git"
}
Invoke-Native "git -C $DepotTools fetch -q --depth 1 origin $env:DEPOT_TOOLS_COMMIT"
Invoke-Native "git -C $DepotTools checkout -q FETCH_HEAD"
# Otherwise every gclient call moves depot_tools to origin/main.
$env:DEPOT_TOOLS_UPDATE = "0"
$env:PATH = "$DepotTools;$env:PATH"
Invoke-Native "call $DepotTools\cipd_bin_setup.bat"
Invoke-Native "call $DepotTools\bootstrap\win_tools.bat"
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
$lines = $ArgsGn -split '\s+(?=[A-Za-z_][A-Za-z0-9_]*=)'
Set-Content -Path "out\$Out\args.gn" -Value ($lines -join "`n")
Write-Host "--- out\$Out\args.gn"; Get-Content "out\$Out\args.gn"; Write-Host "---"
Invoke-Native "call gn.bat gen out/$Out"
Invoke-Native "call ninja.bat -C out/$Out $Targets"
if (-not (Test-Path "out\$Out\obj\client\client.lib")) { throw "client.lib was not produced" }
