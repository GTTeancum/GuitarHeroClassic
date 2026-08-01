param(
  [string]$Root = "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX",
  [string]$ArkDir = "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\GEN",
  [string]$BuildDir = "engine/out/build/win-amd64-debug",
  [string]$OutDir = "",
  [switch]$Build,
  [string]$InactiveRef = "",
  [string]$ActiveRef = "",
  [int]$InactiveScore = 121168,
  [int]$InactiveStreak = 18,
  [int]$InactiveMultiplier = 4,
  [double]$InactiveSp = 0.10,
  [double]$InactiveRock = 0.75,
  [int]$ActiveScore = 851305,
  [int]$ActiveStreak = 38,
  [int]$ActiveMultiplier = 4,
  [double]$ActiveSp = 0.80,
  [double]$ActiveRock = 0.90,
  [string]$HudTunePath = "",
  [int]$CaptureTimeoutSec = 90
)

$ErrorActionPreference = "Stop"

if (-not $OutDir) {
  $stamp = Get-Date -Format "yyyyMMdd_HHmmss"
  $OutDir = Join-Path $Root "engine/out/codex_hud_parity_$stamp"
}

$exe = Join-Path $Root (Join-Path $BuildDir "src/app/ghogx_app.exe")
$cmakeBuildDir = Join-Path $Root $BuildDir

function Run-Step([string]$Name, [scriptblock]$Body) {
  $sw = [Diagnostics.Stopwatch]::StartNew()
  & $Body
  $sw.Stop()
  Write-Host ("{0} seconds={1}" -f $Name, [math]::Round($sw.Elapsed.TotalSeconds, 1))
}

