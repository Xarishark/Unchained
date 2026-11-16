# Records a play run for the tests: starts the game with UNCHAINED_INPUT_RECORD, and once it is closed copies the
# presses (input_record.txt beside the exe) to tools\tests\runs\<Name>.txt. A scenario replays it with
# input_file = "runs/<Name>.txt" (scenarios.toml). Timing counts from the game's first pad read, as the replay's does,
# and the replay starts from the same save, so play from the start without waiting on anything that varies.
#
#   powershell -ExecutionPolicy Bypass -File tools\tests\record.ps1 -Name lobby
param(
    [Parameter(Mandatory)][string]$Name,
    [string]$Exe = "$PSScriptRoot\..\..\out\build\release\Unchained.exe"
)
$ErrorActionPreference = "Stop"
$Exe = (Resolve-Path $Exe).Path
$dir = Split-Path $Exe
if (Get-Process Unchained -ErrorAction SilentlyContinue) { Write-Error "Unchained is already running: close it first"; exit 2 }
$record = Join-Path $dir "input_record.txt"
Remove-Item $record -ErrorAction SilentlyContinue
$env:UNCHAINED_INPUT_RECORD = $record
$env:XBE_AUTOSTART = "1"
Remove-Item Env:UNCHAINED_INPUT -ErrorAction SilentlyContinue
Remove-Item Env:UNCHAINED_OFFSCREEN -ErrorAction SilentlyContinue
Write-Host "Recording. Play, then close the game window."
Start-Process -FilePath $Exe -WorkingDirectory $dir -Wait
# a code build relaunches the game: wait for that copy too
while (Get-Process Unchained -ErrorAction SilentlyContinue) { Start-Sleep -Milliseconds 500 }
if (-not (Test-Path $record)) { Write-Error "no input was recorded"; exit 1 }
$runs = Join-Path $PSScriptRoot "runs"
New-Item -ItemType Directory -Force $runs | Out-Null
$out = Join-Path $runs "$Name.txt"
Copy-Item $record $out -Force
$n = (Get-Content $out | Measure-Object -Line).Lines
Write-Host "Saved $n presses to $out"
