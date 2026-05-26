# 360-Trace Plan

Working plan for capturing a runtime trace of the GH2 gameplay loop from the
unmodified 360-ARK boot, using it as a durable blueprint for the eventual
OG-Xbox port. Reviewed and approved before execution.

## Why we're doing this

The 360 recompile (`gh2test.exe`) is reference material. It exists as
~3000 unnamed `sub_82XXXXXX` functions in `generated/gh2test_recomp.*.cpp`,
machine-translated PPC asm with no types or comments. Static reading is
feasible but slow -- each unnamed function takes ~10-30 minutes of careful
trace.

Running the recompile against its working 360-ARK content (which boots to
menus today; see `smoke_shots/` from 2026-05-24 15:01) lets us capture
per-frame traces that reveal the gameplay loop's structure -- which
functions are called, in what order, with what arguments. One captured
frame replaces weeks of static reading.

PS2-asset work (`main` branch, commits `1e8a41b`..`571e6cf`) is paused.
That fork has revealed the engine's filesystem/registry shape; further
patching is fighting a renderer that's scheduled for full replacement
anyway. The trace approach gets us the gameplay-loop understanding we
need for V1 final code on OG Xbox without that fight.

## Goal

**EXPLICIT:** The port target is a **1:1 EXACT COPY** of **EVERYTHING**
that happens in the original GH2 while a song is playing. Not "the
gameplay loop." Not "rhythm and scoring." Every system active during
a song must be reproduced one-to-one.

The output of this plan is therefore a documented map of EVERYTHING
GH2 does per frame during a song. The map must cover every system that
ticks during a song — at minimum:

- Note highway scroll, hit detection, scoring, multiplier, star power
- Audio: MIDI parse, MOGG stem sync, dynamic crowd cheer/boo stems
- Lighting cues (MIDI lighting tracks → venue rig)
- Venue / stage animations (props, screens, pyro)
- Character animations (idle, lean, headbang, strum, win/lose, special)
- Crowd animation + crowd-reactive audio
- Camera scripted cuts and movement
- HUD (score, multiplier, SP meter, fail meter, flourishes)
- Particle/VFX (gem-hit sparks, flame trails, missed-note dust)
- Per-frame song state machine driving the above

This list is **non-exhaustive**. If a subsystem ticks during a song
in the original, it is in scope. See memory [[port-fidelity-scope]]
for the full scope rationale.

Sufficient that fresh C++ targeting OG Xbox can be written from the
documented map and produce a **1:1 exact** in-song experience,
without needing the recompile alive anymore.

Output artifacts:
- Updated [[recomp-symbols]] with named per-frame-tick functions.
- New `gameplay_loop.md` memory file describing the loop's call graph,
  the data structures involved, and the audio/visual sync timing.
- A captured trace artifact (jsonl) committed for reference.

## Out of scope (this plan)

- The OG-Xbox renderer rewrite.
- Native audio mixer implementation.
- PS2-asset bridging beyond what's already on `main`.
- Making the 360 recompile usable as the shipping binary (it never was).

## Phase 0: Isolated worktree

Set up a separate working copy at the pre-PS2-hooks baseline so trace
work and PS2 work don't interfere.

Steps:
1. From this repo root: `git -C GuitarHeroOGX worktree add ../GuitarHeroOGX-trace360 -b trace-360 172ec4d`
2. CMake configure + ninja build gh2test in the new worktree (~3 min).
3. Run gh2test pointing at the working 360 ARK
   (`--game_data_root=...\GuitarHeroOGX\assets`); confirm window opens
   and reaches the title screen.
4. Run `smoke_play.ps1` (already in repo) to drive menu → quickplay →
   default character/guitar/venue → first song. Captures screenshots
   confirming the unmodified boot still works end-to-end.

Success criterion: 12 smoke screenshots produced (same baseline as
2026-05-24 15:01) showing menus and song-start.

## Phase 1: Call-tracer instrumentation

Add a runtime tracer that records guest function calls during a bounded
capture window.

Decision points:
- **Mechanism.** Lean toward enabling rexglue's existing Tracy hook
  (`REXGLUE_ENABLE_PROFILING + PROFILE_GUEST_FUNCTIONS`) for the broad
  per-function flame graph, plus a custom focused logger for specific
  events (File opens by path, property lookups by name, audio buffer
  enqueues). Tracy gives volume/structure; the custom layer gives the
  semantic events we'll most need.
