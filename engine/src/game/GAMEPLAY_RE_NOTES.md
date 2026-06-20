# Gameplay RE Notes

## Venue Camera

- Rexglue traces show the venue director owns camera, crowd, lighting, and band
  updates together. Camera shot selection is a two-tier system:
  `pick_new_shot` / `start_shot` at roughly bar-scale cadence, plus
  `post_switch_cam` for intra-shot camera position changes.
- `CamShot` / `BandCamShot` data contains a path-frame camera transform and an
  optional focus/source object. Accepted PS2 traces split this from the final
  render-camera result row. Native now resolves body-bone source shots by
  adding the live body-bone target position to the decoded path-frame eye; empty
  targets and prop/spot targets remain authored-world/aim-only until their
  source transform branch is mapped.
- Evidence from hidden native captures:
  - `engine/out/codex_native_yyz_f500_regular_camera_20260614.bmp` used an
    older broad target-relative eye path and rendered a clipped floor-level
    shot through foreground venue geometry.
  - `engine/out/codex_native_yyz_f500_camera_absolute_eye_20260614.bmp` kept
    the `flr_far_lft02` eye at the decoded venue-space pose and used
    `guitarist0` only as the aim target; the resulting native frame showed the
    YYZ guitarist, bassist, drummer, and keyboardist in the theatre venue.
  - `engine/out/codex_native_shout_f1300_clean_band_20260614.bmp` validates
    the same interpretation on the arena venue with singer, bassist, drummer,
    and `glam1` guitarist visible in a regular camera shot.
  - `engine/out/camera_bone_source_20260614/small1_psychobilly_f900_bone_source.bmp`
    validates the body-bone source branch on small1 `flr_near_lft01`: the
    previous authored-world eye was below/inside foreground tunnel geometry at
    `(-34.12, -51.35, -46.39)`, while the body-bone result eye is
    `(-90.11, -29.52, 13.92)` aimed at the live `bone_neck.mesh` target.
  - `engine/out/camera_bone_source_20260614/theatre_yyz_f500_bone_source.bmp`
    and `engine/out/camera_bone_source_20260614/arena_shout_f1300_bone_source.bmp`
    guard the conservative rule: empty-target `flr_far_lft02` and prop/spot
    target `flr_near_lft01x13w` stay aim-only and do not receive body-bone
    source translation.

Open work:

- Camera pose decoding is still heuristic and should be traced further against
  Rexglue/PS2 `CamShot` object layout before declaring exact parity. With
  `GHOGX_DEBUG_CAMERA=1`, native logs emit `camera-candidate` rows for each
  plausible decoded `CamShot` pose and final `[camera]` rows for the selected
  render-camera eye/aim/up.
- 2026-06-20 CamShot layout pass: PS2 object data and the local
  `world_objects_ps2.dta::CamShot` schema place `field_of_view` immediately
  before the decoded camera transform. Native now decodes that value from
  `pose_offset - 4` and interpolates it into the runtime camera. YYZ
  `flr_near_rt03` validates the decode with FOV `0.746733`, but the resulting
  native frame is still not camera-parity because the same key also carries
  `screen_offset=(0.618077,-1.030767)` at `pose_offset + 0x30/+0x34`, and
  native has not yet mapped the exact PS2 screen/projection application.
- 2026-06-20 CamShot screen-offset decode/application pass:
  native now carries the two `screen_offset` floats from `pose_offset + 0x30`
  and `+0x34` through `CameraKey`, interpolates them with the selected
  CamShot, and applies them to the authored projection matrix through the
  accepted render-camera/screen family scale that traces showed carrying
  stable `768.0` values. Validation folder
  `engine/out/codex_goal_20260620_cam_screen_offset_yyz/` keeps a YYZ A/B with
  `GHOGX_DISABLE_CAMERA_SCREEN_OFFSET=1`; the active `flr_near_rt03` guard logs
  raw `screen_offset=(0.618077,-1.030767)`, but the 768-scaled projection nudge
  is sub-pixel at the retained frame and the screenshots are byte-identical.
  Treat this as structural field plumbing, not as final camera parity; exact
  render-camera output-row semantics still require a focused trace if a native
  camera mismatch depends on this field.
- Empty-target regular shots still use authored world-space pose/basis only.
  Do not invent targets for them without object-layout or runtime trace proof.
