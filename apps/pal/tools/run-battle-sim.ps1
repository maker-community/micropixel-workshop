# SPDX-License-Identifier: MIT
# Builds tools/battle_sim.cpp for wasm32-wasip1 with the MicroPixel wasi-sdk and
# runs it under node's WASI. Needs a prior `micropixel build` so that
# build/generated/pal_strings.hpp exists.
param([int]$Runs = 300)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$packages = Join-Path $env:LOCALAPPDATA 'MicroPixel\packages'
$clang = Get-ChildItem $packages -Recurse -Depth 4 -Filter 'clang++.exe' | Select-Object -First 1
$sdk = Get-ChildItem $packages -Directory | Where-Object { Test-Path (Join-Path $_.FullName 'micropixel-sdk-*\guest\sdk') } |
    ForEach-Object { Get-ChildItem $_.FullName -Directory -Filter 'micropixel-sdk-*' } | Select-Object -First 1
if (-not $clang -or -not $sdk) { throw 'wasi-sdk or MicroPixel SDK not found' }

$out = Join-Path $root 'build\battle_sim.wasm'
& $clang.FullName --target=wasm32-wasip1 -std=c++23 -O1 -fno-exceptions -fno-rtti `
    -Wno-unused-parameter -I (Join-Path $sdk.FullName 'guest') -I (Join-Path $root 'build\generated') `
    (Join-Path $root 'tools\battle_sim.cpp') (Join-Path $root 'pal_battle.cpp') (Join-Path $root 'pal_model.cpp') `
    (Join-Path $root 'pal_content.cpp') '-Wl,--allow-undefined' '-Wl,--gc-sections' -o $out
if ($LASTEXITCODE -ne 0) { throw 'battle_sim build failed' }

node (Join-Path $PSScriptRoot 'run-wasi.mjs') $out $Runs
