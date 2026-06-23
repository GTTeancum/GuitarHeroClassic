# PCSX2 Trace Resume Instructions

Read this file first when resuming the venue/band goal from the trace folder.
If resuming from the workspace root, read `CODEX_RESUME_FIRST.md` first, then
return here.

## Current Gate

The active goal was trace-first, and the 2026-06-14 gate is now
implementation-ready:

- Native character animation, venue, camera, and lighting code may proceed
  from the accepted PS2 evidence documented here and in
  `PS2_ANIMATION_PIPELINE_MAP.md` / `CHARACTER_DEFORM_FORMAT.md`.
- Do not run native visual proof as a substitute for missing PS2 evidence.
- Reopen PCSX2 tracing only for a specific native validation mismatch or a
  deliberately new route/character.
- Use one runtime tracing source: GH2 PS2 under PCSX2. For arms/hands and
  per-character coverage, prefer trace-only GH2DXu direct-autoplay builds
  because they boot straight into successful-note gameplay and remove
  menu/input ambiguity. Generated trace ISOs and staged
  `GH2DXu_PS2_trace_*` combined-disc folders are temporary; keep only the one
  current build during a trace, then delete both the ISO and staged folder
  after the JSON/screenshot/log are captured. Rebuild from stock disc data plus
  the current GH2DXu `out\ps2` files when older variants are needed again.
  When using bundled ImgBurn headlessly, keep
  `dependencies/windows/imgburn.ini` `ISOBUILD_DontPromptRootContent=1`; if it
  is `0`, ImgBurn can hang behind a root-folder prompt or build an image with
  the staging folder itself at disc root, which PCSX2 rejects before boot.
  Keep the stock ISO path available for
  retail comparison:
  `C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso`.
- Use Rexglue/community object metadata only as the compass for names,
  expected class behavior, and interpretation.
- 2026-06-19 hand/fret correction: GHDX autoplay probes
  `gh2dxu_fret_ik_vtable_trace_20260619.json`,
  `gh2dxu_fret_ik_candidate_probe_20260619.json`, and
  `gh2dxu_fret_ik_pointer_follow_probe_20260619.json` prove the live
  `fret.ik` object (`0x00e67d20`) updates through `0x0017bbd0` but only
  changes a near-1.0 scalar at `+0x44`; the moving per-note hand target is in
  the MIDI-selected `finger_*` clip outputs, especially `bone_fret_hand.pos`.
  Do not restore a native lane-to-`spot_neck_fretNN.mesh` transform guess unless
  a new PS2 trace proves the exact selector.

The complete trace map must cover:

- song load into venue performance
- controller dispatch
- clip selection and clip application
- blend and weight handling
- bone transform evaluation
- twist and IK helpers
- hair
- eyes/look-at
- prop attachment
- performer placement
- venue camera properties and camera shot flow
- venue animation
- venue lighting

2026-06-14 trace-gate status: implementation-ready for first native pass.
Do not keep tracing by default. Move to native implementation only from the
accepted PS2 evidence below, and reopen PCSX2 tracing only for a specific
native validation mismatch or a deliberately new route/character.

Closed-enough implementation rules:

- Character arms/hands: use the traced hand driver -> scheduler/blend -> clip
  output -> IK -> foretwist/uppertwist -> Trans dirty/world graph. No
  per-character twist-offset shortcuts.
- Hair/eyes: use per-character controller/mesh graph evidence. GH2-style
  characters may expose `.hair` and `*.lookat` controllers; GH1-style and
  some performer variants use mesh/Trans rows instead. Do not force one
  controller shape across all characters.
- Female singer: main driver, upper twist, and `dreads.hair` are live; static
  inventory has no `CharForeTwist`, `CharLookAt`, or `CharEyes`, so absence of
  those controllers is format-backed.
- Props: normal active-song attachment is Trans/clip-output centered. Preserve
  named prop/constraint rows and dirty propagation; reopen prop tracing only
  for setup/event/alternate singer states.
- Camera: GH1 and GH2/GH80s evidence backs script dispatch, CamShot eval/apply,
  camera row update, submit/handoff, and result/quantize helpers. Keep scalar
  result/handle fields conservatively named until native camera validation.
- Venue animation and lighting: preserve script-dispatched view/anim/light
  rows, common view tick / transform-copy helpers, and lighting branch/root
  structure. Do not collapse lighting to constants just because some command
  branches were zero-hit in one state.
- Performer placement: source-block movement and static apply branch are known,
  but the concrete `lose_teleport` / `teleport` / waypoint branch remains
  route-gated. Treat it as a known reopen trigger, not a reason to keep
  repeating the same fail-transition trace.

GH1 PS2 has now been used as the active cross-title comparison source for the
core character, camera, venue, and lighting paths. GH80s PAL is closed for
this trace phase by user decision; do not resume GH80s breadth work unless
implementation evidence later points back to it. Future cross-title sweeps, if
needed, should include song selections that put female singers in active venue
playback, so singer animation/controller coverage is not inferred only from
male-singer GH2 traces.

GH2 female singer is now active-song proven for main driver, upper-twist, and
hair via `crazyonyou`, `ftk`, and `tattooedloveboys`. A focused visible-singer
look-at argument sample resolved active look-at rows through alterna1, not the
female source, and the static female-singer object inventory has no
`CharForeTwist`, `CharLookAt`, or `CharEyes`. Treat that as format-backed
absence for the female singer, not as permission to infer male-singer controller
behavior onto her.

Current character closure:

1. GH2 YYZ/`metal_keyboard` keyboardist coverage is accepted and documented.
2. GH2 female-singer-specific closure is accepted and documented; foretwist and
   look-at are absent by the static female-singer format, not unresolved.
3. The final GH2DXu `goth3` cloth/secondary-motion pass is deferred by user
   direction. Do not block GH1 on goth3; revisit it only when the GH80s/GH2DXu
   outfit context is intentionally in scope.
4. GH1 and GH80s PAL are already accepted as sufficient for this audit pass.

- Arms/hands are no longer a raw tracing blocker for the steady active-song
  path. The accepted traces prove the hand drivers, scheduler/blend rows, clip
  outputs, IK, foretwist, uppertwist, visible arm/hand output rows, and Trans
  dirty/world bridge. Native implementation must preserve that full graph and
  cadence; do not replace it with per-character twist offsets or a single
  guitarist-only formula. Reopen arm tracing only if a native visual mismatch
  points to a specific missing branch.
- Hair and eyes have enough evidence for the first native pass when the loader
  preserves the real controller/mesh graph. Glam1, rock2, deathmetal1, and
  metal_bass concerns are covered by accepted GH2/GH2DXu traces; GH1 hair,
  face, and eye behavior is covered through mesh/Trans rows. Reopen tracing
  only for goth3 cloth/secondary motion when that outfit/source is intentionally
  back in scope, or for a concrete native mismatch.
- Do not let legs, placement, venue, camera, or lighting displace that
  character priority unless the user explicitly changes it.
- `deathmetal3` is currently Xbox-only in the GH2DXu PS2 tree and is not listed
  by `char_objects_ps2.dta`. Do not count it as a PS2 trace gap unless a real
  PS2 runtime source is first proven.
- GH1 autoplay is currently rejected, not solved. Active-state probes located
  live `player_matcher` object base `0x0149e5b0` and static
  `set_auto_play` function `0x00109e10`, but the active-song state still failed
  at 25% after:
  `gh1_autoplay_candidate_poke_20260612.json`,
  `gh1_autoplay_matcher_poke_20260612.json`,
  `gh1_autoplay_setter_call_20260612.json`,
  `gh1_autoplay_setter_call_mid_20260612.json`, and
  `gh1_autoplay_setter_call_receiver8_20260612.json`. The `_early_` setter
  call patched during black state-load and froze/never reached valid gameplay.
  Do not use those as successful-note evidence; either find a pre-song data
  route or trace a different hit/fail/autoplay path.
- Current GH1 arms/hands checkpoint: `gh1_output_workblock_trace_20260612.json`,
  `gh1_output_workblock_arrays_sample_20260612.json`,
  `gh1_output_target_objects_sample_20260612.json`, and
  `gh1_output_helper_function_snippets_20260612.json` establish the downstream
  output bridge below the clip/span helpers. The order is
  `bone.servo 0x00182730 -> driver/current/clip eval/apply -> 0x001896f8`,
  then role-sized helper passes through `0x0024a290`, `0x001da730`,
  `0x0024ae78`, and math helper `0x0024cb70`. `0x001da730` copies a 16-byte
  local row into `*(target+0x10)+0x50` and sets `*(target+0x10)+0xa0 = 1`.
  Do not retrace this same cut unless a new role/song requires coverage; the
  `gh1_blend_child_fpu_trace_20260612.json` then closes the first
  blend/weight scalar pass by capturing `f12/f13/f20`: eval helpers carry
  small delta-like weights, apply helpers carry blend weights, split/quat
  helpers carry channel fractions plus span range/duration scalars, and
  `0x0017ae98` receives `pi` in `f20` for offset-driven quat mix calls.
  `gh1_source_selector_trace_20260612.json` then closes the first
  source/current selector pass: `0x0018d860`, `0x0018d780`, and `0x0018d978`
  cycle guitarist0/singer/bass/drummer roots; `0x0018a4b0` updates current
  rows; `current+0x24` resolves to named clip instances including
  `alterna_stand_bad`, `finger_open`, `strum_open`, `female_singer_idle`, and
  `bassist_active_medium`; rare `0x0018d9f0` dispatches role roots against
  script environments rather than acting as the per-frame hand writer. Do not
  retrace this same cut unless a new role/song requires coverage; the next GH1
  character work should broaden role/song coverage and move to GH1 hair/eyes.

## Files To Read Next

Read these before launching PCSX2 or changing any code:

1. `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_trace_checkpoint_20260607.md`
2. `GuitarHeroOGX/engine/src/game/GAMEPLAY_RE_NOTES.md`
3. `GuitarHeroOGX-trace360/analysis/ps2_trace/PS2_ANIMATION_PIPELINE_MAP.md`
4. `GuitarHeroOGX-trace360/analysis/ps2_trace/CHARACTER_DEFORM_FORMAT.md`
5. `GuitarHeroOGX-trace360/analysis/ps2_trace/tools/trace_pcsx2_animation_vtables.py`
6. The specific trace helper for the next task, such as:
   - `tools/sample_pcsx2_object_words.py`
   - `tools/sample_pcsx2_chardriver_chain.py`
   - `tools/sample_pcsx2_worlddir_state.py`
   - `tools/sample_pcsx2_world_symbols.py`
   - `tools/sample_pcsx2_shot_graph.py`
   - `tools/trace_pcsx2_controller_targets.py`

## Do Not Repeat These Mistakes

- Do not launch the raw `SLUS_214.47` executable, extracted disc folder, or
  `SYSTEM.CNF` for active runtime traces. Those launch contexts produced
  `Unknown Game`, indexed-state lookup failure, startup errors, or Retry-only
  captures.
- Do not treat a screenshot of a startup error as a successful input probe.
- Do not burn more runs retrying raw-executable/statefile input paths. The
  accepted active-song route is the real ISO plus indexed `-state 1`.
- Exception: for current PCSX2 builds that expose SLUS code pages as read-only
  to external `WriteProcessMemory`, generate a throwaway prepatched `.p2s` from
  the accepted indexed state with
  `tools/prepatch_pcsx2_state_call_sequence.py`, then run
  `tools/trace_pcsx2_call_sequence.py --statefile <patched.p2s>
  --patch-manifest <manifest.json> --skip-code-patch --skip-stub-write`.
  This route is accepted by
  `pcsx2_known_hot_stateprepatch_hardened_control_20260622_current.json` and
  `pcsx2_lighting_keyqueue_stateprepatch_20260622_current.json`; it avoids live
  code-page writes while still using the normal writable trace ring.
- Do not force PCSX2 to the foreground. The user explicitly asked not to.
- Do not use `--focus-window` unless the user explicitly approves it.
- Do not use screen capture that can grab the Codex window. Use the trace tools'
  PCSX2 HWND `PrintWindow` capture path.
- Do not trust a trace unless the associated screenshot proves the run is
  in-song and not stuck on Retry, fail menu, or startup error.
- Do not treat fail-menu traces as gameplay proof.
- Do not count zero-call vtable traces as proof that a system is inactive
  unless the screenshot proves the right in-song window and the sampled camera
  window should exercise that system.
- Do not patch SLUS text/code pages with direct `WriteProcessMemory`; this
  failed with error `998`. Use the patched-state trace route for function-entry
  hooks, or live object/vtable redirection where the target page is writable.
- Do not use impossible counter values such as `0x3c19xxxx`; those came from
  scratch/counter contamination and are rejected.
- Do not infer native fixes from isolated visual guesses. The user rejected
  that workflow.
- Do not use the following rejected resume/setup traces as gameplay evidence:
  - `pcsx2_main_driver_node_children_8s_20260608.json`: indexed state failed
    and screenshot was the PCSX2 startup error.
  - `pcsx2_main_driver_node_children_statefile_8s_20260608.json`: absolute
    statefile loaded, but screenshot stayed on Song Failed/Retry.
  - `pcsx2_main_driver_node_children_statefile_pulses_8s_20260608.json`:
    repeated `Keyboard/L` pulses still stayed on Song Failed/Retry.
  - `pcsx2_retry_childpost_probe_20260608.json`: posting to child windows still
    stayed on Song Failed/Retry.
  - `pcsx2_retry_space_probe_20260608.json`: `Keyboard/Space` toggled PCSX2's
    pause hotkey, not the in-game retry/continue action.
  - `pcsx2_folder_state_probe_20260608.json`: launching the extracted disc
    folder with indexed state produced no usable PCSX2 window or screenshot.
  - `pcsx2_retry_input_probe_statefile_exact_20260608.json`: exact helper with
    raw executable/statefile stayed on Retry.
  - `pcsx2_retry_input_probe_systemcnf_state1_20260608.json`: `SYSTEM.CNF`
    launch produced a startup error.
  - `pcsx2_retry_input_probe_systemcnf_statefile_20260608.json`:
    `SYSTEM.CNF` launch failed to open CDVD.

## PCSX2 Setup Facts

Local emulator:

- `C:\Games\Emulators\PCSX2\pcsx2-qt.exe`
- Prior checkpoint version: `PCSX2 v2.7.93`

Local PS2 executable:

- `C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47`

Accepted local PS2 disc image:

- `C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso`

Accepted trace-only direct/autoplay PS2 disc image:

- ISO path when present:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_direct_autoplay.iso`
- Staged disc: `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_direct_autoplay_disc`
- Executable: `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_direct_autoplay_disc\GHDX_003.00`
- Source basis: user-provided `hmxmilohax/Guitar-Hero-II-Deluxe-Unified` plus
  stock GH2 PS2 disc data.
- Trace-only data patches forced `force_autoplay TRUE` and direct quickplay
  boot through `ui/init.dta`; do not treat these as native port code changes.
- Space note: this older generated ISO was deleted after the punk1 ISO rebuild
  on 2026-06-11. Rebuild it from the staged disc if needed; do not assume the
  ISO file currently exists.

Accepted trace-only direct/autoplay rock2 PS2 disc image:

- ISO path when present:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_rock2_autoplay.iso`
- Staged disc:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_rock2_autoplay_disc`
- Executable:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_rock2_autoplay_disc\GHDX_003.00`
- Source basis: same GH2DXu trace clone plus stock GH2 PS2 disc data.
- Trace-only data patches force direct quickplay/autoplay and
  `{game set_character rock2 TRUE}` in `ui/init.dta`.
- Build note: do not build an ISO directly from GH2DXu `out\ps2`; that creates
  a small disc image without stock assets. Copy the known-good combined
  direct-autoplay staged disc, overlay the rebuilt `out\ps2` files, then build
  the ISO. The accepted rock2 ISO size is `4,315,545,600` bytes.
- Space note: this older generated ISO was deleted after the deathmetal1 ISO
  rebuild on 2026-06-11. Rebuild it from the staged disc if needed; do not
  assume the ISO file currently exists.

Accepted trace-only direct/autoplay deathmetal1 PS2 disc image:

- ISO path when present:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_deathmetal1_autoplay.iso`
- Staged disc:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_deathmetal1_autoplay_disc`
- Executable:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_deathmetal1_autoplay_disc\GHDX_003.00`
- Trace-only data patches force direct quickplay/autoplay and
  `{game set_character deathmetal1 TRUE}` in `ui/init.dta`.
- Boot note: this cold path needed a 180-second pre-trace wait before the
  accepted active-gameplay window. A 100-second wait failed before patching.
- Space note: this older generated ISO was deleted after the direct-autoplay
  ISO rebuild on 2026-06-11. Rebuild it from the staged disc if needed; do not
  assume the ISO file currently exists.

Accepted trace-only direct/autoplay rockabill1 PS2 disc image:

- ISO path when present:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_rockabill1_autoplay.iso`
- Staged disc:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_rockabill1_autoplay_disc`
- Executable:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_rockabill1_autoplay_disc\GHDX_003.00`
- Trace-only data patches force direct quickplay/autoplay and
  `{game set_character rockabill1 TRUE}` in `ui/init.dta`.
- Space note: this older generated ISO was deleted after trace capture on
  2026-06-11. Rebuild it from the staged disc if needed; do not assume the ISO
  file currently exists.

Accepted trace-only direct/autoplay glam1 PS2 disc image:

- ISO path when present:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_glam1_autoplay.iso`
- Staged disc:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_glam1_autoplay_disc`
- Executable:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_glam1_autoplay_disc\GHDX_003.00`
- Trace-only data patches force direct quickplay/autoplay and
  `{game set_character glam1 TRUE}` in `ui/init.dta`.
- Space note: this older generated ISO was deleted after trace capture on
  2026-06-11. Rebuild it from the staged disc if needed; do not assume the ISO
  file currently exists.

Accepted trace-only direct/autoplay metal1 PS2 disc image:

- ISO path when present:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_metal1_autoplay.iso`
- Staged disc:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_metal1_autoplay_disc`
- Executable:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_metal1_autoplay_disc\GHDX_003.00`
- Trace-only data patches force direct quickplay/autoplay and
  `{game set_character metal1 TRUE}` in `ui/init.dta`.
- Space note: this older generated ISO was deleted after trace capture on
  2026-06-11. Rebuild it from the staged disc if needed; do not assume the ISO
  file currently exists.

Accepted trace-only direct/autoplay punk1 PS2 disc image:

- ISO path when present:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_punk1_autoplay.iso`
- Staged disc:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_punk1_autoplay_disc`
- Executable:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_punk1_autoplay_disc\GHDX_003.00`
- Trace-only data patches force direct quickplay/autoplay and
  `{game set_character punk1 TRUE}` in `ui/init.dta`.
- Build note: after rebuilding GH2DXu `out\ps2`, overlay with PowerShell
  `Copy-Item -Path "$outPs2\*" ...`, not `-LiteralPath` with `*`. The rejected
  first punk1 ISO did not overlay `GEN\MAIN.HDR` / `GEN\MAIN_1.ARK` and still
  traced classic. Verify staged timestamps before building the ISO.
- Space note: this older generated ISO was deleted after the alterna1 ISO
  rebuild on 2026-06-11. Rebuild it from the staged disc if needed; do not
  assume the ISO file currently exists.

Accepted trace-only direct/autoplay alterna1 PS2 disc image:

- ISO path when present:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_alterna1_autoplay.iso`
- Staged disc:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_alterna1_autoplay_disc`
- Executable:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_alterna1_autoplay_disc\GHDX_003.00`
- Trace-only data patches force direct quickplay/autoplay and
  `{game set_character alterna1 TRUE}` in `ui/init.dta`.
- Build note: verified fresh staged `GEN\MAIN.HDR` and `GEN\MAIN_1.ARK`
  timestamps before ISO build.
- Space note: this older generated ISO was deleted after the goth2 ISO rebuild
  on 2026-06-11. Rebuild it from the staged disc if needed; do not assume the
  ISO file currently exists.

Accepted trace-only direct/autoplay goth2 PS2 disc image:

- ISO path when present:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_goth2_autoplay.iso`
- Staged disc:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_goth2_autoplay_disc`
- Executable:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_goth2_autoplay_disc\GHDX_003.00`
- Trace-only data patches force direct quickplay/autoplay and
  `{game set_character goth2 TRUE}` in `ui/init.dta`.
- Build note: verified fresh staged `GEN\MAIN.HDR` and `GEN\MAIN_1.ARK`
  timestamps `2026-06-11 18:26:26` before ISO build. ImgBurn returned a
  nonzero process exit, but its log records successful image build and write;
  the accepted ISO size is `4,315,545,600` bytes.
- Space note: this older generated ISO was deleted after the funk1 ISO rebuild
  on 2026-06-11. Rebuild it from the staged disc if needed; do not assume the
  ISO file currently exists.

Accepted trace-only direct/autoplay funk1 PS2 disc image:

- ISO path when present:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_funk1_autoplay.iso`
- Staged disc:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_funk1_autoplay_disc`
- Executable:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_funk1_autoplay_disc\GHDX_003.00`
- Trace-only data patches force direct quickplay/autoplay and
  `{game set_character funk1 TRUE}` in `ui/init.dta`.
- Build note: verified fresh staged `GEN\MAIN.HDR` and `GEN\MAIN_1.ARK`
  timestamps `2026-06-11 18:39:25` before ISO build. ImgBurn returned a
  nonzero process exit, but its log records successful image build and write;
  the accepted ISO size is `4,315,545,600` bytes.
- Space note: this older generated ISO was deleted after the glam3 ISO rebuild
  on 2026-06-11. Rebuild it from the staged disc if needed; do not assume the
  ISO file currently exists.

Accepted trace-only direct/autoplay glam3 PS2 disc image:

- ISO path when present:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_glam3_autoplay.iso`
- Staged disc:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_glam3_autoplay_disc`
- Executable:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_glam3_autoplay_disc\GHDX_003.00`
- Trace-only data patches force direct quickplay/autoplay and
  `{game set_character glam3 TRUE}` in `ui/init.dta`.
- Build note: verified fresh staged `GEN\MAIN.HDR` and `GEN\MAIN_1.ARK`
  timestamps `2026-06-11 18:48:18` before ISO build. ImgBurn log records
  successful image build and write; the accepted ISO size is `4,315,545,600`
  bytes.
- Space note: this older generated ISO was deleted after the alterna3 ISO
  rebuild on 2026-06-11. Rebuild it from the staged disc if needed; do not
  assume the ISO file currently exists.

Accepted trace-only direct/autoplay alterna3 PS2 disc image:

- ISO path when present:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_alterna3_autoplay.iso`
- Staged disc:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_alterna3_autoplay_disc`
- Executable:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_alterna3_autoplay_disc\GHDX_003.00`
- Trace-only data patches force direct quickplay/autoplay and
  `{game set_character alterna3 TRUE}` in `ui/init.dta`.
- Build note: verified fresh staged `GEN\MAIN.HDR` and `GEN\MAIN_1.ARK`
  timestamps `2026-06-11 18:56:20` before ISO build. ImgBurn log records
  successful image build and write; the accepted ISO size is `4,315,545,600`
  bytes.
- Space note: this older generated ISO was deleted after the punk3 ISO rebuild
  on 2026-06-11. Rebuild it from the staged disc if needed; do not assume the
  ISO file currently exists.

Accepted trace-only direct/autoplay punk3 PS2 disc image:

- ISO path when present:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_punk3_autoplay.iso`
- Staged disc:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_punk3_autoplay_disc`
- Executable:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_punk3_autoplay_disc\GHDX_003.00`
- Trace-only data patches force direct quickplay/autoplay and
  `{game set_character punk3 TRUE}` in `ui/init.dta`.
- Build note: verified fresh staged `GEN\MAIN.HDR` and `GEN\MAIN_1.ARK`
  timestamps `2026-06-11 19:06:06` before ISO build. ImgBurn log records
  successful image build and write; the accepted ISO size is `4,315,545,600`
  bytes.
- Space note: this older generated ISO was deleted after the goth3 ISO rebuild
  on 2026-06-11. Rebuild it from the staged disc if needed; do not assume the
  ISO file currently exists.

Accepted trace-only direct/autoplay goth3 PS2 disc image:

- ISO path when present:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_goth3_autoplay.iso`
- Staged disc:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_goth3_autoplay_disc`
- Executable:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_goth3_autoplay_disc\GHDX_003.00`
- Trace-only data patches force direct quickplay/autoplay and
  `{game set_character goth3 TRUE}` in `ui/init.dta`.
- Build note: verified fresh staged `GEN\MAIN.HDR` and `GEN\MAIN_1.ARK`
  timestamps `2026-06-11 19:14:25` before ISO build. ImgBurn log records
  successful image build and write; the accepted ISO size is `4,315,545,600`
  bytes.
- Space note: this older generated ISO and staged folder were deleted during
  the post-metal3 cleanup on 2026-06-11. Rebuild it from stock disc data plus
  the current GH2DXu `out\ps2` files if needed; do not assume the ISO or
  staged folder currently exists.

Accepted trace-only direct/autoplay goth3 hair follow-up:

- `gh2dxu_goth3_hair_followup_20260611.json` launched cold with `--no-state`,
  `--disable-ee-recompiler`, and a 180-second pre-trace wait against a fresh
  trace-only goth3 ISO.
- The 100-second active gameplay trace recorded 321,305 total calls and
  retained the final 65,536-record ring with zero unknown function IDs.
- `hair_update_00176ff0` stayed zero-hit again. Treat goth3 hair as still
  unobserved in this song/window, not as a short-window miss.
- Same-window sampled rows still resolve goth3 IK/twist/look-at through
  `char/goth3/og/goth3.milo`.
- The generated ISO and staged folder were deleted immediately after parsing.

Accepted trace-only female-singer `crazyonyou` coverage:

- Rejected attempts:
  `gh2dxu_female_singer_character_probe_20260611.json`,
  `gh2dxu_female_singer_crazyonyou_character_probe_20260611.json`, and
  `gh2dxu_female_singer_crazyonyou_standard_probe_20260611.json` all produced
  gray-screen/zero-call captures. Do not use index-only direct boot for this
  song.
- Accepted route:
  set quickplay mode, set song index 6, set song symbol `crazyonyou`, set
  venue `fest`, set character `alterna1`, set guitar `lespaul default`, set
  difficulty `kDifficultyExpert`, then call `game set_quickplay`.
- Accepted artifacts:
  `gh2dxu_female_singer_explicit_crazyonyou_sampled_probe_20260611.json`
  recorded 27,726 calls with standard character targets live, and
  `gh2dxu_female_singer_explicit_crazyonyou_driver_sample_20260611.json`
  recorded 28,345 calls with driver/source sampling.
- Source proof:
  `main.drv` at `0x00dff6b0`, `upperTwist_L.ik` at `0x00dff730`,
  `upperTwist_R.ik` at `0x00dffd80`, and `dreads.hair` at `0x00dffa10`
  resolve through sampled owner/source pointers to
  `char/female_singer/og/female_singer.milo` at source row `0x00cc624c`.
- Follow-up `ftk` artifact:
  `gh2dxu_female_singer_explicit_ftk_face_twist_sample_20260611.json` used
  explicit song `ftk`, venue `battle`, character `punk1`, and guitar
  `lespaul`. Its screenshot visibly shows the female singer on camera and its
  sampled rows again prove female `main.drv`, `upperTwist_L/R.ik`, and
  `dreads.hair` through `char/female_singer/og/female_singer.milo`.
- Follow-up `tattooedloveboys` artifact:
  `gh2dxu_female_singer_explicit_tattooedloveboys_face_twist_sample_20260611.json`
  used explicit song `tattooedloveboys`, venue `small1`, character `alterna1`,
  and guitar `sg`. It again proves female `main.drv`, `upperTwist_L/R.ik`, and
  `dreads.hair`.
- Follow-up accepted argument-sampling artifact:
  `gh2dxu_female_lookat_arg_sample_20260611.json` used the same direct
  `tattooedloveboys` route and captured a valid active gameplay screenshot with
  the female singer visible. Counts were `16994` total calls:
  `chardriver_update_00171868` 84, `foretwist_001756b0` 18,
  `uppertwist_00182400` 48, `hair_update_00176ff0` 12,
  `lookat_update_0017d690` 12, `lookat_vec_002da768` 12,
  `lookat_math_002d5a40` 12, `trans_dirty_001dd788` 6176, and
  `trans_world_003d7220` 10620.
- That follow-up strengthens the caveat rather than closing it:
  `dreads.hair` `0x00e45260`, `upperTwist_L.ik` `0x00e44f80`, and
  `upperTwist_R.ik` `0x00e455d0` resolve to
  `char/female_singer/og/female_singer.milo`, but `lookat_update_0017d690`
  sampled only `l-eye.lookat` `0x00eebc90` and `r-eye.lookat` `0x00eecfe0`,
  with downstream look-at math resolving through `char/alterna1/og/alterna1.milo`.
- Remaining caveat:
  across `crazyonyou`, `ftk`, and multiple `tattooedloveboys` traces,
  female-singer look-at and foretwist still did not resolve to
  `char/female_singer/og/female_singer.milo`. Treat those as unresolved
  trigger/coverage gaps before claiming full parity with male-singer coverage.
- The generated ISO and staged folder were deleted immediately after parsing.

Accepted trace-only direct/autoplay metal3 PS2 disc image:

- ISO path when present:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_metal3_autoplay.iso`
- Staged disc when present:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_metal3_autoplay_disc`
- Executable when staged:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_metal3_autoplay_disc\GHDX_003.00`
- Trace-only data patches force direct quickplay/autoplay and
  `{game set_character metal3 TRUE}` in `ui/init.dta`.
- Build note: verified fresh staged `GEN\MAIN.HDR` and `GEN\MAIN_1.ARK`
  timestamps `2026-06-11 19:23:14` before ISO build. ImgBurn log
  `gh2dxu_metal3_imgburn_20260611.log` records a successful image build and
  write; the accepted ISO size is `4,315,545,600` bytes.
- Space note: the generated ISO and staged folder were deleted immediately
  after `gh2dxu_metal3_character_probe_long_20260611.json` was captured and
  parsed. Do not leave `GH2DXu_PS2_trace_*` staged folders in the workspace.

Accepted trace-only direct/autoplay Grim-family PS2 disc image:

- ISO path when present:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_gr80_autoplay.iso`
- Staged disc when present:
  `C:\Programming\GitHub\Guitar Hero II\GH2DXu_PS2_trace_gr80_autoplay_disc`
- Trace-only data patches force direct quickplay/autoplay and
  `{game set_character grim TRUE}` in `ui/init.dta`, plus a temporary
  `gh2.dta` outfit-order swap from `(grim (grim) (gr80))` to
  `(grim (gr80) (grim))`.
- Rejected direct attempt:
  `gh2dxu_gr80_character_probe_20260611.json` used
  `{game set_character gr80 TRUE}`, stayed on the loading screen, and recorded
  zero calls. Do not use direct `gr80` as the runtime character token.
- Accepted route:
  `gh2dxu_gr80_grimroute_character_probe_20260611.json` reached active
  gameplay and recorded 178,565 total calls. Same-process sampled rows resolved
  the runtime source as `char/grim/og/grim.milo`, not
  `char/gr80/og/gr80.milo`. GH2DXu includes `char/gr80/.../gr80.milo_ps2`
  files, but `char_objects_ps2.dta` still lists the in-song GH2 char source as
  `char/grim/og/grim.milo`.
- Space note: the generated ISO and staged folder were deleted immediately
  after the accepted Grim-family trace was captured and parsed.

Known savestate file:

- `C:\Games\Emulators\PCSX2\sstates\SLUS-21447 (2A6C845B).01.p2s`
- `C:\Games\Emulators\PCSX2\sstates\GHDX-00300 (A9BBA52A).01.p2s`

Current inventory note: the GHDX savestate is the preferred arms/hands trace
state. It was captured during active direct-autoplay gameplay with visible band,
score, and note hits. Additional-song/additional-guitarist coverage requires
new accepted GHDX savestates or a direct-boot data tweak for each target song.

Important: for GHDX direct-autoplay traces, use the GHDX ISO and indexed
`-state 1`; for retail comparison traces, use the stock ISO and stock SLUS
state. Do not mix the stock SLUS ELF addresses with `GHDX_003.00`; the GHDX
animation functions are shifted and must be remapped/verified before tracing.
Before any long trace, reestablish active-song entry with the least expensive
probe and visually inspect the resulting screenshot.

Current GHDX direct-autoplay proof:

- Stable savestate:
  `C:\Games\Emulators\PCSX2\sstates\GHDX-00300 (A9BBA52A).01.p2s`
- Song-start/state capture report:
  `gh2dxu_direct_autoplay_state_capture_20260611.json`
- Song-start/state capture screenshot:
  `gh2dxu_direct_autoplay_state_capture_20260611.window.png`
- Long arm/hand autoplay trace:
  `gh2dxu_arm_hand_autoplay_trace_20260611.json`
- Long trace screenshot:
  `gh2dxu_arm_hand_autoplay_trace_20260611.window.png`
- Downstream hand output/Trans bridge trace:
  `gh2dxu_hand_output_trans_bridge2_20260611.json`
- Downstream bridge screenshot:
  `gh2dxu_hand_output_trans_bridge2_20260611.window.png`
- Hair/eye coverage trace:
  `gh2dxu_hair_eyes_lookat_trace_20260611.json`
- Hair/eye coverage screenshot:
  `gh2dxu_hair_eyes_lookat_trace_20260611.window.png`
- Direct/classic full character-controller trace:
  `gh2dxu_direct_character_trace_long_20260611.json`
- Direct/classic full character-controller screenshot:
  `gh2dxu_direct_character_trace_long_20260611.window.png`
- Punk1 accepted character-controller probe:
  `gh2dxu_punk1_character_probe_20260611.json`
- Punk1 accepted character-controller screenshot:
  `gh2dxu_punk1_character_probe_20260611.window.png`
- Alterna1 accepted character-controller probe:
  `gh2dxu_alterna1_character_probe_20260611.json`
- Alterna1 accepted character-controller screenshot:
  `gh2dxu_alterna1_character_probe_20260611.window.png`
- Alterna1 confirming long character-controller trace:
  `gh2dxu_alterna1_character_trace_long_20260611.json`
- Alterna1 confirming long screenshot:
  `gh2dxu_alterna1_character_trace_long_20260611.window.png`
- Goth2 accepted character-controller probe:
  `gh2dxu_goth2_character_probe_20260611.json`
- Goth2 accepted character-controller screenshot:
  `gh2dxu_goth2_character_probe_20260611.window.png`
- Funk1 accepted character-controller probe:
  `gh2dxu_funk1_character_probe_20260611.json`
- Funk1 accepted character-controller screenshot:
  `gh2dxu_funk1_character_probe_20260611.window.png`
- Glam3 accepted character-controller probe:
  `gh2dxu_glam3_character_probe_20260611.json`
- Glam3 accepted character-controller screenshot:
  `gh2dxu_glam3_character_probe_20260611.window.png`
- Alterna3 accepted character-controller probe:
  `gh2dxu_alterna3_character_probe_20260611.json`
- Alterna3 accepted character-controller screenshot:
  `gh2dxu_alterna3_character_probe_20260611.window.png`
- Punk3 accepted character-controller probe:
  `gh2dxu_punk3_character_probe_20260611.json`
- Punk3 accepted character-controller screenshot:
  `gh2dxu_punk3_character_probe_20260611.window.png`
- Goth3 accepted partial character-controller probe:
  `gh2dxu_goth3_character_probe_20260611.json`
- Goth3 accepted partial character-controller screenshot:
  `gh2dxu_goth3_character_probe_20260611.window.png`
- Rock2 hair/look-at trace:
  `gh2dxu_rock2_hair_trace_20260611.json`
- Rock2 hair/look-at screenshot:
  `gh2dxu_rock2_hair_trace_20260611.window.png`
- Rock2 full character-controller trace:
  `gh2dxu_rock2_character_trace_20260611.json`
- Rock2 full character-controller screenshot:
  `gh2dxu_rock2_character_trace_20260611.window.png`
- Deathmetal1 no-patch boot probe:
  `gh2dxu_deathmetal1_boot_probe_20260611.json`
- Deathmetal1 clean hair/look-at trace:
  `gh2dxu_deathmetal1_hair_trace_clean_20260611.json`
- Deathmetal1 clean hair/look-at screenshot:
  `gh2dxu_deathmetal1_hair_trace_clean_20260611.window.png`
- Deathmetal1 full character-controller trace:
  `gh2dxu_deathmetal1_character_trace_long_20260611.json`
- Deathmetal1 full character-controller screenshot:
  `gh2dxu_deathmetal1_character_trace_long_20260611.window.png`
- Rockabill1 full character-controller trace:
  `gh2dxu_rockabill1_character_trace_20260611.json`
- Rockabill1 full character-controller screenshot:
  `gh2dxu_rockabill1_character_trace_20260611.window.png`
- Glam1 full character-controller trace:
  `gh2dxu_glam1_character_trace_20260611.json`
- Glam1 full character-controller screenshot:
  `gh2dxu_glam1_character_trace_20260611.window.png`
- Metal1 full character-controller trace:
  `gh2dxu_metal1_character_trace_20260611.json`
- Metal1 full character-controller screenshot:
  `gh2dxu_metal1_character_trace_20260611.window.png`
- The long trace proves active successful-note gameplay and retained
  hand-command dispatch, hand scheduler, scheduler push, blend entry/tick,
  clip eval/output/final, IK child, foretwist, uppertwist, and hair updates in
  one 240-second window. See `CHARACTER_DEFORM_FORMAT.md` for exact remapped
  addresses and object rows. The downstream bridge trace proves the same
  successful-note route continues through clip output lanes, twist math,
  Trans dirty recursion, and Trans world resolution.
- The hair/eye coverage trace proves `hair.hair` is live for
  `char/classic/og/classic.milo` in the same GHDX successful-note state.
  Eye/look-at object update slots stayed zero in that retained window, so
  eyes remain an open coverage gap requiring another accepted state/window.
- The direct/classic long character trace used `--no-state`,
  `--disable-ee-recompiler`, a 180-second cold wait, and the regular high
  scratch range. Its total counter was contaminated (`12,013,888`), so use the
  retained 65,536-record ring only. The retained window had zero unknown
  function IDs and proves classic-owned hand dispatch/schedule,
  scheduler/blend, clip output, IK, foretwist, uppertwist, `hair.hair`, and
  Trans resolution under `char/classic/og/classic.milo`. Its screenshot shows
  successful-note gameplay with visible HUD/note highway/band/venue. Classic
  look-at updates remained zero-hit in the retained window.
- The punk1 accepted probe used `--no-state`, `--disable-ee-recompiler`, a
  180-second cold wait, and the regular high scratch range. Its total counter
  was contaminated (`155,908`), so use the retained 65,536-record ring only.
  The retained window had zero unknown function IDs and proves punk1-owned hand
  dispatch/schedule, scheduler/blend, clip output, IK, foretwist, uppertwist,
  `hair.hair`, `l-eye.lookat`, `r-eye.lookat`, and Trans resolution under
  `char/punk1/og/punk1.milo`. The screenshot visibly shows punk1 playing
  guitar.
- The alterna1 accepted probe used `--no-state`, `--disable-ee-recompiler`, a
  180-second cold wait, and the regular high scratch range. It retained all
  6,913 calls with zero unknown function IDs and proves alterna1-owned IK,
  foretwist, uppertwist, `bangs.hair`, `l-eye.lookat`, `r-eye.lookat`, clip
  output, and Trans resolution under `char/alterna1/og/alterna1.milo`. Its
  screenshot shows successful-note gameplay with HUD/note highway/band/venue.
  Hand command dispatch/scheduler stayed zero in both the accepted probe and
  the 8,258-call confirming long trace, so alterna1 hand-command coverage is
  still open.
- The goth2 accepted probe used `--no-state`, `--disable-ee-recompiler`, a
  180-second cold wait, and the regular high scratch range. It recorded 196,714
  total calls and retained the final 65,536-record ring with zero unknown
  function IDs. The retained window proves goth2-owned hand dispatch/schedule,
  scheduler/blend, clip output, IK, foretwist, uppertwist, `coat.hair`,
  `hair_front.hair`, `l-eye.lookat`, `r-eye.lookat`, and Trans resolution under
  `char/goth2/og/goth2.milo`. The screenshot is an active in-venue forced
  guitarist frame; source proof comes from sampled rows.
- The funk1 accepted probe used `--no-state`, `--disable-ee-recompiler`, a
  180-second cold wait, and the regular high scratch range. It recorded 160,521
  total calls and retained the final 65,536-record ring with zero unknown
  function IDs. The retained window proves funk1-owned hand dispatch/schedule,
  scheduler/blend, clip output, IK, foretwist, uppertwist, `coat_C.hair`,
  `coat_LR.hair`, `hair.hair`, `l-eye.lookat`, `r-eye.lookat`, adjacent
  `CharEyes.eyes`, and Trans resolution under `char/funk1/og/funk1.milo`. The
  screenshot is active venue/crowd proof only; source proof comes from sampled
  rows.
- The glam3 accepted probe used `--no-state`, `--disable-ee-recompiler`, a
  180-second cold wait, and the regular high scratch range. It recorded 159,936
  total calls and retained the final 65,536-record ring with zero unknown
  function IDs. The retained window proves glam3-owned hand dispatch/schedule,
  scheduler/blend, clip output, IK, foretwist, uppertwist, `hair.hair`,
  adjacent `CharEyes.eyes`, `l-eye.lookat`, `r-eye.lookat`, and Trans
  resolution under `char/glam3/og/glam3.milo`. The screenshot is an active
  venue/crowd/performer frame; source proof comes from sampled rows.
- The alterna3 accepted probe used `--no-state`, `--disable-ee-recompiler`, a
  180-second cold wait, and the regular high scratch range. It recorded and
  retained all 38,487 calls with zero unknown function IDs. The window proves
  alterna3-owned hand dispatch/schedule, scheduler/blend, clip output, IK,
  foretwist, uppertwist, `bangs.hair`, `l-eye.lookat`, `r-eye.lookat`, and
  Trans resolution under `char/alterna3/og/alterna3.milo`. The screenshot is
  active blurred venue proof only; source proof comes from sampled rows.
- The punk3 accepted probe used `--no-state`, `--disable-ee-recompiler`, a
  180-second cold wait, and the regular high scratch range. It recorded 113,926
  total calls and retained the final 65,536-record ring with zero unknown
  function IDs. The retained window proves punk3-owned hand dispatch/schedule,
  scheduler/blend, clip output, IK, foretwist, uppertwist, `hair.hair`,
  adjacent `CharEyes.eyes`, `l-eye.lookat`, `r-eye.lookat`, and Trans
  resolution under `char/punk3/og/punk3.milo`. The screenshot is active venue
  proof only; source proof comes from sampled rows.
- The goth3 accepted probe used `--no-state`, `--disable-ee-recompiler`, a
  180-second cold wait, and the regular high scratch range. It recorded 145,799
  total calls and retained the final 65,536-record ring with zero unknown
  function IDs. The retained window proves goth3-owned hand dispatch/schedule,
  scheduler/blend, clip output, IK, foretwist, uppertwist, `l-eye.lookat`,
  `r-eye.lookat`, adjacent `CharEyes.eyes`, and Trans resolution under
  `char/goth3/og/goth3.milo`. It did not prove goth3 hair:
  `hair_update_00176ff0` stayed zero in this window. Keep goth3 hair as an open
  per-character gap.
- The rock2 cold/direct trace used `--no-state` and proves rock2-specific
  `hair_back.hair`, `hair_front.hair`, `l-eye.lookat`, and `r-eye.lookat`
  controller rows under `char/rock2/og/rock2.milo`. Its screenshot is an
  in-venue camera frame, not a note-highway proof frame; cite the per-frame
  controller counts and direct-autoplay route for runtime proof.
- The deathmetal1 clean trace used `--no-state`, `--disable-ee-recompiler`,
  180-second cold wait, and `--data-base 0x01F00000`. Its retained window had
  zero unknown function IDs and proves `hair_back.hair`, `hair_front.hair`,
  `l-eye.lookat`, and `r-eye.lookat` controller rows for the forced
  deathmetal1 character. Its total counter is not accepted because the high
  data-base counter was contaminated; use retained records only.
- The deathmetal1 long character trace used `--no-state`,
  `--disable-ee-recompiler`, a 180-second cold wait, and the regular high
  scratch range. Its 97,984 total calls wrapped the 65,536-record ring, so use
  retained records only. The retained window had zero unknown function IDs and
  includes hand dispatch/schedule, scheduler/blend, clip output, IK, foretwist,
  uppertwist, `hair_back.hair`, `hair_front.hair`, `l-eye.lookat`,
  `r-eye.lookat`, and Trans resolution. Its screenshot is active venue/crowd
  only; deathmetal1 proof comes from sampled rows resolving to
  `char/deathmetal1/og/deathmetal1.milo`.
- The rockabill1 trace used `--no-state`, `--disable-ee-recompiler`, a
  180-second cold wait, and the regular high scratch range. Its screenshot is
  an active in-venue frame with the bassist on camera, not visual rockabill1
  proof. Rockabill1 is proven by same-process sampled controller/source rows
  resolving to `char/rockabill1/og/rockabill1.milo`, with same-window hand
  dispatch, scheduler/blend, clip output, IK, foretwist, uppertwist, hair,
  look-at, and Trans resolution. The sampled rows also show singer/bass/drummer
  controller rows in the same window, so future implementation must filter by
  owner/source, not just function family.
- The glam1 trace used the same cold/no-state route and retained all 22,567
  records with zero unknown function IDs. Its screenshot is an active venue
  frame without a visible performer, so glam1 proof comes from sampled rows
  resolving to `char/glam1/og/glam1.milo`. It proves glam1-specific
  `left_hand.ik`, `right_hand.ik`, foretwist, uppertwist, `hair.hair`,
  `CharEyes.eyes`, `l-eye.lookat`, and `r-eye.lookat` rows in the same update
  window.
- The metal1 trace used the same cold/no-state route and retained all 24,224
  records with zero unknown function IDs. Its screenshot is active venue only,
  so metal1 proof comes from sampled rows resolving to
  `char/metal1/og/metal1.milo`. It proves metal1-specific hand IK, foretwist,
  uppertwist, `bangs.hair`, `pony.hair`, `l-eye.lookat`, `r-eye.lookat`, and
  adjacent `CharEyes.eyes` rows in the same update window.
- The rock2 full character trace used the same cold/no-state route and retained
  all 21,614 records with zero unknown function IDs. Its screenshot is active
  venue proof only; rock2 proof comes from sampled rows resolving to
  `char/rock2/og/rock2.milo`. It extends the earlier rock2 hair/look-at trace
  with same-window hand dispatch, scheduler/blend, clip output, IK, foretwist,
  uppertwist, `hair_back.hair`, `hair_front.hair`, `l-eye.lookat`,
  `r-eye.lookat`, and Trans resolution. The same retained window also includes
  singer, bassist, and drummer twist rows, so owner/source filtering remains
  mandatory.
- Rejected trace:
  `gh2dxu_hand_output_trans_bridge_20260611.json` showed active gameplay but
  zero calls because the lower scratch/stub range was unsuitable. Reuse the
  high `--stub-base 0x01C00000 --data-base 0x01D00000` range for GHDX traces.
- Rejected trace:
  `gh2dxu_punk1_character_trace_long_20260611.json` was captured from a
  bad-staged ISO and resolves to `char/classic/og/classic.milo`, not punk1.
  The staging bug was using `Copy-Item -LiteralPath` with a wildcard, which did
  not replace `GEN\MAIN.HDR` / `GEN\MAIN_1.ARK` in the combined-disc folder.
- Secondary/rejected trace:
  `gh2dxu_deathmetal1_hair_trace_20260611.json` reached the correct rows but
  had 618 unknown/corrupted ring records. Use
  `gh2dxu_deathmetal1_hair_trace_clean_20260611.json` for the accepted retained
  deathmetal1 window.

Current resume-specific launch findings:

- Accepted active-song proof:
  - command used `tools/probe_pcsx2_retry_input.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --method post --keys 0x4c --settle 4`
  - report:
    `pcsx2_retry_input_probe_iso_state1_20260608.json`
  - screenshot:
    `pcsx2_retry_input_probe_iso_state1_20260608.0x4c.window.png`
  - screenshot state: active in-song gameplay at Battle of the Bands, 59/60
    FPS/VPS, no Retry/fail/startup screen.
- Therefore, use the ISO plus indexed `-state 1` route for the next trace.
- `--statefile "C:\Games\Emulators\PCSX2\sstates\SLUS-21447 (2A6C845B).01.p2s"`
  successfully loads the known Retry savestate from raw launch contexts, but
  the resumed key-post path stayed on Retry. Treat it as rejected for active
  gameplay trace entry.
- Launching the raw `SLUS_214.47` executable reports `Unknown Game` in the
  PCSX2 title bar; this explains the indexed-state lookup failures and is
  rejected for active trace entry.
- Launching the extracted disc folder did not produce a usable window in the
  quick probe and is not currently a proven replacement launch path.
- The current config has `Cross = Keyboard/L` and `Start = Keyboard/Space`,
  but posted Windows key messages are not reaching the emulated pad in the
  resumed Retry state. `Keyboard/Space` only toggled PCSX2 pause.
- Current pad map also has d-pad/left stick on WASD, `Circle` on
  `Keyboard/Semicolon`, `Square` on `Keyboard/K`, and `Triangle` on
  `Keyboard/O`. Do not blindly use `Space` for in-game Start during trace
  setup without resolving the PCSX2 pause hotkey conflict first.
- PCSX2 logs reported `Enumerated 0 devices`; local checks found no active
  ViGEm/vJoy service and no usable Python virtual-controller module
  (`vgamepad`, `pyvjoy`, `inputs`, `pygame`, `sdl2` were missing).
- A reversible XInput probe was attempted on 2026-06-08:
  - backed up `C:\Games\Emulators\PCSX2\inis\PCSX2.ini` to
    `PCSX2.ini.codex_input_probe_backup_20260608`
  - temporarily set `XInput = true` and added `XInput-0/A` to Cross
  - Windows reported the Xbox/XInput devices as `Unknown`
  - `Enable-PnpDevice` failed with `Generic failure`
  - the PCSX2 config was restored to the backup; current config is again
    `XInput = false` and `Cross = Keyboard/L`
- The XInput detour is obsolete for the current trace path. Do not repeat it
  unless the ISO plus indexed-state route stops producing active gameplay.
- Temporary Start navigation is now proven when the PCSX2 pause hotkey is moved
  away from `Keyboard/Space`. Use a reversible config edit only inside the
  trace helper's setup/cleanup window: set `TogglePause = Keyboard/P`, post
  `Cross` (`0x4c`) to enter gameplay, post `Start` (`0x20`) to open the pause
  menu, then restore the original config.
- From state `1`, the accepted route back to the setlist is:
  `Cross`, `Start`, two `S` posts, `Cross`, `Cross`. Two `S` posts select
  `QUIT` because both d-pad down and left-stick down are mapped to `Keyboard/S`,
  so each post moves two menu positions in that pause menu.
- From the setlist, one `S` post moves to `Surrender`, then `Cross` reaches
  difficulty and another `Cross` starts the song. Use this for alternate
  guitarist coverage until a better accepted savestate exists.

## Input And Capture Rules

Preferred input path:

- Use posted window messages to the PCSX2 HWND.
- The historically accepted Retry/Cross key was `0x4c`.
- In current tools this is `--input-method post` or the helper's default
  `post_key(...)`.
- Current accepted context: posted `0x4c` leaves Retry when launching the real
  ISO with indexed `-state 1`. It did not leave Retry when launching raw
  executable/statefile contexts, so do not use those rejected paths.
- `tools/trace_pcsx2_call_sequence.py` and
  `tools/sample_pcsx2_object_words.py` now accept `--nav-key VK[:wait]` for
  deterministic post-retry menu navigation. `trace_pcsx2_call_sequence.py`
  also accepts `--sample-a0 target_name:size` and
  `--sample-a0-follow offset:size`; use those for same-process owner/source
  sampling so shifted allocations do not corrupt role mapping.
- Both trace helpers now accept `--nav-chord VK+VK[:wait]`. This was added for
  controller-cheat probes such as d-pad-right plus Square without foregrounding
  PCSX2.

Preferred capture path:

- Use `PrintWindow` capture against the PCSX2 HWND.
- Accepted screenshots must show active in-song gameplay and, when visible,
  healthy PCSX2 status such as `FPS 60`, `VPS 60`, and nonzero EE/VU/GS.

Rejected capture states:

- PCSX2 startup error dialog.
- Retry screen after the input pulse.
- Fail menu unless the trace target is explicitly fail-menu behavior.
- Frozen screen with no useful PCSX2 activity when the trace depends on live
  per-frame calls.
- Codex/browser/native app window accidentally captured.

## Proven Trace Mechanism

The proven mechanism is live object vtable redirection:

1. Launch PCSX2 and parse the EE host base from its log.
2. Wait until the saved Retry screen is ready.
3. If tracing gameplay-only objects, leave Retry first.
4. Copy selected live vtables into scratch EE RAM.
5. Replace code-pointer slots in the copies with small logging trampolines.
6. Redirect only the exact live object vptr words.
7. Sample counters and last `a0..a3`.
8. Capture the PCSX2 HWND and verify the screenshot.

Scratch ranges used by accepted traces:

- stubs around `0x01c00000`
- counters around `0x01d00000`
- copied tables around `0x01e00000`

Accepted exact deferred twist/hair trace:

- `pcsx2_anim_vtable_trace_20260608_exact_twist_hair_deferred_printwindow.json`
- Mode: `--redirect-after-retry --input-method post --include "exact_*"`
- Redirects: `17/17`
- Nonzero calls:
  - `CharForeTwist`: table `0x003e77a8`, slot `0x0c`, function `0x00175678`
  - `CharHair`: table `0x003e77e8`, slot `0x0c`, function `0x00176fb8`
  - `CharUpperTwist`: table `0x003e8030`, slot `0x0c`, function `0x001823c8`

Accepted character exact rerun also proved:

- `CharLookAt`: table `0x003e7c28`, slot `0x0c`, function `0x0017d658`

## Current Evidence Summary

Character driver and IK:

- `CharDriver`/driver table path hit functions `0x001737b8`, `0x00173780`,
  and `0x001d2c48`.
- Static fanout now proves:
  - `0x00173780` calls reset/setup `0x00170d10`.
  - `0x001737b8` calls per-frame scheduler `0x00171830`.
  - `0x00171830` calls state test `0x001710b8`, current node/clip selector
    `0x00171db0`, scheduler/blend push `0x00171248`, and an indirect
    clip/object callback at `0x00171ad8`.
  - `0x003399f0` is only a tiny wrapper slot that stores `a1` at `this+0x04`;
    do not treat it as the complete `CharDriverMidi` event path.
- `CharIKHand` path hit `0x0017a080`, plus setup/shared paths
  `0x001d2ab0` and `0x001d2c48`.
- Servo/main-driver path hit `0x001bb2d0` and `0x001bab10`.
- Short no-focus in-song CharDriver chain probes succeeded:
  - `pcsx2_chardriver_chain_20260608_probe.json`: right hand outer
    `0x00dbc9a0`, inner `0x00dbc9ac`, active state `0x00dbc9f0`
  - `pcsx2_chardriver_chain_left_20260608_probe.json`: left hand outer
    `0x00dbca40`, inner `0x00dbca4c`, active state `0x00dbca90`
- Both screenshots show active in-song PCSX2 at 60 FPS/VPS.
- Both short probes found `inner+0x1c = 0x0044d630`; that location reads as a
  property/table area with first word zero, so the `0x00171ad8` indirect
  callback target was not captured yet.
- Right/left hand current symbols differed as expected:
  - right `inner+0x20`: `strum_open`
  - left `inner+0x20`: `finger_open`
- The only changing inner-driver word in each two-second probe was
  `inner+0x28`, likely a phase/time accumulator:
  - right `0x00dbc9d4`: `0x3e202149 -> 0x40391eb2`
  - left `0x00dbca74`: `0x3e201d4f -> 0x403ac08d`
- Active-state vtable tracing found a new downstream command slot:
  - `pcsx2_chardriver_state_vtables_20260608.json`
  - `pcsx2_chardriver_active_adjacent_vtables_20260608.json`
  - table `0x003e74e8`, slot `0x34`, function `0x00173b98`, 15 calls in
    each accepted 10-second in-song trace
  - last registers: `a0=0x01ffe6e0`, `a1=0x00dbc980`, `a2=0x00850c80`,
    `a3=0`
- Static fanout for `0x00173b98`:
  - classifies `a2` through `0x002b7e50`
  - compares against `midi_parser` and `set_inactive_clip`
  - `midi_parser` path calls `0x00173d20`
  - `set_inactive_clip` path calls `0x00173e18`
  - fallback path calls broad command dispatcher `0x001726d8`
- `0x00173d20` reaches `0x00171248`, so the active-state command path is now
  trace-backed as a live bridge into the scheduler/blend push.
- `pcsx2_sample_chardriver_state_args_20260608.json` sampled live
  `a2=0x00850c80`: ten words changed over four seconds, mostly pointer/list
  cells around `+0x60..+0x78` and `+0xd0..+0xd8`.
- Focused return-wrapper tracing:
  - tool: `tools/trace_pcsx2_vtable_return_slot.py`
  - accepted trace: `pcsx2_chardriver_state_return_short_20260608.json`
  - used scratch ranges `0x01c00000`, `0x01d00000`, `0x01e00000`
  - wrapped table `0x003e74e8`, slot `0x34`, original `0x00173b98`
  - captured one call in five seconds
  - last registers: `a0=0x01ffe6e0`, `a1=0x00dbca20`, `a2=0x00850c80`,
    `a3=0`
  - top-level return `v0=0x01ffe6e0`
  - return rows include `+0x10=0x0084f550` and `+0x20=0x0059f010`
- Rejected wrapper trace: `pcsx2_chardriver_state_return_20260608.json`
  ended on the fail menu with counter zero. Do not use it as evidence.
- The accepted wrapper trace proves the top-level `0x00173b98` return object,
  not the internal `0x00171248` return object.
- Accepted pointer-cell sampler for the internal scheduler/blend object:
  - tool: `tools/sample_pcsx2_pointer_targets.py`
  - accepted report:
    `pcsx2_chardriver_scheduler_return_cells_rerun_20260608.json`
  - accepted screenshot:
    `pcsx2_chardriver_scheduler_return_cells_rerun_20260608.window.png`
  - screenshot state: active in-song stage view, 60 FPS/VPS
  - right hand owner `0x00dbc980`, inner driver `0x00dbc98c`, cell
    `inner+0x38 = 0x00dbc9c4`, pointer `0x0076bb10`
  - left hand owner `0x00dbca20`, inner driver `0x00dbca2c`, cell
    `inner+0x38 = 0x00dbca64`, pointer `0x0076be90`
  - both pointed objects changed rows `+0x0c`, `+0x10`, `+0x14`, and
    `+0x1c` during the two-second active sample
  - common rows included `+0x00=0x00000024`, `+0x08=0x3f800000`,
    `+0x18=0x3f800000`, `+0x20=0x3f7fec44`, `+0x38=0xf149f2ca`,
    and `+0x3c=0xffffffff`
  - left pointed object additionally had `+0x48=0x00552217`
    (`HandMap_DropD2`) plus EE-RAM list/object pointers through the sampled
    `+0x48..+0x7c` range; right pointed object was zero in that later range
    in this short sample
- Rejected pointer-cell sampler:
  `pcsx2_chardriver_scheduler_return_cells_20260608.json`; before/final
  screenshots were both the fail menu, so do not use it as gameplay evidence.
- Accepted child-pointer sampler:
  - report: `pcsx2_chardriver_scheduler_child_cells_20260608.json`
  - screenshot: `pcsx2_chardriver_scheduler_child_cells_20260608.window.png`
  - screenshot state: active in-song stage view, 60 FPS/VPS
  - right scheduler `+0x24` cell `0x0076bb34` -> `0x00ebf420`
  - right scheduler `+0x2c` cell `0x0076bb3c` -> `0x00b8be10`
  - left scheduler `+0x24` cell `0x0076beb4` -> `0x00f1e760`
  - left scheduler `+0x2c` cell `0x0076bebc` -> `0x00b8be10`
  - `+0x24` targets are hand-specific and stable in the short sample
  - `+0x2c` target was shared by both hands in this short sample
  - child rows did not change over two seconds, so this is structural/list
    evidence, not the changing scheduler timing rows
- Accepted focused owner/inner driver sampler:
  - tool: `tools/sample_pcsx2_object_words.py --no-default-targets`
  - report: `pcsx2_sample_chardriver_owner_blocks_focused_20260608.json`
  - screenshot: `pcsx2_sample_chardriver_owner_blocks_focused_20260608.window.png`
  - screenshot state: active in-song stage view, 60 FPS/VPS
  - right owner block starts at `0x00dbc980`; right inner base for
    `0x00171248` starts at `0x00dbc98c`
  - left owner block starts at `0x00dbca20`; left inner base for
    `0x00171248` starts at `0x00dbca2c`
  - right inner rows: `+0x38=0x0076bb10`, `+0x3c=0x0044d630`,
    `+0x40=0x00551f7b` (`strum_open`), `+0x44=5`,
    `+0x48` changed `0x3dd57810 -> 0x4035c830`,
    `+0x60=0x00ebf420`
  - left inner rows: `+0x38=0x0076be90`, `+0x3c=0x0044d630`,
    `+0x40=0x005520b0` (`finger_open`), `+0x44=5`,
    `+0x48` changed `0x3dd57810 -> 0x4035c830`,
    `+0x60=0x00f1e760`
  - broad owner ranges overlap adjacent packed hand blocks; do not interpret a
    `0x100` owner sample as one isolated hand object
- Accepted owner-base callback-chain probes:
  - right report: `pcsx2_chardriver_chain_owner_right_20260608.json`
  - right screenshot: `pcsx2_chardriver_chain_owner_right_20260608.window.png`
  - left report: `pcsx2_chardriver_chain_owner_left_seq_20260608.json`
  - left screenshot:
    `pcsx2_chardriver_chain_owner_left_seq_20260608.window.png`
  - both screenshots were active in-song at 60 FPS/VPS
  - both resolved the corrected callback source as
    `inner+0x1c -> 0x00b8be10 -> 0x00b8c170 -> vtable 0x003e3050`
  - vtable `0x003e3050 + 0x30` signed this-adjust was `-864`
  - vtable `0x003e3050 + 0x34` callback target was `0x0010c988`
  - adjusted callback `this` resolves back to `0x00b8be10`
  - `0x00b8be5c` names `char/glam1/og/glam1.milo`, and outer row
    `0x00b8c184` names `guitarist0`
  - rejected parallel left report:
    `pcsx2_chardriver_chain_owner_left_20260608.json`; it hit a PCSX2
    memory-card dialog because two PCSX2 instances were launched in parallel.
    Do not run PCSX2 trace processes in parallel.
- Static callback body dump:
  `ps2_function_snippets_20260608_callback_0010c988_full.json`
  - `0x0010c988` classifies incoming command/object through `0x002b7e50`
  - it lazily interns command symbols through `0x002d3a48`
  - resolved command names include `play`, `normal`, `idle`, `wail_on`,
    `extreme`, `wail_off`, `gtr_solo_on`, `solo`, `gtr_solo_off`,
    `band_jump`, `sync_jump`, `sync_wag`, `sync_head_bang`,
    `set_game_over`, `lose_teleport`, `active_players_changed`,
    `actually_walking`, `playing_starpower`, `playing_far_starpower`,
    `outro_complete`, and `player`
  - observed handler targets include `0x0010b7f8`, `0x0010c5b8`,
    `0x0010c730`, `0x0010b458`, `0x0010cfa0`, `0x0010d148`,
    `0x00184fd0`, `0x00171c68`, `0x0010c948`, and `0x001264f8`
- Static callback handler dump:
  `ps2_function_snippets_20260608_callback_handlers.json`
  - `0x0010b7f8` reaches `0x00171c68`, `0x00171f08`, `0x0010b7b0`, and
    `0x00171330`
  - `0x0010c5b8` resolves `normal` and uses the same object/ref/indirect
    pattern as `0x0010b7f8`
  - `0x0010c730` reaches `0x0010b9e8`, `0x00126098`, `0x00305b54`, and
    `0x00171330`
  - `0x0010c948` is a tiny wrapper around `0x00171c68`
  - `0x0010cfa0` resolves `singer` and `keyboard`, calls `0x003d8ea0` three
    times, and reaches `0x00190770` and `0x00162b30`
  - `0x00184fd0` calls `0x00171e08`
  - `0x00171c68` calls `0x002dc500` and is likely timing/clip math, but still
    needs runtime proof
- Static idle-branch downstream dump:
  `ps2_function_snippets_20260608_idle_branch_downstream.json`
  - `0x0010b7f8` uses object/ref helpers `0x002c1d50`, `0x002bb4f0`,
    `0x00101ec0`, `0x002c1df8`, and `0x002c2098`, plus indirect handlers
  - if selected state is nonzero, `0x0010b7f8` calls `0x00171c68`
  - then it calls `0x00171f08`, `0x0010b7b0`, and `0x00171330`
  - `0x0010b7f8` calls `0x00171330` with `a0=*(s1+0x244)`,
    `a1=v0` from `0x0010b7b0`, `a2=mode 1 or 4`, `f12=0x7149f2ca`, `f13=0`
  - `0x00171330` calls `0x002d1c00`, `0x00169560`, and `0x00198660`, then
    stores returned `v0` at `this+0x38`
  - `0x00171c68` walks a node chain, reads node `+0x18`, calls
    `0x002dc500`, and advances through node `+0x28`
  - `0x00171f08` wraps `0x00171ef0`
  - `0x0010b7b0` calls `0x001264f8` and `0x00112670`
- Accepted direct wrapper of callback vtable slot:
  - report: `pcsx2_chardriver_callback_0010c988_slot_20260608.json`
  - screenshot: `pcsx2_chardriver_callback_0010c988_slot_20260608.window.png`
  - screenshot state: active in-song stage view, 60 FPS/VPS
  - redirected live vptr `0x00b8c170` from table `0x003e3050` to copied table
    `0x01e80000`
  - wrapped slot `+0x34`, original `0x0010c988`
  - counter stayed zero for the two-second active window; treat this only as
    a phase-window negative sample, not proof the callback is generally
    inactive
- Accepted eight-second direct wrapper of callback vtable slot:
  - report: `pcsx2_chardriver_callback_0010c988_slot_8s_20260608.json`
  - after-Retry screenshot:
    `pcsx2_chardriver_callback_0010c988_slot_8s_20260608.after_retry.window.png`
  - after-Retry screenshot state: active in-song, 60 FPS/VPS
  - final screenshot:
    `pcsx2_chardriver_callback_0010c988_slot_8s_20260608.window.png`
  - final screenshot stayed in-song but showed `FPS:N/A` and low EE/VU/GS; use
    the trace with that status caveat
  - redirected live vptr `0x00b8c170` from `0x003e3050` to `0x01e80000`
  - wrapped slot `+0x34`, original `0x0010c988`
  - counter reached 1
  - last registers: `a0=0x01ffe6e0`, `a1=0x00b8be10`,
    `a2=0x00850c40`, `a3=0`
  - return `v0=0x01ffe6e0`; return rows include `+0x10=0x0084f440`,
    `+0x20=0x002b7e60`, `+0x40=0x00abeeb0`, `+0x60=0x00845f10`,
    and `+0x80=0x002c095c`
- Accepted follow-up sample of callback `a2` object:
  - report: `pcsx2_sample_callback_a2_00850c40_8s_20260608.json`
  - screenshot: `pcsx2_sample_callback_a2_00850c40_8s_20260608.window.png`
  - screenshot state: active in-song, 60 FPS/VPS
  - sampled `a2=0x00850c40` was stable over one second
  - rows: `+0x00=0x00850c50`, `+0x04=0x0044d630`,
    `+0x08=0x00010002`, `+0x10=0x00f1eaf0`,
    `+0x18=0x00dbca80`
  - object at `0x00850c50` had `+0x08=0x0054f31b`, ASCII `idle`,
    and `+0x0c=5`
  - combining runtime `a2` with static dispatcher maps the fired callback to
    the `idle` command branch, handler `0x0010b7f8`; this is a static-dispatch
    inference from captured command data, not an independent direct counter on
    `0x0010b7f8`
- `0x00171db0` selector clarification:
  - static body only follows active/scheduler object's `+0x28` chain when
    `+0x18 == 0`
  - accepted scheduler objects had `+0x18 = 0x3f800000` and `+0x28 =
    0`, so the node-chain layout was not exercised in that window
  - accepted early scheduler sample:
    `pcsx2_chardriver_scheduler_cells_early_20260608.json`
  - accepted early screenshot:
    `pcsx2_chardriver_scheduler_cells_early_20260608.window.png`
  - early sample used `--post-retry-seconds 0.1` and still kept
    `+0x18 = 0x3f800000` with `+0x28 = 0` across both scheduler objects
  - early sample's changing row was `+0x0c`, from `0xbeb6f8d0` to
    `0xc01f9b38`
  - accepted four-second scheduler sample:
    `pcsx2_chardriver_scheduler_cells_mid4_20260608.json`
  - accepted four-second screenshot:
    `pcsx2_chardriver_scheduler_cells_mid4_20260608.window.png`
  - four-second sample still kept `+0x18 = 0x3f800000` and `+0x28 = 0`;
    changing rows were `+0x0c`, `+0x10`, `+0x14`, `+0x1c`, and `+0x20`
  - accepted eight-second scheduler sample:
    `pcsx2_chardriver_scheduler_cells_mid8_20260608.json`
  - accepted eight-second screenshot:
    `pcsx2_chardriver_scheduler_cells_mid8_20260608.window.png`
  - eight-second sample still kept `+0x18 = 0x3f800000` and `+0x28 = 0`;
    changing rows were `+0x0c`, `+0x10`, `+0x14`, and `+0x1c`
  - eight-second sample showed pointer cells swapped scheduler objects:
    `0x00dbc9c4 -> 0x0076be90` and `0x00dbca64 -> 0x0076bb10`
  - treat scheduler pointer cells as live mutable state, not permanent
    right/left object identity
- Accepted large clip-source sample:
  `pcsx2_sample_clip_source_large_8s_20260608.json`
  - screenshot:
    `pcsx2_sample_clip_source_large_8s_20260608.window.png`, accepted active
    in-song at 60 FPS/VPS
  - shared source `0x00b8be10` has `+0x244 = 0x00daf090`; this is the live
    object passed as `a0` to `0x00171330` by the traced `idle` branch
  - changing rows were matrix/float-like rows around `0x00b8bf10..0x00b8bf84`
    and `0x00b8c048..0x00b8c050`
- Accepted idle-branch main driver sample:
  `pcsx2_sample_idle_branch_driver_00daf090_8s_20260608.json`
  - screenshot:
    `pcsx2_sample_idle_branch_driver_00daf090_8s_20260608.window.png`,
    accepted active in-song at 60 FPS/VPS
  - `0x00daf090` is named `main.drv` through `+0x68 = 0x00db5f43`
  - key rows: `+0x1c = 0x00b8be10`, `+0x28 = 0x00dbf290`,
    `+0x38 = 0x00768b90`, `+0x48` changed during the sample
  - rows after `+0x80` contain many bone `.trans` names, so this is the
    whole-body guitarist main driver path, not a hand driver
- Accepted main-driver scheduler sample:
  `pcsx2_main_driver_scheduler_8s_20260608.json`
  - screenshot:
    `pcsx2_main_driver_scheduler_8s_20260608.window.png`, accepted active
    in-song at 60 FPS/VPS
  - `main.drv + 0x38` pointed to scheduler object `0x00768b90`
  - this scheduler exercised the selector condition: `+0x18 = 0`,
    `+0x28 = 0x0076bd10`, `+0x2c = 0x00b8be10`
  - changed rows included `+0x0c`, `+0x4c..+0x60`, and `+0x8c..+0xa0`
  - this is the first accepted trace where `0x00171db0`'s node-chain case is
    represented by live runtime data
- Accepted main-driver node-chain sample:
  `pcsx2_main_driver_node_chain_8s_20260608.json`
  - screenshot:
    `pcsx2_main_driver_node_chain_8s_20260608.window.png`, accepted active
    in-song at 60 FPS/VPS
  - followed scheduler cell `0x00768bb8` to node `0x0076bd10`
  - node rows: `+0x00 = 0x00012001`, `+0x18 = 0x3f800000`,
    `+0x24 = 0x00e0d150`, `+0x28 = 0`, `+0x2c = 0x00b8be10`,
    `+0x30 = 0xf149f2ca`, `+0x34 = 0xffffffff`,
    `+0x38 = 0x00843cf0`, `+0x3c = 0x00000010`
  - node rows `+0x40..+0x7c` are a dense run of EE RAM pointers from
    `0x00847e80` down through `0x00846ee0`; these are the next child targets
    to sample for clip/sample layout
  - only node rows `+0x0c`, `+0x10`, `+0x14`, and `+0x1c` changed
  - candidate next cell `0x0076bd38` was zero, so this traced chain terminates
    at `0x0076bd10` in the accepted window
- Accepted main-driver node-child sample:
  `pcsx2_main_driver_node_children_iso_state1_8s_20260608.json`
  - command used the real ISO
    `C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso`,
    indexed state `1`, posted retry key `0x4c`, and an eight-second active
    gameplay settle before sampling
  - screenshot:
    `pcsx2_main_driver_node_children_iso_state1_8s_20260608.window.png`,
    accepted active in-song at Battle of the Bands
  - sampled node child targets from accepted node `0x0076bd10`:
    `0x00843cf0`, `0x00847e80`, `0x00848dd0`, `0x00847cf0`,
    `0x00847b60`, `0x008479d0`, `0x00847840`, and `0x008476b0`
  - all eight sampled targets were stable in the one-second window
  - `0x00843cf0` is command/event-like and contains repeated symbols
    `wail_off`, `wail_on`, and `HandMap_DropD2`
  - the other sampled children share a repeated structural layout with leading
    EE pointers, identity float rows, table-like pointers `0x003e8b08` and
    `0x003e6d88`, self-list pointer pairs, and no row changes in the short
    sample
  - this is accepted structural evidence, not proof of live pose output; the
    next trace should follow child leading/list pointers and table slots
- Accepted main-driver node-child next-layer sample:
  `pcsx2_main_driver_node_child_next_layer_iso_state1_8s_20260608.json`
  - screenshot:
    `pcsx2_main_driver_node_child_next_layer_iso_state1_8s_20260608.window.png`,
    accepted active in-song at Battle of the Bands
  - followed first two pointer/list targets from the accepted child layer
  - most targets repeated the same container/list pattern, with table-like
    values `0x003e8a58`, `0x003f36d0`, `0x003e8ad8`, `0x003e8b08`, and
    `0x003e6d88`
  - `child38_p0` at `0x00843e10` expanded command/event symbols: `wail_on`,
    `solo_on`, `wail_off`, and `solo_off`
  - only changed row: `child44_p1 + 0x58` at `0x00848fb0`, `0` ->
    `0x00ac88d0`
  - this is accepted structural/phase evidence, not yet pose output; next
    trace should classify the table-like values and follow `0x00ac88d0` plus
    deeper leading pointers like `0x00848170`, `0x00848198`, `0x008490c0`,
    and `0x008490e8`
- Static classification, no emulator setup required:
  - table dump: `ps2_static_node_child_tables_20260610.json`
  - snippet dump: `ps2_function_snippets_20260610_node_child_tables.json`
  - `0x003e8a58`, `0x003e8ad8`, `0x003e8b08`, `0x003e6d88`, and
    `0x003f36d0` are real static dispatch/vtable structures
  - the `0x003e8a58` / `0x003e8ad8` family reaches `0x00351060`, which
    registers class string `6PsMesh`
  - `0x001c6398` / `0x001c6490` call transform/world helpers such as
    `0x003d8ea0`, so this layer is currently best classified as
    mesh/scene-graph and transform/reference plumbing
  - do not label this node-child branch as `CharClipSamples` output without a
    later accepted live bridge
- Rejected deeper samples due to missing screenshot evidence:
  - `pcsx2_main_driver_node_child_deeper_iso_state1_8s_20260610.json`
  - `pcsx2_main_driver_node_child_deeper_iso_state1_8s_gui_20260610.json`
  - `pcsx2_main_driver_node_child_deeper_iso_state1_8s_capture_20260610.json`
  These showed plausible stable rows, but screenshot capture returned `None`.
  Keep them out of the accepted evidence chain.
- Screenshot-gated capture restored on 2026-06-10 without changing PCSX2 setup:
  - diagnostic capture:
    `pcsx2_capture_diag_20260610.0x1707d4.print_0x2.png`
  - gate sample:
    `pcsx2_capture_gate_sample_20260610.json`
  - gate screenshot:
    `pcsx2_capture_gate_sample_20260610.window.png`
  - command used the accepted real ISO plus indexed `-state 1`, `--gui`,
    child-window posted `0x4c`, and `--require-screenshot`
  - screenshot state: active Battle of the Bands gameplay, 60 FPS/VPS
  - this proves the current trace path can again produce accepted PCSX2 HWND
    screenshots without foreground forcing
- Accepted replacement deeper node-child sample:
  - report:
    `pcsx2_main_driver_node_child_deeper_iso_state1_screenshot_20260610.json`
  - screenshots:
    `pcsx2_main_driver_node_child_deeper_iso_state1_screenshot_20260610.before_sample.window.png`
    and
    `pcsx2_main_driver_node_child_deeper_iso_state1_screenshot_20260610.window.png`
  - screenshot state: active Battle of the Bands gameplay, 60 FPS/VPS
  - sampled `0x00ac88d0`, `0x00848170`, `0x00848198`, `0x008490c0`,
    `0x008490e8`, `0x00847fe0`, `0x00848008`, `0x00847cc0`, and
    `0x00847ce8`
  - `0x00ac88d0` changed six rows in one second: pointer/list cells
    `+0x18`, `+0x1c`, and `+0xc8`, float-like rows `+0x54` and `+0x94`,
    and small count/state row `+0xc4`
  - `0x00848198` changed one float-like row at `+0xdc`
  - the other sampled `0x003e8a58` / `0x003e8ad8` containers were stable in
    this short window
  - this replaces the rejected no-screenshot deeper samples for these targets
- Accepted dynamic pointer follow-up:
  - report: `pcsx2_node_child_dynamic_pointer_targets_20260610.json`
  - screenshots:
    `pcsx2_node_child_dynamic_pointer_targets_20260610.before_pointer.window.png`
    and `pcsx2_node_child_dynamic_pointer_targets_20260610.window.png`
  - screenshot state: active Battle of the Bands gameplay, 60 FPS/VPS
  - followed changing cells from `0x00ac88d0`:
    `+0x18` (`0x00ac88e8`), `+0x1c` (`0x00ac88ec`), and
    `+0xc8` (`0x00ac8998`)
  - `+0x18` rotated through `0x0074bdc0`, `0x00848740`, and `0x00848d50`
    with 53 changing rows in the pointed objects
  - `+0x1c` rotated through `0x007424c0`, `0x00853bd0`, `0x00820a20`, and
    `0x0073c420` with 61 changing rows; one first-sample row referenced
    ASCII `intro_start_msg`
  - `+0xc8` rotated through `0x00853870`, `0x008536e0`, and `0x00854150`
    with 59 changing rows
  - interpretation: this branch is a live rotating/list state path feeding
    mesh/scene graph objects, not a fixed asset record and not yet proven
    `CharClipSamples` output
- CharClipSamples static/runtime anchor found on 2026-06-10:
  - live string/ref scan:
    `pcsx2_live_strings_clip_refs_20260610.json`
  - screenshot:
    `pcsx2_live_strings_clip_refs_20260610.window.png`, accepted active
    Battle of the Bands gameplay
  - context sample:
    `pcsx2_clip_refs_context_20260610.json`
  - screenshot:
    `pcsx2_clip_refs_context_20260610.window.png`, accepted active gameplay
  - first live refs to the `CharClipSamples` string were metadata/registry
    rows, not live sample/apply output
  - `0x00716dc0` pairs `CharClipSamples` with factory/code pointer
    `0x00335e00`
  - static factory dump:
    `ps2_function_snippets_20260610_clip_ref_functions.json`
  - `0x00335e00` allocates `0x3a0` bytes and calls constructor
    `0x0016ad98(this, 1)`
  - constructor dumps:
    `ps2_function_snippets_20260610_clip_constructor_0016ad98.json` and
    `ps2_function_snippets_20260610_clip_constructor_long.json`
  - `0x0016ad98` installs class/vtable pointer `0x003e70b0` and initializes
    subblocks at `this+0x84`, `this+0x138`, `this+0x1ec`, and `this+0x2a0`
  - old trace helper label `gameplay_hair_anim_b` for table `0x003e70b0` was
    a bad guess; treat `0x003e70b0` as `CharClipSamples` until a later trace
    proves a narrower live role
- Clip scheduler/blend static continuation:
  - `0x00198660` initializes a scheduler/blend entry from source `a2=s1`
  - stored fields include `s0+0x00 = *(s1+0x28)`,
    `s0+0x04 = *(s1+0x30)`, `s0+0x08 = 1.0`, `s0+0x24 = s1`,
    `s0+0x28 = t0`, `s0+0x2c = a1`, `s0+0x30 = 0xf149f2ca`, and
    `s0+0x34 = -1`
  - this extends the scheduler/blend path; it is not yet the final
    `CharClipSamples` sample/apply output
- Accepted direct scheduler function trace:
  - tool: `tools/trace_pcsx2_animation_calls.py`
  - report: `pcsx2_direct_scheduler_calls_20260610.json`
  - screenshot: `pcsx2_direct_scheduler_calls_20260610.window.png`
  - screenshot state: active in-song Battle of the Bands capture, but slowed
    by interpreter/direct-function probing; use it as call/argument evidence,
    not as a normal-speed timing sample
  - the helper temporarily used interpreter mode for the probe and restored
    `C:\Games\Emulators\PCSX2\inis\PCSX2.ini` afterward; `EnableEE = true`
    was verified after the run
  - patched direct entry points:
    - `0x00171248` (`scheduler_push_00171248`), original words
      `0x27bdffa0`, `0x0000182d`
    - `0x00171330` (`scheduler_sibling_00171330`), original words
      `0x27bdff90`, `0x7fb40010`
    - `0x00198660` (`blend_entry_00198660`), original words
      `0x27bdffb0`, `0x3c013f80`
  - captured runtime calls:
    - `0x00171248`: 11 calls, last args `a0=0x010f66b0`,
      `a1=0x01021e50`, `a2=0x00000234`, `a3=0x003e0000`
    - `0x00171330`: 1 call, last args `a0=0x00daf090` (`main.drv`),
      `a1=0x00dc77c0`, `a2=0x00000204`, `a3=0x00000010`
    - `0x00198660`: 36 calls, last args `a0=0x0076bc50`,
      `a1=0x00b8da50`, `a2=0x01021e50`, `a3=0x00000234`
  - interpretation: the upstream scheduler/blend writer path is now live
    trace-backed. Continue from these argument objects; do not fall back to
    guessing from `CharClipSamples` vtable slots.
- Accepted normal-speed scheduler argument object sample:
  - report: `pcsx2_scheduler_args_objects_20260610.json`
  - screenshots:
    `pcsx2_scheduler_args_objects_20260610.before_sample.window.png` and
    `pcsx2_scheduler_args_objects_20260610.window.png`
  - screenshot state: active in-song Battle of the Bands gameplay at
    60 FPS/VPS
  - sampled direct-trace argument objects for two seconds:
    `0x0076bc50`, `0x00b8da50`, `0x01021e50`, `0x00daf090`,
    `0x00dc77c0`, and `0x010f66b0`
  - `0x0076bc50` matches the static `0x00198660` scheduler/blend entry
    layout: `+0x00=0x00000234`, `+0x04=1.0`, `+0x08=1.0`,
    changing float rows at `+0x0c`, `+0x10`, `+0x14`, `+0x18`,
    `+0x1c`, and `+0x20`, source pointer `+0x24=0x0101b5a0`,
    previous/next-like pointer `+0x28=0x0076b9d0 -> 0`,
    `+0x2c=0x00b8ca60`, sentinel `+0x30=0xf149f2ca`, and
    `+0x34=0xffffffff`; the same entry pattern repeats at `+0x40`
  - `0x00b8da50` was stable in the sample and contains pointer rows plus
    ASCII `char/crowd/og/crowd_female04.milo`; do not assume this particular
    sample is the guitarist clip source without a later bridge
  - `0x01021e50` was stable and contains ASCII `{$dude set_hand clap}`;
    treat it as a source/event object captured from the last direct call
  - `0x00daf090` (`main.drv`) changed `+0x38` from `0x00768b90` to
    `0x0076b9d0` and changed timing row `+0x48`; this proves `main.drv +
    0x38` is a live scheduler/blend pointer cell
  - `0x010f66b0` changed `+0x38` from `0x00768b10` to `0x007989d0`;
    `+0x1c=0x00b8da50`, so this object is another live driver-like
    scheduler owner from `0x00171248`
  - interpretation: the next runtime trace should follow the live
    scheduler/blend entry chain and downstream output from `0x0076bc50`,
    `0x00daf090+0x38`, and `0x010f66b0+0x38`, not restart at setup.
- Accepted normal-speed scheduler pointer-chain follow-up:
  - report: `pcsx2_scheduler_pointer_chain_20260610.json`
  - screenshots:
    `pcsx2_scheduler_pointer_chain_20260610.before_pointer.window.png` and
    `pcsx2_scheduler_pointer_chain_20260610.window.png`
  - screenshot state: active Battle of the Bands gameplay at 60 FPS/VPS
  - followed cells:
    - `main_drv_scheduler_cell=0x00daf0c8`
    - `scheduler_entry_prevnext_cell=0x0076bc78`
    - `scheduler_entry_source_cell=0x0076bc74`
    - `scheduler_entry_target_cell=0x0076bc7c`
    - `sched_push_owner_cell=0x010f66e8`
  - `main.drv+0x38` rotated through `0x00768b90`, `0x00768bd0`, and
    `0x0076bd10`, with 58 changed rows in the pointed objects; row `+0x28`
    traversed scheduler/node links including `0x00768bd0` and `0x0076bd10`
  - `0x0076bc50+0x28` (`0x0076bc78`) rotated from `0` to
    `0x0076bbd0`; the pointed object changed timing/float rows and overlaps
    the previously captured scheduler/blend entry neighborhood
  - `0x0076bc50+0x24` stayed on `0x0101b5a0`; that source object contains
    ASCII `{ $dude 'set_hand' 'clap' }` and changing pointer rows around
    `+0xec..+0xfc`
  - `0x0076bc50+0x2c` stayed on `0x00b8cf50`; that target object was stable
    in the short window and names `char/crowd/og/crowd_female02.milo`, so do
    not assume it is the active guitarist clip target without another bridge
  - `0x010f66b0+0x38` rotated through `0x007989d0` and `0x00770fd0`, with
    51 changed rows in pointed scheduler/blend objects
  - interpretation: scheduler/blend pointers are live rotating chains. Do not
    treat `driver+0x38` or `entry+0x28` as stable identity fields.
- Accepted direct callback/handler function trace:
  - tool: `tools/trace_pcsx2_animation_calls.py`
  - report: `pcsx2_callback_handlers_direct_20260610.json`
  - screenshot: `pcsx2_callback_handlers_direct_20260610.window.png`
  - screenshot state: active in-song Battle of the Bands capture, slowed by
    interpreter/direct-function probing; `EnableEE = true` was verified after
    the run
  - patched direct entry points:
    `0x0010c988`, `0x0010b7f8`, `0x0010c5b8`, `0x0010c730`,
    `0x0010c948`, `0x0010cfa0`, `0x0010d148`, `0x00184fd0`,
    `0x00171c68`, `0x00171f08`, `0x0010b7b0`, and `0x00171330`
  - nonzero calls:
    - dispatcher `0x0010c988`: 8 calls, last args
      `a0=0x01ffe300`, `a1=0x00b8be10`, `a2=0x00846230`, `a3=0`
    - wrapper/handler `0x0010c948`: 4 calls, last args
      `a0=0x00b8be10`, `a1=0x00004000`, `a2=0x005d2498`,
      `a3=0x00000010`
    - helper `0x00171c68`: 520 calls, last args
      `a0=0x00daf090`, `a1=0x00800000`, `a2=0x00dbf244`,
      `a3=0x00dd0e64`
    - helper `0x0010b7b0`: 1 call, last args
      `a0=0x00b8be10`, `a1=0x0010ac90`, `a2=0x00b8bfb0`,
      `a3=0x00490000`
    - scheduler sibling `0x00171330`: 1 call, last args
      `a0=0x00daf090`, `a1=0x00dc77c0`, `a2=0x00000204`,
      `a3=0x00000010`
  - zero-call handlers in this window:
    `0x0010b7f8`, `0x0010c5b8`, `0x0010c730`, `0x0010cfa0`,
    `0x0010d148`, `0x00184fd0`, and `0x00171f08`
  - interpretation: the callback dispatcher and a real handler/wrapper path
    are now runtime-proven. The next trace should sample dispatcher
    `a2=0x00846230`, wrapper `a2=0x005d2498`, and helper inputs
    `0x00dbf244` / `0x00dd0e64` under normal speed.
- Accepted normal-speed callback argument object sample:
  - report: `pcsx2_callback_arg_objects_20260610.json`
  - screenshots:
    `pcsx2_callback_arg_objects_20260610.before_sample.window.png` and
    `pcsx2_callback_arg_objects_20260610.window.png`
  - screenshot state: active Battle of the Bands gameplay at 60 FPS/VPS
  - dispatcher `a2=0x00846230` changed three rows:
    - `+0x50` rotated through EE pointers including `0x00747a30`,
      `0x00848860`, `0x00850cf0`, `0x00821130`, `0x008487a0`,
      `0x00850110`, `0x00c9b610`, and `0x00853890`
    - `+0x54` rotated through `0x00848d80`, `0x008487a0`,
      `0x01ffe824`, `0x00821130`, `0x00853560`, and `0x00853d40`
    - `+0x58` rotated through mixed values including code/small values; do
      not treat it as a stable object pointer
  - wrapper `a2=0x005d2498` was stable rodata/text in this window, not a live
    animation object
  - helper `a2=0x00dbf244` was stable and identified a right-side weight/clip
    structure: rows include `+0x30` ASCII `right.weight`, links back to
    `0x00b8be10` and `0x00daf090`, and table-like values
    `0x003e8228`, `0x003e82e8`, `0x003e8268`, and `0x003e7ea0`
  - helper `a3=0x00dd0e64` was stable and contains transform/list rows with
    `0x003e6d88` and repeated `0x003e5830` references
  - `0x0010b7b0` `a2=0x00b8bfb0` changed five rows, including timing/float
    rows `+0x98`, `+0x9c`, `+0xa0`, and state row `+0xd4` toggling
    `0x00dc77c0 -> 0x00dc5fb0`
  - shared source `0x00b8be10` changed 19 pose/matrix-like rows around
    `+0x100..+0x170` and names `char/glam1/og/glam1.milo`
  - `main.drv=0x00daf090` changed `+0x38` through the known scheduler chain
    and `+0x48` timing/phase row
- Accepted dispatcher `a2` pointer-target follow-up:
  - report: `pcsx2_dispatcher_a2_pointer_targets_20260610.json`
  - screenshots:
    `pcsx2_dispatcher_a2_pointer_targets_20260610.before_pointer.window.png`
    and `pcsx2_dispatcher_a2_pointer_targets_20260610.window.png`
  - screenshot state: active Battle of the Bands gameplay at 60 FPS/VPS
  - dispatcher `+0x50` rotated through `0x00846660`, `0x00848860`,
    `0x00850cf0`, `0x008487a0`, `0x00850ce0`, and `0x00853890`
  - dispatcher `+0x54` rotated through `0x00848d80`, `0x008487a0`,
    `0x01ffe824`, `0x008541a0`, `0x00850cf0`, and `0x00851270`
  - dispatcher `+0x58` rotated through mixed code/small/EE values including
    `0x00102d50`, `0x03102d50`, `0x02802de0`, `2`, `3`, and `0x00c505a0`;
    treat this as a tagged/mixed field until the dispatcher body is mapped
  - helper state cell `0x00b8c084` rotated through `0x00dc77c0` and
    `0x00dc5fb0`; first rows from `0x00dc77c0` include table
    `0x003e6fe0` and ASCII `idle`
  - interpretation: the callback path is now connected to live command/list
    objects and phase-dependent clip/state records. The next direct-function
    trace should follow helper `0x00171c68` into `0x002dc500` and scheduler
    update functions in the same active window.
- Accepted direct scheduler/helper function trace:
  - tool: `tools/trace_pcsx2_animation_calls.py`
  - report: `pcsx2_scheduler_helper_direct_20260610.json`
  - screenshot: `pcsx2_scheduler_helper_direct_20260610.window.png`
  - screenshot state: active in-song Battle of the Bands capture, slowed by
    interpreter/direct-function probing; `EnableEE = true` was verified after
    the run
  - patched direct entry points:
    `0x00171830`, `0x00171db0`, `0x00171248`, `0x00171330`,
    `0x00198660`, `0x00171c68`, `0x002dc500`, `0x00199000`,
    `0x00198a48`, and `0x00196888`
  - nonzero calls:
    - `0x00171830`: 3655 calls, last args `a0=0x0135cb90`,
      `a1=0x00171830`, `a2=0x0135cba4`, `a3=0x01333ee0`
    - `0x00171db0`: 22 calls, last args `a0=0x0135cb90`,
      `a1=1`, `a2=0x003fffff`, `a3=0x01333960`
    - `0x00171248`: 11 calls, last args `a0=0x010f66b0`,
      `a1=0x01021e50`, `a2=0x00000234`, `a3=0x003e0000`
    - `0x00171330`: 1 call, last args `a0=0x00daf090`,
      `a1=0x00dc77c0`, `a2=0x00000204`, `a3=0x00000010`
    - `0x00198660`: 36 calls, last args `a0=0x0076ba10`,
      `a1=0x00b8da50`, `a2=0x01021e50`, `a3=0x00000234`
    - `0x00171c68`: 516 calls, last args `a0=0x00daf090`,
      `a1=0x00800000`, `a2=0x00dbf244`, `a3=0x00dd0e64`
    - `0x002dc500`: 96768 calls, last args `a0=0x006f61b0`,
      `a1=0x006f60a0`, `a2=0x42f00000`, `a3=0x42f00000`
    - `0x00199000`: 36 calls, last args `a0=0x0076ba10`,
      `a1=0xb6db0000`, `a2=0x0076ba10`, `a3=0x00000200`
    - `0x00198a48`: 32 calls, last args `a0=0x0076ba10`,
      `a1=3`, `a2=0x010f66c4`, `a3=0x01097c90`
    - `0x00196888`: 14 calls, last args `a0=0x01021e50`,
      `a1=0x01021e50`, `a2=0x00000234`, `a3=1`
  - interpretation: `0x00171830`, selector `0x00171db0`,
    scheduler push/sibling, blend entry initialization, scheduler helper,
    release, and related source helper are now tied together by direct runtime
    counts. `0x002dc500` is extremely hot and must be treated as shared math,
    not a character-only writer, until caller-specific samples prove otherwise.
- Accepted normal-speed scheduler/helper argument object sample:
  - report: `pcsx2_scheduler_helper_arg_objects_20260610.json`
  - screenshots:
    `pcsx2_scheduler_helper_arg_objects_20260610.before_sample.window.png`
    and `pcsx2_scheduler_helper_arg_objects_20260610.window.png`
  - screenshot state: active Battle of the Bands gameplay at 60 FPS/VPS
  - `0x0135cb90` has the same driver shape as `main.drv`: table rows
    `0x003e81b0`, `0x003e7380`, `0x003e7440`, `0x003e3230`,
    scheduler pointer row `+0x38`, name row ASCII `main.drv`, and source
    pointer `+0x6c=0x00b902e0`; changed rows were `+0x38`, `+0x40`,
    `+0x44`, and `+0x48`
  - `0x0135cb90+0x38` rotated through `0x00768a50`, `0x0076bcd0`,
    `0x0076bad0`, and `0x00768c50`
  - `0x0076ba10` is an active scheduler/blend entry; rows changed at
    `+0x00`, timing/weight rows `+0x0c..+0x20`, pointer/link row
    `+0x28`, and symbol/timing rows after `+0x90`
  - `0x0076ba10+0x24=0x011b24c0`, `+0x28=0 -> 0x0076bc10`, and
    `+0x2c=0x00b8f2f0`
  - `0x0076ba10+0x90` changed symbols from `click_hat`/`click_kick` to
    strum names including `strum_pick_01`
  - `0x006f61b0` and `0x006f60a0`, the sampled `0x002dc500` args, were
    stable math/lookup buffers in this window; this supports treating
    `0x002dc500` as shared math, not a pose object
  - `0x010f66c4` has the same driver-like layout as the scheduler owner, with
    `+0x38` changing `0x007989d0 -> 0x0076bc50` and `+0x48` changing as a
    timing/phase row
  - `0x01021e50` was stable and still names `{ $dude set_hand clap }`
  - interpretation: the scheduler/blend entries combine timing/weight fields,
    linked entries, source/target objects, and command/event symbols. The next
    trace should follow `0x0076ba10+0x24`, `+0x28`, `+0x2c`, and the
    driver-like `0x0135cb90+0x38` chain.
- Accepted blend-entry pointer follow-up:
  - report: `pcsx2_blend_entry_pointer_followup_20260610.json`
  - screenshots:
    `pcsx2_blend_entry_pointer_followup_20260610.before_pointer.window.png`
    and `pcsx2_blend_entry_pointer_followup_20260610.window.png`
  - screenshot state: active Battle of the Bands gameplay at 60 FPS/VPS
  - `0x0135cb90+0x38` rotated through `0x00768a50`, `0x0076bd10`,
    `0x0076bad0`, and `0x00768c50`; those entries reference source/target
    pointers including `0x013bb500`, `0x013bc010`, `0x00b902e0`, and
    `0x00b8f7e0`
  - `0x0135cb90+0x40` rotated through `0x013bb870`, `0x005f7b74`, and
    `0x013bc380`
  - the live `0x013bb870` object starts with vtable `0x003e70b0`, table
    helper `0x003f36d0`, name ASCII `drummer_idle`, and owner/source pointer
    `0x01359cc0`; this is a live bridge from the driver current field back to
    the `CharClipSamples` table, not just a metadata string scan
  - `0x0076ba10+0x24` stayed on source `0x01021340`, which names
    `{ $dude 'set_hand' 'clap' }` and has changing pointer rows around
    `+0xec..+0xfc`
  - `0x0076ba10+0x28` toggled `0x007989d0 -> 0`, and when nonzero the pointed
    entry has the same scheduler/blend layout with source `0x01021340` and
    target `0x00b8da50`
  - `0x0076ba10+0x2c` stayed on `0x00b8da50`, which names
    `char/crowd/og/crowd_female04.milo`; keep this as a target object in this
    event window, not a guitarist identity
  - `0x010f66b0+0x38` rotated through `0x0076ba10` and `0x007989d0`
  - `0x0076ba10+0x90` rotated through symbols `click_hat`,
    `strum_long_01`, and `strum_pick_01`
  - interpretation: the scheduler/blend entry now has a trace-backed field
    map: `+0x24` source event/clip object, `+0x28` linked scheduler/blend
    entry, `+0x2c` target object, and later symbol/timing rows. A driver
    current field can point directly to `0x003e70b0` `CharClipSamples` objects
    such as `drummer_idle`.
- Accepted targeted current `CharClipSamples` vtable trace:
  - report: `pcsx2_live_charclipsamples_current_vtable_20260610.json`
  - screenshots:
    `pcsx2_live_charclipsamples_current_vtable_20260610.after_retry.window.png`
    and `pcsx2_live_charclipsamples_current_vtable_20260610.window.png`
  - screenshot state: active Battle of the Bands gameplay at 60 FPS/VPS
  - redirected exact live vptr cells `0x013bb870` and `0x013bc380`, both
    expected/live-before table `0x003e70b0`
  - nonzero slot:
    - table `0x003e70b0`, slot index `25`, offset `0x64`,
      original `0x002c0e28`, count `5`
    - last args: `a0=0x013bc380`, `a1=0x007edb40`,
      `a2=0`, `a3=0x013bc380`
  - all other traced slots through `0xd4` were zero in this eight-second
    focused window
  - interpretation: targeted live `CharClipSamples` current objects do have
    active vtable dispatch, but the hit slot is an accessor/owner-source path,
    not the sample/apply loop.
- Accepted slot `0x64` argument sample and static snippet:
  - runtime report: `pcsx2_charclipsamples_slot64_args_20260610.json`
  - screenshots:
    `pcsx2_charclipsamples_slot64_args_20260610.before_sample.window.png`
    and `pcsx2_charclipsamples_slot64_args_20260610.window.png`
  - static snippets:
    `ps2_function_snippets_20260610_charclipsamples_slot64.json` and
    `ps2_function_snippets_20260610_charclipsamples_slot64_cont.json`
  - `0x013bc380` is stable in this window and names
    `drummer_active_medium_nosnare`; rows include:
    `+0x00=0x003e70b0`, `+0x08=0x003f36d0`,
    `+0x10=this`, `+0x14=name`, `+0x18=0x01359cc0`
  - `0x013bb870` is stable and names `drummer_idle`, with the same header
    shape and `+0x18=0x01359cc0`
  - `a1=0x007edb40` is a stable event/list structure with symbols including
    `<unnamed>`, `hit_hihat`, and `handle`; do not treat it as bone output
  - owner/source `0x01359cc0` changed one pointer row:
    `+0xa8` `0x007ead20 -> 0x007ea640`
  - static `0x002c0e28` reads `this+0x18`. If nonzero it returns that
    pointer through the branch target at `0x002c0e3c`; if null it returns a
    default/global value from the `0x003e` data region.
  - interpretation: `CharClipSamples + 0x64` returns the owning/source object
    pointer (`0x01359cc0` for these drummer clips). It is not the clip sampling
    or pose application function.
- Accepted direct `CharClipSamples` sample/eval candidate trace:
  - report: `pcsx2_charclipsamples_candidate_direct_20260610.json`
  - screenshot:
    `pcsx2_charclipsamples_candidate_direct_20260610.window.png`
  - screenshot state: active Battle of the Bands gameplay; slowed by
    interpreter/direct-function probing; `EnableEE = true` was verified after
    the run
  - patched direct entry points:
    `0x0016a788`, `0x0016a7e0`, `0x0016a868`, `0x0016b0b0`,
    `0x0016b128`, `0x0016b1d0`, `0x0016b240`, and `0x0016b2f0`
  - nonzero calls:
    - `0x0016b0b0`: 1234 calls, last `a0=0x0115da10`
    - `0x0016b128`: 2468 calls, last `a0=0x0115da10`
    - `0x0016b1d0`: 5436 calls, last `a0=0x013bb500`,
      `a1=0x0135ccec`
    - `0x0016b240`: 675 calls, last `a0=0x00fcde00`,
      `a1=0x00dbdd20`, `a2=0x0073e104`
    - `0x0016b2f0`: 9315 calls, last `a0=0x013bb500`,
      `a1=0x0135ccec`, `a3=0x013c500c`
  - zero-call loader/constructor-like entries in this gameplay window:
    `0x0016a788`, `0x0016a7e0`, and `0x0016a868`
  - interpretation: `0x0016b1d0` and `0x0016b2f0` are now the first accepted
    direct runtime bridge from live `CharClipSamples`-shaped objects into
    sampling/evaluation, not class metadata or an accessor slot.
- Accepted normal-speed sample/eval argument sample:
  - report: `pcsx2_charclipsamples_candidate_args_20260610.json`
  - screenshot:
    `pcsx2_charclipsamples_candidate_args_20260610.window.png`, active
    Battle of the Bands gameplay at 60 FPS/VPS
  - eval context `0x013bb500` starts with current clip pointer
    `0x013bb870` (`drummer_idle`) and has moving pointer rows around
    `+0xec..+0x110`
  - count/eval context `0x0115da10` has the same moving pointer pattern in
    a separate sample set
  - `0x0073e104` contains bone channel names including
    `bone_lip-L-corner.pos`, `bone_L-brow1.quat`, and `bone_jaw.quat`
  - `0x00747504` later identifies a whole-body channel list including
    `bone_pelvis.pos`, ankle/clavicle/hand/thigh/thumb/upperArm quats,
    head/spine quats, forearm/knee/toe rotz, and `bone_pos_mic.pos`
- Accepted eval pointer-target follow-up:
  - report: `pcsx2_charclipsamples_eval_pointer_targets_20260610.json`
  - screenshot:
    `pcsx2_charclipsamples_eval_pointer_targets_20260610.window.png`,
    active Battle of the Bands gameplay at 60 FPS/VPS
  - eval context cells at `0x013bb5ec`, `0x013bb5f0`, `0x013bb5f8`, and
    `0x013bb604` rotated through contiguous packed sample records such as
    `0x013c8a00`, `0x013c8a10`, `0x013c8a80`, and `0x013c8a8c`
  - count context cells at `0x0115dafc`, `0x0115db00`, `0x0115db08`, and
    `0x0115db14` rotated through analogous packed sample records in the
    `0x011d66c0..0x011db30e` range
  - `0x013bb870+0xb0` points to another live `CharClipSamples` object
    `0x013bbc20`, named `drummer_active_medium_half`; its float rows
    changed over the sample
  - interpretation: the moving rows are packed sample/value records, not
    vtable objects. The clip object points into active neighboring clip
    objects, while the eval context rotates source record pointers.
- Accepted helper/final-apply direct trace:
  - report: `pcsx2_charclipsamples_helper_direct_20260610.json`
  - screenshot:
    `pcsx2_charclipsamples_helper_direct_20260610.window.png`
  - screenshot state: active Battle of the Bands gameplay; slowed by
    interpreter/direct-function probing; `EnableEE = true` was verified after
    the run
  - nonzero calls:
    - `0x0016b1d0`: 5500 calls, last `a0=0x013bb500`,
      `a1=0x0135ccec`
    - `0x0016b2f0`: 9381 calls, last `a0=0x013bb500`,
      `a1=0x0135ccec`, `a3=0x013c500c`
    - `0x00167fd8`: 21952 calls, last `a0=0x013bb7a0`,
      `a1=0x0135ccec`
    - `0x00193cb0`: 25344 calls, last `a0=0x013bb584`,
      `a1=0x0135ccec`, `a2=0x20`
    - `0x00193d18`: 2046 calls, last `a0=0x00fcdfec`,
      `a1=0x00dbdd20`, `a2=0x0073e154`
    - `0x00193d78`: 9381 calls, last `a0=0x013bb584`,
      `a1=0x0135ccec`, `a2=0x007ea62c`
    - `0x00193e18`: 9381 calls, last `a0=0x013bb6ec`,
      `a1=0x0135ccec`, `a2=0x00747504`
    - `0x0016ab88`: 9381 calls, last `a0=0x013bb7a0`,
      `a1=0x0135ccec`, `a2=0x00747504`, `a3=0x013c620c`
    - `0x00167d98`: 25566 calls, last `a0=0x013bb5ec`,
      `a1=0x013c6180`
    - `0x00168320`: 24875 calls, last `a0=0x013bb584`,
      `a1=0x0135ccec`
  - static snippets:
    `ps2_function_snippets_20260610_charclipsamples_hot_long.json` and
    `ps2_function_snippets_20260610_charclipsamples_apply_helpers.json`
  - interpretation: `0x0016b2f0` fans into helper lookups, then
    `0x0016ab88`, and `0x00168320` performs float/quaternion-like row writes.
    This is the strongest accepted sample/apply chain so far.
- Accepted final-apply argument sample:
  - report: `pcsx2_charclipsamples_final_apply_args_20260610.json`
  - screenshot:
    `pcsx2_charclipsamples_final_apply_args_20260610.window.png`, active
    Battle of the Bands gameplay at 60 FPS/VPS
  - final/apply `a1=0x0135ccec` is a stable destination/list descriptor whose
    rows point to live sample/output records such as `0x013bbc60`,
    `0x013bbc70`, and `0x013bbd50`
  - final/apply `a2=0x00747504` is the whole-body bone channel list
  - final/apply `a3=0x013c620c` is a packed source/value record
  - upstream helper base `0x013bb584` has channel/list fields and rotating
    record pointers at `+0x68..+0x8c`
  - static `0x00167d98` adds record offsets into a base pointer and writes ten
    pointer cells starting at `a0+0x68`
  - static `0x0016ab88` reads `s0+0x94`, `+0x98`, `+0x9c`, `+0xa0`,
    calls `0x001938f8`, writes float rows back to `s0+0xa0` and `s1+4/+8`,
    calls `0x002ffd88` and `0x002dc500`, then calls `0x00168320`
  - static `0x00168320` reads channel/list rows and writes repeated 4-float
    rows through destination pointers (`swc1` to `a3+0`, `+4`, `+8`, `+0xc`)
  - interpretation: this proves the sample/apply boundary and channel-list
    descriptor layer. The remaining trace gap is the bridge from these output
    records into the live `Trans`/bone graph and subsequent IK/twist/hair/eye
    ordering.
- Accepted output descriptor / servo-candidate pointer trace:
  - report:
    `pcsx2_charclipsamples_output_descriptor_pointer_targets_20260610.json`
  - screenshot:
    `pcsx2_charclipsamples_output_descriptor_pointer_targets_20260610.window.png`,
    active Battle of the Bands gameplay at 60 FPS/VPS
  - stable descriptor cells:
    - `0x0135cd54 -> 0x013bbc60`, 63 changed rows in pointed record
    - `0x0135cd58 -> 0x013bbc70`, 64 changed rows
    - `0x0135cd60 -> 0x013bbd50`, 8 changed rows
    - `0x0135cd64 -> 0x013bbd58`, 6 changed rows
    - `0x0135cd70/0x0135cd74 -> 0x013bbd74`, stable descriptor/list rows
  - `0x013bbd74` begins with table `0x003e6f40`, then a repeated list of
    triplets:
    `0x003e6d88`, owner `0x0135ce00`, candidate records such as
    `0x0135b500`, `0x0135b800`, `0x0135c700`, `0x0135c200`,
    and `0x0135bf00`
  - interpretation: `0x0135ccec` owns stable destination/list cells pointing
    to mutable output records. This is the first accepted downstream layer
    after `0x00168320`.
- Accepted output-to-servo candidate sample:
  - report:
    `pcsx2_charclipsamples_output_trans_candidates_20260610.json`
  - screenshot:
    `pcsx2_charclipsamples_output_trans_candidates_20260610.window.png`,
    active Battle of the Bands gameplay at 60 FPS/VPS
  - `0x0135ce00` is stable, table `0x003e7ee0`, and names `bone.servo`;
    rows include owner/source `0x00b902e0`, child/list rows
    `0x0135cf00` and `0x0135cf28`, and a `0x003e6d88` entry
  - candidate records referenced by `0x013bbd74` changed live:
    - `0x0135b500`: 29 changed rows
    - `0x0135b800`: 26 changed rows
    - `0x0135c700`: 26 changed rows
    - `0x0135c200`: 26 changed rows
    - `0x0135bf00`: 26 changed rows
  - those candidate records contain table `0x003e6d88`, parent/peer pointers,
    symbol pointers around `0x007e90xx..0x007ea4xx`, and repeated
    transform-like float rows beginning near `+0x20`
  - interpretation: the sample/apply output records are now trace-linked to a
    named `bone.servo` object and mutable transform-like candidate records.
    The next trace should identify the vtable/direct update functions for
    `bone.servo` table `0x003e7ee0` and table `0x003e6d88`, then connect those
    writes to final `Trans` dirty/world resolution.
- Accepted `bone.servo` / output-record vtable trace with zero-call result:
  - report: `pcsx2_bone_servo_output_vtables_20260610.json`
  - screenshots:
    `pcsx2_bone_servo_output_vtables_20260610.after_retry.window.png` and
    `pcsx2_bone_servo_output_vtables_20260610.window.png`, active gameplay
    at 60 FPS/VPS
  - redirected exact live cells:
    `0x0135ce00 -> 0x003e7ee0` and output-record table cells
    `0x0135b508`, `0x0135b808`, `0x0135c708`, `0x0135c208`,
    `0x0135bf08 -> 0x003e6d88`
  - nonzero samples: none
  - interpretation: in this active window, the named `bone.servo` object and
    sampled `0x003e6d88` records are not updated by virtual dispatch on those
    exact cells. Their rows are moving, so the current bridge is direct/helper
    writes or an owner dispatch elsewhere.
- Accepted direct trace of `bone.servo` table functions and shared record
  helpers:
  - report: `pcsx2_bone_servo_direct_functions_20260610.json`
  - screenshot:
    `pcsx2_bone_servo_direct_functions_20260610.window.png`, active gameplay
    slowed by interpreter/direct-function probing; `EnableEE = true` was
    verified after the run
  - zero-call `bone.servo` table functions in this window:
    `0x00180860`, `0x00192968`, `0x001815d8`, `0x001817f8`,
    `0x001814c0`, `0x00181560`, `0x00181510`, `0x00182210`,
    `0x00182d38`, and `0x00182df8`
  - nonzero broad/shared functions:
    - `0x001d2960`: 120 calls, last `a0=0x01ffe3f0`,
      `a1=0x00b8c590`, `a2=0x008504e0`
    - `0x001d2ab0`: 47 calls, last `a0=0x00c9c5f0`,
      `a2=0x00617d10`
    - `0x001d2c48`: 90 calls, last `a0=0x00e32080`,
      `a1=0x00e32080`
    - `0x001dd6b8`: 35 calls, last `a0=0x0084bd40`,
      `a1=0x008163f0`, `a2=0x0084be60`
  - interpretation: the nonzero helper hits are not yet tied to the specific
    `bone.servo` output records. Do not treat them as the final output bridge
    until object samples or exact wrapping connect them to the `0x0135b500`
    family.
- Accepted `CharClipSamples` live candidate scan and negative slot traces:
  - scanner:
    `pcsx2_live_vptrs_charclipsamples_20260610.json`
  - screenshot:
    `pcsx2_live_vptrs_charclipsamples_20260610.window.png`, accepted active
    Battle of the Bands gameplay, 60 FPS/VPS
  - found 166 current live cells pointing at table `0x003e70b0`
  - only clearly named nearby cluster was `0x00ebbc90`, with ASCII names
    `hair_strum`, `bass_slap_thumb`, `strum_short_01`,
    `bass_pluck_pointer`, `strum_pick_01`, `strum_open`, and related
    strum/pluck names
  - vtable traces:
    `pcsx2_charclipsamples_vtable_trace_20260610.json`,
    `pcsx2_charclipsamples_vtable_trace_30s_20260610.json`,
    `pcsx2_charclipsamples_active_object_trace_20260610.json`, and
    `pcsx2_charclipsamples_first16_trace_20260610.json`
  - all have accepted active in-song screenshots
  - redirected live `0x003e70b0` vptrs correctly
  - repeated nonzero slot was only table offset `0x04` / function
    `0x00335d30`, with frame-cadence counts and caller args like
    `a0=0x01c00000`, `a1=0x00350348`, `a2=1`
  - sample/apply-looking slots from `0x8c..0x11c` did not fire in these
    windows, including the moving candidate `0x00d22f40`
  - interpretation: the live `CharClipSamples` objects are identified, but
    this gameplay window does not dispatch pose sampling through their vtable
    methods; continue through the CharDriver scheduler/callback callers before
    assigning output layout
- Accepted current active-state command return bridge:
  - report: `pcsx2_chardriver_state_return_current_20260610.json`
  - screenshots:
    `pcsx2_chardriver_state_return_current_20260610.after_retry.window.png`
    and `pcsx2_chardriver_state_return_current_20260610.window.png`
  - screenshot state: active in-song; final capture shows `VPS 60` but
    `FPS:N/A` and low EE/GS, so keep the performance caveat
  - wrapped table `0x003e74e8`, slot `0x34`, original `0x00173b98`
  - redirected right active state `0x00dbc9f0` and left active state
    `0x00dbca90`
  - one call captured:
    `a0=0x01ffe6e0`, `a1=0x00dbca20`, `a2=0x00850c80`, `a3=0`
  - returned `v0=0x01ffe6e0`, whose rows include pointers
    `+0x10=0x0084f550`, `+0x20=0x0059f010`, and `+0x30=0x0059ef30`
- Accepted command-object sample from the current return bridge:
  - report: `pcsx2_chardriver_command_00850c80_20260610.json`
  - screenshot:
    `pcsx2_chardriver_command_00850c80_20260610.window.png`, accepted active
    gameplay at 60 FPS/VPS
  - sampled `0x00850c80` and adjacent `0x00850c40`
  - `0x00850c80` rows include:
    `+0x00=0x0076ba90`, `+0x04=0x0044d630`, `+0x08=0x00010007`,
    `+0x10/+0x14=0x00b8b670`, `+0x18=0x00dbf36c`,
    `+0x20=0x00dbf820`, `+0x24=0x00850cb0`,
    `+0x28=0x00dbe4b0`, `+0x40/+0x44=0x00851ab0`,
    `+0x48=0x00ac87f0`, `+0x50=0x00851250`,
    `+0x54=0x008535f0`, `+0x58=0x00825128`
  - `0x00850c80 + 0x60` changed from `0x003ed390` to `0x008500d0`
  - adjacent `0x00850c40` row `+0x18` changed symbol from `idle` to
    `music_start`; this is live command/event state, not a guessed label

Twist, hair, eyes:

- `CharForeTwist` object layout:
  - object base is `vptr_addr - 4`
  - `base+0x14`: hand Trans wrapper pointer
  - `base+0x20`: twist2 Trans wrapper pointer
  - `base+0x24`: offset float, `+90` left / `-90` right in sample
- `CharUpperTwist` object layout:
  - `base+0x14`: upper_arm Trans wrapper pointer
  - `base+0x20`: twist1 Trans wrapper pointer
  - `base+0x2c`: twist2 Trans wrapper pointer
- PS2 Trans helpers:
  - `0x001dd748`: dirty propagation through child list
  - `0x003d8ea0`: world matrix resolver from local rows
- Child mesh samples prove runtime output lands in driven mesh/Trans children,
  not just the named controller headers.
- Hair header rows can stay stable while driven hair bone/mesh child rows move.
- Eye/look-at rows mutate around `r-eye.lookat + 0x6c..0x74`.

World/camera/lighting:

- `WORLD_OBJECT_BASE` scripts drive camera and lighting through message/state
  flow, not an obvious per-frame `CamShot`/`LightPreset` vptr.
- Live `WorldDir` state pocket around `0x00c9ba80` was proven in prior
  accepted samples.
- Proven scalar/value pattern: state symbol at `+0x0`, value/pointer at `+0x8`.
- `camera_beat`, `camera_bars_left`, `ignored_last_light_change`, and
  lighting/category pointer cells changed in accepted in-song samples.
- Camera shot graph roots such as `shot_started`, `start_shot`, and `shot_over`
  were sampled. Later 2026-06-11 traces now connect this graph path to the
  active CamShot runtime object and path apply bridge; exact final output row
  semantics, blend math, and render-camera handoff still need tracing.

## Immediate Next Work

Do this before any new native implementation:

1. Use the accepted ISO plus indexed `-state 1` launch path for all new PCSX2
   runtime traces unless a new screenshot proves it regressed.
2. Do not run node/clip/animation samples from the fail menu. Reject the run
   immediately if the PCSX2 HWND screenshot is not active in-song gameplay.
3. Do not spend more setup time on PCSX2 launch. The accepted route remains
   the real ISO plus indexed `-state 1`; screenshot capture is currently
   restored by using the GUI/no-focus sampler path with required screenshots.
4. Continue from the accepted node-child evidence and the 2026-06-10
   screenshot-gated deeper samples. Static classification already shows the
   sampled child/table layer is `PsMesh` / scene-graph and
   transform/reference plumbing rather than direct `CharClipSamples` output.
5. Treat `0x003e70b0` vtable slots as identified but not as the sample/apply
   dispatch path. Sampling/application is now trace-backed through direct
   helpers `0x0016b1d0`, `0x0016b2f0`, `0x0016ab88`, and `0x00168320`.
6. Trace the CharDriver downstream runtime before touching native animation:
   - continue from the accepted direct scheduler calls:
     `0x00171248`, `0x00171330`, and `0x00198660`
   - use the now-accepted scheduler/blend object layout from
     `pcsx2_blend_stack_objects_20260611.json` and
     `pcsx2_blend_pointer_targets_20260611.json`; the remaining blend work is
     exact static field naming/math for the changing floats
   - direct-call trace `0x0010c988` callback handlers in windows where the
     dispatcher actually fires
   - continue the accepted `0x00171db0` node/list layout from the `main.drv`
     scheduler `0x00768b90` and node `0x0076bd10`, especially node child
     pointers `0x00843cf0` and `0x00847e80..0x00846ee0`
   - continue from the now-proven bridge from `0x00168320`
     destination/list records into live bone/mesh output and Trans
     dirty/world helpers
   - continue from the now-proven prop attachment target bridge in
     `pcsx2_prop_live_objects_20260611.json` and
     `pcsx2_prop_trans_ring_20260611.json`; if a prop-specific handler exists
     before the Trans target updates, trace its identity
7. Extend runtime trace coverage only after the current evidence map is updated:
   - full `CharDriverMidi` clip choice and exact blend math
   - IK feed order relative to twist/hair/eyes
   - performer placement and waypoint path
   - final camera output row semantics, blend math, and render-camera handoff
   - WorldDir message/property mutation for lighting
   - venue animation pollers

## 2026-06-10 Downstream Bone Output Bridge

Accepted child/list pointer trace:

- Tool: `tools/sample_pcsx2_pointer_targets.py`.
- Report:
  `pcsx2_bone_servo_child_pointer_targets_20260610.json`.
- Screenshot:
  `pcsx2_bone_servo_child_pointer_targets_20260610.window.png`.
- Command used the accepted real ISO plus indexed `-state 1`, `--gui`,
  `--require-screenshot`, two Retry pulses, and no focus forcing.
- Screenshot gate: accepted active Battle of the Bands gameplay at 60 FPS/VPS.
- Findings:
  - `bone.servo + 0x28` (`0x0135cf28`) points back to
    `0x0135cf00`; that record exposes a controller-like row with
    `0x003e8030` and owner/source `0x00b902e0`.
  - `bone.servo` owner/source `0x00b902e0` names
    `char/metal_drummer/og/metal_drummer.milo`.
  - `0x013bbd74` payload pointer `0x013d08a0` is stable packed data in this
    window.
  - Output records point to named live mesh nodes:
    `0x0135b5c0` = `bone_pelvis.mesh`,
    `0x0135b8c0` = `bone_L-ankle.mesh`,
    `0x0135c7c0` = `bone_L-clavicle.mesh`,
    `0x0135c2c0` = `bone_L-hand.mesh`,
    `0x0135bfc0` = `bone_L-thigh.mesh`.
  - Transform-like rows inside those mesh nodes changed live. Examples:
    `0x0135b5c0` target changed 9 rows, `0x0135c7c0` changed 9 rows,
    `0x0135c2c0` changed 26 rows, and `0x0135bfc0` changed 26 rows.
- Interpretation: this is the first accepted proof that the
  `CharClipSamples` output/list layer reaches named bone/mesh nodes, not only
  anonymous output records.

Accepted downstream node vtable trace:

- Tool: `tools/trace_pcsx2_animation_vtables.py`.
- Report:
  `pcsx2_downstream_bone_mesh_vtables_20260610.json`.
- Screenshot:
  `pcsx2_downstream_bone_mesh_vtables_20260610.window.png`.
- Exact live vptrs redirected after Retry:
  - `0x0135b5c0`, `0x0135c2c0`, `0x0135bfc0`,
    `0x0135c7c0`, `0x0135b8c0` from table `0x003eaae8`.
  - Their embedded rows at `+0x2c` from table `0x003eab68`.
- Redirects: `10/10`.
- Nonzero samples: none.
- Screenshot gate: accepted active gameplay at 60 FPS/VPS.
- Interpretation: in this active window, the sampled bone/mesh nodes are not
  updated through virtual dispatch on their exact `0x003eaae8` or `0x003eab68`
  cells. Their rows move, so the bridge is direct helper / owner update, not
  those exact vtables.

Accepted direct helper trace:

- Tool: `tools/trace_pcsx2_animation_calls.py`.
- Report:
  `pcsx2_downstream_trans_helpers_direct_20260610.json`.
- Screenshot:
  `pcsx2_downstream_trans_helpers_direct_20260610.window.png`.
- Screenshot gate: accepted active gameplay; interpreter/direct-function
  probing slowed PCSX2 to about 33 FPS/VPS. `EnableEE = true` was verified
  after the run.
- Nonzero calls:
  - `0x001dd748`: 335944 calls, last `a0=0x00851b00`.
  - `0x003d8ea0`: 647415 calls, last `a0=0x00b866c0`.
  - `0x001dd6b8`: 35 calls.
  - `0x001de370`: 114 calls.
  - `0x001df640`: 186 calls.
  - `0x001dfaa0`: 2 calls.
  - `0x001e0438`: 21 calls.
  - `0x001e0620`: 4 calls.
  - `0x001e06e0`: 1 call.
  - `0x001e0fa0`: 2966 calls.
- Interpretation: hot Trans dirty/world helpers and several node/update
  helpers are active in the same gameplay window, but last-arg snapshots are
  too weak to prove exact ownership.

Trace helper added:

- `tools/trace_pcsx2_call_ring.py` keeps a bounded ring buffer of recent
  `a0..a3` values per patched direct function. This is trace tooling only;
  it does not touch native animation code.

Accepted core Trans ring trace:

- Tool: `tools/trace_pcsx2_call_ring.py`.
- Report:
  `pcsx2_trans_core_ring8192_20260610.json`.
- Screenshot:
  `pcsx2_trans_core_ring8192_20260610.window.png`.
- Screenshot gate: accepted active gameplay; interpreter/direct-function
  probing slowed PCSX2 to about 29 FPS/VPS. `EnableEE = true` was verified
  after the run.
- Counts:
  - `0x001dd748` (`Trans` dirty propagation): 141820 calls.
  - `0x003d8ea0` (`Trans` world resolver): 274818 calls.
- Ring-buffer evidence:
  - `0x001dd748` recorded 288 argument hits in the exact
    `0x0135b500..0x0135c900` output/bone family.
  - Representative `0x001dd748` hits:
    `a0=0x0135b500`, `0x0135b800`, `0x0135bf00`,
    `0x0135c200`, `0x0135c700`, plus child rows such as
    `0x0135b600`, `0x0135b700`, `0x0135c300`, `0x0135c800`,
    commonly with `a2=0x0135cce0` and packed source rows such as
    `a3=0x013c926c` or `0x013c92fc`.
  - `0x003d8ea0` recorded 240 argument hits in the same output/bone family
    and the owner/source neighborhood.
  - Representative `0x003d8ea0` hits:
    `a0=0x0135b500`, `0x0135bf00`, `0x0135c200`,
    `0x0135c300`, `0x0135c700`, `0x0135c900`, plus source/parent rows in
    `a1` such as `0x0135b560`, `0x0135bf60`, `0x0135c360`,
    `0x0135c760`.
- Interpretation: the bridge from `CharClipSamples` destination/output records
  into the live bone/mesh graph is now trace-backed. `0x00168320` writes the
  output rows, the output records point to named mesh nodes, and the central
  Trans dirty/world helpers receive those exact records in active gameplay.

Accepted normal-speed bridge object sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report:
  `pcsx2_trans_bridge_arg_objects_20260610.json`.
- Screenshot:
  `pcsx2_trans_bridge_arg_objects_20260610.window.png`.
- Screenshot gate: accepted active gameplay at 60 FPS/VPS.
- Stable bridge args:
  - `0x0135cce0` begins with `0x0135ce00`, table `0x003e7ea0`, and owner
    `0x00b902e0`.
  - `0x013c926c` and `0x013c92fc` are stable packed source/value rows in this
    sample window.
- Live changing output records:
  - `0x0135b500` (`bone_pelvis` output) changed 38 rows.
  - `0x0135b5c0` (`bone_pelvis.mesh`) changed 15 rows.
  - `0x0135c200` (`bone_L-hand` output) changed 43 rows.
  - `0x0135c2c0` (`bone_L-hand.mesh`) changed 32 rows.
  - `0x0135c700` (`bone_L-clavicle` output) changed 35 rows.
  - `0x0135bf00` (`bone_L-thigh` output) changed 43 rows.
- Interpretation: the bridge remains live at normal speed. The descriptor and
  source rows can stay stable while output/mesh transform rows change.

Accepted character ordering sequence trace:

- Trace helper added:
  `tools/trace_pcsx2_call_sequence.py`.
- Tool purpose: patch selected PS2 functions and write all hits into one
  shared chronological ring, preserving function ID plus `a0..a3`.
- Report:
  `pcsx2_character_order_sequence_20260610.json`.
- Screenshot:
  `pcsx2_character_order_sequence_20260610.window.png`.
- Screenshot gate: accepted active gameplay; interpreter/direct-function
  probing slowed PCSX2 to about 27 FPS/VPS. `EnableEE = true` was verified
  after the run.
- Functions traced:
  `0x0016b1d0`, `0x0016b2f0`, `0x0016ab88`, `0x00168320`,
  `0x001dd748`, `0x0017a080`, `0x00175678`, `0x001823c8`,
  `0x00176fb8`, and `0x0017d658`.
- Captured in the shared 8192-record ring:
  - `clip_eval_blocks`: 118 records.
  - `clip_interp_apply`: 201 records.
  - `clip_final_apply`: 201 records.
  - `clip_output_writer`: 543 records.
  - `trans_dirty`: 7014 records.
  - `ik_hand`: 14 records.
  - `fore_twist`: 22 records.
  - `upper_twist`: 58 records.
  - `hair`: 7 records.
  - `lookat`: 14 records.
- Repeated ordering patterns:
  - Clip bursts commonly appear as `clip_eval_blocks` ->
    `clip_interp_apply` -> one or more `clip_output_writer` ->
    `clip_final_apply` -> `trans_dirty` bursts.
  - IK appears in active character windows and is followed immediately by
    dirty propagation over referenced bone rows. Example:
    `ik_hand 0x00dbfa40` then dirty rows such as `0x00dbc4f0`,
    `0x00db89f0`, `0x00db6bf0`, `0x00db6cf0`, `0x00db7cf0`.
  - Foretwist calls are followed by dirty propagation on their target rows.
    Example: `fore_twist 0x00d1f4d0` then dirty rows including
    `0x00db6ef0` and `0x00db8cf0`.
  - Hair and look-at can occur adjacent in the same phase:
    `hair 0x00dbf5a0`, `lookat 0x00dbe470`, then dirty propagation and the
    next clip-eval burst.
  - Look-at on `0x00dbf940` is followed by dirty propagation and then
    upper-twist calls in repeated windows.
- Interpretation: this proves that the PS2 runtime interleaves clip
  evaluation/output, dirty propagation, IK, twist, hair, and look-at in
  repeated active-song phases. It does not yet replace the remaining need for
  a fuller performer-order map, prop attachment order, or venue/camera/light
  ordering.

Accepted blend/scheduler object layout traces:

- Tools:
  `tools/sample_pcsx2_object_words.py` and
  `tools/sample_pcsx2_pointer_targets.py`.
- Reports:
  `pcsx2_blend_stack_objects_20260611.json` and
  `pcsx2_blend_pointer_targets_20260611.json`.
- Screenshots:
  `pcsx2_blend_stack_objects_20260611.window.png` and
  `pcsx2_blend_pointer_targets_20260611.window.png`.
- Screenshot gate: accepted. Both screenshots show active Battle of the Bands
  gameplay at 60 FPS/VPS.
- `driver_0135cb90` live layout:
  - `+0x38` (`0x0135cbc8`) is the current scheduler/blend entry pointer. It
    rotated among `0x00768a50`, `0x0076bcd0`, `0x0076bad0`, and
    `0x00768c50`.
  - `+0x40` (`0x0135cbd0`) is the current clip/source pointer. It rotated
    among `0x013bb870`, `0x005f7b74`, and `0x013bc380`; the first target names
    `drummer_idle`.
  - `+0x48` changed among small state values, and `+0x4c` was a live
    float-like phase/time accumulator.
- Blend entry shape, from entries such as `0x00768a50`, `0x0076ba10`,
  `0x0076bcd0`, `0x0076bad0`, `0x00768c50`, and `0x007989d0`:
  - `+0x00`: flags/type/mode word such as `0x00000234` or `0x00000232`.
  - `+0x04`, `+0x08`: weights, commonly `1.0`.
  - `+0x0c..+0x20`: timing/phase/weight-like floats that changed during the
    sample.
  - `+0x24`: source clip/sample pointer.
  - `+0x28`: next/sibling blend entry pointer.
  - `+0x2c`: target/owner object pointer.
  - `+0x30`: sentinel `0xf149f2ca`.
- Pointer follow-up examples:
  - `0x00768a50+0x24` rotated `0x013bb500 -> 0x0115fb40`.
  - `0x00768a50+0x28` rotated among `0x0076bcd0`, `0`, and `0x00770f10`.
  - `0x00768a50+0x2c` rotated `0x00b902e0 -> 0x00b8df40`.
  - `0x0076ba10+0x24` stayed on `0x011b24c0`, while its `+0x28` rotated
    between `0` and `0x0076bc10`.
  - `0x0076ba10+0x90` rotated through readable event symbols including
    `click_hat`.
- Interpretation: the driver uses a rotating scheduler/blend stack. Native
  code must model source/next/target entries and per-entry timing/weights; it
  must not hardwire one current clip pointer. The exact semantic names for
  every timing float still need static function annotation, but the live stack
  shape is now trace-backed.

Accepted prop/mic attachment object and Trans bridge traces:

- Tools:
  `tools/sample_pcsx2_object_words.py` and
  `tools/trace_pcsx2_call_ring.py`.
- Reports:
  `pcsx2_prop_attachment_candidates_20260611.json`,
  `pcsx2_prop_live_objects_20260611.json`, and
  `pcsx2_prop_trans_ring_20260611.json`.
- Screenshots:
  `pcsx2_prop_attachment_candidates_20260611.window.png`,
  `pcsx2_prop_live_objects_20260611.window.png`, and
  `pcsx2_prop_trans_ring_20260611.window.png`.
- Screenshot gate: accepted. The object samples show active 60 FPS/VPS
  gameplay. The direct Trans ring trace shows active gameplay but slowed to
  about 33 FPS/VPS under interpreter probing; `EnableEE = true` was verified
  after the run.
- First candidate sample proved the `0x00e09xxx`, `0x00e0axxx`, and
  `0x00cdfxxx` rows are name/object directory pairs, not the moving objects
  themselves. Examples:
  - `bone_pos_guitar.mesh -> 0x00db69b0`.
  - `guitar.mesh -> 0x007642e0`.
  - `guitar_strings.mesh -> 0x00764150`.
  - `guitar_fire.mesh -> 0x00764470`.
  - `bone_pos_mic.mesh -> 0x00ce15b0`.
  - `CharPosConstraint.const -> 0x00ce2c20`.
- Live object sample results:
  - `bone_pos_guitar_mesh` at `0x00db69b0` changed 15 rows, including
    transform clusters around `0x00db6a10..0x00db6a38` and
    `0x00db6b10..0x00db6b28`.
  - `guitar_mesh` at `0x007642e0` stayed stable in this sample.
  - `guitar_strings_mesh` at `0x00764150` changed 17 rows, including
    `0x00764220..0x0076425c`.
  - `guitar_fire_mesh` at `0x00764470` stayed stable in this sample.
  - `bone_pos_mic_mesh` at `0x00ce15b0` changed 32 rows, including
    `0x00ce1610..0x00ce1660`.
  - `CharPosConstraint.const` at `0x00ce2c20` and `obj_mic_stand.mat` at
    `0x007a4a10` stayed stable in this sample.
  - Singer `bone_pelvis.mesh` at `0x00ce1db0` changed 25 rows.
- Trans bridge ring:
  - `0x001dd748` (`Trans` dirty) recorded 225230 calls; retained ring hits
    include `bone_pos_mic` rows `0x00ce15f0`, `0x00ce16f0`, singer pelvis rows
    `0x00ce1df0`, `0x00ce1ef0`, guitar rows `0x007641c0`, `0x00764350`,
    `0x00db69f0`, and `0x00db6af0`, plus owner rows near `0x00b8bef0` and
    `0x00b8bcf0`.
  - `0x003d8ea0` (`Trans` world`) recorded 437826 calls; retained ring hits
    include `bone_pos_mic` rows `0x00ce15f0`, `0x00ce1650`, singer pelvis rows
    `0x00ce1df0`, `0x00ce1e50`, guitar rows `0x00db6af0`, `0x00db6b50`, and
    `guitar_strings` row `0x007641c0`.
- Interpretation: prop attachment is not a static mesh placement. The PS2
  runtime has moving attachment targets such as `bone_pos_guitar.mesh` and
  `bone_pos_mic.mesh`, and those rows flow through the same central Trans
  dirty/world bridge as the character bones. The plain `guitar.mesh` and
  material rows may stay stable while child/attachment targets and dependent
  mesh rows move.

Limited performer-placement handler traces:

- Tool:
  `tools/trace_pcsx2_call_sequence.py`.
- Reports:
  `pcsx2_performer_placement_sequence_20260611.json` and
  `pcsx2_performer_placement_sequence_early_20260611.json`.
- Screenshots:
  `pcsx2_performer_placement_sequence_20260611.window.png` and
  `pcsx2_performer_placement_sequence_early_20260611.window.png`.
- Screenshot gate: accepted active gameplay, but interpreter/direct-function
  probing slowed PCSX2 to about 28-33 FPS/VPS. `EnableEE = true` was verified
  after both runs.
- Traced functions:
  `0x0010cfa0`, `0x00190770`, `0x00162b30`, `0x0010c948`, `0x00171c68`,
  `0x003d8ea0`, and `0x001dd748`.
- Results:
  - The normal settled window captured `0` calls to `0x0010cfa0`,
    `0x00190770`, `0x00162b30`, and `0x0010c948`, while `0x00171c68`,
    `0x003d8ea0`, and `0x001dd748` remained active.
  - The immediate post-Retry window produced the same zero-call result for the
    placement handler cluster.
- Interpretation: do not treat this as proof that `0x0010cfa0` is unused. The
  accepted static snippet still shows it resolving objects, calling
  `0x003d8ea0`, comparing against a `64.0` distance threshold, and then
  calling `0x00190770` / `0x00162b30` when needed. The current save slice does
  not exercise that branch; performer placement needs a better event window
  such as `start_at`, `lose_teleport`, `actually_walking`, or a camera-shot
  waypoint transition.

Accepted WorldDir camera/lighting symbol trace:

- Tool:
  `tools/sample_pcsx2_world_symbols.py`.
- Report:
  `pcsx2_world_camera_lighting_symbols_20260611.json`.
- Screenshot:
  `pcsx2_world_camera_lighting_symbols_20260611.window.png`.
- Screenshot gate: accepted active gameplay at 60 FPS/VPS with visibly
  different camera and lighting state.
- Live state cells:
  - `did_lighter_cam`: symbol `0x00c9ba80`, value `0x00c9ba88`, stable `0`.
  - `ignored_last_light_change`: symbol `0x00c9ba90`, value `0x00c9ba98`,
    toggled between `1` and `0`.
  - `camera_beat`: symbol `0x00c9baa0`, value `0x00c9baa8`, advanced
    `0x0b -> 0x1b`.
  - `camera_solo`: symbol `0x00c9bab0`, value `0x00c9bab8`, stable `0`.
  - `camera_bars_left`: symbol `0x00c9bad0`, value `0x00c9bad8`, changed
    among `3`, `2`, and `1`.
  - `excitement_level`: symbol `0x00c9baf0`, value `0x00c9baf8`, stable `0`.
  - `start_shot`: symbol `0x0084f2b8`, value `0x0084f2c0`, stable pointer
    `0x007471a0`.
  - `shot_started`: symbol `0x0084fd18`, value `0x0084fd20`, stable pointer
    `0x007fa410`.
  - `lighting_change`: symbol `0x00850fd8`, value `0x00850fe0`, rotated among
    `0x00842ba0`, `0x00842c00`, and `0x00842c40`.
- Deref clues:
  - The `lighting_change` derefs contain readable script/property atoms such
    as `idle`, `blackout`, `lighting`, `verse`, `section`, and `intro`.
  - The `start_shot` deref neighborhood contains `battle_lighting_RndDir`.
- Interpretation: WorldDir camera counters and lighting-change script pointers
  are live in this save. The next camera/light trace should follow the message
  or property mutation bridge from these script cells into the active
  `CamShot`/lighting application code, not class metadata.

Accepted broad WorldDir camera/lighting symbol trace:

- Tool:
  `tools/sample_pcsx2_world_symbols.py`.
- Report:
  `pcsx2_world_camera_lighting_symbols_broad_20260611.json`.
- Screenshot:
  `pcsx2_world_camera_lighting_symbols_broad_20260611.window.png`.
- Command used the accepted real ISO plus indexed `-state 1` path with posted
  Retry input, a broad camera/lighting term list, `--min-addr 0x00400000`, and
  HWND `PrintWindow` capture.
- Screenshot gate: accepted active gameplay at 60 FPS/VPS.
- Findings:
  - Static handler/name cells such as `set_lighting` stayed stable and should
    not be treated as live application state by themselves.
  - Live `lighting_change` value cells at `0x00600498`, `0x005629b4`, and
    `0x00850fe0` changed or resolved differently in the active window.
  - The `0x00850fe0` cell is the useful live WorldDir lighting cursor; it
    rotated between `0x00842c00` and `0x00842c40` in the broad sample.
  - Camera/shot script roots included `start_shot`, `shot_started`,
    `post_switch_cam`, `shot_over`, `next_shot`, `cam_check_shot`, and
    `cam_shot_ok` cells in the `world/camshot.dtb` region.
- Interpretation: this trace separates live WorldDir state from nearby
  metadata/registry strings. Continue from the heap value cells and camshot
  script graph, not from static string xrefs alone.

Accepted lighting-change pointer target trace:

- Tool:
  `tools/sample_pcsx2_pointer_targets.py`.
- Report:
  `pcsx2_lighting_change_pointer_targets_20260611.json`.
- Screenshot:
  `pcsx2_lighting_change_pointer_targets_20260611.window.png`.
- Command followed `lighting_change_00600490=0x00600498`,
  `lighting_change_005629ac=0x005629b4`,
  `lighting_change_00850fd8=0x00850fe0`,
  `start_shot_0084f2b8=0x0084f2c0`,
  `shot_started_0084fd18=0x0084fd20`, and
  `shot_over_007435c8=0x007435d0`.
- Screenshot gate: accepted active gameplay at 60 FPS/VPS.
- Findings:
  - `lighting_change_00850fd8` rotated through `0x00842b20`,
    `0x00842ba0`, and `0x00842c00`, with `105` changed rows across pointed
    object windows.
  - The pointed records expose live script/list atoms such as `blackout`,
    `color1`, `color2`, `lighting`, `music_start`, `section`, `chorus_1`,
    `chorus`, `sync_wag`, and `flare`.
  - `lighting_change_00600490` pointed at a stable
    `world/world_objects_worldbase.dtb` script block around `0x00600680`.
  - `lighting_change_005629ac` pointed at static song/path text
    `songs/johnthefisherman/johnthefisherman_p65`; treat it as metadata in
    this context.
  - Stable shot roots remained `start_shot=0x007471a0`,
    `shot_started=0x007fa410`, and `shot_over=0x019789b0` in this sample.
- Interpretation: `0x00850fe0` is currently the best live cursor for lighting
  script/keyframe progression. The other same-name cells are useful context
  but not the active lighting state cursor.

Accepted broad camera/lighting shot-graph trace:

- Tool:
  `tools/sample_pcsx2_shot_graph.py`.
- Report:
  `pcsx2_shot_graph_camera_lighting_broad_20260611.json`.
- Screenshot:
  `pcsx2_shot_graph_camera_lighting_broad_20260611.window.png`.
- Command used the accepted real ISO plus indexed `-state 1` path with broad
  terms, `--min-addr 0x00400000`, `--radius 0x100`, `--depth 3`, and
  `--max-nodes 96`.
- Screenshot gate: accepted active gameplay at 60 FPS/VPS.
- Findings:
  - `world/camshot.dtb` roots around `0x0060a570`, `0x0060ad70`, and
    `0x00609940` contain live `shot_started`, `post_switch_cam`,
    `shot_over`, `next_shot`, and `start_shot` graph nodes.
  - The `shot_started` root at `0x0060a570` contains `post_switch_cam` at
    `0x0060a5a8`, whose value points to the `start_shot` graph at
    `0x00609940`.
  - The `shot_over` root at `0x0060ad70` contains `next_shot` nodes and
    world pointers for `world/small2/small2.dtb`, `world/arena/arena.dtb`,
    and `world/big/big.dtb`.
  - The `start_shot` graph at `0x00609940` includes authored metadata/help
    strings for shot facing, special-purpose use, solo use, and successor-shot
    behavior.
  - The lighting script graph remains in `world/world_objects_worldbase.dtb`
    around `0x00600490..0x00600c70`, with `do_lighting_next_keyframe`,
    `do_lighting_prev_keyframe`, `do_lighting_first_keyframe`,
    `ignored_last_light_change`, and `excitement_level` nodes.
- Interpretation: camera shot flow is script-graph driven. The native port
  needs a real `world/camshot.dtb` graph model and should not collapse this to
  a single ad hoc camera pick.

Accepted camera/lighting graph function sequence trace:

- Tool:
  `tools/trace_pcsx2_call_sequence.py`.
- Report:
  `pcsx2_camera_lighting_graph_functions_sequence_20260611.json`.
- Screenshot:
  `pcsx2_camera_lighting_graph_functions_sequence_20260611.window.png`.
- Command used the accepted real ISO plus indexed `-state 1` path,
  `--disable-ee-recompiler`, `--retry-pulses 2`, `--post-retry-seconds 3`,
  `--seconds 12`, and a shared chronological ring for camera/script/light
  function candidates. `EnableEE = true` was verified after the run.
- Screenshot gate: accepted active gameplay with visibly different lighting;
  interpreter probing slowed PCSX2 to about 28 FPS/VPS.
- Nonzero counts:
  - `cam_check_func_0011f848`: `9`.
  - `script_eval_002b6238`: `4`.
  - `script_do_002b3118`: `27`.
  - `script_filter_002b31b0`: `358`.
  - `script_compare_float_002b3658`: `4`.
  - `script_list_002b3818`: `1`.
  - `script_pick_new_002b3d50`: `17`.
- Zero counts in this window:
  - `0x0011f8c8`, `pick_shot_handler 0x002608f8`,
    `lighting_next_handler 0x00271200`, `lighting_prev_handler 0x002716b8`,
    `lighting_first_handler 0x00271778`, `lighting_next_apply 0x00280f60`,
    `lighting_prev_apply 0x00280fe8`, `lighting_first_apply 0x00281070`,
    `0x00271f70`, and `0x00271f78`.
- Ordering and args:
  - Repeated camera checks hit `0x0011f848` with
    `a1=0x0060ace0`, directly inside the `world/camshot.dtb` `check_shot`
    graph.
  - `script_pick_new_002b3d50` repeatedly hit graph nodes such as
    `0x005f0340`, `0x0059cb50`, `0x00600110`, `0x005f77e0`,
    `0x005f7910`, and `0x0059def0`.
  - Lighting script activity did hit generic script helpers near the
    `world_objects_worldbase` graph, for example `script_compare_float` with
    `a1=0x00600be0` and `script_list` with `a1=0x00600bd0`.
- Interpretation: active camera shot selection/checking is now tied to live
  `world/camshot.dtb` graph nodes and generic script evaluator helpers. The
  exact low-level CamShot pose/blend application and LightPreset application
  routines are still not proven by nonzero function hits.

Accepted camera-check runtime object sample:

- Tool:
  `tools/sample_pcsx2_object_words.py`.
- Report:
  `pcsx2_camera_check_runtime_objects_20260611.json`.
- Screenshot:
  `pcsx2_camera_check_runtime_objects_20260611.window.png`.
- Command used the accepted real ISO plus indexed `-state 1` path with
  `--gui`, `--require-screenshot`, `--retry-pulses 2`,
  `--post-retry-seconds 3`, `--seconds 8`, `--interval 0.25`, and explicit
  targets for `0x00b7a2d0`, `0x00b7a3f0`, `0x00b8be10`, `0x00daf090`,
  `0x00b8bf80`, `0x005d2498`, and `0x0060ace0`.
- Screenshot gate: accepted active Battle of the Bands gameplay at
  60 FPS/VPS.
- Changed row counts:
  - `cam_eval_this` at `0x00b7a2d0`: `14`.
  - `cam_eval_ref` at `0x00b7a3f0`: `0`.
  - `char_source` at `0x00b8be10`: `19`.
  - `main_drv` at `0x00daf090`: `2`.
  - `placement_arg` at `0x00b8bf80`: `6`.
  - `shot_ok_aux` at `0x005d2498`: `0`.
  - `cam_script_node` at `0x0060ace0`: `0`.
- Runtime layout clues:
  - `cam_eval_this + 0x00` points to `0x00b7a3f0`; `+0x40` names
    `INTRO_FAST`; the broader sampled range includes another shot block
    naming `flr_near_lft`.
  - `cam_eval_this` has continuously changing float/matrix-like rows around
    `0x00b7a370..0x00b7a3c8`.
  - `cam_eval_ref` is stable and contains the readable shot name
    `Intro_fast`.
  - `char_source` again names `char/glam1/og/glam1.milo` and has changing
    placement/pose rows around `0x00b8bf10..0x00b8bf84` plus time/state rows
    around `0x00b8c048..0x00b8c084`.
  - `main_drv` at `0x00daf090` again names `main.drv`; its `+0x38` entry
    pointer rotated among `0x00768b90`, `0x00770f10`, `0x0076bd10`, and
    `0x0076bbd0`, while `+0x48` advanced as a float-like phase/time row.
  - `cam_script_node` at `0x0060ace0` stayed stable and contains the
    `world/camshot.dtb` graph row with function cells `0x0011f848` and
    `0x0011f8c8`, plus `shot_over` and `next_shot` atoms.
  - `shot_ok_aux` stayed stable and appears to be text/metadata around
    render flags and event names such as `solo_on`, `solo_off`, `peak_on`,
    and `peak_off`.
- Interpretation: this separates the static `world/camshot.dtb` graph node
  from live runtime camera/shot state objects. `0x0011f848` passes through a
  mutable object rooted at `0x00b7a2d0` and a stable shot/ref block at
  `0x00b7a3f0`; do not implement camera selection by reading the graph row
  alone. This still does not prove the final low-level camera pose/blend
  application function.

Accepted CamShot live metadata/object reference samples:

- Tools:
  `tools/sample_pcsx2_object_words.py` and
  `tools/sample_pcsx2_pointer_targets.py`.
- Reports:
  `pcsx2_camshot_live_refs_20260611.json` and
  `pcsx2_camshot_runtime_pointer_targets_20260611.json`.
- Screenshots:
  `pcsx2_camshot_live_refs_20260611.window.png` and
  `pcsx2_camshot_runtime_pointer_targets_20260611.window.png`.
- Commands used the accepted real ISO plus indexed `-state 1` path with
  `--gui`, `--require-screenshot`, `--retry-pulses 2`,
  `--post-retry-seconds 3`, and explicit camera/shot cells. Both screenshots
  were accepted active in-song captures; the pointer-target screenshot showed a
  different active camera/lighting state at 60 FPS/VPS.
- `pcsx2_camshot_live_refs_20260611.json` separated static metadata from live
  runtime state:
  - `current_shot_state` at `0x005feeb8` is a stable
    `world/world_objects_worldbase.dtb` graph block for `current_shot`,
    `check_shot`, `pick_new_shot`, and `camera_bars_left`.
  - `camera_director_0060a3c4` exposes `world/camshot.dtb` predicates and
    category fields such as `distance`, `facing`, `special`, `solo`,
    `walk_ok`, `starpower_ok`, `far_starpower_ok`, `crowd_face_camera`, and
    `next_shot`.
  - `camera_director_0060a520` carries `shot_started`,
    `post_switch_cam`, debug labels such as `Shot Name:`, `distance:`,
    `facing:`, and `solo:`, plus a pointer to the start-shot graph.
  - `camera_director_0060a930` / `0060aa10` carry `shot_ok`, `prev_shot`,
    `do_force_shot`, `script_task`, and `camshot_skip_next_update`.
- `pcsx2_camshot_runtime_pointer_targets_20260611.json` followed the live
  eval object through authored shot data:
  - Runtime eval object `0x00b7a2d0` points at stable shot/ref
    `0x00b7a3f0`, named `Intro_fast`.
  - `Intro_fast` references keyframe/property blocks `0x014dd390`,
    `0x014dd5b0`, and `0x014dd4a0`; the property block contains authored
    values including `distance near`, `facing left`, `special`,
    `solo never`, `hide_crowd = 1`, `force_char_lod = -1`,
    `starpower_ok = 1`, and `far_starpower_ok = 1`.
  - The next shot/ref block is `0x00b7a550`, named
    `flr_near_lft03_raise1.shot`, with category `flr_near_lft` and pointers
    to keyframe/property blocks `0x014dda40` and `0x014ddd70`.
  - Target/list object `0x007fbce0` references camera target bone/mesh names
    including `spot_neck_fret20.mesh`, `bone_spine3.mesh`, and
    `bone_spine1.mesh`.
- Interpretation: static graph rows select/evaluate shot state, but the live
  runtime object then resolves to authored CamShot/BandCamShot keyframe,
  property, category, and target blocks. Native camera code needs both layers.

Limited/rejected CamShot runtime vtable trace:

- Tool: `tools/trace_pcsx2_animation_vtables.py`.
- Report: `pcsx2_camshot_runtime_vtables_20260611.json`.
- Screenshot gate: rejected as negative proof. The after-Retry screenshot was
  in-song but unhealthy at about 2 FPS with abnormal visual state, and the
  final screenshot reached the fail menu.
- Useful static/table data only:
  - Live vptrs matched object cells for `Intro_fast` and
    `flr_near_lft03_raise1.shot`.
  - Primary table `0x003f0b88` contains method slots such as
    `0x00262210`, `0x00262700`, `0x00262b08`, `0x002681d0`,
    `0x002688b0`, `0x00264370`, `0x00263e78`, `0x00264528`, and
    `0x00262658`.
  - Keyframe/path-related tables `0x003f0c08`, `0x003f0c58`, and
    `0x003f0ca8` contain slots including `0x0026c900`, `0x0026c988`,
    `0x0026ca10`, `0x0026c808`, `0x0026ae00`, `0x0026c710`,
    `0x0026cda8`, `0x00269080`, and `0x0026a550`.
- Interpretation: use this as a static slot inventory only. It does not prove
  zero runtime calls.

Accepted CamShot direct method sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_camshot_direct_methods_sequence_20260611.json`.
- Screenshot: `pcsx2_camshot_direct_methods_sequence_20260611.window.png`.
- Command used the accepted real ISO plus indexed `-state 1` path with
  `--disable-ee-recompiler`, `--retry-pulses 2`,
  `--post-retry-seconds 3`, `--seconds 20`, and `--ring-size 8192`.
  `EnableEE = true` was verified after the run.
- Screenshot gate: accepted active in-song gameplay with visible band/stage;
  interpreter probing slowed PCSX2 to about 28 FPS/VPS.
- Nonzero counts:
  - `cam_check_graph` `0x0011f848`: `15`.
  - `cam_check_eval` `0x0011f628`: `15`.
  - `camshot_apply_or_set` `0x00262b08`: `579`.
  - `cam_key_iter_a` `0x0026c900`: `579`.
  - `cam_path_apply` `0x0026ae00`: `578`.
  - `camshot_handler_a` `0x002681d0`: `46`.
  - `camshot_handler_b` `0x002688b0`: `16`.
- Zero counts in this accepted window:
  `0x00262210`, `0x00262700`, `0x00262658`, `0x00264370`,
  `0x00263e78`, `0x00264528`, `0x0026c988`, `0x0026ca10`,
  `0x0026c808`, `0x0026c710`, `0x0026cda8`, `0x00269080`, and
  `0x0026a550`.
- Ordering and args:
  - `cam_check_graph 0x0011f848` used
    `a0=0x01ffdde0`, `a1=0x0060ace0`, `a2=1`, `a3=0x00b7a3f0`.
  - `cam_check_eval 0x0011f628` used
    `a0=0x00b7a2d0`, `a1=0x00536988`, `a2=1`, `a3=0x00b7a2d0`.
  - `camshot_apply_or_set 0x00262b08` repeatedly used
    `a0=a1=0x00b7a2d0`, `a2=0x0059b4c0`, `a3=0x0041f738`.
  - `cam_key_iter_a 0x0026c900` repeatedly used
    `a0=0x00b8e9d0`, `a1=0x0026c900`, `a2=0x00b8ea10`, and a changing
    phase float in `a3`.
  - `cam_path_apply 0x0026ae00` repeatedly used
    `a0=0x00b8e9d0`, `a1=a2=a3=0`.
- Interpretation: this is the first accepted trace tying graph-driven camera
  checking to the per-frame CamShot/path apply bridge:
  `0x0011f848` graph check -> `0x0011f628` eval -> live object
  `0x00b7a2d0` -> `0x00262b08` apply/update -> `0x0026c900` path/key
  iteration on `0x00b8e9d0` / `0x00b8ea10` -> `0x0026ae00` path/transform
  apply. Exact final camera output rows and blend math are still open.

Accepted CamShot path/apply object sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_camshot_path_apply_objects_20260611.json`.
- Screenshot: `pcsx2_camshot_path_apply_objects_20260611.window.png`.
- Command used the accepted real ISO plus indexed `-state 1` path with
  `--gui`, `--require-screenshot`, `--retry-pulses 2`,
  `--post-retry-seconds 3`, `--seconds 8`, `--interval 0.25`, and explicit
  object targets for `0x00b7a2d0`, `0x00b8e9d0`, `0x00b8ea10`,
  `0x0059b4c0`, `0x0060ad90`, `0x00728a30`, `0x01ffe970`, and
  `0x01ffe090`.
- Screenshot gate: accepted active in-song gameplay at 60 FPS/VPS.
- Changed row counts:
  - `cam_runtime_shot` `0x00b7a2d0`: `14`.
  - `cam_path_object` `0x00b8e9d0`: `72`.
  - `cam_path_child` `0x00b8ea10`: `40`.
  - `camshot_aux` `0x0059b4c0`: `0`.
  - `camshot_script_a` `0x0060ad90`: `0`.
  - `camshot_script_b` `0x00728a30`: `1`.
  - `cam_check_tmp` `0x01ffe970`: `55`.
  - `cam_check_tmp2` `0x01ffe090`: `72`.
- Important rows:
  - `0x00b7a2d0 + 0x04` advanced as a large float-like time/phase row
    from about `107.046` to `339.782`.
  - `0x00b7a2d0 + 0xa0..0xf8` contains two small changing matrix/quaternion
    clusters.
  - `0x00b7a2d0 + 0x344` points to `0x00b8e9d0`, the path/apply object
    used as `a0` by `0x0026c900` and `0x0026ae00`.
  - `0x00b8e9d0 + 0x120..0x258` and `0x00b8ea10 + 0x0e0..0x218`
    contain repeated changing transform/key blocks. Example floats include
    rotation-like values around `0.4938`, `0.8688`, `-0.8235`, `0.4688`,
    `-0.3175`, `-0.2769`, `0.1553`, and translation-like values around
    `94.156`, `-53.597`, `83.805`, `129.442`, and `-45.066`.
  - Stack/temp object `0x01ffe970` carried a live pointer to `0x00b8e9d0`
    at `+0xc0` during the sample, along with transient transform scratch rows.
- Interpretation: the path/apply object and child are live, mutating camera
  path/keyframe records, not static metadata. Continue by statically and
  dynamically tracing downstream from `0x00262b08`, `0x0026c900`, and
  `0x0026ae00` into the final Trans/render-camera handoff.

Accepted headless camera result-row handoff sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_camera_result_rows_headless_retry_20260622_resume.json`.
- Log: `pcsx2_camera_result_rows_headless_retry_20260622_resume.log`.
- Command used the accepted real ISO plus indexed `-state 1` path with
  no-front/background PCSX2 input, `--pre-retry-seconds 4`,
  `--retry-pulses 2`, `--post-retry-seconds 3`, `--seconds 24`,
  `--interval 0.2`, and explicit object targets for mutable camera result/path
  rows `0x00b92ef0`, `0x00b92f50`, `0x00b930e0`, `0x00b8e9d0`,
  `0x00b8ea10`, `0x01ffe750`, and `0x01ffe790`.
- Target sample/change counts:
  - `cam_result_00b92ef0`: `120` samples, `107` changed rows.
  - `cam_child_a_00b92f50`: `120` samples, `95` changed rows.
  - `cam_result_path_00b930e0`: `120` samples, `38` changed rows.
  - `cam_path_00b8e9d0`: `120` samples, `73` changed rows.
  - `cam_path_frame_00b8ea10`: `120` samples, `74` changed rows.
  - `stack_result_a_01ffe750`: `120` samples, `93` changed rows.
  - `stack_result_b_01ffe790`: `120` samples, `94` changed rows.
- Interpretation: the Battle result family stayed on one authored camera family
  for the early window, then switched directly to the next family and continued
  with small live motion. This supports native cutting between distinct
  `start_shot` families and reserving interpolation for same-shot
  `post_switch_cam` position changes. It does not close exact final render
  camera field naming or all camera blend math.

Accepted CamShot downstream child sequence traces:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Reports:
  `pcsx2_camshot_downstream_children_sequence_20260611.json` and
  `pcsx2_camshot_downstream_nodirty_sequence_20260611.json`.
- Screenshots:
  `pcsx2_camshot_downstream_children_sequence_20260611.window.png` and
  `pcsx2_camshot_downstream_nodirty_sequence_20260611.window.png`.
- Commands used the accepted real ISO plus indexed `-state 1` path with
  `--disable-ee-recompiler`, `--retry-pulses 2`,
  `--post-retry-seconds 3`, `--seconds 20`, and `--ring-size 8192`.
  `EnableEE = true` was verified after each run.
- Screenshot gates: both accepted active in-song captures with visible
  Battle of the Bands stage/band. Interpreter probing slowed PCSX2 to about
  27 FPS/VPS.
- First trace included dirty propagation and was useful but noisy:
  - total calls: `583051`.
  - retained ring counts: `0x001dd748` `7838`, `0x002ff268` `242`,
    `0x001b1ee0` `72`, plus `8` each for `0x00262b08`, `0x00263410`,
    `0x002665a0`, `0x0026c900`, and `0x0026ae00`.
  - Interpretation: dirty propagation is reachable in the same downstream
    window but is too hot to keep in a CamShot-local chronological ring.
- Second trace removed the dirty helper and kept the CamShot-local order:
  - total calls: `24732`.
  - retained ring counts: `camshot_apply_or_set 0x00262b08` `188`;
    `cam_apply_child_00263410` `188`; `cam_apply_child_002665a0` `188`;
    `cam_key_iter_a 0x0026c900` `188`; `cam_path_apply 0x0026ae00` `187`;
    `cam_path_child_002ff268` `5561`; `cam_path_child_001b1ee0` `1692`.
  - Zero retained counts for `0x00265d90`, `0x00307bc0`, and `0x00266df8`
    in this accepted window.
- Repeated local ordering:
  - `0x00262b08` on `a0=a1=0x00b7a2d0` is followed by
    `0x00263410(0x00b7a2d0, 0x01ffe980, 0x01ffe984, 0x01ffe988)`.
  - It then calls `0x002665a0(0x00494b80, 0x00b7a2d0, 0x014dd4a0,
    0x00b92ef0)`.
  - `0x002665a0` immediately reaches `0x001b1ee0(0x00b92ef0,
    0x00b7a2d0, 0x014dd4a0, 0x00b92ef0)`.
  - The path iteration branch repeatedly shows `0x0026c900(0x00b8e9d0,
    0x0026c900, 0x00b8ea10, phase)` followed by `0x0026ae00(0x00b8e9d0,
    0, 0, 0)`.
  - After `0x0026ae00`, the trace repeatedly alternates
    `0x002ff268` and `0x001b1ee0` on `a0=a1=0x00b8ead0` with target/list
    objects such as `0x007ce440`, `0x007ce530`, `0x007ce580`,
    `0x007d2bd0`, `0x007d2c20`, `0x007d2c70`, `0x007d2cc0`, and
    `0x007d2d10`.
- Static snippets:
  `ps2_function_snippets_camshot_downstream_children_20260611.json` covers
  `0x00263410`, `0x002665a0`, `0x002ff268`, `0x001b1ee0`, `0x00265d90`,
  `0x00307bc0`, and `0x00266df8`.
  - `0x002665a0` is a large CamShot/path evaluator using the live eval object,
    authored block, and result frame.
  - `0x002ff268` is float/math validation or normalization work.
  - `0x001b1ee0` is a camera-specific setter with a hardcoded `0x447a0000`
    scale and a compare/update around `a0 + 0x2c4`.
- Interpretation: the CamShot update now has a trace-backed downstream order.
  The final native camera model should represent both the eval object result
  frame (`0x00b92ef0`) and path-frame branch (`0x00b8ead0`) instead of
  treating `0x0026ae00` as a terminal black box.

Accepted CamShot downstream argument object sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_camshot_downstream_arg_objects_20260611.json`.
- Screenshots:
  `pcsx2_camshot_downstream_arg_objects_20260611.before_sample.window.png`
  and `pcsx2_camshot_downstream_arg_objects_20260611.window.png`.
- Command used the accepted real ISO plus indexed `-state 1` path with
  `--gui`, `--require-screenshot`, `--retry-pulses 2`,
  `--post-retry-seconds 3`, `--seconds 8`, `--interval 0.25`, and explicit
  object targets for the eval object, result frame, path frame, authored block,
  and target-list nodes.
- Screenshot gate: accepted active in-song gameplay at 60 FPS/VPS.
- Changed row counts:
  - `cam_eval_obj` `0x00b7a2d0`: `14`.
  - `cam_apply_result_00b92ef0`: `94`.
  - `cam_path_frame_00b8ead0`: `103`.
  - `cam_path_object` `0x00b8e9d0`: `78`.
  - `cam_path_child` `0x00b8ea10`: `74`.
  - `cam_authored_block_014dd4a0`: `0`.
  - Target/list objects `0x007ce440`, `0x007ce530`, `0x007ce580`,
    `0x007d2bd0`, `0x007d2c20`, `0x007d2c70`, `0x007d2cc0`, and
    `0x007d2d10`: all `0`.
- Important rows:
  - `0x00b92ef0` and `0x00b8ead0` begin with the same list/object header shape:
    linked pointers at `+0x00/+0x04`, vtable-like `0x003e6d88` at `+0x08`,
    and repeated transform blocks beginning at `+0x20`.
  - `0x00b92ef0 + 0x20..0x98` contains two near-identical camera result
    transform blocks with rows like `0.4938`, `0.8688`, `0.001888`,
    `-0.8235`, `0.4688`, `-0.3175`, `-0.2769`, `0.1553`, `0.9476`, and
    translation-like values around `351.286`, `-95.542`, and `150.85`.
  - `0x00b92ef0 + 0xc0..0xf8` contains another changing block with values
    around `0.4945`, `-0.8246`, `-0.2773`, `0.8700`, `0.4694`,
    `0.1555`, `0.001927`, `-0.3179`, `0.9488`, `-90.873`,
    `382.472`, and `-30.853`.
  - `0x00b92ef0 + 0x180..0x1ac` includes projection/screen-like rows,
    including stable `768.0` values and changing small orientation/offset
    rows.
  - `0x00b8ead0` mirrors path-frame transform blocks with translation-like
    values around `94.156`, `-53.597`, `83.805`, `129.442`, and
    `-45.066`.
  - `0x00b8e9d0 + 0x60/+0x64` points at target/list range
    `0x007ce440..0x007d2d10`; `+0x6c` points at `0x00b8ead0`.
  - The sampled target/list nodes are stable authored/runtime target records.
    They include readable data such as `crowd_eyeball.tex`, crowd bitmap paths,
    and at `0x007d2d10` bone channel names such as `bone_pelvis.pos`,
    `bone_L-ankle.quat`, `bone_L-clavicle.quat`, `bone_L-hand.quat`,
    `bone_R-hand.quat`, `bone_head.quat`, and spine/forearm/knee channels.
- Interpretation: `0x014dd4a0` and the target-list nodes are source/static
  data for this window; the moving output is in `0x00b92ef0` and
  `0x00b8ead0`, fed by `0x002665a0`, `0x001b1ee0`, `0x0026ae00`, and
  `0x002ff268`.

Lighting handler static xrefs and snippets:

- Xref report: `ps2_lighting_string_xrefs_fresh_20260611.json`.
- Snippet report: `ps2_function_snippets_lighting_handlers_fresh_20260611.json`.
- Static xref facts:
  - `set_lighting` string `0x004445a8` is referenced by the handler
    registration block around `0x0026fe80`; the nearby call at
    `0x0026fea0` targets `0x00271288`.
  - `lighting_next_keyframe` string `0x004445c8` reaches
    `0x00271200` and `0x002716b8`.
  - `lighting_prev_keyframe` string `0x004445e0` reaches
    `0x002716b8` and `0x00271718`.
  - `lighting_first_keyframe` string `0x004445f8` reaches
    `0x00271718` and `0x00271778`.
- Static snippet facts:
  - `0x00271288` is the PS2 `set_lighting` handler candidate. It checks
    `a1 + 0xa4`, so runtime traces must treat `a1` as the live world/light
    state object.
  - `0x00271200`, `0x002716b8`, `0x00271718`, and `0x00271778` are the
    manual/next/prev/first keyframe handler candidates.
  - `0x00280f60`, `0x00280fe8`, and `0x00281070` call through the global/list
    area at `0x0052ee40` / `0x004957b0`; runtime arguments are required
    before naming their exact keyframe roles.
  - `0x00271f70` and `0x00271f78` are no-op stubs in the sampled PS2
    executable slice.

Accepted lighting set/keyframe handler sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_lighting_set_handlers_sequence_20260611.json`.
- Screenshot: `pcsx2_lighting_set_handlers_sequence_20260611.window.png`.
- Command:
  `python tools/trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 3 --seconds 30 --ring-size 16384 --log pcsx2_lighting_set_handlers_sequence_20260611.log --out pcsx2_lighting_set_handlers_sequence_20260611.json --target set_lighting_handler=0x00271288 --target lighting_next_handler=0x00271200 --target lighting_prev_handler=0x002716b8 --target lighting_prev_alt=0x00271718 --target lighting_first_handler=0x00271778 --target lighting_next_apply=0x00280f60 --target lighting_prev_apply=0x00280fe8 --target lighting_first_apply=0x00281070 --target lighting_advance_271f70=0x00271f70 --target lighting_advance_271f78=0x00271f78 --target script_eval_002b6238=0x002b6238 --target script_do_002b3118=0x002b3118 --target script_filter_002b31b0=0x002b31b0 --target script_compare_float_002b3658=0x002b3658 --target script_list_002b3818=0x002b3818 --target script_pick_new_002b3d50=0x002b3d50`
- Screenshot gate: accepted active in-song Battle of the Bands gameplay.
  `EnableEE = true` was verified afterward and no PCSX2 process was left
  running.
- Retained counts:
  - `set_lighting_handler 0x00271288`: `5`.
  - `lighting_prev_handler 0x002716b8`: `1`.
  - `lighting_next_apply 0x00280f60`: `1`.
  - `script_eval_002b6238`: `28`.
  - `script_do_002b3118`: `77`.
  - `script_filter_002b31b0`: `848`.
  - `script_compare_float_002b3658`: `10`.
  - `script_list_002b3818`: `3`.
  - `script_pick_new_002b3d50`: `52`.
  - Zero retained calls for `0x00271200`, `0x00271718`, `0x00271778`,
    `0x00280fe8`, `0x00281070`, `0x00271f70`, and `0x00271f78` in this
    accepted window.
- Live argument facts:
  - `set_lighting_handler` uses `a1=0x00b78418` as the live world/light state
    object and script/message nodes `a2=0x006006a0` or `0x00600660`.
  - The keyframe path uses
    `lighting_prev_handler(0x01ffdfc0, 0x00b78418, 0x00600770, 1)`, followed
    immediately by `lighting_next_apply(0x00520000, 0x00b78418, 0x00600770,
    1)`.
  - The local script order around the live keyframe hit is:
    `set_lighting_handler` -> `script_eval_002b6238` ->
    `script_filter_002b31b0` -> `script_list_002b3818` ->
    `script_compare_float_002b3658` -> `script_pick_new_002b3d50` ->
    `lighting_prev_handler` -> `lighting_next_apply` -> `script_do_002b3118`.
- Interpretation: PS2 lighting is driven by the `world_objects_worldbase.dtb`
  script graph into a live world/light state object at `0x00b78418`, then into
  keyframe helper `0x00280f60` for this accepted slice. This proves a real
  lighting dispatch/apply chain, but it does not yet name every field or prove
  all next/prev/first variants.

Accepted lighting argument object sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_lighting_arg_objects_20260611.json`.
- Screenshots:
  `pcsx2_lighting_arg_objects_20260611.before_sample.window.png` and
  `pcsx2_lighting_arg_objects_20260611.window.png`.
- Command:
  `python tools/sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 3 --seconds 8 --interval 0.25 --no-default-targets --target lighting_state_00b78418=0x00b78418:0x180 --target set_node_006006a0=0x006006a0:0x120 --target set_node_00600660=0x00600660:0x120 --target key_node_00600770=0x00600770:0x120 --target list_node_006006f0=0x006006f0:0x120 --target compare_node_00600700=0x00600700:0x120 --target world_lighting_cursor_00850fe0=0x00850fe0:0x120 --target lighting_list_004957b0=0x004957b0:0x120 --target global_0052ee40=0x0052ee40:0x120 --log pcsx2_lighting_arg_objects_20260611.log --out pcsx2_lighting_arg_objects_20260611.json`
- Screenshot gate: accepted active in-song gameplay at 60 FPS/VPS.
  `EnableEE = true` was verified afterward and no PCSX2 process was left
  running.
- Changed row counts:
  - `lighting_state_00b78418`: `9`.
  - `world_lighting_cursor_00850fe0`: `30`.
  - `lighting_list_004957b0`: `1`.
  - `set_node_006006a0`, `set_node_00600660`, `key_node_00600770`,
    `list_node_006006f0`, `compare_node_00600700`, and `global_0052ee40`:
    `0`.
- Important rows:
  - `0x00b78418` has list/object-style header rows, including pointers to
    `0x008514f0`, `0x007f2390`, `0x00b78a80`, `0x00b78d88`, `0x0074f430`,
    `0x00851260`, and class/path text resolving to `WorldDir`.
  - `0x00b78418 + 0x60` changed through pointers
    `0x00b885a0`, `0x00b85810`, and `0x00b886b0`.
  - `0x00b78418 + 0x64/+0x68` changed through float-like values including
    `0x407d72c6` and `0x40fbc937`.
  - `0x00b78418 + 0x70/+0x80` changed through `0x0072be50`,
    `0x0072be60`, `0x0072be68`, and `0x0072be70`.
  - `0x00b78418 + 0xc8` changed between `0x007fe790` and `0x00782580`.
  - `0x00b78418 + 0xe0` changed through small state values `3`, `1`, and
    `4`.
  - Script nodes are static graph rows in `world/world_objects_worldbase.dtb`.
    `0x006006d0` names `do_lighting_next_keyframe`;
    `0x00600788` names `lighting_next_keyframe`;
    `0x00600720` names `excitement_level`;
    `0x00600750` and `0x006007b0` name `ignored_last_light_change`.
  - `0x00850fe0` is the previously seen live `lighting_change` cursor and
    rotated through `0x00842b20`, `0x00842ba0`, and `0x00842c00` in this
    accepted sample.
  - `0x004957b0` is stable list/global data; only `0x004957c0` changed from
    `0x00537e20` to `0x00537e28`.
- Interpretation: the script graph nodes are static authored data; live
  lighting state changes are concentrated in `0x00b78418` and the
  `lighting_change` cursor at `0x00850fe0`. Native lighting should model
  `LightPreset`/keyframe state as runtime state rather than baking only the
  script graph.

Accepted lighting pointer-target sample:

- Tool: `tools/sample_pcsx2_pointer_targets.py`.
- Report: `pcsx2_lighting_pointer_targets_20260611.json`.
- Screenshots:
  `pcsx2_lighting_pointer_targets_20260611.before_pointer.window.png` and
  `pcsx2_lighting_pointer_targets_20260611.window.png`.
- Command:
  `python tools/sample_pcsx2_pointer_targets.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 3 --seconds 8 --interval 0.25 --object-size 0x100 --cell light_state_ptr60=0x00b78478 --cell light_state_ptr70=0x00b78488 --cell light_state_ptr80=0x00b78498 --cell light_state_ptrc8=0x00b784cc --cell lighting_change_cursor=0x00850fe0 --cell lighting_list_current=0x004957c0 --log pcsx2_lighting_pointer_targets_20260611.log --out pcsx2_lighting_pointer_targets_20260611.json`
- Screenshot gate: accepted active in-song gameplay at 60 FPS/VPS.
  `EnableEE = true` was verified afterward and no PCSX2 process was left
  running.
- Unique pointer sets:
  - `0x00b78418 + 0x60`: `0x00b885a0`, `0x00b85810`, `0x00b886b0`.
  - `0x00b78418 + 0x70`: `0x0072be50`, `0x0072be60`, `0x0072be68`.
  - `0x00b78418 + 0x80`: `0x0072be60`, `0x0072be68`, `0x0072be70`.
  - `0x00b78418 + 0xc8`: `0x007fe790`.
  - `lighting_change_cursor`: `0x00842b20`, `0x00842ba0`, `0x00842c00`.
  - `lighting_list_current`: `0x00537e20`, `0x00537e28`.
- Pointer-target facts:
  - `0x00b885a0` names `INTRO`; `0x00b85810` and `0x00b886b0` name
    `VERSECHORUSSOLO`. All three have a changing float-like row at `+0x04`,
    pointer rows to blocks around `0x0149bbe0..0x014b29fc`, and category or
    keyframe list pointers around `0x007ee410..0x007f8c6c`.
  - The `0x0072be50..0x0072be70` range contains live link/list rows pointing
    back to the preset records, plus function/code-looking values
    `0x0044d630` and `0x002bfaf0`, unity floats, and target/list rows such as
    `0x007d2c70`, `0x007d2cc0`, and `0x007ce440`.
  - `0x007fe790` changed color-like float rows at offsets `+0x10..+0x18`
    (`0x3e8e8e8f`, `0x3da8a8a9`, `0x3ed0d0d1` style values), while the first
    pointer row moved between `0x00845ca0`, `0`, and `0x00782580`.
  - `lighting_change_cursor` rows are authored script/message records naming
    `blackout`, `color1`, `color2`, `lighting`, `section`, `intro`, `verse`,
    and `music_start`.
  - `lighting_list_current` points at rows with float-like timing values and
    strings under `0x005456cd..0x0054579e`, including `.dta`.
- Interpretation: `0x00b78418 + 0x60` is the active/next LightPreset-style
  record pointer. The `0x0072be50..0x0072be70` rows look like the live
  iterator/list around preset application. `0x007fe790` is the best current
  candidate for mutable render-light/color state because it changes
  normalized float rows while the script graph records remain static.

Lighting apply downstream static and runtime notes:

- Static snippet reports:
  - `ps2_function_snippets_lighting_pointer_candidates_20260611.json`.
  - `ps2_function_snippets_lighting_apply_downstream_20260611.json`.
- Static facts:
  - `0x00280f60` reads timing from `0x0052ee40` via `0x002c6808`, then
    updates the list/global block at `0x004957b0`.
  - `0x00280f60` has a call to `0x003b50e0` after the `0x004957b0` list
    update path.
  - `0x002c6808` is a tiny leaf function beginning with `lw; jr ra`, so it
    must not be trusted as a normal entry-hook target with the current
    call-sequence tracer.
  - The literal function pointer `0x002bfaf0` is a no-op `jr ra`; the active
    body starts at `0x002bfaf8`, but live pointer rows using `0x002bfaf0`
    should be treated as a no-op/default callback slot unless runtime evidence
    proves otherwise.

Accepted lighting apply downstream sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_lighting_apply_downstream_sequence_20260611.json`.
- Screenshot: `pcsx2_lighting_apply_downstream_sequence_20260611.window.png`.
- Command:
  `python tools/trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 3 --seconds 30 --ring-size 16384 --log pcsx2_lighting_apply_downstream_sequence_20260611.log --out pcsx2_lighting_apply_downstream_sequence_20260611.json --target lighting_set_00271288=0x00271288 --target lighting_prev_002716b8=0x002716b8 --target lighting_apply_00280f60=0x00280f60 --target timing_read_002c6808=0x002c6808 --target list_commit_003b50e0=0x003b50e0 --target list_callback_002bfaf8=0x002bfaf8 --target generic_release_002d1c88=0x002d1c88 --target generic_link_002c1208=0x002c1208 --target generic_compare_002c1df8=0x002c1df8 --target script_eval_002b6238=0x002b6238 --target script_do_002b3118=0x002b3118 --target script_filter_002b31b0=0x002b31b0 --target script_list_002b3818=0x002b3818 --target script_pick_new_002b3d50=0x002b3d50`
- Screenshot gate: accepted active in-song gameplay, but interpreter speed was
  slow in the screenshot. `EnableEE = true` was verified afterward and no
  PCSX2 process was left running.
- Retained counts:
  - `lighting_set_00271288`: `5`.
  - `lighting_prev_002716b8`: `1`.
  - `lighting_apply_00280f60`: `1`.
  - `generic_release_002d1c88`: `6475`.
  - `generic_compare_002c1df8`: `508`.
  - `script_eval_002b6238`: `23`.
  - `script_do_002b3118`: `71`.
  - `script_filter_002b31b0`: `823`.
  - `script_list_002b3818`: `3`.
  - `script_pick_new_002b3d50`: `48`.
  - Zero retained calls for `0x002c6808`, `0x003b50e0`, `0x002bfaf8`, and
    `0x002c1208`.
- Interpretation:
  - The trace re-proves the live order
    `script_list` -> `script_pick_new` -> `0x002716b8` -> `0x00280f60` for
    the sampled keyframe branch.
  - The zero count for `0x002c6808` is not proof it did not execute because
    its body starts with `lw; jr ra`, which is incompatible with the current
    entry-hook tracer.
  - The zero count for `0x003b50e0` means the sampled `0x00280f60` path did
    not visibly reach that list-commit branch under this hook/window, or the
    static branch was skipped. Treat it as unproven, not disproven.
  - Generic release/compare helpers are too hot for precise lighting-local
    conclusions in this mixed trace.

Accepted lighting color/render-state candidate sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_lighting_color_candidate_objects_20260611.json`.
- Screenshots:
  `pcsx2_lighting_color_candidate_objects_20260611.before_sample.window.png`
  and `pcsx2_lighting_color_candidate_objects_20260611.window.png`.
- Command:
  `python tools/sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 3 --seconds 10 --interval 0.25 --no-default-targets --target color_candidate_007fe790=0x007fe790:0x240 --target color_alt_00782580=0x00782580:0x240 --target color_head_00845ca0=0x00845ca0:0x180 --target release_obj_008457e0=0x008457e0:0x180 --target release_obj_0084f360=0x0084f360:0x180 --target release_obj_00845be0=0x00845be0:0x180 --target world_light_state_00b78418=0x00b78418:0x240 --target lighting_list_004957b0=0x004957b0:0x240 --target timer_global_0052ee40=0x0052ee40:0x100 --log pcsx2_lighting_color_candidate_objects_20260611.log --out pcsx2_lighting_color_candidate_objects_20260611.json`
- Screenshot gate: accepted active in-song gameplay at 60 FPS/VPS.
  `EnableEE = true` was verified afterward and no PCSX2 process was left
  running.
- Changed row counts:
  - `color_candidate_007fe790`: `6`.
  - `color_alt_00782580`: `8`.
  - `color_head_00845ca0`: `4`.
  - `release_obj_008457e0`: `0`.
  - `release_obj_0084f360`: `7`.
  - `release_obj_00845be0`: `28`.
  - `world_light_state_00b78418`: `8`.
  - `lighting_list_004957b0`: `1`.
  - `timer_global_0052ee40`: `0`.
- Important rows:
  - `0x007fe790 + 0x10/+0x14/+0x18` carry normalized RGB-like values. In this
    sample they moved through values such as `0.2784`, `0.1059`,
    `0.07966`, `0.04314`, `0.08235`, `0.2588`, `0.3211`, and `0.4078`.
  - `0x00845ca0 + 0x10/+0x14/+0x18` mirrors the same color payload shape and
    moved through values such as `0.298`, `0.231`, `0.3504`, `0.698`,
    `0.4818`, `0.159`, `0.06118`, and `0.1916`.
  - `0x00782580` contains `screen_change` and also exposes RGB-like rows at
    `+0x10..+0x18`, plus links to runtime rows such as `0x00b88180`,
    `0x008466e0`, and `0x00850110`.
  - `0x00845be0` overlaps authored venue/stream/script data. Initial rows
    include `one_bar_to` and a path resolving as
    `world/battle/streams/crowd_v1_3norm.vgs`, but rows from
    `0x00845cb0..0x00845cb8` still carry the same RGB-like payload when the
    pointer head advances through this region.
  - `0x0084f360` is a moving script/message list region naming events such as
    `band_jump`, `game_over`, `game_lost`, `game_won_msg`, `crowd_lighters_slow`,
    `downbeat`, and `excitement`.
- Interpretation: `0x007fe790`, `0x00782580`, and `0x00845ca0` are now
  trace-backed mutable lighting/color records. The PS2 lighting path does not
  only select symbolic presets; it advances preset/category state and mutates
  normalized render-color rows during active gameplay.

Venue/crowd static xrefs and accepted world-message trace:

- Community metadata context:
  - `_community_re/Guitar-Hero-II-Deluxe-Unified/_ark/world/crowd.dta`
    defines crowd messages such as `crowd_update`, `crowd_hide`,
    `crowd_lighters_slow`, `crowd_lighters_fast`, `crowd_lighters_off`,
    `crowd_half_tempo`, `crowd_double_tempo`, and `crowd_normal_tempo`.
  - `world_objects.dta` and `world_objects_worldbase.dta` are the naming
    compass for `WorldCrowd`, `WorldDir`, `LightPreset`, `beat`, `downbeat`,
    and `one_bar_to`.
- Static xref reports:
  - `ps2_world_crowd_string_xrefs_fresh_20260611.json`.
  - `ps2_crowd_venue_string_xrefs_fresh_20260611.json`.
  - `ps2_function_snippets_world_venue_message_handlers_20260611.json`.
- Static facts:
  - The literal crowd handler strings from `crowd.dta` are not present as
    direct loaded-SLUS strings in this slice, so the runtime path must be
    traced through script/message records rather than by naive string xref.
  - `WorldCrowd` and `LightAnim` strings are present, but their xrefs appear
    registration/class-like rather than complete runtime poller proof.
  - `one_bar_to` xref/body area around `0x00122c44` has nearby calls through
    generic script helpers such as `0x002b06d0`, `0x00307bc0`,
    `0x002d3a48`, and `0x002b8298`.
  - `downbeat` and `beat` xref/body areas around `0x001221ac`,
    `0x00121ec4`, and `0x00121ff8` are likewise script/message-related
    locations, not yet clean standalone per-frame pollers.
  - `band_jump` has xref/body candidates at `0x0010bc54` and `0x0010cb9c`.
  - `0x00123d08`, `0x00123c28`, and `0x00124310` have function prologues and
    looked like useful world/event/game-related targets for live tracing.

Accepted world/venue message sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_world_venue_message_sequence_20260611.json`.
- Screenshot: `pcsx2_world_venue_message_sequence_20260611.window.png`.
- Command:
  `python tools/trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 3 --seconds 30 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_venue_message_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_venue_message_sequence_20260611.json" --target world_beat_ref_00121ec4=0x00121ec4 --target world_beat_ref_00121ff8=0x00121ff8 --target world_downbeat_ref_001221ac=0x001221ac --target world_onebar_ref_00122c44=0x00122c44 --target excitement_ref_001089dc=0x001089dc --target excitement_runtime_001099d0=0x001099d0 --target band_jump_ref_0010bc54=0x0010bc54 --target band_jump_runtime_0010cb9c=0x0010cb9c --target world_event_00123d08=0x00123d08 --target world_event_001239d0=0x001239d0 --target world_win_00123c28=0x00123c28 --target world_game_001243f8=0x001243f8 --target world_game_00124310=0x00124310 --target script_eval_002b6238=0x002b6238 --target script_do_002b3118=0x002b3118 --target script_filter_002b31b0=0x002b31b0 --target script_list_002b3818=0x002b3818 --target script_pick_new_002b3d50=0x002b3d50`
- Screenshot gate: accepted active in-song venue/gameplay frame. Interpreter
  speed was slow in the screenshot, but the capture is not Retry, fail menu,
  startup error, or the wrong window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Retained counts:
  - `world_onebar_ref_00122c44`: `1`.
  - `world_event_00123d08`: `2`.
  - `world_win_00123c28`: `1`.
  - `world_game_00124310`: `1`.
  - `script_eval_002b6238`: `30`.
  - `script_do_002b3118`: `75`.
  - `script_filter_002b31b0`: `871`.
  - `script_list_002b3818`: `3`.
  - `script_pick_new_002b3d50`: `51`.
  - Zero retained calls for `world_beat_ref_00121ec4`,
    `world_beat_ref_00121ff8`, `world_downbeat_ref_001221ac`,
    `excitement_ref_001089dc`, `excitement_runtime_001099d0`,
    `band_jump_ref_0010bc54`, `band_jump_runtime_0010cb9c`,
    `world_event_001239d0`, and `world_game_001243f8`.
- Ordering and live argument facts:
  - The `one_bar_to` xref/body hit occurred after script picks on
    `a1=0x005f77e0` with live args `a2=0x005235d8` and runtime objects
    `a3=0x00b8e1b0` / `0x00b8ba70`, then called
    `world_onebar_ref_00122c44(0x01ffe894, 0x003fb0a8, 0, 0x01ffe028)`.
  - A world event branch used
    `world_event_00123d08(0x00ad2aa0, 0x00550d62, 0x0060ba20, 0x10)`, then
    immediately called
    `world_game_00124310(0x00ad2aa0, 0x00b94bc0, 0x00c9d060, 0x0021eaf8)`,
    followed by script evals on `0x006329f0`, `0x00632f80`, and
    `0x007edaf0`.
  - A later branch ran lighting/worldbase-like script helpers on
    `0x006006e0`, `0x006006f0`, and `0x00600760`, then
    `world_win_00123c28(0x00ad2aa0, 2, 1, 0)`, then
    `world_event_00123d08(0x00ad2aa0, 0x00550d62, 0x0060ba20, 0x10)`.
  - Near the end of the retained ring a denser script branch touched
    `0x006025e0`, `0x00600da0`, `0x006011b0`, `0x00601570`,
    `0x00601580`, `0x00601610`, `0x006016a0`, `0x00601780`,
    `0x0060a870`, and `0x0060ab40`, with live runtime pointers including
    `0x00b784f0`, `0x00848d80`, `0x0084ae80`, and `0x0073d3d0`.
- Interpretation: this is the first accepted live trace tying the venue/world
  message xref bodies to in-song script/message execution. It proves that
  `one_bar_to` and the `0x00123d08`/`0x00123c28`/`0x00124310` world-event
  targets are active in the performance slice, but it does not yet prove the
  exact `beat`/`downbeat`, crowd-lighter, venue-animation, or `band_jump`
  handler bodies. The next useful work is to sample the live objects from this
  trace (`0x00ad2aa0`, `0x00b94bc0`, `0x00c9d060`, `0x0060ba20`,
  `0x00848d80`, `0x0084ae80`, `0x0073d3d0`, and nearby script nodes) and
  then follow the downstream non-leaf calls from the nonzero world-event
  targets.

Accepted world/venue argument object sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_world_venue_arg_objects_20260611.json`.
- Screenshots:
  `pcsx2_world_venue_arg_objects_20260611.before_sample.window.png` and
  `pcsx2_world_venue_arg_objects_20260611.window.png`.
- Command:
  `python tools/sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 3 --seconds 10 --interval 0.25 --no-default-targets --target world_event_owner_00ad2aa0=0x00ad2aa0:0x240 --target world_game_arg1_00b94bc0=0x00b94bc0:0x240 --target world_game_arg2_00c9d060=0x00c9d060:0x240 --target world_event_node_0060ba20=0x0060ba20:0x200 --target venue_runtime_00848d80=0x00848d80:0x240 --target venue_runtime_0084ae80=0x0084ae80:0x240 --target venue_runtime_0073d3d0=0x0073d3d0:0x240 --target world_light_event_00b784f0=0x00b784f0:0x200 --target script_node_006025e0=0x006025e0:0x180 --target script_node_00600da0=0x00600da0:0x180 --target script_node_006011b0=0x006011b0:0x180 --target script_node_00601570=0x00601570:0x180 --target script_node_00601780=0x00601780:0x180 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_venue_arg_objects_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_venue_arg_objects_20260611.json"`
- Screenshot gate: accepted active in-song full-stage gameplay at 60 FPS/VPS.
  `EnableEE = true` was verified afterward and no PCSX2 process was left
  running.
- Changed row counts:
  - `world_event_owner_00ad2aa0`: `5`.
  - `world_game_arg1_00b94bc0`: `4`.
  - `world_game_arg2_00c9d060`: `36`.
  - `venue_runtime_00848d80`: `4`.
  - `venue_runtime_0084ae80`: `12`.
  - `world_event_node_0060ba20`, `venue_runtime_0073d3d0`,
    `world_light_event_00b784f0`, and sampled script nodes:
    `0`.
- Important rows:
  - `0x00ad2aa0` is a world/event owner-style object. Its initial rows include
    class/table-looking `0x003e47a8`, self pointer `0x00ad2aa0`,
    `crowd_audio` at `0x006d2cf0`, and changed rows at `+0x20`,
    `+0x28`, `+0x40`, `+0x48`, and `+0xa0`.
  - `0x00ad2aa0 + 0xa0` changed from `0x00b94ae0` to `0x00b94bc0`,
    matching the `world_game_arg1` object reached by the live event trace.
  - `0x00b94bc0` names `crowd_upto_norm` and has the same table/header shape
    as the alternate `0x00b94ae0` object, which names `crowd_begin`.
  - `0x00c9d060` is a large moving game/world state block. It carries
    stream/audio/list pointers; `+0x10c` moved through crowd stream path
    pointers including `world/battle/streams/crowd_v1_0intro.vgs` and
    `world/battle/streams/crowd_v1_3norm.vgs`.
  - `0x0060ba20` is static authored script data naming `crowd_v1_4good`.
  - `0x00848d80` and `0x0084ae80` are hot venue/runtime list regions with
    changing pointer rows. `0x0084ae80` rows link to `0x008466e0`,
    `0x00842010`, `0x00845ca0`-style lighting/color regions seen earlier,
    and many list/scratch rows.
  - `0x0073d3d0` is stable in this sample and points back to the live
    `0x00c9ba80` world state pocket plus locale/path rows.
  - `0x00b784f0` is a stable `world` event/light object in this sample.
  - Script nodes `0x006025e0`, `0x00600da0`, `0x006011b0`, `0x00601570`,
    and `0x00601780` are static `world/world_objects_worldbase.dtb` graph
    nodes. They expose atoms such as `active_players_changed`,
    `current_shot`, and `push_back`.
- Interpretation: the world-event branch is touching real crowd and venue
  state, including crowd audio/stream transitions and hot venue list rows. The
  authored script nodes stay static, while live state changes appear in
  owner/event objects and moving list records.

Accepted world/venue pointer-target sample:

- Tool: `tools/sample_pcsx2_pointer_targets.py`.
- Report: `pcsx2_world_venue_pointer_targets_20260611.json`.
- Screenshots:
  `pcsx2_world_venue_pointer_targets_20260611.before_pointer.window.png` and
  `pcsx2_world_venue_pointer_targets_20260611.window.png`.
- Command:
  `python tools/sample_pcsx2_pointer_targets.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 3 --seconds 10 --interval 0.25 --object-size 0x120 --cell owner_current_ptr_00ad2ac0=0x00ad2ac0 --cell owner_game_arg_ptr_00ad2b40=0x00ad2b40 --cell game_arg1_ptr_00b94be8=0x00b94be8 --cell game_arg1_ptr_00b94c24=0x00b94c24 --cell game_arg1_ptr_00b94cc0=0x00b94cc0 --cell game_arg2_stream_ptr_00c9d16c=0x00c9d16c --cell game_arg2_list_a_00c9d174=0x00c9d174 --cell game_arg2_list_b_00c9d178=0x00c9d178 --cell venue_00848d80_ptr0=0x00848d80 --cell venue_00848d80_ptr4=0x00848d84 --cell venue_00848d80_ptr8=0x00848d88 --cell venue_0084ae80_ptr0=0x0084ae80 --cell venue_0084ae80_ptr4=0x0084ae84 --cell venue_0084ae80_ptr8=0x0084ae88 --cell venue_0084ae80_ptr40=0x0084aec0 --cell venue_0084ae80_ptr48=0x0084aec8 --cell venue_0084ae80_ptr50=0x0084aed0 --cell venue_0084ae80_ptr60=0x0084aee0 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_venue_pointer_targets_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_venue_pointer_targets_20260611.json"`
- Screenshot gate: accepted active in-song full-stage gameplay at 60 FPS/VPS.
  `EnableEE = true` was verified afterward and no PCSX2 process was left
  running.
- Important pointer facts:
  - `0x00ad2ac0` alternated between `0x00747050` and `0x00850ec0`; both are
    list/message records pointing back to the world/event owner.
  - `0x00ad2b40` alternated between `0x00b94ae0` (`crowd_begin`) and
    `0x00b94bc0` (`crowd_upto_norm`).
  - `0x00b94be8` alternated between `0x00474254` and `0x00850f90`; the latter
    is a moving list/message record pointing back to `0x00b94be8`.
  - `0x00b94cc0` moved through `0x00782800`, `0x00853710`, and
    `0x00853880`, all list/queue-like structures tied to the crowd state.
  - `0x00c9d16c` is the clearest crowd stream cell. It moved through
    `0x00845be0`, `0x0083fdd0`, and `0x00845b20`, resolving to
    `world/battle/streams/crowd_v1_0intro.vgs` and
    `world/battle/streams/crowd_v1_3norm.vgs`.
  - `0x00c9d174` / `0x00c9d178` walked list rows such as `0x00754870`,
    `0x007434b0`, and `0x00745660`; these rows contain pointers to live
    character/camera/venue records plus static callback-ish cells such as
    `0x0044d630`.
  - Hot venue cells `0x00848d80`, `0x00848d84`, `0x0084ae80`,
    `0x0084ae84`, `0x0084aec0`, and related rows walked many list records.
    Notable derefs include `band_jump` at `0x0084f38c`,
    `game_won_msg` at `0x0074bd3c`, and `downbeat` at `0x0083e23c`.
  - Several `venue_*_ptr8` and `venue_*_ptr48` cells contain encoded
    selector/float rows as well as EE pointers, so they should not be modeled
    as simple pointers without field-level proof.
- Interpretation: crowd stream switching is now trace-backed to the live
  world/game state block at `0x00c9d060 + 0x10c`. `band_jump`,
  `game_won_msg`, and `downbeat` are visible in the same hot venue/event list
  region, but their exact handler dispatch functions are still open.

Limited world/venue downstream sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_world_venue_downstream_sequence_20260611.json`.
- Screenshot: `pcsx2_world_venue_downstream_sequence_20260611.window.png`.
- Command:
  `python tools/trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 3 --seconds 30 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_venue_downstream_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_venue_downstream_sequence_20260611.json" --target world_onebar_00122c44=0x00122c44 --target world_event_00123d08=0x00123d08 --target world_event_child_002d27d0=0x002d27d0 --target world_event_child_0031ac80=0x0031ac80 --target world_event_child_00123e38=0x00123e38 --target world_win_00123c28=0x00123c28 --target world_win_child_002aff10=0x002aff10 --target world_win_child_00123c60=0x00123c60 --target world_game_00124310=0x00124310 --target world_game_child_00223e60=0x00223e60 --target world_game_child_00124380=0x00124380 --target world_game_child_00223dc8=0x00223dc8 --target beat_ref_00121ec4=0x00121ec4 --target beat_ref_00121ff8=0x00121ff8 --target downbeat_ref_001221ac=0x001221ac --target band_jump_ref_0010bc54=0x0010bc54 --target band_jump_runtime_0010cb9c=0x0010cb9c --target band_jump_child_0010c730=0x0010c730 --target band_jump_child_0010cfa0=0x0010cfa0 --target script_eval_002b6238=0x002b6238 --target script_do_002b3118=0x002b3118 --target script_filter_002b31b0=0x002b31b0 --target script_list_002b3818=0x002b3818 --target script_pick_new_002b3d50=0x002b3d50`
- Screenshot gate: accepted active in-song stage gameplay. PCSX2 was running
  slowly in interpreter mode (`VPS 40`, `Speed 66%`), but the image is not a
  Retry/fail/startup/wrong-window capture. `EnableEE = true` was verified
  afterward and no PCSX2 process was left running.
- Retained counts:
  - `world_win_child_00123c60`: `1`.
  - `world_win_child_002aff10`: `1`.
  - All other targeted world, script, beat/downbeat, band_jump, and child
    targets retained zero calls in this window.
- Live args:
  - `world_win_child_00123c60(0x0069b960, 0x00558f6f, 0, 0x30)`.
  - `world_win_child_002aff10(0x00558f6f, 0x0054638e, 0, 0x401b)`.
- Interpretation: this confirms `0x00123c60` and `0x002aff10` can fire in an
  active song window, and static snippets show they are direct children of
  `0x00123c28`, but this trace did not capture parent `0x00123c28` firing in
  the same ring. Do not claim runtime child order for the win branch from this
  trace alone.

Rejected broad child sequence trace:

- Report: `pcsx2_world_venue_message_children_sequence_20260611.json`.
- Screenshot: `pcsx2_world_venue_message_children_sequence_20260611.window.png`.
- Rejection reason: the run used `--ring-size 32768` with default
  `--data-base 0x01f00000`, which exceeds the 32 MB EE RAM end
  (`0x01f00000 + 0x100 + 32768 * 32 > 0x02000000`). The saved JSON had
  `count 0` plus garbage `func_*` ids from scratch/ring contamination.
- Tool fix: `tools/trace_pcsx2_call_sequence.py` now rejects scratch data
  ranges that exceed 32 MB EE RAM, and its summary path tolerates unexpected
  record names instead of crashing before the saved evidence can be inspected.
- Do not use this rejected run as gameplay evidence.

Safe broad child sequence rerun:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_world_venue_message_children_sequence_safe_20260611.json`.
- Screenshot: `pcsx2_world_venue_message_children_sequence_safe_20260611.window.png`.
- Command used the same broad child target list as the rejected run but with
  valid `--ring-size 16384`.
- Screenshot gate: accepted active in-song stage gameplay, but the interpreter
  run was slow (`VPS 37`, `Speed 63%`). `EnableEE = true` was verified
  afterward and no PCSX2 process was left running.
- Retained counts: zero for all targeted world, child, script, beat/downbeat,
  and band_jump targets.
- Interpretation: negative phase evidence only. It proves that this patched
  target mix did not hit the branch in that window; it does not override the
  accepted parent-message traces.

Accepted world/message parent rerun:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_world_venue_message_sequence_rerun_20260611.json`.
- Screenshot: `pcsx2_world_venue_message_sequence_rerun_20260611.window.png`.
- Command:
  `python tools/trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 3 --seconds 30 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_venue_message_sequence_rerun_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_venue_message_sequence_rerun_20260611.json" --target world_beat_ref_00121ec4=0x00121ec4 --target world_beat_ref_00121ff8=0x00121ff8 --target world_downbeat_ref_001221ac=0x001221ac --target world_onebar_ref_00122c44=0x00122c44 --target excitement_ref_001089dc=0x001089dc --target excitement_runtime_001099d0=0x001099d0 --target band_jump_ref_0010bc54=0x0010bc54 --target band_jump_runtime_0010cb9c=0x0010cb9c --target world_event_00123d08=0x00123d08 --target world_event_001239d0=0x001239d0 --target world_win_00123c28=0x00123c28 --target world_game_001243f8=0x001243f8 --target world_game_00124310=0x00124310 --target script_eval_002b6238=0x002b6238 --target script_do_002b3118=0x002b3118 --target script_filter_002b31b0=0x002b31b0 --target script_list_002b3818=0x002b3818 --target script_pick_new_002b3d50=0x002b3d50`
- Screenshot gate: accepted active in-song gameplay. Interpreter speed was
  slow in the screenshot, but this is a valid non-Retry/non-fail capture.
  `EnableEE = true` was verified afterward and no PCSX2 process was left
  running.
- Retained counts:
  - `world_onebar_ref_00122c44`: `1`.
  - `world_event_00123d08`: `2`.
  - `world_win_00123c28`: `1`.
  - `world_game_00124310`: `1`.
  - `script_eval_002b6238`: `28`.
  - `script_do_002b3118`: `76`.
  - `script_filter_002b31b0`: `854`.
  - `script_list_002b3818`: `3`.
  - `script_pick_new_002b3d50`: `51`.
  - Zero retained calls for the beat/downbeat, excitement, band_jump, and
    alternate world-event candidates in this target set.
- Interpretation: this rerun reproduces the accepted parent branch exactly:
  `world_event_00123d08(0x00ad2aa0, 0x00550d62, 0x0060ba20, 0x10)` followed
  immediately by `world_game_00124310(0x00ad2aa0, 0x00b94bc0, 0x00c9d060,
  0x0021eaf8)`, and a later `world_win_00123c28(0x00ad2aa0, 2, 1, 0)` then
  `world_event_00123d08(...)`. This parent message path is reproducible.

Limited world-event child batch trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_world_event_children_sequence_20260611.json`.
- Screenshot: `pcsx2_world_event_children_sequence_20260611.window.png`.
- Screenshot gate: accepted active in-song stage gameplay. Interpreter speed
  was slow (`VPS 42`, `Speed 70%`). `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Retained counts:
  - `world_win_child_00123c60`: `1`.
  - `world_win_child_002aff10`: `1`.
  - All other targeted parent, child, and script targets retained zero calls.
- Interpretation: the child hook mix again only caught `0x00123c60` and
  `0x002aff10`, without parent `0x00123c28` or script helpers. Treat this as
  standalone active-call evidence only, not runtime child order.

Accepted isolated world-game child sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_world_game_children_isolated_20260611.json`.
- Screenshot: `pcsx2_world_game_children_isolated_20260611.window.png`.
- Command:
  `python tools/trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 3 --seconds 30 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_game_children_isolated_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_game_children_isolated_20260611.json" --target world_event_00123d08=0x00123d08 --target world_game_00124310=0x00124310 --target world_game_child_00223e60=0x00223e60 --target world_game_child_00124380=0x00124380 --target world_game_child_00223dc8=0x00223dc8 --target script_eval_002b6238=0x002b6238 --target script_do_002b3118=0x002b3118 --target script_filter_002b31b0=0x002b31b0 --target script_list_002b3818=0x002b3818 --target script_pick_new_002b3d50=0x002b3d50`
- Screenshot gate: accepted active in-song gameplay. Interpreter speed was
  slow in the screenshot. `EnableEE = true` was verified afterward and no
  PCSX2 process was left running.
- Retained counts:
  - `world_event_00123d08`: `2`.
  - `world_game_00124310`: `1`.
  - `world_game_child_00223e60`: `1`.
  - `world_game_child_00124380`: `3`.
  - `world_game_child_00223dc8`: `5`.
  - `script_eval_002b6238`: `30`.
  - `script_do_002b3118`: `74`.
  - `script_filter_002b31b0`: `875`.
  - `script_list_002b3818`: `3`.
  - `script_pick_new_002b3d50`: `51`.
- Local order around the event/game branch:
  - `script_pick_new_002b3d50` ->
    `script_do_002b3118` ->
    `world_event_00123d08(0x00ad2aa0, 0x00550d62, 0x0060ba20, 0x10)` ->
    pre-game `world_game_child_00124380(0x00ad2aa0, 4, 0x0022fc88,
    0x00c9d060)` ->
    `world_game_00124310(0x00ad2aa0, 0x00b94bc0, 0x00c9d060, 0x0021eaf8)` ->
    `world_game_child_00223e60(0x00b94ae0, 0x00b94bc0, 0x00c9d060,
    0x0021eaf8)` ->
    `world_game_child_00124380(0x00ad2aa0, 0x00b94bc0, 0x0084f7b0,
    0x00223150)` ->
    `world_game_child_00223dc8(0x00b94bc0, 0x006688c0, 0x0084f7b0,
    0x00223150)` ->
    script evals.
- Interpretation: this proves the direct child order for the `0x00124310`
  world-game/crowd event branch in the accepted active-song window.

Accepted isolated world-event auxiliary child sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_world_event_aux_children_isolated_20260611.json`.
- Screenshot: `pcsx2_world_event_aux_children_isolated_20260611.window.png`.
- Command:
  `python tools/trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 3 --seconds 30 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_event_aux_children_isolated_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_event_aux_children_isolated_20260611.json" --target world_event_00123d08=0x00123d08 --target world_event_child_002d27d0=0x002d27d0 --target world_event_child_0031ac80=0x0031ac80 --target world_event_child_00123e38=0x00123e38 --target world_win_00123c28=0x00123c28 --target world_game_00124310=0x00124310 --target script_eval_002b6238=0x002b6238 --target script_do_002b3118=0x002b3118 --target script_filter_002b31b0=0x002b31b0 --target script_list_002b3818=0x002b3818 --target script_pick_new_002b3d50=0x002b3d50`
- Screenshot gate: accepted active in-song gameplay. Interpreter speed was
  slow in the screenshot. `EnableEE = true` was verified afterward and no
  PCSX2 process was left running.
- Retained counts:
  - `world_event_00123d08`: `2`.
  - `world_event_child_002d27d0`: `2`.
  - `world_event_child_0031ac80`: `2`.
  - `world_event_child_00123e38`: `2`.
  - `world_win_00123c28`: `1`.
  - `world_game_00124310`: `1`.
  - `script_eval_002b6238`: `30`.
  - `script_do_002b3118`: `78`.
  - `script_filter_002b31b0`: `884`.
  - `script_list_002b3818`: `3`.
  - `script_pick_new_002b3d50`: `53`.
- Local order around the first event branch:
  - `script_pick_new_002b3d50` ->
    `script_do_002b3118` ->
    `world_event_00123d08(0x00ad2aa0, 0x00550d62, 0x0060ba20, 0x10)` ->
    `world_event_child_002d27d0(0x01ffe2a0, 0x00ad2aec, 0,
    0x00c9e500)` ->
    `world_event_child_0031ac80(0x003fb620, 0x01ffe2a0, 0x00550d62,
    0x0084f7b0)` ->
    `world_event_child_00123e38(0x00ad2aa0, 4, 0x0022fc88,
    0x00c9d060)` ->
    `world_game_00124310(0x00ad2aa0, 0x00b94bc0, 0x00c9d060,
    0x0021eaf8)`.
  - The later branch runs `world_win_00123c28(0x00ad2aa0, 2, 1, 0)` then
    the same `world_event_00123d08` -> `0x002d27d0` -> `0x0031ac80` ->
    `0x00123e38` auxiliary child chain.
- Interpretation: this proves the auxiliary child chain under
  `world_event_00123d08` in the active performance slice and links it to the
  `0x00124310` world-game/crowd event branch.

Limited isolated world-win child trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_world_win_children_isolated_20260611.json`.
- Screenshot: `pcsx2_world_win_children_isolated_20260611.window.png`.
- Screenshot gate: accepted active in-song stage gameplay, slow interpreter
  speed. `EnableEE = true` was verified afterward and no PCSX2 process was
  left running.
- Retained counts:
  - `world_win_child_00123c60`: `1`.
  - `world_win_child_002aff10`: `1`.
  - Parent `world_win_00123c28`, `world_event_00123d08`, and all script
    helpers retained zero calls in this window.
- Interpretation: this repeats the limited active-call evidence for
  `0x00123c60` and `0x002aff10`, but it still does not prove their runtime
  order under `0x00123c28`.

Accepted named venue record sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_named_venue_records_20260611.json`.
- Screenshots:
  `pcsx2_named_venue_records_20260611.before_sample.window.png` and
  `pcsx2_named_venue_records_20260611.window.png`.
- Command:
  `python tools/sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 3 --seconds 12 --interval 0.25 --no-default-targets --target venue_band_jump_record_0084f360=0x0084f360:0x240 --target venue_game_won_record_0074bd20=0x0074bd20:0x200 --target venue_downbeat_record_0083e230=0x0083e230:0x240 --target venue_downbeat_list_00853cf0=0x00853cf0:0x200 --target venue_list_00850110=0x00850110:0x240 --target venue_list_008466e0=0x008466e0:0x240 --target world_state_pocket_00c9ba80=0x00c9ba80:0x280 --target world_game_state_00c9d060=0x00c9d060:0x280 --target crowd_event_begin_00b94ae0=0x00b94ae0:0x160 --target crowd_event_norm_00b94bc0=0x00b94bc0:0x160 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_named_venue_records_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_named_venue_records_20260611.json"`
- Screenshot gate: accepted active in-song gameplay at 60 FPS/VPS.
  `EnableEE = true` was verified afterward and no PCSX2 process was left
  running.
- Changed row counts:
  - `venue_band_jump_record_0084f360`: `11`.
  - `venue_game_won_record_0074bd20`: `8`.
  - `venue_downbeat_record_0083e230`: `3`.
  - `venue_downbeat_list_00853cf0`: `13`.
  - `venue_list_00850110`: `4`.
  - `venue_list_008466e0`: `5`.
  - `world_state_pocket_00c9ba80`: `6`.
  - `world_game_state_00c9d060`: `39`.
  - `crowd_event_begin_00b94ae0`: `6`.
  - `crowd_event_norm_00b94bc0`: `4`.
- Important rows:
  - `0x0084f360` is a moving venue/event list record. Initial rows include
    `band_jump` at `0x0084f38c` and `game_over` at `0x0084f3ac`; changed rows
    include category strings `VERSECHORUS` and `VERSECHORUSSOLO`, plus pointer
    movement through `0x0084f420`, `0x0084f424`, `0x0084f428`, and
    `0x0084f440`.
  - `0x0074bd20` is a moving event row containing `game_won_msg` at
    `0x0074bd3c` and `sync_head_bang` at `0x0074bd7c`.
  - `0x0083e230` is a moving `downbeat` record with pointer changes at
    `+0x00` and `+0x04`.
  - `0x00853cf0` is a moving downbeat/list region with changing links back to
    `0x0084aec0`, `0x00853b30`, `0x007ff970`, `0x0074bdc0`, and other venue
    rows.
  - `0x00c9ba80` is the live world state pocket; the sample shows
    `ignored_last_light_change` toggling at `0x00c9ba98`, `camera_beat`
    advancing at `0x00c9baa8`, `camera_bars_left` changing at
    `0x00c9bad8`, and additional state rows at `0x00c9bae8`,
    `0x00c9bb08`, and `0x00c9bb18`.
  - `0x00c9d060 + 0x10c` again moved through crowd stream paths; this
    reproduces the crowd stream switching evidence.
  - `0x00b94ae0` / `0x00b94bc0` are trace-backed crowd event objects named
    `crowd_begin` and `crowd_upto_norm`. Both mutate queue/list rows around
    `+0x28`, `+0x100`, `+0x140`, and `+0x144`.
- Interpretation: venue/world runtime state is not just lighting/camera. The
  same active-song slice mutates crowd stream selection, named crowd event
  objects, world camera/light state, and hot venue event rows containing
  `band_jump`, `game_over`, `game_won_msg`, `sync_head_bang`, and `downbeat`.
  Exact handler identities for those named records remain open.

Accepted crowd/lighter linked-record sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_crowd_lighter_records_20260611.json`.
- Screenshots:
  `pcsx2_crowd_lighter_records_20260611.before_sample.window.png` and
  `pcsx2_crowd_lighter_records_20260611.window.png`.
- Command:
  `python tools/sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 3 --seconds 12 --interval 0.25 --no-default-targets --target crowd_downbeat_region_00845820=0x00845820:0x140 --target crowd_hide_region_00845830=0x00845830:0x140 --target crowd_update_region_00845850=0x00845850:0x140 --target crowd_lighters_slow_region_00845870=0x00845870:0x140 --target crowd_lighters_fast_region_00845890=0x00845890:0x140 --target crowd_lighters_off_region_00853900=0x00853900:0x140 --target crowd_hide_band_region_00743500=0x00743500:0x140 --target venue_event_region_0084f380=0x0084f380:0x180 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_crowd_lighter_records_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_crowd_lighter_records_20260611.json"`
- Screenshot gate: accepted active in-song gameplay at 60 FPS/VPS.
  `EnableEE = true` was verified afterward and no PCSX2 process was left
  running.
- Changed row counts:
  - Static in this 12-second slice: `crowd_downbeat_region_00845820`,
    `crowd_hide_region_00845830`, `crowd_update_region_00845850`,
    `crowd_lighters_slow_region_00845870`, and
    `crowd_lighters_fast_region_00845890`: `0`.
  - `crowd_lighters_off_region_00853900`: `6`.
  - `crowd_hide_band_region_00743500`: `3`.
  - `venue_event_region_0084f380`: `4`.
- Linked-record layout:
  - `0x00845830..0x00845920` is a linked crowd message block:
    - `0x00845838`: `crowd_hide`.
    - `0x00845858`: `crowd_update`.
    - `0x00845878`: `crowd_lighters_slow`.
    - `0x00845898`: `crowd_lighters_fast`.
    - `0x008458b8`: `crowd_lighters_off`.
    - `0x008458d8`: `crowd_half_tempo`.
    - `0x008458f8`: `crowd_double_tempo`.
    - `0x00845918`: `crowd_normal_tempo`.
  - The block links into venue/event rows: slow points through
    `0x0084f3f0` / `0x00742b20`; fast through `0x0084fa00` /
    `0x00742660`; off through `0x00853920` / `0x00746660`.
  - `0x00853900` contains `crowd_lighters_off`, then `game_outro_complete`,
    and changed rows at `0x00853950`, `0x00853954`, `0x00853958`,
    `0x00853a00`, `0x00853a04`, and `0x00853a08`.
  - `0x00743500` contains a compact event list with `crowd_hide`,
    `band_jump`, `game_outro_complete`, `downbeat`, and `hit_snare`; changed
    rows were `0x00743530`, `0x00743534`, and `0x00743538`.
  - `0x0084f380` contains the venue event block already seen in named-record
    sampling: `band_jump`, `game_over`, `game_lost`, `game_won_msg`,
    `crowd_lighters_slow`, `downbeat`, and `excitement`. Its changed rows in
    this sample were `0x0084f420`, `0x0084f424`, `0x0084f428`, and
    `0x0084f440`.
- Interpretation: crowd event/lighter/tempo messages are now trace-backed as
  linked live records even though the literal handler strings are absent from
  direct SLUS string xrefs. In this sample most message atoms were static
  records, while nearby list links changed. Handler dispatch identity remains
  open.

Static snippets for newly proven world-event children:

- Snippet report: `ps2_function_snippets_world_event_children_20260611.json`.
- Static facts:
  - `0x00123e38` calls `0x00124380`, then sums float rows from
    `world_event_owner + 0x28/+0x2c/+0x30`, then calls `0x00225450` on the
    object at `world_event_owner + 0x44`.
  - `0x00124380` performs one-time lookup/initialization through
    `0x002d3a48`, then continues through a global/list path. Runtime traces
    show it can fire both immediately before `0x00124310` and inside the
    `0x00124310` branch.
  - `0x00223e60` is a heavier prologue function operating on the two crowd
    event objects and world/game state passed by `0x00124310`.
  - `0x00223dc8` calls `0x00223fc0`, then `0x00223340`, using float arguments
    from the `0x00124310` branch.
  - `0x0022fc88` writes `f12` through an indexed pointer and then calls
    `0x0021eaf8` on the target at `+0x3c`.
  - `0x002d27d0` calls `0x002d22d8` and `0x002d2b48`, and initializes a
    stack/helper structure used by the following `0x0031ac80`.
  - `0x0031ac80` builds a large stack formatting/message frame through
    helpers including `0x002ccb28`, `0x002cd068`, `0x002ccf60`, and
    `0x002cd158`.
  - `0x002aff10` and `0x00123c60` have callable bodies, but runtime order
    under parent `0x00123c28` remains unproven.
  - `0x0044d630`, seen repeatedly in pointer samples, is not a handler body in
    the loaded SLUS range; the bytes read as data/ASCII-like rows such as
    `gfx_mode`/`system`.
- Interpretation: the event/game child functions are code-backed and their
  local runtime order is now known. Some pointer-sample cells that look like
  code addresses are actually data and must not be used as handler proof.

Accepted deeper world-event/game child sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_world_event_game_deeper_children_20260611.json`.
- Screenshot: `pcsx2_world_event_game_deeper_children_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 3 --seconds 30 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_event_game_deeper_children_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_event_game_deeper_children_20260611.json" --target world_event_00123d08=0x00123d08 --target world_event_child_00123e38=0x00123e38 --target world_game_00124310=0x00124310 --target world_game_phase_00124380=0x00124380 --target world_game_transition_00223e60=0x00223e60 --target world_game_apply_00223dc8=0x00223dc8 --target world_game_apply_child_00223fc0=0x00223fc0 --target world_game_apply_child_00223340=0x00223340 --target world_game_apply_child_00223400=0x00223400 --target world_game_apply_child_002232a8=0x002232a8 --target world_game_apply_child_002230c8=0x002230c8 --target world_event_apply_child_00225450=0x00225450 --target world_event_value_writer_0022fc88=0x0022fc88 --target world_event_value_target_0021eaf8=0x0021eaf8 --target world_event_aux_002d27d0=0x002d27d0 --target world_event_aux_0031ac80=0x0031ac80 --target script_eval_002b6238=0x002b6238 --target script_do_002b3118=0x002b3118 --target script_filter_002b31b0=0x002b31b0 --target script_list_002b3818=0x002b3818 --target script_pick_new_002b3d50=0x002b3d50`.
- Screenshot gate: accepted active in-song gameplay. The interpreter run was
  slow (`FPS/VPS 30`, speed `51%`), but the capture is not Retry, fail-menu,
  startup, or a wrong-window grab. `EnableEE = true` was verified afterward and
  no PCSX2 process was left running.
- Retained counts:
  - `world_event_00123d08`: `2`.
  - `world_event_child_00123e38`: `2`.
  - `world_game_00124310`: `1`.
  - `world_game_phase_00124380`: `3`.
  - `world_game_transition_00223e60`: `1`.
  - `world_game_apply_00223dc8`: `4`.
  - `world_game_apply_child_00223fc0`: `6`.
  - `world_game_apply_child_00223340`: `6`.
  - `world_game_apply_child_00223400`: `8`.
  - `world_game_apply_child_002232a8`: `2`.
  - `world_game_apply_child_002230c8`: `6`.
  - `world_event_apply_child_00225450`: `2`.
  - `world_event_value_writer_0022fc88`: `4`.
  - `world_event_value_target_0021eaf8`: `0`.
  - `world_event_aux_002d27d0`: `2`.
  - `world_event_aux_0031ac80`: `2`.
  - Script helpers: eval/do/filter/list/pick `30/75/860/3/51`.
- Local order around the first retained world-event branch:
  - `world_event_00123d08(0x00ad2aa0, 0x00550d62, 0x0060ba20, 0x10)` ->
    `0x002d27d0(0x01ffe2a0, 0x00ad2aec, 0, 0x00c9e500)` ->
    `0x0031ac80(0x003fb620, 0x01ffe2a0, 0x00550d62, 0x0084f7b0)` ->
    `0x0022fc88(0x00c9d060, 0, 0x0022fc88, 0x0083fdd0)` ->
    `0x0022fc88(0x00c9d060, 4, 0x0022fc88, 0x00c9d060)` ->
    `0x00123e38(0x00ad2aa0, 4, 0x0022fc88, 0x00c9d060)` ->
    `0x00124380(0x00ad2aa0, 4, 0x0022fc88, 0x00c9d060)` ->
    `0x00225450(0x00c9d060, 0x006688c0, 0x0022fc88, 0x00c9d060)` ->
    `world_game_00124310(0x00ad2aa0, 0x00b94bc0, 0x00c9d060, 0x0021eaf8)`.
- Local order inside the game/apply branch:
  - `0x00124310` ->
    `0x00223e60(0x00b94ae0, 0x00b94bc0, 0x00c9d060, 0x0021eaf8)` ->
    `0x00124380(0x00ad2aa0, 0x00b94bc0, 0x0084f7b0, 0x00223150)` ->
    `0x00223dc8(0x00b94bc0, 0x006688c0, 0x0084f7b0, 0x00223150)` ->
    `0x00223fc0(0x00b94bc0, 0x006688c0, 0x0084f7b0, 0x00223150)` ->
    `0x00223340(0x007997d0, 0x0c, 0x00456a18, 0x00854140)` ->
    `0x002230c8(0x007997d0, stack, 0xffffffff, 0x00854140)` ->
    `0x00223400(0x007997d0, stack, 0xffffffff, 0x00854140)`.
- Other retained helper bursts outside the main branch reached list/event
  objects such as `0x0083e230`, `0x0084f260`, `0x0084ada0`, and `0x00848c20`.
  These are trace-backed runtime arguments, but this trace does not yet prove
  the exact named dispatch identity for `downbeat`, `band_jump`, or
  `crowd_lighters_*`.
- Interpretation: the previously proven world-event/world-game branch now has
  a trace-backed deeper application path through the list/value helper layer:
  `0x0022fc88` writes values for the event branch, `0x00225450` applies the
  owner/world state side, and `0x00223dc8` fans into `0x00223fc0`,
  `0x00223340`, `0x00223400`, `0x002232a8`, and `0x002230c8` for event/list
  target updates. `0x0021eaf8` is a tiny dirty/valid flag writer from static
  snippets, but it recorded zero direct hits in this patched trace because
  patching the callee at the same time as its caller changed the direct-call
  observation point.

Static snippets for deeper world-event/game children:

- Snippet report:
  `ps2_function_snippets_world_event_game_deeper_children_20260611.json`.
- Static facts:
  - `0x00223fc0` initializes or prepares a per-event/list object, calls
    `0x0022b8f8`, allocates `0x80` bytes through `0x002d1c00`, and then calls
    `0x00222e28`.
  - `0x00223340` walks a child list at `a0 + 0x68` and calls `0x0022c1f0` on
    each child with the incoming float.
  - `0x00223400` writes the incoming float to `a0 + 0x28` when not locked, then
    walks the same `a0 + 0x68` child list.
  - `0x002232a8` calls `0x0022b7f0`, then forwards the returned float to
    `0x00223288`.
  - `0x002230c8` calls `0x00223400`, then `0x002232d8`, then walks list rows
    at `a0 + 0x68` and calls `0x0022c1a0` style child handlers.
  - `0x00225450` interns a symbol once through `0x002d3a48`, uses incoming
    float state, and is now runtime-proven immediately after `0x00124380` in
    the world-event branch.
  - `0x0022fc88` indexes through `a0 + 0x08`, writes `f12` to the pointed
    value row, then calls `0x0021eaf8` on target `+0x3c`.
  - `0x0021eaf8` only sets word `+0x14` to `1`; the following function at
    `0x0021eb08` clears the same word to `0`.
  - `0x00223150`, passed as an argument in the game branch, walks the
    `a0 + 0x68` child list and calls `0x0022c1a0` on each child.
  - Static dump attempts at runtime data addresses such as `0x0083e230`,
    `0x0084f260`, and `0x00854140` correctly produce no callable function
    body; treat them as live data/list objects, not code.

Accepted world-event/list child callback trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_world_event_list_child_callbacks_20260611.json`.
- Screenshot: `pcsx2_world_event_list_child_callbacks_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 3 --seconds 30 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_event_list_child_callbacks_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_event_list_child_callbacks_20260611.json" --target world_event_00123d08=0x00123d08 --target world_game_00124310=0x00124310 --target world_game_apply_00223dc8=0x00223dc8 --target list_prepare_00223fc0=0x00223fc0 --target list_prepare_child_0022b8f8=0x0022b8f8 --target list_prepare_child_00222e28=0x00222e28 --target list_float_walker_00223340=0x00223340 --target list_float_child_0022c1f0=0x0022c1f0 --target list_value_setter_00223400=0x00223400 --target list_value_child_0022c2c0=0x0022c2c0 --target list_derived_setter_002232a8=0x002232a8 --target list_derived_source_0022b7f0=0x0022b7f0 --target list_derived_sink_00223288=0x00223288 --target list_update_bridge_002230c8=0x002230c8 --target list_update_child_002232d8=0x002232d8 --target list_update_child_0022c168=0x0022c168 --target list_walk_leaf_0022c1a0=0x0022c1a0 --target world_event_apply_child_00225450=0x00225450 --target world_event_apply_child_0021eb10=0x0021eb10 --target world_event_apply_child_0021e1b8=0x0021e1b8 --target script_do_002b3118=0x002b3118 --target script_pick_new_002b3d50=0x002b3d50`.
- Screenshot gate: accepted active in-song gameplay. The interpreter run was
  slow (`FPS/VPS 29`, speed `48%`), but the capture is valid active gameplay,
  not Retry/fail/startup/wrong-window. `EnableEE = true` was verified
  afterward and no PCSX2 process was left running.
- Retained counts:
  - `world_event_00123d08`: `2`.
  - `world_game_00124310`: `1`.
  - `world_game_apply_00223dc8`: `4`.
  - `list_prepare_00223fc0`: `6`.
  - `list_prepare_child_0022b8f8`: `22`.
  - `list_prepare_child_00222e28`: `6`.
  - `list_float_walker_00223340`: `6`.
  - `list_float_child_0022c1f0`: `8`.
  - `list_value_setter_00223400`: `8`.
  - `list_value_child_0022c2c0`: `10`.
  - `list_derived_setter_002232a8`: `2`.
  - `list_derived_source_0022b7f0`: `10`.
  - `list_derived_sink_00223288`: `2`.
  - `list_update_bridge_002230c8`: `6`.
  - `list_update_child_002232d8`: `8`.
  - `list_update_child_0022c168`: `8`.
  - `list_walk_leaf_0022c1a0`: `8`.
  - `world_event_apply_child_00225450`: `2`.
  - `world_event_apply_child_0021eb10`: `83`.
  - `world_event_apply_child_0021e1b8`: `110`.
  - Script helpers: `script_do_002b3118`: `74`, `script_pick_new_002b3d50`:
    `51`.
- Local order in the first retained list branch:
  - `0x00223dc8(0x016184f0, 0x00534428, 1, 0x016184f0)` ->
    `0x00223fc0(0x016184f0, 0x00534428, 1, 0x016184f0)` ->
    `0x0022b8f8(0x01618518, 0x00534428, 1, 0x016184f0)` ->
    `0x00222e28(0x0072bd50, 0x016184f0, 0x00432aa8, 0x0072bd50)` ->
    `0x0022b7f0(...)` ->
    `0x00223340(0x0072bd50, 0x0c, 0x00456a18, 0x00821170)` ->
    `0x0022c1f0(0x00768a10, 0x0c, 0x00456a18, 0x00821170)` ->
    `0x002230c8(0x0072bd50, stack, 0xffffffff, 0x00821170)` ->
    `0x00223400(0x0072bd50, stack, 0xffffffff, 0x00821170)` ->
    `0x0022c2c0(0x00768a10, stack, 0xffffffff, 0x00821170)` ->
    `0x002232d8(0x0072bd50, stack, 0xffffffff, 0x41000000)` ->
    `0x0022c168(0x00768a10, stack, 0xffffffff, 0x41000000)` ->
    `0x0022c1a0(0x00768a10, stack, 0xffffffff, 0x41000000)`.
- Local order for a venue/list burst:
  - `0x00223fc0(0x00b94a00, 0x00221b78, 0x00456a18, 0x0084f260)` ->
    `0x0022b8f8(0x00b94a28, 0x00221b78, 0x00456a18, 0x0084f260)` ->
    `0x00222e28(0x0082d790, 0x00b94a00, 0x00432aa8, 0x0082d790)` ->
    `0x00223340(0x0082d790, 0x0c, 0x00456a18, 0x0083d4c0)` ->
    `0x0022c1f0(0x0076bcd0, 0x0c, 0x00456a18, 0x0083d4c0)` ->
    `0x002232a8(0x0082d790, stack, 0xffffffff, 0x0083d4c0)` ->
    `0x0022b7f0(0x0082d790, stack, 0xffffffff, 0x0083d4c0)` ->
    `0x00223288(0x0082d790, 0x3f7ab07b, 0x7fffffff, 0xbcf7c000)` ->
    `0x002232d8(0x0082d790, 0x3f7ab07b, 0x7fffffff, 0xbcf7c000)` ->
    `0x00223400(0x0082d790, stack, 0xffffffff, 0xbcf7c000)` ->
    `0x0022c2c0(0x0076bcd0, stack, 0xffffffff, 0xbcf7c000)` ->
    `0x002230c8(0x0082d790, stack, 0xffffffff, 0x41000000)` ->
    `0x0022c168(0x0076bcd0, stack, 0xffffffff, 0x41000000)` ->
    `0x0022c1a0(0x0076bcd0, stack, 0xffffffff, 0x41000000)`.
- `0x0021eb10` and `0x0021e1b8` are much hotter than the local event branch.
  Around `world_event_00123d08`, `0x0021e1b8` walks many world/game-state rows
  from roughly `0x00c9d230..0x00c9e310` with event/list targets in `a3`.
  Immediately after `0x00225450`, the chain includes
  `0x0021eb10(0x00c1c420, 0x0055cf91, 1, 0x00c9d060)` then repeated
  `0x0021e1b8` / `0x0021eb10` pairs on the `0x00c9dxxx` rows.
- Interpretation: the generic event/list layer is now trace-backed below the
  previous parent helpers. `0x00223fc0` prepares list/object records,
  `0x00222e28` builds/initializes the `0x0072bd50` / `0x0082d790` style list
  container, `0x00223340` dispatches float updates to per-child objects through
  `0x0022c1f0`, `0x00223400` dispatches value updates through `0x0022c2c0`,
  and `0x002230c8` / `0x002232d8` / `0x0022c168` / `0x0022c1a0` drive the
  child update/leaf callbacks. This still does not name the final dynamic
  `jalr` targets inside `0x0022c1a0`, `0x0022c168`, or `0x0022c2c0`; those
  must be resolved from live child object vtables/descriptor rows next.

Static snippets for world-event/list child callbacks:

- Snippet report:
  `ps2_function_snippets_world_event_list_child_callbacks_20260611.json`.
- Static facts:
  - `0x0022b8f8` prepares a list/object row and calls helper paths after
    checking an internal global/list sentinel.
  - `0x00222e28` initializes a list container: it calls `0x002c1028`, writes a
    vtable-like row `0x003fdec0`, sets defaults around `+0x28..+0x40`, and is
    runtime-proven with containers such as `0x0072bd50`, `0x0072bf50`, and
    `0x0082d790`.
  - `0x0022c1f0` calls `0x002db220`, writes the resulting float to child
    `+0x00`, then calls `0x0022c280`.
  - `0x0022c2c0` writes incoming `f12` to child `+0x0c`, adds child `+0x10`,
    then dispatches through a descriptor function pointer at descriptor
    `+0x5c`.
  - `0x0022b7f0` derives a normalized/sinusoidal float through `0x00300018`.
  - `0x00223288` stores `f12` to container `+0x2c` and calls `0x002232d8`.
  - `0x002232d8` walks children at container `+0x68` and calls `0x0022c330`
    with `container +0x2c` multiplied by `container +0x60`.
  - `0x0022c168` calls `0x0022c1a0`, then dispatches through descriptor
    `+0x44`.
  - `0x0022c1a0` dispatches through descriptor `+0x14`, and conditionally
    through descriptor `+0x4c`.
  - `0x0021eb10` walks/searches an object/list chain against a symbol in `a1`.
  - `0x0021e1b8` calls `0x0021e430` and `0x0021e450` on the same object while
    preserving incoming float state.

Accepted live list-child object sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_world_event_list_child_objects_20260611.json`.
- Screenshots:
  `pcsx2_world_event_list_child_objects_20260611.before_sample.window.png`
  and `pcsx2_world_event_list_child_objects_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 3 --seconds 12 --interval 0.25 --no-default-targets --target list_child_00768a10=0x00768a10:0x100 --target list_child_00770d90=0x00770d90:0x100 --target list_child_0076bcd0=0x0076bcd0:0x100 --target list_child_0076bd90=0x0076bd90:0x100 --target list_child_00746910=0x00746910:0x100 --target list_child_00770e50=0x00770e50:0x100 --target list_container_0072bd50=0x0072bd50:0x120 --target list_container_0072bf50=0x0072bf50:0x120 --target list_container_0082d790=0x0082d790:0x140 --target target_downbeat_0083e230=0x0083e230:0x120 --target target_0083d4c0=0x0083d4c0:0x120 --target target_00848c20=0x00848c20:0x120 --target target_00854140=0x00854140:0x120 --target target_0084ada0=0x0084ada0:0x120 --target target_0084f260=0x0084f260:0x120 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_event_list_child_objects_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_event_list_child_objects_20260611.json"`.
- Screenshot gate: accepted active in-song gameplay at normal speed, not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Changed-row counts:
  - `list_child_00768a10`: `34`.
  - `list_child_00770d90`: `17`.
  - `list_child_0076bcd0`: `43`.
  - `list_child_0076bd90`: `24`.
  - `list_child_00746910`: `14`.
  - `list_child_00770e50`: `17`.
  - `list_container_0072bd50`: `4`.
  - `list_container_0072bf50`: `0`.
  - `list_container_0082d790`: `10`.
  - `target_downbeat_0083e230`: `3`.
  - `target_0083d4c0`: `6`.
  - `target_00848c20`: `10`.
  - `target_00854140`: `16`.
  - `target_0084ada0`: `27`.
  - `target_0084f260`: `6`.
- Object/layout facts:
  - `0x00768a10` changed 34 rows and has descriptor pointer
    `+0x1c -> 0x003ee638`; linked rows include `+0x00 -> 0x00770d90`,
    `+0x68 -> 0x0076bcd0`, and source/list rows near `0x013bb500` and
    `0x00b902e0`.
  - `0x00770d90` changed 17 rows and has descriptor pointer
    `+0x1c -> 0x003ee638`; readable child data includes `normal_color`,
    `focus_color`, and other gameplay/script atoms.
  - `0x0076bcd0` changed 43 rows and has descriptor pointer
    `+0x5c -> 0x003eea08`; linked live rows include `0x00843e20`,
    `0x00843cf0`, `0x00847e80`, `0x00848dd0`, and adjacent venue/list rows.
  - `0x0076bd90` changed 24 rows and has descriptor pointer
    `+0x1c -> 0x003eea08`.
  - `0x00770e50` changed 17 rows and has descriptor pointer
    `+0x1c -> 0x003eea08`; readable fields include `in_solo`, `in_peak`,
    `flame_hands`, `parser`, `player0_parser`, and `HandMap_DropD2`.
  - `0x0072bd50` is the changing `excitement_level` container, with
    child/list links at `+0x68/+0x6c -> 0x00848d20`.
  - `0x0072bf50` was stable in this sample, with list links at
    `+0x68/+0x6c -> 0x0084f420`.
  - `0x0082d790` changed 10 rows and starts with table pointer
    `0x003edec0`; child/list links are at `+0x68 -> 0x00846550`,
    `+0x6c -> 0x00846558`, and `+0x70 -> 0x003e3010`.
  - `0x0083e230` is the named `downbeat` target row and changed during the
    sample.
- Interpretation: the list/value layer now has live object layouts, not just
  function-order traces. Descriptor pointers `0x003ee638` and `0x003eea08`
  are runtime-observed on the same child objects that the list dispatchers
  update.

Accepted world-event descriptor pointer-target sample:

- Tool: `tools/sample_pcsx2_pointer_targets.py`.
- Report: `pcsx2_world_event_descriptor_pointer_targets_20260611.json`.
- Screenshot: `pcsx2_world_event_descriptor_pointer_targets_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_pointer_targets.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 3 --seconds 10 --interval 0.25 --object-size 0xa0 --cell child_00768a10_desc1c=0x00768a2c --cell child_00770d90_desc1c=0x00770dac --cell child_0076bd90_desc1c=0x0076bdac --cell child_00770e50_desc1c=0x00770e6c --cell child_0076bcd0_desc5c=0x0076bd2c --cell container_0082d790_vtable0=0x0082d790 --cell container_0072bd50_vtable0=0x0072bd50 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_event_descriptor_pointer_targets_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_event_descriptor_pointer_targets_20260611.json"`.
- Screenshot gate: accepted active in-song gameplay at normal speed. The
  capture is not Retry/fail/startup/wrong-window. `EnableEE = true` was
  verified afterward and no PCSX2 process was left running.
- Pointer-target facts:
  - `0x00768a2c` and `0x00770dac` stayed stable at `0x003ee638`.
  - `0x003ee638` key slot values include `+0x0c -> 0x0022c138`,
    `+0x14/+0x44/+0x4c/+0x5c -> 0x00312560`, `+0x34 -> 0x00388490`,
    `+0x3c -> 0x003884a0`, `+0x74 -> 0x00388818`, and
    `+0x7c -> 0x0022c4a8`.
  - The first sample for `0x0076bdac`, `0x00770e6c`, and `0x0076bd2c`
    pointed at `0x003eea08`, but later samples drifted into float-like rows.
    Treat these as value-bearing cells after initialization, not stable
    vtable cells for the whole sample window.
  - The initial `0x003eea08` descriptor rows include
    `+0x0c -> 0x0022e040`, `+0x14 -> 0x0022e0f0`,
    `+0x1c -> 0x0022e188`, `+0x24 -> 0x0022e320`,
    `+0x2c -> 0x0022e378`, `+0x34 -> 0x0022e8a0`,
    `+0x3c -> 0x0022e928`, `+0x44 -> 0x0022e1b8`,
    `+0x4c -> 0x0022e1e0`, `+0x54 -> 0x0022e208`,
    `+0x5c -> 0x0022e270`, `+0x64 -> 0x0022e2e0`, and
    `+0x7c -> 0x0022efa0`.
  - `0x0082d790 + 0x00` initially pointed at table `0x003edec0`, but the
    sampled cell later changed to live object pointers `0x007997d0` and
    `0x0072bf50`; do not treat that cell as a stable table pointer beyond
    initialization.
- Interpretation: the descriptor-slot identities for the event/list child
  branch are now live-sampled. Some cells are reused for value state, so only
  stable cells or first-sample initialization rows should be used as proof.

Static snippets for world-event descriptor callbacks:

- Snippet report:
  `ps2_function_snippets_world_event_descriptor_callbacks_20260611.json`.
- Static facts:
  - `0x0022c138` writes descriptor `0x003ee638` to child `+0x1c` and can call
    `0x002cf160`.
  - `0x00312560` is a trampoline to `0x00102648`; tracing the trampoline entry
    directly is not useful with the current two-instruction patch stub.
  - `0x00388490` returns zero float state; `0x003884a0` is an immediate
    return stub.
  - `0x00388818` lazily initializes a table/global and returns it.
  - `0x0022c4a8` writes table `0x003ee6a8` to child `+0x04` and walks a
    child/list range under `+0x58`.
  - `0x0022e040` writes descriptor `0x003eea08` to child `+0x1c` and registers
    child state through the `0x003780b0` path.
  - `0x0022e0f0` tests child `+0x24` and also reaches `0x003780b0`.
  - `0x0022e1b8` sets child `+0x24` to `1` and emits event/message id
    `0x02be` through `0x00388de8`.
  - `0x0022e1e0` clears child `+0x24` and emits event/message id `0x02bf`.
  - `0x0022e270` calls `0x002333d8` on child `+0x34`, then calls
    `0x0022e2a0`.
  - `0x0022e2e0` scales incoming `f12` and emits id `0x02c2` through
    `0x00389630`.
  - `0x0022efa0` is a large destructor/cleanup path for the `0x003eea08`
    family.

Accepted world-event descriptor callback sequence:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_world_event_descriptor_callbacks_sequence_20260611.json`.
- Screenshot: `pcsx2_world_event_descriptor_callbacks_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 3 --seconds 30 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_event_descriptor_callbacks_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_event_descriptor_callbacks_sequence_20260611.json" --target list_update_bridge_002230c8=0x002230c8 --target list_update_child_002232d8=0x002232d8 --target list_update_child_0022c168=0x0022c168 --target list_walk_leaf_0022c1a0=0x0022c1a0 --target list_value_setter_00223400=0x00223400 --target list_value_child_0022c2c0=0x0022c2c0 --target desc638_real_target_00102648=0x00102648 --target desc_ea08_ctor_0022e040=0x0022e040 --target desc_ea08_cb14_0022e0f0=0x0022e0f0 --target desc_ea08_cb44_0022e1b8=0x0022e1b8 --target desc_ea08_cb4c_0022e1e0=0x0022e1e0 --target desc_ea08_cb5c_0022e270=0x0022e270 --target desc_ea08_cb64_0022e2e0=0x0022e2e0 --target desc_ea08_cb84_0022efa0=0x0022efa0 --target generic_event_leaf_0022c4a8=0x0022c4a8`.
- Screenshot gate: accepted active in-song gameplay. The interpreter run was
  slow (`FPS/VPS 29`, speed `48%`), but the capture is not Retry/fail/startup
  or wrong-window. `EnableEE = true` was verified afterward and no PCSX2
  process was left running.
- Retained counts:
  - `list_update_bridge_002230c8`: `7`.
  - `list_update_child_002232d8`: `8`.
  - `list_update_child_0022c168`: `8`.
  - `list_walk_leaf_0022c1a0`: `8`.
  - `list_value_setter_00223400`: `8`.
  - `list_value_child_0022c2c0`: `10`.
  - `desc638_real_target_00102648`: `0`.
  - `desc_ea08_ctor_0022e040`: `12`.
  - `desc_ea08_cb14_0022e0f0`: `1470`.
  - `desc_ea08_cb44_0022e1b8`: `8`.
  - `desc_ea08_cb4c_0022e1e0`: `0`.
  - `desc_ea08_cb5c_0022e270`: `18`.
  - `desc_ea08_cb64_0022e2e0`: `18`.
  - `desc_ea08_cb84_0022efa0`: `2`.
  - `generic_event_leaf_0022c4a8`: `0`.
- Local order examples:
  - Value/update branch:
    `0x002230c8(0x0072bd50, stack, -1, 0x00821170)` ->
    `0x00223400(0x0072bd50, stack, -1, 0x00821170)` ->
    `0x0022c2c0(0x00768a10, stack, -1, 0x00821170)` ->
    `0x0022e270(0x00768a10, 0, -1, 0x00821170)`.
  - Follow-up child/leaf branch:
    `0x002232d8(0x0072bd50, stack, -1, 0x41000000)` ->
    `0x0022e2e0(0x00768a10, 0, -1, 0x41000000)` ->
    `0x0022c168(0x00768a10, stack, -1, 0x41000000)` ->
    `0x0022c1a0(0x00768a10, stack, -1, 0x41000000)` ->
    `0x0022e0f0(0x00768a10, stack, -1, 0x41000000)` ->
    `0x0022e1b8(0x00768a10, stack, -1, 0x41000000)`.
  - The same ordering repeats for `0x00770d90` with targets such as
    `0x00848a40`.
  - `0x0022efa0` fired twice on `a0=0x00c9d060`, matching the crowd stream
    row already tied to world/venue event state.
- Interpretation: the descriptor callback layer under the world/event list
  branch is now trace-backed. `0x0022c2c0` reaches the `0x003eea08 + 0x5c`
  callback (`0x0022e270`), `0x002232d8` reaches `0x0022e2e0`,
  `0x0022c168` / `0x0022c1a0` reach `0x0022e0f0` and `0x0022e1b8`, and
  cleanup can reach `0x0022efa0` on the crowd stream object. This proves the
  generic descriptor callback identities for this slice, but it still does not
  name the higher-level authored event row as `band_jump`, `downbeat`, or
  `crowd_lighters_*` at the exact dispatch moment.

Rejected descriptor target-object sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_world_event_descriptor_target_objects_20260611.json`.
- Screenshot:
  `pcsx2_world_event_descriptor_target_objects_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 3 --seconds 12 --interval 0.25 --no-default-targets --target target_00821170=0x00821170:0x160 --target target_downbeat_0083e230=0x0083e230:0x160 --target target_00848a40=0x00848a40:0x160 --target target_0084ae30=0x0084ae30:0x160 --target target_00854140=0x00854140:0x160 --target child_00768a50=0x00768a50:0x120 --target child_00768c10=0x00768c10:0x120 --target child_0076bd10=0x0076bd10:0x120 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_event_descriptor_target_objects_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_event_descriptor_target_objects_20260611.json"`.
- Rejection reason: final screenshot is the Song Failed/Retry screen, not
  active gameplay. Do not use this run as accepted runtime evidence even though
  it contains readable object rows.

Accepted short descriptor target-object sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_world_event_descriptor_target_objects_short_20260611.json`.
- Screenshots:
  `pcsx2_world_event_descriptor_target_objects_short_20260611.before_sample.window.png`
  and `pcsx2_world_event_descriptor_target_objects_short_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 4 --interval 0.2 --no-default-targets --target target_00821170=0x00821170:0x160 --target target_downbeat_0083e230=0x0083e230:0x160 --target target_00848a40=0x00848a40:0x160 --target target_0084ae30=0x0084ae30:0x160 --target target_00854140=0x00854140:0x160 --target child_00768a50=0x00768a50:0x120 --target child_00768c10=0x00768c10:0x120 --target child_0076bd10=0x0076bd10:0x120 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_event_descriptor_target_objects_short_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_event_descriptor_target_objects_short_20260611.json"`.
- Screenshot gate: accepted active in-song gameplay at normal speed. The
  capture is not Retry/fail/startup/wrong-window. `EnableEE = true` was
  verified afterward and no PCSX2 process was left running.
- Changed-row counts:
  - `target_00821170`: `3`.
  - `target_downbeat_0083e230`: `3`.
  - `target_00848a40`: `15`.
  - `target_0084ae30`: `37`.
  - `target_00854140`: `30`.
  - `child_00768a50`: `35`.
  - `child_00768c10`: `11`.
  - `child_0076bd10`: `4`.
- Object/layout facts:
  - `0x00821170` changed only rows `+0x10/+0x14/+0x18`, switching from
    child pointers `0x00768c10` / `0x0076bd10` toward venue/list rows such as
    `0x00ac89d0` / `0x00848c60`.
  - `0x0083e230` retained readable `downbeat` at `+0x0c` and changed
    `+0x00/+0x04/+0x08`, confirming the named row is live even in a short
    active-song window.
  - `0x00848a40` changed 15 rows and links into `0x0084ae80`,
    `0x00850070`, `0x00ad2a00`, `0x00853e40`, and `0x00853fb0`.
  - `0x0084ae30` changed 37 rows and links into the venue/list cluster around
    `0x00846280`, `0x00846660`, `0x00848fa0`, `0x00853ab0`,
    `0x0073d370`, and `0x00ac8990`.
  - `0x00854140` changed 30 rows and carries links to `0x0084f420`,
    `0x0083e230`, `0x00850eb0`, `0x00848d30`, `0x00854210`, and
    `0x00846570`.
  - `0x00768a50`, `0x00768c10`, and `0x0076bd10` are live child/value rows.
    `0x00768c10 + 0x1c` and `0x0076bd10 + 0x1c` can switch between
    `0x003eea08` and `0x003ee638`, proving descriptor identity is phase/state
    dependent on these children.
- Interpretation: the descriptor callback target rows are connected back to
  live venue/list rows including the named `downbeat` row. This still does not
  prove the complete authored dispatch identity for `band_jump` or
  `crowd_lighters_*`, but it gives the next trace exact changing target rows
  to follow: `0x00821170`, `0x00848a40`, `0x0084ae30`, `0x00854140`, and the
  descriptor-flipping children `0x00768c10` / `0x0076bd10`.

Accepted CharDriver event-dispatch sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_chardriver_event_dispatch_sequence_20260611.json`.
- Screenshot: `pcsx2_chardriver_event_dispatch_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 2 --seconds 20 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_chardriver_event_dispatch_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_chardriver_event_dispatch_sequence_20260611.json" --target chardriver_event_dispatch_0010c988=0x0010c988 --target chardriver_branch_activate_0010b458=0x0010b458 --target chardriver_branch_play_0010b7f8=0x0010b7f8 --target chardriver_branch_mode_0010c5b8=0x0010c5b8 --target chardriver_branch_gated_0010c730=0x0010c730 --target chardriver_branch_check_0010c948=0x0010c948 --target chardriver_branch_place_0010cfa0=0x0010cfa0 --target chardriver_branch_aux_0010d148=0x0010d148 --target chardriver_event_helper_0010b9e8=0x0010b9e8 --target chardriver_msg_helper_0010bc54=0x0010bc54 --target chardriver_active_command_00173b98=0x00173b98 --target chardriver_clip_update_00171830=0x00171830 --target chardriver_selector_00171db0=0x00171db0`.
- Screenshot gate: accepted active in-song gameplay. The interpreter run was
  slow (`FPS/VPS 27`, speed `44%`), but the capture is valid gameplay, not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Retained counts:
  - `chardriver_event_dispatch_0010c988`: `1`.
  - `chardriver_branch_activate_0010b458`: `0`.
  - `chardriver_branch_play_0010b7f8`: `1`.
  - `chardriver_branch_mode_0010c5b8`: `0`.
  - `chardriver_branch_gated_0010c730`: `0`.
  - `chardriver_branch_check_0010c948`: `14`.
  - `chardriver_branch_place_0010cfa0`: `0`.
  - `chardriver_branch_aux_0010d148`: `0`.
  - `chardriver_event_helper_0010b9e8`: `14`.
  - `chardriver_msg_helper_0010bc54`: `0`.
  - `chardriver_active_command_00173b98`: `9`.
  - `chardriver_clip_update_00171830`: `8162`.
  - `chardriver_selector_00171db0`: `22`.
- Local order around the single top-level event-dispatch hit:
  - `0x00171830` clip/update calls on multiple drivers including
    `0x0135cb90`.
  - `0x0010b9e8(0x00b8be10, 0x00080000, 1, 0x00b7a2d0)`.
  - `0x0010c948(0x00b8be10, 0x00004000, 0x005d2498, 0x10)`.
  - `0x0010c988(stack, 0x00b8be10, 0x00850c40, 0)`.
  - `0x0010b7f8(0x00b8be10, 0x003f5778, 0x00850c40, 0)`.
  - Immediate follow-up `0x00171830` calls on `0x00c0d360`,
    `0x00daf090`, `0x00dbca2c`, `0x00dbc98c`, `0x0113f2e0`,
    `0x00fc9cd0`, `0x0101ebb0`, `0x010dbcb0`, `0x010f66b0`,
    `0x011a8c90`, `0x0123b850`, `0x012c2e50`, `0x012e1cf0`, and
    `0x0135cb90`.
  - `0x00171db0(0x0135cb90, 1, 0x003fffff, 0x0133c7a0)`.
- Symbol/object context:
  - Static string resolution maps `0x003f5778` to `normal`, so this accepted
    slice proves a concrete `0x0010c988` event dispatch selecting/routing the
    `normal` branch through `0x0010b7f8`.
  - Existing accepted object samples identify `0x00b8be10` as the shared
    glam1 character/source object with `char/glam1/og/glam1.milo`, and
    `0x00850c40` as a command row whose readable field includes `idle`.
  - Existing accepted object samples identify `0x00850c80` as the active
    command row used by `0x00173b98` active-command calls in the same window.
- Interpretation: this trace gives a PS2-backed event/clip-selection slice:
  the active character source `0x00b8be10` receives a top-level
  `0x0010c988` command, chooses the interned `normal` symbol branch, executes
  `0x0010b7f8`, then resumes the clip/update and selector chain. This does
  not yet cover every `0x0010c988` branch (`idle`, `play`, `wail_on/off`,
  solo/peak, placement), because only the `normal` branch fired in this
  accepted window.

Accepted CharDriver selector/object word sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_chardriver_selector_object_words_20260611.json`.
- Screenshots:
  `pcsx2_chardriver_selector_object_words_20260611.before_sample.window.png`
  and `pcsx2_chardriver_selector_object_words_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 4 --interval 0.25 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_chardriver_selector_object_words_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_chardriver_selector_object_words_20260611.json" --target driver_0135cb90=0x0135cb90:0x160 --target selector_arg_0133c7a0=0x0133c7a0:0x120 --target driver_00daf090=0x00daf090:0x160 --target driver_00dbca2c=0x00dbca2c:0x120 --target driver_00dbc98c=0x00dbc98c:0x120 --target driver_00c0d360=0x00c0d360:0x120 --target scheduler_00768b90=0x00768b90:0x120 --target sched_00768a50=0x00768a50:0x100 --target sched_0076bd10=0x0076bd10:0x100 --target sched_0076bcd0=0x0076bcd0:0x100 --target sched_0076bad0=0x0076bad0:0x100 --target sched_00768c50=0x00768c50:0x100 --target node_00843cf0=0x00843cf0:0x100 --target node_00847e80=0x00847e80:0x100 --target node_00846ee0=0x00846ee0:0x100 --target clip_013bb870=0x013bb870:0x120 --target clip_013bb500=0x013bb500:0x120 --target source_00b902e0=0x00b902e0:0x120`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay at
  normal speed (`FPS 60`, `VPS 60`, speed `100%`). The capture is not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Changed-row counts:
  - `driver_0135cb90`: `2`.
  - `selector_arg_0133c7a0`: `0`.
  - `driver_00daf090`: `1`.
  - `driver_00dbca2c`: `1`.
  - `driver_00dbc98c`: `2`.
  - `driver_00c0d360`: `1`.
  - `scheduler_00768b90`: `26`.
  - `sched_00768a50`: `21`.
  - `sched_0076bd10`: `4`.
  - `sched_0076bcd0`: `12`.
  - `sched_0076bad0`: `6`.
  - `sched_00768c50`: `8`.
  - `node_00843cf0`: `0`.
  - `node_00847e80`: `0`.
  - `node_00846ee0`: `0`.
  - `clip_013bb870`: `0`.
  - `clip_013bb500`: `10`.
  - `source_00b902e0`: `0`.
- Driver/object facts:
  - `0x0135cb90` is the `main.drv` driver object in this sample. Its rows
    include tables `0x003e81b0`, `0x003e7380`, `0x003e7440`, and
    `0x003e3230`, source pointer `0x00b902e0`, clip row `0x013bb870`, and
    readable `main.drv` at `0x0135cbf8`.
  - `0x0135cb90 + 0x38` changed from `0x00768a50` to `0x0076bcd0`, proving
    the active scheduler/list pointer is moving during the same save window
    as the accepted event-dispatch trace.
  - The common driver phase/timing row at `+0x48` advanced on
    `0x00daf090`, `0x00dbca2c`, `0x00dbc98c`, and `0x00c0d360`.
- Scheduler/list facts:
  - `0x00768b90` is a live scheduler/list block with moving rows
    `+0x0c..+0x20`, child/source rows `+0x24 = 0x00ebd2f0`,
    `+0x28 = 0x00770f10`, `+0x2c = 0x00b8be10`, and
    hash/sentinel rows `+0x30 = 0xf149f2ca`, `+0x34 = 0xffffffff`.
    It also contains a second block beginning at `+0x40`.
  - `0x00768a50` carries repeated scheduler blocks. Its first block points
    through `+0x24 = 0x013bb500`, `+0x28 = 0x0076bcd0`,
    `+0x2c = 0x00b902e0`, `+0x30 = 0xf149f2ca`,
    `+0x34 = 0xffffffff`; its second block points through
    `+0x64 = 0x011b24c0`, `+0x68 = 0x0076ba10`,
    `+0x6c = 0x00b8f7e0`.
  - `0x0076bcd0` begins with `0x00000232`, points at
    `+0x24 = 0x013bb500`, `+0x2c = 0x00b902e0`,
    `+0x38 = 0x00843e20`, and changed `+0x28` from `0` to
    `0x00768a50`.
  - `0x0076bd10` exposes descriptor/list children. It has descriptor pointer
    `+0x1c = 0x003eea08` at sample start, child rows including
    `+0x38 = 0x00843cf0`, `+0x40 = 0x00847e80`,
    `+0x44 = 0x00848dd0`, and continuing through `0x00846a30`. During the
    active sample, its first row changed from `1.0` to `0x00768c10` and
    `+0x1c` flipped `0x003eea08 -> 0x003ee638`.
  - `0x0076bad0` links to `0x00768c50`, `0x00b8df40`, `0x00ebf420`, and
    `0x00b8be10`; `0x00768c50` carries repeated blocks with source rows
    `0x00b8df40` and `0x00b8b800`.
  - `0x013bb500` is a changing clip/source-side record linked from
    `0x00768a50` and `0x0076bcd0`; rows `+0xec..+0x110` rotate through EE
    RAM rows such as `0x013c69f0`, `0x013c6de0`, `0x013c7260`, and
    `0x013c5ac0`.
- Interpretation: this trace extends the accepted `0x00171830` /
  `0x00171db0` call slice into concrete live scheduler, list, clip, and
  source rows for `main.drv`. It proves pointer movement and repeated
  scheduler-block layout around the selector path. It does not prove the
  indirect callback target at `0x00171ad8`, exact field names/math for the
  float rows, or the full branch coverage for `0x0010c988`.

Static CharDriver indirect callback site clarification:

- Static snippet report:
  `ps2_function_snippets_chardriver_selector_deep_20260611.json`.
- The direct site at `0x00171ad8` is `jalr ra,v1`; it is not a standalone
  function and must not be patched or traced as a direct function entry.
- Relevant local body:
  - `0x00171a2c`: loads source object from driver `+0x1c`.
  - `0x00171a38`: loads source `+0x00` link object.
  - `0x00171a48`: loads link `+0x00` callback table.
  - `0x00171a50`: reads signed this-adjust at table `+0x30`.
  - `0x00171ac8`: loads callback target from table `+0x34`.
  - `0x00171acc`: passes adjusted source as `a1`.
  - `0x00171ad4`: passes stack temp as `a0`.
  - `0x00171ad8`: calls the resolved callback target through `jalr`.
- Interpretation: follow the source/link/table objects and trace the resolved
  target functions. Do not treat `0x00171ad8` as a normal function target.

Accepted CharDriver callback source-chain object sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_chardriver_callback_source_chain_20260611.json`.
- Screenshots:
  `pcsx2_chardriver_callback_source_chain_20260611.before_sample.window.png`
  and `pcsx2_chardriver_callback_source_chain_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 4 --interval 0.25 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_chardriver_callback_source_chain_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_chardriver_callback_source_chain_20260611.json" --target source_00b902e0=0x00b902e0:0x140 --target link_00b90550=0x00b90550:0x100 --target aux_013bab60=0x013bab60:0x100 --target source_00b8be10=0x00b8be10:0x300 --target link_00b8c170=0x00b8c170:0x100 --target source_00b8b800=0x00b8b800:0x180 --target link_00b8ba70=0x00b8ba70:0x100 --target source_00b8df40=0x00b8df40:0x180 --target link_00b8e1b0=0x00b8e1b0:0x100 --target driver_0135cb90=0x0135cb90:0x80 --target driver_00daf090=0x00daf090:0x80 --target driver_00c0d360=0x00c0d360:0x80`.
- Screenshot gate: accepted active in-song gameplay at normal speed
  (`FPS 60`, `VPS 60`, speed `100%`). The capture is not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Changed-row counts:
  - `source_00b902e0`: `0`.
  - `link_00b90550`: `0`.
  - `aux_013bab60`: `0`.
  - `source_00b8be10`: `17`.
  - `link_00b8c170`: `0`.
  - `source_00b8b800`: `0`.
  - `link_00b8ba70`: `0`.
  - `source_00b8df40`: `14`.
  - `link_00b8e1b0`: `0`.
  - `driver_0135cb90`: `4`.
  - `driver_00daf090`: `2`.
  - `driver_00c0d360`: `1`.
- Source/link facts:
  - `0x00b8be10` is the glam1 guitarist source; `+0x00 = 0x00b8c170`,
    `+0x5c` names `char/glam1/og/glam1.milo`, and pose/float rows around
    `0x00b8bf10..` changed during the active sample.
  - `0x00b8c170` links to table `0x003e3050`; `+0x14` names `guitarist0`.
    Table `0x003e3050 + 0x30 = 0x0000fca0`,
    `+0x34 = 0x0010c988`, and `+0x3c = 0x0010d1b8`.
  - `0x00b902e0` is the metal drummer source; `+0x00 = 0x00b90550`,
    `+0x04 = 0x013bab60`, and `+0x4c` names
    `char/metal_drummer/og/metal_drummer.milo`.
  - `0x00b90550` links to table `0x003e6aa8`; `+0x14` names `drummer`.
    Table `0x003e6aa8 + 0x30 = 0x0000fd90`,
    `+0x34 = 0x00165400`, and `+0x3c = 0x001658d0`.
  - `0x00b8b800` is the metal singer source; `+0x00 = 0x00b8ba70` and
    `+0x4c` names `char/metal_singer/og/metal_singer.milo`.
    `0x00b8ba70` links to table `0x003e6aa8` and names `singer`.
  - `0x00b8df40` is the metal bass source; `+0x00 = 0x00b8e1b0`,
    `+0x4c` names `char/metal_bass/og/metal_bass.milo`, and pose/float rows
    around `0x00b8e040..` changed during the active sample.
    `0x00b8e1b0` links to table `0x003e6aa8` and names `bassist`.
- Interpretation: the `0x00171830` indirect callback source chain is now
  object-backed for guitarist, singer, bassist, and drummer-family sources.
  The resolved callback targets come from source link tables, not from
  `0x00171ad8` itself.

Accepted CharDriver callback target sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Static snippet report:
  `ps2_function_snippets_callback_targets_20260611.json`.
- Report: `pcsx2_chardriver_callback_targets_sequence_20260611.json`.
- Screenshot:
  `pcsx2_chardriver_callback_targets_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 12 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_chardriver_callback_targets_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_chardriver_callback_targets_sequence_20260611.json" --target guitarist_callback_0010c988=0x0010c988 --target guitarist_callback_aux_0010d1b8=0x0010d1b8 --target performer_callback_00165400=0x00165400 --target performer_callback_aux_001658d0=0x001658d0 --target callback_classify_002b7e50=0x002b7e50 --target chardriver_clip_update_00171830=0x00171830 --target chardriver_selector_00171db0=0x00171db0`.
- Screenshot gate: accepted active in-song gameplay. The interpreter run was
  slow (`FPS/VPS 33`, speed `54%`), but the capture is not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Retained counts:
  - `guitarist_callback_0010c988`: `1`.
  - `guitarist_callback_aux_0010d1b8`: `0`.
  - `performer_callback_00165400`: `16`.
  - `performer_callback_aux_001658d0`: `10`.
  - `callback_classify_002b7e50`: `9112`.
  - `chardriver_clip_update_00171830`: `4858`.
  - `chardriver_selector_00171db0`: `9`.
- Local order examples:
  - `performer_callback_00165400` hit drummer, bass, and singer-style sources:
    `a1=0x00b902e0`, `0x00b8df40`, and `0x00b8b800` with
    `a2=0x008504e0`.
  - Aux callback examples include
    `0x001658d0(0x00b902e0, 0x005f9cc0, 0x0051eaf0, 1)` and
    `0x001658d0(0x00b902e0, 0x005fa090, 0x005f7b74, 2)`.
  - The guitarist callback fired once as
    `0x0010c988(0x01ffe6e0, 0x00b8be10, 0x00850c40, 0)`.
  - Selector examples in the same ring include
    `0x00171db0(0x0135cb90, 0, 0x007fffff, 0x0133c8a0)`,
    `0x00171db0(0x0135cb90, 1, 0x003fffff, 0x0133d220)`, and
    `0x00171db0(0x0135cb90, 1, 0x003fffff, 0x0133c7a0)`.
- Interpretation: this finally proves the indirect callback families reached
  by `0x00171830`: guitarist link table `0x003e3050` dispatches to
  `0x0010c988`, while the singer/bassist/drummer-style table `0x003e6aa8`
  dispatches to `0x00165400` and its adjacent aux slot `0x001658d0`.
  The accepted window does not prove all `0x0010c988` branches, all
  `0x00165400` command branches, exact downstream command semantics, exact
  scheduler float field names/math, or the final pose output.

Static performer callback downstream snippet dump:

- Static snippet report:
  `ps2_function_snippets_performer_callback_downstream_20260611.json`.
- New downstream targets from `0x00165400`:
  - `0x00162b30`
  - `0x00162b10`
  - `0x00180e00`
  - `0x001656a8`
  - `0x00162780`
  - `0x00162358`
  - `0x001b4eb0`
- New downstream targets from `0x001658d0`:
  - `0x00162640`
  - `0x003303d0`
  - `0x0028db50`
- Static interpretation: `0x00165400` fans into command/classification and
  performer event paths. The next runtime question is which branches are live
  in the current active-song window, not whether the source table itself is
  valid.

Accepted performer callback downstream sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_performer_callback_downstream_sequence_20260611.json`.
- Screenshot:
  `pcsx2_performer_callback_downstream_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 12 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_performer_callback_downstream_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_performer_callback_downstream_sequence_20260611.json" --target performer_callback_00165400=0x00165400 --target performer_callback_aux_001658d0=0x001658d0 --target performer_world_apply_00162b30=0x00162b30 --target performer_get_driver_00162b10=0x00162b10 --target performer_apply_bridge_00180e00=0x00180e00 --target performer_branch_001656a8=0x001656a8 --target performer_store_00162780=0x00162780 --target performer_transform_00162358=0x00162358 --target performer_event_apply_001b4eb0=0x001b4eb0 --target performer_aux_transform_00162640=0x00162640 --target object_dispatch_003303d0=0x003303d0 --target aux_bridge_0028db50=0x0028db50`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay. The
  interpreter run was slow (`FPS/VPS 33`, speed `55%`), but the capture is not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Retained counts:
  - `performer_callback_00165400`: `21`.
  - `performer_callback_aux_001658d0`: `11`.
  - `performer_world_apply_00162b30`: `0`.
  - `performer_get_driver_00162b10`: `0`.
  - `performer_apply_bridge_00180e00`: `0`.
  - `performer_branch_001656a8`: `2`.
  - `performer_store_00162780`: `0`.
  - `performer_transform_00162358`: `0`.
  - `performer_event_apply_001b4eb0`: `57`.
  - `performer_aux_transform_00162640`: `0`.
  - `object_dispatch_003303d0`: `0`.
  - `aux_bridge_0028db50`: `0`.
- Local order examples:
  - `0x001b4eb0` fires on several performer/source rows before the first
    performer callback burst, including `0x00b78100`, `0x00b7d200`,
    `0x00b8a530`, and `0x00b8c3f0`.
  - Performer callback burst:
    `0x00165400(0x01ffe350, 0x00b902e0, 0x008504e0, 0)` ->
    `0x001b4eb0(0x01ffe270, 0x00b902e0, 0x008504e0, 0)`,
    then the same shape for `0x00b8df40` and `0x00b8b800`.
  - Later performer-source bursts use
    `0x00165400(0x01ffe6a0, source, event_row, 1)` followed by
    `0x001b4eb0(0x01ffe590, source, event_row, 0)` and then
    `0x001658d0(source, 0x005f8b70, 0x0051eaf0, 1)`.
  - `0x001656a8` fired twice in the same accepted window; the other sampled
    downstream targets did not fire in this window.
- Interpretation: this trace proves `0x001b4eb0` is the hot runtime
  downstream branch under the performer/source callback family in this save
  window, with `0x001656a8` as a less frequent live branch. It does not prove
  the zero-hit branches are dead globally; they remain phase-gated or
  unexercised in this slice.

Static performer event/apply child snippet dump:

- Static snippet report:
  `ps2_function_snippets_performer_event_apply_children_20260611.json`.
- `0x001b4eb0` direct fanout includes:
  - `0x001b5150`
  - `0x002bcb00`
  - `0x001abca8`
  - `0x001b5c58`
  - `0x001de370`
  - `0x001d2960`
  - `0x002c0670`
- `0x001656a8` direct fanout includes `0x00171190`.

Accepted performer event/apply child sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_performer_event_apply_children_sequence_20260611.json`.
- Screenshot:
  `pcsx2_performer_event_apply_children_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 12 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_performer_event_apply_children_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_performer_event_apply_children_sequence_20260611.json" --target performer_event_apply_001b4eb0=0x001b4eb0 --target performer_event_prepare_001b5150=0x001b5150 --target performer_event_obj_002bcb00=0x002bcb00 --target performer_event_001abca8=0x001abca8 --target performer_event_001b5c58=0x001b5c58 --target performer_event_trans_001de370=0x001de370 --target performer_event_pose_001d2960=0x001d2960 --target performer_event_obj_002c0670=0x002c0670 --target performer_branch_001656a8=0x001656a8 --target performer_branch_sched_00171190=0x00171190`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay. The
  interpreter run was slow (`FPS/VPS 32`, speed `53%`), but the capture is not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Retained counts:
  - `performer_event_apply_001b4eb0`: `56`.
  - `performer_event_prepare_001b5150`: `0`.
  - `performer_event_obj_002bcb00`: `62`.
  - `performer_event_001abca8`: `39`.
  - `performer_event_001b5c58`: `24`.
  - `performer_event_trans_001de370`: `24`.
  - `performer_event_pose_001d2960`: `24`.
  - `performer_event_obj_002c0670`: `420`.
  - `performer_branch_001656a8`: `2`.
  - `performer_branch_sched_00171190`: `2`.
- Local order examples:
  - The ring opens with repeated `0x002c0670(0x01ffe410, 0x00aaa360,
    0x0059f040, 0)`, showing a very hot object/event branch.
  - `0x001abca8` interleaves with the hot object branch on source
    `0x00abec0c` and event rows such as `0x0059a330`, `0x0059a3c0`,
    `0x0059a450`, `0x0059a4e0`, and `0x0059a7f0`.
  - One `0x001abca8` call uses `a1=0x00b7a2d0` and `a2=0x008538a0`,
    tying this event/apply branch back to the live CamShot eval object already
    seen in camera traces.
  - `0x001656a8` and `0x00171190` both fired twice, proving the less frequent
    performer branch reaches the scheduler-like `0x00171190` path in this
    same active window.
- Interpretation: `0x001b4eb0` is now traced into concrete child branches.
  The hot branch is `0x002c0670`; other live branches include `0x001abca8`,
  `0x001b5c58`, `0x001de370`, and `0x001d2960`. This still does not name
  the event rows or prove exact placement/pose semantics; the next trace
  should sample the event/source objects such as `0x00aaa360`, `0x00abec0c`,
  `0x0059f040`, `0x0059a330..`, `0x00b7a2d0`, and `0x008538a0`.

Accepted performer event/object row sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_performer_event_object_rows_20260611.json`.
- Screenshots:
  `pcsx2_performer_event_object_rows_20260611.before_sample.window.png` and
  `pcsx2_performer_event_object_rows_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 4 --interval 0.25 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_performer_event_object_rows_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_performer_event_object_rows_20260611.json" --target hot_source_00aaa360=0x00aaa360:0x180 --target hot_event_0059f040=0x0059f040:0x140 --target event_source_00abec0c=0x00abec0c:0x180 --target event_0059a330=0x0059a330:0x120 --target event_0059a3c0=0x0059a3c0:0x120 --target event_0059a450=0x0059a450:0x120 --target event_0059a4e0=0x0059a4e0:0x120 --target event_0059a7f0=0x0059a7f0:0x120 --target cam_eval_00b7a2d0=0x00b7a2d0:0x180 --target cam_event_008538a0=0x008538a0:0x120 --target drummer_source_00b902e0=0x00b902e0:0x120 --target bass_source_00b8df40=0x00b8df40:0x120 --target singer_source_00b8b800=0x00b8b800:0x120`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay at
  normal speed (`FPS 60`, `VPS 60`, speed `100%`). The capture is not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Changed-row counts:
  - `hot_source_00aaa360`: `0`.
  - `hot_event_0059f040`: `0`.
  - `event_source_00abec0c`: `2`.
  - `event_0059a330`: `0`.
  - `event_0059a3c0`: `1`.
  - `event_0059a450`: `1`.
  - `event_0059a4e0`: `0`.
  - `event_0059a7f0`: `2`.
  - `cam_eval_00b7a2d0`: `17`.
  - `cam_event_008538a0`: `13`.
  - `drummer_source_00b902e0`: `0`.
  - `bass_source_00b8df40`: `4`.
  - `singer_source_00b8b800`: `0`.
- Object/layout facts:
  - `0x0059f040` is a UI/game data row, not a performer pose row. Readable
    atoms include `ui/game.dtb`, `ui`, `in_transition`, `game`,
    `is_missing_controller`, and `multiplayer`.
  - `0x0059a3c0`, `0x0059a450`, and `0x0059a7f0` are
    `ui/track_panel.dtb` rows. They include readable atoms such as `delay`,
    `units`, `pop_smasher`, `set_smasher_glowing`, and `script`.
  - `0x00abec0c` names/links through a `track_panel` source row; its
    `+0x74` points to readable `track_panel`, and rows `+0x74..+0x80` changed
    through live UI/track-panel pointers.
  - `0x00b7a2d0` is the live CamShot eval object already traced in the camera
    pipeline. It names `INTRO_FAST`, links to `Intro_fast`, and changed 17
    time/transform-like rows during this sample.
  - `0x008538a0` is a live venue/camera event row. It names `swing` at
    `+0x38` and includes readable `crowd_lighters_off` at `0x0085392c`;
    13 pointer rows changed during the active sample.
  - `0x00b8df40` is the metal bass source and still names
    `char/metal_bass/og/metal_bass.milo`; four pose/float rows around
    `0x00b8e040..0x00b8e054` changed during the sample.
- Interpretation: not every `0x001b4eb0` child row is performer animation.
  The hot `0x002c0670` branch in this slice is tied to UI/game or track-panel
  records, while the CamShot-linked `0x001abca8` branch reaches live
  venue/camera event rows including `swing` and `crowd_lighters_off`. Keep
  these branches separated before using them for native performer placement or
  venue/camera behavior.

Accepted performer non-UI object row sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_performer_nonui_object_rows_20260611.json`.
- Screenshots:
  `pcsx2_performer_nonui_object_rows_20260611.before_sample.window.png` and
  `pcsx2_performer_nonui_object_rows_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 4 --interval 0.25 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_performer_nonui_object_rows_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_performer_nonui_object_rows_20260611.json" --target perf0_base_00b78100=0x00b78100:0x220 --target perf1_base_00b7d200=0x00b7d200:0x220 --target perf2_base_00b8a530=0x00b8a530:0x220 --target perf3_base_00b8c3f0=0x00b8c3f0:0x220 --target evt_0059c940=0x0059c940:0x120 --target evt_0059c9f0=0x0059c9f0:0x120 --target evt_008504e0=0x008504e0:0x160 --target evt_00850c30=0x00850c30:0x160 --target evt_00850c40=0x00850c40:0x160 --target drv_0113f2e0=0x0113f2e0:0x160 --target sched_event_005f7bb0=0x005f7bb0:0x120 --target sched_event_005f9ee0=0x005f9ee0:0x120 --target main_drv_0135cb90=0x0135cb90:0x160`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay at
  normal speed (`FPS 60`, `VPS 60`, speed `100%`). The capture is not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Changed-row counts:
  - `perf0_base_00b78100`: `0`.
  - `perf1_base_00b7d200`: `0`.
  - `perf2_base_00b8a530`: `0`.
  - `perf3_base_00b8c3f0`: `0`.
  - `evt_0059c940`: `1`.
  - `evt_0059c9f0`: `0`.
  - `evt_008504e0`: `0`.
  - `evt_00850c30`: `19`.
  - `evt_00850c40`: `19`.
  - `drv_0113f2e0`: `1`.
  - `sched_event_005f7bb0`: `0`.
  - `sched_event_005f9ee0`: `0`.
  - `main_drv_0135cb90`: `2`.
- Object/layout facts:
  - The four performer/event source blocks are stable over this four-second
    sample, but they expose related sub-blocks used by the child trace:
    `0x00b78190/1d0/1e0/2a0`, `0x00b7d290/2d0/2e0/3a0`,
    `0x00b8a5c0/600/610/6d0`, and `0x00b8c480/4c0/4d0/590`.
  - `0x00850c30` and `0x00850c40` are live event rows. The readable command
    cell at `0x00850c58` changed from `play` to `verse`; downstream pointer
    rows rotated through venue/list rows such as `0x00853ab0`,
    `0x008489e0`, `0x00851250`, and `0x00853990`.
  - `0x0059c940` is a UI/game script row with `slide_meter_in`, `delay`,
    `units`, `script`, `intro_end`, and `intro_complete`; only its script
    state word at `0x0059c9b8` changed.
  - `0x0113f2e0` is a bass/performer driver-style block: it links to
    `0x00b8df40`, names `main.drv`, carries `starved`, and its phase row
    `+0x48` changed during the sample.
  - `0x0135cb90` is still `main.drv`; its active scheduler pointer at
    `+0x38` changed `0x00768a50 -> 0x0076bcd0`, and its phase row `+0x48`
    advanced.
  - `0x005f7bb0` and `0x005f9ee0` are static `char/char_objects.dtb` script
    rows in this short sample. They expose authored atoms including
    `play_mode`, `idle`, `play_idle`, `BAND_COMMON`, `band_jump`,
    `play_clip`, `next_event_beat`, `parser`, `strneq`, `showing`,
    `script_task`, and `dir`.
- Interpretation: the current callback/event traces now split into three
  useful categories: UI/track-panel rows, live world/camera venue rows, and
  CharDriver/performer script rows. The performer-relevant branch should next
  follow `0x001656a8 -> 0x00171190` and the `0x005f7bb0` /
  `0x005f9ee0` `char/char_objects.dtb` rows into concrete scheduler/blend
  effects.

Static performer scheduler branch snippet dump:

- Static snippet report:
  `ps2_function_snippets_performer_scheduler_branch_20260611.json`.
- `0x00171190` direct fanout includes:
  - `0x002b8298`
  - `0x002d1c00`
  - `0x001710e0`
  - `0x00198660`
- `0x001710e0` direct fanout includes `0x0016c1b0` plus object/list helpers.
- `0x00198660` writes scheduler/blend entry fields including:
  - `a0` at entry `+0x00`.
  - float rows at `+0x04`, `+0x08`, `+0x0c`, `+0x10`, and `+0x18`.
  - pointer/source rows at `+0x24`, `+0x28`, `+0x2c`, `+0x30`, and `+0x34`.
  - helper fanout to `0x00198ac8`, `0x00199000`, `0x00195f18`,
    `0x00198a48`, and `0x00196888`.

Accepted performer scheduler branch sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_performer_scheduler_branch_sequence_20260611.json`.
- Screenshot:
  `pcsx2_performer_scheduler_branch_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 12 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_performer_scheduler_branch_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_performer_scheduler_branch_sequence_20260611.json" --target performer_branch_001656a8=0x001656a8 --target performer_sched_00171190=0x00171190 --target sched_child_001710e0=0x001710e0 --target blend_entry_init_00198660=0x00198660 --target scheduler_push_00171248=0x00171248 --target scheduler_create_00171330=0x00171330 --target clip_lookup_0016c1b0=0x0016c1b0`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay. The
  interpreter run was slow (`FPS/VPS 32`, speed `53%`), but the capture is not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Retained counts:
  - `performer_branch_001656a8`: `0`.
  - `performer_sched_00171190`: `0`.
  - `sched_child_001710e0`: `0`.
  - `blend_entry_init_00198660`: `10`.
  - `scheduler_push_00171248`: `8`.
  - `scheduler_create_00171330`: `2`.
  - `clip_lookup_0016c1b0`: `0`.
- Local order examples:
  - Each `0x00171248` scheduler push in the retained ring is followed by
    `0x00198660` blend entry initialization with matching `a2` source pointer
    and `a3` mode/flags value.
  - Examples:
    `0x00171248(0x0135cb90, 0x013bb500, 0x234, 0x003e0000)` ->
    `0x00198660(0x0076bcd0, 0x00b902e0, 0x013bb500, 0x234)`.
  - The `0x00b902e0` source is the drummer-family source already seen in the
    callback-source chain.
  - `0x00171330(0x00daf090, 0x00dc77c0, 0x204, 0x10)` ->
    `0x00198660(0x00768b10, 0x00b8be10, 0x00ebd6a0, 0x204)`.
  - `0x00171330(0x00daf090, 0x00dc5fb0, 1, 0x10)` ->
    `0x00198660(0x0076bd10, 0x00b8be10, 0x00e0d150, 1)`.
  - The `0x00b8be10` source is the glam1/guitarist source already tied to the
    guitarist callback table `0x003e3050`.
- Interpretation: this trace proves active in-song scheduler/create calls are
  still feeding the same `0x00198660` blend-entry initializer with live
  performer source objects. It did not catch the rarer
  `0x001656a8 -> 0x00171190 -> 0x001710e0` branch in this 12-second window,
  so that exact branch remains phase-gated and must be re-captured rather than
  inferred from the static fanout alone.

Accepted scheduler blend-entry object sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_scheduler_blend_entry_objects_20260611.json`.
- Screenshots:
  `pcsx2_scheduler_blend_entry_objects_20260611.before_sample.window.png` and
  `pcsx2_scheduler_blend_entry_objects_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 4 --interval 0.25 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_scheduler_blend_entry_objects_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_scheduler_blend_entry_objects_20260611.json" --target blend_drum_0076bcd0=0x0076bcd0:0x100 --target blend_glam_a_00768b10=0x00768b10:0x100 --target blend_glam_b_0076bd10=0x0076bd10:0x100 --target src_drum_00b902e0=0x00b902e0:0x160 --target src_glam_00b8be10=0x00b8be10:0x160 --target drv_main_0135cb90=0x0135cb90:0x160 --target drv_guitar_00daf090=0x00daf090:0x160 --target sched_drum_013bb500=0x013bb500:0x120 --target sched_glam_a_00ebd6a0=0x00ebd6a0:0x120 --target sched_glam_b_00e0d150=0x00e0d150:0x120`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay at
  normal speed (`FPS 60`, `VPS 60`, speed `100%`). The capture is not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Changed-row counts:
  - `blend_drum_0076bcd0`: `12`.
  - `blend_glam_a_00768b10`: `30`.
  - `blend_glam_b_0076bd10`: `4`.
  - `src_drum_00b902e0`: `0`.
  - `src_glam_00b8be10`: `11`.
  - `drv_main_0135cb90`: `2`.
  - `drv_guitar_00daf090`: `2`.
  - `sched_drum_013bb500`: `10`.
  - `sched_glam_a_00ebd6a0`: `0`.
  - `sched_glam_b_00e0d150`: `0`.
- Object/layout facts:
  - `0x0076bcd0` is a live drummer-family blend entry. Initial rows include
    mode/status `+0x00 = 0x232`, weights `+0x04/+0x08 = 1.0`, scheduler
    source pointer `+0x24 = 0x013bb500`, performer source pointer
    `+0x2c = 0x00b902e0`, event/list pointer `+0x38 = 0x00843e20`, and
    descriptor/class pointer `+0x5c = 0x003eea08`. Changing rows include
    `+0x00`, float bands `+0x0c..+0x20`, next/list pointer `+0x28`, and
    descriptor/list rows around `+0x40/+0x5c/+0x64`.
  - `0x00768b10` is a live glam/guitarist blend entry. Initial rows include
    scheduler source pointer `+0x24 = 0x0101b1f0` and performer source pointer
    `+0x2c = 0x00b8da50`; changing rows include `+0x00`, float bands
    `+0x0c..+0x20`, next/list pointer `+0x28`, and second-entry float bands
    around `+0x4c..+0xa0`.
  - `0x0076bd10` is also live but this sample shows it partly overlapping a
    list/descriptor region: `+0x00` changed from `1.0` to pointer
    `0x00768c10`, descriptor `+0x1c` changed `0x003eea08 -> 0x003ee638`,
    and enable-like rows at `+0x24` and `+0xa4` cleared.
  - `0x0135cb90` main/drummer driver still points to source `0x00b902e0` at
    `+0x1c`; current blend entry `+0x38` changed
    `0x00768a50 -> 0x0076bcd0`, and phase row `+0x48` advanced.
  - `0x00daf090` glam/guitar driver points to source `0x00b8be10` at
    `+0x1c`; current blend entry `+0x38` changed
    `0x00768b90 -> 0x00770f10`, and phase row `+0x48` advanced.
  - `0x00b8be10` glam source changed transform/pose rows around
    `+0x100..+0x154`, including quaternion-like values and translation-like
    values around `85.8..85.6`. The drummer source `0x00b902e0` stayed stable
    in this short sample.
  - `0x013bb500` drummer scheduler data changed pointer bands
    `+0xec..+0x110`, rotating through child rows such as
    `0x013c69f0 -> 0x013c5ac0`, `0x013c6a00 -> 0x013c5ad0`,
    `0x013c6a70 -> 0x013c5b40`, and `0x013c6a7c -> 0x013c5b4c`.
- Interpretation: this sample ties the direct scheduler trace to concrete
  mutable blend-entry and source layouts. The driver `+0x38` current-entry
  pointer, blend-entry `+0x24/+0x2c` scheduler/source pointers, and source
  transform rows are now trace-backed. It still does not provide semantic names
  for every blend-entry float or prove the missing `0x001710e0` clip lookup
  branch.

Accepted performer scheduler branch long sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_performer_scheduler_branch_long_sequence_20260611.json`.
- Screenshot:
  `pcsx2_performer_scheduler_branch_long_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 30 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_performer_scheduler_branch_long_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_performer_scheduler_branch_long_sequence_20260611.json" --target performer_branch_001656a8=0x001656a8 --target performer_sched_00171190=0x00171190 --target sched_child_001710e0=0x001710e0 --target clip_lookup_0016c1b0=0x0016c1b0 --target blend_entry_init_00198660=0x00198660`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay. The
  interpreter run was slow (`FPS/VPS 26`, speed `44%`), but the capture is not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Retained counts:
  - `performer_branch_001656a8`: `2`.
  - `performer_sched_00171190`: `2`.
  - `sched_child_001710e0`: `4`.
  - `clip_lookup_0016c1b0`: `4`.
  - `blend_entry_init_00198660`: `55`.
- Branch order examples:
  - Bass path:
    `0x001656a8(0x01ffdec0, 0x00b8df40, 0x005f7e50, 1)` ->
    `0x00171190(0x0113f2e0, 0x005f7bb0, 0x30, 1)` ->
    `0x001710e0(0x0113f2e0, 0x005f7bb0, 1, 0x00770fd0)` ->
    `0x0016c1b0(0x01158730, 0x005f7bb0, 1, 0x00770fd0)` ->
    `0x00198660(0x00770fd0, 0x00b8df40, 0x0115fb40, 0x30)`.
  - Drummer path:
    `0x001656a8(0x01ffdec0, 0x00b902e0, 0x005fa0b0, 1)` ->
    `0x00171190(0x0135cb90, 0x005f9ee0, 0x30, 1)` ->
    `0x001710e0(0x0135cb90, 0x005f9ee0, 1, 0x0076bad0)` ->
    `0x0016c1b0(0x01359cc0, 0x005f9ee0, 1, 0x0076bad0)` ->
    `0x00198660(0x0076bad0, 0x00b902e0, 0x013bc010, 0x30)`.
  - Additional CharDriver left/right child paths also hit
    `0x001710e0 -> 0x0016c1b0` on `0x0076baa0`, then created blend entries
    for glam/guitarist source `0x00b8be10`.
- Interpretation: this trace closes the missing live branch proof for
  `0x001656a8 -> 0x00171190 -> 0x001710e0 -> 0x0016c1b0 -> 0x00198660`.
  The branch is not merely static fanout; it runs in-song for bass and drums
  and creates blend entries using source/event/script rows.

Accepted performer scheduler branch argument object sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_performer_scheduler_branch_arg_objects_20260611.json`.
- Screenshots:
  `pcsx2_performer_scheduler_branch_arg_objects_20260611.before_sample.window.png`
  and `pcsx2_performer_scheduler_branch_arg_objects_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 4 --interval 0.25 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_performer_scheduler_branch_arg_objects_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_performer_scheduler_branch_arg_objects_20260611.json" --target bass_drv_0113f2e0=0x0113f2e0:0x180 --target drum_drv_0135cb90=0x0135cb90:0x180 --target bass_event_005f7e50=0x005f7e50:0x140 --target bass_script_005f7bb0=0x005f7bb0:0x180 --target drum_event_005fa0b0=0x005fa0b0:0x140 --target drum_script_005f9ee0=0x005f9ee0:0x180 --target bass_clip_ctx_01158730=0x01158730:0x180 --target drum_clip_ctx_01359cc0=0x01359cc0:0x180 --target bass_blend_00770fd0=0x00770fd0:0x100 --target drum_blend_0076bad0=0x0076bad0:0x100 --target bass_clip_sched_0115fb40=0x0115fb40:0x120 --target drum_clip_sched_013bc010=0x013bc010:0x120`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay at
  normal speed (`FPS 60`, `VPS 60`, speed `100%`). The capture is not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Changed-row counts:
  - `bass_drv_0113f2e0`: `1`.
  - `drum_drv_0135cb90`: `2`.
  - `bass_event_005f7e50`: `0`.
  - `bass_script_005f7bb0`: `0`.
  - `drum_event_005fa0b0`: `0`.
  - `drum_script_005f9ee0`: `0`.
  - `bass_clip_ctx_01158730`: `0`.
  - `drum_clip_ctx_01359cc0`: `0`.
  - `bass_blend_00770fd0`: `8`.
  - `drum_blend_0076bad0`: `6`.
  - `bass_clip_sched_0115fb40`: `0`.
  - `drum_clip_sched_013bc010`: `0`.
- Object/layout facts:
  - Bass driver `0x0113f2e0 + 0x1c = 0x00b8df40`,
    `+0x30 = 0x01158730`, `+0x38 = 0x0076bad0`, `+0x3c` resolves to
    readable `starved`, and `+0x48` is the moving phase row.
  - Drummer driver `0x0135cb90 + 0x1c = 0x00b902e0`,
    `+0x30 = 0x01359cc0`, `+0x38 = 0x00768a50 -> 0x0076bcd0`, and `+0x48`
    is the moving phase row.
  - Bass event row `0x005f7e50` and drummer event row `0x005fa0b0` are stable
    `char/char_objects.dtb` rows in this slice. Each contains the readable
    `play` atom at row `+0x20`, then links to its script row:
    `0x005f7bb0` for bass and `0x005f9ee0` for drummer.
  - Bass script row `0x005f7bb0` carries `play_mode`, `idle`, and child event
    pointers such as `0x005f7ea0` / `0x005f7ed0`; drummer script row
    `0x005f9ee0` carries the same `play_mode` structure with drummer-specific
    child rows.
  - `0x01158730` and `0x01359cc0` are stable clip-context objects reached by
    `0x0016c1b0`; both have class pointers `0x003e2eb8` and `0x003de8a0`
    in the first 0x60 bytes.
  - Bass branch blend row `0x00770fd0` changed mode/status `+0x00`, float
    bands `+0x0c..+0x20`, and list/weight row `+0x28`; its static pointers
    include scheduler pointer `+0x24 = 0x010216f0`, source pointer
    `+0x2c = 0x00b8cf50`, and event/list pointers `+0x38..+0x48`.
  - Drummer branch blend row `0x0076bad0` changed float bands around
    `+0x0c` and `+0x4c..+0x60`; its pointers include scheduler pointer
    `+0x24 = 0x0115f030`, next/list pointer `+0x28 = 0x00768c50`, and source
    pointer `+0x2c = 0x00b8df40`.
- Interpretation: branch arguments are now tied to readable
  `char/char_objects.dtb` `play` script rows, stable clip contexts, and
  mutable blend entries. The zero-change script/event rows are authored data
  in this short sample; the mutable state lives on the driver, clip/scheduler
  rows, and blend entries.

Static clip lookup / blend child snippet dump:

- Static snippet report:
  `ps2_function_snippets_clip_lookup_branch_20260611.json`.
- Direct fanout summary:
  - `0x0016c1b0` calls `0x002b8020`, `0x00195b80`, `0x00321990`, and
    `0x002b07d0`.
  - `0x0016b2f0` calls `0x00193cb0`, `0x00193d78`, `0x00193e18`, and
    `0x0016ab88`.
  - `0x0016ab88` calls `0x001938f8`, `0x002ffd88`, `0x002dc500`, and
    `0x00168320`.
  - `0x00198660` calls `0x00198ac8`, `0x00199000`, `0x00195f18`,
    `0x00198a48`, `0x00196888`, `0x002d9d58`, and `0x002ffd88`.
  - `0x00196888` calls `0x001967b0`, `0x00196818`, and `0x002ffd88`.

Accepted clip lookup / blend children sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_clip_lookup_blend_children_sequence_20260611.json`.
- Screenshot:
  `pcsx2_clip_lookup_blend_children_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 30 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_clip_lookup_blend_children_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_clip_lookup_blend_children_sequence_20260611.json" --target clip_lookup_0016c1b0=0x0016c1b0 --target clip_child_00195b80=0x00195b80 --target blend_entry_init_00198660=0x00198660 --target blend_reset_00198ac8=0x00198ac8 --target blend_tick_00199000=0x00199000 --target blend_source_00195f18=0x00195f18 --target blend_release_00198a48=0x00198a48 --target blend_related_00196888=0x00196888 --target blend_related_a_001967b0=0x001967b0 --target blend_related_b_00196818=0x00196818`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay. The
  interpreter run was slow (`FPS/VPS 26`, speed `43%`), but the capture is not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Retained counts:
  - `clip_lookup_0016c1b0`: `6`.
  - `clip_child_00195b80`: `58`.
  - `blend_entry_init_00198660`: `43`.
  - `blend_reset_00198ac8`: `6`.
  - `blend_tick_00199000`: `48`.
  - `blend_source_00195f18`: `91`.
  - `blend_release_00198a48`: `43`.
  - `blend_related_00196888`: `39`.
  - `blend_related_a_001967b0`: `0`.
  - `blend_related_b_00196818`: `33`.
- Local order examples:
  - Generic blend-init burst:
    `0x00198a48(entry, 3, ...)` ->
    `0x00195f18(source_child, source_list, performer_child, ...)` ->
    `0x00198660(entry, performer_source, scheduler_data, flags)` ->
    `0x00199000(entry, ...)` ->
    `0x00195f18(...)` ->
    `0x00196888(scheduler_data, scheduler_data, flags, ...)` ->
    `0x00196818(...)`.
  - Drummer clip lookup:
    `0x0016c1b0(0x01359cc0, 0x005f9ee0, 1, 0x0076bd10)` ->
    repeated `0x00195b80(child, 0x01ffd770, 0x005f7b74, 0x0076bd10)` over
    child candidates including `0x013bced0`, `0x013bb8b0`, `0x013bc770`,
    `0x01399b20`, `0x013bdd90`, `0x013bc010`, `0x013bd9e0`, and
    `0x013bb500` ->
    `0x00198660(0x0076bd10, 0x00b902e0, 0x013bb500, 0x30)`.
  - Later drummer/bass branch-created blend entries use `0x2034` flags:
    `0x00198660(0x00768a50, 0x00b902e0, 0x013bb500, 0x2034)`,
    `0x00198660(0x00770fd0, 0x00b8df40, 0x0115fef0, 0x2034)`, and
    `0x00198660(0x0076bad0, 0x00b902e0, 0x013bc010, 0x2034)`.
  - Singer path also appears in the same accepted trace:
    `0x00198660(0x0076bd10, 0x00b8b800, 0x00d24f20, 0x34)`, with reset and
    source helpers around `0x00768c90`.
- Interpretation: clip lookup `0x0016c1b0` fans through
  `0x00195b80` over candidate child rows before blend-entry creation. Blend
  entry initialization consistently performs source/release/tick/related
  helper work around `0x00198660`. This narrows the remaining work to field
  semantics and the exact math in `0x00195b80`, `0x00195f18`,
  `0x00199000`, and `0x00196818`; it does not yet name those helper fields.

Accepted character deformation order sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_character_deform_order_sequence_20260611.json`.
- Screenshot:
  `pcsx2_character_deform_order_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 8 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_character_deform_order_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_character_deform_order_sequence_20260611.json" --target chardriver_update_00171830=0x00171830 --target clip_eval_0016b1d0=0x0016b1d0 --target clip_apply_0016b2f0=0x0016b2f0 --target clip_final_0016ab88=0x0016ab88 --target clip_output_00168320=0x00168320 --target ik_hand_0017a080=0x0017a080 --target foretwist_00175678=0x00175678 --target uppertwist_001823c8=0x001823c8 --target hair_00176fb8=0x00176fb8 --target lookat_0017d658=0x0017d658`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay. The
  interpreter run was slow (`FPS/VPS 28`, speed `48%`), but the capture is not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Ring note: total call counter reached `39451`, so the 16384-record ring
  wrapped. Counts below are retained steady-state records, not total-window
  counts.
- Retained counts:
  - `chardriver_update_00171830`: `1322`.
  - `clip_eval_0016b1d0`: `1510`.
  - `clip_apply_0016b2f0`: `2583`.
  - `clip_final_0016ab88`: `2583`.
  - `clip_output_00168320`: `6879`.
  - `ik_hand_0017a080`: `188`.
  - `foretwist_00175678`: `283`.
  - `uppertwist_001823c8`: `754`.
  - `hair_00176fb8`: `94`.
  - `lookat_0017d658`: `188`.
- Steady-state order facts:
  - Common driver/clip pattern:
    `0x00171830` ->
    `0x0016b1d0` ->
    `0x0016b2f0` ->
    repeated `0x00168320` output writes ->
    `0x0016ab88` final apply.
  - Drummer example:
    `0x00171830(0x0135cb90, ...)` ->
    `0x0016b1d0(0x013bb500, 0x0135ccec, ...)` ->
    `0x0016b2f0(0x013bb500, 0x0135ccec, ...)` ->
    `0x00168320(0x013bb638, 0x0135ccec, ...)` /
    `0x00168320(0x013bb584, 0x0135ccec, ...)` ->
    `0x0016ab88(0x013bb7a0, 0x0135ccec, ...)`.
  - Post-clip character deformation sequence appears after clip finalization:
    `0x0017a080` IK hand ->
    `0x00175678` foretwist ->
    second `0x0017a080` IK hand ->
    second `0x00175678` foretwist ->
    `0x00176fb8` hair ->
    `0x0017d658` look-at ->
    later `0x001823c8` upper-twist pairs before the next driver update.
  - Transition counts in the retained ring support this order:
    `0x00171830 -> 0x0016b1d0` (`1322`),
    `0x0016b1d0 -> 0x0016b2f0` (`1416`),
    `0x0016b2f0 -> 0x00168320` (`2582`),
    `0x00168320 -> 0x0016ab88` (`2583`),
    `0x0017a080 -> 0x00175678` (`188`),
    `0x00175678 -> 0x0017a080` (`94`),
    `0x00175678 -> 0x00176fb8` (`94`),
    `0x00176fb8 -> 0x0017d658` (`94`),
    and `0x001823c8 -> 0x001823c8` (`377`).
- Interpretation: for spaghetti-arm/hair/eye work, the PS2 order is not just
  "sample clips then draw." Clip evaluation and output writes happen first,
  then IK hand and foretwist passes interleave, then hair and look-at update,
  with upper-twist commonly appearing in pairs around the next driver cycle.
  Native character code must preserve this post-clip deformation ordering.

Accepted deformation helper object sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_deform_helper_objects_20260611.json`.
- Screenshots:
  `pcsx2_deform_helper_objects_20260611.before_sample.window.png` and
  `pcsx2_deform_helper_objects_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 4 --interval 0.25 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_deform_helper_objects_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_deform_helper_objects_20260611.json" --target ik_right_00dbfa40=0x00dbfa40:0x180 --target ik_left_00dbf4f0=0x00dbf4f0:0x180 --target fore_a_00d1f4d0=0x00d1f4d0:0x180 --target fore_b_00dbdf80=0x00dbdf80:0x180 --target hair_00dbf5a0=0x00dbf5a0:0x1c0 --target lookat_a_00dbe470=0x00dbe470:0x180 --target lookat_b_00dbf940=0x00dbf940:0x180 --target upper_a_00dbf620=0x00dbf620:0x180 --target upper_b_00d9e830=0x00d9e830:0x180 --target glam_source_00b8be10=0x00b8be10:0x180 --target bass_source_00b8df40=0x00b8df40:0x180`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay at
  normal speed (`FPS 60`, `VPS 60`, speed `100%`). The capture is not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Changed-row counts:
  - `ik_right_00dbfa40`: `4`.
  - `ik_left_00dbf4f0`: `3`.
  - `fore_a_00d1f4d0`: `0`.
  - `fore_b_00dbdf80`: `0`.
  - `hair_00dbf5a0`: `7`.
  - `lookat_a_00dbe470`: `3`.
  - `lookat_b_00dbf940`: `6`.
  - `upper_a_00dbf620`: `10`.
  - `upper_b_00d9e830`: `0`.
  - `glam_source_00b8be10`: `14`.
  - `bass_source_00b8df40`: `14`.
- Object/layout facts:
  - IK hand objects `0x00dbfa40` and `0x00dbf4f0` use table
    `0x003e79d0`, point to glam source `0x00b8be10` at `+0x1c`, and have
    target/position-like floats at `+0x50..+0x58` that changed every sample.
  - Foretwist objects `0x00d1f4d0` and `0x00dbdf80` use table
    `0x003e77a8`, point to glam source `0x00b8be10` at `+0x08`, link through
    child Trans pointers at `+0x14` / `+0x20`, and expose readable names
    `foreTwist_L.ik` and `foreTwist_R.ik`. They stayed stable in this short
    sample, despite firing in the order trace.
  - Hair object `0x00dbf5a0` uses table `0x003e77e8`, points to glam source
    `0x00b8be10` at `+0x10`, exposes readable `hair.hair`, and changed
    simulation/pose-like rows around `+0x11c..+0x154`.
  - LookAt objects `0x00dbe470` and `0x00dbf940` use table `0x003e7c28`,
    point to glam source `0x00b8be10` at `+0x24`, carry target Trans pointers
    around `+0x30..+0x48`, and changed look vector rows around
    `+0x70..+0x78`. `0x00dbf940` also shares the IK target rows at
    `+0x150..+0x158`.
  - UpperTwist objects `0x00dbf620` and `0x00d9e830` use table `0x003e8030`,
    point to glam source `0x00b8be10` at `+0x08`, expose readable
    `upperTwist_L.ik` / `upperTwist_R.ik`, and link child Trans pointers at
    `+0x14`, `+0x20`, and `+0x2c`. The left/object sample changed rows shared
    with hair (`+0x9c..+0xd4`) plus a second vector band around
    `+0x170..+0x178`; the right object stayed stable in this short sample.
  - Glam source `0x00b8be10` names `char/glam1/og/glam1.milo` and changed
    transform/pose bands around `+0x100..+0x174`. Bass source `0x00b8df40`
    names `char/metal_bass/og/metal_bass.milo` and changed equivalent bands
    around `+0x100..+0x174`.
- Interpretation: the helper objects are not free-floating effects. IK,
  foretwist, hair, look-at, and upper-twist all carry explicit owner/source
  links back to the character source and child Trans/object pointers. Hair and
  look-at have live mutable rows in this sample; foretwist and one upper-twist
  side can fire without changing their sampled state every four seconds.

Accepted live deformation string scan:

- Tool: `tools/scan_live_ee_strings.py`.
- Report: `pcsx2_live_strings_deform_helpers_20260611.json`.
- Screenshot:
  `pcsx2_live_strings_deform_helpers_20260611.window.png`.
- Command:
  `python tools\scan_live_ee_strings.py --pcsx2 "C:\Games\Emulators\PCSX2\pcsx2-qt.exe" --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --retry-pulses 2 --pre-retry-seconds 4 --settle-seconds 4 --snaps "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace" --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_live_strings_deform_helpers_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_live_strings_deform_helpers_20260611.json"`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay at
  normal speed (`FPS 60`, `VPS 60`, speed `100%`). The capture is not
  Retry/fail/startup/wrong-window. No PCSX2 process was left running.
- Relevant scan facts:
  - `hair.hair` has string hit `0x00db602d` and refs `0x00dbf5fc`,
    `0x00e0b1b0`.
  - `CharEyes` refs include `0x00dbf714` and `0x00e0b3d8`.
  - `foreTwist_L.ik` refs include `0x00d1f50c`, `0x00e09f08`,
    `0x0135cfdc`, and `0x013bb1d0`.
  - `foreTwist_R.ik` refs include `0x00dbdfbc` and `0x00e0a8e0`.
  - `upperTwist_L.ik` refs include `0x00dbf664`, `0x00e0b1e8`,
    `0x010d8b74`, `0x011418f0`, `0x012e8794`, and `0x013bac48`.
  - `upperTwist_R.ik` refs include `0x00d9e874`, `0x00e0a0c8`,
    `0x010dae54`, `0x01141a10`, `0x0135cf84`, and `0x013bb180`.
  - `hair_back.hair`, `hair_front.hair`, and `pony.hair` had no live string
    hits in this Battle of the Bands/glam1 state. This is not global negative
    evidence; the later GHDX rock2 trace proves live `hair_back.hair` and
    `hair_front.hair` controller rows for rock2.
- Interpretation: live string refs are useful seeds, but not all refs are
  helper object bases. Some point into directory/list rows that name meshes,
  textures, materials, or component links. Confirm each candidate with object
  sampling before treating it as an update object.

Accepted deformation helper reference object sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_deform_helper_ref_objects_20260611.json`.
- Screenshots:
  `pcsx2_deform_helper_ref_objects_20260611.before_sample.window.png` and
  `pcsx2_deform_helper_ref_objects_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 4 --interval 0.25 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_deform_helper_ref_objects_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_deform_helper_ref_objects_20260611.json" --target hair2_00e0b154=0x00e0b154:0x1c0 --target eyes1_00dbf6b8=0x00dbf6b8:0x180 --target eyes2_00e0b37c=0x00e0b37c:0x180 --target fore2_l_00e09ecc=0x00e09ecc:0x180 --target fore2_r_00e0a8a4=0x00e0a8a4:0x180 --target upper2_l_00e0b1a4=0x00e0b1a4:0x180 --target upper2_r_00e0a084=0x00e0a084:0x180 --target ik2_right_00e0af64=0x00e0af64:0x180 --target ik2_left_00e0b68c=0x00e0b68c:0x180 --target singer_src_00b8b800=0x00b8b800:0x180 --target bass_src_00b8df40=0x00b8df40:0x180 --target glam_src_00b8be10=0x00b8be10:0x180`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay at
  normal speed (`FPS 60`, `VPS 60`, speed `100%`). The capture is not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Changed-row counts:
  - `hair2_00e0b154`: `0`.
  - `eyes1_00dbf6b8`: `13`.
  - `eyes2_00e0b37c`: `0`.
  - `fore2_l_00e09ecc`: `0`.
  - `fore2_r_00e0a8a4`: `0`.
  - `upper2_l_00e0b1a4`: `0`.
  - `upper2_r_00e0a084`: `0`.
  - `ik2_right_00e0af64`: `0`.
  - `ik2_left_00e0b68c`: `0`.
  - `singer_src_00b8b800`: `0`.
  - `bass_src_00b8df40`: `14`.
  - `glam_src_00b8be10`: `14`.
- Object/layout facts:
  - `0x00dbf6b8` is a live eye/face-related block. It changed 13 rows,
    including bands at `+0x04/+0x08`, `+0x28..+0x3c`, `+0xd8..+0xe0`, and
    `+0x118..+0x120`. It contains table/class-like pointer `0x003e7658` at
    `+0x48`, a Trans/object pointer `0x00dbf740` at `+0x0c`, and the sampled
    `CharEyes` name ref at `+0x5c`.
  - Several ref-derived candidates such as `0x00e0b154`, `0x00e09ecc`,
    `0x00e0a8a4`, `0x00e0b1a4`, and `0x00e0a084` are directory/list records,
    not update objects. They contain readable mesh/texture/material names such
    as `glam1_hair.tex`, `glam1_eyes.tex`, `foreTwist_L.ik`,
    `upperTwist_L.ik`, and linked object pointers, but their sampled rows did
    not behave like standalone helper update objects.
  - `0x00e0b1a4` proves the `hair.hair` ref `0x00e0b1b0` is a directory link
    to the actual hair object `0x00dbf5e8`, not a second hair object.
  - `0x00e09f08` and `0x00e0a8e0` likewise link to the actual foretwist
    object records `0x00d1f4f8` and `0x00dbdfa8`.
  - `0x00e0b1e8` and `0x00e0a0c8` link to upper-twist object records
    `0x00dbf650` and `0x00d9e860`.
  - `0x00b8b800` is stable singer source in this sample and names
    `char/metal_singer/og/metal_singer.milo`; bass and glam source transform
    bands changed as in earlier samples.
- Interpretation: the scan/sample pair identifies the real live eye block and
  prevents a bad inference: many helper-name refs are directory links to
  existing objects, not independent helper instances. Native asset hookup needs
  to preserve these directory links so hair/eyes/twist helpers attach to the
  same source-owned objects as PS2.

Static CharEyes table confirmation:

- Static table report:
  `ps2_static_tables_chareyes_20260611.json`.
- Static function snippet report:
  `ps2_function_snippets_chareyes_update_20260611.json`.
- `0x003e7658` has a real dispatch-table shape and matches the live
  `CharEyes` block:
  - `0x003e7658 + 0x00 = 0x0000ff80`, a signed this-adjust style word.
  - `0x003e7658 + 0x04 = 0x0033a738`.
  - `0x003e7658 + 0x0c = 0x00174248`.
  - `0x003e7658 + 0x14 = 0x003c8ce8`.
  - `0x003e7658 + 0x1c = 0x002c2140`.
  - Later code slots include `0x001752f8`, `0x001753b8`, `0x00174d68`,
    `0x00174e50`, and `0x00174dd0`.
- Static child-call scan:
  - `0x00174248` calls object/list helpers `0x002c1df8` and
    `0x002d1c88`; this looks like reference/list setup or apply work, not
    the proven per-frame eye motion path in the sampled slices.
  - `0x0017d658` (`CharLookAt`) calls `0x003d8ea0`, `0x002ffa60`,
    `0x002dad00`, and `0x002daa30`.

Accepted live CharEyes vptr scan:

- Tool: `tools/scan_pcsx2_live_vptrs.py`.
- Report: `pcsx2_live_vptrs_chareyes_20260611.json`.
- Screenshot: `pcsx2_live_vptrs_chareyes_20260611.window.png`.
- Command:
  `python tools\scan_pcsx2_live_vptrs.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_live_vptrs_chareyes_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_live_vptrs_chareyes_20260611.json" --table 0x003e7658 --pre-retry-seconds 4 --retry-pulses 2 --post-retry-seconds 1 --settle 3 --require-screenshot`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay at
  normal speed (`FPS 60`, `VPS 60`, speed `100%`). The capture is not
  Retry/fail/startup/wrong-window.
- Result: one live table candidate:
  `vptr_addr 0x00dbf700 -> table 0x003e7658`, with `15` code slots. The
  first code slots were `0x0033a738`, `0x00174248`, `0x003c8ce8`,
  `0x002c2140`, `0x0033a830`, `0x0033a888`, `0x001752f8`, and
  `0x001753b8`.
- Interpretation: this confirms the live `CharEyes` table pointer at
  `0x00dbf700`; it does not prove that the `0x00174248` slot updates every
  frame.

Accepted CharEyes / LookAt direct sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_chareyes_update_sequence_20260611.json`.
- Screenshot: `pcsx2_chareyes_update_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 10 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_chareyes_update_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_chareyes_update_sequence_20260611.json" --target chareyes_update_00174248=0x00174248 --target chareyes_slot_001752f8=0x001752f8 --target chareyes_slot_001753b8=0x001753b8 --target chareyes_slot_00174d68=0x00174d68 --target chareyes_slot_00174e50=0x00174e50 --target chareyes_slot_00174dd0=0x00174dd0 --target lookat_update_0017d658=0x0017d658 --target trans_dirty_001dd748=0x001dd748 --target trans_world_003d8ea0=0x003d8ea0`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay. The
  interpreter run was slow (`FPS/VPS 33`, speed `55%`), but the capture is
  not Retry/fail/startup/wrong-window. `EnableEE = true` was verified
  afterward and no PCSX2 process was left running.
- Retained counts:
  - `chareyes_update_00174248`: `0`.
  - `chareyes_slot_001752f8`: `0`.
  - `chareyes_slot_001753b8`: `0`.
  - `chareyes_slot_00174d68`: `0`.
  - `chareyes_slot_00174e50`: `0`.
  - `chareyes_slot_00174dd0`: `0`.
  - `lookat_update_0017d658`: `12`.
  - `trans_dirty_001dd748`: `5766`.
  - `trans_world_003d8ea0`: `10606`.
- Interpretation: in this accepted active-song slice, the live `CharEyes`
  table was resident but its sampled direct slots did not dispatch. The
  visible eye/look-at motion path was `CharLookAt` plus Trans dirty/world
  helpers. Treat the `CharEyes` zero-hit result as phase/window-specific, not
  as proof that the class is dead.

Accepted LookAt child sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_lookat_children_sequence_20260611.json`.
- Screenshot: `pcsx2_lookat_children_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 12 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_lookat_children_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_lookat_children_sequence_20260611.json" --target chareyes_update_00174248=0x00174248 --target lookat_update_0017d658=0x0017d658 --target lookat_math_002ffa60=0x002ffa60 --target lookat_vec_002dad00=0x002dad00 --target lookat_norm_002daa30=0x002daa30`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay. The
  interpreter run was slow (`FPS/VPS 29`, speed `49%`), but the capture is
  not Retry/fail/startup/wrong-window. `EnableEE = true` was verified
  afterward and no PCSX2 process was left running.
- Retained counts:
  - `chareyes_update_00174248`: `0`.
  - `lookat_update_0017d658`: `152`.
  - `lookat_math_002ffa60`: `76`.
  - `lookat_vec_002dad00`: `152`.
  - `lookat_norm_002daa30`: `16004`.
- Local order examples:
  - `0x0017d658(0x00dbe470, 0x0017d658, 0x00dbe48c, 0)` for
    `l-eye.lookat`.
  - `0x0017d658(0x00dbf940, 0x0017d658, 0x00dbf95c, 0x007c4114)` for
    `r-eye.lookat`.
  - `0x002dad00` and `0x002daa30` surround stack-vector rows immediately near
    `0x0017d658`; the hot `0x002daa30` helper also services many other
    transform/vector rows, so it must not be interpreted as eye-only.
- Interpretation: `CharLookAt` updates both left and right eye look-at
  controllers in this slice. The child math helpers are trace-backed, but
  their exact vector field names still need static annotation.

Accepted CharEyes / LookAt object row sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_chareyes_lookat_object_rows_20260611.json`.
- Screenshots:
  `pcsx2_chareyes_lookat_object_rows_20260611.before_sample.window.png` and
  `pcsx2_chareyes_lookat_object_rows_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 6 --interval 0.20 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_chareyes_lookat_object_rows_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_chareyes_lookat_object_rows_20260611.json" --target chareyes_block_00dbf6b8=0x00dbf6b8:0x240 --target chareyes_vptr_00dbf700=0x00dbf700:0x180 --target chareyes_child_00dbf740=0x00dbf740:0x180 --target lookat_a_00dbe470=0x00dbe470:0x1c0 --target lookat_b_00dbf940=0x00dbf940:0x1c0 --target glam_source_00b8be10=0x00b8be10:0x180`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay at
  normal speed (`FPS 60`, `VPS 60`, speed `101%`) with visible venue lighting
  change during the sample. The capture is not Retry/fail/startup/wrong-window.
- Changed-row counts:
  - `chareyes_block_00dbf6b8`: `41`.
  - `chareyes_vptr_00dbf700`: `11`.
  - `chareyes_child_00dbf740`: `23`.
  - `lookat_a_00dbe470`: `3`.
  - `lookat_b_00dbf940`: `7`.
  - `glam_source_00b8be10`: `14`.
- Object/layout facts:
  - `0x00dbf700 -> 0x003e7658` is the live `CharEyes` vptr row inside the
    larger eye block. The broad block `0x00dbf6b8` mutates vector/pose bands
    at `+0x28..+0x40`, `+0xd8..+0xe0`, `+0x118..+0x120`,
    `+0x1b8..+0x1f0`, and related nearby rows in this sample.
  - `0x00dbf740` is the child object/Trans-like block tied to the same eye
    data; it changed 23 rows, including the same `0x00dbf790..0x00dbf8b8`
    transform bands.
  - `l-eye.lookat` object `0x00dbe470` uses `0x003e7c28` at `+0x20`, points
    to glam source `0x00b8be10` at `+0x24`, and points to the shared eye child
    `0x00dbf740` at `+0x48`. Its live look vector row
    `0x00dbe4e0..0x00dbe4e8` changed every sample.
  - `r-eye.lookat` object `0x00dbf940` has the same structure, points to
    shared eye child `0x00dbf740` at `+0x48`, and changed live look vector row
    `0x00dbf9b0..0x00dbf9b8`. It also changed IK/target-like rows
    `0x00dbfa90..0x00dbfa98` and `0x00dbfaf4`.
  - Both look-at objects point back to `char/glam1/og/glam1.milo` source
    `0x00b8be10`.
- Interpretation: eye motion in this slice is a linked system:
  `CharEyes` data is resident and mutable, but direct `CharEyes` table slots
  did not dispatch in the accepted direct traces; `CharLookAt` dispatched and
  mutated left/right look vectors plus the shared `0x00dbf740` eye child.
  Native eye fixes should preserve the shared eye child and look-at ownership
  links instead of treating eye meshes, eye look-at controllers, and
  `CharEyes` as independent loose attachments.

Static blend-helper deep dump:

- Static snippet report:
  `ps2_function_snippets_blend_helpers_deep_20260611.json`.
- Static fanout summary:
  - `0x00195b80` calls `0x002d3a48`, `0x002b7ff0`, recursively calls
    `0x00195b80`, then reaches `0x002b7f80`, `0x00101ec0`,
    `0x002b7eb0`, `0x00196610`, `0x002bb4f0`, `0x00169aa0`,
    `0x002b7e28`, and `0x002b7d38`.
  - `0x00195f18` calls list/source helpers `0x002b7b00`,
    `0x002b8020`, `0x002b8298`, `0x002b07d0`, and `0x002b1300`.
  - `0x00199000` is inline tick/math code in the dumped range and makes no
    direct `jal` calls.
  - `0x00196818` calls `0x001966f0`.
  - `0x00196888` calls `0x001967b0`, `0x00196818`, and `0x002ffd88`.
  - `0x00198660` calls `0x00198ac8`, `0x00199000`, `0x00195f18`,
    `0x00198a48`, `0x00196888`, `0x002d9d58`, and `0x002ffd88`.
  - `0x00198ac8` calls `0x00198a48`; `0x00198a48` calls `0x00195f18`,
    recursively calls itself, and calls `0x002d1c88`.

Accepted blend-helper child sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_blend_helper_children_sequence_20260611.json`.
- Screenshot: `pcsx2_blend_helper_children_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 30 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_blend_helper_children_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_blend_helper_children_sequence_20260611.json" --target clip_candidate_00195b80=0x00195b80 --target clip_child_00196610=0x00196610 --target clip_child_00169aa0=0x00169aa0 --target blend_source_00195f18=0x00195f18 --target blend_tick_00199000=0x00199000 --target blend_related_00196888=0x00196888 --target blend_related_a_001967b0=0x001967b0 --target blend_related_b_00196818=0x00196818 --target blend_related_b_child_001966f0=0x001966f0 --target blend_entry_init_00198660=0x00198660`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay. The
  interpreter run was slow (`FPS/VPS 26`, speed `44%`), but the capture is
  not Retry/fail/startup/wrong-window. `EnableEE = true` was verified
  afterward and no PCSX2 process was left running.
- Retained counts:
  - `clip_candidate_00195b80`: `58`.
  - `clip_child_00196610`: `30`.
  - `clip_child_00169aa0`: `0`.
  - `blend_source_00195f18`: `91`.
  - `blend_tick_00199000`: `48`.
  - `blend_related_00196888`: `39`.
  - `blend_related_a_001967b0`: `0`.
  - `blend_related_b_00196818`: `33`.
  - `blend_related_b_child_001966f0`: `33`.
  - `blend_entry_init_00198660`: `43`.
- Local order examples:
  - Generic blend entry creation:
    `0x00195f18(source_child, source_list, performer_child, ...)` ->
    `0x00198660(entry, performer_source, scheduler_data, flags)` ->
    `0x00199000(entry, ...)` ->
    `0x00195f18(...)` ->
    `0x00196888(...)` ->
    `0x00196818(...)` ->
    `0x001966f0(...)`.
  - Drummer clip candidate branch:
    `0x00195b80(0x013bced0, 0x01ffd770, 0x005f7b74, 0x0076bd10)` through
    candidate rows such as `0x013bb8b0`, `0x013bc770`, `0x013bdd90`,
    `0x013bc010`, and `0x013bb500`, then
    `0x00198660(0x0076bd10, 0x00b902e0, 0x013bb500, 0x30)`.
  - Hand/guitar clip candidate branch:
    repeated `0x00195b80(candidate, 0x01ffe4c0, symbol, flags)` immediately
    followed by `0x00196610(candidate, 0, symbol, flags)`, with symbols such
    as `0x005520e9`, `0x005520fa`, `0x00552037`, and `0x00552044`.
- Interpretation: the blend stack is now narrowed further:
  `0x00195b80` can either walk candidate rows without a child hit in the
  drummer/bass branch or immediately call `0x00196610` in hand/guitar
  candidate rows. `0x00196888` reaches the live related child path through
  `0x00196818 -> 0x001966f0`; the alternative `0x001967b0` branch remained
  zero in this accepted slice. Exact field names for the float/timing math
  inside `0x00199000`, `0x00196610`, and `0x001966f0` remain open.

Accepted blend child argument object sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_blend_child_arg_objects_20260611.json`.
- Screenshots:
  `pcsx2_blend_child_arg_objects_20260611.before_sample.window.png` and
  `pcsx2_blend_child_arg_objects_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 6 --interval 0.20 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_blend_child_arg_objects_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_blend_child_arg_objects_20260611.json" --target clip_hand_candidate_00f1f9d0=0x00f1f9d0:0x180 --target clip_hand_candidate_00f143c0=0x00f143c0:0x180 --target clip_guitar_candidate_00ebde00=0x00ebde00:0x180 --target blend_sched_01022960=0x01022960:0x180 --target blend_sched_drum_013bb500=0x013bb500:0x180 --target blend_related_src_00ebd2f0=0x00ebd2f0:0x180 --target blend_related_dst_00e0d150=0x00e0d150:0x180 --target blend_entry_00768a50=0x00768a50:0x180 --target blend_entry_00770fd0=0x00770fd0:0x180`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay at
  normal speed (`FPS 60`, `VPS 60`, speed `100%`). The capture is not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Changed-row counts:
  - `clip_hand_candidate_00f1f9d0`: `0`.
  - `clip_hand_candidate_00f143c0`: `0`.
  - `clip_guitar_candidate_00ebde00`: `0`.
  - `blend_sched_01022960`: `0`.
  - `blend_sched_drum_013bb500`: `10`.
  - `blend_related_src_00ebd2f0`: `10`.
  - `blend_related_dst_00e0d150`: `10`.
  - `blend_entry_00768a50`: `49`.
  - `blend_entry_00770fd0`: `8`.
- Object/layout facts:
  - Hand/guitar candidate rows sampled from the `0x00196610` branch were
    stable in this active window. Treat them as candidate/authored rows until
    a different phase proves otherwise.
  - `blend_sched_drum_013bb500` changed pointer bands
    `+0xec..+0x110`, rotating among `0x013c69f0..0x013c79ac` style rows.
  - `blend_related_src_00ebd2f0` changed equivalent pointer bands
    `+0xec..+0x110`, rotating through `0x00fd0ed0..0x00fd31dc`.
  - `blend_related_dst_00e0d150` changed equivalent pointer bands
    `+0xec..+0x110`, rotating through `0x00f2e150..0x00f2b28c`.
  - `blend_entry_00768a50` changed mode/flags at `+0x00`
    (`0x234 -> 0x2034` in the sample), timing/weight-like floats at
    `+0x0c..+0x20`, source pointer `+0x24`
    (`0x013bb500 -> 0x0115fb40`), sibling/next pointer `+0x28`
    (`0x0076bcd0 -> 0x00770f10` / `0`), target/source object `+0x2c`
    (`0x00b902e0 -> 0x00b8df40`), and additional packed blend rows from
    `+0x4c` onward.
  - `blend_entry_00770fd0` changed mode at `+0x00`, timing/weight-like rows
    `+0x0c..+0x20`, and next/sibling pointer `+0x28`
    (`0 -> 0x007989d0` in this sample).
- Interpretation: the live blend entries are the mutable blend/timing state,
  while several clip candidate rows are stable authored/list inputs. The
  related scheduler rows rotate pointer windows at consistent offsets
  `+0xec..+0x110`. Native blend code needs to preserve mode flags, source
  pointer, sibling pointer, target/source object pointer, and packed secondary
  blend rows; a single current-clip pointer is still not adequate.

Static camera output child dump:

- Static snippet report:
  `ps2_function_snippets_camera_output_deep_20260611.json`.
- Static fanout summary:
  - `0x00262b08` calls `0x00265d90`, `0x00307bc0`, `0x00263410`,
    `0x00266df8`, `0x002665a0`, and script/list helpers.
  - `0x00263410` calls `0x002ffd88`.
  - `0x002665a0` calls `0x002d97e8`, `0x002d9a30`, `0x001b1ee0`,
    `0x00266f80`, `0x002664d0`, `0x00267008`, and `0x002da808`.
  - `0x0026c900` has no direct `jal` in the dumped range.
  - `0x0026ae00` calls `0x003d8ea0`, `0x001dd748`, `0x002ff268`,
    `0x001b1ee0`, `0x001dd7b8`, and `0x00307bc0`.
  - `0x002ff268` calls `0x002ff6d0`.
  - `0x001b1ee0` calls `0x001b1f50`.

Accepted camera output child sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_camera_output_children_sequence_20260611.json`.
- Screenshot: `pcsx2_camera_output_children_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 20 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_camera_output_children_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_camera_output_children_sequence_20260611.json" --target cam_apply_00262b08=0x00262b08 --target cam_apply_child_00263410=0x00263410 --target cam_result_writer_002665a0=0x002665a0 --target cam_result_child_a_00266f80=0x00266f80 --target cam_result_child_b_002664d0=0x002664d0 --target cam_result_child_c_00267008=0x00267008 --target cam_final_copy_001b1ee0=0x001b1ee0 --target cam_path_iter_0026c900=0x0026c900 --target cam_path_apply_0026ae00=0x0026ae00 --target cam_path_child_002ff268=0x002ff268 --target cam_path_child_copy_002ff6d0=0x002ff6d0 --target cam_path_dirty_variant_001dd7b8=0x001dd7b8`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay. The
  interpreter run was slow (`FPS/VPS 27`, speed `45%`), but the capture is not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Total calls: `66032`. Retained-ring counts:
  - `cam_apply_00262b08`: `148`.
  - `cam_apply_child_00263410`: `148`.
  - `cam_result_writer_002665a0`: `148`.
  - `cam_result_child_a_00266f80`: `296`.
  - `cam_result_child_b_002664d0`: `148`.
  - `cam_result_child_c_00267008`: `296`.
  - `cam_final_copy_001b1ee0`: `1332`.
  - `cam_path_iter_0026c900`: `148`.
  - `cam_path_apply_0026ae00`: `148`.
  - `cam_path_child_002ff268`: `4362`.
  - `cam_path_child_copy_002ff6d0`: `5287`.
  - `cam_path_dirty_variant_001dd7b8`: `3923`.
- Local order examples:
  - Result branch:
    `0x00262b08(0x00b7a2d0, 0x00b7a2d0, 0x0059b4c0, 0x0041f738)` ->
    `0x00263410(0x00b7a2d0, stack rows)` ->
    `0x002665a0(0x00494b80, 0x00b7a2d0, 0x014dd4a0, 0x00b92ef0)` ->
    `0x001b1ee0(0x00b92ef0, 0x00b7a2d0, 0x014dd4a0, 0x00b92ef0)` ->
    `0x00266f80(0x00494b80, 0x00b92f50, 0x00b930e0, 0)` and
    `0x00266f80(0x014dd4a0, 0x00b92f50, 0x00b930e0, 0)` ->
    `0x002664d0(0x00494b80, 0x014dd4a0, 0, 0)` ->
    `0x00267008(0x00494b80, 0x00b7a2d0, 0x00b92ef0, stack)` and
    `0x00267008(0x014dd4a0, 0x00b7a2d0, 0x00b92ef0, stack)`.
  - Path-frame branch:
    `0x0026c900(0x00b8e9d0, 0x0026c900, 0x00b8ea10, phase)` ->
    `0x0026ae00(0x00b8e9d0, 0, 0, 0)` ->
    repeated `0x002ff268(0x00b8ead0, 0x00b8ead0, target, 0)` ->
    `0x001b1ee0(0x00b8ead0, 0x00b8ead0, target, 0)` ->
    `0x001dd7b8(target_trans, stack, 0x00b8ecc0, 0)`.
- Interpretation: the final camera output path is now trace-backed beyond the
  earlier `0x002665a0` and `0x0026ae00` bridge. `0x00b92ef0` is the live
  result frame, `0x00b92f50` and `0x00b930e0` are child/output blocks used by
  result helpers, `0x00b8ead0` is the path-frame output, and
  `0x00b8ecc0` is the dirty/update destination reached from path target rows.
  The exact field names still need annotation, but the runtime order and
  object handoff are no longer guesswork.

Accepted camera output child object sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_camera_output_child_objects_20260611.json`.
- Screenshots:
  `pcsx2_camera_output_child_objects_20260611.before_sample.window.png` and
  `pcsx2_camera_output_child_objects_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 6 --interval 0.20 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_camera_output_child_objects_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_camera_output_child_objects_20260611.json" --target cam_result_frame_00b92ef0=0x00b92ef0:0x260 --target cam_result_child_a_00b92f50=0x00b92f50:0x1c0 --target cam_result_child_b_00b930e0=0x00b930e0:0x1c0 --target cam_path_object_00b8e9d0=0x00b8e9d0:0x320 --target cam_path_frame_00b8ead0=0x00b8ead0:0x260 --target cam_path_dirty_dst_00b8ecc0=0x00b8ecc0:0x180 --target cam_target_007ce440=0x007ce440:0x120 --target cam_target_007d2d10=0x007d2d10:0x120`.
- Screenshot gate: both screenshots accepted active in-song Battle of the
  Bands gameplay at normal speed (`FPS 60`, `VPS 60`, speed `100%`). The
  captures are not Retry/fail/startup/wrong-window. `EnableEE = true` was
  verified afterward and no PCSX2 process was left running.
- Changed-row counts:
  - `cam_result_frame_00b92ef0`: `81`.
  - `cam_result_child_a_00b92f50`: `61`.
  - `cam_result_child_b_00b930e0`: `37`.
  - `cam_path_object_00b8e9d0`: `72`.
  - `cam_path_frame_00b8ead0`: `80`.
  - `cam_path_dirty_dst_00b8ecc0`: `37`.
  - `cam_target_007ce440`: `0`.
  - `cam_target_007d2d10`: `0`.
- Object/layout facts:
  - `0x00b92ef0` has pointer/table rows
    `+0x00=0x00b931e0`, `+0x04=0x00b93208`, `+0x08=0x003e6d88`,
    `+0x0c=0x00b931e0`, `+0x18/+0x1c=0x00792c10`,
    `+0xac=0x003e5830`, `+0xb0=0x00b931e0`, and `+0xb8=0x003e89b0`.
    It changed transform/basis clusters at `+0x20..+0x58`,
    `+0x60..+0x98`, `+0xc0..+0xf8`, and later projection/screen-like rows.
    Example values include basis rows around `0.4938`, `0.8688`, `-0.8235`,
    `0.4688`, `-0.3175`, `-0.2769`, `0.1553`, and `0.9476`, plus
    translation-like rows around `351.3`, `-95.5`, and `150.9`.
  - `0x00b92f50` is the child block passed to `0x00266f80`. Its first
    transform block mirrors `0x00b92ef0 + 0x20`, and it carries
    `+0x4c=0x003e5830`, `+0x50=0x00b931e0`, and `+0x58=0x003e89b0`.
  - `0x00b930e0` is the second child/output block passed to `0x00266f80`.
    It changed translation rows at `+0x00..+0x08`, screen/projection-like
    rows at `+0x0c`, `+0x4c`, `+0x5c`, `+0x6c`, and `+0xfc`, with
    `0 -> 768.0` transitions in the sample. It also carries table/object
    pointers including `+0xf4=0x003e93f0`, `+0xf8=0x00b931e0`,
    `+0x100=0x003e8918`, `+0x108=0x003f36d0`, and
    `+0x118=0x00b8c3f0`.
  - `0x00b8e9d0` still owns stable target/list pointers
    `+0x60=0x007ce440`, `+0x64=0x007d2d10`, and `+0x9c=0x00b8ead0`.
    Its moving output blocks start at `+0x120`, matching the path-frame
    transform pattern.
  - `0x00b8ead0` is the moving path-frame output. Its first transform cluster
    starts at `+0x20`, and pointer/table rows include
    `+0x00=0x00b8edc0`, `+0x04=0x00b8ede8`, `+0x08=0x003e6d88`,
    `+0x18/+0x1c=0x00b8eae8`, `+0xac=0x003e5830`,
    `+0xb0=0x00b8edc0`, and `+0xb8=0x003e89b0`.
  - `0x00b8ecc0` is the path dirty/update destination. It changed
    translation rows at `+0x00..+0x0c`, orientation rows at
    `+0x30..+0x68`, and later screen/projection-like rows; it carries
    pointers including `+0xf8=0x00b8edc0`, `+0xfc=0x007cf0d0`,
    `+0x110=0x00b8edc0`, `+0x120=0x007d25a0`, and
    `+0x124=0x007d2360`.
  - The sampled target descriptor/list rows `0x007ce440` and `0x007d2d10`
    stayed stable during the active sample. Treat them as authored/list
    inputs, not the moving camera pose.
- Interpretation: the render-camera side needs to preserve a multi-block
  camera result/path-frame structure. The live output is not reducible to one
  position/quaternion row. The final native handoff should model the result
  frame, child output block, path frame, dirty destination, and stable target
  descriptors separately.

Static lighting set-child dump:

- Static snippet report:
  `ps2_function_snippets_lighting_set_child_00271a08_20260611.json`.
- `0x00271a08` static child calls:
  `0x003ac4b8`, `0x002cf1d0`, `0x00305624`, `0x003aaa30`,
  `0x003ac2b0`, `0x002cf210`, `0x003ab068`, `0x003aaa80`, and
  `0x00313ca0`.

Accepted lighting writer child sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_lighting_writer_children_sequence_20260611.json`.
- Screenshot: `pcsx2_lighting_writer_children_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 45 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_lighting_writer_children_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_lighting_writer_children_sequence_20260611.json" --target lighting_set_00271288=0x00271288 --target lighting_key_prev_002716b8=0x002716b8 --target lighting_key_next_00271778=0x00271778 --target lighting_key_first_00271200=0x00271200 --target lighting_apply_prev_00280f60=0x00280f60 --target lighting_apply_alt_00280fe8=0x00280fe8 --target lighting_apply_next_00281070=0x00281070 --target lighting_finalize_00271f78=0x00271f78 --target lighting_finalize_alt_00271f70=0x00271f70 --target lighting_set_child_00271a08=0x00271a08 --target lighting_color_child_002c6808=0x002c6808 --target lighting_writer_003b50e0=0x003b50e0`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay. The
  interpreter run was slow (`FPS/VPS 34`, speed `57%`), but the capture is not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Counts:
  - `lighting_set_00271288`: `6`.
  - `lighting_key_prev_002716b8`: `2`.
  - `lighting_key_next_00271778`: `0`.
  - `lighting_key_first_00271200`: `0`.
  - `lighting_apply_prev_00280f60`: `2`.
  - `lighting_apply_alt_00280fe8`: `0`.
  - `lighting_apply_next_00281070`: `0`.
  - `lighting_finalize_00271f78`: `0`.
  - `lighting_finalize_alt_00271f70`: `0`.
  - `lighting_set_child_00271a08`: `8`.
  - `lighting_color_child_002c6808`: `0`.
  - `lighting_writer_003b50e0`: `0`.
- Local order examples:
  - `0x00271288(stack, 0x00b78418, 0x006006a0, 1)` ->
    `0x00271a08(0x00b78418, lighting row, 0x006006b0, 1)`, where lighting
    row rotated through `0x00842b20`, `0x00842ba0`, `0x00842c00`,
    `0x00842c40`, and `0x00842c80`.
  - `0x00271288(stack, 0x00b78418, 0x00600660, 1)` ->
    `0x00271a08(0x00b78418, 0x00b78460, 0x00600300, 1)`.
  - `0x002716b8(stack, 0x00b78418, 0x00600770, 1)` ->
    `0x00280f60(0x00520000, 0x00b78418, 0x00600770, 1)`.
- Interpretation: in this active-song window, the live lighting path uses the
  previous/keyframe branch `0x002716b8 -> 0x00280f60`, while the next/first
  sibling branches remained phase-gated zero. The `set_lighting` handler feeds
  live lighting rows through `0x00271a08`. The static children of
  `0x00280f60` (`0x002c6808` and `0x003b50e0`) did not fire in this window,
  so they remain static candidates rather than proven writer calls for this
  slice.

Accepted lighting writer child object sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_lighting_writer_child_objects_20260611.json`.
- Screenshots:
  `pcsx2_lighting_writer_child_objects_20260611.before_sample.window.png` and
  `pcsx2_lighting_writer_child_objects_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 8 --interval 0.20 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_lighting_writer_child_objects_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_lighting_writer_child_objects_20260611.json" --target world_light_state_00b78418=0x00b78418:0x100 --target lighting_cursor_00850fe0=0x00850fe0:0x100 --target lighting_row_00842b20=0x00842b20:0x120 --target lighting_row_00842ba0=0x00842ba0:0x120 --target lighting_row_00842c00=0x00842c00:0x120 --target lighting_row_00842c40=0x00842c40:0x120 --target lighting_row_00842c80=0x00842c80:0x120 --target color_candidate_007fe790=0x007fe790:0x220 --target color_alt_00782580=0x00782580:0x240 --target color_head_00845ca0=0x00845ca0:0x120`.
- Screenshot gate: both screenshots accepted active in-song Battle of the
  Bands gameplay at normal speed (`FPS 60`, `VPS 60`, speed `100%`) with a
  visible lighting change between captures. `EnableEE = true` was verified
  afterward and no PCSX2 process was left running.
- Changed-row counts:
  - `world_light_state_00b78418`: `8`.
  - `lighting_cursor_00850fe0`: `26`.
  - `lighting_row_00842b20`: `2`.
  - `lighting_row_00842ba0`: `1`.
  - `lighting_row_00842c00`: `0`.
  - `lighting_row_00842c40`: `0`.
  - `lighting_row_00842c80`: `0`.
  - `color_candidate_007fe790`: `6`.
  - `color_alt_00782580`: `8`.
  - `color_head_00845ca0`: `4`.
- Object/layout facts:
  - `0x00b78418` is the live world lighting state object. Rows
    `+0x60..+0x68` changed pointer/timer-like values, with `+0x64`
    reaching about `7.8685` and `+0x68` about `3.9601`; `+0x70` and
    `+0x80` changed pointer rows; `+0xc8` advanced `3 -> 4`.
  - `0x00850fe0` is the lighting cursor/state row. `+0x00` changed from
    `0x00842ee0` to `0x00842ba0`, and rows around `+0x90..+0xb0` changed
    through list/timing/value cells.
  - `0x00842b20` and `0x00842ba0` are script/list records touched in this
    window. They carry readable atoms such as `blackout`, `color1`,
    `lighting`, `verse`, `section`, `intro`, and `music_start`.
    Rows `0x00842c00`, `0x00842c40`, and `0x00842c80` stayed stable in this
    sample but carry `color1`, `color2`, `lighting`, `chorus_1`, and
    `chorus` atoms.
  - `0x007fe790`, `0x00782580`, and `0x00845ca0` are live color/light records.
    Their RGB-like rows at `+0x10..+0x18` changed during the same active
    sample, e.g. `0x007fe790 + 0x10` from about `0.2784` to `0.0431`, and
    `0x00845ca0 + 0x10..+0x18` from about `0.298/0.298/0.159` to
    `0.350/0/0.350`.
- Interpretation: lighting state is trace-backed as script cursor/list rows
  feeding world lighting state and render-light/color records. Native lighting
  should keep these as stateful/cursor-driven updates; do not implement venue
  lighting as a one-shot color assignment.

Accepted lighting set-child deeper sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_lighting_set_child_deeper_sequence_20260611.json`.
- Screenshot: `pcsx2_lighting_set_child_deeper_sequence_20260611.window.png`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay. The
  parent `0x00271a08` did not fire in this phase, so this trace is limited
  evidence only.
- Counts:
  - `lighting_set_child_00271a08`: `0`.
  - `lighting_child_b_002cf1d0`: `242`.
  - `lighting_child_c_00305624`: `939`.
  - `lighting_child_f_002cf210`: `243`.
  - All other traced `0x00271a08` static children were zero.
- Interpretation: `0x002cf1d0`, `0x00305624`, and `0x002cf210` are hot
  active-song helpers, but this run does not prove they were reached from
  `0x00271a08`. Do not use it as a lighting-parent chain proof without a
  future same-window parent hit.

Static crowd / venue record table dump:

- Static table report:
  `ps2_static_tables_crowd_venue_records_20260611.json`.
- Tables resolved from live crowd/venue objects:
  - Crowd event table `0x003edf60`: code slots include `0x00385490`,
    `0x00223848`, `0x003c8ce8`, and `0x002c2140`.
  - Crowd aux table `0x003edfe0`: code slots include `0x00385418`,
    `0x00385768`, `0x00385918`, and `0x00385920`.
  - Crowd record table `0x003ed268`: code slots include `0x0037f2a8`,
    `0x0037f320`, `0x0037f4d0`, and `0x0037f4d8`.
  - World descriptor table `0x003eea78`: code slots include `0x0038b3d8`,
    `0x0022efa0`, `0x0022f848`, and `0x0022f858`.
  - Descriptor table `0x003eea08`: code slots include `0x00389aa8`,
    `0x0022e040`, `0x0022e0f0`, and `0x0022e188`.

Accepted crowd / venue callback sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_crowd_venue_callbacks_sequence_20260611.json`.
- Screenshot: `pcsx2_crowd_venue_callbacks_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 45 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_crowd_venue_callbacks_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_crowd_venue_callbacks_sequence_20260611.json" --target world_event_00123d08=0x00123d08 --target world_game_00124310=0x00124310 --target world_win_00123c28=0x00123c28 --target downbeat_xref_00122188=0x00122188 --target crowd_tbl_ctor_00385490=0x00385490 --target crowd_tbl_apply_00223848=0x00223848 --target crowd_aux_ctor_00385418=0x00385418 --target crowd_aux_a_00385768=0x00385768 --target crowd_aux_b_00385918=0x00385918 --target crowd_aux_c_00385920=0x00385920 --target crowd_rec_ctor_0037f2a8=0x0037f2a8 --target crowd_rec_a_0037f320=0x0037f320 --target crowd_rec_b_0037f4d0=0x0037f4d0 --target crowd_rec_c_0037f4d8=0x0037f4d8 --target desc_ea08_cb14_0022e0f0=0x0022e0f0 --target desc_ea08_cb5c_0022e270=0x0022e270 --target desc_ea08_cb64_0022e2e0=0x0022e2e0`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay. The
  interpreter run was slow (`FPS/VPS 32`, speed `54%`), but the capture is not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Counts:
  - `world_event_00123d08`: `4`.
  - `world_game_00124310`: `2`.
  - `world_win_00123c28`: `2`.
  - `downbeat_xref_00122188`: `0`.
  - `crowd_tbl_ctor_00385490`: `12`.
  - `crowd_tbl_apply_00223848`: `0`.
  - `crowd_aux_c_00385920`: `14`.
  - Other crowd aux/record static table slots traced in this run were zero.
  - `desc_ea08_cb14_0022e0f0`: `2236`.
  - `desc_ea08_cb5c_0022e270`: `31`.
  - `desc_ea08_cb64_0022e2e0`: `31`.
- Local order examples:
  - Crowd/descriptor construction/update window:
    `0x00385490(0x00385490, 0x00385490, 1, 0x01618250)` twice, followed by
    descriptor callbacks on row `0x00770d90`:
    `0x0022e270(0x00770d90, 0, -1, 0x00770d90)`,
    `0x0022e2e0(0x00770d90, 0, -1, 0x41000000)`,
    `0x0022e270(0x00770d90, 0, -1, 0x008515e0)`, and
    `0x0022e0f0(0x00770d90, 0x01ffe348, -1, 0x41000000)`.
  - Additional descriptor row families used `0x00768a10`, `0x00768c10`,
    `0x0076bd90`, `0x0076bcd0`, `0x00746910`, and `0x00770e50`.
  - World event to crowd event:
    `0x00123d08(0x00ad2aa0, 0x00550d62, 0x0060ba20, 0x10)` ->
    `0x00124310(0x00ad2aa0, 0x00b94bc0, 0x00c9d060, 0x0021eaf8)`.
  - A later world event used:
    `0x00123d08(0x00ad2aa0, 0x00550d53, 0x0060ba00, 0x10)` ->
    `0x00124310(0x00ad2aa0, 0x00b95f10, 0x00c9d060, 0x0021eaf8)`.
  - World win/event bridge:
    `0x00123c28(0x00ad2aa0, 1 or 2, 1, 0)` immediately preceded or followed
    `0x00123d08(...)` in the same active-song window.
  - Crowd aux callback examples:
    `0x00385920(0x016182ac, 0x0072bf50, 0, 0x016182ac)`,
    `0x00385920(0x0161838c, 0x0072bd50, 0, 0x0161838c)`, and
    `0x00385920(0x00b94c1c, 0x007997d0, 0, 0x00b94c1c)`.
- Interpretation: this trace backs the live crowd/venue dispatch layer beyond
  generic world-event hits. It ties `world_event` to crowd event objects and
  proves the descriptor callback layer is actively mutating/list-walking the
  rows reached from named venue/crowd records. It still does not prove the
  `downbeat` xref path, nor the zero-hit crowd table slots, in this phase.

Accepted crowd / venue callback object sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_crowd_venue_callback_objects_20260611.json`.
- Screenshots:
  `pcsx2_crowd_venue_callback_objects_20260611.before_sample.window.png` and
  `pcsx2_crowd_venue_callback_objects_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 8 --interval 0.20 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_crowd_venue_callback_objects_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_crowd_venue_callback_objects_20260611.json" --target crowd_event_begin_00b94ae0=0x00b94ae0:0x180 --target crowd_event_norm_00b94bc0=0x00b94bc0:0x180 --target crowd_event_new_00b95f10=0x00b95f10:0x180 --target crowd_aux_obj_00b94c1c=0x00b94c1c:0x120 --target desc_row_00768a10=0x00768a10:0x120 --target desc_row_00768c10=0x00768c10:0x120 --target desc_row_00746910=0x00746910:0x120 --target desc_row_00770e50=0x00770e50:0x120 --target desc_row_00770d90=0x00770d90:0x120 --target crowd_aux_target_007997d0=0x007997d0:0x120`.
- Screenshot gate: both screenshots accepted active in-song Battle of the
  Bands gameplay at normal speed (`FPS 60`, `VPS 60`, speed `100%`) with a
  visible stage-lighting change. `EnableEE = true` was verified afterward and
  no PCSX2 process was left running.
- Changed-row counts:
  - `crowd_event_begin_00b94ae0`: `6`.
  - `crowd_event_norm_00b94bc0`: `4`.
  - `crowd_event_new_00b95f10`: `0`.
  - `crowd_aux_obj_00b94c1c`: `3`.
  - `desc_row_00768a10`: `40`.
  - `desc_row_00768c10`: `35`.
  - `desc_row_00746910`: `14`.
  - `desc_row_00770e50`: `24`.
  - `desc_row_00770d90`: `29`.
  - `crowd_aux_target_007997d0`: `8`.
- Object/layout facts:
  - `0x00b94ae0` is named `crowd_begin` at `+0x14`, uses table
    `0x003edf60` at `+0x00`, points to aux table `0x003ed268` at `+0x44`,
    and changed flag/pointer rows at `+0x60..+0x64` and `+0x140..+0x144`.
  - `0x00b94bc0` is named `crowd_upto_norm` at `+0x14`, uses the same
    `0x003edf60` / `0x003ed268` table pattern, and changed rows at
    `+0x28`, `+0x60..+0x64`, and `+0x100`.
  - `0x00b95f10` is named `crowd_dnto_poor` at `+0x14`, uses the same crowd
    event table pattern, and stayed stable in this sample. This matches the
    `0x00b95f10` argument observed in the accepted callback sequence.
  - `0x00b94c1c` is an aux object tied to `crowd_upto_norm`; it uses table
    `0x003edfe0` at `+0x00`, points back to `0x00b94bc0` at `+0x0c`, and
    changed rows `+0x04..+0x08` and `+0xa4`.
  - Descriptor rows `0x00768a10`, `0x00768c10`, `0x00746910`,
    `0x00770e50`, and `0x00770d90` all changed during the sample. Several
    carry descriptor table `0x003eea08` in-row and point back to performer or
    world objects such as `0x00b902e0`, `0x00b8df40`, `0x00b8b800`, and
    `0x00b8be10`.
  - `0x007997d0` is a crowd aux target with a live pointer at `+0x7c` that
    changed to `0x00b94bc0`, and it names
    `world/battle/og/battle_lighting.milo` at `+0x90`.
- Interpretation: `crowd_begin`, `crowd_upto_norm`, and `crowd_dnto_poor`
  are now mapped to concrete PS2 objects, and the descriptor rows that the
  callback layer mutates are live. The exact named-row mapping for every
  `crowd_lighters_*`, `band_jump`, and `downbeat` event is still open, but
  this removes one more fuzzy layer between world-event dispatch and concrete
  crowd object state.

Accepted lighter / band-jump / downbeat list-value sequence trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_lighter_bandjump_downbeat_sequence_20260611.json`.
- Screenshot: `pcsx2_lighter_bandjump_downbeat_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 45 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_lighter_bandjump_downbeat_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_lighter_bandjump_downbeat_sequence_20260611.json" --target world_event_00123d08=0x00123d08 --target world_game_00124310=0x00124310 --target world_win_00123c28=0x00123c28 --target desc_cb14_0022e0f0=0x0022e0f0 --target desc_cb5c_0022e270=0x0022e270 --target desc_cb64_0022e2e0=0x0022e2e0 --target list_prepare_00223fc0=0x00223fc0 --target list_prepare_child_0022b8f8=0x0022b8f8 --target list_float_walker_00223340=0x00223340 --target list_float_child_0022c1f0=0x0022c1f0 --target list_update_bridge_002230c8=0x002230c8 --target list_value_setter_00223400=0x00223400 --target list_update_child_002232d8=0x002232d8 --target list_update_child_0022c168=0x0022c168 --target list_walk_leaf_0022c1a0=0x0022c1a0 --target list_value_child_0022c2c0=0x0022c2c0`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay. The
  interpreter run was slow (`FPS/VPS 29`, speed `48%`), but the capture is not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Counts:
  - `world_event_00123d08`: `4`.
  - `world_game_00124310`: `2`.
  - `world_win_00123c28`: `2`.
  - `desc_cb14_0022e0f0`: `2265`.
  - `desc_cb5c_0022e270`: `31`.
  - `desc_cb64_0022e2e0`: `31`.
  - `list_prepare_00223fc0`: `11`.
  - `list_prepare_child_0022b8f8`: `35`.
  - `list_float_walker_00223340`: `11`.
  - `list_float_child_0022c1f0`: `14`.
  - `list_update_bridge_002230c8`: `11`.
  - `list_value_setter_00223400`: `14`.
  - `list_update_child_002232d8`: `14`.
  - `list_update_child_0022c168`: `14`.
  - `list_walk_leaf_0022c1a0`: `14`.
  - `list_value_child_0022c2c0`: `17`.
- Local order examples:
  - `0x00123d08(0x00ad2aa0, 0x00550d62, 0x0060ba20, 0x10)` ->
    `0x00124310(0x00ad2aa0, 0x00b94bc0, 0x00c9d060, 0x0021eaf8)` ->
    `0x00223fc0(0x00b94bc0, 0x006688c0, 0x0084f7b0, 0x00223150)` ->
    `0x0022b8f8(0x00b94be8, 0x006688c0, 0x0084f7b0, 0x00223150)`.
  - The same branch reached list/value application:
    `0x00223340(0x007997d0, 0x0c, 0x00456a18, 0x00854140)` ->
    `0x0022c1f0(0x00746910, 0x0c, 0x00456a18, 0x00854140)` and
    `0x0022c1f0(0x00770e50, stack, -1, 0x00854140)`, followed by
    `0x002230c8`, `0x00223400`, `0x0022c2c0`, `0x002232d8`,
    `0x0022c168`, and `0x0022c1a0`.
  - The later `0x00550d53 / 0x0060ba00` branch routed into
    `0x00b95f10` and then into the same list/value family with target
    rows such as `0x0083cbc0` and descriptor rows `0x00746910` /
    `0x00768c90`.
  - Auxiliary/event rows later used `0x00b94a00`, `0x00b94a28`,
    `0x0082d790`, and row targets such as `0x00746680`,
    `0x00742820`, and `0x00747090`.
- Interpretation: this proves the active world-event branch reaches the
  generic list/value application layer in the same ring as `world_event` and
  `world_game`, instead of stopping at static named records. It still does not
  prove which exact named row was the live `band_jump`, `downbeat`, or
  `crowd_lighters_*` dispatch in this slice.

Accepted lighter / band-jump / downbeat object sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_lighter_bandjump_downbeat_objects_20260611.json`.
- Screenshots:
  `pcsx2_lighter_bandjump_downbeat_objects_20260611.before_sample.window.png`
  and `pcsx2_lighter_bandjump_downbeat_objects_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 8 --interval 0.20 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_lighter_bandjump_downbeat_objects_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_lighter_bandjump_downbeat_objects_20260611.json" --target crowd_event_norm_00b94bc0=0x00b94bc0:0x180 --target crowd_event_poor_00b95f10=0x00b95f10:0x180 --target list_target_battle_lighting_007997d0=0x007997d0:0x180 --target desc_row_00746910=0x00746910:0x180 --target desc_row_00770e50=0x00770e50:0x180 --target desc_row_00768c90=0x00768c90:0x180 --target desc_row_0076bcd0=0x0076bcd0:0x180 --target value_source_0082d790=0x0082d790:0x180 --target crowd_event_aux_00b94a00=0x00b94a00:0x180 --target crowd_event_aux_child_00b94a28=0x00b94a28:0x180 --target list_row_00854140=0x00854140:0x180 --target list_row_0083cbc0=0x0083cbc0:0x180 --target list_row_00746680=0x00746680:0x180 --target list_row_00742820=0x00742820:0x180 --target list_row_00747090=0x00747090:0x180 --target event_script_0084f7b0=0x0084f7b0:0x180 --target event_owner_006688c0=0x006688c0:0x180`.
- Screenshot gate: both screenshots accepted active in-song Battle of the
  Bands gameplay at normal speed (`FPS 60`, `VPS 60`, speed `100%`) with
  visible venue/camera/lighting changes. `EnableEE = true` was verified
  afterward and no PCSX2 process was left running.
- Changed-row counts:
  - `crowd_event_norm_00b94bc0`: `7`.
  - `crowd_event_poor_00b95f10`: `0`.
  - `list_target_battle_lighting_007997d0`: `8`.
  - `desc_row_00746910`: `14`.
  - `desc_row_00770e50`: `15`.
  - `desc_row_00768c90`: `9`.
  - `desc_row_0076bcd0`: `31`.
  - `value_source_0082d790`: `10`.
  - `crowd_event_aux_00b94a00`: `4`.
  - `crowd_event_aux_child_00b94a28`: `3`.
  - `list_row_00854140`: `33`.
  - `list_row_0083cbc0`: `0`.
  - `list_row_00746680`: `6`.
  - `list_row_00742820`: `3`.
  - `list_row_00747090`: `3`.
  - `event_script_0084f7b0`: `7`.
  - `event_owner_006688c0`: `0`.
- Object/layout facts:
  - `0x00b94bc0` remains `crowd_upto_norm`; this sample changed
    `+0x28`, `+0x60..+0x64`, `+0x100..+0x104`, and related pointer rows.
  - `0x00b95f10` remains `crowd_dnto_poor`; it was reached in the sequence
    trace but stayed stable in this short sample.
  - `0x007997d0` is the `world/battle/og/battle_lighting.milo` target row.
    It changed table/list pointers and `+0x7c` again reached `0x00b94bc0`.
  - Descriptor rows `0x00746910`, `0x00770e50`, `0x00768c90`, and
    `0x0076bcd0` changed timing/float/list rows during the sample.
    `0x00770e50 + 0x40` carries atom `in_solo`.
  - `0x00b94a00` is another crowd/event object row with name pointer
    `0x00b93671` (`crowd_preview_loop`) and the same table-family pattern.
    Its child `0x00b94a28` uses the `0x003edfe0` aux-table pattern and changed
    linked rows near `+0xe0..+0xe4`.
  - `0x00747090 + 0x44` names `battle_lighting_RndDir`, tying one sampled
    list row to venue lighting random-directory state.
  - `0x0084f7b0` changed header flags/pointers and later rows
    `+0x150..+0x158`; its in-row ASCII decodes to "PRESS ANY BUTTON TO ROCK",
    so treat this row as mixed live/script UI memory and do not over-classify
    it as a pure event object.
- Interpretation: the branch now has concrete moving rows below the list/value
  helper calls. This narrows the next trace to naming which exact authored
  rows map to `band_jump`, `downbeat`, and the `crowd_lighters_*` records, not
  rediscovering the PCSX2 launch path or generic callback layer.

Accepted band-jump / downbeat authored-row object sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_bandjump_downbeat_row_objects_20260611.json`.
- Screenshots:
  `pcsx2_bandjump_downbeat_row_objects_20260611.before_sample.window.png` and
  `pcsx2_bandjump_downbeat_row_objects_20260611.window.png`.
- Screenshot gate: both screenshots accepted active in-song Battle of the
  Bands gameplay at normal speed (`FPS 60`, `VPS 60`, speed `100%`) with
  visible venue lighting/camera changes. `EnableEE = true` was verified
  afterward and no PCSX2 process was left running.
- Changed-row counts:
  - `event_script_chain_0084f7b0`: `24`.
  - `band_jump_row_0084f860`: `0`.
  - `band_jump_arg_a_008461e0`: `54`.
  - `band_jump_arg_b_0084f380`: `4`.
  - `game_outro_row_0084f880`: `0`.
  - `game_outro_arg_a_00743520`: `3`.
  - `game_outro_arg_b_0072c940`: `0`.
  - `game_over_row_0084f8a0`: `3`.
  - `game_over_arg_a_00846210`: `57`.
  - `game_over_arg_b_0084f3a0`: `4`.
- Object/layout facts:
  - `0x0084f860` is the authored `band_jump` row:
    `+0x00=0x0084f880`, `+0x04=0x0084f840`, `+0x08=0x00b8a740`,
    `+0x10=0x008461e0`, `+0x14=0x0084f380`, `+0x18=0x00b8c170`,
    `+0x1c=0x0054fa14` (`band_jump`).
  - `0x008461e0` is a moving `band_jump` payload row. It changed 54 rows and
    carries a mirrored `band_jump` atom at `+0x0c`.
  - `0x0084f380` is the secondary event chain reached from `band_jump`;
    it also starts with a `band_jump` atom and then contains `game_over`,
    `game_lost`, `game_won_msg`, `crowd_lighters_slow`, `downbeat`,
    `excitement`, and additional linked rows.
  - The primary chain at `0x0084f7b0` includes authored atoms such as
    `active_players_changed`, `sync_head_bang`, `sync_wag`, `band_jump`,
    `game_outro_complete`, `game_over`, `peak_off_player`, `peak_on_player`,
    `peak_off`, `peak_on`, `crowd_half_tempo`, `solo_on/off`,
    `intro_start_msg`, `crowd_lighters_fast`, `phrase_miss`, `play`,
    `blew_big_note`, and `end_streak`.
- Interpretation: `band_jump` row identity is now trace-backed at the PS2
  authored-row level. This is not yet a full per-handler semantics trace; it
  maps the live rows and moving payloads to continue from.

Accepted downbeat / crowd-lighters authored-row object sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_downbeat_lighters_row_objects_20260611.json`.
- Screenshots:
  `pcsx2_downbeat_lighters_row_objects_20260611.before_sample.window.png` and
  `pcsx2_downbeat_lighters_row_objects_20260611.window.png`.
- Screenshot gate: both screenshots accepted active in-song Battle of the
  Bands gameplay at normal speed (`FPS 60`, `VPS 60`, speed `100%`) with
  visible venue lighting/camera changes. `EnableEE = true` was verified
  afterward and no PCSX2 process was left running.
- Changed-row counts:
  - `secondary_event_chain_0084f380`: `4`.
  - `lighters_slow_row_0084f3f0`: `4`.
  - `lighters_slow_arg_a_00742b20`: `0`.
  - `lighters_slow_arg_b_00845880`: `0`.
  - `downbeat_row_0084f400`: `4`.
  - `downbeat_arg_a_00850e80`: `24`.
  - `downbeat_arg_b_0083c900`: `3`.
  - `downbeat_data_a_00845820`: `0`.
  - `downbeat_data_b_00845820_dup`: `0`.
- Object/layout facts:
  - `0x0084f3f0` is `crowd_lighters_slow`:
    `+0x00=0x00742b20`, `+0x04=0x00845880`, `+0x08=0x00b8ea90`,
    `+0x0c=0x00545e6d` (`crowd_lighters_slow`).
  - `0x0084f400` is `downbeat`:
    `+0x00=0x00850e80`, `+0x04=0x0083c900`, `+0x08=0x00b8e1b0`,
    `+0x0c=0x00548802` (`downbeat`).
  - `0x0083c900` is a moving `downbeat` payload/list row; it changed 3 rows
    and carries `downbeat` at `+0x0c`, plus nearby `crowd_double_tempo`,
    `hit_hihat`, `game_over`, and `crash_symbal` atoms.
  - The static linked block `0x00845820..0x00845998` names the crowd/lighter
    row family: `crowd_hide`, `crowd_update`, `crowd_lighters_slow`,
    `crowd_lighters_fast`, `crowd_lighters_off`, `crowd_half_tempo`,
    `crowd_double_tempo`, `crowd_normal_tempo`, `hit_hihat`, and `start`.
- Interpretation: exact authored-row identities are now trace-backed for
  `band_jump`, `downbeat`, and `crowd_lighters_slow/fast/off`. Remaining work
  is per-atom handler semantics and any phase-specific branches that did not
  execute in this active-song slice.

Accepted authored-message candidate sequence follow-up:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_authored_message_candidates_sequence_20260611.json`.
- Screenshot: `pcsx2_authored_message_candidates_sequence_20260611.window.png`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay with a
  close band/camera view. The interpreter run was slow (`FPS/VPS 27`, speed
  `45%`), but the capture is not Retry/fail/startup/wrong-window.
  `EnableEE = true` was verified afterward and no PCSX2 process was left
  running.
- Counts:
  - `beat_ref_a_00121ec4`: `0`.
  - `beat_ref_b_00121ff8`: `0`.
  - `downbeat_xref_00122188`: `0`.
  - `downbeat_ref_001221ac`: `0`.
  - `band_jump_msg_0010bc54`: `0`.
  - `band_jump_runtime_0010cb9c`: `0`.
  - `world_event_00123d08`: `5`.
  - `world_game_00124310`: `3`.
  - `world_win_00123c28`: `2`.
  - `list_prepare_00223fc0`: `13`.
  - `list_float_walker_00223340`: `13`.
  - `list_value_setter_00223400`: `17`.
  - `desc_cb14_0022e0f0`: `2536`.
  - `desc_cb5c_0022e270`: `38`.
  - `desc_cb64_0022e2e0`: `38`.
- Local order examples:
  - `0x00223fc0(0x01618250, 0x00534428, 1, 0x01618250)` ->
    `0x0022e270(0x00770d90, 0, -1, 0x00770d90)` ->
    `0x0022e2e0(0x00770d90, 0, -1, 0x41000000)` ->
    `0x00223340(0x0072bf50, 0x0c, 0x00456a18, 0x008515e0)` ->
    `0x00223400(0x0072bf50, stack, -1, 0x008515e0)`.
  - The same shape repeated through container `0x0072bd50` and target
    `0x008500d0`, and through container `0x0072bf50` and target
    `0x0084ae30`.
- Interpretation: the direct beat/downbeat and `band_jump` code candidates
  stayed cold in this accepted 60-second gameplay window, while the
  world-event/list/value/descriptor path was live again. Treat this as proof
  of the current generic authored-event application layer, not as proof that
  the cold direct candidates are dead globally.

Accepted authored-message target-object sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_authored_message_target_objects_20260611.json`.
- Screenshots:
  `pcsx2_authored_message_target_objects_20260611.before_sample.window.png`
  and `pcsx2_authored_message_target_objects_20260611.window.png`.
- Screenshot gate: both screenshots accepted active in-song Battle of the
  Bands gameplay at normal speed (`FPS 60`, `VPS 60`, speed `100%`) with
  visible band, camera, and venue lighting changes. `EnableEE = true` was
  verified afterward and no PCSX2 process was left running.
- Changed-row counts:
  - `list_container_a_0072bf50`: `8`.
  - `list_container_b_0072bd50`: `14`.
  - `list_target_a_008515e0`: `1`.
  - `list_target_b_008500d0`: `11`.
  - `list_target_c_0084ae30`: `35`.
  - `desc_row_00770d90`: `29`.
  - `desc_row_00768a10`: `39`.
  - `desc_row_00768c10`: `35`.
  - `desc_row_0076bd90`: `28`.
  - `live_stack_01618250`: `0`.
  - `live_stack_01618330`: `1`.
  - `live_stack_01618410`: `4`.
- Object/layout facts:
  - `0x0072bf50` exposes readable rows `ui/impactor2.milo` and `ui/gen`.
    It changed list/container links at `+0x00`, `+0x08`,
    `+0x20/+0x24`, `+0x68/+0x6c/+0x74`, and stack/source link `+0x7c`.
  - `0x0072bd50` exposes `excitement_level`. It changed its leading
    container/list rows and later pointer/value rows at `+0x100..+0x10c`.
  - `0x0084ae30` is the hottest sampled target row in this follow-up
    (`35` changed rows), rotating through the same venue/list cluster seen in
    the prior descriptor target samples.
  - Descriptor rows remain mutable state, not static metadata:
    `0x00770d90` exposes `normal_color`, `focus_color`,
    `selecting_color`, `disabled_color`, and `in_solo`; `0x00768a10`,
    `0x00768c10`, and `0x0076bd90` mutate float/list/link fields during the
    accepted active-song sample.
  - Live stack rows `0x01618250`, `0x01618330`, and `0x01618410` expose
    `nowbar_1`, `nowbar_2`, `nowbar_3`, `nowbar_4`, and `_default`.
    `0x01618410` changed `+0x28`, `+0x60`, `+0x64`, and `+0x108`, including
    the `0x0084ae30` link observed in the sequence trace.
- Interpretation: the generic authored-message list/value layer is tied to
  concrete live containers, descriptor rows, and transient stack records in
  the same active gameplay window. Continue from these moving rows only when
  tracing the generic event/list path; do not use them as a substitute for
  per-atom handler semantics for `band_jump`, `downbeat`, or
  `crowd_lighters_*`.

Accepted focused world-win parent/child sequence rerun:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_world_win_parent_child_sequence_20260611.json`.
- Screenshot: `pcsx2_world_win_parent_child_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 60 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_win_parent_child_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_win_parent_child_sequence_20260611.json" --target world_win_00123c28=0x00123c28 --target world_win_child_002aff10=0x002aff10 --target world_win_child_00123c60=0x00123c60 --target world_event_00123d08=0x00123d08 --target world_event_child_002d27d0=0x002d27d0 --target world_event_child_0031ac80=0x0031ac80 --target world_event_child_00123e38=0x00123e38 --target world_game_00124310=0x00124310 --target world_game_child_00223e60=0x00223e60 --target world_game_child_00124380=0x00124380 --target world_game_child_00223dc8=0x00223dc8 --target list_prepare_00223fc0=0x00223fc0 --target list_float_walker_00223340=0x00223340 --target list_value_setter_00223400=0x00223400`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay. The
  interpreter run was slow (`VPS 43`, speed `71%`), but the capture is not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Counts:
  - `world_win_child_00123c60`: `1`.
  - `world_win_child_002aff10`: `1`.
  - All other targeted parent/event/game/list functions, including
    `world_win_00123c28`, recorded `0` calls.
- Retained order:
  - `0x00123c60(0x0069b960, 0x00558f6f, 0, 0x30)` ->
    `0x002aff10(0x00558f6f, 0x0054638e, 0, 0x401b)`.
- Interpretation: this is accepted active-song evidence that the two
  world-win child bodies can fire in the current save window, but it still
  does not prove runtime order under parent `0x00123c28`. Keep the parent
  branch marked incomplete; do not infer parent-child semantics from this
  isolated child-only hit.

Accepted focused world-win parent/child tight rerun:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_world_win_parent_child_tight_sequence_20260611.json`.
- Screenshot: `pcsx2_world_win_parent_child_tight_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 90 --ring-size 4096 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_win_parent_child_tight_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_win_parent_child_tight_sequence_20260611.json" --target world_win_00123c28=0x00123c28 --target world_win_child_002aff10=0x002aff10 --target world_win_child_00123c60=0x00123c60`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay. The
  interpreter run was slow (`VPS 43`, speed `72%`), but the capture is not
  Retry/fail/startup/wrong-window. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Counts:
  - `world_win_00123c28`: `0`.
  - `world_win_child_002aff10`: `1`.
  - `world_win_child_00123c60`: `1`.
- Retained order matched the previous child-only run:
  - `0x00123c60(0x0069b960, 0x00558f6f, 0, 0x30)` ->
    `0x002aff10(0x00558f6f, 0x0054638e, 0, 0x401b)`.
- Static cross-check: the `0x00123c28` body still statically calls
  `0x002aff10` first and then `0x00123c60`. The repeated active-song runtime
  traces did not catch those children through the parent; they only prove that
  the child bodies are live elsewhere in this save window.

Accepted character controller target sampler from ISO/state:

- Tool: `tools/trace_pcsx2_controller_targets.py`.
- Report: `pcsx2_controller_targets_iso_state1_20260611.json`.
- Screenshots:
  `pcsx2_controller_targets_iso_state1_20260611.before.window.png` and
  `pcsx2_controller_targets_iso_state1_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_controller_targets.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --retry-pulses 2 --post-retry-seconds 1 --seconds 12 --poll-interval 0.002 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_controller_targets_iso_state1_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_controller_targets_iso_state1_20260611.json"`.
- Screenshot gate: both screenshots accepted active in-song Battle of the
  Bands gameplay at normal speed (`FPS 60`, `VPS 60`, speed `100%`). The
  capture is not Retry/fail/startup/wrong-window. `EnableEE = true` was
  verified afterward and no PCSX2 process was left running.
- Tick samples:
  - Total tick samples: `160`.
  - Slot counts: `0x00175678` (`CharForeTwist`) `40`,
    `0x00176fb8` (`CharHair`) `40`,
    `0x0017d658` (`CharLookAt`) `40`,
    `0x001823c8` (`CharUpperTwist`) `40`.
  - Repeated sampled order:
    `0x00175678` -> `0x00176fb8` -> `0x0017d658` -> `0x001823c8`.
  - First retained args:
    `0x00175678(a0=0x00dbdf80, a1=0x01c80080, a2=0x00dbdf80, a3=0)`;
    `0x00176fb8(a0=0x00dbf5a0, a1=0x01c80f00, a2=0x00dbf5a8, a3=0x40c90fdb)`;
    `0x0017d658(a0=0x00dbf940, a1=0x01c81d80, a2=0x00dbf95c, a3=0x007c4114)`;
    `0x001823c8(a0=0x00d9e830, a1=0x01c82c00, a2=0x00d9e830, a3=0x3f3553a7)`.
- Change summary:
  - Controller header objects stayed stable in this 160-sample window:
    `fore_twist_l_obj`, `fore_twist_r_obj`, `upper_twist_l_obj`,
    `upper_twist_r_obj`, `hair_obj`, and `char_eyes_obj` each changed `0`
    rows.
  - The referenced Trans rows moved every tick. Examples:
    `fore_twist_l_hand_ref` changed `18` rows,
    `fore_twist_l_twist2_ref` changed `16`,
    `fore_twist_r_hand_ref` changed `18`,
    `fore_twist_r_twist2_ref` changed `16`,
    `upper_twist_l_upper_arm_ref` changed `21`,
    `upper_twist_l_twist1_ref` changed `16`,
    `upper_twist_l_twist2_ref` changed `19`,
    `upper_twist_r_upper_arm_ref` changed `21`,
    `upper_twist_r_twist1_ref` changed `16`, and
    `upper_twist_r_twist2_ref` changed `19`.
  - `r_eye_lookat_obj` changed `3` rows at `+0x70`, `+0x74`, and `+0x78`;
    the broader `char_eyes_obj` stayed stable in this tick sampler.
- Interpretation: this accepted ISO/state trace confirms that deformation
  controller headers can remain stable while PS2 writes animated state through
  the referenced hand, arm, twist, and eye rows. Native character fixes must
  follow the controller's linked Trans rows and not rely on mutation of the
  controller header object itself.

Trace-helper update for isolated IK sampling:

- File: `tools/trace_pcsx2_controller_targets.py`.
- Change: added explicit IK watch rows and `--no-default-vptrs` / `--vptr`
  options so shared table slots can be isolated one live vptr at a time.
- Reason: both IK hands share table `0x003e79d0`; when both are redirected at
  once, the tick sampler records the last call's registers for the shared
  slot. Isolated vptr runs are required for unambiguous right/left IK args.
- This is instrumentation-only and does not change native animation code.

Accepted isolated right-hand IK controller sampler:

- Tool: `tools/trace_pcsx2_controller_targets.py`.
- Report: `pcsx2_controller_targets_ik_right_iso_state1_20260611.json`.
- Screenshots:
  `pcsx2_controller_targets_ik_right_iso_state1_20260611.before.window.png`
  and `pcsx2_controller_targets_ik_right_iso_state1_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_controller_targets.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --retry-pulses 2 --post-retry-seconds 1 --seconds 8 --poll-interval 0.002 --no-default-vptrs --vptr exact_ik_hand_r=0x00DBFA58:0x003E79D0 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_controller_targets_ik_right_iso_state1_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_controller_targets_ik_right_iso_state1_20260611.json"`.
- Screenshot gate: both screenshots accepted active in-song Battle of the
  Bands gameplay at normal speed (`FPS 60`, `VPS 60`, speed `100%`). The
  capture is not Retry/fail/startup/wrong-window.
- Redirect: `exact_ik_hand_r` vptr `0x00dbfa58`, expected/live table
  `0x003e79d0`, redirected to copied table `0x01e80000`.
- Tick samples: `160`, all slot `0x0017a080`.
- First retained args:
  `0x0017a080(a0=0x00dbfa40, a1=0x01c80080, a2=0x00dbfa54, a3=0)`.
- IK row movement in this accepted window:
  - `ik_hand_r_obj`: `3` rows changed (`+0x50/+0x54/+0x58`), all 160 unique.
  - `ik_hand_r_vptr` sample region: the same moving vector appears at
    relative `+0x38/+0x3c/+0x40`.
  - `ik_hand_r_target`: `1` row changed (`+0x44`).
  - `ik_hand_r_hand_ref`: `18` rows changed.
  - `ik_hand_r_elbow_ref`: `15` rows changed.
- Interpretation: right-hand IK's live update object is `0x00dbfa40`, target
  row is `0x00dbfa54`, and the animated output is not confined to the IK
  header. The linked hand and elbow Trans rows are active every sampled tick.

Accepted isolated left-hand IK controller sampler:

- Tool: `tools/trace_pcsx2_controller_targets.py`.
- Report: `pcsx2_controller_targets_ik_left_iso_state1_20260611.json`.
- Screenshots:
  `pcsx2_controller_targets_ik_left_iso_state1_20260611.before.window.png`
  and `pcsx2_controller_targets_ik_left_iso_state1_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_controller_targets.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --retry-pulses 2 --post-retry-seconds 1 --seconds 8 --poll-interval 0.002 --no-default-vptrs --vptr exact_ik_hand_l=0x00DBF508:0x003E79D0 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_controller_targets_ik_left_iso_state1_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_controller_targets_ik_left_iso_state1_20260611.json"`.
- Screenshot gate: both screenshots accepted active in-song Battle of the
  Bands gameplay at normal speed (`FPS 60`, `VPS 60`, speed `100%`). The
  capture is not Retry/fail/startup/wrong-window. `EnableEE = true` was
  verified after the isolated IK runs and no PCSX2 process was left running.
- Redirect: `exact_ik_hand_l` vptr `0x00dbf508`, expected/live table
  `0x003e79d0`, redirected to copied table `0x01e80000`.
- Tick samples: `160`, all slot `0x0017a080`.
- First retained args:
  `0x0017a080(a0=0x00dbf4f0, a1=0x01c80080, a2=0x00dbf504, a3=0x40c90fdb)`.
- IK row movement in this accepted window:
  - `ik_hand_l_obj`: `3` rows changed (`+0x50/+0x54/+0x58`), all 160 unique.
  - `ik_hand_l_vptr` sample region: the same moving vector appears at
    relative `+0x38/+0x3c/+0x40`.
  - `ik_hand_l_target`: `0` rows changed in this short sample.
  - `ik_hand_l_hand_ref`: `18` rows changed.
  - `ik_hand_l_elbow_ref`: `15` rows changed.
- Interpretation: left-hand IK's live update object is `0x00dbf4f0`, target
  row is `0x00dbf504`, and the linked hand/elbow Trans rows carry the live
  animation state. The different `a3` values between right (`0`) and left
  (`0x40c90fdb`) are now trace-backed and should not be guessed.

Accepted isolated foretwist / uppertwist side samplers:

- Tool: `tools/trace_pcsx2_controller_targets.py`.
- Reports:
  - `pcsx2_controller_targets_foretwist_l_iso_state1_20260611.json`.
  - `pcsx2_controller_targets_foretwist_r_iso_state1_20260611.json`.
  - `pcsx2_controller_targets_uppertwist_l_iso_state1_20260611.json`.
  - `pcsx2_controller_targets_uppertwist_r_iso_state1_20260611.json`.
- Screenshots:
  - `pcsx2_controller_targets_foretwist_l_iso_state1_20260611.before.window.png`
    and `pcsx2_controller_targets_foretwist_l_iso_state1_20260611.window.png`.
  - `pcsx2_controller_targets_foretwist_r_iso_state1_20260611.before.window.png`
    and `pcsx2_controller_targets_foretwist_r_iso_state1_20260611.window.png`.
  - `pcsx2_controller_targets_uppertwist_l_iso_state1_20260611.before.window.png`
    and `pcsx2_controller_targets_uppertwist_l_iso_state1_20260611.window.png`.
  - `pcsx2_controller_targets_uppertwist_r_iso_state1_20260611.before.window.png`
    and `pcsx2_controller_targets_uppertwist_r_iso_state1_20260611.window.png`.
- Screenshot gate: all eight screenshots accepted active in-song Battle of the
  Bands gameplay at normal speed (`FPS 60`, `VPS 60`, speed `99-100%`). None
  were Retry/fail/startup/wrong-window. `EnableEE = true` was verified
  afterward and no PCSX2 process was left running.
- Commands used the accepted ISO/state route and isolated one vptr at a time:
  - Foretwist L:
    `--no-default-vptrs --vptr exact_fore_twist_l_a=0x00D1F4D4:0x003E77A8`.
  - Foretwist R:
    `--no-default-vptrs --vptr exact_fore_twist_r_a=0x00DBDF84:0x003E77A8`.
  - Uppertwist L:
    `--no-default-vptrs --vptr exact_upper_twist_l_a=0x00DBF624:0x003E8030`.
  - Uppertwist R:
    `--no-default-vptrs --vptr exact_upper_twist_r_a=0x00D9E834:0x003E8030`.
- Tick samples and first retained args:
  - Foretwist L: `160` ticks of `0x00175678`;
    `a0=0x00d1f4d0`, `a1=0x01c80080`, `a2=0x00d1f4d0`,
    `a3=0x01ffe778`.
  - Foretwist R: `160` ticks of `0x00175678`;
    `a0=0x00dbdf80`, `a1=0x01c80080`, `a2=0x00dbdf80`, `a3=0`.
  - Uppertwist L: `160` ticks of `0x001823c8`;
    `a0=0x00dbf620`, `a1=0x01c80080`, `a2=0x00dbf620`, `a3=0`.
  - Uppertwist R: `160` ticks of `0x001823c8`;
    `a0=0x00d9e830`, `a1=0x01c80080`, `a2=0x00d9e830`,
    `a3=0x3f369a28`.
- Row movement:
  - Foretwist L header stayed stable; `fore_twist_l_hand_ref` changed `18`
    rows and `fore_twist_l_twist2_ref` changed `16`.
  - Foretwist R header stayed stable; `fore_twist_r_hand_ref` changed `18`
    rows and `fore_twist_r_twist2_ref` changed `16`.
  - Uppertwist L header changed `1` row in the sampled region; linked
    `upper_twist_l_upper_arm_ref` changed `21` rows,
    `upper_twist_l_twist1_ref` changed `16`, and
    `upper_twist_l_twist2_ref` changed `19`.
  - Uppertwist R header stayed stable; linked
    `upper_twist_r_upper_arm_ref` changed `21` rows,
    `upper_twist_r_twist1_ref` changed `16`, and
    `upper_twist_r_twist2_ref` changed `19`.
- Interpretation: foretwist and uppertwist side identity is now trace-backed
  instead of inferred from shared-table mixed samples. Native twist fixes must
  preserve the left/right controller object identity and the side-specific
  `a3` values, while writing through linked hand/upper-arm/twist Trans rows.

Accepted focused IK/twist/hair child-call traces:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Broad report: `pcsx2_ik_twist_hair_children_sequence_20260611.json`.
- Broad screenshot:
  `pcsx2_ik_twist_hair_children_sequence_20260611.window.png`.
- Broad screenshot gate: accepted active in-song Battle of the Bands gameplay
  with visible venue, band, crowd, and lighting. `EnableEE = true` was
  verified afterward and no PCSX2 process was left running.
- Broad counts:
  - `ik_hand_0017a080`: `10`.
  - `ik_pre_0017a558`: `10`.
  - `foretwist_00175678`: `15`.
  - `uppertwist_001823c8`: `40`.
  - `hair_00176fb8`: `5`.
  - `hair_reset_00176ab0`: `0`.
  - `twist_norm_002dadf8`: `50`.
  - `twist_vec_002dae80`: `50`.
  - `twist_apply_a_002ffc60`: `100`.
  - `twist_apply_b_002ffd88`: `195`.
  - `shared_angle_002dc500`: `1834`.
  - `lookat_vec_002dad00`: `10`.
  - `vector_norm_002daa30`: `1050`.
  - `trans_dirty_001dd748`: `4734`.
  - `trans_world_003d8ea0`: `8281`.
- The broad trace's retained ring was full and dominated by shared transform
  helpers, so branch-specific traces below are the preferred evidence for
  order.

IK branch:

- Report: `pcsx2_ik_branch_sequence_20260611.json`.
- Screenshot: `pcsx2_ik_branch_sequence_20260611.window.png`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay.
  `EnableEE = true` was verified afterward and no PCSX2 process was left
  running.
- Counts:
  - `ik_hand_0017a080`: `6`.
  - `ik_pre_0017a558`: `6`.
  - `lookat_vec_002dad00`: `6`.
  - `vector_norm_002daa30`: `630`.
  - `trans_dirty_001dd748`: `2801`.
  - `trans_world_003d8ea0`: `4743`.
- Order facts:
  - `0x0017a080` on `0x00dbfa40` / `0x00dbf4f0`.
  - Immediately followed by `0x0017a558` with the same object and target args.
  - Then multiple `0x003d8ea0` world-transform resolves.
  - Then `0x001dd748` dirty propagation.
  - `0x002dad00` fires at the same six-call cadence as IK in this slice.
- Interpretation: IK is a live per-frame-ish transform feeder. It is not a
  static stance or bind-pose correction.

Twist branch:

- Report: `pcsx2_twist_branch_sequence_20260611.json`.
- Screenshot: `pcsx2_twist_branch_sequence_20260611.window.png`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay.
  `EnableEE = true` was verified afterward and no PCSX2 process was left
  running.
- Counts:
  - `foretwist_00175678`: `15`.
  - `uppertwist_001823c8`: `40`.
  - `twist_norm_002dadf8`: `50`.
  - `twist_vec_002dae80`: `50`.
  - `vector_norm_002daa30`: `1050`.
  - `twist_apply_a_002ffc60`: `100`.
  - `twist_apply_b_002ffd88`: `200`.
  - `shared_angle_002dc500`: `1850`.
  - `trans_dirty_001dd748`: `4837`.
- Order facts:
  - Foretwist repeats as `0x00175678 -> 0x002dadf8 -> 0x002dae80 ->
    0x002daa30 -> 0x002ffc60 -> 0x002ffd88 -> dirty/angle propagation`.
  - Uppertwist repeats as `0x001823c8 -> 0x002dadf8 -> 0x002dae80 ->
    0x002daa30 -> 0x002ffc60 -> 0x002dc500/0x001dd748`.
  - Upper-twist appears in paired object groups such as
    `0x00dbf620`/`0x00d9e830` and `0x010d8b30`/`0x010dae10`.
- Interpretation: spaghetti-arm fixes must preserve PS2's helper order and
  target-row writes; a single native roll split is not trace-backed.

Hair branch:

- Report: `pcsx2_hair_branch_sequence_20260611.json`.
- Screenshot: `pcsx2_hair_branch_sequence_20260611.window.png`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay.
  `EnableEE = true` was verified afterward and no PCSX2 process was left
  running.
- Counts:
  - `hair_00176fb8`: `3`.
  - `hair_reset_00176ab0`: `0`.
  - `trans_world_003d8ea0`: `5355`.
  - `trans_dirty_001dd748`: `2834`.
- Order facts:
  - Hair repeats as `0x00176fb8` on `0x00dbf5a0`, then
    `0x003d8ea0` calls over child/root transform rows including
    `0x00db81f0`, `0x00db9ef0`, `0x00dbaef0`, and `0x00dbc7f0`.
  - `0x00176ab0` did not fire in this active-song window.
- Interpretation: normal sampled hair update is a live transform path. The
  reset helper is field-gated or phase-specific and still needs a separate
  trigger if native code later needs to reproduce reset semantics.

Accepted deformation branch object sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_deform_branch_objects_20260611.json`.
- Screenshots:
  `pcsx2_deform_branch_objects_20260611.before_sample.window.png` and
  `pcsx2_deform_branch_objects_20260611.window.png`.
- Screenshot gate: both screenshots accepted active in-song Battle of the
  Bands gameplay; final screenshot was normal speed (`FPS 60`, `VPS 60`,
  speed `100%`). `EnableEE = true` was verified afterward and no PCSX2
  process was left running.
- Changed-row counts:
  - `ik_right_00dbfa40`: `4`.
  - `ik_left_00dbf4f0`: `3`.
  - `fore_a_00d1f4d0`: `0`.
  - `fore_b_00dbdf80`: `0`.
  - `upper_a_00dbf620`: `10`.
  - `upper_b_00d9e830`: `0`.
  - `upper_c_010d8b30`: `0`.
  - `upper_d_010dae10`: `0`.
  - `hair_00dbf5a0`: `7`.
  - `ik_target_right_00dbfa54`: `4`.
  - `ik_target_left_00dbf504`: `3`.
  - `hair_child_a_00db81f0`: `12`.
  - `hair_child_b_00db9ef0`: `21`.
  - `hair_child_c_00dbaef0`: `21`.
  - `hair_child_d_00dbc7f0`: `12`.
  - `fore_work_a_00dba210`: `21`.
  - `fore_work_b_00db8a10`: `21`.
  - `upper_work_a_00dbac10`: `22`.
  - `upper_work_b_01142d60`: `26`.
- Object/link facts:
  - IK objects `0x00dbfa40` / `0x00dbf4f0` use table `0x003e79d0`, point
    to owner/source `0x00b8be10`, expose names `left_hand.ik` and
    `right_hand.ik`, and mutate vector rows around `+0x50..+0x58`.
  - Foretwist controller headers `0x00d1f4d0` / `0x00dbdf80` expose
    `foreTwist_L.ik` / `foreTwist_R.ik`, owner/source `0x00b8be10`, and
    Trans links including `0x00db89f0`, `0x00db6ef0`, `0x00dba1f0`, and
    `0x00dba0f0`. The headers stayed stable in this sample, but downstream
    work rows `0x00dba210` and `0x00db8a10` moved and name
    `bone_R-hand.mesh` / `bone_L-hand.mesh`.
  - Upper-twist headers expose `upperTwist_L.ik` / `upperTwist_R.ik`.
    Glam-linked `0x00dbf620` points to owner/source `0x00b8be10` and moved;
    bass-linked `0x010d8b30` / `0x010dae10` point to owner/source
    `0x00b8df40` and stayed stable in this short sample. Moving output rows
    include `0x00dbac10` and `0x01142d60`, both naming
    `bone_L-upperArm.mesh`.
  - Hair object `0x00dbf5a0` uses table `0x003e77e8`, points to
    owner/source `0x00b8be10`, exposes `hair.hair`, links onward to
    `upperTwist_L.ik` and `CharEyes.eyes`, and changed rows around
    `+0x11c..+0x154`.
  - Hair child/root rows `0x00db81f0`, `0x00db9ef0`, `0x00dbaef0`, and
    `0x00dbc7f0` moved and name `bone_hair01.mesh`, `bone_head.mesh`,
    `bone_neck.mesh`, and `bone_bangL.mesh`.
- Interpretation: static controller headers are not proof of inactive
  deformation. The PS2 writes through child Trans/work rows. Native fixes for
  arms and hair need those owner/source and child-row links preserved.

Accepted look-at / eye branch rerun:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_lookat_branch_sequence_rerun_20260611.json`.
- Screenshot: `pcsx2_lookat_branch_sequence_rerun_20260611.window.png`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay.
  `EnableEE = true` was verified afterward and no PCSX2 process was left
  running.
- Counts:
  - `lookat_0017d658`: `6`.
  - `lookat_child_002ffa60`: `3`.
  - `lookat_vec_002dad00`: `6`.
  - `vector_norm_002daa30`: `630`.
  - `lookat_dirty_alt_001dd7b8`: `66`.
  - `lookat_math_002d5fd8`: `6`.
  - `trans_dirty_001dd748`: `2799`.
  - `trans_world_003d8ea0`: `4676`.
- Order facts:
  - `0x0017d658` runs on look-at objects `0x00dbe470` and `0x00dbf940`.
  - It resolves pivot/source/shared-head transforms with `0x003d8ea0`.
  - It calls `0x002d5fd8` on math rows `0x00dbe500` and `0x00dbf9d0`.
  - It then dirties source/output Trans rows with `0x001dd748`.

Accepted look-at / eye object sample:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_lookat_branch_objects_rerun_20260611.json`.
- Screenshots:
  `pcsx2_lookat_branch_objects_rerun_20260611.before_sample.window.png` and
  `pcsx2_lookat_branch_objects_rerun_20260611.window.png`.
- Screenshot gate: both screenshots accepted active in-song Battle of the
  Bands gameplay; final screenshot was normal speed (`FPS 60`, `VPS 60`,
  speed `100%`). `EnableEE = true` was verified afterward and no PCSX2
  process was left running.
- Changed-row counts:
  - `lookat_a_00dbe470`: `3`.
  - `lookat_b_00dbf940`: `7`.
  - `lookat_a_target_00dbe48c`: `3`.
  - `lookat_b_target_00dbf95c`: `3`.
  - `lookat_math_a_00dbe500`: `0`.
  - `lookat_math_b_00dbf9d0`: `3`.
  - `eyes_00dbf700`: `23`.
  - `shared_head_00db9ef0`: `21`.
  - `source_a_00766880`: `25`.
  - `source_b_00779070`: `25`.
  - `pivot_00dbf740`: `11`.
- Object/link facts:
  - Look-at objects `0x00dbe470` and `0x00dbf940` expose
    `l-eye.lookat` and `r-eye.lookat`, use table `0x003e7c28`, and point to
    owner/source `0x00b8be10`.
  - Their source rows are `0x00766880` and `0x00779070`; those rows name
    `eye-L.mesh` and `eye-R.mesh` and each changed 25 rows.
  - Both look-at objects use shared head row `0x00db9ef0`, which names
    `bone_head.mesh` and changed 21 rows, plus pivot row `0x00dbf740`,
    which changed 11 rows.
  - `eyes_00dbf700` exposes `CharEyes.eyes`, points to owner/source
    `0x00b8be10`, and changed 23 rows.
- Interpretation: eye correctness depends on the look-at object, source eye
  mesh rows, shared head transform, and pivot transform. Native code must not
  treat the eyes as standalone static attachments.

Accepted isolated hair / look-at vptr controller samplers:

- Tool: `tools/trace_pcsx2_controller_targets.py`.
- Reports:
  - `pcsx2_controller_targets_hair_iso_state1_20260611.json`.
  - `pcsx2_controller_targets_lookat_l_iso_state1_20260611.json`.
  - `pcsx2_controller_targets_lookat_r_iso_state1_20260611.json`.
- Screenshots:
  - `pcsx2_controller_targets_hair_iso_state1_20260611.before.window.png`
    and `pcsx2_controller_targets_hair_iso_state1_20260611.window.png`.
  - `pcsx2_controller_targets_lookat_l_iso_state1_20260611.before.window.png`
    and `pcsx2_controller_targets_lookat_l_iso_state1_20260611.window.png`.
  - `pcsx2_controller_targets_lookat_r_iso_state1_20260611.before.window.png`
    and `pcsx2_controller_targets_lookat_r_iso_state1_20260611.window.png`.
- Screenshot gate: all six screenshots are accepted active in-song Battle of
  the Bands gameplay at normal speed (`FPS 60`, `VPS 60`, speed `100%`).
  None were Retry/fail/startup/wrong-window captures. `EnableEE = true` was
  verified afterward and no PCSX2 process was left running.
- Commands used the accepted ISO/state route and isolated one vptr at a time:
  - Hair:
    `--no-default-vptrs --vptr exact_hair_b=0x00DBF5AC:0x003E77E8`.
  - Left look-at:
    `--no-default-vptrs --vptr exact_l_eye_lookat_c=0x00DBE490:0x003E7C28`.
  - Right look-at:
    `--no-default-vptrs --vptr exact_r_eye_lookat_c=0x00DBF960:0x003E7C28`.
- Tick samples and first retained args:
  - Hair: `160` ticks of `0x00176fb8`;
    `a0=0x00dbf5a0`, `a1=0x01c80080`, `a2=0x00dbf5a8`,
    `a3=0x40c90fdb`.
  - Left look-at: `160` ticks of `0x0017d658`;
    `a0=0x00dbe470`, `a1=0x01c80080`, `a2=0x00dbe48c`, `a3=0`.
  - Right look-at: `160` ticks of `0x0017d658`;
    `a0=0x00dbf940`, `a1=0x01c80080`, `a2=0x00dbf95c`,
    `a3=0x007c4114`.
- Row movement:
  - Hair controller header `0x00dbf5a0` stayed stable in the isolated hair
    run, but linked child/root rows moved: `0x00db81f0` changed `9` rows,
    `0x00db9ef0` changed `18`, `0x00dbaef0` changed `17`, and
    `0x00dbc7f0` changed `9`.
  - In the left look-at run, `0x00dbe470` and target row `0x00dbe48c`
    changed `3` rows each, source eye mesh row `0x00766880` changed `17`
    rows, right-eye source row `0x00779070` also changed `17` rows, and
    shared pivot `0x00dbf740` changed `3` rows.
  - In the right look-at run, `0x00dbf940` and target row `0x00dbf95c`
    changed `3` rows each, source eye mesh row `0x00779070` changed `17`
    rows, left-eye source row `0x00766880` also changed `17` rows, and
    shared pivot `0x00dbf740` changed `3` rows.
- Interpretation: isolated vptr evidence confirms hair and eye correctness is
  driven by controller update calls plus linked child/source Trans rows, not
  by one-time attachment of the visible mesh. The different look-at `a3`
  values are trace-backed and must not be collapsed into a generic eye rule.

Accepted static hair / look-at helper dump plus gate check:

- Tool: `tools/dump_function_snippets.py`.
- Report: `ps2_function_snippets_hair_lookat_isolated_20260611.json`.
- Command:
  `python tools\dump_function_snippets.py --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\ps2_function_snippets_hair_lookat_isolated_20260611.json" --insns 220 --func 0x00176fb8 --func 0x00176ab0 --func 0x0017d658 --func 0x002ffa60 --func 0x002dad00 --func 0x002d5fd8 --func 0x001dd748 --func 0x003d8ea0`.
- Static hair facts:
  - `0x00176fb8` stores `this` in `s4`, uses `s0=this+0x2c`, reads
    `this+0x2c`, `this+0x30`, `this+0x40`, and `this+0x44`.
  - The reset branch is gated by `this+0x40`: `lw v0,64(s4)`;
    if nonzero, it calls `0x00176ab0` and then clears `this+0x40`.
  - `0x00176ab0` starts from `this+0x2c` / `this+0x30`, walks list-like child
    rows, calls `0x003d8ea0`, and calls vector helper `0x002daf00` on child
    data.
- Runtime gate check from
  `pcsx2_controller_targets_hair_iso_state1_20260611.json`:
  `0x00dbf5e0` (`hair + 0x40`) stayed `0` for all `160` retained hair ticks,
  while `0x00dbf5e4` (`hair + 0x44`) stayed `1`.
- Long active-window follow-up:
  - `pcsx2_hair_reset_setup_long_sequence_20260611.json` ran headless for
    45 seconds. The total counter reached `102903`, so the 65,536-record ring
    wrapped; retained counts are steady-window evidence, not full-window
    totals.
  - Retained hot counts: `hair_update_00176fb8` 23, `lookat_0017d658` 46,
    `foretwist_00175678` 69, `uppertwist_001823c8` 184,
    `trans_dirty_001dd748` 22,166, and `trans_world_003d8ea0` 43,048.
  - Reset/setup targets were zero-hit: `hair_setup_00176aa0` 0,
    `hair_reset_00176ab0` 0, and `shared_setup_001d2c48` 0.
- Direct hair gate object follow-up:
  - `pcsx2_hair_reset_gate_rows_20260611.json` sampled `0x00dbf5a0` for
    24 seconds at 0.20s intervals.
  - `hair_obj_00dbf5a0`, `hair_gate_00dbf5e0`, and
    `hair_child_area_00dbf5cc` each changed 0 rows across 120 samples.
  - `0x00dbf5e0` stayed `0`, `0x00dbf5e4` stayed `1`, and owner/source
    `0x00b8be10` moved 14 rows while naming `char/glam1/og/glam1.milo`.
- Static look-at facts:
  - `0x0017d658` stores `this` in `s4`, requires `this+0x48`, a target block
    at `this+0x28`, and a second target/source block at `this+0x34`.
  - The live left object maps those to `0x00dbe4b8`, `0x00dbe498`, and
    `0x00dbe4a4`; the live right object maps them to `0x00dbf988`,
    `0x00dbf968`, and `0x00dbf974`.
  - It calls `0x003d8ea0`, `0x002ffa60`, `0x002dad00`, `0x002daa30`,
    `0x001dd7b8`, and `0x001dd748` on the linked eye/source rows.
- Interpretation: the hair reset miss is not evidence of a missing hook in the
  active-song slice; the reset gate is trace-checked as closed. Look-at field
  mapping now has static offsets tied to the isolated runtime objects, but the
  exact semantic names for the float rows still need annotation.

Accepted look-at helper field sequence:

- Report: `pcsx2_lookat_helper_field_sequence_20260611.json`.
- Log: `pcsx2_lookat_helper_field_sequence_20260611.log`.
- Command class: headless `trace_pcsx2_call_sequence.py` from state `1` for
  12 seconds, 65,536-record ring, EE recompiler disabled, no screenshot gate.
- Counts: retained all `21,677` calls:
  - `lookat_0017d658`: 16
  - `hair_update_00176fb8`: 8
  - `ik_update_0017a080`: 16
  - `lookat_setup_0017d640`: 0
  - `helper_002ffa60`: 8
  - `vec_helper_002dad00`: 14
  - `vec_norm_002daa30`: 1,476
  - `helper_002d5fd8`: 14
  - `trans_dirty_001dd748`: 6,856
  - `trans_dirty_alt_001dd7b8`: 175
  - `trans_world_003d8ea0`: 13,094
- Runtime split:
  - `0x00dbe470` fired 8 look-at updates.
  - `0x00dbf940` fired 8 look-at updates.
  - `0x002d5fd8` used `0x00dbe500` and `0x00dbf9d0` 7 times each.
- Local left order:
  `0x0017d658(0x00dbe470, ..., 0x00dbe48c, 0)` ->
  `0x003d8ea0(0x00dbf740)` -> `0x003d8ea0(0x00766880)` ->
  `0x003d8ea0(0x00db9ef0)` -> `0x002d5fd8(0x00dbe500, ...)` ->
  `0x001dd748(0x00766880, 0x00dbe4e0, 0x00db9f70, 0)`.
- Local right order:
  `0x0017d658(0x00dbf940, ..., 0x00dbf95c, 0x007c4114)` ->
  `0x003d8ea0(0x00dbf740)` -> `0x003d8ea0(0x00779070)` ->
  `0x003d8ea0(0x00db9ef0)` -> `0x002d5fd8(0x00dbf9d0, ...)` ->
  `0x001dd748(0x00779070, 0x00dbf9b0, 0x00db9f70, 0)`.

Accepted long performer-placement isolation trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_performer_placement_long_sequence_20260611.json`.
- Screenshot: `pcsx2_performer_placement_long_sequence_20260611.window.png`.
- Screenshot gate: accepted active in-song gameplay with a close band/camera
  view. The trace was slow because of interpreter instrumentation, but it was
  not a startup/fail/retry capture. `EnableEE = true` was verified afterward
  and no PCSX2 process was left running.
- Counts:
  - `place_main_0010cfa0`: `0`.
  - `place_child_a_00190770`: `0`.
  - `place_child_b_00162b30`: `0`.
  - `event_check_0010c948`: `0`.
  - `event_timing_00171c68`: `12`.
  - `trans_world_003d8ea0`: `10491`.
  - `trans_dirty_001dd748`: `5872`.
  - `place_related_0010c730`: `0`.
  - `place_aux_0010d148`: `0`.
  - `maybe_target_001264f8`: `9`.
- Order facts:
  - `0x00171c68` fired on glam/guitar driver `0x00daf090` with flag-like
    args `0x00400000` and `0x00800000`, then dirty propagation over character
    rows followed.
  - `0x001264f8` fired with `a0=0x00abeefc`; nearby records immediately
    resolved and dirtied camera/target-like rows including `0x00b92ef0` and
    `0x00b8bcf0`.
- Interpretation: this longer accepted window still did not exercise
  `0x0010cfa0` or its children `0x00190770` / `0x00162b30`. Keep the
  placement branch marked phase-gated. Do not infer native placement behavior
  from this save slice.

Accepted focused blend/weight helper trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_blend_math_focus_sequence_20260611.json`.
- Screenshot: `pcsx2_blend_math_focus_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 30 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_blend_math_focus_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_blend_math_focus_sequence_20260611.json" --target blend_entry_init_00198660=0x00198660 --target blend_tick_00199000=0x00199000 --target blend_source_00195f18=0x00195f18 --target blend_related_00196888=0x00196888 --target blend_related_b_00196818=0x00196818 --target blend_related_b_child_001966f0=0x001966f0 --target clip_candidate_00195b80=0x00195b80 --target clip_candidate_child_00196610=0x00196610 --target blend_reset_00198ac8=0x00198ac8 --target blend_release_00198a48=0x00198a48 --target clip_lookup_0016c1b0=0x0016c1b0`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay with
  visible band/venue. PCSX2 was slow because interpreter tracing was active
  (`FPS/VPS 26`, speed `44%`), but the capture is not Retry/fail/startup or
  a wrong window. `EnableEE = true` was verified afterward and no PCSX2
  process was left running.
- Counts:
  - `blend_entry_init_00198660`: `37`.
  - `blend_tick_00199000`: `42`.
  - `blend_source_00195f18`: `79`.
  - `blend_related_00196888`: `33`.
  - `blend_related_b_00196818`: `28`.
  - `blend_related_b_child_001966f0`: `28`.
  - `clip_candidate_00195b80`: `53`.
  - `clip_candidate_child_00196610`: `30`.
  - `blend_reset_00198ac8`: `5`.
  - `blend_release_00198a48`: `37`.
  - `clip_lookup_0016c1b0`: `5`.
- Order facts:
  - A repeated entry-update chain is:
    `0x00198a48(entry, 3, ...) -> 0x00195f18(source_child, source_list, performer_child, previous) -> 0x00198660(entry, performer_source, scheduler, flags) -> 0x00199000(entry) -> 0x00195f18(...) -> 0x00196888(...) -> 0x00196818(...) -> 0x001966f0(...)`.
  - The first retained chain uses:
    `0x00198660(a0=0x00768bd0, a1=0x00b8da50, a2=0x01021340, a3=0x234)`,
    then `0x00199000(a0=0x00768bd0)`, then related branch
    `0x00196888/0x00196818(a0=0x01021340, a1=0x01021340, a2=0x234, a3=1)`,
    and `0x001966f0(a0=0x01021340, a1=0x01021340)`.
  - Drum-style candidate lookup appears as
    `0x0016c1b0(0x01359cc0, 0x005f9ee0, 1, 0x0076bd10)` followed by repeated
    `0x00195b80(candidate, 0x01ffd770, 0x005f7b74, 0x0076bd10)`, then
    `0x00198660(0x0076bd10, 0x00b902e0, 0x013bb500, 0x30)`.
  - Hand/guitar-style candidate rows still reach `0x00196610` directly, for
    example `0x00196610(a0=0x00f1f9d0, a1=0, a2=0x005520e9, a3=0)`.
  - Glam/guitar entry example:
    `0x00198660(0x00770dd0, 0x00b8be10, 0x00ebd6a0, 0x204)`,
    `0x00199000(0x00770dd0)`,
    `0x00196888/0x00196818(0x00ebd2f0, 0x00ebd6a0, 0x204, 0x00ebd6a0)`,
    then `0x001966f0(0x00ebd2f0, 0x00ebd6a0)`.
- Static helper dump:
  - Tool: `tools/dump_function_snippets.py`.
  - Report: `ps2_function_snippets_blend_math_focus_20260611.json`.
  - Command:
    `python tools\dump_function_snippets.py --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\ps2_function_snippets_blend_math_focus_20260611.json" --insns 260 --func 0x00199000 --func 0x00196610 --func 0x001966f0 --func 0x00195f18 --func 0x00196818 --func 0x00196888 --func 0x00198660`.
  - `0x00198660` initializes blend entry fields: `entry+0x00` flags/mode from
    scheduler `+0x28`, `entry+0x04` from scheduler `+0x30`, `entry+0x08`
    starts at `1.0`, `entry+0x24` stores scheduler/source, `entry+0x28`
    stores an input flag/mask, `entry+0x2c` stores performer source,
    `entry+0x30` starts zero, and `entry+0x34` is initialized to `-1`.
  - `0x00199000` is the blend tick/math helper for an entry. It reads
    `entry+0x24`, then the scheduler/source list at `source+0x6c`, increments
    `entry+0x34` while stepping that list, and stores the selected child time
    float from `list_child+0x18` into `entry+0x30`.
  - `0x00195f18` uses source/list helpers to bind performer child rows into
    the current blend entry; runtime args show it is called before and after
    `0x00199000` around entry initialization.
  - `0x00196888` chooses the related blend branch. In the live slice it mainly
    reaches `0x00196818 -> 0x001966f0`; the static alternate
    `0x001967b0` branch remains zero-hit in accepted traces.
  - `0x00196888` writes a small global/result row, uses scheduler fields
    `+0x18`, `+0x28`, and `+0x30`, clamps with `0x002ffd88`, and returns the
    two-float result consumed by `0x00198660` for `entry+0x0c` and
    `entry+0x10`.
  - `0x00196610` resolves a candidate's child source through
    `candidate->+0x00 -> +0x18 -> +0x88`, then calls `0x00101ec0`.
  - `0x001966f0` builds/searches a compact table from related scheduler rows:
    it reads related object `+0x04` / `+0x08`, walks entries in `0x1c`
    strides, and returns a table used by `0x00196818` to select a float pair
    by threshold.
- Interpretation: blend entry timing/weight is now tied to concrete static
  fields and accepted runtime order. Native blend code should model
  `0x00198660 -> 0x00199000 -> 0x00195f18 -> 0x00196888` rather than treating
  blend entries as simple immediate clip switches. This trace still covers
  the current active-song slice only; alternate `0x001967b0` and other
  clip/mode branches remain unproven.

Accepted focused performer poll-order / branch trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_performer_poll_order_sequence_20260611.json`.
- Log: `pcsx2_performer_poll_order_sequence_20260611.log`.
- Screenshot: `pcsx2_performer_poll_order_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 20 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_performer_poll_order_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_performer_poll_order_sequence_20260611.json" --target chardriver_update_00171830=0x00171830 --target chardriver_selector_00171db0=0x00171db0 --target guitarist_callback_0010c988=0x0010c988 --target performer_callback_00165400=0x00165400 --target performer_callback_aux_001658d0=0x001658d0 --target performer_event_apply_001b4eb0=0x001b4eb0 --target performer_branch_001656a8=0x001656a8 --target performer_sched_00171190=0x00171190 --target sched_child_001710e0=0x001710e0 --target clip_lookup_0016c1b0=0x0016c1b0 --target blend_entry_init_00198660=0x00198660 --target blend_tick_00199000=0x00199000`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay with
  visible band, venue, note highway, HUD, and PCSX2 title/menu. PCSX2 was slow
  because interpreter tracing was active (`FPS/VPS 26`, speed `44%`), but the
  capture is not Retry/fail/startup/frozen/wrong-window. `EnableEE = true` was
  verified afterward and no PCSX2 process was left running.
- Counts:
  - `chardriver_update_00171830`: `7980`.
  - `chardriver_selector_00171db0`: `19`.
  - `guitarist_callback_0010c988`: `1`.
  - `performer_callback_00165400`: `45`.
  - `performer_callback_aux_001658d0`: `20`.
  - `performer_event_apply_001b4eb0`: `127`.
  - `performer_branch_001656a8`: `2`.
  - `performer_sched_00171190`: `2`.
  - `sched_child_001710e0`: `4`.
  - `clip_lookup_0016c1b0`: `4`.
  - `blend_entry_init_00198660`: `31`.
  - `blend_tick_00199000`: `36`.
- Object identities from prior accepted object-row traces:
  - `0x00b8be10` is the glam/guitar source.
  - `0x00b8df40` is the bass source.
  - `0x00b902e0` is the drummer source.
  - `0x00b8b800` is the singer source.
  - `0x00b78100`, `0x00b7d200`, `0x00b8a530`, and `0x00b8c3f0` are the
    previously sampled non-UI performer base rows `perf0..perf3`.
- Order facts:
  - The hot CharDriver update path dominated the ring, but filtered records
    show band-wide event application batches in stable performer-base order:
    `0x00b78100 -> 0x00b7d200 -> 0x00b8a530 -> 0x00b8c3f0` for event rows
    such as `0x0059c940`, `0x0059c9f0`, `0x00850c30`, and `0x00850c40`.
  - First retained source order for the known role rows was drummer
    `0x00b902e0`, bass `0x00b8df40`, then singer `0x00b8b800` through
    `0x00165400` / `0x001b4eb0` with event `0x008504e0`.
  - Glam/guitar source `0x00b8be10` hit direct guitarist callback
    `0x0010c988` at index `4528`, immediately followed by
    `0x00198660(0x0076bd10, 0x00b8be10, 0x00e0d150, 1)` and
    `0x00199000(0x0076bd10)`.
  - Bass source `0x00b8df40` took the branch-created blend route:
    `0x00165400 -> 0x001b4eb0 -> 0x00165400 -> 0x001b4eb0 -> 0x001658d0 -> 0x00165400 -> 0x001656a8 -> 0x00171190(0x0113f2e0, 0x005f7bb0, 0x30, 1) -> 0x001710e0 -> 0x0016c1b0(0x01158730, 0x005f7bb0, 1, 0x0076ba10) -> 0x00198660(0x0076ba10, 0x00b8df40, 0x0115fb40, 0x30) -> 0x00199000(0x0076ba10)`.
  - Drummer source `0x00b902e0` immediately followed the bass branch with the
    analogous route:
    `0x00165400 -> 0x001b4eb0 -> 0x00165400 -> 0x001b4eb0 -> 0x001658d0 -> 0x00165400 -> 0x001656a8 -> 0x00171190(0x0135cb90, 0x005f9ee0, 0x30, 1) -> 0x001710e0 -> 0x0016c1b0(0x01359cc0, 0x005f9ee0, 1, 0x0076bad0) -> 0x00198660(0x0076bad0, 0x00b902e0, 0x013bc010, 0x30) -> 0x00199000(0x0076bad0)`.
  - Singer source `0x00b8b800` appeared in the hot callback/event path and in
    later blend entries, for example
    `0x00198660(0x00768c10, 0x00b8b800, 0x00d22bd0, 0x30)`,
    `0x00198660(0x00768c90, 0x00b8b800, 0x00d22bd0, 0x1034)`, and
    `0x00198660(0x0076bd90, 0x00b8b800, 0x00d24f20, 0x34)`. The singer did
    not hit `0x001656a8` in this accepted window.
  - All `31` observed `0x00198660` blend-entry init records were immediately
    followed by `0x00199000` on the same entry.
- Interpretation: the current active-song slice now has trace-backed
  performer poll/order evidence for the hot event path, the glam/guitar direct
  callback route, and the bass/drum branch-created blend route. Do not
  generalize the bass/drum branch to every singer/guitar event mode yet; the
  trace proves this slice's order, not every possible authored clip/event
  phase.

Accepted longer no-hot performer/blend branch trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_performer_blend_nohot_long_sequence_20260611.json`.
- Log: `pcsx2_performer_blend_nohot_long_sequence_20260611.log`.
- Screenshot: `pcsx2_performer_blend_nohot_long_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 60 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_performer_blend_nohot_long_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_performer_blend_nohot_long_sequence_20260611.json" --target chardriver_selector_00171db0=0x00171db0 --target guitarist_callback_0010c988=0x0010c988 --target performer_callback_00165400=0x00165400 --target performer_callback_aux_001658d0=0x001658d0 --target performer_event_apply_001b4eb0=0x001b4eb0 --target performer_branch_001656a8=0x001656a8 --target performer_sched_00171190=0x00171190 --target sched_child_001710e0=0x001710e0 --target clip_lookup_0016c1b0=0x0016c1b0 --target blend_entry_init_00198660=0x00198660 --target blend_tick_00199000=0x00199000 --target blend_source_00195f18=0x00195f18 --target blend_related_00196888=0x00196888 --target blend_related_b_00196818=0x00196818 --target blend_related_b_child_001966f0=0x001966f0 --target clip_candidate_00195b80=0x00195b80 --target clip_candidate_child_00196610=0x00196610 --target blend_alt_child_001967b0=0x001967b0 --target blend_alt_child_00169aa0=0x00169aa0`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay with a
  different live band/camera angle, note highway, HUD, and PCSX2 title/menu.
  PCSX2 was slow because interpreter tracing was active (`FPS/VPS 31`, speed
  `52%`), but the capture is not Retry/fail/startup/frozen/wrong-window.
  `EnableEE = true` was verified afterward and no PCSX2 process was left
  running.
- Counts:
  - `chardriver_selector_00171db0`: `66`.
  - `guitarist_callback_0010c988`: `7`.
  - `performer_callback_00165400`: `141`.
  - `performer_callback_aux_001658d0`: `86`.
  - `performer_event_apply_001b4eb0`: `539`.
  - `performer_branch_001656a8`: `7`.
  - `performer_sched_00171190`: `7`.
  - `sched_child_001710e0`: `9`.
  - `clip_lookup_0016c1b0`: `9`.
  - `blend_entry_init_00198660`: `86`.
  - `blend_tick_00199000`: `91`.
  - `blend_source_00195f18`: `177`.
  - `blend_related_00196888`: `66`.
  - `blend_related_b_00196818`: `57`.
  - `blend_related_b_child_001966f0`: `57`.
  - `clip_candidate_00195b80`: `82`.
  - `clip_candidate_child_00196610`: `30`.
  - `blend_alt_child_001967b0`: `0`.
  - `blend_alt_child_00169aa0`: `0`.
- Order facts:
  - Branch source counts were drummer `0x00b902e0` twice, bass `0x00b8df40`
    four times, and singer `0x00b8b800` once.
  - The singer branch at index `1323` followed:
    `0x00165400 -> 0x001b4eb0 -> 0x00165400 -> 0x001b4eb0 -> 0x001658d0 -> 0x00165400 -> 0x001656a8 -> 0x00171190(0x00c0d360, 0x005f7bb0, 0x30, 1) -> 0x001710e0 -> 0x0016c1b0(0x00d22000, 0x005f7bb0, 1, 0x00770dd0) -> 0x00195b80(0x00d236e0, ..., 0x00770dd0) -> 0x00198660(0x00770dd0, 0x00b8b800, 0x00d236e0, 0x30) -> 0x00195f18 -> 0x00199000 -> 0x00195f18 -> 0x00196888`.
  - Bass branch-created entries repeatedly used scheduler/driver
    `0x0113f2e0`, event/list row `0x005f7bb0`, and candidate sources under
    `0x0115....`. Drummer branch-created entries used scheduler/driver
    `0x0135cb90`, event/list row `0x005f9ee0`, and candidate sources under
    `0x013b....`.
  - Guitar/glam source `0x00b8be10` hit `0x0010c988` seven times in this
    window, including event rows `0x00850c40`, `0x00601b80`, and
    `0x00601be0`. It still did not hit `0x001656a8` in this sample.
  - Blend-entry init order split into two traced shapes: `71` entries were
    `0x00198660 -> 0x00199000`; `15` branch-created/source-bound entries were
    `0x00198660 -> 0x00195f18 -> 0x00199000`, followed by another
    `0x00195f18` and `0x00196888` in the branch neighborhoods.
  - The alternate static blend children `0x001967b0` and `0x00169aa0`
    remained zero-hit over this longer accepted active-song sample.
- Interpretation: the branch-created blend route is now trace-backed for
  bass, drums, and singer in the current song slice. Guitar/glam remains a
  direct-callback sample here, and the zero-hit alternate children remain
  unproven rather than dead globally. Native code must preserve both
  init/tick shapes: direct `init -> tick` and source-bound
  `init -> source-bind -> tick`.

Accepted combined lighting parent/deeper-child trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_lighting_parent_children_combined_sequence_20260611.json`.
- Log: `pcsx2_lighting_parent_children_combined_sequence_20260611.log`.
- Screenshot: `pcsx2_lighting_parent_children_combined_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 60 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_lighting_parent_children_combined_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_lighting_parent_children_combined_sequence_20260611.json" --target lighting_set_00271288=0x00271288 --target lighting_set_child_00271a08=0x00271a08 --target lighting_key_prev_002716b8=0x002716b8 --target lighting_key_next_00271778=0x00271778 --target lighting_key_first_00271200=0x00271200 --target lighting_apply_prev_00280f60=0x00280f60 --target lighting_apply_alt_00280fe8=0x00280fe8 --target lighting_apply_next_00281070=0x00281070 --target child_a_003ac4b8=0x003ac4b8 --target child_b_002cf1d0=0x002cf1d0 --target child_c_00305624=0x00305624 --target child_d_003aaa30=0x003aaa30 --target child_e_003ac2b0=0x003ac2b0 --target child_f_002cf210=0x002cf210 --target child_g_003ab068=0x003ab068 --target child_h_003aaa80=0x003aaa80 --target child_i_00313ca0=0x00313ca0 --target lighting_color_child_002c6808=0x002c6808 --target lighting_writer_003b50e0=0x003b50e0`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay with
  visible venue lighting, band, note highway, HUD, and PCSX2 title/menu. The
  interpreter warning overlay is expected from `--disable-ee-recompiler`.
  PCSX2 was slow because interpreter tracing was active (`FPS/VPS 27`, speed
  `46%`), but the capture is not Retry/fail/startup/frozen/wrong-window.
  `EnableEE = true` was verified afterward and no PCSX2 process was left
  running.
- Counts:
  - `lighting_set_00271288`: `0`.
  - `lighting_set_child_00271a08`: `0`.
  - `lighting_key_prev_002716b8`: `0`.
  - `lighting_key_next_00271778`: `0`.
  - `lighting_key_first_00271200`: `0`.
  - `lighting_apply_prev_00280f60`: `0`.
  - `lighting_apply_alt_00280fe8`: `0`.
  - `lighting_apply_next_00281070`: `0`.
  - `child_a_003ac4b8`: `0`.
  - `child_b_002cf1d0`: `239`.
  - `child_c_00305624`: `939`.
  - `child_d_003aaa30`: `0`.
  - `child_e_003ac2b0`: `0`.
  - `child_f_002cf210`: `238`.
  - `child_g_003ab068`: `0`.
  - `child_h_003aaa80`: `0`.
  - `child_i_00313ca0`: `0`.
  - `lighting_color_child_002c6808`: `0`.
  - `lighting_writer_003b50e0`: `0`.
- Order/object facts:
  - The nonzero helpers repeat in clusters of `0x002cf1d0`, many
    `0x00305624` copies/math calls, then `0x002cf210`.
  - First retained calls:
    `0x002cf1d0(0x18, 0x00456a18, stack, 0)`,
    `0x00305624(stack, stack, 0x20, 0x00010006)`, and
    `0x002cf210(8, 0x00853b70, 0x00853b70, 0)`.
  - Later rows include hot global/list-style args such as `0x00c9d060`,
    `0x00851190`, `0x008511b0`, and `0x0084fcd0`, but no same-window
    `0x00271a08` parent call.
- Interpretation: this is another accepted active-song phase where
  `0x002cf1d0`, `0x00305624`, and `0x002cf210` are hot helpers, but it still
  does not prove they are reached from lighting set-child `0x00271a08` in the
  same window. Keep the final render-light writer path open and treat these
  helpers as active generic/list/math helpers until a parent-hit window ties
  them to `0x00271a08`.

Accepted focused lighting parent/hot-helper retry:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_lighting_parent_hot_helpers_focused_sequence_20260611.json`.
- Log: `pcsx2_lighting_parent_hot_helpers_focused_sequence_20260611.log`.
- Screenshot: `pcsx2_lighting_parent_hot_helpers_focused_sequence_20260611.window.png`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay with
  visible band, note highway, HUD, venue lighting, and PCSX2 title/menu. The
  interpreter warning overlay is expected from `--disable-ee-recompiler`.
  PCSX2 was slow because interpreter tracing was active (`FPS/VPS 28`, speed
  `46%`), but the capture is not Retry/fail/startup/frozen/wrong-window.
  `EnableEE = true` was verified afterward and no PCSX2 process was left
  running.
- Counts:
  - `lighting_set_00271288`: `0`.
  - `lighting_set_child_00271a08`: `0`.
  - `lighting_key_prev_002716b8`: `0`.
  - `lighting_apply_prev_00280f60`: `0`.
  - `child_b_002cf1d0`: `11`.
  - `child_c_00305624`: `85`.
  - `child_f_002cf210`: `10`.
  - `lighting_color_child_002c6808`: `0`.
  - `lighting_writer_003b50e0`: `0`.
- Interpretation: a narrower parent-hit-style retry still saw only the hot
  helper cluster and no lighting parent/keyframe calls. This reinforces the
  current rule: the hot helper cluster is not evidence of the lighting
  parent-chain by itself. The next lighting proof needs a capture phase that
  explicitly hits `0x00271288` / `0x00271a08` again while tracing a small
  downstream set.

Accepted focused world/venue live object sampler:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_world_venue_live_objects_focus_20260611.json`.
- Log: `pcsx2_world_venue_live_objects_focus_20260611.log`.
- Screenshots:
  `pcsx2_world_venue_live_objects_focus_20260611.before_sample.window.png`
  and `pcsx2_world_venue_live_objects_focus_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 12 --interval 0.20 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_venue_live_objects_focus_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_venue_live_objects_focus_20260611.json" --target world_event_owner_00ad2aa0=0x00ad2aa0:0x280 --target crowd_event_norm_00b94bc0=0x00b94bc0:0x200 --target crowd_stream_00c9d060=0x00c9d060:0x220 --target world_msg_node_0060ba20=0x0060ba20:0x180 --target hot_list_a_00848d80=0x00848d80:0x240 --target hot_list_b_0084ae80=0x0084ae80:0x240 --target venue_row_0073d3d0=0x0073d3d0:0x200 --target script_node_006025e0=0x006025e0:0x160 --target script_node_00600da0=0x00600da0:0x160 --target script_node_006011b0=0x006011b0:0x160 --target script_node_00601570=0x00601570:0x160 --target script_node_00601780=0x00601780:0x160`.
- Screenshot gate: both screenshots accepted active in-song Battle of the
  Bands gameplay at normal speed (`FPS 60`, `VPS 60`, speed `100%`), with
  visible band, note highway, HUD, venue, and a camera/lighting change between
  captures. They are not Retry/fail/startup/frozen/wrong-window captures.
  `EnableEE = true` was verified afterward and no PCSX2 process was left
  running.
- Changed-row counts:
  - `world_event_owner_00ad2aa0`: `5`.
  - `crowd_event_norm_00b94bc0`: `4`.
  - `crowd_stream_00c9d060`: `36`.
  - `world_msg_node_0060ba20`: `0`.
  - `hot_list_a_00848d80`: `7`.
  - `hot_list_b_0084ae80`: `22`.
  - `venue_row_0073d3d0`: `0`.
  - `script_node_006025e0`: `0`.
  - `script_node_00600da0`: `0`.
  - `script_node_006011b0`: `0`.
  - `script_node_00601570`: `0`.
  - `script_node_00601780`: `0`.
- Object/layout facts:
  - `0x00ad2aa0` is live world/crowd event owner state. It names
    `crowd_audio` and `world/battle/streams`, changes event cursor-like rows
    at `+0x20`, `+0x40`, `+0x48`, and current event pointer `+0xa0`, which
    switched from `0x00b94ae0` to `0x00b94bc0`.
  - `0x00b94bc0` is the live `crowd_upto_norm` event object. It changed
    rows at `+0x28`, `+0x60`, `+0x64`, and `+0x100`, carries `_default`,
    `blewbignote_v1_1.cue`, `blewbignote_v1_3.wav`, and
    `world/battle/samples/blewbignote_v1_3.wav`.
  - `0x00c9d060` is the live crowd stream state. It changed 36 rows, including
    counters/timers, stream pointer rows, and the known stream-switch region
    around `+0x10c`. Readable rows include
    `world/battle/streams/crowd_v1_3norm`,
    `world/battle/streams/crowd_v1_3norm.vgs`, and
    `sfx/samples/sp_gemhit_elec4.wav`.
  - Hot list row `0x00848d80` changed 7 rows and rotated through list/payload
    pointers. Readable atoms in/through the row include `band_jump`,
    `crowd_lighters_slow`, `crowd_update`, `downbeat`, `excitement`,
    `game_lost`, `game_outro_complete`, `game_over`, and `game_won_msg`.
  - Hot list row `0x0084ae80` changed 22 rows and rotated through broader
    list/state pointers. Readable atoms in/through the row include
    `downbeat`, `beat`, `crowd_hide`, `game_won_msg`, `hit_snare`, and
    `shot_over`.
  - The sampled script/message rows stayed stable, confirming they are
    authored graph data in this sample: `0x0060ba20` names `crowd_v1_4good`,
    `loop_ms`, `encore_intro`, `sound`, `world/small2/small2.dtb`, and
    `world/battle/battle.dtb`; `0x006025e0` names
    `world/world_objects_worldbase.dtb`, `check_camera_shot`,
    `camera_bars_left`, and `get_shot_duration`; `0x00601570` names
    `world` / `current_shot`; `0x00601780` names camera-facing atoms such as
    `right`, `far`, `null`, `near`, and `closeup`.
- Interpretation: venue/world runtime state is concentrated in the live owner,
  crowd event, crowd stream, and hot list rows; the nearby script nodes are
  stable authored graph records. Native venue animation/crowd/lighting should
  preserve a stateful list/cursor/event model rather than treating the DTA
  script rows as directly mutable state.

Accepted focused world/venue live pointer follow-up:

- Tool: `tools/sample_pcsx2_pointer_targets.py`.
- Report: `pcsx2_world_venue_live_pointer_followup_20260611.json`.
- Log: `pcsx2_world_venue_live_pointer_followup_20260611.log`.
- Screenshots:
  `pcsx2_world_venue_live_pointer_followup_20260611.before_pointer.window.png`
  and `pcsx2_world_venue_live_pointer_followup_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_pointer_targets.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 10 --interval 0.20 --object-size 0x120 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_venue_live_pointer_followup_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_world_venue_live_pointer_followup_20260611.json" --cell owner_event_cursor_00ad2ac0=0x00ad2ac0 --cell owner_current_crowd_00ad2b40=0x00ad2b40 --cell crowd_norm_ptr_00b94be8=0x00b94be8 --cell crowd_norm_payload_00b94cc0=0x00b94cc0 --cell crowd_stream_switch_00c9d16c=0x00c9d16c --cell crowd_stream_vgs_00c9d1e8=0x00c9d1e8 --cell hot_list_a_head_00848d80=0x00848d80 --cell hot_list_a_next_00848d84=0x00848d84 --cell hot_list_a_payload_00848d88=0x00848d88 --cell hot_list_b_head_0084ae80=0x0084ae80 --cell hot_list_b_next_0084ae84=0x0084ae84 --cell hot_list_b_row_0084aeb0=0x0084aeb0 --cell hot_list_b_row2_0084aec0=0x0084aec0 --cell hot_list_b_row3_0084aed0=0x0084aed0`.
- Screenshot gate: both screenshots accepted active in-song Battle of the
  Bands gameplay at normal speed (`FPS 60`, `VPS 60`, speed `100%`), with
  visible band, note highway, HUD, venue, and clear lighting/camera change
  between captures. They are not Retry/fail/startup/frozen/wrong-window
  captures. `EnableEE = true` was verified afterward and no PCSX2 process was
  left running.
- Pointer facts:
  - `owner_event_cursor_00ad2ac0` rotated between `0x00747050` and
    `0x00850ec0`; pointed rows include `battle_lighting_RndDir`,
    `lighting_change`, and `elephantbones` atoms.
  - `owner_current_crowd_00ad2b40` rotated between `0x00b94ae0`
    (`crowd_begin`) and `0x00b94bc0` (`crowd_upto_norm`), with target rows
    also naming `_default` and `blewbignote_v1_1.cue`.
  - `crowd_norm_ptr_00b94be8` rotated between `0x00474254` and `0x00850f90`;
    pointed rows include `lighting_change`, `measure`, `miss`, `debug`,
    `fx`, and `vgs`.
  - `crowd_stream_switch_00c9d16c` rotated between `0x00845be0`,
    `0x0083fdd0`, and `0x00845b20`; the paired stream cell
    `0x00c9d1e8` rotated among those plus `0x00c9d1dc`. Pointed rows include
    `one_bar_to`, `gem_miss_callback`, and `_default`.
  - `hot_list_a_*` cells rotated through many live rows and named
    `band_jump`, `crowd_lighters_slow`, `crowd_update`, `downbeat`,
    `excitement`, `game_lost`, `game_outro_complete`, `game_over`,
    `game_won_msg`, `gem_pass_callback`, and `intro_start_msg`.
  - `hot_list_b_*` cells rotated through many live rows and named `beat`,
    `downbeat`, `crowd_hide`, `game_won_msg`, `hit_snare`, and `shot_over`.
- Interpretation: this follow-up ties the moving world/venue cells to specific
  event atoms and stream/list targets. It strengthens the rule that venue
  animation and crowd/lighting transitions are driven by mutable runtime
  cursor/list objects over authored event atoms, not by direct one-shot string
  dispatch from static script rows.

Accepted focused venue dispatch handler trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_venue_dispatch_handlers_sequence_20260611.json`.
- Log: `pcsx2_venue_dispatch_handlers_sequence_20260611.log`.
- Screenshot: `pcsx2_venue_dispatch_handlers_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 60 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_venue_dispatch_handlers_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_venue_dispatch_handlers_sequence_20260611.json" --target world_event_00123d08=0x00123d08 --target world_win_00123c28=0x00123c28 --target world_game_00124310=0x00124310 --target script_do_002b3118=0x002b3118 --target script_pick_new_002b3d50=0x002b3d50 --target script_eval_002b6238=0x002b6238 --target script_list_002b3818=0x002b3818 --target list_update_bridge_002230c8=0x002230c8 --target list_value_setter_00223400=0x00223400 --target list_value_child_0022c2c0=0x0022c2c0 --target list_update_child_002232d8=0x002232d8 --target list_update_child_0022c168=0x0022c168 --target list_walk_leaf_0022c1a0=0x0022c1a0 --target desc_ea08_ctor_0022e040=0x0022e040 --target desc_ea08_cb44_0022e1b8=0x0022e1b8 --target desc_ea08_cb5c_0022e270=0x0022e270 --target desc_ea08_cb64_0022e2e0=0x0022e2e0 --target desc_ea08_cb84_0022efa0=0x0022efa0 --target crowd_tbl_ctor_00385490=0x00385490 --target crowd_aux_c_00385920=0x00385920`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay with
  visible venue, band, note highway, HUD, and live lighting/camera state. The
  interpreter warning and `46%` speed are expected from
  `--disable-ee-recompiler`; this is not Retry/fail/startup/frozen or a wrong
  window. `EnableEE = true` was verified afterward and no PCSX2 process was
  left running.
- Counts:
  - `world_event_00123d08`: `6`.
  - `world_win_00123c28`: `3`.
  - `world_game_00124310`: `3`.
  - `script_do_002b3118`: `162`.
  - `script_pick_new_002b3d50`: `117`.
  - `script_eval_002b6238`: `60`.
  - `script_list_002b3818`: `4`.
  - `list_update_bridge_002230c8`: `13`.
  - `list_value_setter_00223400`: `17`.
  - `list_value_child_0022c2c0`: `21`.
  - `list_update_child_002232d8`: `17`.
  - `list_update_child_0022c168`: `17`.
  - `list_walk_leaf_0022c1a0`: `17`.
  - `desc_ea08_ctor_0022e040`: `20`.
  - `desc_ea08_cb44_0022e1b8`: `17`.
  - `desc_ea08_cb5c_0022e270`: `38`.
  - `desc_ea08_cb64_0022e2e0`: `38`.
  - `desc_ea08_cb84_0022efa0`: `6`.
  - `crowd_tbl_ctor_00385490`: `12`.
  - `crowd_aux_c_00385920`: `15`.
- Runtime order facts:
  - Repeated world-event dispatch shape:
    `script_* -> world_event_00123d08(0x00ad2aa0, message row, script row, 0x10) -> desc_ea08_cb84_0022efa0(0x00c9d060, 3, 0x00c9d060, 0x10)`.
  - Three message/script row pairs fired:
    `0x00550d62 / 0x0060ba20`, `0x00550d53 / 0x0060ba00`, and
    `0x00550d42 / 0x0060b9e0`.
  - Two of the `world_win_00123c28` calls immediately preceded the world-event
    dispatch with `a1` values `2`, `1`, and `0` across the accepted window.
  - `world_game_00124310` followed three world-event calls and bound owner
    `0x00ad2aa0` to crowd event objects `0x00b94bc0`, `0x00b95f10`, and
    `0x00b94df0`, always with crowd stream `0x00c9d060`.
  - The post-world-game list/value fanout reused previously sampled live rows:
    crowd/lighting list target `0x007997d0`; descriptor rows `0x00746910`,
    `0x00770e50`, `0x00768c10`, and `0x0076bc10`; and list rows
    `0x00854140`, `0x00739fd0`, and `0x007424c0`.
  - A representative neighborhood was:
    `world_event_00123d08 -> desc_ea08_cb84_0022efa0 -> world_game_00124310 -> desc_ea08_cb5c/64 rows -> list_update_bridge -> list_value_setter -> list_value_child -> list_update_child -> list_walk_leaf -> desc_ea08_cb44`.
- Interpretation: this accepts the venue/crowd dispatch skeleton from script
  scheduling into world-event owner state, crowd stream descriptor callback,
  optional world-game crowd event binding, and descriptor/list value fanout.
  It does not yet prove per-atom semantics for `band_jump`, `downbeat`,
  `crowd_lighters_*`, or the final render-facing venue animation/lighting
  consumers. Interior atom xrefs such as `0x001221ac` and `0x0010bc54` should
  not be patched as function entries without proving they are safe entry
  points.

Accepted venue dispatch row/object sampler:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_venue_dispatch_row_objects_20260611.json`.
- Log: `pcsx2_venue_dispatch_row_objects_20260611.log`.
- Screenshots:
  `pcsx2_venue_dispatch_row_objects_20260611.before_sample.window.png` and
  `pcsx2_venue_dispatch_row_objects_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 10 --interval 0.20 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_venue_dispatch_row_objects_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_venue_dispatch_row_objects_20260611.json" --target world_event_owner_00ad2aa0=0x00ad2aa0:0x280 --target script_row_0060b9e0=0x0060b9e0:0x180 --target script_row_0060ba00=0x0060ba00:0x180 --target script_row_0060ba20=0x0060ba20:0x180 --target crowd_event_00b94bc0=0x00b94bc0:0x200 --target crowd_event_00b95f10=0x00b95f10:0x200 --target crowd_event_00b94df0=0x00b94df0:0x200 --target crowd_stream_00c9d060=0x00c9d060:0x220 --target list_target_007997d0=0x007997d0:0x180 --target desc_row_00746910=0x00746910:0x180 --target desc_row_00770e50=0x00770e50:0x180 --target desc_row_00768c10=0x00768c10:0x180 --target desc_row_0076bc10=0x0076bc10:0x180 --target list_row_00854140=0x00854140:0x180 --target list_row_00739fd0=0x00739fd0:0x180 --target list_row_007424c0=0x007424c0:0x180`.
- Screenshot gate: both screenshots accepted active in-song Battle of the
  Bands gameplay at normal speed (`FPS 60`, `VPS 60`, speed `100%`), with
  visible band, venue, HUD, and clear camera/lighting movement between
  captures. They are not Retry/fail/startup/frozen/wrong-window captures.
  `EnableEE = true` was verified afterward and no PCSX2 process was left
  running.
- Changed counts:
  - `world_event_owner_00ad2aa0`: `5`.
  - `script_row_0060b9e0`: `0`.
  - `script_row_0060ba00`: `0`.
  - `script_row_0060ba20`: `0`.
  - `crowd_event_00b94bc0`: `4`.
  - `crowd_event_00b95f10`: `0`.
  - `crowd_event_00b94df0`: `6`.
  - `crowd_stream_00c9d060`: `28`.
  - `list_target_007997d0`: `8`.
  - `desc_row_00746910`: `14`.
  - `desc_row_00770e50`: `15`.
  - `desc_row_00768c10`: `35`.
  - `desc_row_0076bc10`: `50`.
  - `list_row_00854140`: `39`.
  - `list_row_00739fd0`: `0`.
  - `list_row_007424c0`: `6`.
- Row identity/layout facts:
  - `0x00ad2aa0` again names `crowd_audio` and
    `world/battle/streams`, with stream pointer `+0x44 = 0x00c9d060` and
    crowd event list entries including `0x00b94df0`, `0x00b95f10`, and
    `0x00b94bc0`. The current-event slot `+0xa0` moved
    `0x00b94ae0 -> 0x00b94bc0`.
  - The three script rows from the dispatch trace are stable authored rows:
    `0x0060b9e0` contains `crowd_v1_2poor`, `0x0060ba00` contains
    `crowd_v1_3norm`, and `0x0060ba20` contains `crowd_v1_4good`; nearby
    rows also name `loop_ms`, `encore_intro`, `encore_v1_intro`, `sound`,
    `world/small2/small2.dtb`, and `world/battle/battle.dtb`.
  - Crowd event objects are now named in the same sample:
    `0x00b94bc0` is `crowd_upto_norm`, `0x00b95f10` is
    `crowd_dnto_poor`, and `0x00b94df0` is `crowd_dnto_danger`.
  - `0x00b94bc0` carries `_default`, `blewbignote_v1_1.cue`,
    `blewbignote_v1_3.wav`, and
    `world/battle/samples/blewbignote_v1_3.wav`; it changed pointer/counter
    rows at `+0x28`, `+0x60`, `+0x64`, and `+0x100`.
  - `0x00b95f10` stayed stable in this 10-second sample but carries
    `_default`, `blewbignote_v1_5.wav`, `blewbignote_v1_1.wav`,
    `clap_v1_5.wav`, and the matching `world/battle/samples/...` paths.
  - `0x00b94df0` carries `Sequence10`, `Sequence11`, and `crowd_lose`, with
    toggling rows around `+0x108`, `+0x114`, `+0x118`, `+0x178`,
    `+0x184`, and `+0x188`.
  - `0x00c9d060` again changed the crowd stream switch region around
    `+0x10c`, rotating pointers including `0x00845be0`, `0x0083fdd0`,
    and `0x00845b20`; it names
    `world/battle/streams/crowd_v1_3norm`,
    `world/battle/streams/crowd_v1_3norm.vgs`, and
    `sfx/samples/sp_gemhit_elec4.wav`.
  - `0x007997d0` is the battle lighting/crowd list target row. It names
    `world/battle/og/battle_lighting.milo`,
    `world/battle/og/gen`, `world/battle/battle_chars.milo`, and
    `world/battle/gen`; it moved list pointers at `+0x20`, `+0x24`,
    `+0x68`, `+0x6c`, `+0x74`, and source/object pointer `+0x7c`.
  - Descriptor/list rows are highly dynamic. `0x00746910`, `0x00770e50`,
    `0x00768c10`, and `0x0076bc10` changed float/vector-like fields and
    performer/object pointers; visible names include `in_solo`, `in_peak`,
    `flame_hands`, `parser`, `player0_parser`, `HandMap_DropD2`,
    `world/battle/og/textures/bballnet.png`, and `nFader`.
  - `0x007424c0` is a live event/list row containing `intro_start_msg`, and
    it changed six pointer fields in the accepted sample.
- Interpretation: this ties the accepted dispatch function arguments to
  named authored crowd rows and live crowd/event/list objects. It still does
  not identify final venue animation or render-light consumers, and it shows
  that some sampled rows are authored/static while nearby descriptor rows are
  active mutable state.

Accepted venue world-game deep-order trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_venue_worldgame_deep_order_sequence_20260611.json`.
- Log: `pcsx2_venue_worldgame_deep_order_sequence_20260611.log`.
- Screenshot: `pcsx2_venue_worldgame_deep_order_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 60 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_venue_worldgame_deep_order_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_venue_worldgame_deep_order_sequence_20260611.json" --target world_event_00123d08=0x00123d08 --target world_win_00123c28=0x00123c28 --target world_game_00124310=0x00124310 --target world_game_phase_00124380=0x00124380 --target world_game_transition_00223e60=0x00223e60 --target world_game_apply_00223dc8=0x00223dc8 --target world_event_value_writer_0022fc88=0x0022fc88 --target world_event_state_child_00225450=0x00225450 --target list_prepare_00223fc0=0x00223fc0 --target list_prepare_child_0022b8f8=0x0022b8f8 --target list_float_walker_00223340=0x00223340 --target list_float_child_0022c1f0=0x0022c1f0 --target list_update_bridge_002230c8=0x002230c8 --target list_value_setter_00223400=0x00223400 --target list_value_child_0022c2c0=0x0022c2c0 --target list_update_child_002232d8=0x002232d8 --target list_update_child_0022c168=0x0022c168 --target list_walk_leaf_0022c1a0=0x0022c1a0 --target desc_ea08_cb5c_0022e270=0x0022e270 --target desc_ea08_cb64_0022e2e0=0x0022e2e0`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay with
  close band/venue view, note highway, HUD, and PCSX2 title/menu. The
  interpreter warning and `43%` speed are expected from
  `--disable-ee-recompiler`; this is not Retry/fail/startup/frozen or a wrong
  window. `EnableEE = true` was verified afterward and no PCSX2 process was
  left running.
- Counts:
  - `world_event_00123d08`: `6`.
  - `world_win_00123c28`: `3`.
  - `world_game_00124310`: `3`.
  - `world_game_phase_00124380`: `9`.
  - `world_game_transition_00223e60`: `3`.
  - `world_game_apply_00223dc8`: `9`.
  - `world_event_value_writer_0022fc88`: `12`.
  - `world_event_state_child_00225450`: `6`.
  - `list_prepare_00223fc0`: `13`.
  - `list_prepare_child_0022b8f8`: `45`.
  - `list_float_walker_00223340`: `13`.
  - `list_float_child_0022c1f0`: `17`.
  - `list_update_bridge_002230c8`: `13`.
  - `list_value_setter_00223400`: `17`.
  - `list_value_child_0022c2c0`: `21`.
  - `list_update_child_002232d8`: `17`.
  - `list_update_child_0022c168`: `17`.
  - `list_walk_leaf_0022c1a0`: `17`.
  - `desc_ea08_cb5c_0022e270`: `38`.
  - `desc_ea08_cb64_0022e2e0`: `38`.
- Runtime order facts:
  - Each observed world-event first writes crowd stream values:
    `world_event_00123d08 -> world_event_value_writer_0022fc88(a1=0) -> world_event_value_writer_0022fc88(a1=4) -> world_game_phase_00124380 -> world_event_state_child_00225450`.
  - Three crowd transitions are now ordered in one accepted trace:
    `0x00b94ae0 -> 0x00b94bc0` (`crowd_begin` to `crowd_upto_norm`),
    `0x00b94bc0 -> 0x00b95f10` (`crowd_upto_norm` to
    `crowd_dnto_poor`), and `0x00b95f10 -> 0x00b94df0`
    (`crowd_dnto_poor` to `crowd_dnto_danger`).
  - The `crowd_v1_3norm` event path used:
    `world_event_00123d08(0x00ad2aa0, 0x00550d62, 0x0060ba20, 0x10) -> world_game_00124310(0x00ad2aa0, 0x00b94bc0, 0x00c9d060, ...) -> world_game_transition_00223e60(0x00b94ae0, 0x00b94bc0, 0x00c9d060, ...) -> world_game_phase_00124380 -> world_game_apply_00223dc8 -> list_prepare_00223fc0 -> list_prepare_child_0022b8f8 -> descriptor/list fanout`.
  - The `crowd_v1_2poor` path used the same shape with
    `world_game_transition_00223e60(0x00b94bc0, 0x00b95f10, 0x00c9d060, ...)`,
    then descriptor rows `0x00768c10` and `0x00746910`.
  - The `crowd_v1_2poor` / danger-down transition later used
    `world_game_transition_00223e60(0x00b95f10, 0x00b94df0, 0x00c9d060, ...)`,
    then descriptor rows `0x00746910` and `0x0076bcd0`.
  - The event-script apply path is now ordered as:
    `world_game_apply_00223dc8(event, 0x006688c0, 0x0084f7b0, 0x00223150) -> list_prepare_00223fc0(event, 0x006688c0, 0x0084f7b0, 0x00223150) -> list_prepare_child_0022b8f8(event + child offset, ...) -> desc_ea08_cb5c/64 -> list_float_walker_00223340 -> list_float_child_0022c1f0 -> list_update_bridge_002230c8 -> list_value_setter_00223400 -> list_value_child_0022c2c0 -> list_update_child_002232d8 -> list_update_child_0022c168 -> list_walk_leaf_0022c1a0`.
  - World-win calls with `a1=2`, `1`, and `0` immediately preceded same-row
    world-event stream writes in this capture, but their full child semantics
    remain open.
- Interpretation: this proves the world-event-to-world-game transition order
  and the generic list/descriptor apply order for active crowd/venue event
  changes. It still does not make `band_jump`, `downbeat`, or
  `crowd_lighters_*` direct handler semantics complete; those named authored
  rows must be tied to these generic transitions through isolated branch
  captures or row-driven child traces.

Accepted named event-chain live block sampler:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_named_event_chain_live_blocks_20260611.json`.
- Log: `pcsx2_named_event_chain_live_blocks_20260611.log`.
- Screenshots:
  `pcsx2_named_event_chain_live_blocks_20260611.before_sample.window.png` and
  `pcsx2_named_event_chain_live_blocks_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 12 --interval 0.20 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_named_event_chain_live_blocks_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_named_event_chain_live_blocks_20260611.json" --target event_script_chain_0084f7b0=0x0084f7b0:0x260 --target band_jump_row_0084f860=0x0084f860:0x80 --target band_jump_payload_008461e0=0x008461e0:0x140 --target secondary_chain_0084f380=0x0084f380:0x180 --target lighters_slow_row_0084f3f0=0x0084f3f0:0x100 --target lighters_payload_a_00742b20=0x00742b20:0x180 --target lighters_payload_b_00845880=0x00845880:0x120 --target downbeat_row_0084f400=0x0084f400:0x100 --target downbeat_payload_a_00850e80=0x00850e80:0x180 --target downbeat_payload_b_0083c900=0x0083c900:0x180 --target crowd_lighter_data_00845820=0x00845820:0x180 --target adjacent_hot_00853900=0x00853900:0x180 --target adjacent_hot_00743500=0x00743500:0x180`.
- Screenshot gate: both screenshots are accepted active in-song Battle of the
  Bands gameplay at normal speed (`FPS 60`, `VPS 60`, speed `100%`) with
  visible band, venue, HUD, note highway, and camera/lighting movement. They
  are not Retry/fail/startup/frozen/wrong-window captures. No `pcsx2-qt`
  process was left running, and `EnableEE = true` / `EnableEE=enabled` was
  verified afterward.
- Changed counts:
  - `event_script_chain_0084f7b0`: `13`.
  - `band_jump_row_0084f860`: `0`.
  - `band_jump_payload_008461e0`: `42`.
  - `secondary_chain_0084f380`: `4`.
  - `lighters_slow_row_0084f3f0`: `4`.
  - `lighters_payload_a_00742b20`: `0`.
  - `lighters_payload_b_00845880`: `0`.
  - `downbeat_row_0084f400`: `4`.
  - `downbeat_payload_a_00850e80`: `24`.
  - `downbeat_payload_b_0083c900`: `6`.
  - `crowd_lighter_data_00845820`: `0`.
  - `adjacent_hot_00853900`: `20`.
  - `adjacent_hot_00743500`: `3`.
- Row identity/layout facts:
  - `0x0084f7b0` is the primary event script chain. It contains
    `active_players_changed`, `sync_head_bang`, `sync_wag`, `band_jump`,
    `game_outro_complete`, `game_over`, `peak_off_player`, `peak_on_player`,
    `peak_off`, `peak_on`, `starved`, `solo_off`, `crowd_half_tempo`,
    `solo_on`, `intro_skip`, `intro_start_msg`, `extend_track`, and
    `crowd_lighters_fast`.
  - The `band_jump` authored row at `0x0084f860` is stable in this slice:
    `+0x00=0x0084f880`, `+0x04=0x0084f840`, `+0x08=0x00b8a740`,
    `+0x10=0x008461e0`, `+0x14=0x0084f380`, `+0x18=0x00b8c170`,
    `+0x1c=0x0054fa14` (`band_jump`).
  - Dynamic cells under the primary chain moved at `0x0084f900`,
    `0x0084f904`, `0x0084f908`, `0x0084f940`, `0x0084f944`,
    `0x0084f948`, `0x0084f9f0`, `0x0084f9f4`, and `0x0084f9f8`; the
    immediate `band_jump` row itself did not move.
  - `0x008461e0` is a live `band_jump` payload region. It contains
    `band_jump` at `0x008461ec` and nearby `game_over` at `0x0084621c`.
    Dynamic rows include `0x00846240..0x008462b8` and pointer rotations
    through live rows such as `0x00c9d...`, `0x00850ce0`, `0x00846660`,
    `0x00848860`, `0x00821130`, `0x008500d0`, and `0x008541a0`.
  - `0x0084f380` contains `band_jump`, `game_over`, `game_lost`,
    `game_won_msg`, `crowd_lighters_slow`, `downbeat`, `excitement`, and
    `intro_start_msg`; the moving cells in this range were `0x0084f420`,
    `0x0084f424`, `0x0084f428`, and `0x0084f440`.
  - `0x0084f3f0` contains `crowd_lighters_slow`, `downbeat`, `excitement`,
    `intro_start_msg`, and `P0`; it moved the same overlap cells as the
    secondary chain in this sample.
  - `0x0084f400` contains `downbeat`, `excitement`, `intro_start_msg`, and
    `P0`; it also moved the same overlap cells in this sample.
  - `0x00742b20`, `0x00845880`, and `0x00845820` are stable
    structural/name-list blocks in this slice. `0x00845880` contains
    `crowd_lighters_fast`, `crowd_lighters_off`, `crowd_half_tempo`,
    `crowd_double_tempo`, and `crowd_normal_tempo`; `0x00845820` contains
    `crowd_hide`, `crowd_update`, `crowd_lighters_slow`,
    `crowd_lighters_fast`, `crowd_lighters_off`, `crowd_half_tempo`,
    `crowd_double_tempo`, `crowd_normal_tempo`, `hit_hihat`, and `start`.
  - `0x00850e80` is a moving `downbeat` payload region with `downbeat` at
    `0x00850e8c`; dynamic rows include `0x00850e80`,
    `0x00850ec0..0x00850ed8`, `0x00850f00..0x00850f98`, and
    `0x00850fe0`.
  - `0x0083c900` is another moving downbeat-related block containing
    `downbeat`, `crowd_double_tempo`, `hit_hihat`, `game_over`, and
    `crash_symbal`; dynamic rows were `0x0083c970`, `0x0083c974`,
    `0x0083c978`, `0x0083c9e0`, `0x0083c9e4`, and `0x0083c9e8`.
  - `0x00853900` is a moving adjacent hot region containing
    `crowd_lighters_off`, `game_outro_complete`, `starved`, and
    `crowd_update`; 20 cells changed in this sample. `0x00743500` contains
    `crowd_hide`, `band_jump`, `game_outro_complete`, `downbeat`, and
    `hit_snare`, with changed rows `0x00743530`, `0x00743534`, and
    `0x00743538`.
- Interpretation: the named authored event rows are now mapped to live payload
  and adjacent hot regions. This sample proves that the immediate `band_jump`
  row can remain stable while its payload and adjacent list/cursor state move.
  `downbeat` and crowd-lighter related regions likewise separate authored
  name-list blocks from mutable payload/list state. This still does not prove
  the final handler semantics or final render-facing venue/lighting consumers;
  continue with child-call tracing and pointer-target sampling from the moving
  cells rather than patching interior atom rows as function entries.

Accepted named event dynamic pointer-target sampler:

- Tool: `tools/sample_pcsx2_pointer_targets.py`.
- Report: `pcsx2_named_event_dynamic_pointer_targets_20260611.json`.
- Log: `pcsx2_named_event_dynamic_pointer_targets_20260611.log`.
- Screenshots:
  `pcsx2_named_event_dynamic_pointer_targets_20260611.before_pointer.window.png`
  and `pcsx2_named_event_dynamic_pointer_targets_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_pointer_targets.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 12 --interval 0.20 --object-size 0x100 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_named_event_dynamic_pointer_targets_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_named_event_dynamic_pointer_targets_20260611.json" --cell band_payload_00846240=0x00846240 --cell band_payload_00846244=0x00846244 --cell band_payload_00846248=0x00846248 --cell secondary_0084f420=0x0084f420 --cell secondary_0084f424=0x0084f424 --cell secondary_0084f428=0x0084f428 --cell secondary_0084f440=0x0084f440 --cell downbeat_a_00850e80=0x00850e80 --cell downbeat_a_00850ec0=0x00850ec0 --cell downbeat_a_00850ed0=0x00850ed0 --cell downbeat_a_00850f80=0x00850f80 --cell downbeat_b_0083c970=0x0083c970 --cell downbeat_b_0083c9e0=0x0083c9e0 --cell hot_lighters_00853900=0x00853900 --cell hot_lighters_00853950=0x00853950 --cell hot_band_00743530=0x00743530`.
- Screenshot gate: both screenshots are accepted active in-song Battle of the
  Bands gameplay at normal speed, with visible band, venue, HUD, note highway,
  and different camera/lighting states between before and after. They are not
  Retry/fail/startup/frozen/wrong-window captures. No `pcsx2-qt` process was
  left running, and `EnableEE = true` / `EnableEE=enabled` was verified
  afterward.
- Pointer summaries:
  - `0x00846240`, `0x00846244`, and `0x00846248` rotate through crowd stream
    style rows such as `0x00c9d590`, `0x00c9e090`, `0x00c9d650`,
    `0x00846480`, `0x00c9e04c`, and `0x0081ee10`; pointed names include
    `_parent` and `_default`.
  - `0x0084f420` rotates through at least 16 event/list rows including
    `0x00854150`, `0x00770d90`, `0x008541a0`, `0x0084ada0`,
    `0x008536c0`, `0x00743530`, `0x00850070`, `0x00848b00`,
    `0x0084f310`, and `0x0084ae30`. Pointed rows include atoms such as
    `downbeat`, `hit_snare`, `shot_over`, and `game_won_msg`.
  - `0x0084f424` rotates through rows including `0x00851530`,
    `0x00848680`, `0x00768a10`, `0x00848b00`, `0x0083e230`,
    `0x008541a0`, `0x00743530`, `0x00848c80`, and `0x0084ae30`.
    Pointed rows include `game_lost`, `downbeat`, `hit_snare`,
    `shot_over`, `game_won_msg`, `band_jump`, `game_over`, and
    `crowd_lighters_slow`.
  - `0x0084f440` alternates between `0x0074bd20` and `0x0161e3b8`, with
    pointed rows including `game_won_msg`, `sync_head_bang`, `game_lost`,
    `PART GUITAR`, and `player0_parser`.
  - `0x00850e80` alternates between `0x00743530` and `0x00845040`; pointed
    rows include `downbeat`, `hit_snare`, `shot_over`, and `game_won_msg`.
  - `0x00850ec0`, `0x00850ed0`, and `0x00850f80` rotate through named rows
    including `game_lost`, `beat`, `downbeat`, `game_won_msg`,
    `crowd_lighters_off`, `crowd_hide`, `intro_start_msg`, `excitement`,
    and the lighting/content row carrying
    `world/battle/og/battle_lighting.milo`, `world/battle/og/gen`, and
    `world/battle/battle_chars.milo`.
  - `0x0083c970` alternates between `0x00854230` and `0x00853c50`;
    `0x0083c9e0` alternates between `0x00854290` and `0x00c9b610` and
    points through `crowd_hide`.
  - `0x00853900` alternates between `0x00853c50` and `0x00821110`;
    `0x00853950` alternates between `0x0083c9e0` and `0x00c9b604`, with
    `crash_symbal` visible in the pointed rows.
  - `0x00743530` rotates through event rows including `0x0072dba0`,
    `0x008536c0`, `0x008466e0`, `0x00854080`, `0x0084ae80`,
    `0x00853b80`, `0x0084f260`, `0x00848b00`, `0x00850d50`,
    `0x00850ec0`, `0x0084f420`, and `0x00848d50`. Pointed rows include
    `downbeat`, `game_won_msg`, `game_over`, `start_shot`, `game_lost`,
    `peak_off_player`, and `msg_last_frame`.
- Interpretation: the moving `band_jump` / `downbeat` / crowd-lighter cells
  are now tied to concrete event rows, crowd stream defaults, camera-shot
  atoms, and the battle lighting content row. This strengthens the model that
  the venue/band/camera/lighting event system is a shared mutable cursor/list
  graph over named event atoms. It still does not identify final per-atom
  handlers or final render-light/camera consumers; the next trace should
  capture child-call order around these rows and the known generic
  list/descriptor functions.

Accepted named event child-order trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_named_event_child_order_sequence_20260611.json`.
- Log: `pcsx2_named_event_child_order_sequence_20260611.log`.
- Screenshot: `pcsx2_named_event_child_order_sequence_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 60 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_named_event_child_order_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_named_event_child_order_sequence_20260611.json" --target script_do_002b3118=0x002b3118 --target script_pick_new_002b3d50=0x002b3d50 --target script_eval_002b6238=0x002b6238 --target script_list_002b3818=0x002b3818 --target world_win_00123c28=0x00123c28 --target world_event_00123d08=0x00123d08 --target world_game_00124310=0x00124310 --target world_game_phase_00124380=0x00124380 --target world_game_transition_00223e60=0x00223e60 --target world_game_apply_00223dc8=0x00223dc8 --target world_event_value_writer_0022fc88=0x0022fc88 --target world_event_state_child_00225450=0x00225450 --target list_prepare_00223fc0=0x00223fc0 --target list_prepare_child_0022b8f8=0x0022b8f8 --target list_float_walker_00223340=0x00223340 --target list_float_child_0022c1f0=0x0022c1f0 --target list_update_bridge_002230c8=0x002230c8 --target list_value_setter_00223400=0x00223400 --target list_value_child_0022c2c0=0x0022c2c0 --target list_update_child_002232d8=0x002232d8 --target list_update_child_0022c168=0x0022c168 --target list_walk_leaf_0022c1a0=0x0022c1a0 --target desc_poll_0022e0f0=0x0022e0f0 --target desc_enable_0022e1b8=0x0022e1b8 --target desc_cb5c_0022e270=0x0022e270 --target desc_cb64_0022e2e0=0x0022e2e0 --target desc_cb84_0022efa0=0x0022efa0`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay with
  visible venue, band, HUD, note highway, and live blue/red lighting. The
  interpreter warning and `46%` speed are expected from
  `--disable-ee-recompiler`; this is not Retry/fail/startup/frozen or a wrong
  window. No `pcsx2-qt` process was left running, and `EnableEE = true` /
  `EnableEE=enabled` was verified afterward.
- Counts:
  - `script_do_002b3118`: `163`.
  - `script_pick_new_002b3d50`: `118`.
  - `script_eval_002b6238`: `60`.
  - `script_list_002b3818`: `4`.
  - `world_win_00123c28`: `3`.
  - `world_event_00123d08`: `6`.
  - `world_game_00124310`: `3`.
  - `world_game_phase_00124380`: `9`.
  - `world_game_transition_00223e60`: `3`.
  - `world_game_apply_00223dc8`: `9`.
  - `world_event_value_writer_0022fc88`: `12`.
  - `world_event_state_child_00225450`: `6`.
  - `list_prepare_00223fc0`: `13`.
  - `list_prepare_child_0022b8f8`: `45`.
  - `list_float_walker_00223340`: `13`.
  - `list_float_child_0022c1f0`: `17`.
  - `list_update_bridge_002230c8`: `13`.
  - `list_value_setter_00223400`: `17`.
  - `list_value_child_0022c2c0`: `21`.
  - `list_update_child_002232d8`: `17`.
  - `list_update_child_0022c168`: `17`.
  - `list_walk_leaf_0022c1a0`: `17`.
  - `desc_poll_0022e0f0`: `2520`.
  - `desc_enable_0022e1b8`: `17`.
  - `desc_cb5c_0022e270`: `38`.
  - `desc_cb64_0022e2e0`: `38`.
  - `desc_cb84_0022efa0`: `6`.
- Runtime order facts:
  - This run reconfirms the script-to-world event skeleton with the descriptor
    poll/enable callback in the same retained ring:
    `world_event_00123d08 -> desc_cb84_0022efa0 -> value_writer(0) -> value_writer(4) -> world_game_phase_00124380 -> world_event_state_child_00225450`.
  - The `crowd_v1_3norm` path again used:
    `world_event_00123d08(0x00ad2aa0, 0x00550d53, 0x0060ba00, 0x10) -> desc_cb84_0022efa0(0x00c9d060, 3, 0x00c9d060, 0x10) -> value_writer -> world_game_00124310(0x00ad2aa0, 0x00b95f10, 0x00c9d060, 0x0021eaf8) -> world_game_transition_00223e60(0x00b94bc0, 0x00b95f10, 0x00c9d060, 0x0021eaf8) -> world_game_apply_00223dc8(0x00b95f10, 0x006688c0, 0x0084f7b0, 0x00223150)`.
  - The danger-down path again used:
    `world_event_00123d08(0x00ad2aa0, 0x00550d42, 0x0060b9e0, 0x10) -> world_game_00124310(0x00ad2aa0, 0x00b94df0, 0x00c9d060, 0x0021eaf8) -> world_game_transition_00223e60(0x00b95f10, 0x00b94df0, 0x00c9d060, 0x0021eaf8) -> world_game_apply_00223dc8(0x00b94df0, 0x006688c0, 0x0084f7b0, 0x00223150)`.
  - The list/descriptor fanout is now explicitly paired with the hot poll and
    enable callbacks:
    `list_prepare -> list_prepare_child -> desc_cb5c/64 -> list_float_walker -> list_float_child -> list_update_bridge -> list_value_setter -> list_value_child -> list_update_child -> list_update_child_0022c168 -> list_walk_leaf -> desc_poll_0022e0f0 -> desc_enable_0022e1b8`.
  - `world_win_00123c28` with `a1=1` and `a1=0` appears immediately before
    same-row `world_event_00123d08` stream writes in retained neighborhoods,
    but those specific retained neighborhoods did not always continue into a
    `world_game_00124310` call before the ring moved on. Treat `world_win` as
    a proven pre-event helper, not yet as a fully mapped transition owner.
  - Several script-triggered `list_prepare_child_0022b8f8` bursts touched
    child rows such as `0x00b95bb8`, `0x00b95668`, `0x00b956d8`,
    `0x00b957c8`, `0x00b94ef8`, `0x00b94f68`, `0x00b962b8`,
    `0x00b964f8`, and `0x00b94a28`, with event/list payload args including
    `0x00850e40`, `0x0084f260`, `0x00853ab0`, `0x00848aa0`,
    `0x00853d40`, `0x00848800`, `0x0084aef0`, `0x008536c0`,
    `0x00850070`, and `0x00853780`.
- Interpretation: this trace does not introduce a new final per-atom handler,
  but it does lock the descriptor poll/enable callbacks into the same accepted
  runtime order as the named event/list transitions. Continue by sampling the
  child rows and payload args from the script-triggered bursts, then isolate
  final camera/light/venue consumers from rows such as `0x00850e40`,
  `0x0084f260`, `0x00850070`, `0x00853780`, and the known battle lighting
  row.

Accepted named event child-row/object sampler:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_named_event_child_rows_objects_20260611.json`.
- Log: `pcsx2_named_event_child_rows_objects_20260611.log`.
- Screenshots:
  `pcsx2_named_event_child_rows_objects_20260611.before_sample.window.png`
  and `pcsx2_named_event_child_rows_objects_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 12 --interval 0.20 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_named_event_child_rows_objects_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_named_event_child_rows_objects_20260611.json" --target child_00b95bb8=0x00b95bb8:0x120 --target child_00b95668=0x00b95668:0x120 --target child_00b956d8=0x00b956d8:0x120 --target child_00b957c8=0x00b957c8:0x120 --target child_00b94ef8=0x00b94ef8:0x120 --target child_00b94f68=0x00b94f68:0x120 --target child_00b962b8=0x00b962b8:0x120 --target child_00b964f8=0x00b964f8:0x120 --target child_00b94a28=0x00b94a28:0x120 --target payload_00850e40=0x00850e40:0x160 --target payload_0084f260=0x0084f260:0x160 --target payload_00853ab0=0x00853ab0:0x160 --target payload_00848aa0=0x00848aa0:0x160 --target payload_00853d40=0x00853d40:0x160 --target payload_00848800=0x00848800:0x160 --target payload_0084aef0=0x0084aef0:0x160 --target payload_008536c0=0x008536c0:0x160 --target payload_00850070=0x00850070:0x160 --target payload_00853780=0x00853780:0x160 --target battle_lighting_row_007997d0=0x007997d0:0x180`.
- Screenshot gate: both screenshots are accepted active in-song Battle of the
  Bands gameplay at normal speed (`FPS 60`, `VPS 60`, speed `100%`) with
  visible band, venue, HUD, note highway, and live camera/lighting changes.
  They are not Retry/fail/startup/frozen/wrong-window captures. No
  `pcsx2-qt` process was left running, and `EnableEE = true` /
  `EnableEE=enabled` was verified afterward.
- Changed counts:
  - Child rows: `0x00b95bb8` 3, `0x00b95668` 6, `0x00b956d8` 6,
    `0x00b957c8` 3, `0x00b94ef8` 6, `0x00b94f68` 3, `0x00b962b8` 4,
    `0x00b964f8` 3, and `0x00b94a28` 6.
  - Payload rows: `0x00850e40` 26, `0x0084f260` 9, `0x00853ab0` 23,
    `0x00848aa0` 12, `0x00853d40` 18, `0x00848800` 15,
    `0x0084aef0` 7, `0x008536c0` 67, `0x00850070` 16, and
    `0x00853780` 52.
  - `battle_lighting_row_007997d0`: `8`.
- Row identity/layout facts:
  - `0x00b95bb8` carries `clap_v1_4.wav`,
    `world/battle/samples/clap_v1_4.wav`, and `vroom.cue`; it changed
    `+0x00`, `+0x0c`, and `+0x10`.
  - `0x00b95668` / `0x00b956d8` carry `Sequence8`, `Sequence9`, and
    `claps`; each changed local/pointer rows in the accepted sample.
  - `0x00b957c8` carries `blewbignote_v1_4.wav`,
    `world/battle/samples/blewbignote_v1_4.wav`, and `encore_no`.
  - `0x00b94ef8` / `0x00b94f68` carry `Sequence11`, `crowd_lose`, and
    `_default`.
  - `0x00b962b8` carries `vroom1_boston.wav`,
    `world/battle/samples/vroom1_boston.wav`, and `blewbignote_v1_4.cue`.
  - `0x00b964f8` carries `blewbignote_v1_2.wav` and
    `world/battle/samples/blewbignote_v1_2.wav`.
  - `0x00b94a28` carries `_default` and `crowd_begin`.
  - `0x00850e40` is a live `downbeat` payload neighborhood with 26 changed
    cells, including the previously sampled `0x00850e80` and
    `0x00850ec0..0x00850ed8` ranges.
  - `0x0084f260` carries `start_shot`, `game_lost`, `band_jump`, and
    `game_over`; changed rows include `0x0084f260`, `0x0084f264`,
    `0x0084f268`, `0x0084f310..0x0084f318`, and
    `0x0084f360..0x0084f368`.
  - `0x00853d40` includes `fade`; `0x00853780` includes `swing`. Both are
    moving payload neighborhoods that need pointer-target follow-up.
  - `0x008536c0` is the hottest sampled payload row in this pass with 67
    changed cells; it needs a pointer-target follow-up rather than string-only
    interpretation.
  - `0x007997d0` again names
    `world/battle/og/battle_lighting.milo`,
    `world/battle/og/gen`, `world/battle/battle_chars.milo`, and
    `world/battle/gen`; changed cells include `0x007997d0`,
    `0x007997d8`, `0x007997f0`, `0x007997f4`, `0x00799838`,
    `0x0079983c`, `0x00799844`, and `0x0079984c`.
- Interpretation: the script-triggered child rows are mostly crowd audio /
  sequence rows, while the payload side is where named gameplay/camera/lighting
  atoms remain live (`downbeat`, `start_shot`, `band_jump`, `game_over`,
  `game_lost`, `fade`, `swing`, and the battle lighting content row). The
  next step is pointer-target follow-up for the hottest payload cells and then
  function tracing around the final camera/light consumers those rows expose.

Accepted named event payload pointer follow-up:

- Tool: `tools/sample_pcsx2_pointer_targets.py`.
- Report: `pcsx2_named_event_payload_pointer_followup_20260611.json`.
- Log: `pcsx2_named_event_payload_pointer_followup_20260611.log`.
- Screenshots:
  `pcsx2_named_event_payload_pointer_followup_20260611.before_pointer.window.png`
  and `pcsx2_named_event_payload_pointer_followup_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_pointer_targets.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 12 --interval 0.20 --object-size 0x100 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_named_event_payload_pointer_followup_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_named_event_payload_pointer_followup_20260611.json" --cell downbeat_head_00850e40=0x00850e40 --cell downbeat_next_00850e44=0x00850e44 --cell downbeat_payload_00850e80=0x00850e80 --cell downbeat_payload2_00850ec0=0x00850ec0 --cell shot_head_0084f260=0x0084f260 --cell shot_next_0084f264=0x0084f264 --cell shot_payload_0084f310=0x0084f310 --cell shot_payload2_0084f360=0x0084f360 --cell fade_head_00853d40=0x00853d40 --cell fade_payload_00853d90=0x00853d90 --cell hot_head_008536c0=0x008536c0 --cell hot_next_008536d0=0x008536d0 --cell hot_payload_008536e0=0x008536e0 --cell swing_head_00853780=0x00853780 --cell swing_payload_008537a0=0x008537a0 --cell value_head_00850070=0x00850070 --cell lighting_head_007997d0=0x007997d0 --cell lighting_next_007997f0=0x007997f0 --cell lighting_payload_00799838=0x00799838`.
- Screenshot gate: both screenshots are accepted active in-song Battle of the
  Bands gameplay at normal speed (`FPS 60`, `VPS 60`, speed `100%`) with
  visible band, venue, HUD, note highway, and camera/lighting movement. They
  are not Retry/fail/startup/frozen/wrong-window captures. No `pcsx2-qt`
  process was left running, and `EnableEE = true` / `EnableEE=enabled` was
  verified afterward.
- Pointer facts:
  - `downbeat_head_00850e40` and `downbeat_next_00850e44` rotate through
    event/list rows naming `intro_start_msg`, `game_won_msg`, `excitement`,
    `lighting_change`, `measure`, `miss`, `beat`, `blow_streak`,
    `elephantbones`, and `downbeat`.
  - `downbeat_payload_00850e80` alternates between `0x00743530` and
    `0x00845040`, with pointed rows naming `downbeat`, `hit_snare`,
    `shot_over`, and `game_won_msg`.
  - `downbeat_payload2_00850ec0` rotates through rows including
    `0x008515a0`, `0x00850070`, `0x00851400`, `0x008541a0`,
    `0x00820a20`, `0x0074bdc0`, `0x00747050`, and `0x00c52390`; pointed
    rows include `game_lost` and `battle_lighting_RndDir`.
  - `shot_head_0084f260` / `shot_next_0084f264` rotate through rows naming
    `downbeat`, `hit_snare`, `shot_over`, `game_won_msg`, `fade`,
    `intro_start_msg`, and event/list rows such as `0x00853d40`,
    `0x008536e0`, `0x007424c0`, and `0x00848d80`.
  - `shot_payload_0084f310` rotates through rows naming `beat`, `band_jump`,
    `game_over`, `game_lost`, `game_won_msg`, `crowd_lighters_slow`,
    `downbeat`, `excitement`, `intro_start_msg`, and `crowd_update`.
  - `fade_head_00853d40` rotates through rows naming `start_shot`,
    `game_lost`, `beat`, `intro_start_msg`, `game_won_msg`, `excitement`,
    and `game_over`. `fade_payload_00853d90` points through
    `0x00853570`, `0x0074a3e0`, and `0x008537a0`.
  - `hot_head_008536c0` / `hot_next_008536d0` / `hot_payload_008536e0`
    rotate through rows naming `downbeat`, `already_entered`,
    `msg_last_frame`, `peak_off_player`, `excitement`, `beat`,
    `VERSECHORUS`, `VERSECHORUSSOLO`, `lighting_change`, `measure`,
    `world`, `hud`, `track`, `start_shot`, `game_lost`, `intro_start_msg`,
    `elephantbones`, `band_jump`, `game_over`, `game_won_msg`, and
    `crowd_lighters_slow`.
  - `swing_head_00853780` rotates through rows naming `downbeat`,
    `hit_snare`, `shot_over`, `game_won_msg`, and `clap_v1_6.wav`.
    `swing_payload_008537a0` points through `swing`, `crowd_lighters_off`,
    and `ui/eng/locale.dtb`.
  - `value_head_00850070` rotates through rows naming `excitement`,
    `crowd_hide`, `crowd_update`, `crowd_lighters_slow`,
    `crowd_lighters_fast`, `crowd_lighters_off`, `crowd_half_tempo`,
    `lighting_change`, `measure`, and `miss`.
  - `lighting_head_007997d0` points through `0x003edec0` and
    `0x0072bf50`; pointed rows include `ui/impactor2.milo`, `ui/gen`, and
    live list data. `lighting_next_007997f0` points through `0x00850f80`,
    `0x007997f0`, and `0x0083e230`, naming `lighting_change`, `measure`,
    `miss`, `onGood5`, `world/battle/og/battle_lighting.milo`,
    `world/battle/og/gen`, `world/battle/battle_chars.milo`, and
    `downbeat`. `lighting_payload_00799838` points through `0x00821180` and
    `0x00821120`.
- Interpretation: payload graph evidence now directly links `start_shot`,
  `fade`, `swing`, `downbeat`, crowd lighters, gameplay state atoms, and
  battle lighting rows into the same mutable event/list graph. This is still
  not the final camera or lighting renderer consumer; next trace work should
  isolate function consumers for `start_shot`, `lighting_change`, and the
  `0x007997d0` / `0x00850f80` / `0x00821180` lighting path.

Accepted camera/lighting bridge argument object sampler:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_camera_lighting_bridge_arg_objects_20260611.json`.
- Log: `pcsx2_camera_lighting_bridge_arg_objects_20260611.log`.
- Screenshots:
  `pcsx2_camera_lighting_bridge_arg_objects_20260611.before_sample.window.png`
  and `pcsx2_camera_lighting_bridge_arg_objects_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 12 --interval 0.20 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_camera_lighting_bridge_arg_objects_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_camera_lighting_bridge_arg_objects_20260611.json" --target lighting_owner_00b78418=0x00b78418:0x180 --target lighting_script_00600660=0x00600660:0x120 --target lighting_script_006006a0=0x006006a0:0x140 --target lighting_script_006006b0=0x006006b0:0x140 --target lighting_script_006006f0=0x006006f0:0x120 --target lighting_script_00600770=0x00600770:0x120 --target lighting_apply_base_00520000=0x00520000:0x120 --target lighting_dyn_00842ba0=0x00842ba0:0x120 --target lighting_dyn_00842c00=0x00842c00:0x120 --target lighting_dyn_00842c40=0x00842c40:0x120 --target lighting_dyn_00842c80=0x00842c80:0x120 --target lighting_dyn_00842d50=0x00842d50:0x120 --target lighting_dyn_00842dc0=0x00842dc0:0x120 --target lighting_dyn_00842e30=0x00842e30:0x120 --target cam_graph_check_0060ace0=0x0060ace0:0x180 --target cam_graph_alt_0060ad30=0x0060ad30:0x180 --target cam_eval_00b7c440=0x00b7c440:0x160 --target cam_eval_00b7a2d0=0x00b7a2d0:0x160 --target cam_eval_00b7bd60=0x00b7bd60:0x160 --target cam_eval_00b7c020=0x00b7c020:0x160 --target cam_result_00b92ef0=0x00b92ef0:0x180 --target cam_child_a_00b92f50=0x00b92f50:0x180 --target cam_child_b_00b930e0=0x00b930e0:0x180 --target cam_path_00b8e9d0=0x00b8e9d0:0x180 --target cam_path_frame_00b8ea10=0x00b8ea10:0x180 --target authored_014f5b00=0x014f5b00:0x160 --target authored_014dd4a0=0x014dd4a0:0x160 --target authored_014f4ed0=0x014f4ed0:0x160 --target authored_014f4fe0=0x014f4fe0:0x160 --target authored_014f53d0=0x014f53d0:0x160 --target lighting_row_007997d0=0x007997d0:0x180 --target lighting_payload_00850f80=0x00850f80:0x180 --target lighting_payload_00821180=0x00821180:0x180 --target lighting_payload_00821120=0x00821120:0x180 --target lighting_payload_0083cbc0=0x0083cbc0:0x120 --target lighting_payload_00850460=0x00850460:0x120`.
- Screenshot gate: both screenshots are accepted active in-song Battle of the
  Bands gameplay at normal speed (`FPS 60`, `VPS 60`, speed `100%`) with
  visible band, venue, HUD, note highway, camera output, and lighting. They
  are not Retry/fail/startup/frozen/wrong-window captures. No `pcsx2-qt`
  process was left running, and `EnableEE = true` / `EnableEE=enabled` was
  verified afterward.
- Changed counts:
  - Lighting owner/path: `lighting_owner_00b78418` 10,
    `lighting_dyn_00842ba0` 2, `lighting_dyn_00842c00` 1,
    `lighting_dyn_00842e30` 1, `lighting_row_007997d0` 8,
    `lighting_payload_00850f80` 43, `lighting_payload_00821180` 3,
    `lighting_payload_00821120` 20, and `lighting_payload_00850460` 6.
  - Camera output/path: `cam_eval_00b7a2d0` 17,
    `cam_result_00b92ef0` 41, `cam_child_a_00b92f50` 53,
    `cam_child_b_00b930e0` 37, `cam_path_00b8e9d0` 18, and
    `cam_path_frame_00b8ea10` 25.
  - Static rows that did not change in this 12 second slice:
    `lighting_script_00600660`, `lighting_script_006006a0`,
    `lighting_script_006006b0`, `lighting_script_006006f0`,
    `lighting_script_00600770`, `lighting_apply_base_00520000`,
    `lighting_dyn_00842c40`, `lighting_dyn_00842c80`,
    `lighting_dyn_00842d50`, `lighting_dyn_00842dc0`,
    `cam_graph_check_0060ace0`, `cam_graph_alt_0060ad30`,
    `cam_eval_00b7c440`, `cam_eval_00b7bd60`, `cam_eval_00b7c020`,
    the sampled authored camera blocks, and `lighting_payload_0083cbc0`.
- Object/layout facts:
  - `0x00b78418` is a moving lighting/world owner neighborhood. It carries
    `ctDir` / `world` strings and changes pointer cells at `+0x3c`,
    `+0x4c`, `+0x60`, `+0x70`, `+0x80`, scalar/float-like cells at
    `+0x64..+0x6c`, and a small counter/state at `+0xc8`.
  - Dynamic lighting rows `0x00842ba0`, `0x00842c00`, and `0x00842e30`
    identify `color1`, `color2`, `lighting`, `music_start`, `section`,
    `chorus_1`, `chorus`, `sync_wag`, and `flare`. Their changing cells are
    compact state words such as `0x00010001 -> 0x00030001`.
  - `0x00b7a2d0` is the moving camera eval row for `INTRO_FAST` /
    `Intro_fast`; its changing cells include a phase/time-like float at
    `+0x04` and dense float/quaternion/matrix-like blocks from roughly
    `+0xa0` onward.
  - `0x00b92ef0` and `0x00b92f50` contain matching moving camera result
    matrix/pose blocks. Their first moving blocks start at `0x00b92f10` and
    `0x00b92f50`, and include position-like floats around
    `0x00b92f40..0x00b92f48` / `0x00b92f80..0x00b92f88`.
  - `0x00b930e0` carries `default.cam`, `chillerswing.trig`, and `start`,
    and changes the camera-space position block at `+0x00..+0x0c` plus
    rotation/basis-like rows from `+0x30` onward.
  - `0x00b8e9d0` / `0x00b8ea10` carry `crowd` and mirror the moving camera
    path/result block from `0x00b8eaf0` through the `0x00b8eb20` range.
  - `0x007997d0` again names
    `world/battle/og/battle_lighting.milo`,
    `world/battle/og/gen`, `world/battle/battle_chars.milo`, and
    `world/battle/gen`. It swaps `+0x20/+0x24` through
    `0x00850f80`, `0x007997f0`, and `0x00854140`; swaps `+0x68/+0x6c/+0x74`
    through `0x00821180` / `0x00821120` style payload rows; and changes
    `+0x7c` from `0x01619c40` to `0x00b94bc0`.
  - `0x00850f80` is the hot `lighting_change` / `measure` / `miss`
    payload neighborhood. Its head cells `+0x00..+0x18` rotate through live
    event/list rows, `+0x60` rotates through dynamic lighting rows including
    `0x00842ee0` and `0x00842c00`, and later cells around
    `0x00851030..0x0085103c` can transiently collapse from live pointers to
    small values such as `0x1390`.
  - `0x00821180` changes only at the first three words but cycles through
    many row pointers, including rows previously seen in the descriptor/list
    path.
  - `0x00821120` is a larger moving lighting/list payload row, rotating
    pointers at `+0x00`, `+0x04`, `+0x10..+0x38`, `+0x40`, and adjacent
    cells into rows such as `0x00746910`, `0x00770e50`, `0x007464e0`,
    `0x0084f900`, `0x0084f940`, `0x00848d20`, `0x00854210`, and
    `0x00846290`.
  - `0x00850460` is another moving lighting/list payload row; its first
    three words and `+0x20..+0x28` rotate through live rows such as
    `0x00853720`, `0x0084aec0`, `0x00747b50`, `0x0084ae30`,
    `0x00825438`, `0x0073bb98`, `0x00853db0`, `0x0084aeb0`,
    `0x008539b0`, and `0x008370e8`.
- Interpretation: this trace backs the current camera/lighting bridge object
  identities with live moving data. The camera side now has moving eval,
  result, child, and path rows that line up with the prior
  `cam_apply/result/path` call sequence. The lighting side now ties the
  `0x007997d0` battle lighting row to moving `lighting_change` payloads and
  dynamic `color1/color2/flare` rows. This still does not prove the final
  render-camera handoff, LightPreset/keyframe math, or render-light consumer;
  continue by tracing consumers of the moving output rows and by isolating the
  `0x00271288 -> 0x00271a08` / `0x002716b8 -> 0x00280f60` lighting branches
  against these object addresses.

Limited lighting set-child parent/children retry:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_lighting_set_child_parent_children_retry_20260611.json`.
- Screenshot: `pcsx2_lighting_set_child_parent_children_retry_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 45 --ring-size 16384 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_lighting_set_child_parent_children_retry_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_lighting_set_child_parent_children_retry_20260611.json" --target lighting_set_00271288=0x00271288 --target lighting_set_child_00271a08=0x00271a08 --target child_a_003ac4b8=0x003ac4b8 --target child_b_002cf1d0=0x002cf1d0 --target child_c_00305624=0x00305624 --target child_d_003aaa30=0x003aaa30 --target child_e_003ac2b0=0x003ac2b0 --target child_f_002cf210=0x002cf210 --target child_g_003ab068=0x003ab068 --target child_h_003aaa80=0x003aaa80 --target child_i_00313ca0=0x00313ca0 --target lighting_key_prev_002716b8=0x002716b8 --target lighting_apply_prev_00280f60=0x00280f60 --target lighting_writer_003b50e0=0x003b50e0 --target script_list_002b3818=0x002b3818 --target script_pick_new_002b3d50=0x002b3d50 --target script_do_002b3118=0x002b3118`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay with
  visible band, venue, HUD, note highway, and lighting. The interpreter overlay
  and `45%` speed are expected from `--disable-ee-recompiler`; the capture is
  not Retry/fail/startup/frozen/wrong-window. No `pcsx2-qt` process was left
  running, and `EnableEE = true` / `EnableEE=enabled` was verified afterward.
- Counts:
  - `child_c_00305624`: `939`.
  - `child_b_002cf1d0`: `237`.
  - `child_f_002cf210`: `238`.
  - `lighting_set_00271288`, `lighting_set_child_00271a08`,
    `lighting_key_prev_002716b8`, `lighting_apply_prev_00280f60`,
    `lighting_writer_003b50e0`, the script targets, and all other sampled
    `0x00271a08` static children: `0`.
- Interpretation: this is accepted visual/runtime evidence only for the hot
  helper activity, not for the desired parent chain. Because the same window
  did not hit `0x00271288` or `0x00271a08`, do not use this run to claim that
  `0x002cf1d0`, `0x00305624`, or `0x002cf210` are children of the live
  lighting set-child path. Continue seeking a same-window parent hit.

Accepted narrow lighting parent/keyframe trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_lighting_parent_keyframe_narrow_20260611.json`.
- Screenshot: `pcsx2_lighting_parent_keyframe_narrow_20260611.window.png`.
- Command:
  `python tools\trace_pcsx2_call_sequence.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 45 --ring-size 4096 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_lighting_parent_keyframe_narrow_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_lighting_parent_keyframe_narrow_20260611.json" --target lighting_set_00271288=0x00271288 --target lighting_set_child_00271a08=0x00271a08 --target lighting_key_prev_002716b8=0x002716b8 --target lighting_apply_prev_00280f60=0x00280f60 --target lighting_writer_003b50e0=0x003b50e0 --target script_list_002b3818=0x002b3818 --target script_pick_new_002b3d50=0x002b3d50 --target script_do_002b3118=0x002b3118 --target script_compare_float_002b3658=0x002b3658`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay with
  visible band, venue, HUD, note highway, and strong blue/red lighting. The
  interpreter speed (`48%`) is expected from `--disable-ee-recompiler`; the
  capture is not Retry/fail/startup/frozen/wrong-window. No `pcsx2-qt`
  process was left running, and `EnableEE = true` / `EnableEE=enabled` was
  verified afterward.
- Counts:
  - `lighting_set_00271288`: `6`.
  - `lighting_set_child_00271a08`: `8`.
  - `lighting_key_prev_002716b8`: `2`.
  - `lighting_apply_prev_00280f60`: `2`.
  - `script_list_002b3818`: `4`.
  - `script_pick_new_002b3d50`: `73`.
  - `script_do_002b3118`: `104`.
  - `script_compare_float_002b3658`: `14`.
  - `lighting_writer_003b50e0`: `0`.
- Runtime order/args:
  - Initial set-lighting burst:
    `script_do` rows -> `lighting_set_00271288(stack, 0x00b78418, 0x006006a0, 1)` ->
    `lighting_set_child_00271a08(0x00b78418, 0x00842b20, 0x006006b0, 1)`,
    then `lighting_set_00271288(stack, 0x00b78418, 0x00600660, 1)` ->
    `lighting_set_child_00271a08(0x00b78418, 0x00b78460, 0x00600300, 1)`.
  - Later set-lighting rows repeat with live lighting rows
    `0x00842ba0`, `0x00842c00`, `0x00842c40`, and `0x00842c80`.
  - Two keyframe branches were captured:
    `lighting_set_00271288(stack, 0x00b78418, 0x006006a0, 1)` ->
    `lighting_set_child_00271a08(0x00b78418, 0x00842c00 or 0x00842c80, 0x006006b0, 1)` ->
    `script_list_002b3818(stack, 0x006006f0, 1, 0x00b784f0)` ->
    `script_compare_float_002b3658(stack, 0x00600700, 1, 0x00b784f0)` ->
    `script_pick_new_002b3d50(stack, 0x00600760, 1, 1)` ->
    `lighting_key_prev_002716b8(stack, 0x00b78418, 0x00600770, 1)` ->
    `lighting_apply_prev_00280f60(0x00520000, 0x00b78418, 0x00600770, 1)` ->
    `script_do_002b3118(stack, 0x00600790, 0, 1)`.
  - An isolated direct child call also appears:
    `lighting_set_child_00271a08(0x00b78418, 0x0055032f, 0x0000000e, 1)`,
    near script rows using `0x006015b0`, `0x00601640`, `0x005ff090`,
    `0x005ff0e0`, and `0x0073d3d0`.
- Static/runtime interpretation:
  - The clean same-window parent chain is now re-proven after the failed wider
    helper trace. The parent path is:
    `script rows -> 0x00271288 -> 0x00271a08`, with `0x00b78418` as the live
    lighting/world state and dynamic lighting rows as `a1`.
  - The keyframe path is:
    `script_list/compare/pick_new -> 0x002716b8 -> 0x00280f60`, with
    `0x00520000` as the apply base and `0x00600770` as the keyframe script
    row.
  - `0x003b50e0` stayed zero in this parent-hit run. Static snippets show
    `0x00280f60` first writes the `0x004957b0` list/global block and only
    conditionally calls `0x003b50e0`; treat `0x003b50e0` as a conditional
    commit path, not as the default apply path for this slice.

Accepted lighting keyframe data object sampler:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_lighting_keyframe_data_objects_20260611.json`.
- Log: `pcsx2_lighting_keyframe_data_objects_20260611.log`.
- Screenshots:
  `pcsx2_lighting_keyframe_data_objects_20260611.before_sample.window.png`
  and `pcsx2_lighting_keyframe_data_objects_20260611.window.png`.
- Command:
  `python tools\sample_pcsx2_object_words.py --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 16 --interval 0.20 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_lighting_keyframe_data_objects_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_lighting_keyframe_data_objects_20260611.json" --target lighting_list_global_004957b0=0x004957b0:0x300 --target lighting_list_current_00537e20=0x00537e20:0x180 --target lighting_list_next_00537e28=0x00537e28:0x180 --target timer_global_0052ee40=0x0052ee40:0x120 --target world_light_state_00b78418=0x00b78418:0x240 --target lighting_set_row_006006a0=0x006006a0:0x160 --target lighting_set_child_row_006006b0=0x006006b0:0x160 --target key_script_list_006006f0=0x006006f0:0x160 --target key_script_compare_00600700=0x00600700:0x160 --target key_script_pick_00600760=0x00600760:0x160 --target key_script_apply_00600770=0x00600770:0x160 --target key_script_done_00600790=0x00600790:0x160 --target lighting_dyn_00842b20=0x00842b20:0x160 --target lighting_dyn_00842ba0=0x00842ba0:0x160 --target lighting_dyn_00842c00=0x00842c00:0x160 --target lighting_dyn_00842c40=0x00842c40:0x160 --target lighting_dyn_00842c80=0x00842c80:0x160 --target color_candidate_007fe790=0x007fe790:0x240 --target color_alt_00782580=0x00782580:0x240 --target color_head_00845ca0=0x00845ca0:0x180`.
- Screenshot gate: both screenshots are accepted active in-song Battle of the
  Bands gameplay at normal speed (`FPS 60`, `VPS 60`, speed `100%`) with
  visible band, venue, HUD, note highway, and blue/red venue lighting. They
  are not Retry/fail/startup/frozen/wrong-window captures. No `pcsx2-qt`
  process was left running, and `EnableEE = true` / `EnableEE=enabled` was
  verified afterward.
- Changed counts:
  - `lighting_list_global_004957b0`: `1`.
  - `lighting_list_current_00537e20`: `1`.
  - `lighting_list_next_00537e28`: `0`.
  - `timer_global_0052ee40`: `0`.
  - `world_light_state_00b78418`: `10`.
  - Static script rows `0x006006a0`, `0x006006b0`, `0x006006f0`,
    `0x00600700`, `0x00600760`, `0x00600770`, and `0x00600790`: `0`.
  - Dynamic lighting rows: `0x00842b20` 4, `0x00842ba0` 3,
    `0x00842c00` 2, `0x00842c40` 1, and `0x00842c80` 0.
  - Color/render-state rows: `0x007fe790` 6, `0x00782580` 8, and
    `0x00845ca0` 4.
- Object/layout facts:
  - `0x004957b0 + 0x10` moved from `0x00537e20` to `0x00537e28`, matching
    the static `0x00280f60` list/global update path.
  - `0x00537e20 + 0x04` changed as a float-like timing value while
    `0x00537e28` stayed static in this sample.
  - The static script/keyframe rows remain stable graph data and carry
    `world/world_objects_worldbase.dtb`, `do_lighting_next_keyframe`,
    `lighting_next_keyframe`, `do_lighting_prev_keyframe`,
    `excitement_level`, and `ignored_last_light_change`.
  - `0x00b78418` continued to move its active preset/state pointers:
    `+0x3c`, `+0x4c`, `+0x60`, `+0x64..+0x6c`, `+0x70`, `+0x80`,
    `+0xa0`, and `+0xc8` changed in the same accepted sample.
  - Dynamic lighting rows carry named authored atoms such as `blackout`,
    `color1`, `color2`, `lighting`, `section`, `intro`, `verse`,
    `music_start`, `chorus_1`, `chorus`, `sync_wag`, and `flare`. Their
    moving state cells are compact words such as `0x00010001` /
    `0x00030001`.
  - `0x007fe790`, `0x00782580`, and `0x00845ca0` again expose RGB-like
    mutable rows at `+0x10..+0x18`. In this sample `0x00845ca0 + 0x10/+0x14/+0x18`
    moved from about `0.298/0.298/0.159` toward `0.350/0/0.350` style
    values, while `0x00782580` carried `screen_change` and linked rows such
    as `0x00b88180`.
- Interpretation: this sample ties the successful parent/keyframe trace to
  the live data model. The script rows are static DTB graph nodes; the moving
  state lives in `0x00b78418`, dynamic lighting rows under `0x00842b20..`,
  the `0x004957b0` list/global cursor, and the RGB-like render-light/color
  rows. Native lighting should model both graph-driven keyframe selection and
  stateful color row mutation, not just set a single immediate venue color.

Focused character deformation field/layout notes are split out in
`CHARACTER_DEFORM_FORMAT.md`. Read that file before changing any native
IK/twist/hair/eye/prop code; it is a derivative guide over accepted PCSX2
traces, not a replacement for the full trace map.

Accepted prop clip/Trans same-window trace:

- Tool: `tools/trace_pcsx2_call_ring.py`.
- Report: `pcsx2_prop_clip_trans_same_window_20260611.json`.
- Log: `pcsx2_prop_clip_trans_same_window_20260611.log`.
- Screenshot: `pcsx2_prop_clip_trans_same_window_20260611.window.png`.
- Command:
  `python "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\tools\trace_pcsx2_call_ring.py" --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 12 --ring-size 16384 --stub-base 0x01d00000 --data-base 0x01e00000 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_prop_clip_trans_same_window_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_prop_clip_trans_same_window_20260611.json" --target clip_eval_0016b1d0=0x0016b1d0 --target clip_apply_0016b2f0=0x0016b2f0 --target clip_output_00168320=0x00168320 --target clip_final_0016ab88=0x0016ab88 --target trans_dirty_001dd748=0x001dd748 --target trans_world_003d8ea0=0x003d8ea0`
- Screenshot gate: accepted active in-song Battle of the Bands gameplay with
  visible band, venue, HUD, note highway, guitar props, camera, and lighting.
  The interpreter speed (`54%`) is expected from `--disable-ee-recompiler`;
  this is not a Retry/fail/startup/wrong-window capture. No `pcsx2-qt`
  process was left running, and `EnableEE = true` / `EnableEE=enabled` was
  verified afterward.
- Counts:
  - `clip_eval_0016b1d0`: `5632`.
  - `clip_apply_0016b2f0`: `9650`.
  - `clip_output_00168320`: `26120`.
  - `clip_final_0016ab88`: `9677`.
  - `trans_dirty_001dd748`: `342612`.
  - `trans_world_003d8ea0`: `662873`.
- Prop-relevant Trans hits:
  - `trans_dirty_001dd748` retained `238` prop/prop-adjacent records:
    `bone_pos_mic` dirty row `0x00ce15f0` `34` times, singer pelvis dirty
    row `0x00ce1df0` `34` times, `bone_pos_mic` child dirty row
    `0x00ce16f0` `34` times, singer pelvis child dirty row `0x00ce1ef0`
    `34` times, `guitar_strings` dirty row `0x007641c0` `17` times,
    `guitar_fire` dirty row `0x00764350` `17` times, guitar attachment/world
    row `0x00db6af0` `34` times, and `bone_pos_guitar` dirty row
    `0x00db69f0` `34` times.
  - `trans_world_003d8ea0` retained `243` prop/prop-adjacent records:
    `bone_pos_mic` dirty/world rows `0x00ce15f0` / `0x00ce1650`, singer
    pelvis dirty/world rows `0x00ce1df0` / `0x00ce1e50`, guitar attachment
    rows `0x00db6af0` / `0x00db6b50`, and `guitar_strings` row
    `0x007641c0`.
  - Representative retained records include
    `trans_dirty(0x00ce15f0, 0x00180a48, 0x00ce2970, 0x00d38c9c)`,
    `trans_dirty(0x007641c0, 0x00180a48, 0x00dbf290, 0x00dca7e6)`,
    `trans_dirty(0x00db69f0, 0x00180a48, 0x00dbf290, 0x00dca7e6)`,
    `trans_world(0x00ce15f0, 0x00ce1650, 0x700000b0, 0)`, and
    `trans_world(0x00db6af0, 0x00db6b50, 0x700020e0, 0)`.
- Interpretation: this same-window trace proves the clip pipeline was active
  in the exact active-song window where guitar/mic attachment rows entered the
  Trans dirty/world bridge. It does not prove a direct clip-output argument
  pointer to the prop rows, because retained `clip_eval`, `clip_apply`,
  `clip_output`, and `clip_final` args did not contain the sampled prop row
  addresses. Keep looking for descriptor/channel-to-Trans binding if exact
  clip-to-prop field names are needed.

Accepted prop/constraint vtable-slot trace:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_prop_vtable_slots_sequence_20260611.json`.
- Log: `pcsx2_prop_vtable_slots_sequence_20260611.log`.
- Screenshot: `pcsx2_prop_vtable_slots_sequence_20260611.window.png`.
- Command:
  `python "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\tools\trace_pcsx2_call_sequence.py" --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 1 --seconds 12 --ring-size 16384 --stub-base 0x01c00000 --data-base 0x01d00000 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_prop_vtable_slots_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_prop_vtable_slots_sequence_20260611.json" --target charpos_update_0017f950=0x0017f950 --target charpos_slot34_00180440=0x00180440 --target charpos_slot3c_00180500=0x00180500 --target trans_update_001dcf60=0x001dcf60 --target trans_slot34_001de370=0x001de370 --target trans_slot3c_001df640=0x001df640 --target mesh_update_0019dd88=0x0019dd88 --target mesh_slot1c_001c87f0=0x001c87f0 --target mesh_slot34_001c8c70=0x001c8c70`
- Screenshot gate: accepted active in-song Battle of the Bands gameplay with
  visible venue, crowd, band, props, camera, and lighting. The interpreter
  speed (`62%`) is expected from `--disable-ee-recompiler`; this is not a
  Retry/fail/startup/wrong-window capture. No `pcsx2-qt` process was left
  running, and `EnableEE = true` / `EnableEE=enabled` was verified afterward.
- Live table source:
  - `CharPosConstraint.const` at `0x00ce2c20` has table `0x003e7dc0`.
    Relevant slots are `+0x0c -> 0x0017f950`, `+0x34 -> 0x00180440`,
    and `+0x3c -> 0x00180500`.
  - Prop/bone Trans rows use table `0x003eaae8`; relevant slots include
    `+0x0c -> 0x001dcf60`, `+0x34 -> 0x001de370`, and
    `+0x3c -> 0x001df640`.
  - Visible mesh rows use table `0x003e8a58`; sampled slots included
    `+0x0c -> 0x0019dd88`, `+0x1c -> 0x001c87f0`, and
    `+0x34 -> 0x001c8c70`.
- Counts:
  - `charpos_update_0017f950`: `0`.
  - `charpos_slot34_00180440`: `0`.
  - `charpos_slot3c_00180500`: `0`.
  - `trans_update_001dcf60`: `0`.
  - `trans_slot34_001de370`: `20`.
  - `trans_slot3c_001df640`: `10`.
  - `mesh_update_0019dd88`: `0`.
  - `mesh_slot1c_001c87f0`: `0`.
  - `mesh_slot34_001c8c70`: `0`.
- Representative retained records:
  - `trans_slot3c_001df640(0x00b781e0, 0x005fee70, 0x005fec24, 2)`.
  - `trans_slot34_001de370(0x01ffe5a0, 0x00b781e0, 0x008504e0, 0)`.
  - `trans_slot34_001de370(0x01ffe510, 0x00b7d2e0, 0x008504e0, 0)`.
  - `trans_slot34_001de370(0x01ffe480, 0x00b8a610, 0x008504e0, 0)`.
- Interpretation: in this accepted active-song slice, no sampled
  `CharPosConstraint` or visible mesh update slot fired. Trans vtable slots did
  fire. This is negative evidence against a per-frame `CharPosConstraint`
  prop handler in the sampled window, and positive evidence that the live
  prop/attachment path is Trans-centered. It does not prove the constraint
  slots are never used; they may be setup/event/placement paths or require a
  different singer/prop state.

Accepted long prop/constraint vtable-slot follow-up:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_prop_vtable_slots_long_sequence_20260611.json`.
- Log: `pcsx2_prop_vtable_slots_long_sequence_20260611.log`.
- Screenshot: `pcsx2_prop_vtable_slots_long_sequence_20260611.window.png`.
- Screenshot state: accepted active in-song Battle of the Bands gameplay with
  venue, band, props, camera, and lighting visible. No foreground forcing was
  used.
- Counts from a 45-second run:
  - `total_calls`: `93438`.
  - `ring_size`: `65536`; the retained ring wrapped, so use this as
    retained-window evidence, not whole-run frequency.
  - `charpos_update_0017f950`: `0`.
  - `charpos_slot34_00180440`: `0`.
  - `charpos_slot3c_00180500`: `0`.
  - `trans_update_001dcf60`: `0`.
  - `trans_slot34_001de370`: `4`.
  - `trans_slot3c_001df640`: `22`.
  - `mesh_update_0019dd88`: `0`.
  - `mesh_slot1c_001c87f0`: `0`.
  - `mesh_slot34_001c8c70`: `0`.
  - `trans_dirty_001dd748`: `22078`.
  - `trans_world_003d8ea0`: `41736`.
  - `clip_output_00168320`: `1696`.
- Representative retained records:
  - `trans_slot3c_001df640(0x00b781e0, 0x005fee70, 0x005fec24, 2)`
    followed by camera-result world/dirty rows around `0x00b92ef0`.
  - `trans_slot3c_001df640(0x00b8b560, 0x00728a30, 0x0051eaf0, 1)`
    immediately followed by a clip-output burst on `0x00dbf29c`.
  - `trans_slot3c_001df640(0x00b8d640, 0x005f8b70, 0x0051eaf0, 1)`
    immediately followed by clip outputs on `0x010dbd3c`.
  - Similar clip-output-adjacent slot calls appeared for `0x00b8cb40`,
    `0x00b8f3d0`, `0x00b8fdb0`, `0x00b8db30`, `0x00b8eee0`, and
    `0x00b8d030`.
  - `trans_slot34_001de370` fired as a four-call block with stack output rows
    and source rows `0x00b781e0`, `0x00b7d2e0`, `0x00b8a610`, and
    `0x00b8c4d0`.
- Interpretation: this strengthens the earlier Trans-centered prop/attachment
  finding and repeats the zero-hit result for the sampled `CharPosConstraint`
  and Mesh slots over a longer active window. It still does not prove alternate
  setup/event/singer states never call those slots.

Accepted prop slot-argument object sampler:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_prop_slot_arg_objects_20260611.json`.
- Log: `pcsx2_prop_slot_arg_objects_20260611.log`.
- Screenshots:
  `pcsx2_prop_slot_arg_objects_20260611.before_sample.window.png` and
  `pcsx2_prop_slot_arg_objects_20260611.window.png`.
- Screenshot state: accepted active in-song Battle of the Bands gameplay with
  close guitarist/prop/camera view.
- Sampled the long trace's slot argument rows. Most sampled rows were stable
  structural/source rows in the 12-second window. The moving exception was
  `0x00b8e020`, which changed 14 matrix/position-style cells in paired local
  and output bands:
  - `+0x20`, `+0x24`, `+0x30`, `+0x34`, `+0x48`, `+0x50`, `+0x54`.
  - `+0x60`, `+0x64`, `+0x70`, `+0x74`, `+0x88`, `+0x90`, `+0x94`.
- Interpretation: slot arguments include stable Trans/source structures and at
  least one live moving Trans-style block. Treat stable slot args as structural
  evidence, not as proof that the associated actor or prop is inactive.

Static placement/camera follow-up:

- Tool: `tools/dump_function_snippets.py`.
- Report: `ps2_function_snippets_placement_camera_followup_20260611.json`.
- Command:
  `python "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\tools\dump_function_snippets.py" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\ps2_function_snippets_placement_camera_followup_20260611.json" --insns 140 --func 0x0010cfa0 --func 0x00190770 --func 0x00162b30 --func 0x001264f8 --func 0x0011f628 --func 0x0011f848 --func 0x0010c948 --func 0x0010b9e8`.
- Static facts:
  - `0x0010cfa0` resolves two script/object refs through `0x002bb4f0` and
    indirect object access, calls `0x003d8ea0` three times on `s2+0xe0`,
    then calls `0x00190770` and `0x00162b30`.
  - `0x00190770` iterates a global/list row at `0x0046c694`, calls
    `0x003d8ea0` on each candidate's `+0x40` Trans, computes a distance
    against its input vector, and returns the nearest candidate object.
  - `0x00162b30` calls `0x003d8ea0` on `a1+0x40`, copies a world-matrix-like
    block into `this+0xe0`, calls `0x001dd748`, then updates object refs via
    `0x002c1df8` / `0x002c1d50`.
  - `0x0011f848` dispatches a `world/camshot.dtb` graph object into
    `0x0011f628`.
  - `0x0011f628` has its own camera-shot placement-distance branch:
    `0x001264f8 -> 0x0010b9e8 -> script/object lookups -> 0x0010c948 ->
    0x002b7ff0 -> 0x002c6808 -> 0x003d8ea0 -> 0x00190770`.
- Interpretation: there are at least two placement-related paths to keep
  separate. `0x0010cfa0` is the phase-gated performer placement/apply handler.
  `0x0011f628 -> 0x00190770` is a camera-shot check path that asks for a
  nearest performer/candidate by world-transform distance. Do not collapse
  these into one native rule.

Focused camera placement-distance sequence:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_camera_placement_distance_sequence_20260611.json`.
- Log: `pcsx2_camera_placement_distance_sequence_20260611.log`.
- Screenshot: `pcsx2_camera_placement_distance_sequence_20260611.window.png`.
- Command:
  `python "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\tools\trace_pcsx2_call_sequence.py" --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 3 --seconds 30 --ring-size 16384 --stub-base 0x01c00000 --data-base 0x01d00000 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_camera_placement_distance_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_camera_placement_distance_sequence_20260611.json" --target cam_check_graph_0011f848=0x0011f848 --target cam_check_eval_0011f628=0x0011f628 --target placement_dist_00190770=0x00190770 --target placement_apply_00162b30=0x00162b30 --target placement_handler_0010cfa0=0x0010cfa0 --target trans_world_003d8ea0=0x003d8ea0 --target trans_dirty_001dd748=0x001dd748 --target obj_index_001264f8=0x001264f8 --target char_shot_ok_0010c948=0x0010c948 --target char_cmd_helper_0010b9e8=0x0010b9e8 --target ref_lookup_002c6808=0x002c6808 --target object_ref_002c1580=0x002c1580 --target script_arg_sym_002b7ff0=0x002b7ff0 --target script_arg_obj_002b7e28=0x002b7e28 --target script_arg_obj_alt_002b7f80=0x002b7f80`.
- Screenshot gate: accepted as limited active-song evidence. The screenshot
  shows Battle of the Bands gameplay with visible band, venue, HUD, camera,
  and lighting; it is not Retry/fail/startup/wrong-window. The interpreter
  overlay shows the slowed probing state, and the run still captured live
  Trans traffic. No `pcsx2-qt` process was left running, and
  `EnableEE = true` / `EnableEE=enabled` was verified afterward.
- Counts:
  - `cam_check_graph_0011f848`: `1`.
  - `cam_check_eval_0011f628`: `1`.
  - `placement_dist_00190770`: `0`.
  - `placement_apply_00162b30`: `0`.
  - `placement_handler_0010cfa0`: `0`.
  - `trans_world_003d8ea0`: `10651`.
  - `trans_dirty_001dd748`: `5686`.
  - `obj_index_001264f8`: `8`.
  - `char_shot_ok_0010c948`: `1`.
  - `char_cmd_helper_0010b9e8`: `1`.
  - `ref_lookup_002c6808`: `0`.
  - `object_ref_002c1580`: `10`.
  - `script_arg_sym_002b7ff0`: `9`.
  - `script_arg_obj_002b7e28`: `15`.
  - `script_arg_obj_alt_002b7f80`: `1`.
- Interpretation: this run proves the camera-check and character-source helper
  path was active in the same accepted window, but the shot did not request
  the `0x00190770` distance branch. It is limited negative branch evidence,
  not proof that the branch is dead.

Accepted performer placement candidate object sampler:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_performer_placement_candidate_objects_20260611.json`.
- Log: `pcsx2_performer_placement_candidate_objects_20260611.log`.
- Screenshots:
  `pcsx2_performer_placement_candidate_objects_20260611.before_sample.window.png`
  and `pcsx2_performer_placement_candidate_objects_20260611.window.png`.
- Command:
  `python "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\tools\sample_pcsx2_object_words.py" --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 3 --seconds 10 --interval 0.20 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_performer_placement_candidate_objects_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_performer_placement_candidate_objects_20260611.json" --target char_source_00b8be10=0x00b8be10:0x380 --target placement_arg_00b8bf80=0x00b8bf80:0x180 --target linked_start_obj_00b8c170=0x00b8c170:0x280 --target linked_actor_00b8c9d0=0x00b8c9d0:0x180 --target maybe_trans_00db1d60=0x00db1d60:0x180 --target maybe_left_00dbe570=0x00dbe570:0x100 --target maybe_right_00dbf680=0x00dbf680:0x100 --target source_back_00b8bef0=0x00b8bef0:0x180 --target placement_child_00dc77c0=0x00dc77c0:0x180 --target script_or_ref_005f47c0=0x005f47c0:0x180 --target route_list_007c3d50=0x007c3d50:0x100 --target route_list_007c3da4=0x007c3da4:0x100`.
- Screenshot gate: both screenshots are accepted active in-song Battle of the
  Bands gameplay at normal speed (`FPS 60`, `VPS 60`, speed `100%`) with
  visible band, venue, HUD, camera, props, and lighting. No `pcsx2-qt`
  process was left running, and `EnableEE = true` / `EnableEE=enabled` was
  verified afterward.
- Changed row counts:
  - `char_source_00b8be10`: `19`.
  - `placement_arg_00b8bf80`: `7`.
  - `linked_start_obj_00b8c170`: `0`.
  - `linked_actor_00b8c9d0`: `0`.
  - `maybe_trans_00db1d60`: `0`.
  - `maybe_left_00dbe570`: `0`.
  - `maybe_right_00dbf680`: `7`.
  - `source_back_00b8bef0`: `17`.
  - `placement_child_00dc77c0`: `1`.
  - `script_or_ref_005f47c0`: `0`.
  - `route_list_007c3d50`: `0`.
  - `route_list_007c3da4`: `0`.
- Runtime layout facts:
  - `placement_arg_00b8bf80` and `char_source_00b8be10` share the same
    moving placement/pose rows: `0x00b8bf80` moved
    `0x42ab7fc2 -> 0x42ac0042`, `0x00b8bf84` moved
    `0x42a25ec1 -> 0x42a13a77`, `0x00b8c048` moved
    `0x4019672e -> 0x418dffd1`, `0x00b8c04c` and `0x00b8c050` also changed
    every sample.
  - `placement_arg_00b8bf80 + 0x104` (`0x00b8c084`) flipped from
    `0x00dc77c0` to `0x00dc5fb0`.
  - `placement_arg_00b8bf80 + 0x124` (`0x00b8c0a4`) flipped from `1` to `0`.
  - `source_back_00b8bef0` mirrors the same moving rows, so the source block
    overlaps/backs the placement candidate, not a separate unrelated object.
  - `maybe_right_00dbf680` moved 7 rows, including matrix/vector-like rows at
    `0x00dbf6e0..0x00dbf6f4`; this is linked performer-side data in the same
    active window.
  - `placement_child_00dc77c0` only flipped `0x00dc77d8` from `2` to `0`.
  - Stable linked rows (`0x00b8c170`, `0x00b8c9d0`, `0x00db1d60`,
    `0x00dbe570`, `0x005f47c0`, `0x007c3d50`, `0x007c3da4`) are structural
    in this slice, not dead globally.
- Interpretation: the performer source block has live root/placement-like
  data even when the event-driven `0x0010cfa0` placement apply branch is
  phase-gated. Native placement must preserve this live source-block state,
  the camera-shot `0x0011f628 -> 0x00190770` nearest-candidate branch, and the
  separate phase-gated `0x0010cfa0 -> 0x00162b30` apply path.

Accepted performer/camera event trace follow-up:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Static support: `ps2_function_snippets_placement_waypoint_event_20260612.json`.
- Reports:
  - `pcsx2_placement_waypoint_event_trace_20260612.json`
  - `pcsx2_placement_waypoint_event_arg_sample_20260612.json`
- Logs:
  - `pcsx2_placement_waypoint_event_trace_20260612.log`
  - `pcsx2_placement_waypoint_event_arg_sample_20260612.log`
- Screenshots:
  - `pcsx2_placement_waypoint_event_trace_20260612.window.png`
  - `pcsx2_placement_waypoint_event_arg_sample_20260612.window.png`
- Command shape: stock GH2 ISO and `SLUS_214.47`, `--state 1`,
  `--background-input`, `--require-screenshot`, `--disable-ee-recompiler`,
  no `--gui`, `--retry-pulses 0`, 90 seconds, rare placement/waypoint/camera
  event targets, and explicit argument sampling on `0x00165400.a1/a2` plus
  `shot_over.a0/a3`.
- Screenshot gate: both runs reached the normal `Song Failed`/Retry screen for
  `Shout at the Devil`, which is accepted for fail-transition event evidence
  but not active-performance placement proof. No PCSX2 process remained after
  either run.
- Reproduced counts in both runs:
  - `teleport_handler_00165400`: `190`.
  - `shot_over_context_00262dcc`: `95`.
  - Zero-hit in this fail-transition window:
    `placement_event_0010cfa0`, `actual_walking_branch_0010d148`,
    `actually_walking_query_00184fd0`, `placement_dist_00190770`,
    `placement_apply_00162b30`, `waypoint_find_func_00191020`,
    `waypoint_nearest_func_00191078`, `waypoint_last_func_00191160`,
    `start_shot_handler_00262700`, and `current_shot_context_0026fcb0`.
- Static correction: `ps2_static_strings_placement_event_20260612.json` shows
  `0x00165400` is a performer message/event dispatcher with a first subcommand
  branch for literal `teleport` at `0x0040dcf0`, followed by branches for
  `recenter`, `play_clip`, `set_min_lod`, and `calc_bounding_sphere`. Because
  `0x00162b30` stayed zero in this run, the actual
  `teleport -> placement apply` branch did not execute here.
- Runtime argument facts:
  - `teleport_handler_00165400` is the trace target name, but the live calls
    are generic `0x00165400` performer-message dispatch calls. They always used
    `a0=0x01ffe940` scratch, `a3=1`, and alternated receiver rows
    `a1=0x00b8b800` and `a1=0x00b8df40`.
  - `0x00b8b800` resolves to `char/metal_singer/og/metal_singer.milo` at
    `+0x4c`.
  - `0x00b8df40` resolves to `char/metal_bass/og/metal_bass.milo` at
    `+0x4c` and carries script strings `start` and
    `{ $dude 'set_hand' 'devil' }` near `+0x1fc..+0x21c`.
  - The four sampled `teleport.a2` rows include event/script rows with strings
    `lighting_change`, `measure`, `miss`, and `game_over`.
  - `shot_over_context_00262dcc` used `a0=0x00b8bcf0` and
    `a3=0x00b7d170`. The `a3` row contains shot/camera string `lose01` at
    `+0x64`; the `a0` row links `rim_lighting.lit` at `+0xf4` and
    `char/glam1/og/glam1.milo` at `+0x16c`.
- Interpretation: the fail/result transition proves the `0x00165400`
  performer-message dispatcher and `shot_over` are live event-side paths tied
  to performer rows, lighting rows, and the `lose01` shot. It does not exercise
  the actual `teleport` subcommand, the `lose_teleport` / `0x0010cfa0`
  placement-apply handler, or the waypoint handlers. Keep
  `0x0010cfa0 -> 0x00190770 -> 0x00162b30` as a separate phase-gated path,
  still requiring a trigger that fires the actual placement event.

Limited callback follow-up:

- Static support: `ps2_function_snippets_shot_callbacks_20260612.json`.
  `0x00262700` installs registered `start_shot` callback `0x002626b0`, and
  the `shot_over` context installs registered callback `0x00262ab8`.
- Report: `pcsx2_shot_callback_placement_event_trace_20260612.json`.
- Log: `pcsx2_shot_callback_placement_event_trace_20260612.log`.
- Screenshot: `pcsx2_shot_callback_placement_event_trace_20260612.window.png`.
- Visual gate: limited. The screenshot is black, so do not use this run as
  standalone visual proof. Its counts reproduce the preceding accepted
  fail-transition traces and are useful only as same-route callback/child-edge
  evidence.
- Counts:
  - `shot_over_context_00262dcc`: `94`.
  - `performer_message_00165400`: `188`.
  - `performer_event_apply_001b4eb0`: `188`.
  - Zero-hit: registered callbacks `start_shot_callback_002626b0` and
    `shot_over_callback_00262ab8`, registration/dispatch `0x00262700`,
    `guitarist_event_0010c988`, `placement_event_0010cfa0`,
    `placement_dist_00190770`, `placement_apply_00162b30`,
    `recenter_apply_00162b10`, `play_clip_message_001656a8`,
    `performer_set_min_lod_00162780`, `performer_calc_bound_00162358`,
    `actual_walking_branch_0010d148`, and `actually_walking_query_00184fd0`.
- Argument rule: `0x00165400` and `0x001b4eb0` used the same receiver/event
  pairs in the same order: singer row `0x00b8b800` and bass row `0x00b8df40`
  with event rows `0x008539a0`, `0x00850e80`, `0x008540d0`, and
  `0x00739a10`. The sampled `a2` rows again include `lighting_change` and
  `game_over` strings. This proves the generic event dispatcher falls through
  to `0x001b4eb0` in the fail route, while all concrete placement subcommands
  remain unexercised.

Accepted long placement-dispatcher negative follow-up:

- Report: `pcsx2_placement_dispatcher_long_trace_20260614.json`.
- Log: `pcsx2_placement_dispatcher_long_trace_20260614.log`.
- Screenshot: `pcsx2_placement_dispatcher_long_trace_20260614.window.png`.
- Command shape: stock GH2 ISO and `SLUS_214.47`, `--state 1`,
  `--background-input`, `--require-screenshot`, `--disable-ee-recompiler`,
  no `--gui`, no retry pulses, 120 seconds, with the dispatcher, concrete
  placement/waypoint, recenter, performer-message, event-apply, and shot-over
  targets in one window.
- Visual gate: accepted for fail/result event routing only. The final frame is
  the real `Song Failed` Retry screen for `Shout at the Devil` at 14% complete
  with the venue still visible behind the overlay. Do not count it as active
  on-stage placement proof.
- Counts:
  - `performer_message_00165400`: `188`.
  - `performer_event_apply_001b4eb0`: `206`.
  - `shot_over_context_00262dcc`: `94`.
  - Zero-hit in this long fail-transition window:
    `guitarist_event_dispatch_0010c988`,
    `lose_teleport_branch_0010cce0`, `placement_event_0010cfa0`,
    `placement_related_0010c730`, `actual_walking_branch_0010d148`,
    `actually_walking_query_00184fd0`, `placement_dist_00190770`,
    `placement_apply_00162b30`, `recenter_apply_00162b10`,
    `waypoint_find_00191020`, `waypoint_nearest_00191078`,
    `waypoint_last_00191160`, and `shot_start_00262700`.
- Interpretation: this third accepted fail/result route reinforces the split
  between the live generic performer-message/event path and the concrete
  placement/waypoint path. `0x00165400 -> 0x001b4eb0` and `0x00262dcc` are
  definitely part of the result/fail event route; the actual
  `lose_teleport` / `teleport` / `recenter` / waypoint placement branches are
  still route-gated and unhit. Do not spend more traces on the same fail
  transition unless a new trigger is expected to fire one of those concrete
  branches.

Static camera render-handoff follow-up:

- Tool: `tools/dump_function_snippets.py`.
- Report: `ps2_function_snippets_camera_render_handoff_20260611.json`.
- Command:
  `python "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\tools\dump_function_snippets.py" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\ps2_function_snippets_camera_render_handoff_20260611.json" --insns 180 --func 0x00262b08 --func 0x00263410 --func 0x002665a0 --func 0x00266f80 --func 0x002664d0 --func 0x00267008 --func 0x0026c900 --func 0x0026ae00 --func 0x002ff268 --func 0x001b1ee0`.
- Static facts:
  - `0x00262b08` calls `0x00263410`, repeated `0x00266df8`, and result
    writer `0x002665a0`.
  - `0x002665a0` receives the repeated bridge args seen in
    `pcsx2_camera_lighting_consumer_bridge_sequence_20260611.json`
    (`a0=0x00494b80`, `a1=0x00b7c440`, `a2=0x014f5b00`,
    `a3=0x00b92ef0`). It calls `0x001b1ee0`, `0x00266f80` twice,
    `0x002664d0`, `0x00267008` twice, and vector/math helpers. Treat it as a
    camera result blend/write bridge.
  - `0x00266f80` tests list/member rows from `this+0x84` through
    `0x00261c10`.
  - `0x002664d0` compares the `+0x84` list/member spans for two result
    objects.
  - `0x00267008` builds child result rows. It resolves a child object via
    `0x00261c10` and `0x00101ec0`, calls `0x00266e58`, calls `0x001b1270`,
    then reaches `0x00300520`, `0x002d9668`, `0x002d97e8`, `0x002d9a30`,
    and an indirect callback.
  - `0x0026ae00` is the path/Trans apply side and calls `0x003d8ea0`,
    `0x001dd748`, `0x002ff268`, and `0x001b1ee0`.
- Interpretation: the camera handoff is not a single graph node or a single
  pose struct. It is a result blend/write bridge plus a path/Trans apply
  bridge. The next function-trace target for the camera side should include
  `0x00266df8`, `0x00266e58`, `0x001b1270`, `0x00261c10`, and the existing
  result/path functions.

Accepted camera render-handoff object sampler:

- Tool: `tools/sample_pcsx2_object_words.py`.
- Report: `pcsx2_camera_render_handoff_objects_20260611.json`.
- Log: `pcsx2_camera_render_handoff_objects_20260611.log`.
- Screenshots:
  `pcsx2_camera_render_handoff_objects_20260611.before_sample.window.png`
  and `pcsx2_camera_render_handoff_objects_20260611.window.png`.
- Command:
  `python "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\tools\sample_pcsx2_object_words.py" --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 3 --seconds 12 --interval 0.20 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_camera_render_handoff_objects_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_camera_render_handoff_objects_20260611.json" --target render_cam_base_00494b80=0x00494b80:0x500 --target cam_eval_current_00b7a2d0=0x00b7a2d0:0x180 --target cam_eval_arg_00b7c440=0x00b7c440:0x180 --target cam_result_00b92ef0=0x00b92ef0:0x300 --target cam_result_child_00b92f50=0x00b92f50:0x300 --target cam_result_path_00b930e0=0x00b930e0:0x300 --target cam_path_00b8e9d0=0x00b8e9d0:0x300 --target cam_path_frame_00b8ea10=0x00b8ea10:0x300 --target authored_cam_014f5b00=0x014f5b00:0x300 --target authored_alt_014dd4a0=0x014dd4a0:0x300 --target stack_result_a_01ffe750=0x01ffe750:0x180 --target stack_result_b_01ffe790=0x01ffe790:0x180 --target global_cam_like_0059b4c0=0x0059b4c0:0x300 --target cam_graph_check_0060ace0=0x0060ace0:0x180`.
- Screenshot gate: both screenshots are accepted active in-song Battle of the
  Bands gameplay at normal speed (`FPS 60`, `VPS 60`, speed `100%`). The
  sample covers a visible camera transition from a full-band view to a close
  guitarist view, making it stronger camera handoff evidence than a static
  camera slice. No `pcsx2-qt` process was left running, and `EnableEE = true`
  / `EnableEE=enabled` was verified afterward.
- Changed row counts:
  - `render_cam_base_00494b80`: `0`.
  - `cam_eval_current_00b7a2d0`: `16`.
  - `cam_eval_arg_00b7c440`: `0`.
  - `cam_result_00b92ef0`: `107`.
  - `cam_result_child_00b92f50`: `95`.
  - `cam_result_path_00b930e0`: `38`.
  - `cam_path_00b8e9d0`: `75`.
  - `cam_path_frame_00b8ea10`: `76`.
  - `authored_cam_014f5b00`: `0`.
  - `authored_alt_014dd4a0`: `0`.
  - `stack_result_a_01ffe750`: `92`.
  - `stack_result_b_01ffe790`: `85`.
  - `global_cam_like_0059b4c0`: `0`.
  - `cam_graph_check_0060ace0`: `0`.
- Runtime layout facts:
  - `0x00494b80` did not mutate across the camera transition. Treat it as a
    stable render-camera/list base in this slice, not the mutable camera pose.
  - The mutable result family is `0x00b92ef0`, `0x00b92f50`, and
    `0x00b930e0`. Example changing translation-like rows include
    `0x00b92f40/44/48`, `0x00b92f80/84/88`, and
    `0x00b930e0/0x00b930e4/0x00b930e8`.
  - The mutable path/frame family is `0x00b8e9d0` / `0x00b8ea10`, with
    matching rows around `0x00b8eaf0..0x00b8eb28` and a state flag at
    `0x00b8e9d8` flipping from `1` to `0`.
  - Stack/result rows `0x01ffe750` and `0x01ffe790` changed heavily and carry
    pointers such as `0x00abec90`, `0x01ffe8e0`, `0x015e6244`,
    `0x01ffe910`, `0x007f3e90`, and `0x007f3eb0`. Treat these as transient
    bridge rows, not durable object identities.
  - Authored/static rows `0x014f5b00`, `0x014dd4a0`, `0x0059b4c0`, and
    `0x0060ace0` stayed stable. They select or describe the camera, but they
    are not the per-frame output handoff.
- Interpretation: the final render-camera consumer is still not fully named,
  but the mutable handoff objects are now separated from stable graph/authored
  state. Native camera code must preserve this result/path separation and not
  drive the renderer directly from `world/camshot.dtb` or static authored shot
  rows.

Accepted camera child-helper sequence:

- Tool: `tools/trace_pcsx2_call_sequence.py`.
- Report: `pcsx2_camera_child_helpers_sequence_20260611.json`.
- Log: `pcsx2_camera_child_helpers_sequence_20260611.log`.
- Screenshot: `pcsx2_camera_child_helpers_sequence_20260611.window.png`.
- Command:
  `python "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\tools\trace_pcsx2_call_sequence.py" --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --post-retry-seconds 3 --seconds 20 --ring-size 16384 --stub-base 0x01c00000 --data-base 0x01d00000 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_camera_child_helpers_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_camera_child_helpers_sequence_20260611.json" --target cam_apply_00262b08=0x00262b08 --target cam_apply_child_00263410=0x00263410 --target cam_result_writer_002665a0=0x002665a0 --target cam_result_list_check_00266f80=0x00266f80 --target cam_result_compare_002664d0=0x002664d0 --target cam_result_child_00267008=0x00267008 --target cam_result_related_00266df8=0x00266df8 --target cam_child_resolve_00266e58=0x00266e58 --target cam_member_lookup_00261c10=0x00261c10 --target cam_path_iter_0026c900=0x0026c900 --target cam_path_apply_0026ae00=0x0026ae00 --target trans_world_003d8ea0=0x003d8ea0 --target trans_dirty_001dd748=0x001dd748 --target cam_float_helper_002ff268=0x002ff268 --target cam_angle_helper_001b1ee0=0x001b1ee0 --target cam_vec_helper_001b1270=0x001b1270 --target cam_nan_helper_00300520=0x00300520 --target cam_vec_norm_002d9668=0x002d9668 --target cam_sin_002d97e8=0x002d97e8 --target cam_cos_002d9a30=0x002d9a30 --target cam_helper_00307bc0=0x00307bc0`.
- Screenshot gate: accepted active in-song Battle of the Bands gameplay with
  visible band, venue, HUD, note highway, camera, props, and lighting. The
  overlay is slow because `--disable-ee-recompiler` was used for function
  patch tracing; this is not Retry/fail/startup/wrong-window. No `pcsx2-qt`
  process was left running, and `EnableEE = true` / `EnableEE=enabled` was
  verified afterward.
- Counts:
  - `cam_apply_00262b08`: `6`.
  - `cam_apply_child_00263410`: `6`.
  - `cam_result_writer_002665a0`: `6`.
  - `cam_result_list_check_00266f80`: `12`.
  - `cam_result_compare_002664d0`: `6`.
  - `cam_result_child_00267008`: `12`.
  - `cam_result_related_00266df8`: `0`.
  - `cam_child_resolve_00266e58`: `12`.
  - `cam_member_lookup_00261c10`: `12`.
  - `cam_path_iter_0026c900`: `6`.
  - `cam_path_apply_0026ae00`: `5`.
  - `trans_world_003d8ea0`: `10173`.
  - `trans_dirty_001dd748`: `5660`.
  - `cam_float_helper_002ff268`: `164`.
  - `cam_angle_helper_001b1ee0`: `46`.
  - `cam_vec_helper_001b1270`: `258`.
  - `cam_nan_helper_00300520`: `0`.
  - `cam_vec_norm_002d9668`: `0`.
  - `cam_sin_002d97e8`: `0`.
  - `cam_cos_002d9a30`: `0`.
  - `cam_helper_00307bc0`: `0`.
- Runtime order/args:
  - `cam_apply_00262b08(0x00b7a2d0, 0x00b7a2d0, 0x0059b4c0, 0x0041f738)`.
  - `cam_result_writer_002665a0(0x00494b80, 0x00b7a2d0, 0x014dd4a0, 0x00b92ef0)`.
  - `cam_result_list_check_00266f80` alternates over `a0=0x00494b80` and
    `a0=0x014dd4a0`, with `a1=0x00b92f50`, `a2=0x00b930e0`.
  - `cam_result_compare_002664d0(0x00494b80, 0x014dd4a0, 0, 0)`.
  - `cam_result_child_00267008` alternates
    `(0x00494b80, 0x00b7a2d0, 0x00b92ef0, 0x01ffe750)` and
    `(0x014dd4a0, 0x00b7a2d0, 0x00b92ef0, 0x01ffe790)`.
  - `cam_member_lookup_00261c10` alternates `a0=0x00494c30` and
    `a0=0x014dd550`, carrying the same eval/result/stack args.
  - `cam_child_resolve_00266e58` alternates `a0=0x00494b80` and
    `a0=0x014dd4a0`, with `a1=0x01ffe5e0`, `a2=0x00b92ef0`, and the two
    stack result rows.
  - `cam_path_apply_0026ae00` uses `a0=0x00b8e9d0`.
  - `cam_vec_helper_001b1270` appears both on result rows
    `0x00b92ef0` / stack rows and on path rows around `0x00b8ead0` with
    list rows `0x0072b7d0..0x0072b820`.
- Interpretation: the current camera output chain now has live runtime proof
  through:
  `0x00262b08 -> 0x002665a0 -> 0x00266f80/0x002664d0/0x00267008 ->
  0x00261c10/0x00266e58/0x001b1270`, plus the path side
  `0x0026c900 -> 0x0026ae00 -> 0x003d8ea0/0x001dd748/0x002ff268/0x001b1ee0`.
  The zero-hit helpers `0x00266df8`, `0x00300520`, `0x002d9668`,
  `0x002d97e8`, `0x002d9a30`, and `0x00307bc0` are not current-slice
  requirements, but remain alternate/gated branches until other camera states
  prove otherwise.

Accepted bass upper-twist follow-up:

- Reports:
  - `pcsx2_bass_upper_twist_objects_20260611.json`
  - `pcsx2_bass_upper_twist_vptrs_20260611.json`
  - `pcsx2_bass_upper_l_vptr_only_20260611.json`
  - `pcsx2_bass_upper_twist_child_rows_20260611.json`
- Screenshots:
  - `pcsx2_bass_upper_twist_objects_20260611.before_sample.window.png`
    and `pcsx2_bass_upper_twist_objects_20260611.window.png`
  - `pcsx2_bass_upper_twist_vptrs_20260611.before.window.png`
    and `pcsx2_bass_upper_twist_vptrs_20260611.window.png`
  - `pcsx2_bass_upper_l_vptr_only_20260611.before.window.png`
    and `pcsx2_bass_upper_l_vptr_only_20260611.window.png`
  - `pcsx2_bass_upper_twist_child_rows_20260611.before_sample.window.png`
    and `pcsx2_bass_upper_twist_child_rows_20260611.window.png`
- Commands:
  - `python "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\tools\sample_pcsx2_object_words.py" --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 12 --interval 0.20 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_bass_upper_twist_objects_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_bass_upper_twist_objects_20260611.json" --target bass_upper_l_010d8b30=0x010d8b30:0x180 --target bass_upper_r_010dae10=0x010dae10:0x180 --target bass_upper_l_ref_011418f0=0x011418f0:0x180 --target bass_upper_r_ref_01141a10=0x01141a10:0x180 --target bass_upper_l_ref_012e8794=0x012e8794:0x180 --target bass_upper_l_ref_013bac48=0x013bac48:0x180 --target bass_upper_r_ref_0135cf84=0x0135cf84:0x180 --target bass_upper_r_ref_013bb180=0x013bb180:0x180 --target bass_source_00b8df40=0x00b8df40:0x220 --target glam_source_00b8be10=0x00b8be10:0x220`
  - `python "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\tools\trace_pcsx2_controller_targets.py" --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --retry-pulses 2 --post-retry-seconds 1 --seconds 12 --no-default-vptrs --vptr bass_upper_l=0x010D8B34:0x003E8030 --vptr bass_upper_r=0x010DAE14:0x003E8030 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_bass_upper_twist_vptrs_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_bass_upper_twist_vptrs_20260611.json"`
  - `python "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\tools\trace_pcsx2_controller_targets.py" --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --retry-pulses 2 --post-retry-seconds 1 --seconds 12 --no-default-vptrs --vptr bass_upper_l_only=0x010D8B34:0x003E8030 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_bass_upper_l_vptr_only_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_bass_upper_l_vptr_only_20260611.json"`
  - `python "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\tools\sample_pcsx2_object_words.py" --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 12 --interval 0.20 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_bass_upper_twist_child_rows_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_bass_upper_twist_child_rows_20260611.json" --target bass_upper_l_obj_010d8b30=0x010d8b30:0xc0 --target bass_upper_l_upper_ref_01143040=0x01143040:0x180 --target bass_upper_l_twist1_ref_01142940=0x01142940:0x180 --target bass_upper_l_twist2_ref_01142d40=0x01142d40:0x180 --target bass_upper_r_obj_010dae10=0x010dae10:0xc0 --target bass_upper_r_upper_ref_01142140=0x01142140:0x180 --target bass_upper_r_twist1_ref_01143240=0x01143240:0x180 --target bass_upper_r_twist2_ref_01143340=0x01143340:0x180 --target bass_source_00b8df40=0x00b8df40:0x220`
- Results:
  - Initial object sampler: bass upper-twist headers `0x010d8b30` and
    `0x010dae10` stayed stable, as did descriptor refs `0x011418f0`,
    `0x01141a10`, `0x012e8794`, `0x013bac48`, `0x0135cf84`, and
    `0x013bb180`. Bass source `0x00b8df40` moved 14 rows.
  - Combined vptr redirect: both bass upper-twist vptrs redirected from
    `0x003e8030` to the copied table. The retained 160 ticks were
    `0x001823c8` on `a0=0x010dae10`, with live `a3` float-like values.
  - Left-only vptr redirect: `0x010d8b34` also fired 160 `0x001823c8` ticks
    with `a0=0x010d8b30`, `a1=0x01c80080`, `a2=0x010d8b30`, and `a3=0`.
  - Corrected child-row sampler: bass upper-twist child rows changed heavily.
    Left object fields point to `0x01143040`, `0x01142940`, and
    `0x01142d40`, which changed 26, 24, and 42 rows. Right object fields point
    to `0x01142140`, `0x01143240`, and `0x01143340`, which changed 21, 21,
    and 43 rows.
- Interpretation: bass upper twist uses the same `CharUpperTwist`
  `0x003e8030/+0x0c -> 0x001823c8` update path as glam, but the mutable
  deformation output lives in the child Trans/output rows, not the stable
  controller header or descriptor rows. Header stability is not evidence that
  the controller is inactive.

Accepted bassist hair/head descriptor follow-up:

- Rejected scan:
  - `pcsx2_live_ascii_hair_bass_scan_20260611.json` captured
    `pcsx2_live_ascii_hair_bass_scan_20260611.window.png`, but the screenshot
    is a fail-menu frame. Do not use that run as accepted active-song evidence.
- Accepted reports:
  - `pcsx2_live_ascii_hair_bass_scan_active_20260611.json`
  - `pcsx2_bass_hair_mesh_descriptor_rows_20260611.json`
- Screenshots:
  - `pcsx2_live_ascii_hair_bass_scan_active_20260611.active.window.png` is the
    accepted active gameplay gate for the live ASCII scan.
  - `pcsx2_live_ascii_hair_bass_scan_active_20260611.after.window.png` also
    remained active after the RAM scan.
  - `pcsx2_bass_hair_mesh_descriptor_rows_20260611.before_sample.window.png`
    and `pcsx2_bass_hair_mesh_descriptor_rows_20260611.window.png` are both
    accepted active gameplay frames for the object sampler.
- Commands:
  - The accepted ASCII scan launched the real ISO/state route, captured the
    active screenshot immediately after retry input, then scanned live EE RAM
    ranges `0x003f0000..0x006effff`, `0x00530000..0x0082ffff`,
    `0x00b80000..0x0117ffff`, and `0x01000000..0x015fffff` for live ASCII
    strings and refs containing hair/bass/head terms. Output:
    `pcsx2_live_ascii_hair_bass_scan_active_20260611.json`.
  - `python "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\tools\sample_pcsx2_object_words.py" --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --require-screenshot --retry-pulses 2 --post-retry-seconds 1 --seconds 12 --interval 0.20 --no-default-targets --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_bass_hair_mesh_descriptor_rows_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_bass_hair_mesh_descriptor_rows_20260611.json" --target bass_hair_mat_desc_011418e0=0x011418e0:0x60 --target bass_hair_lower_desc_01141850=0x01141850:0x70 --target bass_hair_top_desc_01141898=0x01141898:0x70 --target bass_bone_head_desc_01141748=0x01141748:0x70 --target bass_hair_mat_obj_007cff74=0x007cff74:0x180 --target bass_hair_lower_obj_00756c94=0x00756c94:0x180 --target bass_hair_top_obj_00756334=0x00756334:0x180 --target bass_bone_head_obj_01142914=0x01142914:0x180 --target bass_source_00b8df40=0x00b8df40:0x220`
- Results:
  - The accepted live ASCII scan identifies metal-bass descriptor strings and
    refs: `hair_bassist.mat` string `0x01141be4` / descriptor ref
    `0x011418e8`, `hair_lower.mesh` string `0x01141cd2` / descriptor ref
    `0x01141858`, `hair_top.mesh` string `0x01141d2c` / descriptor ref
    `0x011418a0`, and `bone_head.mesh` string `0x01141edd` / descriptor ref
    `0x01141750`.
  - Object sampler counts: `bass_hair_mat_desc_011418e0` 0,
    `bass_hair_lower_desc_01141850` 0, `bass_hair_top_desc_01141898` 0,
    `bass_bone_head_desc_01141748` 0, `bass_hair_mat_obj_007cff74` 0,
    `bass_hair_lower_obj_00756c94` 0, `bass_hair_top_obj_00756334` 17,
    `bass_bone_head_obj_01142914` 14, and `bass_source_00b8df40` 14.
  - `hair_bassist.mat` object `0x007cff74` and `hair_lower.mesh` object
    `0x00756c94` point back to bass source `0x00b8df40` but stayed stable in
    this slice.
  - `hair_top.mesh` object `0x00756334` points back to `0x00b8df40` and
    changed rows around `0x007563f0..0x0075641c`.
  - `bone_head.mesh` object `0x01142914` points back to `0x00b8df40` and
    changed rows around `0x01142974..0x01142a84`.
- Interpretation: bassist hair attachment is not a single static mesh bind.
  The descriptor/material rows are stable, while the moving visible hair/head
  objects are separate rows linked back to the bassist source. Native import
  must preserve the descriptor-to-object chain and the moving mesh/head rows.

Accepted arms/hands priority follow-up:

- User correction: arms and hands are the immediate character focus. Legs are
  not the active priority unless they block arm/hand verification.
- Reports:
  - `pcsx2_live_ascii_arm_hand_scan_active_20260611.json`
  - `pcsx2_arm_hand_mesh_rows_20260611.json`
  - `pcsx2_arm_hand_controller_rows_20260611.json`
- Screenshots:
  - `pcsx2_live_ascii_arm_hand_scan_active_20260611.window.png`
  - `pcsx2_arm_hand_mesh_rows_20260611.before_sample.window.png`
  - `pcsx2_arm_hand_mesh_rows_20260611.window.png`
  - `pcsx2_arm_hand_controller_rows_20260611.before_sample.window.png`
  - `pcsx2_arm_hand_controller_rows_20260611.window.png`
- Commands:
  - `python "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\tools\scan_live_ee_strings.py" --pcsx2 "C:\Games\Emulators\PCSX2\pcsx2-qt.exe" --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --retry-pulses 2 --pre-retry-seconds 4 --settle-seconds 8 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_live_ascii_arm_hand_scan_active_20260611.log" --snaps "C:\Games\Emulators\PCSX2\snaps" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_live_ascii_arm_hand_scan_active_20260611.json" --term bone_L-hand --term bone_R-hand --term bone_L-foreArm --term bone_R-foreArm --term bone_L-upperArm --term bone_R-upperArm --term bone_L-clavicle --term bone_R-clavicle --term bone_L-foreTwist1 --term bone_L-foreTwist2 --term bone_R-foreTwist1 --term bone_R-foreTwist2 --term bone_L-upperTwist1 --term bone_L-upperTwist2 --term bone_R-upperTwist1 --term bone_R-upperTwist2 --term bone_fret_hand --term bone_strum_hand --term bone_pos_guitar --term left_hand.ik --term right_hand.ik --term left_hand.drv --term right_hand.drv --term foreTwist_L.ik --term foreTwist_R.ik --term upperTwist_L.ik --term upperTwist_R.ik --term bone_L-index01 --term bone_L-index02 --term bone_L-middlefinger01 --term bone_L-thumb01 --term bone_R-index01 --term bone_R-index02 --term bone_R-middlefinger01 --term bone_R-thumb01 --term guitar.mesh --term guitar_strings.mesh --term guitar_fire.mesh --term metal_bass --term glam1 --term rockabill1 --term rockabill2`
  - `python "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\tools\sample_pcsx2_object_words.py" --pcsx2 "C:\Games\Emulators\PCSX2\pcsx2-qt.exe" --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --retry-pulses 2 --pre-retry-seconds 4 --post-retry-seconds 3 --seconds 10 --interval 0.20 --require-screenshot --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_arm_hand_mesh_rows_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_arm_hand_mesh_rows_20260611.json" --no-default-targets --target glam_l_hand_mesh=0x00db8ac4:0x90 --target glam_r_hand_mesh=0x00dba2c4:0x90 --target glam_l_forearm_mesh=0x00dbc5c4:0x90 --target glam_r_forearm_mesh=0x00db8fc4:0x90 --target glam_l_upperarm_mesh=0x00dbacc4:0x90 --target glam_r_upperarm_mesh=0x00dbbbc4:0x90 --target glam_l_clavicle_mesh=0x00db7fc4:0x90 --target glam_r_clavicle_mesh=0x00db8cc4:0x90 --target glam_fret_hand_mesh=0x00db93c4:0x90 --target glam_strum_hand_mesh=0x00dbbdc4:0x90 --target glam_guitar_pos_mesh=0x00db69c4:0x90 --target glam_l_foretwist1_mesh=0x00db8dc4:0x90 --target glam_l_foretwist2_mesh=0x00db6fc4:0x90 --target glam_r_foretwist1_mesh=0x00dbc1c4:0x90 --target glam_r_foretwist2_mesh=0x00dba1c4:0x90 --target glam_l_uppertwist1_mesh=0x00db6bc4:0x90 --target glam_l_uppertwist2_mesh=0x00dbb4c4:0x90 --target glam_r_uppertwist1_mesh=0x00db83c4:0x90 --target glam_r_uppertwist2_mesh=0x00db62c4:0x90 --target bass_l_hand_mesh=0x01143214:0x90 --target bass_r_hand_mesh=0x011415f8:0x90 --target bass_l_forearm_mesh=0x01142314:0x90 --target bass_r_forearm_mesh=0x01142514:0x90 --target bass_l_upperarm_mesh=0x01142e14:0x90 --target bass_r_upperarm_mesh=0x01143414:0x90 --target bass_l_clavicle_mesh=0x01142f14:0x90 --target bass_r_clavicle_mesh=0x01143914:0x90 --target bass_l_uppertwist1_mesh=0x01143114:0x90 --target bass_l_uppertwist2_mesh=0x01142a14:0x90 --target bass_r_uppertwist1_mesh=0x01142214:0x90 --target bass_r_uppertwist2_mesh=0x01143314:0x90`
  - `python "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\tools\sample_pcsx2_object_words.py" --pcsx2 "C:\Games\Emulators\PCSX2\pcsx2-qt.exe" --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --retry-pulses 2 --pre-retry-seconds 4 --post-retry-seconds 3 --seconds 10 --interval 0.20 --require-screenshot --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_arm_hand_controller_rows_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_arm_hand_controller_rows_20260611.json" --no-default-targets --target ik_obj_a_00dbfa40=0x00dbfa40:0x180 --target ik_obj_b_00dbf4f0=0x00dbf4f0:0x180 --target ik_target_a_00dbfa54=0x00dbfa54:0xc0 --target ik_target_b_00dbf504=0x00dbf504:0xc0 --target right_hand_owner_00dbc980=0x00dbc980:0x160 --target right_hand_inner_00dbc98c=0x00dbc98c:0x160 --target left_hand_owner_00dbca20=0x00dbca20:0x160 --target left_hand_inner_00dbca2c=0x00dbca2c:0x160 --target right_hand_sched_0076bb10=0x0076bb10:0x120 --target left_hand_sched_0076be90=0x0076be90:0x120 --target foretwist_l_ctrl_00d1f4d0=0x00d1f4d0:0x180 --target foretwist_r_ctrl_00dbdf80=0x00dbdf80:0x180 --target uppertwist_l_ctrl_00dbf620=0x00dbf620:0x180 --target uppertwist_r_ctrl_00d9e830=0x00d9e830:0x180 --target bass_uppertwist_l_ctrl_010d8b30=0x010d8b30:0x180 --target bass_uppertwist_r_ctrl_010dae10=0x010dae10:0x180 --target glam_source_00b8be10=0x00b8be10:0x220 --target bass_source_00b8df40=0x00b8df40:0x220`
- Screenshot gate: all five screenshots are accepted active Battle of the
  Bands gameplay at 60 FPS/VPS. None are Retry/fail/startup/wrong-window
  captures.
- Mesh-row results:
  - Glam/guitarist rows moved: `0x00dba2c4` `bone_R-hand.mesh` changed 5,
    `0x00dbc5c4` `bone_L-foreArm.mesh` changed 5,
    `0x00db8fc4` `bone_R-foreArm.mesh` changed 5,
    `0x00dbacc4` `bone_L-upperArm.mesh` changed 10,
    `0x00dbbbc4` `bone_R-upperArm.mesh` changed 5,
    `0x00db7fc4` `bone_L-clavicle.mesh` changed 10,
    `0x00db8cc4` `bone_R-clavicle.mesh` changed 9,
    `0x00db93c4` `bone_fret_hand.mesh` changed 5,
    `0x00dbbdc4` `bone_strum_hand.mesh` changed 10, and
    `0x00db69c4` `bone_pos_guitar.mesh` changed 9.
  - Glam/guitarist twist rows moved: `0x00db8dc4` changed 9,
    `0x00db6fc4` changed 1, `0x00dbc1c4` changed 5,
    `0x00dba1c4` changed 10, `0x00db6bc4` changed 10,
    `0x00dbb4c4` changed 5, `0x00db83c4` changed 5, and
    `0x00db62c4` changed 3.
  - Metal-bass arm rows moved: `0x01143214` changed 4,
    `0x01142314` changed 10, `0x01142514` changed 10,
    `0x01142e14` changed 10, `0x01143414` changed 10,
    `0x01142f14` changed 5, and `0x01143914` changed 9.
  - Metal-bass upper-twist rows moved: `0x01143114` changed 10,
    `0x01142a14` changed 13, `0x01142214` changed 5, and
    `0x01143314` changed 10.
- Controller/driver results:
  - IK object `0x00dbfa40` changed rows `0x00dbfa90`, `0x00dbfa94`,
    `0x00dbfa98`, and `0x00dbfaf4`.
  - IK object `0x00dbf4f0` changed rows `0x00dbf540`, `0x00dbf544`, and
    `0x00dbf548`.
  - Both IK objects use table `0x003e79d0` and owner/source `0x00b8be10`.
    The name labels must not be trusted over traced object refs and linked
    target rows.
  - Right hand driver `0x00dbc980` / `0x00dbc98c` moved and routes through
    scheduler pointer `0x0076bb10`, which changed 23 rows and carried current
    command string `strum_open`.
  - Left hand driver `0x00dbca20` / `0x00dbca2c` moved and routes through
    scheduler pointer `0x0076be90`, which changed 12 rows and carried current
    command string `finger_open`.
  - Glam upper-twist controller `0x00dbf620` changed 10 rows. Sampled
    foretwist headers and other upper-twist headers stayed stable in this
    object sample, but prior accepted vtable traces already prove
    `0x00175678` and `0x001823c8` dispatch. Stable headers are not inactive
    systems.
- Interpretation: arm/hand correctness requires the whole chain:
  hand-driver scheduler state, IK controller rows, twist controller dispatch,
  guitar attachment Trans rows, and visible arm/hand/twist mesh rows. Native
  implementation must not drive hand bones from clip channels alone and must
  not collapse IK left/right labels without confirming object refs and target
  links.

Accepted focused arm/hand call-order trace:

- Report:
  - `pcsx2_arm_hand_order_sequence_20260611.json`
- Screenshot:
  - `pcsx2_arm_hand_order_sequence_20260611.window.png`
- Command:
  - `python "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\tools\trace_pcsx2_call_sequence.py" --pcsx2 "C:\Games\Emulators\PCSX2\pcsx2-qt.exe" --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --pre-retry-seconds 4 --post-retry-seconds 3 --seconds 12 --ring-size 32768 --stub-base 0x01c00000 --data-base 0x01d00000 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_arm_hand_order_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_arm_hand_order_sequence_20260611.json" --target chardriver_update_00171830=0x00171830 --target chardriver_selector_00171db0=0x00171db0 --target scheduler_push_00171248=0x00171248 --target active_command_00173b98=0x00173b98 --target command_midi_00173d20=0x00173d20 --target command_inactive_00173e18=0x00173e18 --target broad_dispatch_001726d8=0x001726d8 --target ik_hand_0017a080=0x0017a080 --target ik_child_0017a558=0x0017a558 --target foretwist_00175678=0x00175678 --target uppertwist_001823c8=0x001823c8 --target trans_world_003d8ea0=0x003d8ea0 --target trans_dirty_001dd748=0x001dd748 --target trans_dirty_alt_001dd7b8=0x001dd7b8 --target quat_or_vec_a_002dadf8=0x002dadf8 --target quat_or_vec_b_002dae80=0x002dae80 --target vec_helper_002dad00=0x002dad00 --target vec_helper2_002daa30=0x002daa30 --target clip_eval_0016b1d0=0x0016b1d0 --target clip_apply_0016b2f0=0x0016b2f0 --target clip_output_00168320=0x00168320 --target clip_final_0016ab88=0x0016ab88`
- Counts from the 12-second active-song call ring:
  - Total records: 26,236.
  - `chardriver_update_00171830`: 126.
  - `chardriver_selector_00171db0`: 4.
  - `scheduler_push_00171248`: 3.
  - `broad_dispatch_001726d8`: 2.
  - `clip_eval_0016b1d0`: 128.
  - `clip_apply_0016b2f0`: 215.
  - `clip_output_00168320`: 582.
  - `clip_final_0016ab88`: 215.
  - `ik_hand_0017a080`: 16.
  - `ik_child_0017a558`: 16.
  - `foretwist_00175678`: 24.
  - `uppertwist_001823c8`: 64.
  - `quat_or_vec_a_002dadf8`: 80.
  - `quat_or_vec_b_002dae80`: 80.
  - `vec_helper_002dad00`: 16.
  - `vec_helper2_002daa30`: 1,686.
  - `trans_world_003d8ea0`: 14,964.
  - `trans_dirty_001dd748`: 7,815.
  - `trans_dirty_alt_001dd7b8`: 200.
  - `active_command_00173b98`, `command_midi_00173d20`, and
    `command_inactive_00173e18`: 0 in this specific window only.
- Ordering learned:
  - The focused arm/hand chain is hand driver/scheduler ->
    clip eval/apply/output/final -> IK update/child ->
    Trans world/dirty propagation -> foretwist/uppertwist math ->
    quaternion/vector helper family -> Trans dirty propagation on driven
    output rows.
  - `clip_final_0016ab88` transitions into `ik_hand_0017a080` in the same
    frame-order window, and IK/twist calls are surrounded by hot
    `0x003d8ea0` world resolution and `0x001dd748` dirty propagation.
- IK argument rows learned:
  - First hand pass:
    `ik_hand_0017a080(a0=0x00dbfa40, a1=0x0017a080,
    a2=0x00dbfa54, a3=0)` immediately calls `ik_child_0017a558`, then
    resolves world rows `0x00db89f0`, `0x00dbc4f0`, `0x00dbabf0`,
    `0x00db7ef0`, `0x00dbaef0`, `0x00db92f0`, `0x00db6ff0`, and
    `0x00dbabf0`; dirty rows include `0x00dbc4f0`, `0x00db89f0`,
    `0x00db6bf0`, `0x00db6cf0`, `0x00db7cf0`, `0x00db7ff0`, and
    `0x00db86f0`.
  - Second hand pass:
    `ik_hand_0017a080(a0=0x00dbf4f0, a1=0x0017a080,
    a2=0x00dbf504, a3=0x40c90fdb)` immediately calls `ik_child_0017a558`,
    then resolves world rows `0x00dba1f0`, `0x00db8ef0`, `0x00dbbaf0`,
    `0x00db8bf0`, `0x00dbaef0`, `0x00dbbcf0`, `0x00db87f0`,
    `0x00db68f0`, and `0x00dbbaf0`; dirty rows include `0x00db8ef0`,
    `0x00dba1f0`, `0x00db74f0`, `0x00db9bf0`, `0x00db9cf0`,
    `0x00dbadf0`, and `0x00dbb6f0`.
- Twist argument rows learned:
  - Foretwist `0x00d1f4d0` drives `0x00db8a10` / `0x00db8a20` helper
    rows and dirties `0x00db6ef0` / `0x00db8cf0`.
  - Foretwist `0x00dbdf80` drives `0x00dba210` / `0x00dba220` helper
    rows and dirties `0x00dba0f0` / `0x00dbc0f0`.
  - Upper-twist `0x00dbf620` drives `0x00dbac10` / `0x00dbac20` helper
    rows and dirties `0x00db6af0` / `0x00dbb3f0`.
  - Upper-twist `0x00d9e830` drives `0x00dbbb10` / `0x00dbbb20` helper
    rows and dirties `0x00db82f0` / `0x00db61f0`.
  - Bass upper-twist `0x010d8b30` drives `0x01142d60` / `0x01142d70`
    helper rows and dirties `0x01143040` / `0x01142940`.
  - Bass upper-twist `0x010dae10` drives `0x01143360` / `0x01143370`
    helper rows and dirties `0x01142140` / `0x01143240`.
- Interpretation:
  - The arm/hand failure cannot be fixed by only applying animation clip
    channels to named hand bones. PS2 computes visible arms through a live
    chain of driver/scheduler state, clip output records, IK objects,
    twist controller objects, quaternion/vector helper math, and Trans
    world/dirty propagation.
  - Stable descriptor/header rows must remain linked to their moving output
    rows. The metal-bass right hand and the twist objects already prove this
    pattern.

Accepted bandwide no-hot arm/hand trace:

- Report:
  - `pcsx2_arm_hand_bandwide_nohot_sequence_20260611.json`
- Log:
  - `pcsx2_arm_hand_bandwide_nohot_sequence_20260611.log`
- Screenshot:
  - `pcsx2_arm_hand_bandwide_nohot_sequence_20260611.window.png`
- Screenshot state: accepted active in-song Battle of the Bands gameplay with
  visible venue, band, props, camera, lighting, and HUD. No foreground forcing
  was used.
- Command:
  - `python "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\tools\trace_pcsx2_call_sequence.py" --pcsx2 "C:\Games\Emulators\PCSX2\pcsx2-qt.exe" --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --pre-retry-seconds 4 --post-retry-seconds 3 --retry-pulses 2 --seconds 45 --ring-size 65536 --stub-base 0x01c00000 --data-base 0x01d00000 --disable-ee-recompiler --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_arm_hand_bandwide_nohot_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_arm_hand_bandwide_nohot_sequence_20260611.json" --target chardriver_update_00171830=0x00171830 --target hand_cmd_dispatch_00173b98=0x00173b98 --target hand_cmd_sched_00173d20=0x00173d20 --target scheduler_push_00171248=0x00171248 --target blend_entry_00198660=0x00198660 --target clip_apply_0016b2f0=0x0016b2f0 --target clip_output_00168320=0x00168320 --target clip_final_0016ab88=0x0016ab88 --target ik_hand_0017a080=0x0017a080 --target ik_child_0017a558=0x0017a558 --target foretwist_00175678=0x00175678 --target uppertwist_001823c8=0x001823c8 --target hair_update_00176fb8=0x00176fb8 --target lookat_0017d658=0x0017d658`
- Counts from the 45-second active-song retained ring:
  - `total_calls`: `5280`; `ring_size`: `65536`, so this did not wrap.
  - `chardriver_update_00171830`: `476`.
  - `hand_cmd_dispatch_00173b98`: `2`.
  - `hand_cmd_sched_00173d20`: `2`.
  - `scheduler_push_00171248`: `14`.
  - `blend_entry_00198660`: `19`.
  - `clip_apply_0016b2f0`: `891`.
  - `clip_output_00168320`: `2391`.
  - `clip_final_0016ab88`: `891`.
  - `ik_hand_0017a080`: `66`.
  - `ik_child_0017a558`: `66`.
  - `foretwist_00175678`: `99`.
  - `uppertwist_001823c8`: `264`.
  - `hair_update_00176fb8`: `33`.
  - `lookat_0017d658`: `66`.
- Distribution learned:
  - `0x00171830` updated 14 distinct driver rows 34 times each:
    `0x00c0d360`, `0x00daf090`, `0x00dbca2c`, `0x00dbc98c`,
    `0x0113f2e0`, `0x00fc9cd0`, `0x0101ebb0`, `0x010dbcb0`,
    `0x010f66b0`, `0x011a8c90`, `0x0123b850`, `0x012c2e50`,
    `0x012e1cf0`, and `0x0135cb90`.
  - IK stayed paired on `0x00dbfa40` and `0x00dbf4f0`, 33 calls each.
  - Foretwist split three ways in this bandwide slice:
    `0x00d1f4d0`, `0x00dbdf80`, and `0x0135cfa0`, 33 calls each.
  - Uppertwist split eight ways, 33 calls each:
    `0x00dbf620`, `0x00d9e830`, `0x00c0b6c0`, `0x00ce2c60`,
    `0x010d8b30`, `0x010dae10`, `0x012e8750`, and `0x0135cf40`.
  - Hair stayed on `0x00dbf5a0`; look-at stayed paired on `0x00dbe470` and
    `0x00dbf940`.
- Hand command route learned:
  - `0x00173b98(stack, 0x00dbca20, 0x00850c80, 0)` ->
    `0x00173d20(stack, 0x00dbca20, 0x00850c80, 0)` ->
    `0x00171248(0x00dbca2c, 0x00f20890, 0, 0x00e06c84)` ->
    `0x00198660(0x00768ad0, 0x00b8be10, 0x00f20890, 0)`.
  - The paired right-hand path immediately followed:
    `0x00173b98(stack, 0x00dbc980, 0x00850c80, 0)` ->
    `0x00173d20(stack, 0x00dbc980, 0x00850c80, 0)` ->
    `0x00171248(0x00dbc98c, 0x00ebe910, 0, 0x00e0ce44)` ->
    `0x00198660(0x0076bd90, 0x00b8be10, 0x00ebe910, 0)`.
- Interpretation:
  - This no-hot trace broadens the focused arm order trace. It proves the
    left/right hand command scheduler path and wider performer-driver clip
    scheduling coexist in one active band window.
  - Native arm/hand work must preserve the per-driver scheduler/blend graph and
    per-controller cadence; a single final-bone adjustment is not
    PS2-equivalent.

Accepted bandwide driver/owner row samples:

- Driver rows report:
  - `pcsx2_bandwide_driver_rows_20260611.json`
- Driver rows log:
  - `pcsx2_bandwide_driver_rows_20260611.log`
- Owner rows report:
  - `pcsx2_bandwide_driver_owner_rows_20260611.json`
- Owner rows log:
  - `pcsx2_bandwide_driver_owner_rows_20260611.log`
- Screenshots:
  - `pcsx2_bandwide_driver_rows_20260611.before_sample.window.png`
  - `pcsx2_bandwide_driver_rows_20260611.window.png`
  - `pcsx2_bandwide_driver_owner_rows_20260611.before_sample.window.png`
  - `pcsx2_bandwide_driver_owner_rows_20260611.window.png`
- Screenshot state: accepted active in-song Battle of the Bands gameplay.
- Driver-row offsets learned:
  - For every sampled inner driver, `driver+0x1c` and `driver+0x6c` point to
    the same owner/source row.
  - `driver+0x38` is the moving scheduler/blend row.
  - `driver+0x68` names the driver object.
- Live role map:
  - `0x00c0d360` owner `0x00b8b800`:
    `char/metal_singer/og/metal_singer.milo`, role `singer`, `main.drv`.
  - `0x00daf090` owner `0x00b8be10`:
    `char/glam1/og/glam1.milo`, `main.drv`.
  - `0x00dbc98c` owner `0x00b8be10`: `right_hand.drv`, command row initially
    naming `strum_open`.
  - `0x00dbca2c` owner `0x00b8be10`: `left_hand.drv`, command row initially
    naming `finger_open`.
  - `0x0113f2e0` owner `0x00b8df40`:
    `char/metal_bass/og/metal_bass.milo`, role `bassist`, `main.drv`.
  - `0x0135cb90` owner `0x00b902e0`:
    `char/metal_drummer/og/metal_drummer.milo`, role `drummer`, `main.drv`.
  - `0x00fc9cd0`, `0x0101ebb0`, `0x010dbcb0`, and `0x010f66b0`
    are `crowd_female01..04` main drivers.
  - `0x011a8c90`, `0x0123b850`, `0x012c2e50`, and `0x012e1cf0`
    are `crowd_male01..04` main drivers.
- Interpretation:
  - The 14-row `0x00171830` bandwide set is mixed performer/hand/crowd
    scheduling, not 14 guitarist arm drivers.
  - For arms/hands in this song slice, the specific hand path is glam1 owner
    `0x00b8be10` through `right_hand.drv` and `left_hand.drv`. Singer, bass,
    drums, and crowd rows are main-driver clip/scheduler paths and should not
    receive guitarist hand-specific logic in native code.

Accepted hair/eye follow-up after arms/hands priority note:

- Reports:
  - `pcsx2_hair_eye_bandwide_slots_sequence_20260611.json`
  - `pcsx2_hair_eye_active_rows_20260611.json`
- Logs:
  - `pcsx2_hair_eye_bandwide_slots_sequence_20260611.log`
  - `pcsx2_hair_eye_active_rows_20260611.log`
- Screenshots:
  - `pcsx2_hair_eye_bandwide_slots_sequence_20260611.window.png`
  - `pcsx2_hair_eye_active_rows_20260611.before_sample.window.png`
  - `pcsx2_hair_eye_active_rows_20260611.window.png`
- Screenshot state: accepted active in-song Battle of the Bands gameplay. These
  are trace artifacts only; screenshots are not required in user updates unless
  needed for diagnosis.
- Slot-trace command:
  - `python "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\tools\trace_pcsx2_call_sequence.py" --pcsx2 "C:\Games\Emulators\PCSX2\pcsx2-qt.exe" --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --pre-retry-seconds 4 --post-retry-seconds 3 --retry-pulses 2 --seconds 45 --ring-size 65536 --stub-base 0x01c00000 --data-base 0x01d00000 --disable-ee-recompiler --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_hair_eye_bandwide_slots_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_hair_eye_bandwide_slots_sequence_20260611.json" --target hair_setup_00176aa0=0x00176aa0 --target hair_reset_00176ab0=0x00176ab0 --target hair_update_00176fb8=0x00176fb8 --target lookat_setup_0017d640=0x0017d640 --target lookat_update_0017d658=0x0017d658 --target lookat_child_002ffa60=0x002ffa60 --target lookat_vec_002dad00=0x002dad00 --target lookat_math_002d5fd8=0x002d5fd8 --target chareyes_slot_00174248=0x00174248 --target chareyes_slot_001752f8=0x001752f8 --target chareyes_slot_001753b8=0x001753b8 --target chareyes_slot_00174d68=0x00174d68 --target chareyes_slot_00174e50=0x00174e50 --target chareyes_slot_00174dd0=0x00174dd0`
- Slot-trace result:
  - `total_calls`: `288`; `ring_size`: `65536`, so this did not wrap.
  - `hair_update_00176fb8`: `36`.
  - `lookat_update_0017d658`: `72`.
  - `lookat_child_002ffa60`: `36`.
  - `lookat_vec_002dad00`: `72`.
  - `lookat_math_002d5fd8`: `72`.
  - `hair_setup_00176aa0`, `hair_reset_00176ab0`, `lookat_setup_0017d640`,
    and the sampled `CharEyes` table slots stayed zero-hit in this active
    window.
- Repeating slot order starts:
  - `lookat_child_002ffa60(0x00db9f80, 0x00dbaf50, 0x00db9f80, 0)`.
  - Two `lookat_vec_002dad00` calls using `a3=0x00dbac60` and
    `a3=0x00dbbb60`.
  - `hair_update_00176fb8(0x00dbf5a0, 0x00176fb8, 0x00dbf5a8, 0x40c90fdb)`.
  - `lookat_update_0017d658(0x00dbe470, ..., 0x00dbe48c, 0)` followed by
    `lookat_math_002d5fd8(0x00dbe500, 0x01ffe7f0, 0x00db9f70, 0)`.
  - `lookat_update_0017d658(0x00dbf940, ..., 0x00dbf95c, 0x007c4114)` followed
    by `lookat_math_002d5fd8(0x00dbf9d0, 0x01ffe7f0, 0x00db9f70, 0)`.
- Active-row sampler result:
  - Glam1 `hair.hair` `0x00dbf5a0` changed 10 rows; reset gate
    `0x00dbf5e0` changed 0.
  - Glam1 child/root rows moved: `bone_hair01.mesh` `0x00db81f0` changed 30,
    `bone_head.mesh` `0x00db9ef0` changed 27, `bone_neck.mesh`
    `0x00dbaef0` changed 39, and `bone_bangL.mesh` `0x00dbc7f0` changed 12.
  - Glam1 `CharEyes.eyes` row `0x00dbf700` changed 34; child row
    `0x00dbf740` changed 23.
  - `l-eye.lookat` `0x00dbe470` changed 3, `r-eye.lookat` `0x00dbf940`
    changed 7, and `eye-L.mesh` `0x00766880` / `eye-R.mesh` `0x00779070`
    changed 25 rows each.
  - Glam1 source `0x00b8be10` changed 21 rows.
  - Metal-bass `hair_top.mesh` `0x00756334` changed 17, `hair_lower.mesh`
    `0x00756c94` changed 0, `bone_head.mesh` `0x01142914` changed 14, and
    bass source `0x00b8df40` changed 17.
- Interpretation:
  - Keep arms/hands as the current closure gate. Once arms/hands are finished,
    hair and eyes are next.
  - Glam1 hair/eyes are controller-plus-linked-row systems under source
    `0x00b8be10`; direct `CharEyes` table slots staying cold does not mean
    the eyes are static.
  - Metal-bass hair is not proven to use the same active `hair.hair` tick path
    in this slice. Its moving hair/head rows are under source `0x00b8df40`,
    so a native fix must preserve descriptor-to-object ownership.

Accepted static arm/hand field follow-up:

- Report:
  - `ps2_function_snippets_arm_hand_deep_20260611.json`
- Command:
  - `python "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\tools\dump_function_snippets.py" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\ps2_function_snippets_arm_hand_deep_20260611.json" --insns 260 --func 0x0017a080 --func 0x0017a558 --func 0x00175678 --func 0x001823c8 --func 0x002dadf8 --func 0x002dae80 --func 0x002daa30 --func 0x002ffc60 --func 0x002ffd88 --func 0x002dc500 --func 0x003d8ea0 --func 0x001dd748`
- Static IK fields:
  - `0x0017a080` calls `0x0017a558` first.
  - `0x0017a558` checks pending/active refs at `base+0x44` and
    `base+0x40`, clears `base+0x44`, and writes scalar fields at
    `base+0x60` / `base+0x64`. Those scalar fields were stable in the
    accepted live slice, so they are not the primary moving hand position for
    this window.
  - `0x0017a080` reads the active weight through `base+0x10 -> +0x04`.
  - `base+0x20` / `base+0x2c` are wrapper metadata; the target `Trans` refs
    used by the update are at `base+0x28` and `base+0x34`.
  - Moving IK target vector output is at `base+0x50..0x58`.
- Live IK field tie-in:
  - `0x00dbfa40`: `+0x10 = 0x00dbfaf0`, live scalar
    `0x00dbfaf4`; target refs `+0x28 = 0x00db89f0`,
    `+0x34 = 0x00db92f0`; moving vector rows
    `0x00dbfa90`, `0x00dbfa94`, `0x00dbfa98`.
  - `0x00dbf4f0`: `+0x10 = 0x00dbf230`; target refs
    `+0x28 = 0x00dba1f0`, `+0x34 = 0x00dbbcf0`; moving vector rows
    `0x00dbf540`, `0x00dbf544`, `0x00dbf548`.
- Static twist fields:
  - `0x00175678` `CharForeTwist` uses refs at `base+0x0c..0x14` and
    `base+0x18..0x20`, plus side/bias float `base+0x24`.
  - Accepted live examples:
    `0x00d1f4d0 +0x14 = 0x00db89f0`,
    `+0x20 = 0x00db6ef0`, `+0x24 = 0x42b40000`;
    `0x00dbdf80 +0x14 = 0x00dba1f0`,
    `+0x20 = 0x00dba0f0`, `+0x24 = 0xc2b40000`.
  - `0x001823c8` `CharUpperTwist` uses three refs at
    `base+0x0c..0x14`, `base+0x18..0x20`, and `base+0x24..0x2c`.
  - Accepted live examples:
    `0x00dbf620` links `0x00db6af0`, `0x00dbb3f0`,
    and `0x00dbabf0`; `0x00d9e830` links `0x00db82f0`,
    `0x00db61f0`, and `0x00dbbaf0`.
- Static `Trans` helper fields:
  - `0x001dd748` marks `Trans` dirty by setting `base+0xa0` and recurses
    through the child list rooted at `base+0x18`.
  - `0x003d8ea0` resolves local rows `base+0x20..0x50` into world rows
    `base+0x60..0x90`, using parent/mode fields around `base+0x10` and
    `base+0xa4`, and returns `base+0x60`.
- Interpretation:
  - The accepted live samples already cover these offset neighborhoods, so no
    extra PCSX2 pass was needed for this field-semantics note.
  - Native arm/hand implementation must preserve `Trans` dirty/world behavior
    before IK/twist outputs can be considered final.

Accepted hand-driver scheduler field follow-up:

- Static source:
  - `ps2_function_snippets_arm_hand_deep_20260611.json`
- Live source:
  - `pcsx2_arm_hand_controller_rows_20260611.json`
  - `pcsx2_arm_hand_order_sequence_20260611.json`
- Static fields:
  - `0x00171830` is the per-frame `CharDriver` tick for the accepted
    `left_hand.drv` and `right_hand.drv` inner rows.
  - Relative to the inner row, `+0x38` is the current scheduler/blend pointer,
    `+0x48` is a phase/time accumulator, and `+0x50` is a scalar/time scale.
  - `0x00171db0` selects the current scheduler node by reading driver
    `+0x38`, checking scheduler `+0x18`, and following the `+0x28` chain
    until it finds a node with nonzero `+0x18`.
  - `0x00171248` allocates/initializes a scheduler entry through
    `0x00198660` and writes the returned pointer back to driver `+0x38`.
  - `0x00173b98` dispatches command symbols to `0x00173d20`,
    `0x00173e18`, or broad dispatcher `0x001726d8`. `0x00173d20` can call
    `0x00171248`; `0x00173e18` refreshes a cached clip/source ref under
    driver `+0x60`.
- Live right-hand driver:
  - Inner row `0x00dbc98c`.
  - `+0x38 = 0x0076bb10`, current scheduler pointer.
  - `+0x40 = strum_open`.
  - `+0x44 = 5`.
  - `+0x48 = 0x00dbc9d4`, moving phase/time row with 50 unique values.
- Live left-hand driver:
  - Inner row `0x00dbca2c`.
  - `+0x38 = 0x0076be90`, current scheduler pointer.
  - `+0x40 = finger_open`.
  - `+0x44 = 5`.
  - `+0x48 = 0x00dbca74`, moving phase/time row with 50 unique values.
- Scheduler rows:
  - `0x0076bb10` changed 23 rows. It carried source/target pointer
    `+0x24 = 0x00ebf420`, owner/source `+0x2c = 0x00b8be10`, and moving
    timing/weight rows across `+0x00..+0x20`.
  - `0x0076be90` changed 12 rows. It carried `+0x24 = 0x00f1e760`,
    owner/source `+0x2c = 0x00b8be10`, current hand-map symbol
    `+0x38 = HandMap_DropD2`, and live child/list rows starting near
    `+0x60`.
- Latest-window caveat:
  - `pcsx2_arm_hand_order_sequence_20260611.json` had zero calls to
    `0x00173b98`, `0x00173d20`, and `0x00173e18`; the command dispatcher is
    therefore statically mapped here and supported by older notes, but not
    active in that focused 12-second capture.
- Interpretation:
  - `strum_open` and `finger_open` are scheduler command symbols inside live
    driver objects. Native hand/finger behavior must preserve the driver
    scheduler state feeding clip/output before IK/twist; these strings must
    not be treated as direct final bone transform selectors.

Accepted blend/clip scheduler field follow-up:

- Static sources:
  - `ps2_function_snippets_blend_clip_deep_20260611.json`
  - `ps2_static_dispatch_tables_blend_clip_20260611.json`
  - `ps2_function_snippets_clip_candidate_helpers_20260611.json`
- Live sources:
  - `pcsx2_blend_math_focus_sequence_20260611.json`
  - `pcsx2_scheduler_blend_entry_objects_20260611.json`
  - `pcsx2_blend_child_arg_objects_20260611.json`
- `0x00198660` blend-entry common active-song layout:
  - `+0x00`: mode/flags.
  - `+0x04`: scheduler `+0x30` scalar.
  - `+0x08`: initialized to `1.0`.
  - `+0x0c`: blend delta/rate from explicit input or related-entry result.
  - `+0x10`: target/start value from explicit input, related-entry result, or
    scheduler `+0x18` fallback.
  - `+0x18`: blend gate/weight flag; fallback writes `1.0`, mode low nibble
    `8` writes tiny float `0x358637bd`.
  - `+0x20`: cleared after init.
  - `+0x24`: scheduler/source pointer.
  - `+0x28`: previous/related blend entry link, recursively released by
    `0x00198ac8` / `0x00198a48`.
  - `+0x2c`: performer/source object.
  - `+0x30`: selected child time.
  - `+0x34`: selected child/list index, initialized to `-1`.
- `0x00199000` advances the `entry+0x24 + 0x6c` list in `0x1c`-byte rows,
  wraps `entry+0x34`, and copies selected row `+0x18` to `entry+0x30`.
- `0x00196888` low-nibble dispatch is now static-table mapped:
  modes `2/3` go to `0x001967b0`, mode `4` goes to live-proven
  `0x00196818 -> 0x001966f0`, and modes `0/1/5/6/7/8` use local
  clamp/math. `0x001967b0` remains zero-hit in accepted active-song traces.
- `0x00195b80` clip candidate dispatch is now static-table mapped:
  kind `0 -> 0x00195d54`, kind `2/17/19 -> 0x00195d84`,
  kind `4 -> 0x00195c40`, kind `5/18 -> 0x00195cb0`,
  kind `16 -> 0x00195bf0`, and kinds `1/3/6..15` reject in this table.
- `0x0016c1b0` walks candidate list `clip_context+0xa4`, calls
  `0x00195b80` on list row `+0x08`, removes the accepted node through
  `0x00321990`, and returns the accepted candidate pointer.
- Hot accepted hand/guitar candidate rows use the kind `5/18` route and call
  `0x00196610(candidate, child, symbol, flags)`. Static alternate predicate
  `0x00169aa0` is mapped but still zero-hit in accepted active-song windows.
- Implementation consequence:
  - The arm/hand scheduler is a linked blend-entry system, not a direct string
    or clip selector. Preserve `driver+0x38`, blend-entry `+0x28`, source
    bind/unbind via `0x00195f18`, child-list ticking, and related-row math
    before attempting native arm/hand output.

Accepted arm/hand command and alternate-branch trace:

- Report:
  - `pcsx2_arm_hand_command_alt_trace_20260611.json`
- Run shape:
  - Headless PCSX2, state `1`, EE recompiler disabled for instrumentation,
    45 seconds, 16K ring.
  - Retained `16384` records from `20202` total calls.
  - PCSX2 exited cleanly after the run; `EnableEE = true` and
    `EnableEE=enabled` were verified afterward.
- Counts:
  - `chardriver_update_00171830`: 15365
  - `scheduler_selector_00171db0`: 40
  - `scheduler_push_00171248`: 46
  - `scheduler_sibling_00171330`: 4
  - `command_dispatch_00173b98`: 42
  - `command_midi_00173d20`: 42
  - `command_refresh_00173e18`: 0
  - `clip_event_root_0010c988`: 3
  - `clip_event_branch_0010b7f8`: 1
  - `hot_path_00165400`: 96
  - `hot_child_001b4eb0`: 327
  - `branch_001658d0`: 52
  - `performer_branch_001656a8`: 2
  - `performer_scheduler_00171190`: 2
  - `performer_child_001710e0`: 4
  - `clip_lookup_0016c1b0`: 4
  - `clip_candidate_00195b80`: 48
  - `clip_candidate_child_00196610`: 30
  - `clip_candidate_alt_00169aa0`: 0
  - `blend_entry_init_00198660`: 63
  - `blend_tick_00199000`: 68
  - `blend_related_00196888`: 51
  - `blend_related_alt_001967b0`: 0
  - `blend_related_b_00196818`: 47
  - `blend_related_b_child_001966f0`: 47
- Hand-owner command evidence:
  - `0x00173b98` / `0x00173d20` hit owner row `0x00dbca20` 19 times and
    owner row `0x00dbc980` 23 times, always with command row `0x00850c80`.
  - Left route example:
    `0x00173b98(0x01ffe6e0,0x00dbca20,0x00850c80,0)` ->
    `0x00173d20(0x01ffe640,0x00dbca20,0x00850c80,0)` ->
    `0x00171248(0x00dbca2c,0x00f1e760,0,0)` ->
    `0x00198660(0x0076bb10,0x00b8be10,0x00f1e760,0)` ->
    `0x00199000(0x0076bb10)`.
  - Right route example:
    `0x00173b98(0x01ffe6e0,0x00dbc980,0x00850c80,0)` ->
    `0x00173d20(0x01ffe640,0x00dbc980,0x00850c80,0)` ->
    `0x00171248(0x00dbc98c,0x00ebf420,0,0)` ->
    `0x00198660(0x0076be90,0x00b8be10,0x00ebf420,0)` ->
    `0x00199000(0x0076be90)`.
  - Additional hand candidate/push routes:
    `0x00dbca2c -> 0x00f20890 -> 0x0076bcd0` and
    `0x00dbc98c -> 0x00ebe910 -> 0x00768c10`, both preceded by live
    `0x00195b80 -> 0x00196610` candidate walks.
- Interpretation:
  - The hand command path is no longer static-only. The native arm/hand
    scheduler must reproduce command dispatch into scheduler push and blend
    entry init for the owner/inner-row split.
  - `0x00169aa0`, `0x001967b0`, and `0x00173e18` are still zero-hit in this
    accepted window. Keep them mapped as alternates, but do not require them
    for the common active hand path until a trace proves otherwise.

Accepted hand command object-row follow-up:

- Report:
  - `pcsx2_hand_command_rows_20260611.json`
- Row movement:
  - `cmd_row_00850c80`: 10 changed words over 60 samples, mostly pointer/list
    rows from `+0x60` onward.
  - `left_owner_00dbca20`: 5 changed words.
  - `left_inner_00dbca2c`: 4 changed words.
  - `right_owner_00dbc980`: 10 changed words. This sample range overlaps the
    left owner, so use the inner row for right-only field counts.
  - `right_inner_00dbc98c`: 9 changed words.
  - `left_sched_f1e760`, `right_sched_ebf420`, and `left_sched_f20890` stayed
    stable in this short object sample.
  - `right_sched_ebe910`: 10 changed words.
  - `blend_0076bb10`: 17 changed words.
  - `blend_0076be90`: 11 changed words.
  - `blend_0076bcd0`: 41 changed words.
  - `blend_00768c10`: 36 changed words.
- Field confirmation:
  - Left owner `0x00dbca20` / inner `0x00dbca2c` expose `left_hand.drv`.
    Owner `+0x44` equals inner `+0x38` current blend pointer, owner `+0x4c`
    equals inner `+0x40` command/source symbol, owner `+0x50` equals inner
    `+0x44` mode, and owner `+0x54` equals inner `+0x48` moving phase.
  - Right owner `0x00dbc980` / inner `0x00dbc98c` expose `right_hand.drv`
    with the same pattern.
  - `0x00850c80` is a moving event/list row. Do not treat it as a stable hand
    pose name or direct final bone selector.
  - `0x0076bb10+0x24` rotated between `0x00ebf420` and `0x00f1e760`;
    `0x0076be90+0x24` rotated between `0x00f1e760` and `0x00ebf420`.
    `0x0076be90+0x38` exposed `HandMap_DropD2` in the initial object sample.

Accepted hand command/deformation bridge trace:

- Report:
  - `pcsx2_hand_command_deform_bridge_notrans_20260611.json`
- Run shape:
  - Headless PCSX2, state `1`, EE recompiler disabled for instrumentation,
    45 seconds, 16K ring.
  - `Trans` functions were omitted on purpose; a previous bridge attempt with
    `0x001dd748` and `0x003d8ea0` produced `2456212` total calls and washed
    out command/scheduler evidence.
  - Retained `16384` records from `168970` total calls. Treat this as
    steady-state order evidence, not total-window counts.
  - PCSX2 exited cleanly; EE settings were restored afterward.
- Retained counts:
  - `command_dispatch_00173b98`: 10
  - `command_midi_00173d20`: 10
  - `scheduler_push_00171248`: 2
  - `blend_entry_init_00198660`: 2
  - `blend_tick_00199000`: 2
  - `clip_eval_0016b1d0`: 1678
  - `clip_apply_0016b2f0`: 2463
  - `clip_output_00168320`: 6604
  - `clip_final_0016ab88`: 2463
  - `ik_hand_0017a080`: 420
  - `ik_child_0017a558`: 420
  - `foretwist_00175678`: 630
  - `uppertwist_001823c8`: 1680
- Retained hand command/deform order:
  - Command pair: `0x00173b98/0x00173d20` on `0x00dbca20` and
    `0x00dbc980`, with command row `0x00850c80`.
  - Then clip eval/apply/output/final on hand scheduler sources
    `0x00f1e760` and `0x00ebf420`.
  - Then IK hand/child calls on `0x00dbfa40` and `0x00dbf4f0`.
  - Then foretwist and uppertwist rows.
- Caveat:
  - The two retained `0x00171248` scheduler-push rows in this bridge trace are
    performer rows (`0x0135cb90 -> 0x013bc010` and
    `0x0113f2e0 -> 0x0115fb40`), not hand pushes. Use
    `pcsx2_arm_hand_command_alt_trace_20260611.json` for direct hand
    `0x00171248` evidence.

Accepted hand destination/lane sample and static clip-output follow-up:

- Reports:
  - `pcsx2_hand_dest_lanes_sample_20260611.json`
  - `pcsx2_hand_dest_pointer_targets_20260611.json`
  - `pcsx2_hand_output_trans_short_sequence_20260611.json`
  - `ps2_function_snippets_clip_output_deep_20260611.json`
- Run shape:
  - Headless PCSX2, state `1`, 12 seconds, 0.20 second sample interval.
  - Screenshot sidecars were captured by the helper, but this run did not use
    screenshot review as a gate because the current user instruction permits
    omitting screenshots unless needed.
  - PCSX2 exited after the run; no `pcsx2-qt` process remained in the
    immediate process check.
- Object movement:
  - Shared destination/object block `0x00dbf29c`: 55 changed words.
  - Hand source clip objects `0x00f1e760` and `0x00ebf420`: 0 changed words
    in this sample.
  - Sampled source/output lanes `0x00f1e898`, `0x00f1ea00`,
    `0x00ebf558`, and `0x00ebf6c0`: 0 changed words in this sample.
  - IK rows `0x00dbfa40` and `0x00dbf4f0`: 4 and 3 changed words.
  - Foretwist headers `0x00d1f4d0` and `0x00dbdf80`: 0 changed words in
    this sample.
  - Upper-twist rows `0x00dbf620` and `0x00d9e830`: 10 and 0 changed words.
- Key live moving destination/controller offsets:
  - `0x00dbf414`, `0x00dbf420`, `0x00dbf440`, `0x00dbf444`.
  - `0x00dbf540..0x00dbf548` overlaps the `right_hand.ik` vector fields.
  - `0x00dbf6bc..0x00dbf6f4` and `0x00dbf790..0x00dbf798` overlap the
    `upperTwist_L.ik`/adjacent controller neighborhood.
- Live object names visible in this neighborhood:
  - `bone.servo`
  - `right_hand.ik`
  - `left_hand.ik`
  - `left.weight`
  - `hair.hair`
  - `upperTwist_L.ik`
  - `CharEyes.eyes`
- Static clip-output semantics:
  - `0x0016b1d0` advances CharClipSamples lanes at base offsets `+0x84`,
    `+0x138`, `+0x1ec`, and `+0x2a0`.
  - `0x0016b2f0` computes normalized timing from base `+0x18/+0x1c` and
    applies/interpolates those lanes through `0x00193d78`,
    `0x00193e18`, and `0x0016ab88`.
  - `0x0016ab88` clamps interpolation parameters, samples via `0x001938f8`,
    handles angular wrap with `0x002ffd88` and trig helper `0x002dc500`,
    then calls `0x00168320`.
  - `0x00168320` matches source IDs from the lane against destination IDs and
    accumulates three separate output classes: 16-byte vector rows,
    quaternion-style four-float rows with dot-product sign correction, and
    scalar rows.
- Destination pointer-target bridge:
  - Destination `+0x04` cell `0x00dbf2a0 -> 0x00e060d0` is the stable
    channel ID/name list. The first rows include `bone_facing.pos`,
    `bone_fret_hand.pos`, `bone_pos_guitar.pos`, `bone_strum_hand.pos`,
    left/right clavicle, hand, finger/thumb, and upper-arm quats,
    `bone_fret_hand.quat`, and `bone_head.quat`.
  - Destination live arrays moved: `0x00dbf304 -> 0x00f39f30` changed
    86 rows, `0x00dbf30c -> 0x00f39f90` changed 96 rows, and
    `0x00dbf310 -> 0x00f3a160` changed 28 rows.
  - Left hand lane IDs `0x00f1e89c -> 0x0073da80` name
    `bone_fret_hand.pos`, left finger quats, `bone_fret_hand.quat`, and left
    finger rotz channels; values `0x00f1e900 -> 0x00dd0e10` were stable.
  - Right hand lane IDs `0x00ebf55c -> 0x0073d670` name
    `bone_strum_hand.pos`, right clavicle/upper-arm/hand/finger/thumb quats,
    `bone_strum_hand.quat`, `bone_R-foreArm.rotz`, and right finger rotz
    channels; values `0x00ebf5c0 -> 0x00dca780` were stable.
  - Final/source metadata cells `0x00f1ea68` and `0x00ebf728` pointed back
    to rows naming `finger_open` and `strum_open`.
- No-wrap hand output/Trans order:
  - `pcsx2_hand_output_trans_short_sequence_20260611.json` used a
    65,536-record ring for 2 seconds and retained all `17065` calls.
  - Counts: `clip_output_00168320` 504, `ik_hand_0017a080` 14,
    `ik_child_0017a558` 14, `foretwist_00175678` 19,
    `uppertwist_001823c8` 48, `trans_dirty_001dd748` 5742, and
    `trans_world_003d8ea0` 10724.
  - Hand output burst: repeated `0x00168320(..., a1=0x00dbf29c, ...)`,
    including source lanes `0x00f1e898` and `0x00ebf558`, followed directly
    by dirty propagation on arm/hand rows such as `0x00dbf3f0`,
    `0x00db92f0`, `0x00dbaff0`, `0x00db68f0`, `0x00db72f0`,
    `0x00db97f0`, `0x00dbb0f0`, and `0x00dbbff0`.
  - IK order: `0x0017a080/0x0017a558(0x00dbfa40)` resolves
    `0x00db89f0`, `0x00dbc4f0`, `0x00dbabf0`, `0x00db7ef0`,
    `0x00dbaef0`, and `0x00db92f0`, then dirties `0x00dbc4f0`,
    `0x00db89f0`, `0x00db6bf0`, `0x00db6cf0`, `0x00db7cf0`,
    `0x00db7ff0`, and `0x00db86f0`.
  - Paired IK `0x0017a080/0x0017a558(0x00dbf4f0)` resolves
    `0x00dba1f0`, `0x00db8ef0`, `0x00dbbaf0`, `0x00db8bf0`,
    `0x00dbaef0`, and `0x00dbbcf0`, then dirties `0x00db8ef0`,
    `0x00dba1f0`, `0x00db74f0`, `0x00db9bf0`, `0x00db9cf0`,
    `0x00dbadf0`, and `0x00dbb6f0`.
  - Twist order: foretwist dirties `0x00db6ef0` / `0x00db8cf0` and
    `0x00dba0f0` / `0x00dbc0f0`; uppertwist dirties
    `0x00db6af0` / `0x00dbb3f0`, `0x00db82f0` / `0x00db61f0`,
    and bass equivalents `0x01143040` / `0x01142940` plus
    `0x01142140` / `0x01143240`.
- Implementation gate consequence:
  - Arms/hands are now traced through command dispatch, scheduler push,
    clip lane output, destination descriptor/object rows, IK, and twist.
    Native animation code still should not resume until the remaining trace
    gate is closed, but the common spaghetti-arm path must preserve lane IDs,
    destination arrays keyed by the PS2 channel list, quaternion sign
    correction, IK rows, twist rows, and the shared source-owner graph. Do
    not shortcut this into direct named-bone writes.

Accepted arm IK/twist Trans-row semantic sample:

- Report: `pcsx2_arm_ik_twist_trans_rows_20260611.json`.
- Log: `pcsx2_arm_ik_twist_trans_rows_20260611.log`.
- Command class: headless `sample_pcsx2_object_words.py` run from state `1`
  for 10 seconds with two retry pulses and no default targets.
- Purpose: follow the exact Trans rows named by the no-wrap
  `0x00168320`/IK/twist order trace and resolve parent/name semantics for the
  current glam1 arm graph.
- Results:
  - Left chain:
    `bone_L-hand.mesh` `0x00db89f0` -> `bone_L-foreArm.mesh`
    `0x00dbc4f0` -> `bone_L-upperArm.mesh` `0x00dbabf0` ->
    `bone_L-clavicle.mesh` `0x00db7ef0` -> shared `bone_neck.mesh`
    `0x00dbaef0`.
  - Left target/finger rows:
    `bone_fret_hand.mesh` `0x00db92f0`, plus finger/thumb children
    `0x00db6bf0`, `0x00db6cf0`, `0x00db7cf0`, `0x00db7ff0`, and
    `0x00db86f0`.
  - Right chain:
    `bone_R-hand.mesh` `0x00dba1f0` -> `bone_R-foreArm.mesh`
    `0x00db8ef0` -> `bone_R-upperArm.mesh` `0x00dbbaf0` ->
    `bone_R-clavicle.mesh` `0x00db8bf0` -> shared `bone_neck.mesh`
    `0x00dbaef0`, plus `bone_strum_hand.mesh` `0x00dbbcf0`.
  - Right finger/thumb rows:
    `0x00db74f0`, `0x00db9bf0`, `0x00db9cf0`, `0x00dbadf0`, and
    `0x00dbb6f0`.
  - Foretwist rows:
    `0x00db6ef0` -> `0x00db8cf0` on the left and
    `0x00dba0f0` -> `0x00dbc0f0` on the right.
  - Upper-twist rows:
    `0x00db6af0` -> `0x00dbb3f0` on the left and
    `0x00db82f0` -> `0x00db61f0` on the right.
  - Moving fields group into the Trans local band `+0x20..+0x4f`, world band
    `+0x60..+0x8f`, and tail/output band `+0x90..+0xbf`; stable parent links
    live at `+0x10`.
  - Source owner row `0x00b8be10` names `char/glam1/og/glam1.milo`.
- Interpretation: the current arm/hand issue must be handled as a parented
  Trans graph with driver/scheduler/lane output feeding IK and twist. The PS2
  path updates hand, forearm, upper-arm, clavicle, finger/thumb, fret/strum
  hand, and twist mesh rows; it is not a direct write to a small set of named
  hand bones.

Accepted arm IK/twist field sequence:

- Report: `pcsx2_arm_ik_twist_field_sequence_20260611.json`.
- Log: `pcsx2_arm_ik_twist_field_sequence_20260611.log`.
- Command class: headless `trace_pcsx2_call_sequence.py` from state `1` for
  8 seconds, 65,536-record ring, EE recompiler disabled, no screenshot gate.
- Counts: retained all `14,141` calls:
  - `chardriver_update_00171830`: 84
  - `scheduler_push_00171248`: 1
  - `clip_apply_0016b2f0`: 135
  - `clip_output_00168320`: 359
  - `clip_final_0016ab88`: 135
  - `ik_hand_0017a080`: 10
  - `ik_child_0017a558`: 10
  - `foretwist_00175678`: 15
  - `uppertwist_001823c8`: 32
  - `twist_norm_002dadf8`: 40
  - `twist_vec_002dae80`: 40
  - `twist_apply_a_002ffc60`: 88
  - `twist_apply_b_002ffd88`: 170
  - `shared_angle_002dc500`: 1499
  - `trans_dirty_001dd748`: 3980
  - `trans_world_003d8ea0`: 7543
- Glam hot path:
  - Hand/clip output for left/right hand lanes immediately precedes
    `IK 0x00dbfa40` with arg row `0x00dbfa54`,
    `foretwist 0x00d1f4d0`, `IK 0x00dbf4f0` with arg row
    `0x00dbf504`, then `foretwist 0x00dbdf80`.
  - Foretwist `0x00d1f4d0` feeds helper outputs `0x00db8a10` /
    `0x00db8a20` and dirties `0x00db6ef0` / `0x00db8cf0`.
  - Foretwist `0x00dbdf80` feeds `0x00dba210` / `0x00dba220` and dirties
    `0x00dba0f0` / `0x00dbc0f0`.
  - Upper twist `0x00dbf620` feeds `0x00dbac10` / `0x00dbac20` and dirties
    `0x00db6af0` / `0x00dbb3f0`.
  - Upper twist `0x00d9e830` feeds `0x00dbbb10` / `0x00dbbb20` and dirties
    `0x00db82f0` / `0x00db61f0`.
- Metal-bass hot path:
  - Upper twist `0x010d8b30` feeds `0x01142d60` / `0x01142d70` and dirties
    `0x01143040` / `0x01142940`.
  - Upper twist `0x010dae10` feeds `0x01143360` / `0x01143370` and dirties
    `0x01142140` / `0x01143240`.
- Interpretation: twist output is not a single shared arm rule. Each side and
  character has a controller object, helper output pair, and dirty Trans pair
  that must stay together.

Accepted deeper SLUS field-offset follow-up:

- Report: `ps2_function_snippets_arm_hand_deeper_20260611.json`.
- Static field ties:
  - `CharIKHand` `0x0017a080`: calls `0x0017a558`, resolves controller
    `+0x20` and `+0x2c` through `0x003d8ea0`, reads controller float rows
    `+0x60/+0x64`, uses vector row `+0x50`, and branches on optional
    controller refs `+0x38/+0x3c`.
  - IK prepass `0x0017a558`: selects `+0x44` if set, otherwise `+0x40`,
    clears `+0x44`, updates `+0x60/+0x64`, and clears the selected ref after
    the object callback.
  - Foretwist `0x00175678`: loads source ref `+0x0c`, output ref `+0x18`,
    runs `0x002dadf8`, `0x002dae80`, `0x002ffc60`, `0x002ffd88`, then dirties
    and writes the output Trans rows.
  - Uppertwist `0x001823c8`: uses source/helper ref `+0x24` and paired output
    refs `+0x18` / `+0x0c`, then runs the same helper family and dirties both
    output branches.
- Interpretation: runtime call args and static field loads now agree for the
  arm/hand hot path. Remaining arm/hand work should target any still-unknown
  semantic names for these fields, not their structural offsets.

Accepted long alternate-branch arm/hand trace:

- Report: `pcsx2_arm_hand_alternate_branch_long_sequence_20260611.json`.
- Log: `pcsx2_arm_hand_alternate_branch_long_sequence_20260611.log`.
- Screenshot: `pcsx2_arm_hand_alternate_branch_long_sequence_20260611.window.png`.
- Screenshot gate: accepted active Battle of the Bands gameplay frame.
- Command:
  - `python "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\tools\trace_pcsx2_call_sequence.py" --pcsx2 "C:\Games\Emulators\PCSX2\pcsx2-qt.exe" --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --elf "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\SLUS_214.47" --state 1 --gui --require-screenshot --disable-ee-recompiler --retry-pulses 2 --pre-retry-seconds 4 --post-retry-seconds 3 --seconds 90 --ring-size 65536 --stub-base 0x01c00000 --data-base 0x01d00000 --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_arm_hand_alternate_branch_long_sequence_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_arm_hand_alternate_branch_long_sequence_20260611.json" --target chardriver_update_00171830=0x00171830 --target command_dispatch_00173b98=0x00173b98 --target command_midi_00173d20=0x00173d20 --target command_refresh_00173e18=0x00173e18 --target scheduler_push_00171248=0x00171248 --target blend_entry_00198660=0x00198660 --target blend_tick_00199000=0x00199000 --target blend_related_alt_001967b0=0x001967b0 --target clip_candidate_alt_00169aa0=0x00169aa0 --target ik_update_0017a080=0x0017a080 --target ik_prepass_0017a558=0x0017a558 --target foretwist_00175678=0x00175678 --target uppertwist_001823c8=0x001823c8 --target shared_setup_001d2ab0=0x001d2ab0 --target shared_setup_001d2c48=0x001d2c48 --target shared_setup_001d2e70=0x001d2e70 --target clip_output_00168320=0x00168320`
- Counts: retained all `5,005` calls in a `65,536` record ring:
  - `chardriver_update_00171830`: `700`
  - `command_dispatch_00173b98`: `8`
  - `command_midi_00173d20`: `8`
  - `command_refresh_00173e18`: `0`
  - `scheduler_push_00171248`: `18`
  - `blend_entry_00198660`: `25`
  - `blend_tick_00199000`: `25`
  - `blend_related_alt_001967b0`: `0`
  - `clip_candidate_alt_00169aa0`: `0`
  - `ik_update_0017a080`: `98`
  - `ik_prepass_0017a558`: `98`
  - `foretwist_00175678`: `147`
  - `uppertwist_001823c8`: `384`
  - `shared_setup_001d2ab0`: `0`
  - `shared_setup_001d2c48`: `0`
  - `shared_setup_001d2e70`: `0`
  - `clip_output_00168320`: `3,494`
- Distribution:
  - All 14 known bandwide driver rows ticked exactly `50` times each.
  - Command dispatch and MIDI split evenly: `0x00dbca20` four calls and
    `0x00dbc980` four calls, always with command/event row `0x00850c80`.
  - Retained hand scheduler pushes:
    `0x00dbca2c -> 0x00f20890 -> 0x00768a50`,
    `0x00dbc98c -> 0x00ebe910 -> 0x0076bd90`,
    `0x00dbca2c -> 0x00f1e760 -> 0x0076bb10`, and
    `0x00dbc98c -> 0x00ebf420 -> 0x0076be90`. All blend entries use source
    `0x00b8be10`.
  - IK stayed paired on `0x00dbf4f0` and `0x00dbfa40`, `49` calls each.
  - Foretwist stayed split across `0x00d1f4d0`, `0x00dbdf80`, and
    `0x0135cfa0`, `49` calls each.
  - Uppertwist stayed split across `0x00dbf620`, `0x00d9e830`,
    `0x00c0b6c0`, `0x00ce2c60`, `0x010d8b30`, `0x010dae10`,
    `0x012e8750`, and `0x0135cf40`, `48` calls each.
- Interpretation:
  - This is the strongest no-wrap evidence so far that the steady active-song
    hand path uses `0x00173b98 -> 0x00173d20 -> 0x00171248 -> 0x00198660`
    for left/right hand entries, followed by hot clip output, IK, foretwist,
    and uppertwist.
  - `0x00173e18`, `0x001967b0`, `0x00169aa0`, and the targeted setup/shared
    slots remain mapped but not part of the common active-window path in this
    state.
  - This does not close the additional-song/additional-guitarist coverage gap.

Accepted alternate-song/classic arms-hands trace:

- Reports:
  - `pcsx2_start_input_probe_20260611.json`
  - `pcsx2_pause_quit_nav_probe2_20260611.json`
  - `pcsx2_alt_song_arm_hand_sequence3_20260611.json`
  - `pcsx2_alt_song_arm_hand_sampled_sequence_20260611.json`
  - `pcsx2_alt_song_driver_follow_sequence_20260611.json`
- Screenshots:
  - `pcsx2_start_input_probe_20260611.0x4c.window.png`: active Battle of the
    Bands gameplay.
  - `pcsx2_start_input_probe_20260611.0x20.window.png`: pause menu after
    posted Start with the pause hotkey moved away from Space.
  - `pcsx2_pause_quit_nav_probe2_20260611.0x4c.window.png`: setlist reached.
  - `pcsx2_alt_song_arm_hand_sampled_sequence_20260611.window.png`:
    `SURRENDER` / Cheap Trick title overlay.
  - `pcsx2_alt_song_driver_follow_sequence_20260611.window.png`: active
    Surrender stage frame, but visually imperfect because of a black
    triangular occluder/slow frame. Use the trace data, not this screenshot, as
    role proof.
- Same-process follow sampling resolved the alternate guitarist in Surrender:
  - `0x00ce5db0`: `main.drv`, owner `0x00beed30`,
    `char/metal_singer/og/metal_singer.milo`.
  - `0x00db5720`: `main.drv`, owner `0x00bef340`,
    `char/classic/og/classic.milo`.
  - `0x00dc018c` and `0x00dc022c`: hand-driver rows owned by `0x00bef340`,
    therefore the shifted classic hand rows for this allocation.
  - `0x010a4e10`: `main.drv`, owner `0x00bf1470`,
    `char/metal_bass/og/metal_bass.milo`.
  - `0x0138f630`: `main.drv`,
    `char/metal_drummer/og/metal_drummer.milo`.
- `pcsx2_alt_song_arm_hand_sequence3_20260611.json` retained `2,286` calls in
  a 30-second Surrender window: `chardriver_update_00171830` 308,
  `scheduler_push_00171248` 31, `blend_entry_00198660` 36,
  `clip_output_00168320` 1,559, `ik_hand_0017a080` 44,
  `ik_child_0017a558` 44, `foretwist_00175678` 66,
  `uppertwist_001823c8` 176, and `hair_update_00176fb8` 22.
- `pcsx2_alt_song_driver_follow_sequence_20260611.json` retained `681` calls
  in a shorter same-process sample: `chardriver_update_00171830` 98,
  `scheduler_push_00171248` 12, `blend_entry_00198660` 13,
  `clip_output_00168320` 469, `ik_hand_0017a080` 14,
  `foretwist_00175678` 19, `uppertwist_001823c8` 50, and
  `hair_update_00176fb8` 6.
- Important limitation: successful Surrender/classic traces did not yet hit
  `hand_cmd_dispatch_00173b98` or `hand_cmd_sched_00173d20`. This is not proof
  that classic lacks hand commands; it only means the accepted captured windows
  did not exercise that route. Trace longer or deeper active gameplay before
  closing classic hand command semantics.
- Do not use `pcsx2_alt_song_driver_rows_20260611.json` as role proof. It
  sampled a separate PCSX2 process after allocation shifted. It is useful only
  as visual/context evidence.

Accepted late Battle-of-the-Bands arm/hand command trace:

- Report: `pcsx2_battle_autoplay_handcmd_late_20260611.json`.
- Screenshot: `pcsx2_battle_autoplay_handcmd_late_20260611.window.png`.
- Screenshot gate: accepted only as active venue/performance evidence. It is
  not accepted as note-hit/autoplay proof because the captured frame has an
  occluding camera/render state and no visible highway/note-hit state.
- Command class: state `1`, real ISO, GUI, EE recompiler disabled for function
  patching, 210 wall-clock seconds, 65,536-record ring, same-process
  `--sample-a0`/follow rows, with three attempted `Right+Square` chords before
  patching. Those chords are not proven to have enabled autoplay.
- Counts: retained all `13,761` calls with no ring wrap:
  - `chardriver_update_00171830`: `1,096`
  - `hand_cmd_dispatch_00173b98`: `20`
  - `hand_cmd_sched_00173d20`: `20`
  - `command_refresh_00173e18`: `0`
  - `scheduler_push_00171248`: `23`
  - `blend_entry_00198660`: `31`
  - `blend_tick_00199000`: `40`
  - `clip_eval_0016b1d0`: `1,240`
  - `clip_apply_0016b2f0`: `2,098`
  - `clip_output_00168320`: `5,673`
  - `clip_final_0016ab88`: `2,098`
  - `ik_hand_0017a080`: `158`
  - `ik_child_0017a558`: `158`
  - `foretwist_00175678`: `237`
  - `uppertwist_001823c8`: `632`
  - `hair_update_00176fb8`: `79`
  - `lookat_0017d658`: `158`
- Hand command rows recurred much later than the short traces and stayed on
  the same left/right glam1 owner rows:
  - `0x00173b98(stack=0x01ffe6e0, owner=0x00dbca20, event=0x00850c80, 0)`
    followed by `0x00173d20(stack=0x01ffe640, owner=0x00dbca20,
    event=0x00850c80, 0)`.
  - `0x00173b98(... owner=0x00dbc980 ...)` followed by
    `0x00173d20(... owner=0x00dbc980 ...)`.
  - Later hits appear around retained indexes `7616`, `8681`, `9218`, `9930`,
    `10106`, `10280`, and `11900`, so this is stronger sustained-window
    hand-command evidence than the short traces.
- Same-process distributions stayed stable:
  - IK: `0x00dbfa40` and `0x00dbf4f0`, `79` calls each.
  - Foretwist: `0x00d1f4d0`, `0x00dbdf80`, and `0x0135cfa0`, `79` calls each.
  - Uppertwist: `0x00c0b6c0`, `0x00ce2c60`, `0x00dbf620`, `0x00d9e830`,
    `0x010d8b30`, `0x010dae10`, `0x012e8750`, and `0x0135cf40`, `79` calls
    each.
- Interpretation: this strengthens the active-song glam1 arm/hand command
  chain and post-clip IK/twist cadence. It does not prove autoplay, does not
  prove successful note hits, and does not close classic/Surrender hand-command
  coverage.

Rejected autoplay-enabler probes:

- Extracted PS2 DTBs using local tools into `analysis/ps2_trace/dtb_cheats/`:
  `cheats_funcs.dtb`, `cheats.dtb`, and `long_cheats.dtb`.
- DTB source evidence:
  - `cheats_funcs.dtb.txt` defines `toggle_auto_play` as
    `cycle_multiplayer_auto_play`.
  - `cycle_multiplayer_auto_play` cycles states `(off, player1, player2, all)`
    by setting player config `autoplay` and calling
    `player_matcher0/1 set_auto_play`.
  - `cheats.dtb.txt` maps keyboard `p` to `toggle_auto_play` and maps
    controller block `right / kPad_Square` to `toggle_auto_play`.
- Visual proof failed for both activation attempts:
  - `pcsx2_autoplay_visual_90s_20260611.*`: three `Right+Square` chords, then
    90 normal-speed seconds. Before screenshot shows visible notes on the
    highway; after screenshot shows `Song Failed`, `14% Complete`.
  - `pcsx2_autoplay_keyp_visual_90s_20260611.*`: three keyboard `p` posts,
    then 90 normal-speed seconds. Before screenshot shows gameplay/highway;
    after screenshot again shows `Song Failed`, `14% Complete`.
- Conclusion: the autoplay DTB functions exist, but posted PCSX2 inputs did not
  activate them in the current retail PS2 run. Do not use these probes as
  proof of autoplay or successful note-hit animation. Future options are to
  trace the cheat input dispatcher, call the DTB function through the engine,
  or hook the hit/fail path explicitly after locating it from PS2 evidence.

Accepted IK/twist setup-slot active-window trace:

- Report: `pcsx2_ik_twist_setup_slots_sequence_20260611.json`.
- Log: `pcsx2_ik_twist_setup_slots_sequence_20260611.log`.
- Command class: headless `trace_pcsx2_call_sequence.py` from state `1` for
  16 seconds, 65,536-record ring, EE recompiler disabled, no screenshot gate.
- Counts: retained all `26,662` calls:
  - `ik_update_0017a080`: 20
  - `ik_prepass_0017a558`: 20
  - `foretwist_00175678`: 30
  - `uppertwist_001823c8`: 80
  - `shared_setup_001d2ab0`: 0
  - `shared_setup_001d2c48`: 0
  - `shared_setup_001d2e70`: 0
  - `trans_dirty_001dd748`: 9,621
  - `trans_world_003d8ea0`: 16,891
- Interpretation: the steady active-song IK/twist update loop is hot while
  these setup/shared slots are absent. Earlier broad traces saw
  `0x001d2ab0` and `0x001d2c48`, so keep them mapped as phase/setup paths
  rather than per-frame arm deformation work in this state.

Accepted metal-bass right-hand child-row follow-up:

- Reports:
  - `pcsx2_bass_right_hand_candidates_20260611.json`
  - `pcsx2_bass_right_hand_child_rows_20260611.json`
- Screenshots:
  - `pcsx2_bass_right_hand_candidates_20260611.before_sample.window.png`
  - `pcsx2_bass_right_hand_candidates_20260611.window.png`
  - `pcsx2_bass_right_hand_child_rows_20260611.before_sample.window.png`
  - `pcsx2_bass_right_hand_child_rows_20260611.window.png`
- Screenshot gate: all four screenshots are accepted active Battle of the
  Bands gameplay frames.
- Commands:
  - `python "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\tools\sample_pcsx2_object_words.py" --pcsx2 "C:\Games\Emulators\PCSX2\pcsx2-qt.exe" --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --retry-pulses 2 --pre-retry-seconds 4 --post-retry-seconds 3 --seconds 10 --interval 0.20 --require-screenshot --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_bass_right_hand_candidates_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_bass_right_hand_candidates_20260611.json" --no-default-targets --target bass_r_hand_desc_a_01140b74=0x01140b74:0x100 --target bass_r_hand_desc_b_011415f8=0x011415f8:0x100 --target bass_r_hand_trans_desc_a_01157664=0x01157664:0x100 --target bass_r_hand_trans_desc_b_01158c90=0x01158c90:0x100 --target bass_r_hand_channel_out=0x01168e68:0x100 --target bass_r_hand_neighborhood_a=0x011414e0:0x300 --target bass_r_hand_neighborhood_b=0x01157000:0x300 --target bass_r_hand_neighborhood_c=0x01158c00:0x300`
  - `python "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\tools\sample_pcsx2_object_words.py" --pcsx2 "C:\Games\Emulators\PCSX2\pcsx2-qt.exe" --iso "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso" --state 1 --gui --retry-pulses 2 --pre-retry-seconds 4 --post-retry-seconds 3 --seconds 10 --interval 0.20 --require-screenshot --log "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_bass_right_hand_child_rows_20260611.log" --out "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX-trace360\analysis\ps2_trace\pcsx2_bass_right_hand_child_rows_20260611.json" --no-default-targets --target bass_r_hand_mesh_obj=0x01140b74:0x180 --target bass_r_hand_child_a=0x01140c00:0x180 --target bass_r_hand_child_b=0x01140c28:0x180 --target bass_r_hand_child_c=0x011993d0:0x180 --target bass_r_hand_child_d=0x0119a410:0x180 --target bass_r_hand_trans_obj=0x01157664:0x180 --target bass_r_hand_trans_child_a=0x01157650:0x180 --target bass_r_hand_trans_child_b=0x01143640:0x180 --target bass_r_hand_trans_child_c=0x01142e40:0x180 --target bass_r_hand_trans_child_d=0x01142d40:0x180 --target bass_r_hand_trans_child_e=0x01142840:0x180`
- Results:
  - `bone_R-hand.mesh` candidate `0x01140b74`, descriptor/list row
    `0x011415f8`, `bone_R-hand.trans` rows `0x01157664` and `0x01158c90`,
    and channel-name row `0x01168e68` stayed stable.
  - `0x01140b74` points to owner/source `0x00b8df40` and child/output refs
    `0x01140c00`, `0x01140c28`, `0x011993d0`, and `0x0119a410`, which also
    stayed stable in this slice.
  - `bone_R-hand.trans` exposed child/output refs `0x01143640`,
    `0x01142e40`, `0x01142d40`, and `0x01142840`; these changed 38, 38, 43,
    and 30 rows respectively.
- Interpretation: the metal-bass right-hand candidate stable rows are
  descriptor/structural rows. The live right-hand motion is in the `.trans`
  child/output rows. This matches the broader rule for arms, legs, hair, and
  twist: stable headers are not inactivity.

Accepted GH2DXu direct-route alterna1 hand/twist argument samples:

- Trace helper update: `tools/trace_pcsx2_call_sequence.py` now supports
  `--sample-arg target:a0|a1|a2|a3:size`, using the already-recorded ring
  arguments. This is trace tooling only; no native animation/runtime code was
  changed.
- Route: explicit PS2 Deluxe direct boot through `tattooedloveboys`, venue
  `small1`, character `alterna1`, guitar `sg`, expert, `$dx_auto_play TRUE`,
  and `$first_screen loading_screen`. This route was staged into a temporary
  ISO named `GH2DXu_PS2_trace_alterna1_hand_owner.iso` and cleaned up after
  parsing; no `GH2DXu_PS2_trace*` ISO/folder remains in the repo root.
- Hand owner sample report:
  `gh2dxu_alterna1_tattooed_hand_owner_sample_20260611.json`.
  Log: `gh2dxu_alterna1_tattooed_hand_owner_sample_20260611.log`.
  Window PNG: `gh2dxu_alterna1_tattooed_hand_owner_sample_20260611.window.png`.
  Counts: `26,085` total calls; `chardriver_update_00171868` 102,
  `hand_cmd_dispatch_00173bd0` 12, `hand_cmd_sched_00173d58` 12,
  `scheduler_push_00171280` 19, `blend_entry_001986a0` 28,
  `blend_tick_00199040` 28, `clip_eval_0016b208` 112,
  `clip_output_00168358` 565, `clip_final_0016abc0` 200,
  `ik_hand_0017a0b8` 16, `ik_child_0017a590` 16.
- Hand argument sample report:
  `gh2dxu_alterna1_tattooed_hand_arg_sample_20260611.json`.
  Log: `gh2dxu_alterna1_tattooed_hand_arg_sample_20260611.log`.
  Window PNG: `gh2dxu_alterna1_tattooed_hand_arg_sample_20260611.window.png`.
  Counts: `10,223` total calls; `hand_cmd_dispatch_00173bd0` 229,
  `hand_cmd_sched_00173d58` 229, `scheduler_push_00171280` 227,
  `blend_entry_001986a0` 273, `blend_tick_00199040` 275,
  `ik_hand_0017a0b8` 1,288, `ik_child_0017a590` 1,288.
- Direct alterna1 closure: `hand_cmd_dispatch_00173bd0` and
  `hand_cmd_sched_00173d58` take `a1=0x00eebbf0` for `left_hand.drv` and
  `a1=0x00eebb50` for `right_hand.drv`; both resolve through followed rows to
  `char/alterna1/og/alterna1.milo`. Their shared `a2=0x008acca0` is the live
  command/event payload for that hand event.
- The immediate hand burst order is:
  `chardriver_update` for the performer and left/right hand drivers,
  `ik_hand/ik_child` for left and right hands, then
  `hand_cmd_dispatch -> hand_cmd_sched -> scheduler_push -> blend_entry ->
  blend_tick`, once for left and once for right. Example left burst:
  `dispatch(stack, 0x00eebbf0, 0x008acca0, 0)` ->
  `sched(stack, 0x00eebbf0, 0x008acca0, 0)` ->
  `scheduler_push(0x00eebbfc, 0x00f9c2c0, 0, ...)` ->
  `blend_entry(0x008a0e20, 0x00cb4a30, 0x00f9c2c0, 0)`.
  The right side repeats with `0x00eebb50`, `0x00eebb5c`, and `0x008a0d20`.
- `scheduler_push` / `blend_entry` / `blend_tick` argument sampling shows
  scripted `{set_hand ...}` objects feeding character owners. Same-window owners
  include alterna1, female singer, metal_bass, metal_drummer, and crowd rows;
  native code must filter by owner/source and not route all hand commands to
  the player guitarist.
- Twist argument sample report:
  `gh2dxu_alterna1_tattooed_twist_arg_sample_20260611.json`.
  Log: `gh2dxu_alterna1_tattooed_twist_arg_sample_20260611.log`.
  Window PNG: `gh2dxu_alterna1_tattooed_twist_arg_sample_20260611.window.png`.
  Counts: `11,368` total calls; `clip_eval_0016b208` 1,136,
  `clip_output_00168358` 5,704, `clip_final_0016abc0` 2,027,
  `ik_hand_0017a0b8` 210, `ik_child_0017a590` 210,
  `foretwist_001756b0` 315, `uppertwist_00182400` 840.
- Alterna1 twist rows are now directly sampled in the same route:
  `foreTwist_L.ik` at `0x00eed570`, `foreTwist_R.ik` at `0x00eee780`,
  `upperTwist_L.ik` at `0x00e820b0`, and `upperTwist_R.ik` at
  `0x00eedac0`, all resolving through owner/source rows to
  `char/alterna1/og/alterna1.milo`. Upper-twist follows reach
  `bone_L-upperArm.mesh` and `bone_R-upperArm.mesh`; hand IK follows reach
  `bone_L-hand.mesh` and `bone_R-hand.mesh`.
- Interpretation: the old alterna1 short-window zero-hit hand dispatch gap is
  closed for this direct PS2 route. Alterna1 uses the common hand command,
  scheduler/blend, clip, IK, foretwist, and uppertwist path. Remaining
  arms/hands uncertainty is exact field naming/math and the retail DTB
  cheat/input hit-path, not whether alterna1 participates in the hand
  scheduler.

## Documentation Rule

Every accepted trace must record:

- command used
- tool used
- output JSON path
- screenshot path
- whether the screenshot is accepted or rejected
- exact nonzero function/table/slot counts
- object addresses and layouts learned
- what the result proves
- what it does not prove

Append those findings to this file or to
`GuitarHeroOGX/engine/src/game/GAMEPLAY_RE_NOTES.md` before using them to
justify native work.

## 2026-06-12 GH2 Rejected Female-Singer Route And GH1 Start

Rejected GH2DXu `lesstalkmorerokk` female-singer route:

- Report: `gh2dxu_lesstalk_female_face_twist_sample_20260612.json`.
- Log: `gh2dxu_lesstalk_female_face_twist_sample_20260612.log`.
- Screenshot: `gh2dxu_lesstalk_female_face_twist_sample_20260612.window.png`.
- Command shape: direct GH2DXu trace-only boot with `set_song_index 54`,
  `set_song lesstalkmorerokk`, `set_venue small2`, `set_character goth2`,
  `set_guitar lespaul`, expert, `$dx_auto_play TRUE`, and
  `$first_screen loading_screen`.
- Targets included the GHDX-remapped character driver, foretwist, upper-twist,
  hair, look-at, vector/math, and Trans helpers.
- Result: rejected. Counts were zero for every target and the screenshot is a
  gray PCSX2 render surface, not active gameplay. Do not use this JSON as
  female-singer evidence.
- Cleanup: `GH2DXu_PS2_trace_lesstalk_female.iso` and
  `GH2DXu_PS2_trace_lesstalk_female_disc` were deleted immediately afterward.
- Bootstrap restoration: `_ark/ui/init.dta` was restored to the last accepted
  direct `tattooedloveboys` / `small1` / `alterna1` / `sg` route so the next
  GH2DXu rebuild does not inherit the rejected lesstalk path.

Stock GH1 trace setup has started:

- Disc: `C:\Programming\GitHub\Guitar Hero II\Guitar Hero (USA).iso`.
- Extracted executable: `C:\Programming\GitHub\Guitar Hero II\Guitar Hero (USA)\SLUS_212.24`.
- GH1 executable MD5 observed earlier in this run:
  `1D0E93F3FB0A8CB8160DD047D67AB7CC`.
- Static Rosetta artifact:
  `gh1_static_rosetta_probe_20260612.json`.
- Static method: masked instruction-body scan from traced GH2 PS2 functions
  into the GH1 SLUS. This is not runtime proof; it is only an address-candidate
  filter.
- Unique static matches so far:
  - GH2 scheduler/list helper `0x00171db0` -> GH1 `0x00180670`.
  - GH2 look-at/vector helper `0x002da768` -> GH1 `0x0024ac28`.
- Non-matches are important: the main GH2 character, hand, IK, twist, hair,
  camera, lighting, and world-event addresses cannot be blindly reused for GH1.
  GH1 needs its own active-song runtime trace addresses.

Accepted stock GH1 boot/setup probes:

- `gh1_stock_boot_nav_probe_20260612.json`: reached the stock save prompt but
  is rejected as a gameplay route because `Keyboard/Space` also toggled PCSX2
  pause in the baseline config.
- `gh1_stock_menu_nav_probe2_20260612.json`: proved posted `Keyboard/L`
  Green/Cross style input reaches GH1 without forcing PCSX2 foreground, but it
  entered the band-name flow.
- `gh1_stock_menu_nav_probe3_startfix_20260612.json`: temporarily changed
  `TogglePause` to `Keyboard/P` and restored it in `finally`; timing still
  pulsed Start before the name screen.
- `gh1_stock_menu_nav_probe4_timed_20260612.json`: corrected the timing but
  proved `Keyboard/Space` Start was still not a reliable posted-input route.
- `gh1_stock_menu_nav_probe5_startN_20260612.json` and
  `gh1_stock_start_return_probe_20260612.json`: temporarily rebound pad Start
  to `Keyboard/N` and `Keyboard/Return`, respectively, restored config in
  `finally`, and reached the guitar-controller help card from the band-name
  path. Both later returned to the name editor, so neither is accepted as an
  active-song entry route.
- PCSX2 config was verified restored afterward:
  `TogglePause = Keyboard/Space` and `Start = Keyboard/Space`.
- No PCSX2 or ImgBurn process remained after the probes.

Next GH1 work:

- Do not trace GH1 animation from the name editor, title screen, or guitar-help
  card.
- Either create a stable stock GH1 active-song savestate or solve the
  band-name/setup accept path without forcing PCSX2 to the foreground.
- After an accepted active-song screenshot exists, map GH1-specific animation
  addresses from live code/object evidence instead of using GH2 addresses.

Follow-up GH1 progress from the same session:

- Accepted stock GH1 cold-route active-song probe:
  `gh1_stock_song_entry_probe_20260612.json`.
- Key screenshot:
  `gh1_stock_song_entry_probe_20260612.cross_confirm1.window.png`, showing
  active `I Love Rock & Roll` gameplay in `34 Winship St Basement` with full
  venue, band, note highway, HUD, lighting, and camera.
- Later screenshots in the same probe show continued active camera movement
  and then the expected fail screen because autoplay is not active.
- Route learned from cold boot:
  wait for title, Green/Cross through title, enter a non-empty band name with
  repeated Green/Cross, press Start via a temporary `Keyboard/Return` binding,
  dismiss guitar help, select Quick Play, choose the first setlist song, choose
  difficulty, then confirm into gameplay.
- Temporary input edits used during setup probes were restored after each run:
  `TogglePause = Keyboard/Space` and `Start = Keyboard/Space`.
- A GH1 savestate now exists:
  `C:\Games\Emulators\PCSX2\sstates\SLUS-21224 (B815F724).01.p2s`.
  It is useful for menu reduction, but do not assume it is active-song; current
  probes found it around setlist/guitar/band-card flow depending on the route.

Rejected or non-evidentiary GH1 trace attempts:

- `gh1_stock_first_runtime_trace_20260612.json` and
  `gh1_stock_first_runtime_trace_slow_20260612.json`: zero calls and rejected
  screenshots on black/menu transition, not active gameplay.
- `gh1_stock_state1_first_runtime_trace_20260612.json`: zero calls and
  screenshot at guitar select, not active gameplay.
- `gh1_stock_state1_live_route_trace_20260612.json` and
  `gh1_stock_state1_live_route_trace_long_20260612.json`: zero calls and
  screenshots at the venue-ticket card, not active gameplay.
- `gh1_stock_state1_live_route_trace_fast_20260612.json`: rejected because
  patching without disabling the EE recompiler failed with
  `WriteProcessMemory failed ... error 998`.
- `gh1_stock_state1_active_trace_20260612.json`: zero calls and screenshot at
  the setlist, so the state route was still not active-song under the trace
  helper.
- Interpretation: GH1 active playback is proven, but the first two static
  Rosetta matches are not enough to trace the animation pipeline. The next GH1
  trace should first stabilize an active-song state route or add a helper that
  can save/load after the proven cold route reaches gameplay, then identify
  GH1-specific hot functions from live object/vtable evidence.

Accepted GH1 active-state bootstrap and first runtime animation traces:

- Corrected state route from the GH1 band card: the previous state starts with
  `CHANGE GUITAR` selected. One `Keyboard/W` moves to `PLAY LIVE SHOW`; two
  moves overshoot to `CHANGE CHARACTER`. The accepted route is one `W`, then
  Green/Cross (`Keyboard/L`) through the venue ticket, loading/help card, song
  start, and first gameplay frames.
- New accepted active savestate:
  `C:\Games\Emulators\PCSX2\sstates\SLUS-21224 (B815F724).01.p2s`, copied to
  `gh1_active_song_state_20260612.p2s`. Screenshot
  `gh1_state1_active_savestate_seed_20260612.save_state_f1.window.png` shows
  active `I Love Rock & Roll` playback with Judy, drummer, HUD, highway, venue
  lighting, and camera live.
- Accepted live string/reference scan:
  `gh1_active_string_scan_20260612.json`, screenshot
  `gh1_active_string_scan_20260612.window.png`. It found GH1 live controller
  names and refs during active gameplay: `main.drv`, `left_hand.drv`,
  `right_hand.drv`, `left_hand.ik`, `right_hand.ik`, twist bone rows,
  `female_singer`, `bassist`, and `drummer`.
- Accepted live object/table sample:
  `gh1_active_controller_ref_sample_20260612.json`, screenshot
  `gh1_active_controller_ref_sample_20260612.window.png`. Important GH1 table
  pointers from live objects include `0x002f8ef8` for `main.drv`-style rows,
  `0x002f90e0` for IK rows, and `0x002f9090` for twist/servo rows. These
  replace the GH2 `0x003e....` table range assumptions for GH1.
- Accepted table dump:
  `gh1_active_table_dump_20260612.json`, screenshot
  `gh1_active_table_dump_20260612.window.png`. Key nonzero GH1 candidate slots
  include `0x0017ff18`, `0x00180440`, `0x0017fea8`, `0x00181f18`,
  `0x00183a20`, `0x00184198`, and the previously static-matched
  `0x00180670` / `0x0024ac28`.
- First accepted GH1 runtime table-slot trace:
  `gh1_active_table_slot_trace_20260612.json`, screenshot
  `gh1_active_table_slot_trace_20260612.window.png`. It recorded 1,936 calls
  from active state. Nonzero targets: `0x0017ff18` 639, `0x00180440` 513,
  `0x0017fea8` 513, `0x00181f18` 20, `0x00180670` 12, `0x00183a20` 98,
  `0x00184198` 98, and `0x0024ac28` 43.
- Stronger accepted GH1 nonzero-slot sample:
  `gh1_active_nonzero_slot_sample_20260612.json`, screenshot
  `gh1_active_nonzero_slot_sample_20260612.window.png`, active gameplay with
  Judy, drummer, HUD, highway, and lighting visible. It recorded 12,643 calls:
  `0x0017ff18` 3,984, `0x00180440` 3,253, `0x0017fea8` 3,253,
  `0x00181f18` 25, `0x00180670` 24, `0x00183a20` 926, `0x00184198` 926,
  and `0x0024ac28` 252.
- Ownership from the accepted nonzero-slot sample:
  `0x0017ff18`, `0x00180440`, and `0x0017fea8` tick named `main.drv`,
  `left_hand.drv`, and `right_hand.drv` rows across guitarist, singer,
  bassist, drummer, and crowd/hand sub-objects. `0x00180670` samples
  performer `main.drv` rows, including singer and drummer rows.
  `0x00183a20` ticks `foreTwist_L.servo` and `foreTwist_R.servo`, both
  following `+0x08` to `guitarist0`. `0x00184198` ticks `right_hand.ik` and
  `left_hand.ik`, also following `+0x08` to `guitarist0`.
- Accepted GH1 secondary-table probe:
  `gh1_active_secondary_table_trace_20260612.json`, screenshot
  `gh1_active_secondary_table_trace_20260612.window.png`, active gameplay with
  Judy and HUD visible. It traced live secondary table candidates from
  `0x002faf48` and `0x002fa540`. Most candidate slots stayed zero in this
  active window, but three routines fired: `0x001fb3b0` 1,858,
  `0x001cfa88` 5,616, and `0x001cfae8` 1,872.
- Secondary ownership interpretation is provisional: `0x001fb3b0` samples
  guitarist/face rows and follows through `+0x1c` to
  `venues/basement/basement.dtb`; `0x001cfa88` and `0x001cfae8` sample
  high-frequency rows with venue/blend-adjacent follows. Treat these as
  transform/venue-object candidates until paired with named Trans and clip
  rows; do not call them clip output yet.
- Accepted GH1 arm/hand argument trace:
  `gh1_active_arm_hand_arg_trace_20260612.json`, log
  `gh1_active_arm_hand_arg_trace_20260612.log`, screenshot
  `gh1_active_arm_hand_arg_trace_20260612.window.png`. The screenshot is
  active `I Love Rock & Roll` gameplay with Judy visible on guitar, the note
  highway/HUD live, and interpreter slowdown expected from patch tracing.
  Exact command shape: stock GH1 ISO and `SLUS_212.24`, `--state 1`, `--gui`,
  `--require-screenshot`, `--disable-ee-recompiler`,
  `--retry-pulses 0`, `--seconds 32`, `--ring-size 65536`,
  `--stub-base 0x01c00000`, `--data-base 0x01d00000`, targets
  `0x0017ff18`, `0x00180440`, `0x0017fea8`, `0x00181f18`,
  `0x00180670`, `0x00183a20`, and `0x00184198`, with `--sample-a0`,
  `--sample-arg`, and follows at `0x08`, `0x0c`, `0x1c`, and `0x6c`.
- Arm/hand counts from that accepted trace: `39,590` total calls;
  `0x0017ff18` 12,744, `0x00180440` 11,293, `0x0017fea8` 11,293,
  `0x00181f18` 120, `0x00180670` 48, `0x00183a20` 2,046, and
  `0x00184198` 2,046.
- Arm/hand order and ownership from the same trace:
  each driver cycle enters `0x0017ff18`, then `0x0017fea8`, then
  `0x00180440` for the same row. Early active rows are Judy `main.drv`
  `0x00bb6aa0`, `left_hand.drv` `0x00bcb990`, `right_hand.drv`
  `0x00bcb9d0`, then hand IK rows `right_hand.ik` `0x00bcba10` and
  `left_hand.ik` `0x00bcbab0`, followed by `foreTwist_L.servo`
  `0x00bb6ae0` and `foreTwist_R.servo` `0x00bcb900`. Hand IK and foretwist
  rows follow `+0x08` to `guitarist0`.
- Driver source rows from the same trace:
  Judy `main.drv` follows `+0x0c` to `alterna.cset`; Judy hand drivers follow
  to `alterna_hand.cset`; singer, bass, and drummer rows follow to
  `singer.cset`, `bass.cset`, and `drummer.cset`. Crowd/hand sub-rows also
  tick through the same driver table, so do not treat all `main.drv` hits as
  stage performers.
- Static GH1 snippet support:
  `gh1_arm_hand_function_snippets_20260612.json` shows `0x0017fea8` as a small
  virtual dispatch through the row's `+0x2c` child/controller pointer,
  `0x00180670` walking that same child chain while checking float state,
  `0x00183a20` pulling transform rows from its foretwist servo controller, and
  `0x00184198` as the hand IK update routine. This is support evidence only;
  the accepted runtime trace above is the proof source.
- Accepted GH1 hair/eye/face resource scan:
  `gh1_active_hair_eye_string_scan_20260612.json` plus focused ref sample
  `gh1_active_hair_eye_ref_sample_20260612.json`, log
  `gh1_active_hair_eye_ref_sample_20260612.log`, screenshot
  `gh1_active_hair_eye_ref_sample_20260612.window.png`. The screenshot is
  active gameplay with Judy in-frame and no input pulses sent by the sampler.
- GH1 hair/eye interpretation from that accepted sample:
  exact GH2-style names such as `CharHair`, `CharLookAt`, `hair.hair`,
  `l-eye.lookat`, and `r-eye.lookat` were not found in this GH1 active state.
  Live-ish `eye`/`blink`/`lash`/`face` hits resolve mostly to resources:
  `eye.tex`, `eyes.mat`, `eye.mesh`, `blink.mesh`, `blink.2.mesh`,
  `lashes.mesh`, `lashes.mrf`, `face.mesh`, and `face.mrf`. The `eyes` refs
  at `0x007467e0` and `0x00748810` are `venues/basement/camera.dtb` script
  properties, not character look-at controller rows.
- GH1 attachment/resource caveat:
  the sample also ties `face.mrf` into Judy's `main.drv` neighborhood at
  `0x00bb6a4c` and female singer face rows around `0x00c144d8` /
  `0x00c14c68`, while `ponytail*.mesh` strings live in the metal-bass mesh
  neighborhood. Treat these as loaded mesh/material attachment evidence, not
  as completed hair/eye controller semantics. Hair/eyes still need controller
  or transform-output tracing if GH1 has a separate runtime path.
- GH1 `.cset` table sample:
  `gh1_active_cset_table_sample_20260612.json` ties the driver `+0x0c`
  sources to real active-song animation-set rows. `alterna.cset` at
  `0x00bcc530` uses `charsys/alterna/anims`; `alterna_hand.cset` at
  `0x00bcc5b0` uses `charsys/alterna/anims/finger`; `singer.cset` at
  `0x00c14d50` uses `charsys/female_singer/anims`; `bass.cset` at
  `0x00c16310` uses `charsys/metal_bass/anims`; `drummer.cset` at
  `0x00c178a0` uses `charsys/metal_drummer/anims`; `crowd.cset` at
  `0x00c21aa0` uses `charsys/crowd/anims`. All point at table `0x002f8ea8`.
- GH1 `.cset` target slots:
  the table entries immediately before the already-proven driver trio are
  `0x002ad8b0`, `0x0017ddf0`, `0x0017f660`, `0x002ddef0`,
  `0x002addc0`, `0x0017fae0`, and `0x00180740`. Trace these from the accepted
  active state next, with `0x0017ff18`, `0x00180440`, and `0x0017fea8` as
  comparison anchors. This is the current best GH1 bridge toward clip/blend
  selection and hand/finger animation-set playback.
- Next GH1 work: expand from these proven table slots to GH1 clip
  sample/apply/output, blend/weight, hair/eyes/look-at controller objects, and
  Trans dirty/world helpers. Do not reuse GH2 addresses where GH1 table/live
  evidence has not mapped them yet.

Accepted GH2DXu YYZ keyboardist coverage:

- Route: clean GH2DXu trace-only direct boot through `yyz`, venue `theatre`,
  player character `funk1`, guitar `lespaul`, expert, `$dx_auto_play TRUE`.
  This route reaches active autoplay gameplay under normal EE recompiler.
- Accepted state seed:
  `gh2dxu_yyz_keyboard_savestate_seed_20260612.window.png` shows active YYZ
  gameplay in the theatre venue. Saving that state wrote
  `C:\Games\Emulators\PCSX2\sstates\GHDX-00300 (A9BBA52A).01.p2s` at
  `2026-06-12 09:59:06`.
- Accepted trace:
  `gh2dxu_yyz_keyboardist_state_trace_20260612.json`.
  Log: `gh2dxu_yyz_keyboardist_state_trace_20260612.log`.
  Screenshot: `gh2dxu_yyz_keyboardist_state_trace_20260612.window.png`, active
  venue/playback with the player guitarist and drummer visible.
- Command shape: load the accepted GHDX state with `--state 1`,
  `--disable-ee-recompiler`, `--pre-retry-seconds 3`, standard GHDX remapped
  character targets, `--stub-base 0x01c00000`, and
  `--data-base 0x01d00000`. This avoids the cold interpreter gray-screen path
  while preserving patchable EE code pages.
- Counts: `21,665` total calls; `chardriver_update_00171868` 77,
  `clip_eval_0016b208` 84, `clip_output_00168358` 429,
  `clip_final_0016abc0` 148, `ik_hand_0017a0b8` 14,
  `ik_child_0017a590` 14, `foretwist_001756b0` 21,
  `uppertwist_00182400` 56, `hair_update_00176ff0` 21,
  `lookat_update_0017d690` 14, `trans_dirty_001dd788` 6,860,
  `trans_world_003d7220` 13,927. Hand dispatch/scheduler/blend targets stayed
  zero in this YYZ window.
- Keyboardist source rows: `chardriver_update_00171868:a0` sampled
  `0x00e292d0`, naming `main.drv`; follow offsets `0x1c` and `0x6c` resolve
  to `char/metal_keyboard/og/metal_keyboard.milo`.
  `uppertwist_00182400:a0` sampled `0x00e29350` / `upperTwist_L.ik` and
  `0x00e296a0` / `upperTwist_R.ik`; both resolve through owner rows to
  `char/metal_keyboard/og/metal_keyboard.milo`. The left upper-twist follow
  also reaches `bone.servo`.
- Negative evidence: in this accepted YYZ window, keyboardist-owned hand IK,
  foretwist, hair, and look-at rows did not fire. The live hand/IK/foretwist,
  hair, and look-at rows in the same trace resolve to the player `funk1` and
  other band/crowd owners, not `metal_keyboard`. Treat this as route/window
  evidence, not as proof that the format lacks those controller classes.
- Rejected setup attempts, do not repeat as proof: cold direct YYZ with
  `--disable-ee-recompiler` stayed on gray render even after a long wait;
  normal recompiler reached gameplay but rejected live code writes with
  `WriteProcessMemory` error 998; broad and single-hook prepatched code-cave
  attempts stayed on the loading screen because the candidate cave/scratch path
  was not neutral during load. The accepted route is normal-recompiler active
  savestate first, then interpreter trace from that state.

Accepted GH2DXu female-singer closure trace:

- Route: clean GH2DXu trace-only direct boot through `tattooedloveboys`, venue
  `small1`, player character `alterna1`, guitar `sg`, expert,
  `$dx_auto_play TRUE`. Song metadata band is
  `metal_bass metal_drummer female_singer`.
- State seed:
  `gh2dxu_female_closure_savestate_seed_20260612.window.png` shows active
  venue playback with the female singer visible on stage.
- Accepted trace: `gh2dxu_female_closure_state_trace_20260612.json`.
  Log: `gh2dxu_female_closure_state_trace_20260612.log`.
  Screenshot: `gh2dxu_female_closure_state_trace_20260612.window.png`, active
  small1 venue playback with the female singer visible at camera left.
- Counts: `15,562` total calls; `chardriver_update_00171868` 50,
  `clip_eval_0016b208` 52, `clip_output_00168358` 239,
  `clip_final_0016abc0` 82, `ik_hand_0017a0b8` 4,
  `ik_child_0017a590` 4, `foretwist_001756b0` 8,
  `uppertwist_00182400` 28, `hair_update_00176ff0` 6,
  `lookat_update_0017d690` 4, `trans_dirty_001dd788` 5,472,
  `trans_world_003d7220` 9,613. Hand dispatch/scheduler/blend stayed zero in
  this window.
- Female source rows: `chardriver_update_00171868:a0` sampled `0x00e44f00`,
  naming `main.drv`; follow offsets `0x1c` and `0x6c` resolve to
  `char/female_singer/og/female_singer.milo`. `uppertwist_00182400:a0`
  sampled `0x00e44f80` / `upperTwist_L.ik` and `0x00e455d0` /
  `upperTwist_R.ik`, both resolving to the same female-singer owner.
  `hair_update_00176ff0:a0` sampled `0x00e45260` / `dreads.hair`, resolving to
  `char/female_singer/og/female_singer.milo`, and follows to
  `char/female_singer/anims/female_viseme.milo`.
- Female look-at/foretwist closure: live `lookat_update_0017d690` rows in this
  trace resolve to `alterna1` eye look-at controllers, not the female singer.
  Live `foretwist_001756b0` rows resolve to `alterna1` and
  `metal_drummer`, not the female singer. Static body inventory for
  `female_singer.list.txt` has `CharDriver`, `CharHair`,
  `CharUpperTwist` x2, `FaceFxLipSyncServo`, `CharServoBone`, and
  `CharPosConstraint`, but no `CharForeTwist`, `CharLookAt`, or `CharEyes`.
  Treat female singer as format-backed absent for foretwist/look-at in GH2,
  not as an unresolved runtime trigger.

Deferred GH2DXu goth3 cloth follow-up:

- User correction on 2026-06-12: the attempted `goth3` cloth route should not
  block the GH1 sweep. Treat the `goth3` cloth/secondary-motion pass as
  deferred until the GH80s/GH2DXu outfit context is intentionally in scope.
- A temporary `GH2DXu_PS2_trace_goth3_cloth.iso` and
  `GH2DXu_PS2_trace_goth3_cloth_disc` were created from the GH2DXu trace
  bootstrap, but no accepted trace was captured. The first bad launch was a
  rejected startup-error screenshot caused by mangled `-logfile`/ISO argument
  quoting; the clean relaunch was stopped by user direction before saving an
  accepted state or trace.
- Cleanup completed: PCSX2 was stopped and both the temporary ISO and staged
  folder were deleted. `_ark/ui/init.dta` was restored to the accepted
  `tattooedloveboys` / `small1` / `alterna1` / `sg` female-closure route.
- Latest GH1 downstream trace evidence:
  `gh1_downstream_cset_named_table_trace_20260612.json` used the accepted
  stock GH1 ISO/state-1 active song route with `--disable-ee-recompiler`,
  `--gui`, `--require-screenshot`, no retry pulses, and a 16 KiB ring after a
  larger scratch/ring layout exceeded 32 MiB EE RAM. Screenshot
  `gh1_downstream_cset_named_table_trace_20260612.window.png` is accepted
  active gameplay with performers, HUD, note highway, and no fail/menu state.
  Counts: `0x0017ff18` 5,631, `0x00180440` 5,088,
  `0x0017fea8` 5,088, `0x00182688` 544, and `0x00181f18` 33.
- `0x00182688` is now the GH1 `bone.servo` output/list walker candidate.
  Runtime samples include bass `bone.servo` row `0x00c16290`, source
  `0x00c150b0`, adjacent `bass.cset`, and `charsys/metal_bass/anims`; crowd
  rows `0x014df730` and `0x014df7b0` show the same class pattern. Following
  the row's `+0x44` reaches channel names including `bone_pelvis.pos`,
  `bone_L/R-ankle.quat`, `bone_L/R-clavicle.quat`,
  `bone_L/R-hand.quat`, `bone_L/R-thigh.quat`,
  `bone_L/R-upperArm.quat`, `bone_head.quat`, `bone_pelvis.quat`,
  `bone_spine.quat`, `bone_L/R-forearm.rotz`, `bone_L/R-knee.rotz`,
  `bone_base.rotz`, and `bone_neck.rotz`.
- Static support in
  `gh1_downstream_cset_named_function_snippets_20260612.json` shows
  `0x00182688` loading the source/root at `row+0x08`, reading floats from
  `source+0x84/+0x88/+0x8c`, iterating the list at `row+0x54`, and calling a
  per-entry callback from each entry's table at `table+0x0c` using the object
  offset at `lh(table+0x08)`. This is evidence for a channel/output apply
  walker, not a high-level clip selector by itself.
- `gh1_boneservo_00182688_object_sample_20260612.json` confirms the stable
  row layout without code writes: `bone.servo +0x00` name, `+0x04` class table,
  `+0x08` source/root performer, `+0x44` named channel/list pointer, and
  `+0x54` iterated output-entry list. The accepted screenshot
  `gh1_boneservo_00182688_object_sample_20260612.window.png` shows active
  gameplay at normal speed. Next GH1 work should sample the `+0x54` entry
  lists (`0x00716c40`, `0x007732d0`, `0x00777950`, and adjacent heads), then
  trace the callback functions found at each entry table's `+0x0c`.
- Follow-up accepted samples:
  `gh1_boneservo_entry_lists_sample_20260612.json`,
  `gh1_boneservo_entry_objects_sample_20260612.json`, and
  `gh1_boneservo_callback_tables_sample_20260612.json`. These prove
  `bone.servo+0x54` is a pointer/list structure whose first entries are
  driver-shaped rows, not raw quaternion arrays. Bass `+0x54` begins with
  `0x00c16250`, a `main.drv` row adjacent to bass `bone.servo`
  `0x00c16290`; crowd rows follow the same pattern.
- Class-table sample:
  `bone.servo` table `0x002f8ff0` has live update slot `+0x24 -> 0x00182688`
  and another live slot `+0x2c -> 0x00182730`. The driver table
  `0x002f8ef8` has `+0x0c -> 0x0017ff18`, so the `+0x54` list entries feed
  the normal GH1 driver begin path.
- `gh1_boneservo_table_function_trace_20260612.json` is accepted active
  gameplay evidence for the full exposed `bone.servo` table sweep. Counts:
  `0x00182688` 613, `0x00182730` 2,726, `0x0017ff18` 4,757,
  `0x00180440` 4,144, and `0x0017fea8` 4,144. Other traced `bone.servo`
  table slots (`0x00182488`, `0x00182d60`, `0x00182b48`, `0x00183170`,
  `0x001837e8`, `0x001832b8`, `0x001837c8`, `0x001834d8`, `0x001839c8`,
  `0x00183ed8`) were zero in this active window.
- `0x00182730` is the common GH1 stage/crowd `bone.servo` dispatch slot in the
  current state. Sampled owners include singer `0x00c14cd0`, bass
  `0x00c16290`, drummer `0x00c17820`, guitarist0/Judy `0x00bcb780`, and crowd
  rows. The frame-order pattern is explicit: `0x00182730` on a `bone.servo`
  row is immediately followed by `0x0017ff18` on that row's first `+0x54`
  driver entry, for example bass `0x00c16290 -> 0x00c16250`, drummer
  `0x00c17820 -> 0x00c177e0`, singer `0x00c14cd0 -> 0x00bd24a0`, and
  guitarist0 `0x00bcb780 -> 0x00bb6aa0`.
- Static support is in
  `gh1_boneservo_table_function_snippets_20260612.json`. `0x00182730` starts
  by comparing the `bone.servo+0x40/+0x44` range, while `0x00182688` is the
  smaller `+0x54` list walker described above. Do not collapse these two
  slots: both are live, but `0x00182730` is the broad per-frame performer
  dispatcher in this state.
- `0x00181f18` remains performer/root script dispatch evidence until proven
  otherwise. Its sampled `a1` rows resolve to `guitarist0`, `singer`, `bass`,
  and `drummer` roots through `charsys/charsys.dtb` and
  `charsys/band_chars.dtb`; its `a0` samples are scratch/stack-like rather
  than stable controller roots.
- GH1/GH80s autoplay is now tracked as trace infrastructure only. GH1 autoplay
  is still unaccepted: active-state field pokes and direct `set_auto_play`
  calls failed at 25%, and the early state-load call froze. The next useful
  path is to catch real `set_auto_play`/game-entry routing before active
  gameplay or trace the hit/fail path directly.
- GH1 driver/root table sweep:
  `gh1_driver_table_full_trace_20260612.json` is accepted active gameplay
  evidence for the exposed driver table. Counts: `0x0017ff18` 3,498,
  `0x00180440` 3,123, `0x0017fea8` 3,123, `0x0018e248` 2,328,
  `0x0018e380` 1,956, `0x0018e0a8` 2,331, `0x0018e1e0` 4, and
  `0x00181f18` 21. Driver-table slots `0x0017fee0`, `0x0017fae0`,
  `0x00180c10`, `0x00180dc0`, `0x001810a8`, `0x00181148`,
  `0x00181eb0`, and setup/static helper slots stayed zero in that window.
- Focused root-cluster trace:
  `gh1_driver_root_cluster_arg_trace_20260612.json` recorded
  `0x0018e0a8` 3,548, `0x0018e380` 3,386, `0x0018e248` 2,980,
  `0x0018e1e0` 8, `0x00181f18` 40, and `0x0017ff18` 5,748. Screenshot
  `gh1_driver_root_cluster_arg_trace_20260612.window.png` is accepted active
  gameplay, slowed by interpreter tracing.
- Root-cluster interpretation: this is performer/root routing, not proven
  clip/blend/sample output. `a0` resolves cleanly to root objects
  `guitarist0`, `singer`, `bass`, `drummer`, and crowd roots; follow `+0x14`
  reaches the corresponding per-role object block, and the next calls dispatch
  that role's `main.drv` through `0x0017ff18`. `0x00181f18:a1` likewise
  samples root owners, while `a2` samples script/resource rows.
- Static support:
  `gh1_driver_root_cluster_function_snippets_20260612.json` shows
  `0x0018e0a8` calling helpers `0x0018e5f0` and `0x002480d8`,
  `0x0018e380` calling `0x0018e280` when root state changes,
  `0x0018e248` conditionally calling `0x002ec850` on `root+0x18+0x30`, and
  `0x0018e1e0` calling `0x001da6c8` then an indirect table callback from
  `*(root+0x18+0x124)`. These helper targets are the next concrete GH1 trace
  targets before naming clip/blend semantics.
- Follow-up root-child helper trace:
  `gh1_root_child_helpers_trace_20260612.json` is accepted active gameplay
  evidence. It recorded `0x0018e0a8` 84, `0x0018e5f0` 84,
  `0x002480d8` 4,665, `0x0018e380` 84, `0x0018e248` 83,
  `0x002ec850` 10,806, `0x001da6c8` 452, and driver begin `0x0017ff18`
  126. `0x0018e280` and `0x0018e1e0` stayed zero in this window.
- Root-child interpretation: the repeated path is
  `0x0018e0a8(root, work/state, root, flags) -> 0x0018e5f0 -> 0x002480d8`,
  followed by root dispatch/cleanup and eventual `main.drv` dispatch. Sampled
  `0x002ec850` rows include character mesh rows, venue meshes, and camera/view
  rows, so keep it labeled as broad object/mesh traversal. Sampled
  `0x001da6c8` rows are venue/camera/mesh tree rows such as `top.view`,
  `6 foot camera.cam`, cymbal/lighting/venue meshes, and props. This closes
  the root-child target as routing/traversal evidence, not clip output.
- Bone-servo child helper trace:
  `gh1_boneservo_child_helpers_trace_20260612.json` is accepted active
  gameplay evidence. It recorded `0x00182730` 261, `0x001896f8` 261,
  `0x0024af18` 132, `0x0024cb70` 14,416, `0x0018e238` 132,
  `0x0017ff18` 396, `0x00180440` 393, and `0x0017fea8` 393.
- Bone-servo child interpretation: `0x00182730` dispatches live `bone.servo`
  rows for singer, bass, drummer, guitarist0/Judy, and crowd. Immediately after
  the `+0x54` driver-entry walk, `0x001896f8` receives the `bone.servo+0x10`
  work blocks (`0x00c14ce0`, `0x00c162a0`, `0x00c17830`, `0x00bcb790`).
  Those blocks contain size/count words and runs of pointers to transform data.
  `0x0018e238` is only a root helper returning `*(root+0x18+0x80)+0x20`.
  `0x0024af18` and especially `0x0024cb70` are hot float/trig helpers and must
  not be labeled as bone-specific clip functions.
- GH1 driver/current-row samples:
  `gh1_driver_work_rows_sample_20260612.json`,
  `gh1_driver_child_pointer_rows_sample_20260612.json`, and
  `gh1_live_table_rows_sample_20260612.json` are accepted active-gameplay
  samples. They show role `main.drv` rows using table `0x002f8ef8`, source
  `.cset` rows at `+0x0c`, changing current/output-row pointers at `+0x2c`,
  and adjacent `bone.servo` or hand/IK/twist controller rows.
- Important sampled driver rows:
  guitarist0/Judy `main.drv 0x00bb6aa0`, left hand `0x00bcb990`,
  right hand `0x00bcb9d0`, singer `0x00bd24a0`, bass `0x00c16250`, and
  drummer `0x00c177e0`. The `+0x2c` current rows use table `0x002f9368` and
  change live: examples include Judy main `0x0071c580`, hand rows
  `0x0071c000/0x0071c040`, singer rows `0x0071c0c0/0x0071c2c0`, bass
  `0x0071c100`, and drummer `0x0071c180/0x0071c480`.
- The `.cset` samples also prove active clip/resource names in the same live
  state: `alterna_extreme_fast_01`, `finger_chord_bar`,
  `female_singer_active_fast`, `bassist_active_fast`, and
  `drummer_active_fast_allbeat`. Treat these as clip/source rows, not output
  transforms.
- `gh1_cset_current_table_trace_20260612.json` is accepted active gameplay
  with the EE recompiler disabled only for the trace and restored afterward.
  Named counts: `0x0018a4b0` 5,856, `0x0018a870` 4,851,
  `0x0018a970` 3,014, `0x0018d860` 2,344, `0x0018d978` 2,343,
  `0x0018d780` 2,343, `0x001896f8` 1,842, driver begin `0x0017ff18`
  3,515, driver leaf `0x0017fea8` 3,014, and driver mid `0x00180440`
  3,014. `.cset` setup slots `0x0017ddf0` and `0x0017f660` were zero in this
  steady active-song window; child `0x00180740` hit 14 times with stack-like
  rows, so it is not yet a named clip-selection stage.
- Proven GH1 call order for the current/output bridge:
  `0x0017ff18(main.drv) -> 0x0018a4b0(current row)`,
  `0x0017fea8(main.drv) -> 0x0018a970(current row, bone.servo+0x10 work block)`,
  then `0x00180440(main.drv) -> 0x0018a870(current row, same work block)`.
  This is currently the cleanest GH1 evidence linking driver dispatch,
  current/output rows, and the `bone.servo+0x10` transform work block.
- GH1 clip-instance table sample:
  `gh1_clip_instance_table_sample_20260612.json` shows current-row `+0x24`
  pointers resolving to clip-instance rows with table `0x002f8df0`. Examples:
  `0x00c79eb0 alterna_stand_bad`, `0x00cf0db0 finger_open`,
  `0x00cf31a0 strum_open`, `0x012aa3e0 female_singer_active_fast`,
  and `0x0135ac80 bassist_active_medium`. The same rows hold source `.cset`
  pointers at `+0x28` and channel/span payload pointers around `+0x34`,
  `+0x38`, `+0x78`, and `+0x80`.
- `gh1_clip_instance_callbacks_trace_20260612.json` is accepted active
  gameplay and proves the live clip-instance slots in this state:
  `0x0017c0c8` 4,385, `0x0017c008` 2,724, `0x0017bf68` 1,996, and
  `0x0017bf00` 998. Other sampled clip-instance slots were zero in this
  steady active-song window. The anchored order is:
  `driver leaf -> current 0x0018a970 -> clip-instance 0x0017c008`, then
  `driver mid -> current 0x0018a870 -> clip-instance 0x0017c0c8`.
- Static support in `gh1_clip_instance_function_snippets_20260612.json`:
  `0x0018a970` is a tiny virtual dispatch through current-row `+0x24` and
  the clip-instance table `+0x1c`, which is `0x0017c008`. `0x0018a870`
  blends/applies through the same current-row family and dispatches table
  `+0x14`, which is `0x0017c0c8`.
- `gh1_clip_child_helpers_trace_20260612.json` is accepted active gameplay and
  maps the first child layer. Counts: `0x0017b238` 5,136,
  `0x0017ae98` 3,368, `0x0017b6b8` 2,510, `0x0017baf8` 1,255,
  `0x0017b8e8` 1,921, `0x0017bb90` 1,921, `0x0017adb0` 512, and
  current helper `0x0018a9d0` 1,927.
- Clip child interpretation: the eval path is
  `0x0017c008 -> 0x0017b6b8 twice -> 0x0017baf8`; the apply/blend path is
  `0x0017c0c8 -> 0x0017b238 -> 0x0017b8e8 -> 0x0017bb90`, with
  `0x0017ae98` hot under the apply path. The child helpers receive the
  `bone.servo+0x10` work block as `a1`, and their `a0` rows are per-channel
  clip/span blocks. Sampled payload rows include bone channel names such as
  `bone_L-index01.quat`, proving this layer is where clip data is being driven
  toward named transform channels.
- `gh1_clip_child_helper_function_snippets_20260612.json` and
  `gh1_clip_deeper_helper_function_snippets_20260612.json` are static support
  for the child layer. They show eval-side helpers `0x0017b6b8` and
  `0x0017baf8` calling `0x0017b5d0` / `0x0017ba00`; apply-side helpers
  `0x0017b8e8` and `0x0017bb90` call `0x0017ace8`, `0x0017b238`,
  `0x0017ae98`, and math helper `0x0027d9b0`.
- `gh1_clip_deeper_helpers_trace_20260612.json` is accepted active gameplay
  and keeps the EE recompiler restored afterward. Retained live counts:
  `0x0017b5d0` 5,274, `0x0017ba00` 2,198, `0x0017ace8` 3,806,
  `0x0027d9b0` 6,186, plus the surrounding clip/current helpers. The sampled
  static-looking targets `0x00189070` and `0x00188a48` were zero-hit in this
  steady active-song window, so they are not the live GH1 arm/hand output
  bridge for this state.
- Deeper child interpretation: the repeated GH1 order is
  `current -> clip eval 0x0017c008 -> b6b8/b5d0 -> b6b8/b5d0 -> baf8/ba00`,
  then `clip apply 0x0017c0c8 -> b238/b5d0 -> b8e8/ace8 -> b238/b5d0 -> bb90/ba00 -> ae98/ace8`.
  Argument samples show `a1` remains the role `bone.servo+0x10` work block
  (`0x00bcb790` for Judy/guitarist0 and `0x00c14ce0` for the singer in this
  slice). The `a0` span rows carry channel-name lists at `+0x10`, including
  full-body channels (`bone_facing.pos`, `bone_pelvis.pos`,
  `bone_L-hand.quat`, `bone_R-upperArm.quat`), fret-hand channels
  (`bone_fret_hand.pos`, `bone_fret_hand.quat`), and finger channels
  (`bone_L-index01.quat`, `bone_L-index02.rotz`, `bone_R-hand.quat`, etc.).
  This is direct evidence that GH1 arms/hands are channel-span driven, not a
  per-bone one-off correction path.
- Static write scan guardrail: `0x0017ace8` is a small time/index helper that
  writes to caller-provided scratch storage. `0x0017b5d0` and `0x0017ba00`
  update fields on the channel-span rows, including cached indices/pointers.
  They are span-state/cache maintenance, not final output writers. The final
  GH1 output bridge is now traced separately through
  `gh1_output_workblock_trace_20260612.json`: `0x001896f8` walks role work
  blocks, `0x001da730` copies local rows to target Trans rows and sets
  `+0xa0`, and `0x0024ae78` expands quaternion rows into local matrix rows.
- Local GH2 Deluxe / GH1 Redux comparison breadcrumb: the user-provided local
  `_community_re/Guitar-Hero-II-Deluxe-Unified` tree has trace-useful package
  evidence: per-character `og` PS2 payloads, `ng` Xbox payloads, and
  per-character `anims/gen` banks for `main`, `fret`, `strum`, `ui`, and
  `viseme`. Use this as a worked asset/package comparison source after PS2
  trace evidence defines runtime meaning. Do not treat GH1 Redux assets or
  comments as authoritative over PCSX2 trace results.
- Latest accepted GH1 selector trace:
  `gh1_source_selector_trace_20260612.json` / `.window.png`. Counts:
  `0x0018d860` 1,080, `0x0018d780` 1,080, `0x0018d978` 1,080,
  `0x0018a4b0` 2,700, `0x0018a9d0` 1,795, rare `0x0018d9f0` 14,
  rare `0x00180740` 9. Zero-hit in that active window:
  `0x0017ddf0`, `0x0017f660`, `0x0017fae0`, `0x0018aa98`,
  `0x0018a3c8`.
- Next active task is remaining GH1 hair/eyes/look-at row semantics and any
  still-missing role/song-specific coverage from the now proven
  source/current/clip/blend/output map.
- Latest broadened GH1 active-song pipeline trace:
  `gh1_pipeline_broaden_trace_20260612.json` / `.window.png`. It captured
  842,276 total traced calls over 16 seconds with a 32,768-record ring.
  Retained role source rows stayed evenly cycled across `guitarist0`, `singer`,
  `bass`, and `drummer`. Current clip coverage now includes
  `alterna_stand_bad`, `finger_open`, `strum_open`,
  `female_singer_active_fast`, `bassist_active_medium`, and
  `drummer_active_medium_normal`; bone-servo roots for all four roles were
  sampled. Downstream mesh/vector work was active in the same run:
  `0x001da570` 11,952 retained records, `0x001da1a8` 997,
  `0x0024b608` 15,971, and `0x0024a420` 98.
- Parsed broadened GH1 mesh modes: 539 unique `0x001da570` rows sampled.
  Major buckets are 409 ordinary `0x002f9838` rows, 64 ordinary `0x002fa990`
  rows, 28 dirty mode-3 rows with `+0xac=0x0139b1d0`, 12 dirty mode-0 rows,
  2 visible eye mode-1 rows (`0x006c08e0`, `0x0068c020`), and 6 mode-8 vector
  rows. Matrix-helper parent names include GH1 twist rows
  `bone_L-upperTwist1/2.mesh`, `bone_L-foreTwist1/2.mesh`, and mirrored
  right-arm rows, confirming twist bones are ordinary graph-propagated Trans
  rows in this slice.
- Parsed GH1 `0x001da1a8` retained calls from the broadened trace: 37 unique
  extra-helper rows, 997 retained calls. Mode-1 visible eye rows:
  `L-eye.mesh 0x006c08e0 -> parent bone_head.mesh 0x006bc540 -> a2 0x006c0970`,
  `R-eye.mesh 0x0068c020 -> parent bone_head.mesh 0x006bc540 -> a2 0x0068c0b0`.
  Mode-3 rows are broad skeleton/attachment rows with shared
  `+0xac=0x0139b1d0`, including head/neck, hands, forearms, twist rows,
  pelvis/spine/limbs, clavicles, and `bone_pos_guitar`; examples:
  `bone_L-foreTwist1.mesh 0x006f99a0`, `bone_L-foreTwist2.mesh 0x006fb780`,
  `bone_R-foreTwist1.mesh 0x00710d40`, `bone_R-foreTwist2.mesh 0x00711420`,
  `bone_L-hand.mesh 0x00702460`, `bone_R-hand.mesh 0x00709500`,
  `bone_pos_guitar.mesh 0x00701800`. Do not label mode-3 as head/face-only.
- Latest accepted GH1 hair/eye/face evidence:
  `gh1_face_eye_vptr_scan_20260612.json`,
  `gh1_face_mesh_slots_trace_20260612.json`, and
  `gh1_live_face_hair_eye_rows_sample_20260612.json`.
  `scan_pcsx2_live_vptrs.py` was updated with `--table-start` /
  `--table-end`; use `0x002f0000..0x00340000` for GH1 vtable scans.
  The live GH1 face/hair/eye path is mesh/Trans based, not GH2-style named
  `CharHair` / `CharLookAt` controllers. Rows proven moving:
  Judy `bone_head.mesh` `0x006bc4e0`, `hair01.mesh` `0x006814c0`,
  `face.mesh` `0x00681fc0`, `L-eye.mesh` `0x006c08e0`,
  `R-eye.mesh` `0x0068c020`, and blink cluster `0x00700210`; bassist
  `bone_head.mesh` `0x006bc900` plus ponytail meshes `0x006c7300`,
  `0x006c6ac0`, `0x006c7c40`, `0x006c4e40`; drummer hair
  `0x006d5460`. `lashes.mesh` `0x006973c0` was stable in this sampled
  window.
- Latest GH1 mesh/Trans helper trace:
  `gh1_mesh_trans_helpers_trace_20260612.json` plus static support
  `gh1_face_mesh_function_snippets_20260612.json`. Counts:
  `0x001da570` 2,910, `0x0024b608` 3,593, `0x001da1a8` 217,
  `0x001c4038` 1,472. Live rule: `0x001da570(row,parent,1)` calls
  `0x0024b608(row+0x20,parent,row+0x60)`. Eye/head rows additionally call
  `0x001da1a8`, e.g. `L-eye.mesh 0x006c08e0 -> a2=0x006c0970`,
  `R-eye.mesh 0x0068c020 -> a2=0x0068c0b0`, and head row `0x006fa340`.
  Bass ponytails are chained through bass head output `0x006bc960`.
- Latest GH1 extra-helper closure:
  `gh1_mesh_extra_helper_full_snippet_20260612.json` and
  `gh1_eye_extra_helper_trace_20260612.json`. The focused trace hit
  `0x001da570` 3,485, `0x001da1a8` 273, `0x0024a420` 30, and
  `0x0024b608` 4,404. `0x0024b608` is the three-row matrix combine helper.
  `0x001da1a8` is row-mode driven: visible Judy eye rows are
  `+0xa4=1/+0xa8=0/+0xac=0` and do not call `0x0024a420` in this window;
  mode-3 rows are broad skeleton/attachment rows
  `+0xa4=3/+0xa8=0/+0xac=0x0139b1d0` rather than head/face-only; a separate
  six-row vector cluster `0x0079ba00..0x0079c240` is
  `+0xa4=8/+0xa8=1/+0xac=0` and calls `0x0024a420(row+0x60,scratch,scratch)`.
- Latest GH1 mode-8 vector cluster row sample:
  `gh1_vector_cluster_rows_sample_20260612.json`. The six rows are forced
  mesh/Trans children under parent output `0x00bcdf50`. Call order from the
  existing traces is `0x001da570(row,0x00bcdf50,1)` ->
  `0x0024b608(row+0x20,0x00bcdf50,row+0x60)` ->
  `0x001da1a8(row,0x00bcdf50,row+0x90)`. The live-changing row words in the
  sample are only `row+0x54` and `row+0x94`. Owner/name is still unresolved;
  do not use `0x0032e980` as name evidence because it resolves to generic
  static data, not a character/mesh label.
- Static `0x0024a420` helper read: caller passes `a0=row+0x60` and
  `a1=stack`; the helper writes three floats to that scratch buffer after
  VU/COP2 vector work over row-local output blocks. Its scalar tail performs a
  dot-like sign test and can negate the third scratch output. VU opcode-level
  meaning remains open, so keep the label as row-output/scratch-vector helper,
  not a guessed look-at/IK controller.
- Latest GH1 mesh update static closure:
  `gh1_mesh_update_full_snippet_20260612.json`. `0x001da570` updates when
  forced by `a2` or when `row+0xa0` is dirty, clears `row+0xa0`, writes
  `row+0x60..0x90` either via `0x0024b608(row+0x20,parent,row+0x60)` or a
  local copy if parent is null, optionally runs `0x001da1a8`, and recursively
  dispatches children from `row+0x08` with `a1=parent_row+0x60`,
  `a2=dirty_or_forced`, `a3=0`. The focused trace shows this exact graph for
  Judy head -> hair/eyes/face, bass head -> ponytail chains, and drummer hair.
- Latest GH1 visible gate static closure:
  `gh1_mesh_visible_gate_snippet_20260612.json`. `0x001c4038` checks the
  scalar at `a0+0x20`; if zero it returns 0. If nonzero it combines the matrix
  at `[a0+0x50]+0x60` with local block `a0+0x10`, writes to `a1`, copies the
  scalar to `a1+0x10`, and returns 1. Treat it as a visibility/output-copy
  helper, not an animation solver.
- Next trace target: GH80s breadth work is closed; continue from GH1 and the
  remaining main trace gaps. GH1 source/current/clip/blend/output-to-Trans,
  mesh/extra helper, hair, eye, face, and dirty propagation are now traced
  enough to preserve the loader structure. The `+0xa4=8` vector-helper
  structure is now traced as unnamed structural child data under the
  hand/Trans graph; chase a human-readable owner only if a future live sample
  exposes a real name string. Venue, camera, and lighting remain open after
  character animation structure.

GH1 mode-8 vector cluster follow-up:
`gh1_mode8_owner_follow_trace_20260612.json` / `.window.png` is accepted
active GH1 gameplay evidence. It launched the stock GH1 ISO
`Guitar Hero (USA).iso`, ELF `Guitar Hero (USA)\SLUS_212.24`, indexed
`--state 1`, `--background-input`, `--disable-ee-recompiler`,
`--retry-pulses 0`, `--require-screenshot`, `--seconds 16`,
`--ring-size 32768`, `--stub-base 0x01c00000`, and
`--data-base 0x01d00000`. The command traced
`mesh_update_001da570=0x001da570`,
`mesh_extra_001da1a8=0x001da1a8`,
`vector_helper_0024a420=0x0024a420`,
`matrix_helper_0024b608=0x0024b608`, and
`output_dirty_001da730=0x001da730`, with `--sample-a0`,
`--sample-arg`, `--sample-a0-follow`, and `--delta-sample-a0` on the mesh
extra/vector/dirty rows. PCSX2 was not foregrounded.

Retained counts from that trace:

- `matrix_helper_0024b608`: 17,614
- `mesh_update_001da570`: 13,456
- `mesh_extra_001da1a8`: 1,205
- `output_dirty_001da730`: 369
- `vector_helper_0024a420`: 124

The screenshot shows active `I Love Rock & Roll` gameplay in the basement
venue, so the trace is accepted as runtime character evidence. The follow rows
resolve the named hand neighbors:

- `bone_L-hand.mesh` row `0x00702460` has mode
  `+0xa4=3/+0xa8=0/+0xac=0x0139b1d0`; its name pointer follows through
  row `+0x00 -> 0x00702570 -> 0x00bcbdee`.
- `bone_R-hand.mesh` row `0x00709500` has the same mode-3 shared data; its
  name pointer follows through `+0x00 -> 0x00709610 -> 0x00bcbd6f`.
- The vector-helper rows themselves are still unnamed structural children:
  sampled mode-8 rows such as `0x0079ba00`, `0x0079bcc0`,
  `0x0079be20`, `0x0079bf80`, `0x0079c0e0`, and `0x0079c240` carry
  `+0xa4=8/+0xa8=1/+0xac=0`, vtable `0x002f9838`, and follow through
  generic Trans/object rows whose first word is the generic class pointer
  `0x0032e980`, not a human-readable mesh name.
- `vector_helper_0024a420` received `a0=row+0x60` for seven live output rows
  in this active slice: `0x0079ba60`, `0x0079bbc0`, `0x0079bd20`,
  `0x0079be80`, `0x0079bfe0`, `0x0079c140`, and `0x0079c2a0`.
- Same-process deltas confirm the mode-8 row motion is concentrated in the
  same scalar/vector fields already seen: mesh-extra rows changed
  `row+0x54` and `row+0x94`, while the vector-helper `a0=row+0x60` samples
  changed the corresponding output-space word at `a0+0x34`.

Interpretation: the GH1 mode-8 cluster is not a missing named controller. It
is a structural child chain under the hand/Trans graph, adjacent to named
hand, forearm, foretwist, uppertwist, finger, guitar, and venue rows. The
native/custom-character loader must preserve these child rows, mode fields,
vtable/class rows, parent/output links, and dirty propagation. Do not invent a
GH2-style `CharLookAt`, `CharHair`, or IK semantic name for the mode-8 rows
unless a later trace exposes a real name.

GH1 venue/camera/lighting candidate support:
`gh1_static_camera_lighting_string_refs_20260612.json`,
`gh1_live_camera_lighting_strings_20260612.json`, and
`gh1_live_venue_camera_lighting_vptr_scan_20260612.json` are support/candidate
artifacts, not accepted runtime proof by themselves. The live string scan and
vptr scan were useful because they surfaced `venues/basement/camera.dtb`,
`lighting`, `lighting_rt.view`, `pick_shot`, `post_switch_cam`, `one_bar_to`,
and `downbeat` data refs, plus candidate live rows around `0x00745848` /
`0x007458d8` (`venues/basement/gen/lighting_ps2.rnd_ps2`,
`lighting.rnd_ps2`, `crowd_ps2.rnd_ps2`) and `0x00745c64..0x00745cc4`
(`glow_j.bmp`, `lamplight.bmp`, `lava.png`, `gtr_on`). They lacked accepted
runtime screenshots, so use them only to explain why the later targets were
chosen.

Accepted GH1 venue/camera/lighting candidate trace:
`gh1_venue_camera_lighting_candidate_trace_20260612.json` / `.window.png`
ran the stock GH1 ISO and `SLUS_212.24` from accepted indexed `--state 1`,
with `--require-screenshot`, `--background-input`,
`--disable-ee-recompiler`, `--retry-pulses 0`, `--seconds 18`,
`--ring-size 32768`, `--stub-base 0x01c00000`, and
`--data-base 0x01d00000`. The screenshot is active GH1 basement gameplay with
performers, HUD/highway, and lighting visible, so the trace is accepted. It
targeted the candidate table slots
`lighttex_slot0_002dd6d0=0x002dd6d0`,
`lighttex_slot1_002dd720=0x002dd720`,
`lighttex_slot2_002dd7a0=0x002dd7a0`,
`lighttex_slot3_002ddef0=0x002ddef0`,
`lighttex_slot4_002dde98=0x002dde98`,
`lighttex_slot5_00236818=0x00236818`,
`lighttex_slot6_002dded8=0x002dded8`,
the `lighting_rnd_slot*` family from table `0x002f8d68`, and static
`Cam`/`Light` label refs. Retained calls were
`lighttex_slot2_002dd7a0` 77 and `lighttex_slot4_002dde98` 754; all
`lighting_rnd_slot*`, static `Cam`/`Light` labels, and the other
`lighttex_slot*` entries were zero-hit in that active window.

Accepted GH1 venue/camera/lighting hot-argument trace:
`gh1_venue_lighting_hot_arg_trace_20260612.json` / `.window.png` reran the
same stock GH1 active-song route with accepted screenshot proof. The screenshot
shows active GH1 gameplay with the band, HUD/highway, and visible venue
lighting; PCSX2 was not left running afterward. Targets were
`lighttex_apply_002dd7a0=0x002dd7a0`,
`lighttex_singleton_002dde98=0x002dde98`,
`lighttex_base_002dd6d0=0x002dd6d0`,
`lighttex_ctor_002dd720=0x002dd720`,
`lighttex_init_002dded8=0x002dded8`, and
`lighttex_get_002ddef0=0x002ddef0`, with `--sample-a0`,
`--sample-arg`, `--sample-a0-follow`, and `--delta-sample-a0` on the two hot
functions. Retained calls were `0x002dd7a0` 80 and `0x002dde98` 756; the
base/ctor/init/get entries remained zero-hit.

Hot trace interpretation:

- `0x002dd7a0` is a small dispatch/apply wrapper. Static snippet
  `gh1_venue_lighting_hot_function_snippets_20260612.json` shows it saves
  `a0`, forces `a2=2`, clears `a3`, loads `a1=*(a1+8)`, calls
  `0x0022ecb8`, and returns the original `a0`. Runtime arguments prove it is
  used by GH1 script/data rows for camera, crowd, band, and venue lighting:
  `get_shot_duration`, `pick_shot`, `pick_regular_shot`, `eval_shot`,
  `check_shot`, `update_crowd`, `set_crowd_sizes`, `animate_crowd`,
  `band_changeup`, `main_clip_flags`, and `set_lights_per_excitement`.
- The same trace ties the basement lighting script atoms directly to the
  wrapper: tuple `a0=0x01fff110`, `a1=0x00745c60`, `a2=0x00745400`,
  `a3=0x00745c60` follows to `set_lights_bad`,
  `set_lights_okay_verse`, `set_lights_okay_chorus`,
  `set_lights_okay_solo`, `set_lights_great_verse`,
  `set_lights_great_chorus`, `set_lights_great_solo`, `anim_bad`,
  `anim_okay`, and `anim_great`. The paired data row at `0x00745400`
  resolves `venues/basement/basement.dtb` and
  `venues/basement/camera.dtb` entries such as `far` and `near`.
- `0x002dde98` is a broad singleton/root helper, not lighting-specific by
  itself. The static snippet initializes/returns global cell `0x00343a90`.
  Runtime `a3` samples resolve many object rows, including camera/venue rows
  (`shaky_cam1.tnm`, `venue.view`, `Cam_nt_np_close.tnm`) and character/bone
  rows (`bone_head.mesh`, `stage_spot_01.mesh`, `bone_L-hand.mesh`, etc.).
- The table `0x002f8d68` resource/container family around
  `lighting_ps2.rnd_ps2`/`lighting.rnd_ps2` was zero-hit in this slice, so it
  is not proven as the per-frame apply path. The zero-hit static `Cam`/`Light`
  label refs and zero-hit constructor entries must remain candidate/support
  only.

Current GH1 venue/camera/lighting gap after these traces: the script dispatch
bridge and basement lighting/camera/crowd/band atoms are runtime-proven, but
final CamShot pose/blend output, render-camera handoff, venue animation apply,
and LightPreset/render-light field semantics are not closed.

GH1 script-dispatch inner trace:
`gh1_script_dispatch_inner_sequence_20260613.json` / `.window.png` is accepted
active GH1 gameplay evidence. It used the stock GH1 ISO / `SLUS_212.24`
active `--state 1` route with `--require-screenshot`, `--background-input`,
`--disable-ee-recompiler`, `--retry-pulses 0`, `--seconds 18`,
`--ring-size 65536`, `--stub-base 0x01c00000`, and
`--data-base 0x01d00000`. The screenshot shows active basement gameplay with
band, HUD/highway, and lighting; PCSX2 was stopped afterward.

Retained calls:

- `dispatch_eval_00235b50`: 10,461
- `dispatch_store_00235f30`: 9,735
- `dispatch_root_00235fb8`: 6,195
- `dispatch_inner_0022e418`: 2,066
- `dispatch_cleanup_002374f8`: 578
- `script_dispatch_0022ecb8`: 557
- `dispatch_copy_00235e58`: 121
- `lighttex_apply_002dd7a0`: 47

The static snippet `gh1_script_dispatch_0022ecb8_snippet_20260613.json`
shows `0x0022ecb8` is the generic DTB/script dispatch path reached by
`0x002dd7a0`. Runtime `a1` rows prove the dispatcher walks script/data
records from `ghui/game.dtb`, `config/player.dtb`, `config/beatmatch.dtb`,
`arena/camera.dtb`, `arena/crowd.dtb`, `arena/venue.dtb`, and
`charsys/theband.dtb`. Camera rows include `pick_shot`, `pick_solo_shot`,
`pick_regular_shot`, `change_to_regular_shot`, `change_to_solo_shot`,
`eval_shot`, `check_shot`, `get_shot_duration`, `downbeat`, and
`camera.pool_index`. Venue/crowd rows include `set_lights_bad`, `bad`, `ok`,
`great`, `set_crowd_sizes`, `full`, `flat`, `update_crowd`, `crowd`, and
`set_sizes`. Band rows include `main_clip_flags`, `anim_space`, `main.drv`,
and `get_first_flags`.

Interpretation: this is valuable layout evidence for GH1 DTB/script execution,
but it is mostly generic interpreter/container plumbing. Do not keep tracing
only `0x0022ecb8` and its helper functions when the question is final
render-camera or render-light semantics; use it to choose downstream live
objects and low-level helper targets.

GH2-to-GH1 camera/helper body-match support:
`gh2_to_gh1_camera_lighting_body_match_20260613.json` compares already-proven
GH2 camera/lighting function bodies from `SLUS_214.47` against GH1
`SLUS_212.24`. It is static support only unless paired with live traces. Strong
unique helper matches from this pass:

- GH2 CamShot/path math helper `0x002ff268` -> GH1 `0x0027cfa0`
  (unique 64/48/32-byte match).
- GH2 CamShot/path copy child `0x002ff6d0` -> GH1 `0x0027d408`
  (unique 192/160/128-byte match).
- GH2 camera vector helper `0x001b1270` -> GH1 `0x001b1108`
  (unique 160/128/96-byte match after an 8-byte skip).

The same static pass did not produce a clean GH1 map for the GH2 top-level
CamShot eval/apply/result-writer functions. Treat weak 16-byte matches such as
`cam_apply_child_00263410 -> 0x00104a38` as non-actionable until live evidence
backs them.

Accepted GH1 body-matched camera-helper trace:
`gh1_camera_helper_bodymatch_sequence_20260613.json` / `.window.png` traced
the strong helper candidates above in the accepted stock GH1 active-song
state. The screenshot is active basement gameplay with band, HUD/highway, and
venue lighting visible, so the trace is accepted. Retained calls were:

- `cam_float_child_bodymatch_0027d408`: 6,872
- `cam_float_helper_bodymatch_0027cfa0`: 4,775
- `script_dispatch_0022ecb8`: 510
- `cam_vec_helper_bodymatch_001b1108`: 435
- `script_wrapper_002dd7a0`: 46

Runtime samples show the float helper/child mostly receiving float-like
arguments, but sampled rows also tie the body-matched camera helper family to
basement camera/crowd data: `0x00bcd3f0` / `0x00bcd404` follow to
`balcony_rt`, `SOLO_NEAR`, `flr_near_lft`, `flr_near_rt`, `flr_far_lft`, and
`venues/basement/streams/crowd_v1_2poor.vgs`. The vector helper
`0x001b1108` is stronger camera evidence: `a0=0x00c51190` follows to
`6 foot camera.cam`, while `a1` samples include rows such as `0x0074e550`,
`0x0074e5b0`, `0x0074e610`, `0x0074e670`, `0x0074e6d0`, `0x0074e730`,
`0x0074e790`, `0x0074e7f0`, `0x0074e850`, `0x0074e8b0`, `0x0074e910`, and
`0x0077fbb0`.

Same-process deltas from that trace prove `0x001b1108` updates the live
camera row: `0x00c51190` changed 22 words in 5 seconds, including float-like
fields at `+0x60`, `+0x64`, `+0x70`, `+0x74`, `+0x78`, `+0x80`, `+0x84`,
`+0x88`, `+0x90`, and `+0x94`. The float child row `0x00bcd3f0` changed six
words, including code/table pointer cells and float-like values at
`0x00bcd404` / `0x00bcd408`.

Current GH1 camera-helper conclusion: the low-level camera math/helper family
is now runtime-proven in GH1 and tied to `6 foot camera.cam`, but the
top-level GH1 CamShot evaluator/result-writer and render-camera consumer are
still not mapped. Next trace should discover the caller/owner path feeding
`0x001b1108` / `0x0027cfa0` / `0x0027d408`, not repeat generic
`0x0022ecb8` interpreter tracing.

Accepted GH1 camera owner/caller traces:
`gh1_camera_owner_candidate_sequence_20260614.json` / `.window.png` and
`gh1_camera_owner_hot_arg_sample_20260614.json` / `.window.png` were both
headless, background-input, accepted stock GH1 active-song traces. Both
screenshots show in-song basement gameplay with HUD/highway, band, venue, and
lighting visible. The first trace retained 26,746 calls over 20 seconds; the
second retained 7,493 calls over 12 seconds and sampled the hot owner rows.

The candidate trace proves which static xref owners are actually live:

- `cam_entry_001b1100`: 555 calls
- `cam_float_helper_0027cfa0`: 6,257 calls
- `cam_float_child_0027d408`: 8,823 calls
- `owner_0016e390`: 543 calls
- `owner_0027e8e0`: 6,953 calls
- `owner_0027e540`: 1,072 calls
- `owner_0027d788`: 1,072 calls
- `owner_0027ee20`: 543 calls
- `owner_0028077c`: 543 calls
- `owner_00248fd0`: 383 calls
- `owner_001727b0`: 2 calls
- Dead in this gameplay window: `0x00179c20`, `0x001999e8`, `0x001d2e38`,
  `0x001d2db8`, `0x001f4d08`, `0x00248df8`, `0x00248ef0`,
  `0x0027d688`, and `0x0027f6b0`.

The strongest caller/owner evidence is the repeated per-frame sequence
`owner_0016e390 -> cam_entry_001b1100 -> owner_0028077c`. Runtime samples show
`owner_0016e390` receiving `a0=0x014ac070`; that live owner object has
`+0x08 = 0x00c51190`. The next call to `cam_entry_001b1100` receives
`a0=0x00c51190`, the same live row already tied to `6 foot camera.cam`.
Typical hot tuples from the resample:

- `owner_0016e390`: 344/351 calls used
  `(a0=0x014ac070, a1=0x00469050, a2=0x006c5c30, a3=0)`.
- `cam_entry_001b1100`: 351/363 calls used
  `(a0=0x00c51190, a1=0x01fff900, a2=0x01fff910, a3=0x006bc4e0)`.
- `owner_0028077c`: 351/351 calls used
  `(a0=1, a1=0, a2=0x3ec90fda, a3=0)`.

Static snippet support in `gh1_camera_owner_hot_snippets_20260614.json` shows
`0x001b1100` is the actual function entry for the previously traced
`0x001b1108` vector body: it starts with `addiu sp,sp,-16`, then computes
`v0=a0+0x1c0` before the vector instructions at `0x001b1108`. The hot math
cluster remains shared scalar/curve support rather than the owner itself:
`0x00248fd0` calls `0x0027cfa0`, `0x0027d788` calls `0x0027e540`, and
`0x0027e8e0`/`0x0027e540`/`0x0027d788` mostly run on stack float/vector
rows or sentinel-like float arguments.

Current GH1 camera conclusion after the 2026-06-14 owner traces:
the path from a live higher-level camera owner object to the `6 foot
camera.cam` update helper is now trace-backed as
`0x014ac070 + 0x08 -> 0x00c51190 -> 0x001b1100/0x001b1108`. The remaining
GH1 camera gap is no longer "find the owner"; it is to map the CamShot
eval/blend result writer and the render-camera consumer that reads the updated
`0x00c51190` row. Venue animation apply and LightPreset/render-light field
semantics are still open.

Accepted GH1 downstream camera-result trace:
`gh1_camera_downstream_candidate_sequence_20260614.json` / `.window.png` was
another headless, background-input, accepted stock GH1 active-song trace. The
screenshot is active basement gameplay with HUD/highway, performer, venue, and
lighting visible. It retained 479,048 total calls over 16 seconds. Hot counts:

- `matrix_blend_0024b608`: 57,464
- `angle_ratio_0027d9b0`: 2,367
- `curve_eval_001de038`: 2,214
- `cam_transform_alt_001da6c8`: 1,683
- `cam_transform_001da730`: 1,208
- `shot_time_0016e370`: 225
- `owner_0016e390`: 75
- `preblend_0016e080`: 75
- `cam_entry_001b1100`: 75
- `cam_orient_001b1df8`: 75
- `camera_submit_0019adf0`: 75
- `sqrt_fallback_0027dff8`: 0

The first owner-centered window proves the downstream per-frame order:

1. `owner_0016e390(a0=0x014ac070, a2=0x006c5c30)`
2. `shot_time_0016e370(a0=0x014ac070)`
3. `preblend_0016e080(a0=0x01fff900, a1=0x014ac070)`
4. `cam_entry_001b1100(a0=0x00c51190, a1=0x01fff900,
   a2=0x01fff910, a3=0x006bc4e0)`
5. `cam_transform_001da730(a0=0x00714920, a1=0x01fff900)`
6. `curve_eval_001de038(a0=0x00e4b7e0, a1=0x01fff920)`
7. `cam_orient_001b1df8(a0=0x00c51190, a1=0x00c51bf0,
   a2=0x01fff950)`
8. `cam_transform_alt_001da6c8(a0=0x00c511d0, a1=0x01fff920,
   a2=0x00c513c0)`
9. `camera_submit_0019adf0(a0=0x00363f60, a1=0x00c51190,
   a2=0x00c513c0)`

This trace upgrades `0x0019adf0` to the best current render-camera handoff
candidate, not just a helper. Static disassembly shows `0x0019adf0` checks
`a1`, then loads `v1 = *(a1+0x3c)`, calls the virtual at `v1+0x3c`, and stores
the returned value/pointer to `a0+0x518`. With the live arguments above, that
means `0x00363f60 + 0x518` receives the result of a virtual method reached
through `0x00c51190 + 0x3c`. Static table inspection shows
`0x00c51190 + 0x3c` currently points at class table `0x002f9740`; entries near
that table include camera/scene methods such as `0x0019c7b8`, `0x0019c720`,
`0x001bfef8`, `0x001be458`, `0x001be658`, and `0x001be2a8`.

Other static/runtime facts from this downstream trace:

- `0x001b1100` writes the 2-float result to its `a2` output and reads camera
  row fields around `a0+0x314` through `a0+0x320`.
- `0x001b1df8` writes orientation/FOV-ish camera fields at
  `a0+0x300`, `a0+0x304`, and `a0+0x308`, then calls `0x001b1eb8`;
  `0x001b1eb8` proceeds into `0x00247d90` using `a0+0x240`.
- `0x001da730` and `0x001da6c8` are transform upload/copy helpers: they copy
  stack or camera-row matrices into object-owned transform slots and set the
  dirty flag at object child `+0xa0`.
- `0x0024b608` is a very hot matrix blend/copy helper and should be treated
  as support math unless a narrower caller proves otherwise.

Current downstream GH1 camera gap: trace the virtual method reached by
`0x0019adf0` through `0x00c51190 + 0x3c`, and sample/delta
`0x00363f60 + 0x518` to prove the final render-camera consumer/result object.
The owner/update/submit path itself is now trace-backed.

Accepted GH1 camera submit virtual trace:
`gh1_camera_submit_virtual_sequence_20260614.json` / `.window.png` was a
focused, accepted active-song trace for the `0x0019adf0` submit handoff. The
screenshot is active basement gameplay. It retained 4,770 calls over 14
seconds:

- `camera_method_0019c7b8`: 3,385
- `owner_0016e390`: 383
- `cam_orient_001b1df8`: 383
- `camera_submit_0019adf0`: 383
- `camera_submit_virtual_0019c720`: 236
- `camera_method_001bfef8` / `0x001be458` / `0x001be658` / `0x001be2a8`: 0

The accepted call window proves `0x0019adf0` reaches the resolved virtual
target:

- `camera_submit_0019adf0(a0=0x00363f60, a1=0x00c51190,
  a2=0x00c513c0)`
- `camera_submit_virtual_0019c720(a0=0x00c51190, a1=0x00360000,
  a2=0x00c513c0)`

Sampling `camera_submit_0019adf0`'s `a0` row captured the global/state object
at `0x00363f60`. The `a0+0x518` slot, written by static code in
`0x0019adf0`, was `0x00000c15` in this frame. Treat that as a render-camera
integer/handle result, not a pointer, unless a later trace proves otherwise.

Static body of `0x0019c720`: reads camera row fields `a0+0x300`, `+0x304`,
`+0x30c`, and `+0x310`, combines them with global state at `0x00364050`, and
calls `0x00297478` with the final float in `f12`. That makes `0x00297478` the
next narrow target for the result stored by `0x0019adf0`.

The nearby virtual method `0x0019c7b8` is hot and also receives
`a0=0x00c51190` in some calls, but it is not the `0x0019adf0` submit virtual
at table offset `+0x3c`; it appears to be a broader camera/render update
method and should be traced separately if needed.

Current GH1 camera status after submit-virtual trace: update, orientation,
transform copy, submit handoff, and submit virtual are trace-backed. Remaining
camera-specific implementation gap is the `0x00297478` result path and exact
meaning/lifetime of global `0x00363f60+0x518`. Venue animation and
LightPreset/render-light are still separate open gates.

Accepted GH1 camera result quantize trace:
`gh1_camera_result_quantize_sequence_20260614.json` / `.window.png` was a
focused accepted trace for `0x00297478` and its helper `0x002973b8`. It
retained 62,231 calls over 12 seconds:

- `camera_result_quantize_00297478`: 16,118
- `float_classify_002973b8`: 16,118
- `owner_0016e390`: 178
- `camera_submit_0019adf0`: 178
- `camera_submit_virtual_0019c720`: 176

The submit-linked runtime window proves the call chain:
`camera_submit_0019adf0(a1=0x00c51190) ->
camera_submit_virtual_0019c720(a0=0x00c51190) ->
camera_result_quantize_00297478(f12=0x453ff0e8 in the first captured linked
window) -> float_classify_002973b8`. The helper is very hot globally, so it is
not camera-specific by itself; the camera-specific evidence is the
submit-adjacent window from `0x0019c720`.

Sampling `0x00363f60` in this later frame captured
`0x00363f60+0x518 = 0x00000c1e` (previous accepted submit trace captured
`0x00000c15`). Treat this slot as a per-frame camera/render scalar
integer/handle/index. It is not a direct pointer.

Current GH1 camera trace gate status: the accepted PS2 trace chain now covers
script dispatch into camera atoms, live owner row, camera row update, camera
orientation/matrix work, submit handoff, submit virtual, and the generic
float-to-result helper used by submit. Camera is close to implementation-ready
for the native loader, with one caveat: document field names conservatively
until native validation confirms the visual effect of `0x00363f60+0x518`.
Do not spend more tracing on generic `0x00297478` unless a visual mismatch
points back at that result slot.

Accepted GH1 venue/light traversal trace:
`gh1_venue_lighting_method_sequence_20260614.json` / `.window.png` is an
accepted active GH1 gameplay trace for basement venue/light view traversal. It
used the stock GH1 ISO / `SLUS_212.24` `--state 1` route, headless/background
input, interpreter mode, and required PrintWindow screenshot. Counts:

- `view_tick_001efa10`: 2,365
- `lighttex_singleton_002dde98`: 160
- `lighttex_apply_002dd7a0`: 29
- `view_script_001efea8`: 2
- `trans_copy_001da730`: 897
- `trans_alt_001da6c8`: 1,182
- `matrix_blend_0024b608`: 40,323

Runtime samples tie `view_tick_001efa10` to named rows including
`venue_rt.view`, `venue.view`, `rugs.view`, `washing machine.view`,
`bass_combo.view`, `drum_kit.view`, `mainlight.anim`, `lighting_rt.view`,
`lighting.view`, `crowd.view`, `full.anim`, `verse.anim`, `chorus.anim`,
`solo.anim`, and `police.anim`. Wrapper rows use `+0x00/+0x14` for the named
object, `+0x10` for a stable link/list when present, `+0x20 = 0x002fa9f0`,
and moving `+0x04/+0x08` time/state floats. The accepted live order around the
lighting/view burst is script wrapper `0x002dd7a0`, singleton/root visits
`0x002dde98`, transform copies `0x001da730` / `0x001da6c8`, then
`0x001efa10` view ticks. Adjacent `env_*` / `anim_*` table families were
zero-hit in this state, so treat them as unexercised sibling families.

Accepted GH1 lighting dispatcher negative trace:
`gh1_lighting_dispatcher_sequence_20260614.json` / `.window.png` is accepted
active GH1 gameplay evidence for the current state. GH1 static string search
finds `set_lighting` at `0x00322180`, with its one code reference inside case
body `0x001c0220` of dispatcher `0x001bfef8`. That case parses four values
through `0x00235c20` and calls `0x001be288`. In the accepted 30-second active
trace, `0x001bfef8`, `0x001c0220`, `0x001be288`, and nearby case handlers all
recorded zero calls, while `0x00235c20` hit 463, `0x00235fb8` hit 6,757,
`0x001efa10` hit 32,487, `0x001da730` hit 11,270, and `0x001da6c8` hit
14,559. Do not keep retrying this dispatcher from the same state unless a new
route is expected to trigger `set_lighting`.

Accepted GH1 lighting/view object delta sample:
`gh1_lighting_view_object_delta_20260614.json` / `.window.png` is accepted
active object evidence. Moving rows include `lighting_rt.view` at
`0x0135d0e0`, `lighting.view` at `0x013c7a30`, `mainlight.anim` at
`0x00c50ed0`, `full.anim` at `0x0135be30`, `verse.anim` at `0x0139b2d0`, and
`chorus.anim` at `0x013c6310`. Child rows `0x0139b410`, `0x013c6450`, and
`0x013c5380` expose additional moving timing/state fields. Stable link rows
such as `0x007503f0`, `0x00750e90`, `0x006aee60`, `0x0074fb40`,
`0x007504c0`, and `0x00750900` are structural/list links in this slice.

## GH80s PAL Static Trace Setup

Local GH80s media is the PAL/EU disc:
`Guitar Hero - Rocks the 80s (Europe, Australia).iso`, extracted under
`Guitar Hero Rocks The 80s (EU)`, with ELF `SLES_548.59`. The existing
`SLUS-21374 (960C7892)` savestates are not a PAL serial match, so do not treat
them as proven usable with this disc until PCSX2 actually loads them.

Static matching from GH2 `SLUS_214.47` to GH80s `SLES_548.59` is strong enough
to seed the first GH80s trace. These are exact unique body matches unless
noted:

| Stage | GH2 address | GH80s PAL address | Match evidence |
| --- | ---: | ---: | --- |
| CharDriver inner update | `0x001737b8` | `0x00173808` | unique 24-byte match after skipping 12-byte prologue |
| CharDriver per-frame update | `0x00171830` | `0x00171880` | unique 96-byte match |
| CharDriver selector | `0x00171db0` | `0x00171e00` | unique 128-byte match |
| CharDriver event dispatch | `0x0010c988` | `0x0010c9a0` | unique 64-byte match |
| Normal/play branch | `0x0010b7f8` | `0x0010b810` | unique 24-byte match |
| CharForeTwist update | `0x00175678` | `0x001756c8` | unique 96-byte match |
| CharHair update | `0x00176fb8` | `0x00177008` | unique 96-byte match |
| CharClipSamples eval blocks | `0x0016b1d0` | `0x0016b220` | unique 32-byte match |
| CharClipSamples interp/apply | `0x0016b2f0` | `0x0016b340` | unique 48-byte match |
| CharClipSamples final apply | `0x0016ab88` | `0x0016abd8` | unique 128-byte match |
| CharClipSamples output writer | `0x00168320` | `0x00168370` | unique 128-byte match |
| Bone-servo slot A | `0x00180860` | `0x001808b0` | unique 32-byte match |
| Bone-servo slot B | `0x00192968` | `0x001929c0` | unique 64-byte match |
| Bone-servo slot C | `0x001815d8` | `0x00181628` | unique 48-byte match |
| Trans dirty/world helper | `0x001dd748` | `0x001dd7a0` | unique 64-byte match |
| CharHair follow helper | `0x0017d658` | `0x0017d6a8` | unique 64-byte match |
| IK/scheduler helper | `0x0017a080` | `0x0017a0d0` | unique 48-byte match |
| Hand/prop helper | `0x001823c8` | `0x00182418` | unique 96-byte match |

First GH80s live trace should use these PAL addresses and prove actual
active-song ownership/call order before any GH80s conclusion is imported back
into the native loader.

GH80s PAL static anchors were dumped to
`gh80s_pal_static_anchors_20260612.json`. The file records local string-anchor
addresses for character and venue/camera/light systems including
`CharClipSamples`, `CharClipSet`, `CharDriver`, `CharDriverMidi`,
`CharForeTwist`, `CharUpperTwist`, `CharHair`, `CharLookAt`, `CharEyes`,
`CharServoBone`, `CharPosConstraint`, `CamShot`, `WorldCrowd`, and
`LightPreset`. The string scan is static support only; runtime table/function
semantics still require PCSX2 live evidence.

Accepted GH80s PAL boot probe:
`gh80s_pal_boot_probe_20260612.json` / `.window.png` ran `SLES_548.59` from
boot with no savestate, installed the PAL trace hooks, and retained 1,460 calls
all in `trans_dirty_world_001dd7a0`. The screenshot proves the game was stopped
at the first-run `SAVE NOT FOUND` prompt, so this is boot/setup evidence, not
active character playback. The mismatched `SLUS-21374 (960C7892)` savestates
cannot be used as GH80s PAL active-song proof.

GH80s PAL setup/navigation update:

- Current PCSX2 keyboard config maps pad `Cross = Keyboard/L`,
  `Start = Keyboard/Space`, `L2 = Keyboard/I`, `R2 = Keyboard/P`, and
  d-pad/left-stick movement to `W/A/S/D`.
- `probe_pcsx2_retry_input.py` now supports `--no-state`; the earlier
  no-state-less probe was invalid because it only showed PCSX2's missing
  savestate dialog.
- A valid background-only probe created the PAL save file and reached the title
  flow. Do not use `--focus-window` again for this thread; the working route is
  background `PostMessage` only.
- `trace_pcsx2_call_sequence.py` now supports `--background-input`, reacquires
  the PCSX2 window before each delayed input, permits `--retry-pulses 0`, and
  uses a 0.35s hold for background keys. This avoids foregrounding PCSX2 while
  still letting GH80s consume menu input.
- Proven no-foreground menu route from boot with the PAL save present:
  `L` at the guitar-controller prompt -> `L` at title -> `L` at Quick Play ->
  `L` on the first song -> `L` on the default difficulty -> wait for loading /
  venue playback.

Accepted GH80s PAL active-song trace:
`gh80s_pal_active_song_trace2_20260612.json` / `.window.png`. This was a
headless `-nogui` run using `--background-input`; PCSX2 was not foregrounded.
The screenshot is real in-song gameplay for `(Bang Your Head) Metal Health`.
The trace captured 964,158 total calls over 35 seconds and retained a full
32,768-record ring:

- `chardriver_inner_update_00173808`: 58
- `chardriver_per_frame_00171880`: 408
- `chardriver_selector_00171e00`: 2
- `foretwist_update_001756c8`: 88
- `hair_update_00177008`: 29
- `clip_eval_0016b220`: 466
- `clip_apply_0016b340`: 841
- `clip_final_0016abd8`: 841
- `clip_output_writer_00168370`: 2,299
- `trans_dirty_world_001dd7a0`: 27,386
- `hair_follow_0017d6a8`: 58
- `ik_scheduler_0017a0d0`: 58
- `hand_prop_00182418`: 234

Zero-hit in this accepted active window: the sampled bone-servo slots
`0x001808b0`, `0x001929c0`, and `0x00181628`, plus event/normal branch
`0x0010c9a0` / `0x0010b810`. Treat those as wrong-window or non-active-path
for this slice, not as absent from GH80s.

Sampled live ownership from the active GH80s trace:

- `chardriver_per_frame_00171880` resolved live `main.drv`, `left_hand.drv`,
  `right_hand.drv`, `upperTwist_L.ik`, and `upperTwist_R.ik`.
- Owner/source rows include `char/alterna1/og/alterna1.milo`,
  `char/metal_singer/og/metal_singer.milo`,
  `char/metal_bass/og/metal_bass.milo`,
  `char/metal_drummer/og/metal_drummer.milo`, and crowd actors
  `char/crowd/og/crowd_female01..04.milo` /
  `char/crowd/og/crowd_male01..04.milo`.
- `foretwist_update_001756c8` sampled `foreTwist_L.ik` and
  `foreTwist_R.ik`.
- `hair_update_00177008` sampled `bangs.hair`.
- Clip eval/apply samples expose hand event strings such as
  `set_hand clap` and `set_hand devil`, confirming live performance hand
  event paths in the GH80s run.

GH80s character coverage is accepted as sufficient for this audit pass. The
goal was to broaden format understanding beyond GH2/GH1 and confirm the
specific areas of concern; do not reopen GH80s just to chase additional
song/character breadth unless a later native implementation bug points back to
it.

Rejected / limited GH80s second-song attempts:

- A background route probe with temporary `LDown = Keyboard/G` proved one-row
  setlist navigation to `We Got The Beat` without foregrounding PCSX2.
- `gh80s_pal_wegotbeat_clean_trace_20260612.json` is rejected as active
  character evidence: its final screenshot was still on the setlist and it
  retained only `trans_dirty_world_001dd7a0` calls. Do not count it as a clean
  `We Got The Beat` performance trace.
- Earlier `gh80s_pal_wegotbeat_trace_20260612.json` is accepted only as another
  GH80s active gameplay window if needed; the route sent `S` before the
  setlist and did not prove a clean second song selection. Treat
  `gh80s_pal_active_song_trace2_20260612.json` as the primary GH80s active
  character trace until a cleaner song-selection state is created.

Accepted GH80s PAL venue/camera/lighting traces:

- `gh80s_pal_venue_camera_lighting_body_map_20260612.json` maps the first
  GH2 venue/camera/lighting seed functions to GH80s PAL with the same exact
  body matcher used for the accepted character map. Unique matches include
  CamShot eval `0x002665a0 -> 0x00266600`, CamShot apply bridge
  `0x0026ae00 -> 0x0026adc8`, camera setter `0x001b1ee0 -> 0x001b1f38`,
  lighting set `0x00271288 -> 0x00271250`, lighting next
  `0x00271200 -> 0x002711c8`, timing read `0x002c6808 -> 0x002c6288`,
  script/list helpers `0x002b6238 -> 0x002b5c18`,
  `0x002b31b0 -> 0x002b2b90`, `0x002b3818 -> 0x002b31f8`,
  `0x002b3d50 -> 0x002b3730`, and world event/game helpers
  `0x00123d08 -> 0x00123d20`, `0x001239d0 -> 0x001239e8`,
  `0x001243f8 -> 0x00124410`, `0x00124310 -> 0x00124328`.
  Non-unique entries in that JSON are not accepted runtime targets by
  themselves.
- `gh80s_pal_lighting_cluster_static_candidates_20260612.json` records the
  neighboring lighting cluster check against the proven `set` / `next`
  functions. Candidate PAL addresses
  `0x00271680`, `0x002716e0`, `0x00271740`, `0x00271f38`,
  `0x00280f28`, `0x00280fb0`, and `0x00281038` preserve the GH2 control-flow
  shape with address-bearing operands adjusted. Treat them as static
  candidates until live hits prove a branch.
- `gh80s_pal_venue_camera_lighting_trace3_20260612.json` / `.window.png` is
  accepted active in-song evidence. It was launched `-nogui` with
  `--background-input`; no foreground/click path was used. The trace enabled
  on the difficulty screen, posted confirm in the background, then captured a
  60-second window through load and performance. Screenshot shows active
  `battle` venue gameplay with band, note highway, and lighting visible.
  Total traced calls: 98,489; retained ring: 32,768. Retained counts:
  `chardriver_per_frame_00171880` 17,006,
  `camera_setter_001b1f38` 10,806,
  `camshot_eval_00266600` 1,238,
  `camshot_apply_bridge_0026adc8` 1,196,
  `lighting_set_handler_00271250` 1,
  `script_eval_002b5c18` 156,
  `script_filter_002b2b90` 1,298,
  `script_list_002b31f8` 12,
  `script_pick_new_002b3730` 110,
  `world_event_00123d20` 2,
  `world_event_001239e8` 1,
  `world_game_00124328` 1,
  and `world_onebar_ref_00122c5c` 1.
- Object samples from the accepted camera trace prove the live camera output
  chain: CamShot eval works through the global/output row at `0x00492000`;
  CamShot apply uses object `0x00b4b950` and points into the live camera rows;
  camera setter writes two active camera/output rows, `0x00b4ba50` and
  `0x00b4fe70`, each containing matrix/vector-style float blocks and live
  scene pointers. The CamShot apply sample also resolves through a venue/crowd
  branch (`crowd` at `0x00b4ba24`).
- The same accepted trace samples world event object `0x00ab2a10`, which
  resolves active venue strings/rows including `crowd_audio` at `0x006c0cc0`
  and `world/battle/streams` at `0x00777870`. This ties the camera/lighting
  trace to the live `battle` venue seen in the screenshot.
- `gh80s_pal_lighting_candidate_trace_20260612.json` / `.window.png` is
  accepted active lighting branch evidence. It used the same background-only
  difficulty-confirm route and captured an in-venue performer camera frame.
  Total and retained calls: 5,374. Runtime-proven lighting counts:
  `lighting_set_handler_00271250` 9,
  `lighting_prev_handler_candidate_00271680` 11,
  `lighting_prev_alt_candidate_002716e0` 5,
  `lighting_next_apply_candidate_00280f28` 11,
  `lighting_prev_apply_candidate_00280fb0` 5,
  plus `script_eval_002b5c18` 338,
  `script_filter_002b2b90` 4,988,
  and `world_event_00123d20` 7. Zero-hit in this accepted lighting window:
  `lighting_next_handler_002711c8`, `lighting_first_candidate_00271740`,
  `lighting_advance_candidate_00271f38`,
  `lighting_first_apply_candidate_00281038`, and
  `timing_read_002c6288`.
- Lighting samples show the set/prev handlers receiving stack/script rows
  around `0x01ffe...`; the apply functions receive the global lighting target
  row `0x00520000`. Do not name individual color fields yet; this trace proves
  branch identity and dispatch order, which is sufficient for the GH80s
  comparison pass.
- `trace_pcsx2_call_sequence.py` now supports `--delta-sample-a0`, which
  samples live deltas for unique `a0` rows observed in the same trace process.
  Use this for heap-backed camera/venue rows; do not reuse camera row
  addresses across fresh PCSX2 launches.
- `gh80s_pal_camera_lighting_sameprocess_delta_20260612.json` / `.window.png`
  is accepted active camera-row delta evidence. It used the same
  background-only difficulty-confirm route and captured active gameplay.
  Counts: CamShot eval 487, CamShot apply bridge 486, camera setter 4,376,
  lighting set 2, script eval 27, script filter 2,201, world event helpers
  1 each. Same-process sampled rows:
  `camshot_apply_bridge_0026adc8` saw `a0=0x00b0b320`, with 7 changed words
  in the delta window, including float-like camera/vector fields at
  `0x00b0b440..0x00b0b458`;
  `camera_setter_001b1f38` saw rows `0x00ab67e0`, `0x00b0f7e0`, and
  `0x00b0b420`; row `0x00b0f7e0` changed 37 words with matrix/vector-style
  fields beginning at `+0x20`, while the other two rows stayed stable in this
  window. World event row `0x00aa7f80` stayed stable after dispatch.
- `gh80s_pal_lighting_sameprocess_delta_20260612.json` / `.window.png` is
  accepted active lighting target evidence. It reproduced the lighting branch
  hits in one process: set 9, prev 11, prev-alt 5, next-apply 11, prev-apply
  5, script eval 420, script filter 5,077, world event 7. Both lighting apply
  branches used `a0=0x00520000`; same-process delta sampling over 8 seconds
  found 0 changed words in that row. Interpretation: this trace proves the
  live global lighting destination row for those apply branches. Per-field
  color/vector names remain an implementation-time follow-up if needed, not a
  blocker for closing the GH80s trace pass.

Rejected GH80s PAL venue/camera/lighting attempts:

- `gh80s_pal_venue_camera_lighting_trace_20260612.json` and
  `gh80s_pal_venue_camera_lighting_trace2_20260612.json` are rejected as
  active venue evidence. Their screenshots remained on the difficulty screen
  and retained only `generic_compare_002c17d8` calls. They are useful only for
  route timing diagnostics.
- Earlier cross-process row samples of `0x00b4...` camera addresses were
  removed. Those addresses were heap rows from a prior PCSX2 process and are
  not authoritative in a fresh launch; use the same-process delta traces above.

GH80s closeout status: closed for this trace phase by user decision after the
accepted character, camera, venue, and lighting evidence above. Return to the
main trace plan rather than continuing GH80s breadth work.

GH2 Battle Spotlight color runner follow-up (2026-06-22):

- `pcsx2_color_runner_scale_20260622_current.json` is accepted in-song Battle
  evidence for the PS2 Spotlight color path. The traced chain is
  `0x00275ee0 -> 0x0026f378 -> 0x003a9170/0x003a8f80`, with
  `0x00275ee0` storing the source RGB into each Spotlight object and applying
  `f12` as the scalar/fade. The run retained 253 `color_scale_commit_00275ee0`
  calls, 253 `color_runner_0026f378` calls, 86 replace calls, and 85 update
  calls.
- `pcsx2_color_runner_objects_20260622_current.json` maps the eleven live
  object addresses to Battle Spotlight names:
  `basketball01/02/03_spotlight.spot`, `left_round01/02/03_spotlight.spot`,
  `right_round01/02/03_spotlight.spot`, `square01_spotlight.spot`, and
  `SHADOW_light.spot`.
- The trace source vectors match raw `battle_lighting.milo_ps2` object defaults
  for the non-target special spotlights. `square01_spotlight.spot` commits
  RGB `(1.0, 1.0, 0.878431)`, which is stored eight bytes after its first
  post-parent `.grp` string. `SHADOW_light.spot` commits
  `(0.105882, 0.105882, 0.258824)`, stored four bytes after its first
  post-parent performer token. Targeted round/basketball spots receive runtime
  `LightPreset` target-state colors and scalar fades through the color manager;
  do not treat their aim/template float runs as object default RGB.
- `pcsx2_env_color_consumer_20260622_resume.json` is accepted active-song
  follow-up evidence for the color-list helper side. It used the prepatched
  state route with background input and retained an active gameplay screenshot.
  Counts: `color_interp_find_003a8b88` 86, `color_interp_replace_003a8f80` 86,
  `color_interp_update_003a9170` 85, and `color_interp_apply_003a8e38` 44. The
  new `0x003a8e38` hits come from `0x003a8f80` and carry the global color-list
  row `0x00b784c4` plus stack color rows; this closes that helper as internal
  color-list maintenance, not as a final renderer or dynamic Environ light
  consumer. Do not enable native `GHOGX_ENABLE_ENVIRON_DYNAMIC_LIGHTS` from
  this trace.
- Local JSON analysis of the accepted consumer trace confirms the helper shape:
  all 44 `0x003a8e38` calls have `ra=0x003a9154`, `a0=0x01ffe7d0`,
  `a1=0x00b784c4`, and `a3=0x01ffe7e0`. The stack payloads carry normalized
  RGB triples, while `0x003a9170` / `0x003a8f80` keep rotating list slots such
  as `0x007fe790`, `0x00782580`, and `0x00845ca0` inside the same
  `0x00b784c4` color-list family. Treat this as color-list plumbing only.
- `pcsx2_lighting_color_consumer_snapshot_20260622_current.json` and
  `pcsx2_lighting_color_consumer_noretry_20260622_current.json` are rejected as
  active renderer-light evidence. Their retained screenshots are SONG FAILED /
  retry-menu captures rather than active gameplay; the no-retry run records
  zero calls, and the snapshot run must not supersede the accepted
  `pcsx2_env_color_consumer_20260622_resume.json` route.
