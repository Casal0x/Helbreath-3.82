# Build Game (Client)
# Usage: build_game.ps1 [-Config <Debug-DDraw|Debug-SFML|Release-DDraw|Release-SFML>]
# Default: Debug-SFML
param(
    [ValidateSet("Debug-DDraw", "Debug-SFML", "Release-DDraw", "Release-SFML")]
    [string]$Config = "Debug-SFML"
)

$ErrorActionPreference = "Continue"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$logFile = Join-Path $scriptDir "build_game.log"

Write-Host "Building Game (Client) - Configuration: $Config"

# Delete old log if it exists
if (Test-Path $logFile) {
    Remove-Item $logFile -Force
}

# Run MSBuild and tee output to log file
$msbuildPath = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
$solutionPath = Join-Path $scriptDir "Helbreath.sln"

& $msbuildPath $solutionPath /t:Game /p:Configuration=$Config /p:Platform=x86 2>&1 | Tee-Object -FilePath $logFile