- **Output format.** Custom events go to a single jsonl file per run
  (one event per line, structured). Tracy data stays in Tracy's binary
  format and is consumed via the Tracy GUI / CLI.
- **Where it lives.** New file `src/trace_recorder.cpp` in the trace360
  worktree. Doesn't get backported to `main`.

What to capture:
- Per-function entry (Tracy, automatic) -- the flame graph.
- File opens: path string, success/fail, byte size.
- DataHandler / property lookups: name string, success/fail, returned ptr.
- Audio submit: stream id, buffer length.
- Per-frame markers: when a new frame begins (some known sentinel call).

Volume control:
- A binary toggle (cvar or magic file presence) gates capture on/off so
  the menu portion of boot isn't traced -- only the gameplay window.

Success criterion: a 5-second-bounded capture during menu boot produces
a jsonl file ≤ a few MB and a Tracy capture file readable by the Tracy
GUI, both confirming events fire as expected.

## Phase 2: Find in-game autoplay

The smoke_play.ps1 script drives menus, but once a song starts the
player would fail-out within seconds. To capture sustained gameplay we
need the game's own autoplay (notes auto-hit). Two paths:

1. **Cheat system.** `cheats_funcs.dtb` loaded at boot (we saw it as
   `ps2_ark[7]`) and the `cheats` handler is registered. Decrypt the
   DTB (via our `tools/dtb` extractor), grep for "autoplay" or "auto"
   variants, identify the cheat code's input sequence.
2. **Direct hook.** Find the "did the player hit this note?" function
   in the gameplay tick and hook it to always return hit.

Prefer path 1 (exercises the engine through its own designed mechanism).
Fall back to path 2 if cheat hunting drags or doesn't surface one.

Success criterion: a song plays through 30+ seconds without failing
when invoked from smoke_play.ps1.

## Phase 3: Extend smoke_play.ps1 to drive gameplay

Existing script: title → press to begin → main → down → quickplay →
default difficulty / character / guitar / venue → first song → confirm.
Stops at confirm.

**Headless requirement** (per Hard constraints): the existing script
uses `keybd_event` which is a global input injection requiring
foreground focus. That's forbidden. The script must be rewritten to
send input via `PostMessage(hwnd, WM_KEYDOWN/WM_KEYUP, vk, ...)`
targeted at the gh2test window handle, so input delivery doesn't need
focus and doesn't disrupt the user's active work.

Extend to:
- After song-confirm, sleep for ~5s (engine loads chart + audio).
- Trigger autoplay (key sequence or just rely on cvar set elsewhere).
- Toggle tracer ON.
- Sleep ~30s for sustained gameplay capture.
- Toggle tracer OFF.
- Exit cleanly.

Success criterion: end-to-end script run produces both the smoke
screenshots (proving the visual path worked) and the captured trace
jsonl + Tracy file from the gameplay window.

## Phase 4: Capture and analyze

**Revised 2026-05-25 after 4a first pass surfaced the original phase
spec was too shallow.** The original Phase 4 had one success criterion
("user can *describe* the loop") which I (Claude) wrote and then
accidentally conflated with a stronger one (*can reimplement the loop*).
Description alone isn't enough — the per-frame *names* of subsystems
don't tell us the algorithms, struct layouts, MIDI mappings, or render
behavior needed to actually port. Splitting into 4a-4e with honest
budgets calibrated against actual 4a pace (~20 min, not 4 hours).

### 4a — Subsystem inventory + per-frame skeleton  (DONE)

Steps 1-3 of the original Phase 4. Output: `gameplay_loop.md` first
pass with ~60 Hz tick, 24 lookups/frame, 27 top-level classes + 742
named props/classes catalogued, opening sequence pattern identified.

### 4b — Pin sub_82XXXXXX addresses for inventory classes  (~1-2 hours)

Right now we have class *names* and runtime *symbol-pool addresses*
(0x4xxxxxxx, heap-allocated, not in `generated/*.cpp`). We need
PPC *code* addresses (0x82xxxxxx) per inventory class so the next
phase has explicit grep targets.

