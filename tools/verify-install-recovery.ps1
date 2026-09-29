<#
.SYNOPSIS
    Verifies Bundle install transfer behaviour on a UART-attached device:
    chunked install, hash-mismatch rejection, and recovery after an aborted install.

.DESCRIPTION
    Covers the remaining PR review asks that the integrity loop does not:
      * a full install over the UART bridge, with the Host's install log lines captured
      * a tampered Bundle must be rejected without touching the installed App
      * an install killed mid-flight must leave the device reachable and installable again

    Fault injection is opt-in. A completed install of the App under test is
    replaced by the same Bundle, so the device ends in the state it started in.

.EXAMPLE
    pwsh -File tools/verify-install-recovery.ps1 -Port COM3 -Bundle build/my-app.bundle.bin

.EXAMPLE
    pwsh -File tools/verify-install-recovery.ps1 -Port COM3 -Bundle build/my-app.bundle.bin -Tamper -InterruptAfterMs 1500
#>
[CmdletBinding()]
param(
    [string]$Port = 'COM3',
    [Parameter(Mandatory = $true)][string]$Bundle,
    [string]$CliPath = '',
    [string]$OutDir = "$env:TEMP\mpx-uart-verify",
    [switch]$Tamper,
    [switch]$Interrupt,
    [int]$InterruptAfterMs = 1500,
    [int]$WaitTimeoutSeconds = 180
)

$ErrorActionPreference = 'Stop'

if (-not $CliPath) {
    $CliPath = if ($env:MICROPIXEL_CLI) { $env:MICROPIXEL_CLI }
               else { Join-Path $env:LOCALAPPDATA 'MicroPixel\bin\micropixel.exe' }
}
if (-not (Test-Path -LiteralPath $CliPath)) { throw "micropixel CLI not found: $CliPath" }
if (-not (Test-Path -LiteralPath $Bundle)) { throw "Bundle not found: $Bundle" }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

function Invoke-Mpx {
    param([string[]]$Arguments)
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $raw = & $CliPath @Arguments 2>&1 | Out-String
    $sw.Stop()
    [pscustomobject]@{ Exit = $LASTEXITCODE; Output = $raw.Trim(); Seconds = [math]::Round($sw.Elapsed.TotalSeconds, 1) }
}

function Get-AppEntry {
    param([string]$AppId)
    $r = Invoke-Mpx -Arguments @('--port', $Port, 'app', 'list')
    if ($r.Exit -ne 0) { throw "app list failed: $($r.Output)" }
    ($r.Output | ConvertFrom-Json).apps | Where-Object { $_.appId -eq $AppId }
}

function Get-HostLogTail {
    param([int]$Lines = 40)
    (Invoke-Mpx -Arguments @('--port', $Port, 'logs', '-n', "$Lines", '--source', 'host')).Output
}

$report = [ordered]@{
    port = $Port; bundle = $Bundle; bundleBytes = (Get-Item -LiteralPath $Bundle).Length
    bundleSha256 = (Get-FileHash -LiteralPath $Bundle -Algorithm SHA256).Hash.ToLower()
    appId = ''; installedBefore = ''; installedAfter = ''
    goodInstall = $null; tamper = $null; interrupt = $null; notes = @(); verdict = ''
}

$status = Invoke-Mpx -Arguments @('--port', $Port, 'device', 'status')
if ($status.Exit -ne 0) { throw "device status failed on $Port: $($status.Output)" }
$snapshot = $status.Output | ConvertFrom-Json
Write-Host "board=$($snapshot.hardware.board) fw=$($snapshot.firmware.version) online=$($snapshot.online)" -ForegroundColor Cyan

# The App id lives in the Bundle header right after the 9-byte "PXBNDL\0\1" prefix area.
$bytes = [System.IO.File]::ReadAllBytes($Bundle)
$ascii = [System.Text.Encoding]::ASCII.GetString($bytes, 0, [Math]::Min(256, $bytes.Length))
$appIdMatch = [regex]::Match($ascii, '[a-z0-9][a-z0-9._-]{3,63}')
if (-not $appIdMatch.Success) { throw 'could not read an App id from the Bundle header' }
$report.appId = $appIdMatch.Value
Write-Host "app id=$($report.appId) bundle=$($report.bundleBytes) bytes sha256=$($report.bundleSha256.Substring(0,16))..." -ForegroundColor Cyan

$before = Get-AppEntry -AppId $report.appId
if (-not $before) { throw "App $($report.appId) is not installed; pick an installed App for this test" }
$report.installedBefore = $before.sha256
Write-Host "installed before: v$($before.version) $($before.sizeBytes) bytes sha256=$($before.sha256.Substring(0,16))..."

