$ErrorActionPreference = "SilentlyContinue"

# Clean locked files
$debugDir = "Z:\Helbreath-3.82\Sources\Server\Debug_x64"
if (Test-Path $debugDir) {
    Remove-Item "$debugDir\*.obj" -Force
    Remove-Item "$debugDir\*.pdb" -Force
    Remove-Item "$debugDir\Server.tlog\*" -Force
}
Remove-Item "Z:\Helbreath-3.82\Sources\build_server.log" -Force

$ErrorActionPreference = "Stop"

# Build
& "Z:\Helbreath-3.82\Sources\build.ps1" -Target Server -Config Debug
