# smoke_trace.ps1 - headless variant of smoke_play.ps1 for trace-360 work.
#
# Differences vs smoke_play.ps1 (kept alongside, not replacing):
#  - Launches the trace-360 build (headless gh2test with WH_CBT focus
#    suppression + SW_HIDE in OnPostSetup; window never visible).
#  - Input is delivered via PostMessage(hwnd, WM_KEYDOWN/UP, ...) so
#    no foreground focus is needed and the user's active app is not
#    disrupted. The original keybd_event path is forbidden per
#    360_TRACE_PLAN.md "Headless execution" hard constraint.
#  - No screenshots (window is hidden; PrintWindow on a D3D12 swapchain
#    is unreliable). Verification is via log tail + trace jsonl event
#    counts instead.
#
# What it does:
#   1. Launch trace-360 gh2test (which auto-starts capture from
#      OnPostSetup; trace_<epoch>.jsonl lands under <exe_dir>/captures/).
#   2. Wait for title screen (log shows SetInterruptCallback).
#   3. PostMessage the menu navigation sequence (same as smoke_play.ps1).
#   4. Hold for $GameplayHoldSec to let the song chart play as long as
#      possible (until first-fail or end of $GameplayHoldSec, whichever
#      comes first). Even a few seconds of in-song trace will surface
#      the per-frame tick functions we need for Phase 2/4.
#   5. Kill the process. Print where the trace landed + event count.
#
# Capture gating is currently always-on in gh2test_app.h::OnPostSetup;
# the whole boot+gameplay window is recorded. Phase 3 plan calls for
# explicit on/off around the gameplay window once we have a signaling
# mechanism; for now the all-events trace is small enough (~7500 events
# / 10 seconds boot) that filtering offline is fine.

param(
  [int]$TitleWaitSec    = 12,
  [int]$StepWaitSec     = 3,
  [int]$GameplayHoldSec = 30,
  # If set, skip the menu-nav PostMessage sequence and just leave the
  # game running for the user to drive with their physical controller
  # (Xbox One Controller via SDL; controller mode is on by default in
  # GH2 360). Use this for the successful-gameplay-capture session.
  # The script will hold the process alive for $InteractiveHoldMin
  # minutes (default 10), then save the trace and exit cleanly.
  [switch]$NoAutoNav,
  [int]$InteractiveHoldMin = 10
)

Add-Type @"
using System;
using System.Runtime.InteropServices;
public class P {
  [DllImport("user32.dll")] public static extern bool PostMessageW(IntPtr h, uint msg, IntPtr wParam, IntPtr lParam);
  [DllImport("user32.dll")] public static extern IntPtr FindWindowExW(IntPtr parent, IntPtr child, string cls, string title);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  public delegate bool EnumWindowsProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc cb, IntPtr l);
}
"@

$WM_KEYDOWN = 0x0100
$WM_KEYUP   = 0x0101

$VK = @{
  Space = 0x20; Enter = 0x0D; Esc = 0x1B; Tab = 0x09
  Up = 0x26; Down = 0x28; Left = 0x25; Right = 0x27
}

function Send-Key([IntPtr]$hwnd, [string]$name, [int]$holdMs = 80) {
  $vk = [int]$VK[$name]
  [P]::PostMessageW($hwnd, $WM_KEYDOWN, [IntPtr]$vk, [IntPtr]0) | Out-Null
  Start-Sleep -Milliseconds $holdMs
  [P]::PostMessageW($hwnd, $WM_KEYUP,   [IntPtr]$vk, [IntPtr]0) | Out-Null
  Start-Sleep -Milliseconds 120
}

# Find a top-level window owned by our PID, even though it's hidden.
# MainWindowHandle on Get-Process is 0 for hidden windows; EnumWindows works.
function Find-PidWindow([int]$pid_) {
  $found = [IntPtr]::Zero
  $cb = [P+EnumWindowsProc]{
    param([IntPtr]$h, [IntPtr]$l)
    $owner = 0
    [P]::GetWindowThreadProcessId($h, [ref]$owner) | Out-Null
    if ($owner -eq $pid_) {
      $script:_found_hwnd = $h
      return $false
    }
    return $true
  }
  $script:_found_hwnd = [IntPtr]::Zero
  [P]::EnumWindows($cb, [IntPtr]::Zero) | Out-Null
  return $script:_found_hwnd
}

$exe = "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\out\build\win-amd64-relwithdebinfo\gh2test.exe"
if (-not (Test-Path $exe)) {
  Write-Host "trace-360 exe not built yet: $exe"
  exit 1
}