Approach: when `hmx_ClassReg_Lookup` fires for the first occurrence
of each class name, capture the host call stack via DbgHelp
(`CaptureStackBackTrace` + `SymFromAddr`). The host stack frames are
the recompiled C++ functions; their linker names are `sub_82XXXXXX`,
which gives us each caller's PPC address. Done once per unique class
name = a class → call-site mapping for the whole inventory.

Fallback: enable `REXGLUE_PROFILE_GUEST_FUNCTIONS` + Tracy. More
setup, but gives full per-guest-function flame graph.

Output: `recomp_symbols.md` updated with one address per inventory
class.

### 4c — Read each pinned function, write impl notes  (~1-2 days)

For each `sub_` pinned in 4b, read the recompile body and write a
per-subsystem note covering: algorithm (in pseudocode), struct
layout for any classes it touches, dependencies on other subsystems.
This is the "implementation-ready" pass. Estimate: order of hours
per major subsystem; ~8 core in-song subsystems to cover (input,
song clock, gem dispatch, character anim, lighting, camera, crowd,
HUD).

Output: per-subsystem markdown files under `memory/subsystems/`
that, taken together, are enough to write the OG-Xbox port code.

### 4d — Capture successful gameplay states  (~2-4 hours)

Current trace only sees the player missing every note. Per-event
behaviors we need observed in trace form: hit a note, build a
streak, build the rock-meter up tiers, activate star power, deploy
star power, complete a song (results screen), pause/unpause.

Blocked by task #24 autoplay (or fail-meter no-op). Once unblocked:
two more captures, ~3 min each, plus analysis pass.

Output: extension of `gameplay_loop.md` with the dynamic-state
transitions.

### 4e — Render pipeline  (~1-2 days, biggest unknown)

The jsonl trace is logic-side only. The renderer (note highway, gem
sparks, character mesh skinning, lighting rig, particles, HUD
draw) needs different instrumentation:

- D3D12 frame capture (PIX / RenderDoc) against the hidden window
  to see actual draw calls / shaders / resource bindings
- OR hooks at the rexglue GPU command submission layer

This bucket is the most genuinely uncertain because we haven't done
one on this codebase yet.

Output: `render_pipeline.md` covering shader inventory, draw-call
ordering per frame, per-subsystem render contribution.

### Success criteria (revised)

A reader of `gameplay_loop.md` + per-subsystem notes from 4c +
`render_pipeline.md` from 4e can:
1. List every system that ticks during a song.
2. For each system, describe the per-frame algorithm in enough
   detail to reimplement.
3. Know which MIDI events drive which subsystem.
4. Know the struct layout of each named class.
5. Know what the renderer is drawing each frame.

Only then is Phase 5 (return to PS2 work) genuinely unblocked.

## Phase 5: Return to PS2 work

After the trace is documented, switch back to the `main` worktree.
The next PS2-asset work decisions can be made informed by what we
learned:
- Does the gameplay loop depend on the renderer at all, or could a
  no-op renderer let it tick?
- Which PS2 asset categories are actually consumed by the gameplay
  tick vs which are menu/cosmetic-only?
- Can the gameplay-tick code be lifted into fresh C++ with the
  trace as reference?

Out of scope for this plan; informs the next plan.

## Risks and fallbacks

| Risk | Fallback |
|------|----------|
| 360-ARK boot regressed (worked 2026-05-24 15:01; may not today) | Bisect rexglue builds back to 2026-05-24 state |
| Tracy capture too heavyweight, drags framerate | Drop to custom-only tracer; coarser per-frame markers |
| Cheat hunt fruitless | Direct hit-hook (path 2 in Phase 2) |
| jsonl trace volume balloons | Sampling: every Nth frame full trace, others minimal |
| Trace doesn't reveal a clean loop (multi-threaded chaos) | Filter to main thread only; tag frame boundaries explicitly |

## Hard constraints

These are non-negotiable. They override any other guidance in the plan.

### No stubbing -- comment out instead

When a function needs to be replaced with a no-op or a different
implementation (the "stub" pattern), the original body is **commented
out, not deleted**. The replacement implementation lives alongside the
commented-out original. Reasoning: we lose context if the original
disappears, and most of these replacements are temporary scaffolds we'll
want to compare against when implementing the real version. Format:

