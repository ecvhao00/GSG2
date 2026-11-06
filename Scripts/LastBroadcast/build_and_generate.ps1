param([switch]$SkipScene)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$engineRoot = 'C:\Program Files\Epic Games\UE_5.8'
& (Join-Path $engineRoot 'Engine\Build\BatchFiles\Build.bat') EX_1Editor Win64 Development "-Project=$projectRoot\EX_1.uproject" -WaitMutex -NoHotReloadFromIDE -Utf8Output
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (-not $SkipScene) {
    & (Join-Path $engineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe') "$projectRoot\EX_1.uproject" -run=pythonscript "-script=$PSScriptRoot\build_school.py" -unattended -nosplash -nullrhi "-abslog=$projectRoot\Saved\Logs\LastBroadcast_SceneBuild.log" *> "$projectRoot\Saved\Logs\LastBroadcast_SceneBuild_console.log"
    if ($LASTEXITCODE -ne 0) { Get-Content -LiteralPath "$projectRoot\Saved\Logs\LastBroadcast_SceneBuild.log" -Tail 30; exit $LASTEXITCODE }
}
Write-Output 'EX_1 LastBroadcast build completed.'
