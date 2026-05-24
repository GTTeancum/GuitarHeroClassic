# Smoke path: boot gh2test, drive the menus to gameplay via synthetic input,
# screenshot at each step. No binary patches.
param(
  [int]$TitleWaitSec = 12,        # how long to wait before assuming title screen is ready
  [int]$StepWaitSec  = 4,         # delay between menu actions
  [string]$ShotDir   = "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX\smoke_shots"
)

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

# Win32 imports for window focus + raw key events
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class W {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int n);
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
}
"@

# VK codes
$VK = @{
  Space = 0x20; Enter = 0x0D; Esc = 0x1B; Tab = 0x09
  Up = 0x26; Down = 0x28; Left = 0x25; Right = 0x27
  Shift = 0x10; R = 0x52; E = 0x45; Q = 0x51; F = 0x46
  A = 0x41; D = 0x44; S = 0x53; W = 0x57
}
$KEYUP = 0x0002

function Send-Key([string]$name, [int]$holdMs = 80) {
  $vk = [byte]$VK[$name]
  [W]::keybd_event($vk, 0, 0, [UIntPtr]::Zero)        # down
  Start-Sleep -Milliseconds $holdMs
  [W]::keybd_event($vk, 0, $KEYUP, [UIntPtr]::Zero)   # up
  Start-Sleep -Milliseconds 120
}

function Focus-Window($hwnd) {
  [W]::ShowWindow($hwnd, 9) | Out-Null  # SW_RESTORE
  [W]::SetForegroundWindow($hwnd) | Out-Null
  Start-Sleep -Milliseconds 250
}

function Shot($name) {
  $f = Join-Path $ShotDir ("{0:D2}_{1}.png" -f $script:step, $name)
  $bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
  $bmp = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
  $bmp.Save($f, [System.Drawing.Imaging.ImageFormat]::Png)
  $g.Dispose(); $bmp.Dispose()
  Write-Host "[shot] $f"
  $script:step++
}

if (-not (Test-Path $ShotDir)) { New-Item -ItemType Directory -Path $ShotDir | Out-Null }
Get-ChildItem $ShotDir -File | Remove-Item -Force
$script:step = 1

$exe = "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX\out\build\win-amd64-release\gh2test.exe"
$p = Start-Process -FilePath $exe `
  -ArgumentList @(
    '--game_data_root="C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX\assets"',
    '--mnk_mode=true',
    '--mnk_user_index=1'
  ) `
  -WorkingDirectory (Split-Path $exe) -PassThru
Write-Host "Launched gh2test PID $($p.Id), waiting $TitleWaitSec s for title screen..."
Start-Sleep -Seconds $TitleWaitSec

$proc = Get-Process -Id $p.Id -ErrorAction SilentlyContinue
if (-not $proc) {
  Write-Host "process died before title screen"
  return
}

$hwnd = $proc.MainWindowHandle
Write-Host "hwnd=$hwnd title='$($proc.MainWindowTitle)'"
Focus-Window $hwnd
Shot "title_screen"

# Sequence to try: A (dismiss splash) -> A (any submenu prompt) -> Down -> A (Quick Play) -> spam A through char/guitar/venue/song-difficulty
$plan = @(
  @{ key='Space'; label='A_press_to_begin'; wait=4 },
  @{ key='Space'; label='A_main_menu_confirm'; wait=3 },
  @{ key='Down';  label='down_to_quickplay'; wait=2 },
  @{ key='Space'; label='A_select_quickplay'; wait=4 },
  @{ key='Space'; label='A_default_difficulty'; wait=3 },
  @{ key='Space'; label='A_default_character'; wait=3 },
  @{ key='Space'; label='A_default_guitar'; wait=3 },
  @{ key='Space'; label='A_default_venue'; wait=3 },
  @{ key='Space'; label='A_first_song'; wait=4 },
  @{ key='Space'; label='A_song_confirm'; wait=8 }
)

foreach ($s in $plan) {
  $proc.Refresh()
  if ($proc.HasExited) {
    Write-Host "Process exited unexpectedly"
    break
  }
  Focus-Window $hwnd
  Write-Host "step: $($s.label) - send $($s.key), wait $($s.wait)s"
  Send-Key $s.key
  Start-Sleep -Seconds $s.wait
  Shot $s.label
}

# Final long wait to see if gameplay actually starts
Start-Sleep -Seconds 6
$proc.Refresh()
if (-not $proc.HasExited) {
  Focus-Window $hwnd
  Shot "final"
  Write-Host "still alive at end - killing"
  Stop-Process -Id $p.Id -Force
} else {
  Write-Host "process exited 0x{0:X8}" -f $proc.ExitCode
}

Write-Host "Done. Shots in $ShotDir"
$latest = Get-ChildItem "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX\out\build\win-amd64-release\logs\gh2test_*.log" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
Write-Host "--- log tail ---"
Get-Content $latest.FullName -Tail 20
