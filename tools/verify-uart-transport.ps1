<#
.SYNOPSIS
    Verifies the MPX1 control transport over a UART (SenseCAP Watcher CH342) link.

.DESCRIPTION
    Reproduces the scenario the PR review asks for: while the Host keeps emitting
    raw ESP_LOG lines on the same UART, hammer the MPX1 channel with screenshots
    and status queries, and check that no response frame or JPEG payload is
    interleaved/corrupted.

    Every failed op is recorded with its exit code and stderr text. A screenshot
    is only counted as good when the JPEG has SOI, EOI and an SOF header with the
    expected dimensions (a truncated payload loses EOI).

.EXAMPLE
    pwsh -File tools/verify-uart-transport.ps1 -Port COM3 -Screenshots 100 -StatusQueries 20
#>
[CmdletBinding()]
param(
    [string]$Port = 'COM3',
    [int]$Screenshots = 100,
    [int]$StatusQueries = 20,
    [int]$LogPolls = 20,
    [int]$TapEvery = 0,
    [string]$OutDir = "$env:TEMP\mpx-uart-verify",
    [string]$CliPath = '',
    [switch]$FollowHostLogs,
    [int]$MinJpegBytes = 2000
)

$ErrorActionPreference = 'Stop'

if (-not $CliPath) {
    $CliPath = if ($env:MICROPIXEL_CLI) { $env:MICROPIXEL_CLI }
               else { Join-Path $env:LOCALAPPDATA 'MicroPixel\bin\micropixel.exe' }
}
if (-not (Test-Path -LiteralPath $CliPath)) { throw "micropixel CLI not found: $CliPath" }

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$shotsDir = Join-Path $OutDir 'shots'
New-Item -ItemType Directory -Force -Path $shotsDir | Out-Null

function Invoke-Mpx {
    param([string[]]$Arguments, [string]$Kind, [int]$Index)
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $raw = & $CliPath @Arguments 2>&1 | Out-String
    $sw.Stop()
    $code = $LASTEXITCODE
    return [pscustomobject]@{
        Kind    = $Kind
        Index   = $Index
        Exit    = $code
        Output  = $raw.Trim()
        Seconds = [math]::Round($sw.Elapsed.TotalSeconds, 3)
    }
}

function Get-JpegInfo {
    param([string]$Path)
    $r = [pscustomobject]@{ Valid = $false; Reason = ''; Width = 0; Height = 0; Bytes = 0 }
    if (-not (Test-Path -LiteralPath $Path)) { $r.Reason = 'missing file'; return $r }
    $b = [System.IO.File]::ReadAllBytes($Path)
    $r.Bytes = $b.Length
    if ($b.Length -lt 4) { $r.Reason = 'too small'; return $r }
    if ($b[0] -ne 0xFF -or $b[1] -ne 0xD8) { $r.Reason = 'no SOI'; return $r }
    if ($b[$b.Length - 2] -ne 0xFF -or $b[$b.Length - 1] -ne 0xD9) { $r.Reason = 'no EOI (truncated)'; return $r }
    $i = 2
    while ($i -lt $b.Length - 8) {
        if ($b[$i] -ne 0xFF) { $i++; continue }
        $m = $b[$i + 1]
        if ($m -eq 0xFF) { $i++; continue }
        if ($m -eq 0xD8 -or $m -eq 0x01 -or ($m -ge 0xD0 -and $m -le 0xD7)) { $i += 2; continue }
        [int]$len = ([int]$b[$i + 2] -shl 8) + $b[$i + 3]
        if ($m -ge 0xC0 -and $m -le 0xCF -and $m -ne 0xC4 -and $m -ne 0xC8 -and $m -ne 0xCC) {
            # [byte] -shl overflows to zero in PowerShell, so widen to [int] first.
            $r.Height = ([int]$b[$i + 5] -shl 8) + $b[$i + 6]
            $r.Width = ([int]$b[$i + 7] -shl 8) + $b[$i + 8]
            $r.Valid = $true
            return $r
        }
        if ($m -eq 0xDA) { $r.Reason = 'SOS before SOF'; return $r }
        $i += 2 + $len
    }
    $r.Reason = 'SOF not found'
    return $r
}

function Get-HostLogSeq {
    $raw = & $CliPath --port $Port logs -n 1 --source host 2>&1 | Out-String
    $m = [regex]::Match($raw, '^\s*(\d+)\s', 'Multiline')
    if ($m.Success) { return [int]$m.Groups[1].Value } else { return -1 }
}

