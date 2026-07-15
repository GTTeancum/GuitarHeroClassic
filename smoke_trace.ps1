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
  # Optional navigation offsets for Quick Play. These keep the proven
  # headless route intact, but let trace runs move on the song/character
  # lists before accepting the default item.
  [int]$SongDown = 0,
  [int]$CharacterDown = 0,
  # Guitar-controller list navigation is strum-driven on some screens. These
  # counts send Right-arrow, which the trace build can bridge into strum_dn
  # via --trace_dpad_right_strum_dn.
  [int]$SongStrumDown = 0,
  [int]$CharacterStrumDown = 0,
  [int]$SelectionNavWaitMs = 350,
  [int]$SongListReadyWaitMs = 0,
  [int]$CharacterListReadyWaitMs = 0,
  # Pass the focused lower-body memory trace flag into gh2test. This keeps
  # leg capture runs explicit instead of making every smoke trace huge.
  [switch]$TraceLowerBodyMemory,
  # Drive the menu path through a trace-only XInput hook inside gh2test.
  # This avoids foreground focus and hidden-window keyboard delivery.
  [switch]$TraceScriptedNav,
  # Extra raw gh2test arguments for focused tracing. Prefer named switches
  # above when one exists, but keep this for one-off source probes.
  [string[]]$ExtraLaunchArg = @(),
  # If set, skip the menu-nav PostMessage sequence and just leave the
  # game running for the user to drive with their physical controller
  # (Xbox One Controller via SDL; controller mode is on by default in
  # GH2 360). Use this for the successful-gameplay-capture session.
  # The script will hold the process alive for $InteractiveHoldMin
  # minutes (default 10), then save the trace and exit cleanly.
  [switch]$NoAutoNav,
  [int]$InteractiveHoldMin = 10,
  # Run the song to natural completion and capture the post-song sequence
  # (results screen, star animation, character victory/defeat reaction,
  # SP effects if activated mid-song by the autoplay hook).
  # Overrides GameplayHoldSec.  Holds up to FullSongMaxSec (default 420s /
  # 7 min), which covers any GH2 song + ~90s of results screen.
  # Stops early if the process exits on its own.
  [switch]$FullSong,
  [int]$FullSongMaxSec  = 420,
  # Capture pause menu + fail state + game-over sequence.
  # Launches with --no_autoplay (all gems missed -> fail meter drains).
  # Sequence:
  #   1. Normal menu nav into song.
  #   2. Wait $FailSongPauseAt seconds (default 15s) into gameplay.
  #   3. Send Esc (Start button = pause).
  #   4. Wait $FailSongPauseHoldSec seconds (default 4s) on pause screen.
  #   5. Send Esc again (unpause).
  #   6. Wait up to $FailSongMaxSec seconds for process to exit or timeout.
  # Covers: pause menu open/close, fail meter depletion, game-over screen,
  # fail character animation, post-fail results.
  [switch]$FailSong,
  [int]$FailSongPauseAt     = 15,
  [int]$FailSongPauseHoldSec = 4,
  [int]$FailSongMaxSec      = 120,
  # Navigate to Practice mode instead of Quick Play.
  # Plan: title -> main menu -> Down x2 -> select Practice -> difficulty ->
  # song -> section select -> speed select -> play.
  # Autoplay stays on so gems are hit; hold for PracticeHoldSec (default 60s)
  # to capture practice-mode-specific props (slow_music on speed change,
  # section loop, practice UI overlays, etc.).
  [switch]$PracticeMode,
  [int]$PracticeHoldSec = 60,
  # Pulse the whammy bar (right trigger via LMB/WM_LBUTTONDOWN) every
  # $Whammy_IntervalMs ms during gameplay.  Full-press for ~half the interval,
  # off for the other half.  Works with any gameplay mode (-FullSong, default).
  # keybind_right_trigger = "LMB" in MnK driver; LMB = right trigger 0xFF.
  [switch]$Whammy,
  [int]$WhamwyIntervalMs = 500,
  # Interactive / visible mode.  Pass --show_window to keep the game window
  # visible so you can navigate menus and gameplay with the keyboard.
  # Keybinds remapped to guitar-friendly layout (ASDFG = frets in order,
  # standard Xbox 360 GH guitar controller mapping, sub_type=0x06):
  #   A = Green  fret (keybind_a             / XInput A   0x1000)
  #   S = Red    fret (keybind_b             / XInput B   0x2000)
  #   D = Yellow fret (keybind_y             / XInput Y   0x8000)
  #   F = Blue   fret (keybind_x             / XInput X   0x4000)
  #   G = Orange fret (keybind_left_shoulder / XInput LB  0x0100)
  #   CONTROLLER MODE (use_joypad, forced on via JoypadConfig_SetJoypadMode hook):
  #   pressing a fret on the beat auto-strums — NO separate strum key needed.
  #   Device reports sub_type=0x01 (GAMEPAD) so GH2 routes to controller mode,
  #   not the real-guitar peripheral path (which would demand a manual strum).
  #   Arrow keys = D-pad navigation (unchanged)
  #   Escape = Start/Pause (unchanged)
  #   LMB = whammy/right-trigger (unchanged)
  # Key forwarding uses GetAsyncKeyState (global) + PostMessage so keys are
  # detected regardless of whether the game or PowerShell console has focus.
  # No auto-nav: you drive the menus yourself.
  # Holds for InteractiveHoldMin minutes (default 10, shared with -NoAutoNav) then exits.
  [switch]$Interactive,
  [string]$GameDataRoot = "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX\assets"
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
  // Returns the async (hardware) state of a virtual key, regardless of focus.
  // High bit (0x8000) set = key is currently pressed.
  [DllImport("user32.dll")] public static extern short GetAsyncKeyState(int vKey);
}
"@

