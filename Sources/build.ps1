# Unified Build Script for Helbreath 3.82
# Usage: build.ps1 [-Target <Game|Server|All>] [-Config <Debug|Release>]
#
# Examples:
#   build.ps1                           # Build Game with Debug-SFML x64 (default)
#   build.ps1 -Target Server            # Build Server
#   build.ps1 -Target Game -Config Release
#   build.ps1 -Target All               # Build all projects
param(
    [ValidateSet("Game", "Server", "All")]
    [string]$Target = "Game",

    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug"
)

$Renderer = "SFML"
$Platform = "x64"

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$msbuildPath = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
$solutionPath = Join-Path $scriptDir "Helbreath.sln"

# Determine configuration string and log file
$configString = "$Config-$Renderer"
switch ($Target) {
    "Server" { $logFile = Join-Path $scriptDir "build_server.log" }
    "All"    { $logFile = Join-Path $scriptDir "build_all.log" }
    default  { $logFile = Join-Path $scriptDir "build_game.log" }
}

# Delete old log
if (Test-Path $logFile) { Remove-Item $logFile -Force }

Write-Host "============================================"
Write-Host "Building: $Target | Config: $configString | Platform: $Platform"
Write-Host "============================================"

# Determine MSBuild target
$msbuildTarget = if ($Target -eq "All") { "" } else { "/t:$Target" }

# Run MSBuild
$msbuildArgs = @(
    $solutionPath
    "/p:Configuration=$configString"
    "/p:Platform=$Platform"
    "/nologo"
    "/v:minimal"
    "/consoleloggerparameters:Summary;ErrorsOnly;WarningsOnly"
    "/fileLogger"
    "/fileloggerparameters:LogFile=$logFile;Verbosity=normal;Encoding=UTF-8"
)
if ($msbuildTarget) { $msbuildArgs += $msbuildTarget }

& $msbuildPath @msbuildArgs
$exitCode = $LASTEXITCODE

# Show summary
Write-Host ""
if ($exitCode -eq 0) {
    Write-Host "BUILD SUCCEEDED" -ForegroundColor Green
} else {
    Write-Host "BUILD FAILED" -ForegroundColor Red
    Write-Host "Check log: $logFile"
    # Show errors from log
    if (Test-Path $logFile) {
        Write-Host ""
        Write-Host "=== Errors ===" -ForegroundColor Red
        Get-Content $logFile | Select-String "error" | Select-Object -First 20
    }
}

exit $exitCode