$result = [ordered]@{
    port            = $Port
    cli             = $CliPath
    startedUtc      = (Get-Date).ToUniversalTime().ToString('o')
    hostLogSeqStart = -1
    hostLogSeqEnd   = -1
    expectedSize    = ''
    screenshots     = 0
    statusQueries   = 0
    logPolls        = 0
    taps            = 0
    distinctShots   = 0
    failures        = @()
    elapsedSeconds  = 0
    verdict         = ''
}

Write-Host "== baseline ==" -ForegroundColor Cyan
$status = Invoke-Mpx -Arguments @('--port', $Port, 'device', 'status') -Kind 'status' -Index 0
if ($status.Exit -ne 0) {
    Write-Host $status.Output
    throw "device status failed on $Port (exit $($status.Exit))"
}
$snapshot = $status.Output | ConvertFrom-Json
$expW = [int]$snapshot.hardware.display.widthPixels
$expH = [int]$snapshot.hardware.display.heightPixels
$result.expectedSize = "${expW}x${expH}"
Write-Host "board=$($snapshot.hardware.board) fw=$($snapshot.firmware.version) display=${expW}x${expH} apps=$($snapshot.runtime.installedAppCount)"

$logJob = $null
if ($FollowHostLogs) {
    $logJob = Start-Process -FilePath $CliPath -PassThru -WindowStyle Hidden `
        -ArgumentList @('--port', $Port, 'logs', '--follow', '--source', 'host') `
        -RedirectStandardOutput (Join-Path $OutDir 'host-follow.txt')
    Write-Host "host log follower started (pid $($logJob.Id))"
}

$result.hostLogSeqStart = Get-HostLogSeq
Write-Host "host log seq at start: $($result.hostLogSeqStart)"

$shotHashes = New-Object 'System.Collections.Generic.HashSet[string]'

$total = $Screenshots + $StatusQueries + $LogPolls
$statusEvery = if ($StatusQueries -gt 0) { [math]::Ceiling($total / $StatusQueries) } else { [int]::MaxValue }
$logEvery = if ($LogPolls -gt 0) { [math]::Floor($total / ($LogPolls + 1)) } else { [int]::MaxValue }
$sw = [System.Diagnostics.Stopwatch]::StartNew()
$shotNo = 0
$statusNo = 0
$logNo = 0

