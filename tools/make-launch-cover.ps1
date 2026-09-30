# SPDX-License-Identifier: Apache-2.0
# Renders assets/launch.png: the 360x360 cover the MicroPixel launcher shows
# (app.json -> launch_asset). Replaces the leftover art from the project this
# repository was forked from, which still carried the old title and logo.
#
# The palette mirrors theme:: in pal_common.hpp and the composition mirrors the
# title screen: night sky, moon, pine ridges and a sword planted on the ridge.
#
#   powershell -NoProfile -File tools\make-launch-cover.ps1 [-Output <path>]
#
# The title string is built from code points on purpose: Windows PowerShell 5.1
# reads .ps1 files as ANSI unless they carry a BOM, so Chinese literals in this
# script would arrive mangled.
param(
    [string]$Output = (Join-Path $PSScriptRoot '..\assets\launch.png')
)

Add-Type -AssemblyName System.Drawing

$size = 360
$title = -join (@(0x4ED9, 0x5251, 0x5947, 0x4FA0, 0x4F20) | ForEach-Object { [char]$_ })
$subtitle = 'CHINESE PALADIN'

function New-Color([int]$r, [int]$gr, [int]$b, [int]$a = 255) {
    return [System.Drawing.Color]::FromArgb($a, $r, $gr, $b)
}

$bmp = New-Object System.Drawing.Bitmap($size, $size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
$g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality

# --- sky ---------------------------------------------------------------------
$rect = New-Object System.Drawing.Rectangle(0, 0, $size, $size)
$sky = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
    $rect, (New-Color 9 11 24), (New-Color 26 32 56), 90.0)
$g.FillRectangle($sky, $rect)
$sky.Dispose()

# Stars are fixed so the cover is byte-stable across runs.
$stars = @(
    @(14, 22, 2), @(38, 48, 1), @(63, 17, 2), @(88, 61, 1), @(112, 29, 2),
    @(141, 52, 1), @(166, 18, 2), @(193, 44, 1), @(219, 25, 2), @(244, 58, 1),
    @(268, 33, 2), @(297, 47, 1), @(322, 21, 2), @(341, 63, 1), @(27, 88, 1),
    @(74, 102, 2), @(126, 84, 1), @(178, 108, 1), @(232, 92, 2), @(286, 104, 1),
    @(334, 96, 2), @(51, 138, 1), @(104, 152, 1), @(206, 146, 1), @(259, 158, 1),
    @(311, 142, 1)
)
$starBrush = New-Object System.Drawing.SolidBrush((New-Color 226 232 255 190))
foreach ($star in $stars) {
    $r = [int]$star[2]
    $g.FillEllipse($starBrush, [int]$star[0], [int]$star[1], $r * 2, $r * 2)
}
$starBrush.Dispose()

# --- moon --------------------------------------------------------------------
$moonX = 264; $moonY = 76; $moonR = 40
for ($halo = 20; $halo -ge 4; $halo -= 4) {
    $alpha = 8 + (20 - $halo)
    $glow = New-Object System.Drawing.SolidBrush((New-Color 238 232 208 $alpha))
    $g.FillEllipse($glow, $moonX - $moonR - $halo, $moonY - $moonR - $halo,
        ($moonR + $halo) * 2, ($moonR + $halo) * 2)
    $glow.Dispose()
}
$moonBrush = New-Object System.Drawing.SolidBrush((New-Color 238 232 208))
$g.FillEllipse($moonBrush, $moonX - $moonR, $moonY - $moonR, $moonR * 2, $moonR * 2)
$moonBrush.Dispose()
# A crescent bite, so the disc reads as a moon rather than a plain circle.
$bite = New-Object System.Drawing.SolidBrush((New-Color 18 22 42))
$g.FillEllipse($bite, $moonX - $moonR + 14, $moonY - $moonR - 6, $moonR * 2, $moonR * 2)
$bite.Dispose()

# --- sword -------------------------------------------------------------------
# Planted on the ridge with the tip up: blade, guard, grip, pommel.
$blade = @(
    (New-Object System.Drawing.PointF(172, 58)),
    (New-Object System.Drawing.PointF(180, 80)),
    (New-Object System.Drawing.PointF(180, 178)),
    (New-Object System.Drawing.PointF(164, 178)),
    (New-Object System.Drawing.PointF(164, 80))
)
$bladeGlow = New-Object System.Drawing.SolidBrush((New-Color 150 190 240 46))
$g.FillPolygon($bladeGlow, $blade)
$bladeGlow.Dispose()
$bladeBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
    (New-Object System.Drawing.Rectangle(160, 58, 24, 122)),
    (New-Color 246 250 255), (New-Color 138 156 192), 0.0)
$g.FillPolygon($bladeBrush, $blade)
$bladeBrush.Dispose()

$steel = New-Object System.Drawing.Pen((New-Color 60 74 108), 1.0)
$g.DrawPolygon($steel, $blade)

