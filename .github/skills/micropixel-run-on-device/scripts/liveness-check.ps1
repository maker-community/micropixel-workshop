# SPDX-License-Identifier: MIT
#
# Liveness check: is the app on the device still alive, or is the screen frozen?
#
# A frozen screen (e.g. the host rejecting every frame after a missing glyph) keeps the
# guest logic and timers running while the panel stays on an old frame. That looks
# exactly like a static menu waiting for input; only a per-pixel diff can tell.
#
# Usage:
#   .\liveness-check.ps1                     # shot -> press confirm -> shot, then diff
#   .\liveness-check.ps1 -Press down         # press another key between the two frames
#   .\liveness-check.ps1 -Press ''           # send no key (for screens that animate)
#   .\liveness-check.ps1 -Port COM4
#   .\liveness-check.ps1 -A a.png -B b.png   # diff two existing frames
#
# Exit code: 0 = frames differ (alive); 1 = identical (static screen, or frozen).

param(
    [string]$Port = 'COM3',
    [string]$A,
    [string]$B,
    [string]$Press = 'confirm',
    [int]$Threshold = 40,
    [int]$Step = 2,
    [int]$WaitMs = 700
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$cli = Join-Path $env:LOCALAPPDATA 'MicroPixel\bin\micropixel.exe'
if (-not (Test-Path $cli)) { $cli = 'micropixel' }

function Save-Shot([string]$Path) {
    # With $ErrorActionPreference = 'Stop', PowerShell 5.1 turns a native command's
    # stderr into a terminating error before we can inspect it - so relax it here.
    $previous = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try { $out = & $cli --port $Port screenshot --output $Path 2>&1 | Out-String }
    finally { $ErrorActionPreference = $previous }
    if (-not (Test-Path $Path)) {
        Write-Host $out.Trim() -ForegroundColor Red
        throw (("screenshot failed on {0}. Is the device attached, and is nothing else " +
                "holding the port (a still-running 'micropixel logs' keeps it open)? " +
                "Note: 'micropixel port list' reports 0 ports on this setup, so it is not a " +
                "reliable check - use the CH342 control port (COM3 here).") -f $Port)
    }
}

if (-not $A -or -not $B) {
    $A = Join-Path $env:TEMP 'liveness-a.png'
    $B = Join-Path $env:TEMP 'liveness-b.png'
    Save-Shot $A
    if ($Press) {
        & $cli --port $Port input press $Press | Out-Null
        Start-Sleep -Milliseconds $WaitMs
    }
    Save-Shot $B
}

$ia = [System.Drawing.Bitmap]::FromFile($A)
$ib = [System.Drawing.Bitmap]::FromFile($B)
$diff = 0
try {
    for ($y = 0; $y -lt $ia.Height; $y += $Step) {
        for ($x = 0; $x -lt $ia.Width; $x += $Step) {
            $p = $ia.GetPixel($x, $y)
            $q = $ib.GetPixel($x, $y)
            if ([Math]::Abs($p.R - $q.R) -gt $Threshold -or
                [Math]::Abs($p.G - $q.G) -gt $Threshold -or
                [Math]::Abs($p.B - $q.B) -gt $Threshold) { $diff++ }
        }
    }
} finally {
    $ia.Dispose()
    $ib.Dispose()
}

Write-Host ("A: {0}" -f $A)
Write-Host ("B: {0}" -f $B)
Write-Host ("differing samples: {0} (threshold {1}, step {2})" -f $diff, $Threshold, $Step)

if ($diff -eq 0) {
    Write-Host 'Frames are identical: normal for a genuinely static screen, otherwise treat it as frozen.' -ForegroundColor Yellow
    Write-Host 'Triage: 1) logic still running but the picture never moves? suspect a missing glyph' -ForegroundColor Yellow
    Write-Host '        (the host log shows rejected op / present rejected).' -ForegroundColor Yellow
    Write-Host '        2) diff again on a screen that must change, to rule out "static anyway".' -ForegroundColor Yellow
    exit 1
}

Write-Host 'Frames differ - the app is alive.' -ForegroundColor Green
exit 0