for ($i = 1; $i -le $total; $i++) {
    if ($LogPolls -gt 0 -and $logNo -lt $LogPolls -and $logEvery -gt 0 -and $i % $logEvery -eq 0) {
        $logNo++
        # A logs read returns the largest payload on this link, so it stresses the
        # same frame path a screenshot uses while the Host keeps writing log text.
        $r = Invoke-Mpx -Arguments @('--port', $Port, 'logs', '-n', '25', '--source', 'all') -Kind 'logs' -Index $logNo
        $ok = ($r.Exit -eq 0) -and ($r.Output.Length -gt 40)
        if ($ok -and $r.Output -match '(?i)(corrupt|truncat|mismatch|checksum|bad frame|invalid response)') {
            $ok = $false
        }
        $result.logPolls++
        if (-not $ok) {
            $result.failures += [pscustomobject]@{ kind = 'logs'; index = $logNo; exit = $r.Exit; detail = $r.Output }
            Write-Host ("  [{0,3}] logs    FAIL  {1}s  {2}" -f $i, $r.Seconds, $r.Output) -ForegroundColor Red
        } else {
            Write-Host ("  [{0,3}] logs    ok    {1}s  {2} chars" -f $i, $r.Seconds, $r.Output.Length) -ForegroundColor DarkGray
        }
        continue
    }
    if ($i % $statusEvery -eq 0 -and $statusNo -lt $StatusQueries) {
        $statusNo++
        $r = Invoke-Mpx -Arguments @('--port', $Port, 'device', 'status') -Kind 'status' -Index $statusNo
        $ok = $r.Exit -eq 0
        if ($ok) {
            try {
                $j = $r.Output | ConvertFrom-Json
                if (-not $j.online) { $ok = $false; $r.Output = "online=false: $($r.Output)" }
            } catch { $ok = $false; $r.Output = "unparseable response: $($r.Output)" }
        }
        $result.statusQueries++
        if (-not $ok) {
            $result.failures += [pscustomobject]@{ kind = 'status'; index = $statusNo; exit = $r.Exit; detail = $r.Output }
            Write-Host ("  [{0,3}] status  FAIL  {1}s  {2}" -f $i, $r.Seconds, $r.Output) -ForegroundColor Red
        } else {
            Write-Host ("  [{0,3}] status  ok    {1}s" -f $i, $r.Seconds) -ForegroundColor DarkGray
        }
    } else {
        $shotNo++
        if ($TapEvery -gt 0 -and $shotNo % $TapEvery -eq 0) {
            # Change the pixels so the JPEG payload differs run to run, i.e. the
            # capture path really moves fresh frame bytes while logs stream.
            $t = Invoke-Mpx -Arguments @('--port', $Port, 'input', 'tap', '206', '206') -Kind 'tap' -Index $shotNo
            $result.taps++
            if ($t.Exit -ne 0) {
                $result.failures += [pscustomobject]@{ kind = 'tap'; index = $shotNo; exit = $t.Exit; detail = $t.Output }
                Write-Host ("  [{0,3}] tap     FAIL  {1}" -f $i, $t.Output) -ForegroundColor Red
            }
        }
        $file = Join-Path $shotsDir ("shot-{0:d4}.jpg" -f $shotNo)
        $r = Invoke-Mpx -Arguments @('--port', $Port, 'screenshot', '--output', $file) -Kind 'screenshot' -Index $shotNo
        $ok = $r.Exit -eq 0
        if ($ok) {
            $ji = Get-JpegInfo -Path $file
            if (-not $ji.Valid) { $ok = $false; $r.Output = "jpeg invalid: $($ji.Reason)" }
            elseif ($ji.Bytes -lt $MinJpegBytes) { $ok = $false; $r.Output = "jpeg too small: $($ji.Bytes) bytes" }
            elseif ($expW -gt 0 -and ($ji.Width -ne $expW -or $ji.Height -ne $expH)) {
                $ok = $false; $r.Output = "wrong size: $($ji.Width)x$($ji.Height), expected ${expW}x${expH}"
            }
        }
        $result.screenshots++
        if ($ok) {
            [void]$shotHashes.Add((Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash)
        }
        if (-not $ok) {
            $result.failures += [pscustomobject]@{ kind = 'screenshot'; index = $shotNo; exit = $r.Exit; detail = $r.Output }
            Write-Host ("  [{0,3}] shot    FAIL  {1}s  {2}" -f $i, $r.Seconds, $r.Output) -ForegroundColor Red
        } elseif ($shotNo % 10 -eq 0) {
            Write-Host ("  [{0,3}] shot    ok    {1}s  {2} bytes" -f $i, $r.Seconds, (Get-Item -LiteralPath $file).Length) -ForegroundColor DarkGray
        }
    }
}

$sw.Stop()
$result.elapsedSeconds = [math]::Round($sw.Elapsed.TotalSeconds, 1)
$result.hostLogSeqEnd = Get-HostLogSeq
if ($logJob -and -not $logJob.HasExited) { Stop-Process -Id $logJob.Id -Force }

$failed = $result.failures.Count
$result.distinctShots = $shotHashes.Count
$result.verdict = if ($failed -eq 0) { 'PASS' } else { "FAIL ($failed)" }

$summaryPath = Join-Path $OutDir 'summary.json'
$result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $summaryPath -Encoding utf8

Write-Host ""
Write-Host "== summary ==" -ForegroundColor Cyan
Write-Host "screenshots      : $($result.screenshots) ($($result.distinctShots) distinct frames)"
Write-Host "status queries   : $($result.statusQueries)"
Write-Host "log polls        : $($result.logPolls)"
Write-Host "taps             : $($result.taps)"
Write-Host "elapsed          : $($result.elapsedSeconds) s"
Write-Host "host log seq     : $($result.hostLogSeqStart) -> $($result.hostLogSeqEnd)  (delta $(($result.hostLogSeqEnd - $result.hostLogSeqStart)))"
Write-Host "failures         : $failed"
Write-Host "verdict          : $($result.verdict)" -ForegroundColor $(if ($failed -eq 0) { 'Green' } else { 'Red' })
Write-Host "summary written  : $summaryPath"
exit $(if ($failed -eq 0) { 0 } else { 1 })