$gold = New-Object System.Drawing.SolidBrush((New-Color 228 196 124))
$goldEdge = New-Object System.Drawing.Pen((New-Color 255 232 178), 1.0)
$guard = New-Object System.Drawing.Rectangle(146, 178, 52, 10)
$g.FillRectangle($gold, $guard)
$g.DrawRectangle($goldEdge, $guard)
$grip = New-Object System.Drawing.Rectangle(166, 188, 12, 34)
$g.FillRectangle($gold, $grip)
$g.DrawRectangle($goldEdge, $grip)
$g.FillEllipse($gold, 162, 222, 20, 14)
$g.DrawEllipse($goldEdge, 162, 222, 20, 14)
$gold.Dispose(); $goldEdge.Dispose(); $steel.Dispose()

# --- ridges ------------------------------------------------------------------
# Three silhouette layers, back to front, so the sword stands on the near one.
$ridgeLayers = @(
    @{ y = 214; pts = @(0, 214, 46, 196, 92, 208, 138, 188, 184, 204, 228, 192, 276, 206, 322, 194, 360, 208);
       color = (New-Color 34 46 78) },
    @{ y = 236; pts = @(0, 238, 40, 222, 86, 234, 130, 216, 176, 232, 222, 220, 268, 234, 314, 222, 360, 236);
       color = (New-Color 24 34 60) },
    @{ y = 258; pts = @(0, 262, 52, 246, 108, 258, 162, 242, 216, 256, 270, 244, 322, 258, 360, 250);
       color = (New-Color 14 20 38) }
)
foreach ($layer in $ridgeLayers) {
    $points = New-Object 'System.Drawing.PointF[]' (($layer.pts.Count / 2) + 2)
    for ($i = 0; $i -lt $layer.pts.Count; $i += 2) {
        $points[$i / 2] = New-Object System.Drawing.PointF($layer.pts[$i], $layer.pts[$i + 1])
    }
    $last = $layer.pts.Count / 2
    $points[$last] = New-Object System.Drawing.PointF(360, 360)
    $points[$last + 1] = New-Object System.Drawing.PointF(0, 360)
    $brush = New-Object System.Drawing.SolidBrush($layer.color)
    $g.FillPolygon($brush, $points)
    $brush.Dispose()
}

# --- pine row along the bottom ----------------------------------------------
# Flat (x, height) pairs: PowerShell mangles nested array literals in foreach.
$ink = New-Object System.Drawing.SolidBrush((New-Color 9 13 26))
$pines = @(28, 34, 74, 46, 126, 38, 232, 42, 288, 32, 334, 44)
for ($i = 0; $i -lt $pines.Count; $i += 2) {
    $cx = [int]$pines[$i]
    $h = [int]$pines[$i + 1]
    $top = 336 - $h
    $cone = @(
        (New-Object System.Drawing.PointF(($cx - 13), 344)),
        (New-Object System.Drawing.PointF($cx, $top)),
        (New-Object System.Drawing.PointF(($cx + 13), 344))
    )
    $g.FillPolygon($ink, $cone)
    $g.FillRectangle($ink, ($cx - 2), 328, 4, 18)
}
$ink.Dispose()

# --- title -------------------------------------------------------------------
$titleFont = New-Object System.Drawing.Font('Microsoft YaHei', 40, [System.Drawing.FontStyle]::Bold,
    [System.Drawing.GraphicsUnit]::Pixel)
$subFont = New-Object System.Drawing.Font('Segoe UI', 10, [System.Drawing.FontStyle]::Regular,
    [System.Drawing.GraphicsUnit]::Pixel)

$center = New-Object System.Drawing.StringFormat
$center.Alignment = [System.Drawing.StringAlignment]::Center
$center.LineAlignment = [System.Drawing.StringAlignment]::Center

# The near ridges are dark enough on their own that the gold title sits on them
# without a backdrop: an overlay band here reads as a translucent rectangle.
$titleBrush = New-Object System.Drawing.SolidBrush((New-Color 255 232 178))
$g.DrawString($title, $titleFont, $titleBrush,
    (New-Object System.Drawing.RectangleF(0, 246, 360, 60)), $center)
$titleBrush.Dispose()

$rule = New-Object System.Drawing.Pen((New-Color 120 134 186), 1.0)
$g.DrawLine($rule, 108, 306, 252, 306)
$rule.Dispose()

$subBrush = New-Object System.Drawing.SolidBrush((New-Color 148 156 190))
$g.DrawString($subtitle, $subFont, $subBrush,
    (New-Object System.Drawing.RectangleF(0, 310, 360, 22)), $center)
$subBrush.Dispose()

$titleFont.Dispose(); $subFont.Dispose(); $center.Dispose()

$g.Dispose()
$resolved = [System.IO.Path]::GetFullPath($Output)
[System.IO.Directory]::CreateDirectory([System.IO.Path]::GetDirectoryName($resolved)) | Out-Null
$bmp.Save($resolved, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()

$info = Get-Item -LiteralPath $resolved
Write-Output "wrote $($info.FullName) ($([int]($info.Length / 1024)) KB, ${size}x${size})"