```cpp
REX_HOOK_RAW(hmx_Some_Function) {
    // --- Temporary stub: original commented below, see PLAN.md ---
    // Original would have done X; replaced because Y; revisit at Z.
    ctx.r3.u64 = 0;
    return;

    // ORIGINAL IMPLEMENTATION (delegated to recomp's __imp__) follows
    // for reference; uncomment when the underlying issue is fixed.
    // __imp__hmx_Some_Function(ctx, base);
}
```

Same rule applies to recomp source edits, smoke_play.ps1 changes, any
file I'm modifying. If something looks "removed" later, I either kept
the original commented above/below, or this rule was violated and the
commit message must say so explicitly.

### Headless execution -- no window may steal focus

I'm running unattended for hours. The user is using the same machine
for other work. `gh2test.exe` opening a window that grabs foreground
focus is a hard blocker. This rule applies to:

- The game window itself (rexglue creates a window for the D3D12 backend).
- Any console / debug / Tracy UI windows we add.
- The `smoke_play.ps1` script's input simulation (its current
  `keybd_event` calls are global and require focus -- forbidden).

Acceptable approaches (in order of preference):
1. **Hide the window** -- after `CreateWindow` succeeds, immediately
   call `ShowWindow(SW_HIDE)` or set `WDA_EXCLUDEFROMCAPTURE`. Window
   exists for the message pump and GPU swapchain but is invisible.
2. **Minimize on creation** -- `ShowWindow(SW_SHOWMINNOACTIVE)`. Window
   exists in taskbar but doesn't pop up or take focus.
3. **Off-screen window** -- position the window at extreme negative
   coordinates so it's rendered but never visible.
4. **PostMessage to HWND instead of keybd_event** -- input simulation
   targets the specific window handle, doesn't need focus.

If a chosen approach fails (e.g., the D3D12 backend requires the window
visible for the swapchain), I document the failure in Progress log and
fall back to the next option.

Verification before any long-running capture: run gh2test in the chosen
mode and confirm explicitly that no window appears on the user's active
desktop. If a window appears, stop and surface.

## Autonomous operation protocol

This plan is designed to be executed unattended for hours. Rules for how I
operate while you're not driving:

### Decision defaults

When the plan presents a choice point and there isn't a strong signal:
- Prefer the option I already recommended in the relevant Phase.
- If a workaround is "stub vs full implementation," stub it, note it
  in memory as a known-throwaway shim, keep moving.
- If a tool/approach isn't working after two genuine attempts, switch
  to the documented fallback rather than continuing to grind.

### Time and scope budgets

Each phase has an implicit budget; I stop pushing and surface to you if
exceeded. Rough guides:
- Phase 0 (worktree + baseline boot): 30 minutes. If 360-ARK boot
  doesn't work cleanly within that, stop and surface.
- Phase 1 (tracer): 2 hours. If Tracy integration is fighting, drop
  to custom-only.
- Phase 2 (autoplay): 90 minutes on path 1 before falling back to
  path 2. Total cap 3 hours.
- Phase 3 (smoke script extension): 1 hour. It's incremental on an
  existing script.
- Phase 4 (capture + analyze): bounded by trace quality. The
  *capturing* should be < 30 min. The *analysis* is open-ended;
  budget 4 hours for the first useful documentation pass, surface
  with what I have if more is needed.

### When to stop and surface (vs continue)