# --- 1. clean chunked install -------------------------------------------------
Write-Host "`n== 1. install over $Port ==" -ForegroundColor Cyan
$install = Invoke-Mpx -Arguments @('--port', $Port, 'app', 'install', $Bundle, '--wait-timeout', "$WaitTimeoutSeconds")
$report.goodInstall = [pscustomobject]@{ exit = $install.Exit; seconds = $install.Seconds; output = $install.Output }
Set-Content -LiteralPath (Join-Path $OutDir 'install-host-logs.txt') -Value (Get-HostLogTail -Lines 60) -Encoding utf8
$after = Get-AppEntry -AppId $report.appId
$report.installedAfter = $after.sha256
$ok = ($install.Exit -eq 0) -and ($after.sha256 -eq $report.bundleSha256)
Write-Host ("  install exit={0} in {1}s; device sha256 {2} bundle sha256" -f $install.Exit, $install.Seconds, $(if ($after.sha256 -eq $report.bundleSha256) { '==' } else { '!=' })) `
    -ForegroundColor $(if ($ok) { 'Green' } else { 'Red' })
if (-not $ok) { $report.notes += "clean install did not leave the Bundle hash on the device (exit $($install.Exit))" }

# --- 2. tampered Bundle must be rejected -------------------------------------
if ($Tamper) {
    Write-Host "`n== 2. tampered bootstrap payload ==" -ForegroundColor Cyan
    $tampered = Join-Path $OutDir 'tampered.bundle.bin'
    $copy = [byte[]]::new($bytes.Length)
    [Array]::Copy($bytes, $copy, $bytes.Length)
    $target = [int]($copy.Length * 0.6)
    $copy[$target] = $copy[$target] -bxor 0xFF
    [System.IO.File]::WriteAllBytes($tampered, $copy)
    $r = Invoke-Mpx -Arguments @('--port', $Port, 'app', 'install', $tampered, '--wait-timeout', "$WaitTimeoutSeconds")
    $still = Get-AppEntry -AppId $report.appId
    $rejected = ($r.Exit -ne 0) -and ($still.sha256 -eq $report.installedAfter)
    $report.tamper = [pscustomobject]@{ exit = $r.Exit; output = $r.Output; installedSha256 = $still.sha256; rejected = $rejected }
    Write-Host ("  tampered install exit={0}; installed App unchanged: {1}" -f $r.Exit, $rejected) -ForegroundColor $(if ($rejected) { 'Green' } else { 'Red' })
    if (-not $rejected) { $report.notes += 'tampered Bundle was not rejected cleanly, or it disturbed the installed App' }
}

# --- 3. aborted install must be recoverable ----------------------------------
if ($Interrupt) {
    Write-Host "`n== 3. install killed after $InterruptAfterMs ms ==" -ForegroundColor Cyan
    $p = Start-Process -FilePath $CliPath -PassThru -WindowStyle Hidden `
        -ArgumentList @('--port', $Port, 'app', 'install', $Bundle, '--wait-timeout', "$WaitTimeoutSeconds")
    Start-Sleep -Milliseconds $InterruptAfterMs
    if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force; Write-Host "  killed pid $($p.Id)" }
    else { Write-Host "  install already finished before the kill window" }
    $alive = Invoke-Mpx -Arguments @('--port', $Port, 'device', 'status')
    $retry = Invoke-Mpx -Arguments @('--port', $Port, 'app', 'install', $Bundle, '--wait-timeout', "$WaitTimeoutSeconds")
    $final = Get-AppEntry -AppId $report.appId
    $recovered = ($alive.Exit -eq 0) -and ($retry.Exit -eq 0) -and ($final.sha256 -eq $report.bundleSha256)
    $report.interrupt = [pscustomobject]@{ cliKilled = (-not $p.HasExited); statusExit = $alive.Exit; retryExit = $retry.Exit; recovered = $recovered }
    Write-Host ("  device reachable={0} retry exit={1} recovered={2}" -f $alive.Exit, $retry.Exit, $recovered) -ForegroundColor $(if ($recovered) { 'Green' } else { 'Red' })
    if (-not $recovered) { $report.notes += 'an aborted install was not recoverable in one retry' }
}

$failed = @($report.goodInstall, $report.tamper, $report.interrupt | Where-Object { $null -ne $_ }).Count
$report.verdict = if ($report.notes.Count -eq 0) { 'PASS' } else { "REVIEW: $($report.notes -join '; ')" }
$path = Join-Path $OutDir 'install-verify.json'
$report | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $path -Encoding utf8
Write-Host "`nverdict: $($report.verdict)" -ForegroundColor $(if ($report.notes.Count -eq 0) { 'Green' } else { 'Yellow' })
Write-Host "report : $path"
exit $(if ($report.notes.Count -eq 0) { 0 } else { 1 })