- Theatre `flr_near_lft03x1` is the concrete guard case for that rule: the PS2
  `CamShot` body has normal `flr_near_lft` metadata and decoded camera poses,
  but no performer/source target string. Native must keep it as an
  authored-world/aim-only shot, not skip it and not invent `guitarist0`.

## Performer Role Routing

- `songs.dta` band order is not a fixed singer/bassist/drummer tuple. YYZ uses
  `(band metal_bass metal_drummer metal_keyboard)`, so role assignment must be
  symbol-classified instead of positional.
- Community `char_objects.dta` defines `keyboardist` with `keyboard_parser` and
  `start_flags kStartSinger`. `world_objects_worldbase.dta` checks
  `{exists keyboard}` alongside `{exists singer}` for band shadow visibility.
- Native implementation:
  - Classifies `metal_bass`, `metal_drummer`, `metal_keyboard`, and singer
    symbols by name before constructing performers.
  - Loads keyboardist as native role `keyboard`, event track `BAND KEYS`,
    model `char/metal_keyboard/og/gen/metal_keyboard.milo_ps2`, clips
    `keyboard_idle` and `keyboard_active_medium`, and start flag `4`.
  - Leaves keyboard hand/foretwist/hair/look-at special cases out of the native
    route, matching accepted YYZ trace evidence that only `main.drv` and
    upper-twist fired for `metal_keyboard` in that captured window.
- Native evidence:
  - `engine/out/codex_native_yyz_f360_keyboard_roles_20260614.log` shows
    `funk1` as `guitarist0`, `metal_bass` as `bassist`, `metal_drummer` as
    `drummer`, and `metal_keyboard` as `keyboard` with `BAND KEYS`.
  - `engine/out/codex_native_yyz_f500_clean_keyboard_20260614.bmp` and `.log`
    show the keyboardist present in the native theatre stage route, driven by
    `keyboard_active_medium`, with `BAND KEYS` transitioning to `[play]`.
  - `engine/out/native_song_20260614/crazyonyou_f900_female_singer_validation.bmp`
    and `.log` validate a native female-singer route in the fest venue. The
    quickplay rig resolves `(band metal_bass metal_drummer female_singer)`;
    `female_singer` loads as role `singer` with the expected format shape:
    `2 upperTwist`, `1 hair` (`dreads.hair`), and no `CharForeTwist`,
    `CharIKHand`, `CharIKMidi`, `CharLookAt`, or `CharEyes`. The active clip
    `singer_active_medium_01` loads from the female singer `singer_main`
    animation MILO, matching the accepted PS2 trace note that female singer
    should not inherit male-singer look-at/foretwist assumptions.

## Character Runtime Trace

- PCSX2 runtime tracing is currently usable through live object vtable
  redirection with posted Retry input and `PrintWindow` capture. This path does
  not require foregrounding PCSX2.
- Accepted rerun:
  `analysis/ps2_trace/pcsx2_anim_vtable_trace_20260608_character_exact_rerun.json`
  from the sibling `GuitarHeroOGX-trace360` repo.
- The run redirected exact in-song object vptrs only after leaving Retry. The
  after-Retry screenshot shows the venue, and the final screenshot can land on
  the fail screen because the scripted trace does not play notes.
- Nonzero in-song update evidence from the rerun:
  - `CharForeTwist` family: vtable `0x003e77a8`, slot `0x0c`,
    function `0x00175678`, `42` calls.
  - `CharUpperTwist` family: vtable `0x003e8030`, slot `0x0c`,
    function `0x001823c8`, `42` calls.
  - `CharHair` family: vtable `0x003e77e8`, slot `0x0c`,
    function `0x00176fb8`, `21` calls.
  - Right-eye look-at path: vtable `0x003e7c28`, slot `0x0c`,
    function `0x0017d658`, `21` calls.
- Native implication: arm spaghetti should be fixed through traced
  `CharForeTwist` / `CharUpperTwist` object parsing and bone-feed execution,
  not by changing common clip sampling or broad bind-pose heuristics. Detached
  hair and eye placement should likewise be driven by the traced `CharHair` and
  `CharEyes` / `CharLookAt` object paths.

Open work:

- Dump and map `0x0017d658` against `CharEyes` / `CharLookAt` community object
  behavior before implementing eye placement fixes.
- Camera/lighting candidate vtable trace had valid redirects but no nonzero
  calls in that run, so it is not accepted as runtime proof yet.