$WM_KEYDOWN     = 0x0100
$WM_KEYUP       = 0x0101
$WM_CLOSE       = 0x0010
$WM_LBUTTONDOWN = 0x0201
$WM_LBUTTONUP   = 0x0202
$MK_LBUTTON     = 0x0001

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

# Pulse the whammy bar once: LMB down for holdMs, then LMB up.
# MnK maps keybind_right_trigger = "LMB" -> right trigger 0xFF.
# In GH2 joypad mode, right trigger is the whammy axis.
function Send-WhamPulse([IntPtr]$hwnd, [int]$holdMs = 230) {
  [P]::PostMessageW($hwnd, $WM_LBUTTONDOWN, [IntPtr]$MK_LBUTTON, [IntPtr]0) | Out-Null
  Start-Sleep -Milliseconds $holdMs
  [P]::PostMessageW($hwnd, $WM_LBUTTONUP,   [IntPtr]0,            [IntPtr]0) | Out-Null
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

function Stop-TraceProcess([System.Diagnostics.Process]$proc, [IntPtr]$hwnd, [string]$reason) {
  if ($proc.HasExited) { return }
  Write-Host "$reason; requesting clean trace shutdown"
  $target = $hwnd
  if ($target -eq [IntPtr]::Zero) {
    $target = Find-PidWindow $proc.Id
  }
  if ($target -ne [IntPtr]::Zero) {
    [P]::PostMessageW($target, $WM_CLOSE, [IntPtr]0, [IntPtr]0) | Out-Null
  }
  for ($i = 0; $i -lt 60; $i++) {
    if ($proc.HasExited) {
      Write-Host "process exited cleanly"
      return
    }
    Start-Sleep -Milliseconds 250
  }
  if (-not $proc.HasExited) {
    Write-Host "clean shutdown timed out; forcing stop"
    Stop-Process -Id $proc.Id -Force
  }
}

$exe = "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\out\build\win-amd64-relwithdebinfo\gh2test.exe"
if (-not (Test-Path $exe)) {
  Write-Host "trace-360 exe not built yet: $exe"
  exit 1
}

$launchArgs = [System.Collections.Generic.List[string]]@(
  "--game_data_root=`"$GameDataRoot`"",
  # MnK is re-enabled. The mouse-arrest symptom is solved at the EXE
  # level by an inline hook on user32!SetCursorPos that no-ops the
  # cursor-warp call MnK makes every frame (see src/main.cpp
  # InstallSetCursorPosHook). With that hook in place, MnK can still
  # translate our PostMessage WM_KEYDOWN/UP -> guest controller
  # buttons but never moves the user's cursor.
  # Previous attempt (mnk_mode OMITTED) broke menu navigation because
  # without MnK our keyboard messages weren't routed to the guest as
  # controller input.
  '--mnk_mode=true'
)
# MnK user index: Interactive uses port 0 (player 1) so physical keyboard
# input reaches GH2's player-1 XInputGetState(0,...) poll.
# Headless uses port 1 so PostMessage nav doesn't collide with SDL at port 0.
$launchArgs.Add($(if ($Interactive) { '--mnk_user_index=0' } else { '--mnk_user_index=1' }))
# -FailSong: disable autoplay so all gems are missed -> fail meter drains.
if ($FailSong) { $launchArgs.Add('--no_autoplay') }
if ($SongStrumDown -gt 0 -or $CharacterStrumDown -gt 0) {
  $launchArgs.Add('--trace_dpad_right_strum_dn')
}
if ($TraceLowerBodyMemory) { $launchArgs.Add('--trace-lower-body-memory') }
if ($TraceScriptedNav) { $launchArgs.Add('--trace_scripted_nav') }
foreach ($extra in $ExtraLaunchArg) {
  if (-not [string]::IsNullOrWhiteSpace($extra)) { $launchArgs.Add($extra) }
}
# -Interactive: show the window and remap keys to ASDFG guitar frets.
if ($Interactive) {
  $launchArgs.Add('--show_window')
  # Interactive play REQUIRES autoplay OFF.  With autoplay on (the default,
  # compiled in via -DAUTOPLAY_ENABLED), GemPass_VtableDispatch forces every
  # gem to HIT regardless of input — so the notes you see hitting are autoplay,
  # not your key presses, and you can't tell whether your frets register.
  # --no_autoplay disables the HIT-forcing so YOUR input drives note hits.
  $launchArgs.Add('--no_autoplay')
  # Standard Xbox 360 GH guitar controller button -> fret mapping (sub_type=0x06):
  #   A  button (0x1000) = Green  fret
  #   B  button (0x2000) = Red    fret
  #   Y  button (0x8000) = Yellow fret
  #   X  button (0x4000) = Blue   fret
  #   LB button (0x0100) = Orange fret
  #   DPAD_UP / DPAD_DOWN = strum bar (arrow keys, unchanged)
  #   Right trigger = whammy bar (LMB click, unchanged)
  # ASDFG keys map left-to-right in fret order: Green-Red-Yellow-Blue-Orange.
  # ----------------------------------------------------------------------
  # FRET-SWEEP MEASUREMENT BINDING (2026-05-29)
  #
  # Static analysis of the recomp can NOT predict which XInput button lights
  # which fret colour: between XInput and the lane there are several
  # data-driven remap layers (RemapButtons engine-word -> joypad button
  # index -> per-lane NoteTracker watchers keyed by the {5,7,9,36,96}
  # indices, polled through the symbol/message system).  Every derivation
  # contradicted the observed "G=Red, A=Orange, rest dead".  So we MEASURE:
  # bind a DISTINCT key to every candidate XInput button, press each once,
  # and read the per-keypress engine word from the log line
  #   "[GuitarPort] IN wButtons=... => OUT engine_word=0x..."
  # cross-referenced with the fret colour the tester reports.
  #
  # Each key maps 1:1 to exactly one XInput button (no collisions), so each
  # press produces exactly one engine bit.  Confirmed so far: LB->Red,
  # A->Orange.  Sweep finds Green / Yellow / Blue.
  #
  #   key  cvar                    XInput button  engine bit (from remap dump)
  #   A    keybind_a               A              6   (Orange - confirmed)
  #   S    keybind_b               B              4
  #   D    keybind_x               X              7
  #   F    keybind_y               Y              5
  #   G    keybind_left_shoulder   LB             2   (Red - confirmed)
  #   H    keybind_right_shoulder  RB             3
  #   I    keybind_back            BACK           8
  #   O    keybind_lstick_press    LSTICK         9
  #   P    keybind_rstick_press    RSTICK         10
  #   Up/Down/Left/Right (arrows)  DPAD U/D/L/R   12/14/15/13
  # ----------------------------------------------------------------------
  $launchArgs.Add('--keybind_a=A')               # A key -> XInput A    (bit 6)
  $launchArgs.Add('--keybind_b=S')               # S key -> XInput B    (bit 4)
  $launchArgs.Add('--keybind_x=D')               # D key -> XInput X    (bit 7)
  $launchArgs.Add('--keybind_y=F')               # F key -> XInput Y    (bit 5)
  $launchArgs.Add('--keybind_left_shoulder=G')   # G key -> XInput LB   (bit 2)
  $launchArgs.Add('--keybind_right_shoulder=H')  # H key -> XInput RB   (bit 3)
  $launchArgs.Add('--keybind_back=I')            # I key -> XInput BACK (bit 8)
  $launchArgs.Add('--keybind_lstick_press=O')    # O key -> XInput LSTICK(bit 9)
  $launchArgs.Add('--keybind_rstick_press=P')    # P key -> XInput RSTICK(bit10)
  # Clear default keybinds that would double-fire on a sweep key:
  #   keybind_lstick_left/down/right default to A/S/D -> would add an analog
  #   axis on top of the fret button.  Park them on Numpad (unreachable).
  $launchArgs.Add('--keybind_lstick_left=Numpad4')
  $launchArgs.Add('--keybind_lstick_down=Numpad2')
  $launchArgs.Add('--keybind_lstick_right=Numpad6')
}

$p = Start-Process -FilePath $exe `
  -ArgumentList $launchArgs `
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
  Stop-TraceProcess $p $hwnd "no-autonav hold elapsed"
  $captures = Join-Path (Split-Path $exe) "captures"
  $latest = Get-ChildItem $captures -Filter "trace_*.jsonl" -ErrorAction SilentlyContinue |
            Sort-Object LastWriteTime -Descending | Select-Object -First 1
  if ($latest) {
    $lines = (Get-Content $latest.FullName | Measure-Object -Line).Lines
    Write-Host "trace: $($latest.FullName) ($lines events)"
  }
  return
}

# Menu nav — Quick Play path (default).
$plan_quickplay = @(
  @{ key='Space'; label='A_press_to_begin'; wait=4 },
  @{ key='Space'; label='A_main_menu_confirm'; wait=3 },
  @{ key='Down';  label='down_to_quickplay'; wait=2 },
  @{ key='Space'; label='A_select_quickplay'; wait=4 },
  @{ key='Space'; label='A_default_difficulty'; wait=3 },
  @{ key='Space'; label='A_default_character'; wait=3; after='character_list' },
  @{ key='Space'; label='A_default_guitar'; wait=3; after='song_list' },
  @{ key='Space'; label='A_default_venue'; wait=3 },
  @{ key='Space'; label='A_first_song'; wait=4 },
  @{ key='Space'; label='A_song_confirm'; wait=8 }
)

# Practice mode path confirmed from interactive trace (trace_1780069917.jsonl):
#   Title -> main menu -> Down x2 -> Practice -> song list -> part select
#   -> difficulty select -> section select -> speed select -> gameplay.
# Screen load order: sel_song_quickplay.milo_xbox (t=24.8s)
#                    practice_selpart.milo_xbox   (t=26.9s, +2.1s)
#                    sel_diff_practice.milo_xbox  (t=30.1s, +3.2s)
#                    practice_sel_section.milo_xbox (t=34.0s, +3.9s)
# Wait times are generous (5s) to absorb load time after each Space press.
$plan_practice = @(
  @{ key='Space'; label='A_press_to_begin'; wait=4 },
  @{ key='Space'; label='A_main_menu_confirm'; wait=3 },
  @{ key='Down';  label='down_1'; wait=1 },
  @{ key='Down';  label='down_2_practice'; wait=2 },
  @{ key='Space'; label='A_select_practice'; wait=5 },
  @{ key='Space'; label='A_first_song'; wait=4 },      # song list -> part select (loads in ~2s)
  @{ key='Space'; label='A_default_part'; wait=5 },    # part select -> difficulty select (loads in ~3s)
  @{ key='Space'; label='A_default_difficulty'; wait=5 }, # difficulty -> section select (loads in ~4s)
  @{ key='Space'; label='A_default_section'; wait=5 }, # section select -> speed select (loads in ~4s)
  @{ key='Space'; label='A_default_speed'; wait=5 }    # speed select -> gameplay
)

$plan = if ($PracticeMode) { $plan_practice } else { $plan_quickplay }

$activePlan = if ($Interactive) { @() } else { $plan }
foreach ($s in $activePlan) {
  if ($p.HasExited) { Write-Host "exited mid-plan"; break }
  Write-Host "step: $($s.label) - PostMessage $($s.key), wait $($s.wait)s"
  Send-Key $hwnd $s.key
  Start-Sleep -Seconds $s.wait
  if ($s.after -eq 'song_list' -and $SongDown -gt 0) {
    if ($SongListReadyWaitMs -gt 0) { Start-Sleep -Milliseconds $SongListReadyWaitMs }
    Write-Host "step: SongDown x$SongDown"
    for ($i = 0; $i -lt $SongDown; $i++) {
      Send-Key $hwnd 'Down'
      Start-Sleep -Milliseconds $SelectionNavWaitMs
    }
  }
  if ($s.after -eq 'song_list' -and $SongStrumDown -gt 0) {
    if ($SongListReadyWaitMs -gt 0) { Start-Sleep -Milliseconds $SongListReadyWaitMs }
    Write-Host "step: SongStrumDown x$SongStrumDown"
    for ($i = 0; $i -lt $SongStrumDown; $i++) {
      Send-Key $hwnd 'Right'
      Start-Sleep -Milliseconds $SelectionNavWaitMs
    }
  }
  if ($s.after -eq 'character_list' -and $CharacterDown -gt 0) {
    if ($CharacterListReadyWaitMs -gt 0) { Start-Sleep -Milliseconds $CharacterListReadyWaitMs }
    Write-Host "step: CharacterDown x$CharacterDown"
    for ($i = 0; $i -lt $CharacterDown; $i++) {
      Send-Key $hwnd 'Down'
      Start-Sleep -Milliseconds $SelectionNavWaitMs
    }
  }
  if ($s.after -eq 'character_list' -and $CharacterStrumDown -gt 0) {
    if ($CharacterListReadyWaitMs -gt 0) { Start-Sleep -Milliseconds $CharacterListReadyWaitMs }
    Write-Host "step: CharacterStrumDown x$CharacterStrumDown"
    for ($i = 0; $i -lt $CharacterStrumDown; $i++) {
      Send-Key $hwnd 'Right'
      Start-Sleep -Milliseconds $SelectionNavWaitMs
    }
  }
}

if ($FailSong) {
  # Fail-song mode: autoplay is OFF (--no_autoplay passed at launch).
  # All gems are missed; fail meter drains to zero in ~30-60s.
  # Sequence: wait PauseAt -> Esc (pause) -> wait PauseHold -> Esc (unpause)
  # -> wait for game-over screen -> kill after FailSongMaxSec total.
  # Covers: pause menu open/close, fail meter depletion, game-over screen,
  # fail character animation.
  Write-Host ""
  Write-Host "=========================================================" -ForegroundColor Red
  Write-Host " Fail-song mode: autoplay OFF, all gems missed." -ForegroundColor Red
  Write-Host (" Pausing at {0}s, unpausing after {1}s, timeout {2}s." -f $FailSongPauseAt, $FailSongPauseHoldSec, $FailSongMaxSec) -ForegroundColor Red
  Write-Host " Covers: pause menu, fail meter, game-over screen." -ForegroundColor Red
  Write-Host " Ctrl+C to stop early — trace flushes on shutdown." -ForegroundColor Red
  Write-Host "=========================================================" -ForegroundColor Red
  Write-Host ""
  $sw = [System.Diagnostics.Stopwatch]::StartNew()

  # Step 1: wait until PauseAt seconds into gameplay.
  while ($sw.Elapsed.TotalSeconds -lt $FailSongPauseAt) {
    if ($p.HasExited) { Write-Host "process exited before pause point"; break }
    Start-Sleep -Seconds 1
  }
  if (-not $p.HasExited) {
    Write-Host ("  [{0}s] Sending Esc (pause)" -f [int]$sw.Elapsed.TotalSeconds)
    Send-Key $hwnd 'Esc'
    Start-Sleep -Seconds $FailSongPauseHoldSec
  }

  # Step 2: unpause.
  if (-not $p.HasExited) {
    Write-Host ("  [{0}s] Sending Esc (unpause)" -f [int]$sw.Elapsed.TotalSeconds)
    Send-Key $hwnd 'Esc'
  }

  # Step 3: wait for game-over or timeout.
  while ($sw.Elapsed.TotalSeconds -lt $FailSongMaxSec) {
    if ($p.HasExited) { Write-Host ("Process exited at {0}s" -f [int]$sw.Elapsed.TotalSeconds); break }
    Start-Sleep -Seconds 5
    $elapsed = [int]$sw.Elapsed.TotalSeconds
    if (($elapsed % 15) -lt 5 -and $elapsed -gt $FailSongPauseAt) {
      Write-Host ("  ... {0}s elapsed" -f $elapsed)
    }
  }
  if (-not $p.HasExited) {
    Stop-TraceProcess $p $hwnd ("Fail-song timeout reached at {0}s" -f [int]$sw.Elapsed.TotalSeconds)
  }
} elseif ($Interactive) {
  # Interactive key-forwarding loop.
  #
  # Rather than fighting Windows OS focus (SetForegroundWindow is unreliable
  # from a non-foreground process), we use GetAsyncKeyState to poll the
  # hardware key state GLOBALLY -- regardless of which window has focus --
  # and forward rising/falling edges via PostMessage to the game window.
  # This is identical to how the headless auto-nav works and is proven to
  # reach MnkInputDriver regardless of window visibility or focus state.
  #
  # The user keeps the PowerShell console in focus; the game window is visible
  # alongside it.  Keys pressed anywhere are forwarded to the game.
  $totalSec = [int]($InteractiveHoldMin * 60)
  Write-Host ""
  Write-Host "=========================================================" -ForegroundColor Green
  Write-Host " Interactive mode — FRET-SWEEP MEASUREMENT." -ForegroundColor Green
  Write-Host " Type here in this console window (no need to click game)." -ForegroundColor Green
  Write-Host "" -ForegroundColor Green
  Write-Host " 1) Navigate into a song:  Space = confirm, arrows = move, Shift = back." -ForegroundColor Green
  Write-Host " 2) Once the highway is scrolling, press each sweep key ONE AT A TIME" -ForegroundColor Green
  Write-Host "    and note which fret COLOUR lights up (or 'nothing'):" -ForegroundColor Green
  Write-Host "" -ForegroundColor Green
  Write-Host "      A  S  D  F  G  H  I  O  P   and the four ARROW keys" -ForegroundColor Yellow
  Write-Host "" -ForegroundColor Green
  Write-Host "    (each key = exactly one controller button; A=Orange and G=Red" -ForegroundColor Green
  Write-Host "     are already confirmed — we're hunting Green / Yellow / Blue.)" -ForegroundColor Green
  Write-Host "" -ForegroundColor Green
  Write-Host " Every press is logged as a [GuitarPort] IN wButtons=... line, so" -ForegroundColor Green
  Write-Host " report results as e.g.  'H = Yellow, I = nothing, Left = Green'." -ForegroundColor Green
  Write-Host (" Ctrl+C to exit early. Auto-exit in {0} min." -f $InteractiveHoldMin) -ForegroundColor Green
  Write-Host "=========================================================" -ForegroundColor Green
  Write-Host ""

  # VK codes to intercept and forward.
  # Fret-sweep set: every key below maps 1:1 to a single XInput button, so
  # each press produces exactly one engine bit in the [GuitarPort] log line.
  $fwdVKs = @(
    0x20,  # Space  -> A button (menu confirm)
    0x1B,  # Escape -> Start / Pause
    0x10,  # Shift  -> B button (menu back)
    0x26,  # Up     -> DPAD_UP    (engine bit 12)
    0x28,  # Down   -> DPAD_DOWN  (engine bit 14)
    0x25,  # Left   -> DPAD_LEFT  (engine bit 15)
    0x27,  # Right  -> DPAD_RIGHT (engine bit 13)
    0x41,  # A      -> XInput A   (engine bit 6)  [Orange - confirmed]
    0x53,  # S      -> XInput B   (engine bit 4)
    0x44,  # D      -> XInput X   (engine bit 7)
    0x46,  # F      -> XInput Y   (engine bit 5)
    0x47,  # G      -> XInput LB  (engine bit 2)  [Red - confirmed]
    0x48,  # H      -> XInput RB  (engine bit 3)
    0x49,  # I      -> XInput BACK(engine bit 8)
    0x4F,  # O      -> XInput LSTICK (engine bit 9)
    0x50   # P      -> XInput RSTICK (engine bit 10)
  )

  # Track previous key state so we only fire on edges (not every poll tick).
  $prevDown = @{}
  foreach ($vk in $fwdVKs) { $prevDown[$vk] = $false }

  $sw = [System.Diagnostics.Stopwatch]::StartNew()
  while ($sw.Elapsed.TotalSeconds -lt $totalSec) {
    if ($p.HasExited) { Write-Host "Process exited at $([int]$sw.Elapsed.TotalSeconds)s"; break }
    foreach ($vk in $fwdVKs) {
      $down = ([P]::GetAsyncKeyState($vk) -band 0x8000) -ne 0
      if ($down -and -not $prevDown[$vk]) {
        # Rising edge -> WM_KEYDOWN
        [P]::PostMessageW($hwnd, $WM_KEYDOWN, [IntPtr]$vk, [IntPtr]0) | Out-Null
      } elseif (-not $down -and $prevDown[$vk]) {
        # Falling edge -> WM_KEYUP
        [P]::PostMessageW($hwnd, $WM_KEYUP,   [IntPtr]$vk, [IntPtr]0) | Out-Null
      }
      $prevDown[$vk] = $down
    }
    Start-Sleep -Milliseconds 16   # ~60 Hz poll
  }
  if (-not $p.HasExited) {
    Stop-TraceProcess $p $hwnd ("Interactive timeout at {0}s" -f [int]$sw.Elapsed.TotalSeconds)
  }
} elseif ($PracticeMode) {
  # Practice mode: hold for PracticeHoldSec seconds.
  # Covers: practice UI props, section loop, speed-change audio path.
  Write-Host ""
  Write-Host "=========================================================" -ForegroundColor Cyan
  Write-Host " Practice mode: holding $PracticeHoldSec s" -ForegroundColor Cyan
  $whamStr = if ($Whammy) { " + whammy pulse every ${WhamwyIntervalMs}ms" } else { "" }
  Write-Host (" Autoplay ON; all gems hit${whamStr}.") -ForegroundColor Cyan
  Write-Host " Ctrl+C to stop early — trace flushes on shutdown." -ForegroundColor Cyan
  Write-Host "=========================================================" -ForegroundColor Cyan
  Write-Host ""
  $sw = [System.Diagnostics.Stopwatch]::StartNew()
  $whamState = $false
  $halfMs = [int]($WhamwyIntervalMs / 2)
  while ($sw.Elapsed.TotalSeconds -lt $PracticeHoldSec) {
    if ($p.HasExited) { Write-Host "Process exited at $([int]$sw.Elapsed.TotalSeconds)s"; break }
    if ($Whammy) {
      if ($whamState) {
        [P]::PostMessageW($hwnd, $WM_LBUTTONUP,   [IntPtr]0,            [IntPtr]0) | Out-Null
      } else {
        [P]::PostMessageW($hwnd, $WM_LBUTTONDOWN, [IntPtr]$MK_LBUTTON, [IntPtr]0) | Out-Null
      }
      $whamState = -not $whamState
      Start-Sleep -Milliseconds $halfMs
    } else {
      Start-Sleep -Seconds 5
    }
    $elapsed = [int]$sw.Elapsed.TotalSeconds
    if (-not $Whammy -and ($elapsed % 15) -lt 5 -and $elapsed -gt 5) {
      Write-Host "  ... ${elapsed}s elapsed"
    }
  }
  if (-not $p.HasExited) {
    Stop-TraceProcess $p $hwnd ("Practice timeout reached at {0}s" -f [int]$sw.Elapsed.TotalSeconds)
  }
} elseif ($FullSong) {
  # Full-song mode: hold up to $FullSongMaxSec and stop early if the
  # process exits on its own (results screen -> menu -> process exit).
  # Covers: complete song playback, SP deployment effects (fired every ~6s
  # by autoplay_hook.cpp), end-of-song results screen, star animation,
  # post-song character reactions.
  Write-Host ""
  Write-Host "=========================================================" -ForegroundColor Yellow
  Write-Host " Full-song mode: holding up to $FullSongMaxSec s" -ForegroundColor Yellow
  $whamStr2 = if ($Whammy) { " + whammy every ${WhamwyIntervalMs}ms" } else { "" }
  Write-Host (" Covers: full song + post-song results + SP effects${whamStr2}") -ForegroundColor Yellow
  Write-Host " Ctrl+C to stop early — trace flushes on shutdown." -ForegroundColor Yellow
  Write-Host "=========================================================" -ForegroundColor Yellow
  Write-Host ""
  $sw = [System.Diagnostics.Stopwatch]::StartNew()
  $whamState2 = $false
  $halfMs2 = [int]($WhamwyIntervalMs / 2)
  while ($sw.Elapsed.TotalSeconds -lt $FullSongMaxSec) {
    if ($p.HasExited) { Write-Host "Process exited naturally at $([int]$sw.Elapsed.TotalSeconds)s"; break }
    if ($Whammy) {
      if ($whamState2) {
        [P]::PostMessageW($hwnd, $WM_LBUTTONUP,   [IntPtr]0,            [IntPtr]0) | Out-Null
      } else {
        [P]::PostMessageW($hwnd, $WM_LBUTTONDOWN, [IntPtr]$MK_LBUTTON, [IntPtr]0) | Out-Null
      }
      $whamState2 = -not $whamState2
      Start-Sleep -Milliseconds $halfMs2
    } else {
      Start-Sleep -Seconds 5
      $elapsed = [int]$sw.Elapsed.TotalSeconds
      # Progress tick every 30s
      if (($elapsed % 30) -lt 5 -and $elapsed -gt 5) {
        $remaining = $FullSongMaxSec - $elapsed
        Write-Host "  ... ${elapsed}s elapsed, up to ${remaining}s remaining"
      }
    }
  }
  if (-not $p.HasExited) {
    Stop-TraceProcess $p $hwnd ("Full-song timeout reached; max {0} s elapsed" -f $FullSongMaxSec)
  }
} else {
  Write-Host "Holding $GameplayHoldSec s for gameplay capture..."
  if ($Whammy) {
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $whamState3 = $false
    $halfMs3 = [int]($WhamwyIntervalMs / 2)
    while ($sw.Elapsed.TotalSeconds -lt $GameplayHoldSec) {
      if ($p.HasExited) { break }
      if ($whamState3) {
        [P]::PostMessageW($hwnd, $WM_LBUTTONUP,   [IntPtr]0,            [IntPtr]0) | Out-Null
      } else {
        [P]::PostMessageW($hwnd, $WM_LBUTTONDOWN, [IntPtr]$MK_LBUTTON, [IntPtr]0) | Out-Null
      }
      $whamState3 = -not $whamState3
      Start-Sleep -Milliseconds $halfMs3
    }
  } else {
    Start-Sleep -Seconds $GameplayHoldSec
  }
  if (-not $p.HasExited) {
    Stop-TraceProcess $p $hwnd "gameplay hold elapsed"
  }
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
  # Grab more lines for -Interactive runs so MnK diagnostic entries are visible.
  $tailLines = if ($Interactive) { 50 } else { 15 }
  Get-Content $ll.FullName -Tail $tailLines
}
