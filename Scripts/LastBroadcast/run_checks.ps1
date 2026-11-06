$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$startedAt = Get-Date
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' "$projectRoot\EX_1.uproject" /Game/LastBroadcast/Maps/L_LastBroadcast -game -nullrhi -nosound -unattended -nosplash -LBFunctionalTest "-abslog=$projectRoot\Saved\Logs\LastBroadcast_Functional.log" *> "$projectRoot\Saved\Logs\LastBroadcast_Functional_console.log"
$reportPath = "$projectRoot\Saved\LastBroadcast\functional_report.json"
if (-not (Test-Path -LiteralPath $reportPath)) { throw 'Game did not produce a regression report. Check the functional log.' }
if ((Get-Item -LiteralPath $reportPath).LastWriteTime -lt $startedAt) { throw 'Game did not produce a fresh report. Check the functional log for a crash.' }
$report = Get-Content -LiteralPath $reportPath -Raw | ConvertFrom-Json
$report | Select-Object suite,passed,failed | Format-List
if ($report.failed -gt 0) { $report.checks | Where-Object { -not $_.passed } | Format-Table; exit 1 }
exit 0