2026-06-16 LightPreset decode note:

- Arena `*_lighting.milo_ps2` `LightPreset` bodies store keyframe mesh-target
  rows before each keyframe description label, but the `.spot`, `.env`, and
  `.lit` object tables appear after the final keyframe record. Checked arena
  presets consistently expose 51 spot refs, 12 env refs, and 12 lit refs in
  that tail table. Native therefore keeps mesh target states per keyframe and
  decodes spot/env/lit refs as preset-level lighting graph membership rather
  than per-keyframe fields.
- Target rows still choose and aim active spotlights. The preset spot table is
  used as a data-backed guard for direct/inferred spotlight activation, avoiding
  named venue or song rules.

2026-06-14 native validation:

- `engine/out/native_song_20260614/yyz_f900_med_default_fullband_keyboard.bmp`
  loads the `yyz` quickplay rig (`funk1`, theatre, `metal_bass`,
  `metal_drummer`, `metal_keyboard`) through the no-env default native route.
  The log shows `metal_keyboard` model and `keyboard_active_medium` clip loaded,
  keyboard performer MIDI entering `[play]`, theatre venue load, and lighting
  preset changes from `blackout.pst` to `chorus_okay.pst`.
- `engine/out/native_song_20260614/yyz_f900_after_world_matrix_fix_autostart.bmp`
  and `.log` revalidate the same native YYZ route after fixing
  `Scene::world_matrix`: all 15 tests pass, `metal_keyboard` still loads as the
  keyboard performer, and the theatre stage/camera/lighting composition matches
  the prior accepted capture.
- `metal_bass` in stock GH2 PS2 has only
  `char/metal_bass/anims/gen/bass_main.milo_ps2`; ARK listing shows no
  `bass_strum` or `bass_fret` MILO. Its graph also has `0 ikHand` and
  `0 ikMidi`, so the native bassist route should use `bass_main` active clips
  plus the bass prop attachment rather than forcing guitar-style hand IK or
  nonexistent overlay clips.
- Drum-kit route: native decodes `dw_<venue>_drums.milo_ps2` `TransAnim`
  entries and EventTrigger -> AnimFilter -> mesh targets. For `dw_theatre`,
  accepted native log rows show `kick.tnm`, `hat.tnm`, and `snare.tnm`, plus
  trigger events `kick_drum`, `hit_snare`, `hit_hihat`, and `crash_symbal`.
  Stock GH2 PS2 `BAND DRUMS` MIDI samples (`yyz`, `shoutatthedevil`,
  `crazyonyou`, `woman`) only use pitches `36` and `37`. Local
  `config/midi_parsers.dta::drummer_kick_drum` maps them as
  `36 -> kick_drum` and `37 -> crash_symbal`, which native now follows.
  `hit_snare` and `hit_hihat` remain accepted drummer messages from
  `char_objects_ps2.dta`, but do not invent MIDI pitch mappings for them until
  the script/parser row that emits those events is mapped.
  Validation:
  `engine/out/native_song_20260614/shout_f1800_drum37_crash_validation.log`
  shows tick `22560` emitting `kick_drum` for pitch `36` and `crash_symbal`
  for pitch `37`; the matching `.bmp` renders the arena venue cleanly at the
  later in-song frame.
- `config/midi_parsers.dta::speaker_pulse` maps `BAND BASS` pitch `36` to
  `{handle (world bass_hit)}`. Native now extracts those bass parser cues and
  dispatches `bass_hit` as a transient venue event so it does not overwrite the
  persistent excitement state. As of this note, decoded GH2 arena/theatre venue
  geometry does not expose a native `bass_hit` material/filter route, so this is
  event plumbing for the accepted parser behavior rather than a freehand speaker
  pulse. Validation:
  `engine/out/native_song_20260614/yyz_f420_basshit_validation.log` reports
  `1092` bass cues and emits early `bass_hit` events from pitch `36`; the
  matching `.bmp` confirms the theatre venue still renders during dispatch.