function Run-Capture([string]$Name, [int]$Score, [int]$Streak,
                     [int]$Multiplier, [double]$Sp, [double]$Rock,
                     [bool]$StarActive = $false) {
  $shot = Join-Path $OutDir "$Name.bmp"
  $log = Join-Path $OutDir "$Name.stderr.log"
  $stdoutLog = Join-Path $OutDir "$Name.stdout.log"
  $appArgs = @(
    "--ark-dir", $ArkDir,
    "--auto-start",
    "--song", "shoutatthedevil",
    "--difficulty", "1",
    "--diagnostic-autoplay",
    "--fixed-dt", "0.0166667",
    "--hud-score", "$Score",
    "--hud-streak", "$Streak",
    "--hud-multiplier", "$Multiplier",
    "--hud-sp", ("{0:0.00}" -f $Sp),
    "--hud-rock", ("{0:0.00}" -f $Rock),
    "--screenshot", $shot,
    "--screenshot-frame", "63",
    "--frames", "66"
  )
  if ($HudTunePath) {
    $appArgs += @("--hud-tune", $HudTunePath)
  }
  if ($StarActive) {
    $appArgs += @("--hud-star-active")
  }

  function Quote-NativeArg([string]$Arg) {
    if ($Arg -match '[\s"]') {
      return '"' + ($Arg -replace '"', '\"') + '"'
    }
    return $Arg
  }

  Run-Step $Name {
    Remove-Item -LiteralPath $log, $stdoutLog -Force -ErrorAction SilentlyContinue
    $argLine = ($appArgs | ForEach-Object { Quote-NativeArg $_ }) -join " "
    $proc = Start-Process -FilePath $exe -ArgumentList $argLine -PassThru `
      -RedirectStandardOutput $stdoutLog -RedirectStandardError $log

    $deadline = (Get-Date).AddSeconds($CaptureTimeoutSec)
    while (-not $proc.HasExited -and (Get-Date) -lt $deadline) {
      Start-Sleep -Milliseconds 250
      $proc.Refresh()
    }
    if (-not $proc.HasExited) {
      Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
      throw "$Name timed out after $CaptureTimeoutSec seconds"
    }
    $proc.WaitForExit()
    $exitCode = $proc.ExitCode
    if ($null -eq $exitCode) {
      $exitCode = 0
    }
    if ($exitCode -ne 0) {
      Get-Content -LiteralPath $log -Tail 80
      throw "$Name failed with exit $exitCode"
    }
  }

  if (-not (Test-Path -LiteralPath $shot)) {
    throw "$Name did not write screenshot $shot"
  }
  $override = Select-String -LiteralPath $log -Pattern "diagnostic HUD override" -Quiet
  $saved = Select-String -LiteralPath $log -Pattern "screenshot saved" -Quiet
  if (-not ($override -and $saved)) {
    throw "$Name log did not confirm HUD override and screenshot save"
  }
}

function Add-Thumb([System.Drawing.Graphics]$Graphics,
                   [string]$Path,
                   [string]$Label,
                   [int]$X,
                   [int]$Y,
                   [int]$W,
                   [int]$H) {
  $brush = [System.Drawing.Brushes]::White
  $font = New-Object System.Drawing.Font("Arial", 10)
  $Graphics.DrawString($Label, $font, $brush, $X + 8, $Y + 7)
  $img = [System.Drawing.Image]::FromFile($Path)
  try {
    $scale = [Math]::Min($W / $img.Width, ($H - 28) / $img.Height)
    $dw = [int]($img.Width * $scale)
    $dh = [int]($img.Height * $scale)
    $dx = $X + [int](($W - $dw) / 2)
    $dy = $Y + 28 + [int](($H - 28 - $dh) / 2)
    $Graphics.DrawImage($img, $dx, $dy, $dw, $dh)
  } finally {
    $img.Dispose()
    $font.Dispose()
  }
}

function Normalize-RefImage([string]$Path, [string]$Name) {
  Add-Type -AssemblyName System.Drawing
  $img = [System.Drawing.Image]::FromFile($Path)
  try {
    $srcX = 0
    $srcY = 0
    $srcW = $img.Width
    $srcH = $img.Height
    if ($img.Width -eq 656 -and $img.Height -eq 519) {
      $srcX = 8
      $srcY = 91
      $srcW = 640
      $srcH = 360
    } elseif ([Math]::Abs(($img.Width / [double]$img.Height) - (16.0 / 9.0)) -gt 0.02) {
      $target = 16.0 / 9.0
      $current = $img.Width / [double]$img.Height
      if ($current -gt $target) {
        $srcW = [int]($img.Height * $target)
        $srcX = [int](($img.Width - $srcW) / 2)
      } else {
        $srcH = [int]($img.Width / $target)
        $srcY = [int](($img.Height - $srcH) / 2)
      }
    }

    $bmp = New-Object System.Drawing.Bitmap 1280, 720
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    try {
      $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
      $g.DrawImage($img,
        (New-Object System.Drawing.Rectangle 0, 0, 1280, 720),
        (New-Object System.Drawing.Rectangle $srcX, $srcY, $srcW, $srcH),
        [System.Drawing.GraphicsUnit]::Pixel)
      $out = Join-Path $OutDir $Name
      $bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
      return $out
    } finally {
      $g.Dispose()
      $bmp.Dispose()
    }
  } finally {
    $img.Dispose()
  }
}

function Make-ContactSheet {
  if (-not ($InactiveRef -and $ActiveRef)) { return }
  if (-not (Test-Path -LiteralPath $InactiveRef)) { return }
  if (-not (Test-Path -LiteralPath $ActiveRef)) { return }

  $inactiveRefNorm = Normalize-RefImage $InactiveRef "ps2_inactive_ref_normalized.png"
  $activeRefNorm = Normalize-RefImage $ActiveRef "ps2_active_ref_normalized.png"

  Add-Type -AssemblyName System.Drawing
  $w = 420
  $h = 264
  $bmp = New-Object System.Drawing.Bitmap ($w * 2), ($h * 2)
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  try {
    $g.Clear([System.Drawing.Color]::FromArgb(12, 12, 16))
    Add-Thumb $g $inactiveRefNorm "PS2 inactive ref" 0 0 $w $h
    Add-Thumb $g (Join-Path $OutDir "ingame_inactive_hud.bmp") "Native inactive" $w 0 $w $h
    Add-Thumb $g $activeRefNorm "PS2 active ref" 0 $h $w $h
    Add-Thumb $g (Join-Path $OutDir "ingame_active_hud.bmp") "Native active" $w $h $w $h
    $sheet = Join-Path $OutDir "hud_gameplay_parity_compare.png"
    $bmp.Save($sheet, [System.Drawing.Imaging.ImageFormat]::Png)
    Write-Host "contact_sheet=$sheet"
  } finally {
    $g.Dispose()
    $bmp.Dispose()
  }
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

if ($Build) {
  Run-Step "build" {
    cmake --build $cmakeBuildDir --target ghogx_app -j 4
    if ($LASTEXITCODE -ne 0) { throw "build failed with exit $LASTEXITCODE" }
  }
}

if (-not (Test-Path -LiteralPath $exe)) {
  throw "missing executable: $exe"
}
if (-not (Test-Path -LiteralPath (Join-Path $ArkDir "MAIN.HDR")) -and
    -not (Test-Path -LiteralPath (Join-Path $ArkDir "main.hdr"))) {
  throw "missing MAIN.HDR/main.hdr under $ArkDir"
}

Run-Capture "ingame_inactive_hud" $InactiveScore $InactiveStreak `
  $InactiveMultiplier $InactiveSp $InactiveRock $false
Run-Capture "ingame_active_hud" $ActiveScore $ActiveStreak `
  $ActiveMultiplier $ActiveSp $ActiveRock $true
Make-ContactSheet

Write-Host "out_dir=$OutDir"
