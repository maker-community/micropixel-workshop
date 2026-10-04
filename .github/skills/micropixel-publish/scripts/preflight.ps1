# SPDX-License-Identifier: MIT
#
# Publish preflight: build + the app's own verification script + licence and doc consistency.
#
# Usage:
#   .\preflight.ps1 -AppPath apps\pal
#   .\preflight.ps1 -AppPath apps\pal -RunVerification
#
# Exit code: 0 = all checks passed; 1 = something failed.

param(
    [Parameter(Mandatory = $true)][string]$AppPath,
    [switch]$RunVerification
)

$ErrorActionPreference = 'Stop'
$failures = 0

function Check([string]$Name, [bool]$Ok, [string]$Detail = '') {
    $mark = if ($Ok) { '[ok]  ' } else { '[FAIL]'; if (-not $Ok) { $script:failures++ } }
    Write-Host ("{0} {1} {2}" -f $mark, $Name, $Detail)
}

function Invoke-Native([scriptblock]$Command) {
    # With $ErrorActionPreference = 'Stop', PowerShell 5.1 turns a native command's
    # stderr into a terminating error - and node prints a WASI warning on stderr.
    $previous = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try { return (& $Command 2>&1 | Out-String) } finally { $ErrorActionPreference = $previous }
}

# --- locate repo root --------------------------------------------------------
$repoRoot = (& git rev-parse --show-toplevel 2>$null)
if (-not $repoRoot) { $repoRoot = (Get-Location).Path }
$appFull = Join-Path $repoRoot $AppPath
$manifestPath = Join-Path $appFull 'app.json'

Write-Host "repo : $repoRoot"
Write-Host "app  : $appFull"
Write-Host ''

Check 'app.json exists' (Test-Path $manifestPath) $manifestPath
if (-not (Test-Path $manifestPath)) { exit 1 }

$manifestText = [System.IO.File]::ReadAllText($manifestPath, [System.Text.Encoding]::UTF8)
$manifest = $manifestText | ConvertFrom-Json
Write-Host ("       app_id = {0}   version = {1}" -f $manifest.app_id, $manifest.version)
Write-Host ''

# --- licensing and docs ------------------------------------------------------
Check 'root LICENSE exists' (Test-Path (Join-Path $repoRoot 'LICENSE'))
Check 'root NOTICE exists'  (Test-Path (Join-Path $repoRoot 'NOTICE'))

$readme = Join-Path $repoRoot 'README.md'
$slug = Split-Path $AppPath -Leaf
$readmeHasApp = (Test-Path $readme) -and (Select-String -Path $readme -Pattern $slug -Quiet)
Check "root README lists $slug" $readmeHasApp

$appAgents = Join-Path $appFull 'AGENTS.md'
Check 'app AGENTS.md exists' (Test-Path $appAgents)

# --- SPDX headers ------------------------------------------------------------
$sources = @(Get-ChildItem -Path $appFull -Recurse -File -Include *.cpp, *.hpp, *.ps1, *.mjs |
    Where-Object { $_.FullName -notmatch '\\build\\' })
$missing = @($sources | Where-Object {
    -not (Select-String -Path $_.FullName -Pattern 'SPDX-License-Identifier' -Quiet)
})
Check ("SPDX headers present ({0} files)" -f $sources.Count) ($missing.Count -eq 0)
if ($missing.Count -gt 0) {
    $missing | ForEach-Object { Write-Host ("       missing: " + $_.FullName.Substring($repoRoot.Length + 1)) }
}

# --- build -------------------------------------------------------------------
$cli = Join-Path $env:LOCALAPPDATA 'MicroPixel\bin\micropixel.exe'
if (-not (Test-Path $cli)) { $cli = 'micropixel' }

Push-Location $appFull
try {
    $buildOutput = Invoke-Native { & $cli build }
    $buildClean = ($buildOutput -notmatch 'error|warning')
    Check 'micropixel build is clean' $buildClean
    if (-not $buildClean) { $buildOutput -split "`n" | Where-Object { $_ -match 'error|warning' } | Select-Object -First 8 | ForEach-Object { Write-Host ("       " + $_.Trim()) } }

    if ($RunVerification) {
        $verify = Join-Path $appFull 'tools\run-battle-sim.ps1'
        if (Test-Path $verify) {
            $verifyOutput = Invoke-Native { & powershell -NoProfile -ExecutionPolicy Bypass -File $verify -Runs 60 }
            # Only smart and skills-no-items must finish 100% (attack-only loses by design).
            $gateLines = @($verifyOutput -split "`n" | Where-Object { $_ -match '== policy (smart|skills-no-items):' })
            # The sim marks real failures with 'PROBLEM:' and passes with '0 problem(s)',
            # so this test has to be case-sensitive.
            $validationOk = ($verifyOutput -cmatch 'content validation: 0 problem')
            $gateOk = ($gateLines.Count -eq 2) -and $validationOk -and
                (@($gateLines | Where-Object { $_ -match '(\d+)/(\d+)' -and $Matches[1] -ne $Matches[2] }).Count -eq 0)
            Check 'tools\run-battle-sim.ps1 gate (smart + skills-no-items)' $gateOk
            $verifyOutput -split "`n" | Where-Object { $_ -match 'content validation|runs finished' } | ForEach-Object { Write-Host ("       " + $_.Trim()) }
        } else {
            Write-Host '       (no tools\run-battle-sim.ps1 in this app, skipping the gate)'
        }
    }
} finally {
    Pop-Location
}

# --- store materials ---------------------------------------------------------
$store = Join-Path $appFull 'store'
if (Test-Path $store) {
    $shots = @(Get-ChildItem $store -File | Where-Object { $_.Extension -in '.png', '.jpg', '.jpeg' })
    $tooBig = @($shots | Where-Object { $_.Length -gt 2MB })
    Check 'store screenshots >= 2' ($shots.Count -ge 2) ("found {0}" -f $shots.Count)
    Check 'store screenshots <= 2 MiB each' ($tooBig.Count -eq 0)
    Check 'store/description.md exists' (Test-Path (Join-Path $store 'description.md'))
} else {
    Write-Host '       (no store/ yet: publishing needs >=2 real screenshots and description.md)'
}

Write-Host ''
if ($failures -gt 0) {
    Write-Host ("{0} check(s) failed - fix them before publishing." -f $failures) -ForegroundColor Red
    exit 1
}
Write-Host 'Preflight passed - ready for publish --dry-run.' -ForegroundColor Green
exit 0