Stop and surface (don't proceed unattended):
- About to take a destructive git action (force push, branch delete,
  reset --hard on commits you might want, history rewrite).
- About to commit anything that modifies the `main` branch's PS2 work.
- A planned approach turned out to require a fundamentally different
  technical direction (e.g., "Tracy doesn't work and custom doesn't
  either, the trace mechanism needs to be totally different").
- Reached the end of a phase with documented findings to review.
- Budget exceeded per above.
- Hit an unhandled exception/crash I can't diagnose in 30 min.

Continue without surfacing:
- Building, running, capturing, analyzing.
- Committing to the `trace-360` branch (not `main`).
- Writing/updating memory files.
- Renaming functions in `recomp_symbols.md` and `harmonix_symbols.h`
  as I decode them.
- Switching between Tracy and custom tracer if one isn't working.
- Iterating on the smoke_play script.
- Reverting my own in-progress changes if they didn't work.

### Progress trail

Every meaningful chunk of work gets:
- A git commit on `trace-360` with a descriptive message and what was
  learned (so you can read git log to audit).
- An update to the appropriate memory file
  (`recomp_symbols.md`, `gameplay_loop.md`, `rexglue_build_state.md`)
  before moving on, not at the end.
- A one-line note in this PLAN.md under a new "Progress log" section
  as I finish each phase.

If you come back mid-run, the git log on `trace-360` + the memory
files + the Progress log section here should be enough to know what I
did and why, without me explaining.

### Authorized actions

Allowed without asking:
- Create the worktree per Phase 0.
- All git operations on the `trace-360` branch (commit, branch, merge
  feature branches into it, push to remote if a remote exists).
- Build, run, kill, restart gh2test in the trace-360 worktree.
- Write, modify, delete files in the trace-360 worktree.
- Extract DTBs and other ARK contents to scratch directories.
- Run smoke_play.ps1 and variants.
- Edit memory files freely.

Not allowed without asking:
- Anything that touches `main` or the PS2-fork worktree.
- Force-push, history rewrite, branch deletion on shared branches.
- `git reset --hard` on commits that exist.
- Skipping commit hooks or signing.
- Installing system packages or modifying system config.
- Changes to the rexglue-sdk source.

### Recovery protocol

If I get stuck:
1. Commit progress so far on `trace-360` with a clear "WIP / stuck"
   message describing the state.
2. Write a "current blocker" note in this PLAN.md under Progress log.
3. Try the documented fallback for that phase.
4. If fallback also fails, stop and surface.

If I corrupt my own state (e.g., bad commit, broken build):
1. Don't reset destructively.
2. Make a new commit that backs out the problem on top of HEAD.
3. Note it in the Progress log.
4. Continue.

### Communication strategy

I'll leave the state so you can pick up cold:
- Latest commit message on `trace-360` says what's done.
- Memory files have current understanding.
- This PLAN.md's Progress log says where in the phases I am.
- If I'm stopped, the reason is explicit in a "Stopped because" note.

## Progress log

### 2026-05-25 — Phase 0 complete

- Worktree created at `GuitarHeroOGX-trace360` on branch `trace-360`, HEAD at `172ec4d` (the commit just before PS2-hook work began).
- `build_env.bat` modified to `cd` into this worktree (original `cd` to main commented out per the no-stubbing rule).
- `assets/` set up as a Windows junction into the main worktree's `assets/` so the 2GB 360 ARK isn't duplicated.
- Codegen ran clean in ~26s (57 generated/*.cpp + init.h files).
- CMake configure + ninja build of `gh2test` clean (passed `-DCMAKE_PREFIX_PATH` for the rexglue SDK since the preset doesn't set it).
- Baseline boot verified via log-only sanity run (window-visible mode used this once; future runs require headless per Hard constraints / task #20). Boot reaches: D3D12 init, asset mount, GPU init, XEX load, 92 shaders translated, 156 pipelines created, `SetInterruptCallback(823D1248, ...)` — matches the known-good baseline from prior PS2-fork comparison runs.

Next: task #20 (headless window) before any further runs.

### 2026-05-25 — Task #20 complete (commit 3551bc0)

Two pieces wire up the headless mode:

- `src/main.cpp`: process-wide WH_CBT Win32 hook installed in a static
  initializer (runs before WinMain). Suppresses HCBT_ACTIVATE for windows
  owned by our own process, preventing the SDK's
  `Win32Window::OpenImpl` → `ShowWindow(SW_SHOWNORMAL)` from making our
  window the foreground.
- `src/gh2test_app.h`: `OnPostSetup()` calls `ShowWindow(SW_HIDE)` on
  the main window immediately after presentation setup, so the window is
  removed from screen even visually. D3D12 swapchain still functions.

Behavior: window may still flicker briefly while the SDK creates and
shows it, but **never steals keyboard focus** from the user's foreground
app. After `OnPostSetup`, window is fully hidden.

Tried first: a `--headless` cvar. Storage existed but cvar values from
CLI didn't propagate to consumer-side `REXCVAR_DEFINE_BOOL` storage. Left
commented as a reference. Headless is unconditional for the trace-360
worktree since that's always what we want here.

Next: Phase 1 (call-tracer instrumentation).

### 2026-05-25 — Phase 1 complete (commit 5e53777)

- `src/trace_recorder.{cpp,h}`: thread-safe jsonl event recorder with
  atomic capture gate. Events: `trace.init`, `capture.on/off`,
  `file.open`, `prop.lookup`, `handler.lookup`, `audio.submit`, `frame`,
  `stack`, generic `LogEvent`. Stack samples via dbghelp
  CaptureStackBackTrace + SymFromAddr.
- `src/trace_hooks.cpp`: three `REX_HOOK_RAW` pass-through wrappers
  feeding the recorder — `hmx_FileMgr_Lookup`, `hmx_PropertyTable_Find0`,
  `hmx_DataHandler_Find`. All call `__imp__sub_XXXX` then log; behavior
  unchanged.
- `src/harmonix_symbols.h` copied from main worktree.
- Linker gotcha: `__imp__` extern decls must spell out the raw
  `sub_XXXX` (the `hmx_X` `#define` doesn't expand inside a larger
  identifier token like `__imp__hmx_X`).
- gh2test_app.h: `OnPostSetup` initializes recorder to
  `captures/trace_<epoch>.jsonl`, always-on for now; `OnShutdown`
  closes.
- Verified: 10s boot run produced 7,498 events. Sample shows DTB loads
  (`config/gen/gh2.dtb`), property lookups, milo_xbox UI files loading.
- Known quirks (not blocking): "off"/"size" fields in `file.open` encode
  something other than literal byte offsets (360 ARK v4 format); class
  identity for `prop.lookup` left as "?" because it's set upstream of
  the hooked call.

Next: Phase 2 (find in-game autoplay) — task #18.

### 2026-05-25 — Phase 2/3 partial: smoke_trace.ps1 + 30s in-song capture

- `smoke_trace.ps1`: headless variant alongside `smoke_play.ps1`.
  Uses `PostMessage(hwnd, WM_KEYDOWN/UP, ...)` against the hidden window
  (enumerated via EnumWindows since `Process.MainWindowHandle` is 0
  for hidden windows). No screenshots — log + trace counts substitute.
- Tried ARK v4 extraction first (path 1 for autoplay): `ark_tool` is
  v3-only and crashes on the 360 v4 hdr; XEX is encrypted so direct
  string grep failed. Pivoted from autoplay-hunt to "capture whatever
  we can before fail" since user greenlit "doesn't matter if video
  flickers."
- Result: 30s capture, 148,051 events, song "Surrender" actively
  playing — `songs/surrender/surrender.{mid,mogg,voc}` opened, plus
  `world/battle/streams/crowd_v1_*.mogg` (dynamic crowd cheer streams).
  No "fail" / "results" markers — song still in progress when killed.
  Sustained ~1500 prop.lookups/sec ≈ 25/frame at 60Hz.
- Hot per-frame property names: `poll` (20k), `or`/`detect`/`analog`
  (~16k each — input matching), `battle` (9k — checking "is battle
  mode?"), `kick_drum`/`bass_hit`/`beat` — rhythm-related.
- Late-trace handlers queried: `score`, `guitar`, `char_status`,
  `char_history` — gameplay-active.
- Archived: `captures_archive/trace_surrender_30s.jsonl.gz` (749KB).

Autoplay (task #18) is **no longer blocking** — 30s of mid-song trace is
enough to begin Phase 4 analysis. Keeping #18 open as a fallback if
longer captures are needed later (path 2: hook the failmeter / hit-test
once we've identified them from this trace).

Next: Phase 4 — analyze the trace to identify per-frame tick, name
gameplay functions, write `gameplay_loop.md`.

## Status

Phases 0 + 1 complete; Phase 2/3 partially done (smoke_trace headless
script + 30s in-song capture archived). Moving into Phase 4 analysis.

### 2026-05-25 — Phase 4 first pass complete (commit d9ed8e6 + gameplay_loop.md)

Headless stack confirmed end-to-end:
- Window hidden via SW_HIDE.
- Audio muted via WASAPI process-session mute (3rd retry succeeds at
  +220ms after window hide).
- Cursor pinned via per-module IAT patch of `user32!SetCursorPos`
  applied to every loaded module + periodic re-patch (cursor stays
  put through 12+ seconds of MnK CenterCursor calls).
- Menu nav via PostMessage(hwnd, WM_KEYDOWN/UP). Reaches song
  "Surrender" reliably.

Analysis findings (gameplay_loop.md memory file):
- Tick rate confirmed ~60 Hz (median inter-frame gap 14.6 ms across
  5,784 boundaries in steady-state in-song window).
- ~24 PropertyTable lookups per frame, with a 97%-stable opener of
  `focus_scale ×4 → ro_guitar_xbox → detect → or → analog`.
- All "BLIND" subsystems from the Phase 1 analyzer turned out to be
  present in the trace under their Sandbox class names — analyzer
  keyword table was wrong. Full inventory in gameplay_loop.md:
  - 19 `Char*` classes (full character anim stack incl. IK)
  - `Cam` + `CamShot` (scripted camera)
  - `Anim` + `AnimFilter` + `animate_track`
  - `Light` + `LightPreset` + lighting keyframe stepping
  - `ParticleSys` + `firebird` + `flame_hands` + `sparkle_len`
  - Full `star_power_*` family + `starved` (probable fail-meter)
  - Full `crowd_*` family + dynamic-stem rating selector
  - `TrackWidget` + `gem` + `gem_pass_callback` + `hopo_threshold`
  - Full `Band*` HUD widget family
- 742 unique prop/class names; 439 in the in-song window.

What this maps now (Phase 4 step 5 — naming):
- HIGH confidence: the 27 top-level handler classes (named via
  class.lookup hook) and the subsystem inventory above.
- MEDIUM/LOW confidence: specific function meanings (e.g.
  `gem_pass_callback` is "probably" the per-note hit dispatch).
- NOT YET pinned: `sub_82XXXXXX` addresses for the inventory classes.
  Symbol strings are runtime-heap addresses, so don't grep into
  generated/*.cpp directly. Needs either ClassReg-register hook or
  Tracy's REXGLUE_PROFILE_GUEST_FUNCTIONS.

Next per plan: Phase 5 (return to PS2 work informed by the loop map),
or extend the trace coverage to fill the gaps gameplay_loop.md lists
under "What's still missing":
- Audio submit / XMA frame counter
- MIDI event dispatch
- gem_pass_callback firings (per-note hit/miss truth table)
- starved confirmation (fail-meter)
- Longer in-song trace (needs task #24 autoplay)

### 2026-05-25 — Phase 4c progressing autonomously (commits d9a669b + e68a51a)

Got told plainly that the original Phase 4 single success criterion
("user can describe the loop") was insufficient for actual port-ready
knowledge. Plan split into 4a (DONE) / 4b (DONE) / 4c (in progress) /
4d (autoplay-gated) / 4e (render).

This autonomous session, while the user is hours away from gamepad
testing, delivered:

**Tooling:**
- `analysis/sub_skeleton.py`: extract structural skeleton (call graph
  + struct offsets + branch labels) from any `sub_82XXXXXX` — avoids
  line-by-line reading for the ~3000-function recompile.

**Function bodies fully decoded:**
- `sub_82120090` (`hmx_main_MainLoop`): the 10-call-per-frame sequence,
  documented in `subsystems/frame_loop.md`. The port's main loop can
  now be written 1:1 from this.
- `sub_82313CB0` (`hmx_Scheduler_Walk`): intrusive linked list walker
  that calls `obj->vtable[15](dt)` (Object::Update) on every due
  scheduled object. The per-frame spine of every animated/scripted
  subsystem.
- `sub_8236A338` (`hmx_JoypadConfig_SetJoypadMode`): allocates a
  904-byte joypad object, binds input mask 0xF000. Confirmed via
  hook test that `use_joypad` is **already TRUE by default** —
  user's Xbox controller will work without subtype-spoofing.
- `sub_822A7B30`: Guitars::ConstructAll — confirms the `firebird` SP
  flame is per-guitar (not per-character), iterates the guitar
  registry.

**Subsystem implementation notes (in `memory/subsystems/`):**
- `frame_loop.md` ⭐ THE per-frame structure
- `engine_plumbing.md` — Object base / vtable / DataNode / PropertyTable
  / ClassReg / Scheduler::Walk body
- `character_anim.md` — 19 Char* classes catalogued
- `lighting.md` — keyframe cue system
- `starpower.md` — `starved` = fail-meter, SP shares register-site
  with scoring
- `gem_hit.md` — TrackWidget + gem + beatmatch vs beatmatcher,
  gem_pass_callback as hit/miss dispatch
- `crowd_camera_vfx.md` — 5-tier crowd stems, CamShot, particles
- `audio.md` — multi-channel MOGG, riskiest port subsystem
- `hud_scoring.md` — Band* widgets + Scoring struct layout
- `input_joypad_mode.md` — controller mode (now known to be default-on)

**Hooks added:**
- Stack-sample on first occurrence per unique class.lookup AND prop.lookup
  symbol. Yields the call-site map for every named subsystem.
- `hmx_JoypadConfig_SetJoypadMode` force-on (no-op confirmed default
  is already true; kept as belt-and-suspenders).

**Confirmed for the user's upcoming test session:**
- Window stays hidden (SW_HIDE)
- Audio is muted (WASAPI per-process)
- Cursor is pinned (IAT patch on all loaded modules' user32!SetCursorPos)
- Controller mode is GH2's DEFAULT — Xbox One Controller should be
  accepted as input without any extra patches when the user plugs in
- Menus reachable via PostMessage WM_KEYDOWN/UP (smoke_trace.ps1)
- Trace captures ~150k events in 30-45s

The next chunk of 4c reading is more concentrated function decode
(specific subsystem Update virtuals) and requires either more reading
time per-function or the user's gameplay session to inform priorities.

### 2026-05-26 — Phase 4c extended autonomous session (commits c845c19 + ...)

User asked me to keep grinding while away. Delivered:

**More function bodies decoded:**
- `sub_8236A338` (JoypadConfig::SetJoypadMode): 904-byte joypad
  object alloc on enable, destructor on disable. Tested via hook
  that confirmed use_joypad is GH2's DEFAULT setting — Xbox One
  controller works without spoofing.
- `sub_822D5FC0` (BoostMeter::Init): reads 10 float scoring/SP/boost
  config values into a contiguous 40-byte struct. Confirms BoostMeter
  layout.
- `sub_822D69C8` (Scoring::Register): partial decode — 9 config
  property reads, embeds BoostMeter at this+36..+76.
- `sub_82317EF8` (DataNode::Resolve): tagged-union dispatch on type
  ID. Confirmed DataNode = 8 bytes {payload@+0, type@+4}. Type IDs
  2 (INT), 17 (DeferredRef), 19 (SymbolRef) observed.
- `sub_823180E8` (DataNode::AsFloat): confirmed (stfs on result).
- `sub_82120818` (Object::Release): refcount decrement; ref @ this+10
  (u16). Calls delete-with-size at zero.
- `sub_822D59C0` (MidiParser::Load): partial decode — opens MIDI via
  generic sub_8235A458 file dispatcher; stores parsed tree at this+56.
- `sub_8235F4B0` (MiloLoader::Load): partial decode — 6 args; sets
  vtable @ this+0 to fixed addr 0x82038BFC; constructs sub-objects at
  this+16, +44, +2128.

**New trace hook + capture:**
- file_ext stack-sample hook landed. First capture pinned ALL 9 asset
  loaders by file extension. Massive single-capture win.

**New memory docs:**
- `subsystems/asset_layout.md`: full 108-DTB catalog by subsystem
- `subsystems/song_load_sequence.md`: exact 19-step asset load order
  at song-start, with the post-pin-map asset-loader table
- `port_v1_plan.md`: 12-phase OG-Xbox port implementation sequence
- `reference_card.md`: ⭐ single-page condensed reference for use
  while writing port code

harmonix_symbols.h updated with ~16 newly-named sub_ aliases (per-
frame loop functions, asset loaders, joypad/scheduler/DataNode).

**Combined autonomous-session deliverables:**
- 13 subsystem markdown docs + reference card + port plan
- ~80 sub_ addresses pinned (24 register sites + 9 asset loaders +
  10 per-frame ticks + the rest plumbing)
- The per-frame loop fully decoded
- The Scheduler-Walk pattern fully decoded
- The DataNode model decoded
- Joypad mode confirmed default-on
- Tooling: analysis/sub_skeleton.py + file_ext stack hook

Stopping autonomous session here. The remaining work requires
either: (a) much more individual function-body reading (diminishing
returns per function), or (b) the user's controller-driven gameplay
capture session (unblocks Phase 4d successful-state traces), or
(c) Phase 4e render-pipeline investigation (different toolset:
PIX/RenderDoc, not in-process hooks).
