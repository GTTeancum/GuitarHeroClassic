[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string] $Diffuse,

    [Parameter(Mandatory = $true)]
    [string] $Mask,

    [Parameter(Mandatory = $true)]
    [ValidatePattern('^#?[0-9A-Fa-f]{6}$')]
    [string] $Color1,

    [Parameter(Mandatory = $true)]
    [ValidatePattern('^#?[0-9A-Fa-f]{6}$')]
    [string] $Color2,

    [Parameter(Mandatory = $true)]
    [string] $Output
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

function Convert-HexRgb {
    param([string] $Value)

    $hex = $Value.TrimStart('#')
    return @(
        [Convert]::ToInt32($hex.Substring(0, 2), 16),
        [Convert]::ToInt32($hex.Substring(2, 2), 16),
        [Convert]::ToInt32($hex.Substring(4, 2), 16)
    )
}

$diffusePath = (Resolve-Path -LiteralPath $Diffuse).Path
$maskPath = (Resolve-Path -LiteralPath $Mask).Path
$outputPath = [IO.Path]::GetFullPath($Output)
$outputParent = Split-Path -Parent $outputPath
if ($outputParent) {
    [IO.Directory]::CreateDirectory($outputParent) | Out-Null
}

$primary = Convert-HexRgb $Color1
$secondary = Convert-HexRgb $Color2
$baseBitmap = [Drawing.Bitmap]::new($diffusePath)
$maskBitmap = [Drawing.Bitmap]::new($maskPath)
$width = $baseBitmap.Width
$height = $baseBitmap.Height
$result = [Drawing.Bitmap]::new(
    $width,
    $height,
    [Drawing.Imaging.PixelFormat]::Format32bppArgb
)

try {
    for ($y = 0; $y -lt $baseBitmap.Height; $y++) {
        $maskY = [Math]::Min(
            $maskBitmap.Height - 1,
            [Math]::Floor($y * $maskBitmap.Height / $baseBitmap.Height)
        )
        for ($x = 0; $x -lt $baseBitmap.Width; $x++) {
            $maskX = [Math]::Min(
                $maskBitmap.Width - 1,
                [Math]::Floor($x * $maskBitmap.Width / $baseBitmap.Width)
            )
            $base = $baseBitmap.GetPixel($x, $y)
            $maskPixel = $maskBitmap.GetPixel($maskX, $maskY)
            $baseChannels = @($base.R, $base.G, $base.B)
            $maskChannels = @($maskPixel.R, $maskPixel.G, $maskPixel.B)
            $outChannels = @(0, 0, 0)
            for ($channel = 0; $channel -lt 3; $channel++) {
                $blend = [Math]::Floor(
                    (
                        (255 - $base.A) * $primary[$channel] +
                        $base.A * $secondary[$channel] +
                        127
                    ) / 255
                )
                $tinted = [Math]::Floor(
                    ($baseChannels[$channel] * $blend + 127) / 255
                )
                $outChannels[$channel] = [Math]::Floor(
                    (
                        $baseChannels[$channel] * $maskChannels[$channel] +
                        $tinted * (255 - $maskChannels[$channel]) +
                        127
                    ) / 255
                )
            }
            $result.SetPixel(
                $x,
                $y,
                [Drawing.Color]::FromArgb(
                    $base.A,
                    $outChannels[0],
                    $outChannels[1],
                    $outChannels[2]
                )
            )
        }
    }
    $result.Save($outputPath, [Drawing.Imaging.ImageFormat]::Png)
}
finally {
    $result.Dispose()
    $maskBitmap.Dispose()
    $baseBitmap.Dispose()
}

Write-Output (
    "COMPOSED_TWO_COLOR size={0}x{1} color1=#{2} color2=#{3} output={4}" -f
    $width,
    $height,
    $Color1.TrimStart('#').ToUpperInvariant(),
    $Color2.TrimStart('#').ToUpperInvariant(),
    $outputPath
)