- Prop attachment now follows the same moving local-chain Trans rows as the
  skinned performer output and camera targets. This is backed by the accepted
  prop/lower-body traces that show `bone_pos_guitar.mesh`,
  `bone_pos_gutbass.mesh`, and `bone_pos_mic.mesh` as moving attachment rows,
  not static guessed prop placements. Native attaches regular guitars to
  `bone_pos_guitar.mesh` and bass props to `bone_pos_gutbass.mesh`.
  Validation:
  `engine/out/native_song_20260614/shout_f900_prop_bass_gutbass.log` shows
  `flyingv_v2` attached to `bone_pos_guitar.mesh` and `bass_music_black`
  attached to `bone_pos_gutbass.mesh`; prop debug rows show both attachment
  positions changing across frames. The matching `.bmp` renders the arena route
  after the change.
- The singer mic stand is not a separate native prop load in stock GH2 PS2.
  `char/metal_singer/og/gen/metal_singer.milo_ps2` contains `mic_stand.mesh`,
  `mic.tex`, `obj_mic_stand.mat`, and `bone_pos_mic.mesh`. Native mesh debug in
  `engine/out/native_song_20260614/shout_f10_singer_mesh_debug.log` shows
  `mic_stand.mesh` as a rigid, palette-free mesh parented directly to
  `bone_pos_mic.mesh`, so it should continue through the generic rigid
  mesh-world path rather than a guitar-style external prop attachment.
- Venue `EventTrigger` payloads are not safe to interpret by filter-name
  substrings. `small1` excitement triggers store an event label, a filter ref
  list, then explicit draw-group show and hide lists. Native now decodes those
  show/hide group lists and applies them to meshes instead of hiding every
  filter whose name contains `_bad`. Validation:
  `engine/out/native_song_20260614/psychobilly_f900_trigger_visibility.log`
  shows `excitement_okay` applying `show=12 hide=12` and later
  `excitement_bad` applying `show=11 hide=12`, matching the decoded trigger
  body shape from `world/small1/og/gen/small1_geom.milo_ps2`.
- `AnimFilter` is decoded separately as an animation-frame route. Native only
  applies venue filter transforms when an `AnimFilter` target resolves through
  a decoded `TransAnim` translation key path; small1 excitement filters do not
  currently resolve through that translation subset, so they remain documented
  evidence gaps rather than guessed movement.
- Native camera mesh proximity logging
  (`engine/out/native_song_20260614/psychobilly_f900_camera_mesh_probe.log`)
  identified the old `psychobilly` frame-900 occluder as the `tunnel.*`
  venue shell around the authored `flr_near_lft01` camera, not a character,
  prop, or excitement group. A `GHOGX_CULL_CCW=1` probe removes that slab in
  `engine/out/native_song_20260614/psychobilly_f900_cull_ccw_probe.bmp`, but
  cross-venue captures
  `engine/out/native_song_20260614/shoutatthedevil_f1300_default_cull_ccw.bmp`
  and `engine/out/native_song_20260614/yyz_f900_default_cull_ccw.bmp` reject a
  global cull flip because arena/theatre visibility regresses badly. Keep the
  default cull mode broad-route compatible; solve small1 tunnel coverage with
  shot/mesh-specific evidence, not a renderer-wide winding change.
- `world_objects_worldbase.dta::intro_start_msg` sets `camera_bars_left` to
  six bars before normal camera selection. Native was logging
  `intro camera window: 9.722s` for `psychobilly` but still picking a regular
  shot at `t=0.017` because `camera_keys_` was empty and the intro branch fell
  through to regular selection. Native now gates regular shot selection on the
  intro window itself, not on whether an intro camera decoded successfully.
  Validation captures after the fix:
  `engine/out/native_song_20260614/psychobilly_f900_camera_script_filter.log`
  first picks a regular shot at `t=9.733`, `bar=6`;
  `engine/out/native_song_20260614/shoutatthedevil_f1300_camera_script_filter.log`
  first picks at `t=15.317`, `bar=6`; and
  `engine/out/native_song_20260614/yyz_f900_camera_script_filter.log` first
  picks at `t=6.550`, `bar=6`.
- Native now decodes CamShot boolean properties from the packed object body
  rather than relying on neighboring strings. The observed layout is
  `len + key + u32 tag(0) + u32 bool`, confirmed on `flr_near_lft01` for
  `walk_ok=1`, `low_excitement_ok=1`, and `starpower_ok=0`. Regular camera
  selection applies the same conditional filters as the local script:
  `low_excitement_ok` only when the current venue excitement is low, and
  `walk_ok`/`starpower_ok` only when native has accepted runtime state for
  those performer conditions. Walking and starpower are currently false until
  their runtime state bridge is traced.