$p = Start-Process -FilePath $exe `
  -ArgumentList @(
    '--game_data_root="C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\assets"',
    # MnK is re-enabled. The mouse-arrest symptom is solved at the EXE
    # level by an inline hook on user32!SetCursorPos that no-ops the
    # cursor-warp call MnK makes every frame (see src/main.cpp
    # InstallSetCursorPosHook). With that hook in place, MnK can still
    # translate our PostMessage WM_KEYDOWN/UP -> guest controller
    # buttons but never moves the user's cursor.
    # Previous attempt (mnk_mode OMITTED) broke menu navigation because
    # without MnK our keyboard messages weren't routed to the guest as
    # controller input.
    '--mnk_mode=true',
    '--mnk_user_index=1'
  ) `
  -WorkingDirectory (Split-Path $exe) -PassThru
Write-Host "Launched gh2test (trace-360) PID $($p.Id); waiting $TitleWaitSec s for title screen..."
Start-Sleep -Seconds $TitleWaitSec

if ($p.HasExited) {
  Write-Host "process died before title screen, exit=0x{0:X8}" -f $p.ExitCode
  exit 1
}

$hwnd = Find-PidWindow $p.Id
if ($hwnd -eq [IntPtr]::Zero) {
  Write-Host "no top-level window for PID $($p.Id) (headless may have hidden it before enum; retrying in 3s)"
  Start-Sleep -Seconds 3
  $hwnd = Find-PidWindow $p.Id
}
Write-Host "hwnd=0x$([Convert]::ToString($hwnd.ToInt64(), 16))"

if ($NoAutoNav) {
  $totalSec = [int]($InteractiveHoldMin * 60)
  Write-Host ""
  Write-Host "=========================================================" -ForegroundColor Cyan
  Write-Host " --no-autonav mode: game is running, hidden, muted, cursor pinned." -ForegroundColor Cyan
  Write-Host " Your Xbox controller (already detected by SDL) should drive the menus." -ForegroundColor Cyan
  Write-Host " Holding for $InteractiveHoldMin minutes ($totalSec s) before auto-exit." -ForegroundColor Cyan
  Write-Host " Ctrl+C this window early when you're done - the trace flushes on shutdown." -ForegroundColor Cyan
  Write-Host "=========================================================" -ForegroundColor Cyan
  Write-Host ""
  $sw = [System.Diagnostics.Stopwatch]::StartNew()
  while ($sw.Elapsed.TotalSeconds -lt $totalSec) {
    if ($p.HasExited) { Write-Host "process exited early"; break }
    Start-Sleep -Seconds 5
    $remaining = [int]($totalSec - $sw.Elapsed.TotalSeconds)
    if ($remaining -gt 0 -and ($remaining % 60 -lt 5)) {
      Write-Host "  ... $([int]($remaining/60)) min remaining"
    }
  }
  if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force }
  $captures = Join-Path (Split-Path $exe) "captures"
  $latest = Get-ChildItem $captures -Filter "trace_*.jsonl" -ErrorAction SilentlyContinue |
            Sort-Object LastWriteTime -Descending | Select-Object -First 1
  if ($latest) {
    $lines = (Get-Content $latest.FullName | Measure-Object -Line).Lines
    Write-Host "trace: $($latest.FullName) ($lines events)"
  }
  return
}

# Menu nav — same sequence as smoke_play.ps1.
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
  if ($p.HasExited) { Write-Host "exited mid-plan"; break }
  Write-Host "step: $($s.label) - PostMessage $($s.key), wait $($s.wait)s"
  Send-Key $hwnd $s.key
  Start-Sleep -Seconds $s.wait
}

Write-Host "Holding $GameplayHoldSec s for gameplay capture (will likely fail-out before then; that's fine)..."
Start-Sleep -Seconds $GameplayHoldSec

if (-not $p.HasExited) {
  Write-Host "stopping process"
  Stop-Process -Id $p.Id -Force
}

$captures = Join-Path (Split-Path $exe) "captures"
$latest = Get-ChildItem $captures -Filter "trace_*.jsonl" -ErrorAction SilentlyContinue |
          Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($latest) {
  $lines = (Get-Content $latest.FullName | Measure-Object -Line).Lines
  Write-Host "trace: $($latest.FullName) ($lines events)"
} else {
  Write-Host "no trace jsonl found under $captures"
}

$logs = Join-Path (Split-Path $exe) "logs"
$ll = Get-ChildItem $logs -Filter "gh2test_*.log" -ErrorAction SilentlyContinue |
      Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($ll) {
  Write-Host "--- log tail ---"
  Get-Content $ll.FullName -Tail 15
}
