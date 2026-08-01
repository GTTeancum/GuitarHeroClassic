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

A documented map of GH2's per-frame gameplay tick: note highway scroll,
hit detection, scoring, song state machine, audio stem sync. Sufficient
that fresh C++ targeting OG Xbox can be written from the documented map,
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

Run, capture, analyze.

Steps:
1. Single end-to-end run with tracer active during gameplay window.
2. Pull the trace jsonl into Python / sqlite for analysis.
3. Identify the per-frame tick function (the loop with consistent
   period in the trace -- likely 60Hz).
4. From the tick, walk the call tree once per frame to identify:
   - Song-state update
   - Note highway scroll math
   - Input-poll → hit-test pathway
   - Score update
   - Audio stem sync (which streams advance when)
5. Name the discovered functions in `harmonix_symbols.h` and
   `recomp_symbols.md` with confidence levels.
6. Write `gameplay_loop.md` capturing the loop's call graph and data
   flow at the level needed to reimplement.

Success criterion: a reader of `gameplay_loop.md` (specifically the
user, who doesn't read the recompile) can describe what the engine
does every frame of a song without needing me to walk through the
recompile source.

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

*Updates here as phases complete or block.*

## Status

Awaiting approval to start Phase 0.