### Character Controller Layout And Transform Feed

Authoritative sources used for this section:

- Community object docs:
  `_community_re/Guitar-Hero-II-Deluxe-Unified/_ark/char/char_objects_ps2.dta`.
- Exact active-song PCSX2 trace:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_anim_vtable_trace_20260608_character_exact_rerun.json`.
- Exact active-song object samples:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_sample_object_words_character_exact_20260608.json`
  and
  `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_sample_foretwist_refs_20260608.json`.
- Helper-function dump:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/ps2_function_snippets_20260608_helpers.json`.

The live vptr addresses captured by the vtable trace point four bytes into the
controller object. The actual PS2 object base is `vptr_addr - 4`; the first word
at that base is an owner/list pointer, and the primary class vtable is at
`base + 0x04`.

`CharForeTwist` exact layout from the sampled left/right objects:

- `base + 0x04`: `0x003e77a8` primary vtable.
- `base + 0x0c`: first embedded object-reference wrapper vtable.
- `base + 0x14`: `hand` Trans wrapper pointer.
- `base + 0x18`: second embedded object-reference wrapper vtable.
- `base + 0x20`: `twist2` Trans wrapper pointer.
- `base + 0x24`: authored offset float, `+90.0` left and `-90.0` right in the
  active sample.

`CharUpperTwist` exact layout from the sampled left/right objects:

- `base + 0x04`: `0x003e8030` primary vtable.
- `base + 0x0c`, `base + 0x18`, `base + 0x24`: embedded object-reference
  wrapper vtables.
- `base + 0x14`: `upper_arm` Trans wrapper pointer.
- `base + 0x20`: `twist1` Trans wrapper pointer.
- `base + 0x2c`: `twist2` Trans wrapper pointer.

The common helper at PS2 `0x001dd748` is a Trans dirty propagation routine, not
the interpolation math. It tests/writes `Trans + 0xa0`, sets it to dirty, and
recursively visits children through the list at `Trans + 0x18`.

The common helper at PS2 `0x003d8ea0` resolves a Trans world matrix. If dirty,
it clears `Trans + 0xa0` and updates world rows at `Trans + 0x60..0x90` from the
local rows at `Trans + 0x20..0x50`, either by copying them or by composing with
the parent depending on the mode at `Trans + 0xa4`.

The active-song samples show the character controllers mutating the referenced
Trans objects, not the controller object storage itself:

- Foretwist `hand` refs changed 15 float cells each, primarily local rows
  `+0x20..+0x38` and world rows `+0x50..+0x68`.
- Foretwist `twist2` refs changed 12 float cells each, including local
  roll-like cells around `+0x24/+0x28/+0x34/+0x38` and world rows
  `+0x50..+0x6c`.
- Uppertwist `upper_arm`, `twist1`, and `twist2` refs changed in the same
  Trans row regions; `upper_arm` had 17 changed cells, `twist1` 12, and
  `twist2` 16 in the sample.

Native implication:

- The current native twist path is still too approximate. It operates only on
  inferred bone locals and ignores the PS2 Trans dirty/world-resolution model.
- A correct implementation needs to reproduce the controller order:
  animation/IK feed into Trans locals, controller math mutates the referenced
  Trans locals, `0x001dd748`-equivalent dirty propagation marks descendants,
  then world transforms are resolved in `0x003d8ea0` order before skinning.
- Do not add a one-off `offset_degrees` fix or another broad roll heuristic
  until the specific PS2 `CharForeTwist` and `CharUpperTwist` interpolation
  math is fully mapped from `0x00175678` and `0x001823c8`.

PCSX2 note:

- Fresh `-state 1` invocations later failed with "Could not resolve path for
  indexed save state load"; direct `-statefile` loads the saved fail/retry
  screen but no longer receives posted Retry input. Those failed launches do
  not invalidate the earlier accepted active-song traces above, but new
  runtime traces should first reestablish deterministic active-song entry.

Rejected native probe:

- A scoped test changed native `CharForeTwist` so `foreTwist1` copied the
  animated forearm local transform before applying the existing roll split.
  Captures:
  `engine/out/codex_foretwist_feed_20260608/rockabill1_psychobilly_f900_foretwist_feed.bmp`
  and
  `engine/out/codex_foretwist_feed_20260608/metal1_freya_f900_foretwist_feed.bmp`.
- The change was reverted because it was not a clear visual improvement and did
  not prove the PS2 feed semantics. Do not reapply that shortcut without
  stronger mapping from `0x00175678`.

2026-06-15 resume validation:

- `engine/out/codex_resume_20260615/yyz_default_camera_recheck/yyz_default_f900.bmp`
  and `.log` recheck the default native YYZ route without the debug gameplay
  camera override. The log shows `funk1`, `metal_bass`, `metal_drummer`, and
  `metal_keyboard` loading, `BAND KEYS` entering `[play]`, `chorus_okay.pst`
  becoming active, drummer kick cues firing, and the regular camera sequence
  starting after the six-bar intro (`flr_far_lft02`, `post_switch_cam`, then
  later regular sweeps). Treat screenshots captured with
  `GHOGX_DEBUG_GAMEPLAY_CAMERA=1` as character-inspection evidence only; that
  env var intentionally replaces the authored camera and must not be used to
  judge camera parity.
- `engine/out/codex_resume_20260615/shout_default_band_recheck/shout_default_f1300.bmp`
  and `.log` recheck a singer-song route with default camera/lighting. The log
  shows `glam1`, `metal_singer`, `metal_bass`, and `metal_drummer` loading,
  singer/bass/drum MIDI state changes, lighting preset/keyframe changes,
  regular camera sweeps, `post_switch_cam`, and drum cues. The captured frame is
  a coherent native arena stage shot, but it is too wide to close detailed
  character fidelity.
- `engine/out/codex_resume_20260615/crazyonyou_female_singer_recheck/crazyonyou_f900.bmp`
  and `.log` recheck the female-singer route in the fest venue. The quickplay
  rig resolves `alterna1`, `metal_bass`, `metal_drummer`, and
  `female_singer`; the singer loads as role `singer`, uses
  `singer_active_medium_01`, and renders in the native venue alongside the
  guitarist, bassist, drummer, props, lighting presets/keyframes, and regular
  camera/post-switch events. The generic clip loader still logs failed fallback
  MILO path attempts before the generated PS2 path succeeds; treat that as log
  noise unless it blocks trace readability or hides a missing accepted clip.

2026-06-19 current native role-route validation:

- Use `> <log> 2>&1` for verbose native capture runs. PowerShell `2> <log>`
  alone can leave the native stdout stream attached to the tool and make a
  healthy bounded capture look hung under heavy logging.
- `engine/out/codex_goal_20260619_role_audit/yyz_fullband_authcam_f700.bmp`
  and `.full.log` recheck the no-singer instrumental route. The log resolves
  `funk1`, `metal_bass`, `metal_drummer`, and `metal_keyboard`, loads the
  theatre venue, theatre lighting, drum kit, drum triggers, and all performers.
  `BAND KEYS` enters `[play]`; drummer mode changes between allbeat and
  nosnare; kick cues fire; `chorus_okay.pst` activates; and regular camera
  sweeps begin after the six-bar intro.
- `engine/out/codex_goal_20260619_role_audit/yyz_keyboard_spine_f700.bmp`
  is debug-camera inspection only. It confirms the same YYZ route can isolate
  the keyboardist with the prop and active animation intact; do not use it for
  camera parity.
- `engine/out/codex_goal_20260619_role_audit/yyz_drummer_spine_f700.bmp`
  and `.full.log` confirm the drummer/kit route in the same active song window.
  The log shows `crash`, `hihat`, `kick`, and `snare` triggers mapped to kit
  meshes and repeated kick cues while the captured performer remains visually
  coherent behind the kit.
- `char_objects_ps2.dta::BandCharacter::guitarist.enter` chooses
  `kStartGuitarist0Mp` when a second guitarist exists, otherwise
  `kStartGuitarist0`. Native start-waypoint lookup now preserves that ordered
  flag fallback for `guitarist0` instead of treating the multiplayer start as
  the only valid performer root. Singer, keyboardist, bassist, drummer, and the
  drum kit keep their existing `start_flags` routes. Validation probes:
  `engine/out/codex_goal_20260619_start_probe_yyz_after.log` and
  `engine/out/codex_goal_20260619_start_probe_shout_after.log`.
  `engine/out/codex_goal_20260619_start_probe_sweep/summary.txt` extends the
  same hidden 3-frame route check across `small1`, `fest`, `battle`, and
  `arena` songs; every loaded performer reports a decoded start transform.
