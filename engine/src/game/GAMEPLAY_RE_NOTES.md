# Gameplay RE Notes

## Venue Camera

- Rexglue traces show the venue director owns camera, crowd, lighting, and band
  updates together. Camera shot selection is a two-tier system:
  `pick_new_shot` / `start_shot` at roughly bar-scale cadence, plus
  `post_switch_cam` for intra-shot camera position changes.
- `CamShot` / `BandCamShot` data contains a path-frame camera transform,
  keyframe target refs, and a separate parent/source object. Accepted PS2
  traces split this from the final render-camera result row. Native resolves
  moving source shots by composing the decoded path-frame eye/basis through the
  live parent transform; target refs are retained as runtime composition
  metadata and a fallback when no authored basis/quaternion exists.
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
- 2026-06-23 CamShot target/parent correction: local
  `world_objects_ps2.dta::CamShot` schema says keyframe `targets` are
  "Target(s) that the camera should look at", while `parent` is "Parent that
  the camera should attach itself to". Raw stock PS2 objects in
  `analysis/camshot_raw_probe_20260623_current/` match that layout after each
  decoded pose: Stone `band_POV01` pose `+0x19d` has target
  `guitarist0:bone_spine2.mesh` at `+0x1e9/+0x1f7` and an empty parent, so its
  path-frame eye must stay authored-world. Small1 `flr_near_lft01` pose
  `+0x14a` has target `guitarist0` at `+0x196` and parent
  `guitarist0:bone_neck.mesh` at `+0x1ac/+0x1ba`, which is the source for the
  live eye offset. Native now decodes pose-local target and parent refs from
  the CamShot keyframe tail and uses flat string inference only if that binary
  ref decode fails.
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
- 2026-06-23 small2 camera correction: `targets` now drive the runtime
  look-at whenever a decoded CamShot target exists, while `parent` remains only
  the source transform for the eye/path-frame offset. The previous basis-first
  path made small2 `band_POV02` and spine-parented `flr_near_rt03` look into
  foreground geometry even though the decoded target named the performer.
  Targeted shots also keep authored camera roll instead of inheriting performer
  bone roll through the `up` vector. Empty target shots still use authored
  basis. The existing source-traced cut between authored shot families remains
  intact; only same-shot `post_switch_cam` position changes use the short
  native interpolation path.
- 2026-06-22 CamShot neutral-basis scanner false-positive filter:
  Arena camera debug showed the heuristic sliding parser accepting exact
  neutral-basis rows (`forward=(0,1,0)`, `up=(0,0,1)`) as additional poses in
  CamShots that also contain non-neutral authored pose rows. Those rows caused
  `post_switch_cam` to switch into scanner artifacts such as
  `flr_near_lft01x12w` body `+0x28A` instead of staying on the real
  body `+0x1EF` pose. Native now tags exact neutral-basis candidates and drops
  them only when the same CamShot has at least one non-neutral pose; neutral-only
  CamShots are preserved. Validation:
  `analysis/native_validation/arena_camera_debug_repro_20260622_current/`
  versus
  `analysis/native_validation/arena_camera_neutral_filter_20260622_current/`
  keeps 40 decoded arena CamShots and 11 regular sweeps, reduces neutral-basis
  candidate rows from 14 to 1, and reduces bogus post-switch rows from 24 to 10.
  The focused small2 regression in
  `analysis/native_validation/small2_direct_intro_camshot_neutral_filter_regression_20260622_current/`
  still chooses `Intro01 -> CamShot:Intro01`, decodes 2 direct poses, exits `0`,
  and retains the valid neutral-only route shape. This is a parser false-positive
  removal, not final camera-composition signoff.
- 2026-06-22 regular CamShot source-filter ordering:
  `world_objects_worldbase.dta::pick_regular_camera_shot` builds a filter from
  current-shot `facing` / `distance` or, when no current shot exists, the
  venue `intro_camera_facing` / `intro_camera_distance`, then calls
  `pick_shot NORMAL_CAMSHOT_CATEGORIES`. Native had been sorting the regular
  CamShot pool with the intro policy as a score, which mixed the intro
  selector rule into the regular camera route. Native now preserves decoded
  MILO directory order for the regular camera pool and applies the intro
  policy only as the first-shot previous-camera fallback used by the source
  filter. If the full transition/state filter finds no candidate, native may
  relax the transition and state predicates, but it now keeps the authored
  mode/category predicate (`regular`, `solo`, `jump`, or `lighter`) instead of
  falling back to any CamShot. Validation:
  `analysis/native_validation/regular_camera_source_filter_shout_20260622_current/`
  runs stock PS2 `shoutatthedevil` hidden from `16.0s`; it logs ordered
  regular CamShots, selects `flr_near_rt01x23w` after intro because the
  source-shaped previous filter is `distance=near` / `facing=left`, continues
  through regular sweeps and `post_switch_cam`, captures a coherent arena
  frame, and records zero unsupported, miss, no-decoded, unresolved, or error
  rows. The fallback-tightened rerun
  `analysis/native_validation/regular_camera_source_filter_fallback_shout_20260622_current/`
  preserves the same source-shaped camera sequence and the same zero-negative
  health scan after the wrong-category fallback was removed.
- 2026-06-22 start-shot camera handoff follow-up:
  native Battle captures showed the extra native-only cross-shot camera blend
  flying through foreground venue geometry during a `band_POV03 -> farpower`
  change. A no-front PCSX2 sample,
  `analysis/ps2_trace/pcsx2_camera_result_rows_headless_retry_20260622_resume.json`,
  reran the accepted Battle state and sampled the mutable camera result rows
  (`0x00b92ef0`, `0x00b92f50`, `0x00b930e0`, `0x00b8e9d0`, and
  `0x00b8ea10`). The result family stayed on one authored camera for samples
  `0..53`, then switched directly to the next camera family at sample `54`
  and continued with only small live motion. That matches the script-level
  split between `start_shot` and `post_switch_cam`: a new shot family should
  cut to the selected authored shot, while same-shot `post_switch_cam`
  position changes remain the intra-shot transition path. Native now skips the
  synthetic 1.25s interpolation when `previous->name != current.name`, keeping
  the existing short blend only for same-shot position changes. Validation:
  `analysis/native_validation/battle_camera_cut_handoff_20260622_resume/`
  reruns stock PS2 `rockthistown` in Battle hidden with diagnostic autoplay,
  fixed `0.25s` steps, and camera/venue debug logs. It exits `0`, captures
  coherent Battle frames at `24`, `72`, and `144`, keeps the same-shot
  `post_switch_cam` blend rows, and the former problem frame at `72` now logs
  `a=farpower b=farpower t=0.000` instead of the old
  `band_POV03 -> farpower` cross-shot fly-through. The health scan records
  zero unsupported, miss, no-decoded, unresolved, missing, or real error rows;
  the only `error`/`failed` text hits are the PowerShell stderr wrapper and
  `failed=0` coverage summaries.
- Post-fix sweep:
  `analysis/native_validation/venue_lighting_route_sweep_after_camera_cut_20260622_bounded/`
  reruns the seven stock route representatives with hidden bounded native
  processes and `GHOGX_DEBUG_CAMERA` / `GHOGX_DEBUG_VENUE_FILTERS`. All seven
  routes exit `0` without timeout, report zero unsupported, miss, no-decoded,
  unresolved, missing, or error rows, keep venue/lighting animation samples and
  active lighting presets/keyframes, and log `140` camera rows each. A follow-up
  scan of all camera debug rows finds `0` cases where `a=<shot>` and `b=<shot>`
  names differ, so post-fix interpolation is limited to same-shot position
  changes.
- 2026-06-23 GH2DXu arena camera row relocation:
  the direct PS2 route for `arena/shoutatthedevil` with autoplay, save disabled,
  `glam1`, and `flying_v` reached active in-song gameplay and proved that the
  old retail/Battle result-row addresses (`0x00b92ef0`, `0x00b92f50`,
  `0x00b930e0`, `0x00b8e9d0`, `0x00b8ea10`) are static in this GH2DXu direct
  route. The same camera output family is relocated around
  `0x00cea830`/`0x00cea890`/`0x00ceaa20`/`0x00ceab20`, with the active
  screenshot and samples stored under
  `GuitarHeroOGX-trace360/analysis/ps2_trace/gh2dxu_arena_camera_*_20260623.*`.
  The mutable clusters mirror the accepted PS2 camera model: a result family,
  child/result rows, output/projection rows carrying repeated `768.0` screen
  fields, and a stable source/authored row. Example output translations in
  `0x00ceaa20` change from roughly `(94.7,-18.9,82.2)` to
  `(174.3,7.1,60.0)`, while the current native arena frame still drives the
  renderer from decoded static CamShot poses such as
  `flr_near_rt02_singer eye=(125,-353,-315)` and can look upward through venue
  rigging. Treat this as structural evidence for implementing the shared
  CamShot result/path/projection bridge, not as license to hardcode arena camera
  positions or restore synthetic cross-shot blending.
- 2026-06-23 rejected arena camera rerun:
  `analysis/pcsx2_trace/arena_camera_live_rows_20260623/` tried to resample the
  relocated GH2DXu camera rows together with live `current_shot` symbols, but
  the associated PCSX2 HWND capture was already on the `shoutatthedevil` fail
  menu at 14% complete and the row sampler recorded zero live changes. Do not
  use it as gameplay evidence. The temporary
  `GH2DXu_PS2_trace_arena_camera_live.iso` and staging folder were deleted
  immediately after the rejected capture.
- 2026-06-23 CamShot authored-basis correction: accepted PS2 camera result
  rows (`gdx_cam_output_00ceaa20` in
  `GuitarHeroOGX-trace360/analysis/ps2_trace/gh2dxu_arena_camera_relocated_rows_20260623.json`)
  carry a complete render-camera basis as `forward, position, right, up`, with
  the derived view rows using `right/up/forward` columns and 768.0 projection
  constants. The packed CamShot pose row uses the same axis order before the
  eye translation. Native now decodes CamShot row 0 as forward and row 2 as up,
  and preserves that decoded basis before falling back to direct target
  look-at. This avoids the previous shared failure mode where a low/offset
  camera pose was re-aimed straight at a performer target and could look up
  through arena geometry. This is a shared result-row bridge step, not a
  shot-specific clamp or singer-camera hack.
  Validation:
  `analysis/native_validation/camera_basis_target_fallback_arena_shout_20260623_current/`
  builds on the previous bad arena `_singer` shot and exits `0`; the retained
  frame no longer points upward through the rigging, but it is still low and
  occluded by foreground arena geometry, so final camera result-row composition
  remains open. `analysis/native_validation/camera_basis_crossroute_smoke_20260623_current/`
  runs small2, battle, big, stone, and theatre routes after the shared basis
  change; all exits are `0`, regular camera sweeps and lighting presets remain
  active, and no new temp ISO/staging artifacts were produced.

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

2026-06-20 lighting format follow-up:

- `LightPreset` target rows are packed after the target string, not before it.
  For a row whose length field starts at `pos`, `payload = pos + 4 + len`.
  The row then carries nine bytes of metadata followed by unaligned floats:
  `payload + 9` is the light amount, while `payload + 29/+33/+37` are RGB.
  The floats between those fields match target/position-style data and are not
  currently used as color. The previous native decoder read `pos - 36` and
  `pos - 16/-12/-8`, which produced zero intensities for active rows.
- `Spotlight` entries in `battle_lighting.milo_ps2` embed their Trans rows at
  stable raw offsets: local matrix at entry `+0x2a`, resolved world matrix at
  `+0x5a`, then the normal 9-byte Trans metadata and parent string. Native now
  skips `0x26` bytes after the version word before reading those matrices.
  Validation rows for `right_round03_spotlight.spot` decode to
  `world_pos=(132.86,172.39,138.81)` instead of zero/garbage.
- Spotlight object string order matters. If an object contains both
  `lightXX_target.mesh` and later decorative meshes, the first
  `_target.mesh` or `.Target.mesh` is the aiming target. Native preserves that
  target instead of overwriting it with later mesh refs.
- Render-state validation found a real D3D fixed-function edge: when a draw has
  no texture, both color and alpha must select diffuse. Leaving alpha as
  `texture * diffuse` makes null-texture debug/overlay draws contribute zero.
- Evidence is retained under
  `engine/out/codex_goal_20260620_lighting_real_after_alpha/`. With authored
  materials and no debug solid override, `woman`/Battle frame 500 now differs
  between spotlight instances on/off by 7,055 RGB pixels, bbox
  `(621,102)..(997,285)`. Frame 900 is unchanged for that camera/preset window.
  Treat this as proof that the shared lighting decode/render path is alive, not
  final lighting parity.
- 2026-06-20 direct Spotlight mesh follow-up: Spotlight bodies can reference
  instance meshes outside their group child list. Battle examples are
  `spot_blurcle01.mesh` / `spot_blurcle02.mesh`, both authored with
  `spotlight_circle.mat` after the lens/flare material refs. Native now keeps
  non-target mesh refs on the decoded `SpotlightObj` and draws them through the
  same active spotlight state, while skipping duplicates already present in the
  group. Validation in
  `engine/out/codex_goal_20260620_spotlight_direct_meshes/` shows
  `basketball01_spotlight.spot` and `right_round01_spotlight.spot` drawing the
  direct blur-circle meshes; `woman`/Battle frame 500 on/off changed pixels
  increase to 31,943, bbox `(577,21)..(1085,285)`. YYZ/theatre sanity evidence
  in `engine/out/codex_goal_20260620_spotlight_direct_meshes_yyz/` loads 44
  decoded spotlights plus the keyboard route without crashing.
- 2026-06-20 lighting MIDI cue follow-up: Rexglue real-play traces classify
  `lighting_change` as MIDI-dispatched and `do_lighting_next_keyframe` /
  `do_lighting_prev_keyframe` / `do_lighting_first_keyframe` as the keyframe
  advance route. `config/midi_parsers.dta::lighting_parser` maps TRIGGERS
  pitches `48`, `49`, and `50` to those messages with `start_offset -4` and
  `zero_length TRUE`, while `world_objects_worldbase.dta` advances a keyframe
  every cue only at great excitement and otherwise alternates through
  `ignored_last_light_change`. Follow-up PS2 static/runtime traces show the
  keyframe handlers enqueue direction records through `0x00280f60`,
  `0x00280fe8`, or `0x00281070`; each helper reads the global timer, adds
  `4.0`, and appends `{+1,-1,0, apply_time}` style rows to the lighting queue.
  Native therefore parses the `start_offset -4` side as four seconds, queues
  the actual `first` / `next` / `prev` advance for `song_time + 4.0`, and uses
  the cue stream to drive `active_lighting_keyframe_index_` when the queued
  apply matures; the older duration/beat loop remains only as a fallback for
  charts without lighting cues. Validation:
  `engine/out/codex_goal_20260620_lighting_midi_cues_surrender/` and
  `engine/out/codex_goal_20260620_lighting_midi_cues_surrender_long/` run
  stock PS2 `surrender` with authored camera/venue. The log decodes
  `lighting cues=204`, dispatches early `first`/`next` cues through the
  skip gate, changes presets from `blackout.pst` to `strobe_okay.pst`,
  `verse_okay.pst`, and `color1.pst`, and renders nonblank arena frames.
  Current validation in
  `analysis/native_validation/lighting_queue_timing_small1_20260622_current/`
  reruns stock PS2 `psychobilly` on `small1` hidden from `10.0s` with
  diagnostic autoplay and fixed `0.25s` steps. The clean `stderr.log` shows
  45 queued lighting cue rows, 40 matured apply rows, `delay=4.000` on the
  queue, matching `scheduled_t`/`t` on apply, and zero unsupported, miss,
  no-decoded, unresolved, or nonzero failed/error rows. This is traced
  keyframe-dispatch plumbing, not final render-light parity.
  Follow-up breadth validation in
  `analysis/native_validation/lighting_timing_crosscheck_20260622_current/`
  reruns `arena/shoutatthedevil`, `battle/rockthistown`, `big/hangar18`, and
  `small2/youreallygotme` after the queue timing change. All four exits are
  `0` with zero unsupported, miss, no-decoded, or unresolved rows; the logs
  record lighting cue queue/apply activity on all four routes plus active
  venue samples, lighting samples, keyframes, and authored cameras.
- 2026-06-22 PCSX2 lighting keyqueue trace refresh:
  current PCSX2 exposes SLUS code pages as read-only to external
  `WriteProcessMemory`, so the accepted route is now a throwaway prepatched
  savestate generated from the stock indexed state. The known-hot control
  `analysis/ps2_trace/pcsx2_known_hot_stateprepatch_hardened_control_20260622_current.json`
  proves the patched-state trace ring records in-song calls without live code
  writes. The lighting run
  `analysis/ps2_trace/pcsx2_lighting_keyqueue_stateprepatch_20260622_current.json`
  captures stock PS2 `shoutatthedevil` in the arena for 60 seconds and records
  `set_lighting` (`0x00271288`) 14 times, its child (`0x00271a08`) 17 times,
  the `+1` keyframe handler (`0x002716b8`) 3 times, and the `+1` enqueue
  helper (`0x00280f60`) 3 times. The `-1`, first-keyframe, and queue-overflow
  writer routes were zero-hit in this window. Decoded FPU args show `+1`
  handler/enqueue hits around `12.007`, `28.014`, and `54.010` seconds, while
  later `set_lighting` traffic lands around the expected timer-plus-four
  windows. Treat this as fresh PS2 evidence for the existing four-second queue
  route and as the current safe tracing method, not as permission to enable the
  still-gated authored dynamic-light bridge.
- 2026-06-22 lighting adjective filter follow-up:
  `world_objects.dta` defines `LIGHTING_ADJECTIVES` as only `blackout`,
  `strobe`, `flare`, `color1`, `color2`, and `sweep`. Some chart text events
  seen in theatre/YYZ validation carry unsupported symbols such as
  `[lighting (chase)]`; native previously copied that string into the lighting
  request and then reached the category-only preset only by fallback. The
  request parser now clears the adjective on a lighting event but only stores
  it when it is in the authored adjective set, so unsupported chart strings
  behave as category-only lighting instead of becoming phantom preset names.
  Validation: `analysis/native_validation/lighting_adjective_filter_20260622_current/`
  reruns `yyz`/theatre from `8.0s` and changes the active preset request from
  `CHORUS/chase` to `CHORUS/` while preserving keyframe cues and post-switch
  camera movement. `analysis/native_validation/lighting_valid_adjective_regression_20260622_current/`
  reruns `shoutatthedevil`/arena and preserves the valid
  `request=VERSE/color1` route into `color1.pst`.
- 2026-06-22 lighting category fallback follow-up:
  `world_objects_worldbase.dta::one_bar_to` passes ordered category lists to
  `set_lighting`: verse and chorus both try the current category, then
  `VERSECHORUS`, then `VERSECHORUSSOLO`; solo tries `SOLO`, then
  `VERSECHORUSSOLO`. Native previously tried only the primary category and a
  hard `VERSECHORUSSOLO` fallback, and its fallback branch could prefer an
  unadjectived fallback over an unadjectived primary preset. Preset selection
  now follows the authored list order: exact adjective matches across that
  list first, then unadjectived matches in the same order, then a last-resort
  category match. Validation:
  `analysis/native_validation/lighting_category_fallback_yyz_20260622_current/`
  keeps YYZ/theatre on direct `CHORUS` presets with category-only requests and
  active lighting cues, while
  `analysis/native_validation/lighting_category_fallback_color1_20260622_current/`
  keeps the valid `VERSE/color1` request falling through to the authored
  `VERSECHORUSSOLO` `color1.pst`.
- 2026-06-21 lighting transition follow-up: the decoded `duration` /
  `fade_out` fields now drive a stateful native spotlight target instead of a
  snap-only `set_active_spotlights` call. On a keyframe change, native keeps the
  current rendered spot state, builds the new target state from decoded
  `LightPreset` rows, uses the outgoing keyframe `fade_out` when available,
  converts PS2-authored frames at 30 fps, and interpolates RGB/intensity each
  rendered frame before drawing the lighting overlay. Spotlights that leave the
  target set remain in the transition with zero final intensity so they fade
  out rather than disappearing. This is still the shared decoded lighting
  graph path, not a venue/song-specific lighting rule.
- 2026-06-21 LightPreset timing sanity follow-up:
  `analysis/native_validation/fest_environ_decode_20260621_current/run_raw.log`
  exposed `color1.pst` / `bassist` and related fest keyframes reading
  `fade_frames=70373617072460814876672.000` when native trusted the four bytes
  immediately after every label. The same source dump contains plausible
  authored timing rows such as `920/500`, `480/50`, `240`, `80/0`, `60/30`,
  `10`, and `1/1` frame counts. Native now keeps the label/target-row
  discovery path but accepts only non-negative plausible LightPreset frame
  counts before feeding duration/fade into the transition system; non-frame
  packed bytes decode as zero instead of producing frozen lighting transitions.
  Validation:
  `analysis/native_validation/fest_lighting_timing_sanity_20260621_current/`
  reruns `crazyonyou`/fest from `16.0s`; `color1.pst` keeps six active
  spotlights while `fade_frames` changes from the bogus
  `70373617072460814876672.000` to `0.000`, Environ coverage remains
  `decoded=2 failed=0 preset_env_refs=13 matched=2 unmatched=11`, and regular
  camera sweeps plus `post_switch_cam` still run. The companion
  `analysis/native_validation/arena_lighting_timing_sanity_20260621_current/`
  reruns `shoutatthedevil`/arena and preserves valid `920/500` and `920/350`
  LightPreset timing rows while recording no six-digit, exponential, or miss
  timing rows.
- 2026-06-21 Light object decode follow-up:
  `world/theatre/og/gen/theatre_lighting.milo_ps2` contains a concrete
  `Light` entry, `spotlight01.lit`, with version `6`, local matrix at raw body
  offset `0x11`, stored world matrix at `0x41`, RGBA floats at `0x7e`, and
  range at `0x8e` (`1000.0` in the theatre dump). Native now decodes these raw
  `Light` entries into `milo_scene::Scene::lights` and logs decoded light
  coverage next to active `LightPreset` `.lit` refs before rendering. In the
  traced theatre `chorus_okay.pst` case, the preset `.lit` refs are the
  authored `char_*`, `crowd_*`, `hands`, and `flames` light names, not
  `spotlight01.lit`; native therefore logs the mismatch and does not render the
  decoded object as a substitute active light. This preserves the source data
  for future render-light parity without inventing brightness or routing.
- 2026-06-21 Environ object decode follow-up:
  `world/fest/og/gen/fest_lighting.milo_ps2` contains concrete `Environ`
  entries `lightbank.env` and `lightbank_bulbs.env`, both version `5` and body
  size `68`. The accepted byte pass shows the first RGBA-ish float block at raw
  offset `0x11`, two range/fog-ish floats at `0x21` and `0x25`, the second
  RGBA-ish block at `0x29`, and a final range float at `0x40` (`1000.0` in
  both retained fest dumps). Native now decodes these objects into
  `milo_scene::Scene::environs` and logs coverage against active
  `LightPreset` `.env` refs. It still does not apply Environ values to
  renderer brightness; the field storage is source-backed, but the exact PS2
  fixed-function/environment-light semantics remain an implementation gate.
- 2026-06-21 Environ dynamic-base follow-up:
  venue geometry Environ bodies are not all the zero-light shape used by the
  original fest lighting probe. `arena`/`small1` groups commonly point at
  `.env` entries whose body starts with a version-5 object header, a u32 `.lit`
  ref count, and length-prefixed light refs before the ambient/fog-ish block.
  Examples retained under `analysis/venue_lighting_audit/*_geom_extract/`:
  `stage.env` starts with `stage_light_02.lit` and `stage_light_03.lit`,
  while `small1` `stage.env` starts with `STAGE_omni.lit`. Native now consumes
  that `.lit` array, sets the Environ payload base after it, and reads the same
  ambient/range/color/range fields relative to that base. This changes the
  previously-garbage geometry environments into sane decoded ambient values,
  for example arena `stage.env` `(1,1,1,1)`, arena `stage_bkg.env`
  `(0.188,0.208,0.396,1)`, small1 `stage.env` `(0.290,0.129,0.067,1)`, and
  small1 `bar.env` `(0.294,0.137,0.047,1)`.
- 2026-06-21 Group/Mat environment routing follow-up:
  local `rnd_objects.dta` defines `Group.environ` and `Mat.use_environ`.
  Extracted venue groups carry `.env` refs at the tail of their group bodies
  (`arena_geom_opaque.grp -> stage.env`, `city.grp -> city.env`,
  `small1/stage.grp -> stage.env`, `small1/bar.grp -> bar.env`). Native now
  preserves `.grp` children in decoded group refs, recursively maps meshes to
  the nearest authored group environment, decodes `Mat.use_environ` /
  `Mat.prelit` from the bytes immediately after material RGBA, and applies
  per-mesh ambient only when the material opts into environment lighting.
  `use_environ=0` is common on authored glow/neon/fire/TV-style materials, so
  this gate prevents self-lit cards from being dimmed by stage ambient.
  `GHOGX_DISABLE_ENVIRON_LIGHTING=1` keeps the previous renderer path for A/B
  validation, and `GHOGX_LOG_ENVIRON_MESHES=1` logs mesh/environment coverage.
- LightPreset `.env` refs in the captured arena/small1 runs are preset-level
  graph-membership tables rather than per-keyframe rows (`keyframe env=0` in
  the native logs). The actual `stage.env`/`bar.env` objects referenced by
  those tables live in the venue geometry MILO, not always in the lighting
  overlay MILO. Native therefore caches decoded venue geometry Environ objects
  and reports LightPreset coverage as `matched_lighting` vs `matched_venue`.
  Do not animate or swap active environments from those preset-level refs until
  a trace shows the exact runtime route.
- 2026-06-22 Big venue extensionless light/env follow-up:
  `world/big/og/gen/big_lighting.milo_ps2` `chorus_great.pst` references
  extensionless object names `curtain_light` and `curtain`; these are real
  venue-geometry objects (`Environ__curtain_light`, `Light__curtain`), not
  LightPreset keyframe labels. Native now builds LightPreset label/ref
  classification from the raw lighting-overlay and venue-geometry object-name
  sets, so extensionless known objects are retained as refs and rejected as
  labels. `Environ__curtain_light` itself contains one extensionless light ref
  string, `curtain`, so the Environ decoder now accepts either explicit `.lit`
  refs or extensionless Harmonix object identifiers. Validation:
  `analysis/native_validation/big_curtain_light_environ_decode_20260622_085948/`
  loads `hangar18`/Big with `chorus_great.pst` reporting `preset_refs=33/7/12`,
  only `teal and white` as the decoded keyframe label, Light coverage
  `matched_venue=4 unmatched=0`, and Environ coverage
  `matched_venue=4 unmatched=0`.
  Current-build breadth check:
  `analysis/native_validation/venue_lighting_route_sweep_20260622_090113/`
  reruns `shoutatthedevil`/arena, `yyz`/theatre, `hangar18`/Big, and
  `psychobilly`/small1; all four routes report Light and Environ
  `unmatched=0` while still dispatching venue AnimFilters, lighting presets /
  keyframes, and authored regular camera sweeps. This is object/reference
  coverage and event-route evidence, not final lighting color parity.
- The same split applies to `.lit` refs. `arena_geom.milo_ps2` owns concrete
  Light entries such as `stage_light_02.lit`, `stage_light_03.lit`,
  `stage_bkg_1.lit`, `grim_light.lit`, `squid_light.lit`,
  `spotlight01.lit`, and `char_rim_lighting.lit`, while `small1_geom.milo_ps2`
  owns `STAGE_omni.lit`, `STAGE_back_wall_flood.lit`, and `bar01.lit`.
  Native now caches decoded venue geometry Light objects and reports
  LightPreset `.lit` coverage as lighting-overlay matches vs venue-geometry
  matches. This is still graph coverage; do not substitute those light bodies
  into a render-light path unless the active preset/keyframe route proves the
  intended object is live.
- `Light` tail offsets after the Trans matrices now match the local
  `rnd_objects.dta` schema: RGBA at raw `0x7e`, range at `0x8e`, type enum at
  `0x92` (`0` point, `1` directional, `2` fake spot, `3` floor spot), then
  `animate_color_from_preset` and `animate_position_from_preset` bytes at
  `0x96`/`0x97`. Arena `stage_light_02.lit` / `stage_light_03.lit` decode as
  directional, while small1 `STAGE_omni.lit`, `STAGE_back_wall_flood.lit`, and
  `bar01.lit` decode as point lights.
- Native has an opt-in probe for authored point/directional dynamic lights
  through the decoded environment route: mesh -> nearest `Group.environ`,
  material `use_environ`, Environ `.lit` list, then decoded `Light` object.
  Unsupported `kLightFakeSpot` / `kLightFloorSpot` entries stay out of this D3D
  light path because their visible behavior is represented by authored
  spotlight/floor geometry, not a simple fixed-function light. The probe is
  behind `GHOGX_ENABLE_ENVIRON_DYNAMIC_LIGHTS=1` (and still A/B-able with
  `GHOGX_DISABLE_ENVIRON_DYNAMIC_LIGHTS=1`) because default-on validation made
  arena's city/background jump to a peach wash. Keep it disabled until a trace
  proves the active preset/keyframe-to-light math.
  Validation after opt-in gating:
  `analysis/native_validation/arena_light_bridge_optin_default_20260621_current`
  and
  `analysis/native_validation/small1_light_bridge_optin_default_20260621_current`
  keep the venue `.lit`/`.env` coverage logs. Arena matched the prior
  dynamic-light-off frame exactly, and same-build small1 default-vs-explicit
  disabled comparison also produced a zero-pixel diff.
- 2026-06-21 LightPreset mesh-target spotlight follow-up:
  the theatre validation log showed `chorus_okay.pst` keyframes with many
  decoded mesh targets and `static_targeted_spots=18`, but native still emitted
  `active_spots=0` because it only activated explicit `.spot` refs or parsed
  target-state rows. The decoded `Spotlight` objects already map each `.spot`
  to its authored `_target.mesh`, so native now treats a keyframe mesh target as
  an activation route for every decoded spotlight aimed at that mesh, using the
  decoded target-state color/intensity when present and the existing neutral
  spotlight state otherwise. This is a shared LightPreset/Spotlight graph rule,
  not a venue-specific light list.
  Validation:
  `analysis/native_validation/theatre_target_suffix_lighting_20260621_current/`
  reruns stock PS2 `yyz` from `8.0s` and changes the same theatre keyframes
  from the earlier `active_spots=0` state to `target_states=26` /
  `active_spots=27` for `chorus_okay` and `target_states=52` /
  `active_spots=27` for `chorus_great`, with authored RGB/intensity rows
  attached to decoded `*.Target.mesh` targets. The same run preserves decoded
  `spotlight01.lit` coverage, the unmatched active-preset `.lit` refs, regular
  camera sweeps, and `post_switch_cam`.
- 2026-06-21 Spotlight target suffix follow-up:
  theatre lighting uses target names such as `right09.Target.mesh`, while some
  other lighting rows use `_target.mesh`. Native was only accepting the lower
  `_target.mesh` suffix in both `Spotlight` target decode and `LightPreset`
  target-state extraction. The shared classifier now accepts both spellings
  case-insensitively, preventing authored targets from being misfiled as
  instance meshes and allowing the packed LightPreset amount/RGB rows to attach
  to theatre spot targets. The intermediate debug-solid probe
  `analysis/native_validation/theatre_spotlight_active_names_20260621_current/`
  is retained as a route check for decoded `master_cannon.grp` instances.
  Cross-venue validation:
  `analysis/native_validation/arena_target_suffix_lighting_20260621_current/`
  reruns stock PS2 `shoutatthedevil` from `16.0s`; arena lighting has no
  decoded raw `Light` bodies, logs 12 unmatched `.lit` refs, keeps
  `color1.pst` / `color2.pst` active with `target_states=15` and
  `active_spots=4`, preserves regular camera sweeps and `post_switch_cam`, and
  produces coherent stage frames at `frame_00240.bmp` and `frame_00470.bmp`.
- 2026-06-22 Spotlight name inference follow-up:
  fest `LightPreset` rows target meshes such as `middle_right01_target.mesh`
  and `top_front06_target.mesh`, while the corresponding decoded spot objects
  are named `middle_right01_spotlight.spot` and
  `top_front06_spotlight.spot`. Some spot objects do not carry the target mesh
  ref in their own body, so the target-row route must infer both `<base>.spot`
  and `<base>_spotlight.spot` from an authored `*_target.mesh` row before
  deciding the row has no active spot. This is a shared naming rule derived from
  the PS2 lighting objects, not a fest spot list. Validation:
  `analysis/native_validation/fest_spotlight_name_infer_20260622_current/`
  changes `verse_okay.pst[0]` from zero inferred spots to
  `inferred_spots=2 active_spots=2`, while preserving the fest biker frame-7
  AnimFilter samples and advancing zombie MeshAnim samples. Cross-venue
  sanity in
  `analysis/native_validation/arena_spotlight_name_infer_crosscheck_20260622_current/`
  keeps the arena route loading performers/camera/venue events and increases
  `color1.pst[0]` target-row activation to
  `inferred_spots=15 active_spots=16`.
- 2026-06-20 venue-effect MIDI cue follow-up:
  `config/midi_parsers.dta::effect_parser` maps TRIGGERS pitch `52` to
  `{handle (world venue_effect)}` with `start_offset 0` and `zero_length TRUE`.
  Native now parses those notes into `Chart::venue_cues` and dispatches
  `venue_effect` at the authored MIDI tick as a transient world event, so it
  does not overwrite persistent venue excitement state. Validation in
  `engine/out/codex_goal_20260620_venue_effect_shout_102s/` runs stock PS2
  `shoutatthedevil`, decodes `venue cues=58`, seeks to `100.000s` with
  `venue_idx=9`, and dispatches pitch `52` at tick `77280`/`102.000s` while
  rendering a nonblank arena frame. The same log reports no trigger visibility
  route and no decoded `AnimFilter` transforms for `venue_effect` in this
  capture, so this is exact parser/event plumbing rather than an invented
  visible venue effect.
- 2026-06-20 camera duration follow-up: Rexglue real-play traces separate
  shot selection at roughly six seconds from `post_switch_cam` position
  changes at roughly two seconds, and
  `world_objects_worldbase.dta::get_shot_duration` selects a random bar count
  from `{find {world get camera_durations} {world get excitement_level}}`.
  Native no longer collapses venue DTB camera durations to only the okay row;
  it keeps all `kExcitementBoot`/`Bad`/`Okay`/`Great`/`Peak` rows and chooses
  the active excitement row when `check_camera_shot`/regular camera selection
  needs a new shot duration. Validation in
  `engine/out/codex_goal_20260620_camera_duration_rows_surrender_clean2/`
  runs stock PS2 `surrender` in the arena, logs the six-bar intro camera
  window, then picks regular shots with `duration=kExcitementBad[3,4]` under
  the native bad-excitement state while preserving `post_switch_cam` at the
  traced two-second cadence. This validates the shared camera-duration route;
  it does not claim final authored camera parity.
- 2026-06-20 current authored-camera venue/band validation:
  `engine/out/codex_goal_20260620_authored_venue_yyz_current/` captures stock
  PS2 `yyz` from `--diagnostic-song-start 8.0` with no debug gameplay camera.
  The retained MP4 `yyz_authored_camera_venue_band_8s.mp4` shows a nonblank
  theatre-stage authored-camera run, while the log resolves `funk1`,
  `metal_bass`, `metal_drummer`, and `metal_keyboard`, loads
  `dw_theatre_drums`, routes `BAND KEYS` into `keyboard_active_medium`, fires
  drum `kick_drum` cues, dispatches TRIGGERS lighting cues, activates
  `chorus_okay.pst`, starts the regular camera at `flr_far_lft02`, and runs
  `post_switch_cam` at `t=10.083`. This is current instrumental route
  validation, not a camera-parity claim.
  `engine/out/codex_goal_20260620_authored_venue_shout_current/` captures
  stock PS2 `shoutatthedevil` from `--diagnostic-song-start 16.0`, also with
  no debug gameplay camera. The retained MP4
  `shout_authored_camera_venue_band_16s.mp4` shows a coherent arena stage with
  the singer, guitarist, bassist, drummer, props, lighting, and moving authored
  camera. The log resolves `glam1`, `metal_singer`, `metal_bass`, and
  `metal_drummer`, loads `dw_arena_drums`, activates `color1.pst`, starts
  regular camera `flr_far_rt02x3`, and runs `post_switch_cam` at `t=18.083`.
  Continue treating these as current route-health evidence; exact camera
  parity still depends on a focused trace if a native/PS2 camera mismatch is
  reported.
- 2026-06-21 current native validation refresh:
  `analysis/native_validation/yyz_authored_venue_band_current_20260621_005321/`
  captures stock PS2 `yyz` from `--diagnostic-song-start 8.0` with no debug
  gameplay camera. The MP4 is nonblank and the log loads `funk1`,
  `metal_bass`, `metal_drummer`, and `metal_keyboard`; `BAND KEYS` enters
  `[play]`, drum cues fire, lighting keyframes advance, regular camera sweeps
  run, and `post_switch_cam` fires. The visual window is a distant,
  speaker-heavy theatre shot, so keep it as keyboard/drum/lighting/camera
  route-health evidence, not camera composition signoff.
  `analysis/native_validation/shout_authored_venue_band_current_20260621_005518/`
  captures stock PS2 `shoutatthedevil` from `--diagnostic-song-start 16.0`
  with no debug gameplay camera. The MP4 shows a coherent arena-stage route
  with `glam1`, `metal_singer`, `metal_bass`, `metal_drummer`, drum kit, props,
  lighting, and moving authored cameras. The singer marker is idle in this
  exact window even though the singer route and `singer_active_medium_01` are
  loaded, so do not use this as singer-performance parity signoff.
  `analysis/native_validation/shout_singer_active_close_valid_20260621_005941/`
  captures the same stock song from `--diagnostic-song-start 30.0` with a
  debug camera on `singer:bone_head.mesh` after a log-only timestamp probe
  found `BAND SINGER` in `[play]`. The MP4 and sheet show active singer
  mouth/face changes with mic/stand and the drum kit visible behind; the log
  records the singer `[play]` marker, `singer_active_medium_01`, 193 singer
  FaceFX `graph=applied` rows, and five drum cues. This is character/FaceFX
  inspection evidence only, not authored-camera parity evidence.
- 2026-06-21 route contract guard:
  `ghogx_gameplay_venue_band_contract_test` locks the source-backed
  orchestration shape described above. It checks keyboard role classification
  and `BAND KEYS`, bassist bass-graph/gut-bass prop routing, venue-specific
  `dw_<venue>_drums` loading and EventTrigger-first drum routing, the traced
  stock GH2 drum MIDI pitch map (`36 -> kick_drum`, `37 -> crash_symbal`),
  transient `bass_hit`/`venue_effect` dispatch, lighting TRIGGERS
  `48/49/50` with the traced four-second parser offset plus delayed apply
  queue, and the separation between
  regular camera duration rows and `post_switch_cam`. This is a drift guard for
  the accepted PS2 route evidence; it does not claim final camera, lighting, or
  animation parity by itself.
- 2026-06-21 venue EventTrigger label-route follow-up:
  arena geometry exposes trigger objects whose object names differ from their
  payload event labels, for example `sky_excitement_bad.trig` carries label
  `excitement_bad`. Native previously keyed group visibility and AnimFilter
  routing by stripped object name only, so `apply_venue_event("excitement_bad")`
  missed the decoded sky visibility route. The shared route now uses the
  payload label as the primary key, keeps the object name as an alias, and
  merges multiple triggers for one label. Validation:
  `analysis/native_validation/venue_event_label_route_postfix_20260621_current/`
  reruns stock PS2 `shoutatthedevil` at `--diagnostic-song-start 16`; the log
  changes `excitement_okay` visibility from `show=0 hide=18` to
  `show=2 hide=20` and changes `excitement_bad` from no route to
  `show=0 hide=4`, while preserving `color1.pst -> bad.pst`, the lighting cue
  skip gate under bad excitement, regular camera `flr_far_rt02x3`, and
  `post_switch_cam`. `frame_00120.jpg` is a nonblank arena-stage sanity frame.
  A separate no-miss preroll probe,
  `analysis/native_validation/venue_anim_okay_preroll_20260621_current/`,
  keeps `excitement_okay` active long enough for `searchlights.filt` to sample
  frames `15..90` with nonzero offsets on `searchlight*.mesh`, proving the
  decoded AnimFilter translation path is not just loaded but advancing.
- 2026-06-21 drum-driven venue event follow-up:
  the same arena trigger inventory exposes `city_lights_kickdrum.trig` with
  payload label `kick_drum`. Native drummer cues already drove the venue-specific
  drum kit, but they did not dispatch transient world EventTriggers. Drum cues
  now also call `apply_venue_event(cue.event, false)`, so venue props/lights can
  react to the same traced `BAND DRUMS` messages without changing persistent
  excitement state. Validation:
  `analysis/native_validation/venue_event_drum_route_postfix_20260621_current/`
  reruns the same `shoutatthedevil` arena window; the log shows `kick_drum`
  applying `trigger visibility show=8 hide=0`, starting
  `speaker_kick_drum.filt` with 20 targets, and sampling 120 kick-drum
  AnimFilter rows with nonzero offsets on speaker-stack meshes while the drum
  kit cue, lighting preset/keyframe route, and `post_switch_cam` continue.
  The follow-up
  `analysis/native_validation/venue_event_visibility_compose_postfix_20260621_current/`
  keeps the persistent `excitement_bad` visibility route active underneath
  transient `kick_drum`: the log has `excitement_bad show=0 hide=4`,
  `kick_drum show=8 hide=0`, two kick trigger applications, and 120
  `speaker_kick_drum.filt` samples. This locks the layering rule that transient
  venue hits compose over the active excitement state instead of replacing it.
- 2026-06-21 player-fret venue event follow-up:
  local Rexglue real-play notes record `hit_p0_fretN` as a player-hit world
  property, with `N` 1-indexed from Green through Orange and hundreds of
  hardware firings in the capture. Arena geometry also decodes
  `city_lights_fret_1..5.trig` payload labels `hit_p0_fret1..5`. Native now
  dispatches `hit_p0_fret{lane+1}` as a transient venue event from the shared
  successful note-hit path, so venue props react through decoded EventTrigger
  labels rather than arena object-name shortcuts. The contract guard locks the
  helper and forbids direct `city_lights_fret` dispatch. A diagnostic native
  autoplay switch exists only for validation capture; it feeds chart notes into
  the normal `fret_mask` before strum edge detection for ordinary frame cadence,
  and now has a fixed-step catch-up path that consumes any crossed notes before
  the miss scanner can turn a coarse validation clock into false
  `excitement_bad` lighting. Both paths still dispatch the same decoded
  `hit_p0_fret*` venue events from source chart notes. Validation:
  `analysis/native_validation/venue_fret_hit_route_regression_20260621_current/`
  reruns the arena window and confirms all five `hit_p0_fret*` label routes are
  present while existing `excitement_bad`, `kick_drum`, `speaker_kick_drum.filt`,
  lighting keyframe, regular camera, and `post_switch_cam` behavior still runs.
  `analysis/native_validation/venue_fret_hit_autoplay_replay_20260621_current/`
  reruns the same window with `--diagnostic-autoplay`; the first hit queues until
  venue load, then replays through decoded `hit_p0_fret1` visibility, and the
  whole run records 7 autoplay ticks, 9 `HIT` rows, 9 routed fret visibility
  events, zero fret no-route rows, 4 kick-drum trigger applications, lighting
  preset/keyframe changes, regular camera sweeps, and `post_switch_cam`.
- 2026-06-21 note-consumption lighting follow-up: the first diagnostic autoplay
  run exposed a native bookkeeping bug where notes already logged as `HIT`
  stayed eligible for later miss processing. That fed false `excitement_bad`
  events into the venue-lighting path and forced `bad.pst` despite valid player
  hits. Native now keeps a per-difficulty consumed-note ledger sized from the
  parsed chart; successful hits and misses consume the source note once,
  diagnostic seek marks skipped notes consumed, and diagnostic autoplay ignores
  consumed notes while still entering through the normal fret-mask/strum path.
  Validation:
  `analysis/native_validation/venue_note_consumption_lighting_20260621_current/`
  reruns the arena autoplay window for 900 frames and records 33 `HIT` rows,
  zero miss rows, 33 routed fret visibility events, zero fret no-route rows,
  11 kick-drum trigger applications, 3 regular camera sweeps, 6
  `post_switch_cam` moves, and lighting transitions into `chorus_great.pst` and
  `flare_great.pst` once the streak reaches `excitement_great`.
- 2026-06-22 fixed-step diagnostic autoplay catch-up:
  the validation-only autoplay path now consumes all unconsumed player notes at
  or before `song_time + hit_window` before normal miss processing. This keeps
  hidden venue/lighting sweeps with coarse deterministic frame steps from
  fabricating misses and bad-excitement lighting while leaving non-diagnostic
  player input untouched. Validation:
  `analysis/native_validation/diagnostic_autoplay_fixedstep_catchup_fest_wait_20260622_current/`
  reruns stock `badreputation` in the Fest venue with `--fixed-dt 0.25`,
  diagnostic autoplay, and venue-filter logging; it exits `0` with 122
  diagnostic `HIT` rows, zero miss rows, zero unsupported/no-route rows, and
  zero `excitement_bad` venue events.
- 2026-06-21 venue lifecycle/visibility follow-up:
  `config/macros.dta` declares `start` as a system world event and the accepted
  real-play stack trace includes `prop:start` through the named-event dispatch
  path. Arena geometry decodes `start.trig` with label `start` and a large
  initial show/hide set, so native now applies that decoded route to the runtime
  venue visibility state before the first world frame. EventTrigger visibility
  is no longer rebuilt from scratch for each event; show/hide actions mutate a
  runtime hidden-mesh state, and material alpha hides are composed on top so a
  material fade cannot unhide a mesh that an EventTrigger explicitly hid.
  `world_objects_worldbase.dta::intro_end` sets `should_resend_excitement`, and
  `world/camshot.dta::start_shot` calls `world resend_excitement`, so native
  now dispatches `intro_end` when the six-bar intro camera window closes and
  consumes that latch on the next regular camera shot by re-entering the normal
  persistent excitement event path. Validation:
  `analysis/native_validation/venue_lifecycle_resend_20260621_current/` records
  one `start` visibility application, one `intro_end` visibility application,
  one `resend_excitement`, 33 `HIT` rows, zero miss rows, 33 routed fret
  visibility events, 11 kick-drum routes, zero `excitement_bad` rows, 3 regular
  camera sweeps, 6 `post_switch_cam` moves, 1,410 venue AnimFilter samples, and
  great-state lighting through `chorus_great.pst` / `flare_great.pst`.
- 2026-06-21 regular-camera shot-start lifecycle follow-up:
  `start_shot -> world resend_excitement` is a shot-start side effect, not a
  camera-name-change side effect. Native now consumes `should_resend_excitement`
  after the scripted regular camera selector returns a key even if a constrained
  or one-shot camera set resolves to the already-active camera. The existing
  changed-camera sweep bookkeeping remains separate. The route contract guards
  this so future camera-parity work cannot accidentally gate the traced
  resend-excitement latch behind `active_regular_camera_ != key->name`.
  Validation:
  `analysis/native_validation/venue_resend_shot_start_20260621_current/`
  reruns stock PS2 `shoutatthedevil` from `16.0s` with diagnostic autoplay.
  The log records `intro_end`, a regular camera shot start at `t=16.017`,
  `resend_excitement: excitement_okay`, the re-entered `excitement_okay`
  MatAnim/ParticleSys/visibility/AnimFilter route, active lighting keyframes,
  and coherent arena frames at 120/240.
- 2026-06-21 persistent MatAnim lifecycle follow-up:
  persistent venue-event changes already clear the previous persistent
  AnimFilter, EnvAnim, LightAnim, and ParticleSys playback before applying the
  new decoded event. Native now clears previous persistent MatAnim playback in
  the same block, so old excitement material animations stop advancing after a
  new persistent state is chosen. Current sampled material alpha/texture state
  remains available for interrupted fades and for new authored MatAnim rows to
  pick up as their start value; this is lifecycle cleanup, not a material reset
  shortcut. Validation:
  `analysis/native_validation/venue_persistent_matanim_lifecycle_20260621_current`
  ran stock PS2 `shoutatthedevil` from `16.0s` for 900 fixed-dt frames with
  diagnostic autoplay and venue-filter logging. The log has 33 note hits, zero
  misses, 3 regular camera sweeps, 6 post-switch camera beats, 9 lighting
  keyframes, and 177 venue MatAnim event rows; frame 870 remains a coherent
  arena render after the persistent event changes.
- 2026-06-21 diagnostic venue-runtime reset follow-up:
  diagnostic mid-song seeks are validation tooling, but stale derived venue
  state there can create false lighting/animation evidence. Native now clears
  runtime-only venue and lighting animation caches on diagnostic seek: active
  venue/lighting MatAnim playback, EnvAnim/LightAnim overrides, ParticleSys
  state, AnimFilter/MeshAnim transform overrides, transient event queues, and
  the active persistent excitement latch. If renderers already exist, the reset
  pushes empty overrides, restores base plus authored `start` visibility, and
  replays the lighting overlay `start` trigger before the normal next-tick
  excitement event is chosen. Validation:
  `analysis/native_validation/venue_diagnostic_seek_reset_20260621_current/`
  reruns stock PS2 `shoutatthedevil` from a 16.0s diagnostic seek for 360
  fixed-dt frames. The log records the seek, `start` visibility, re-entered
  `excitement_okay` MatAnim/ParticleSys/AnimFilter routes, 9 note hits, zero
  misses, 2 regular camera sweeps, and 2 lighting keyframes; frame 300 is a
  coherent arena render with performers, props, lighting, and venue animation.
- 2026-06-21 pre-load venue-event lifecycle follow-up:
  native `tick()` can choose a persistent excitement state before the first
  draw call has loaded `*_geom.milo_ps2` and decoded EventTrigger, MatAnim, and
  AnimFilter route tables. That state should be latched, not applied against
  empty route maps. `apply_venue_event(..., persistent=true)` now stores the
  active excitement event until `draw()` has loaded the venue and then replays
  it through the normal decoded route. Transient hits still queue separately and
  replay after the persistent state, preserving the traced layering order.
  Validation:
  `analysis/native_validation/small1_preload_latch_venue_anim_20260621_current/`
  reruns `psychobilly` from `10.0s` and changes the pre-load persistent row to
  `excitement_okay: latched until venue load`, then applies decoded
  `excitement_okay` visibility plus ceiling-swing AnimFilters after load. It
  keeps 3,230 venue AnimFilter samples, 7 lighting keyframe activations, and
  the regular camera/post-switch path. The companion
  `analysis/native_validation/arena_preload_latch_lighting_anim_20260621_current/`
  reruns `shoutatthedevil` from `16.0s` and preserves the richer arena route:
  queued first fret hit, decoded `excitement_okay` MatAnim/AnimFilters,
  kick-drum trigger visibility plus `speaker_kick_drum.filt`, 1,956 live venue
  AnimFilter samples, two lighting keyframes, regular camera/post-switch
  movement, and zero miss rows.

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
  a decoded `TransAnim` translation key path. Current `small1` validation shows
  persistent `excitement_*` triggers resolving through draw-group visibility
  while transient player-fret speaker filters resolve through decoded
  `EventTrigger -> AnimFilter -> TransAnim` routes; do not invent motion for
  the visibility-only excitement gates without a runtime trace proving they own
  animated TransAnim targets.
- 2026-06-21 venue animation basis follow-up: decoded `TransAnim` keys are
  local Trans movement for the target object, so native now applies sampled
  venue/drum mesh deltas through each mesh's decoded world basis before adding
  them to the matrix translation. This replaces the older direct world-X/Y/Z
  addition and keeps persistent `AnimFilter` movement plus one-shot mesh
  `TransAnim` playback on the same local-space path.
- 2026-06-21 venue MatAnim follow-up: native already decoded `MatAnim`
  material, start alpha, end alpha, and duration, but the runtime was only
  storing `{material, end_alpha}` and jumping immediately. Venue events now
  start an active material-alpha animation using the decoded duration at 30 fps,
  sample alpha each tick, and then feed the same composed hidden-mesh path used
  by EventTrigger visibility. Interrupted material fades begin from the current
  native alpha instead of snapping back to a guessed value.
  Validation:
  `analysis/native_validation/venue_matanim_lighting_anim_20260621_current/`
  runs stock PS2 `shoutatthedevil` from `16.0s` with diagnostic autoplay and
  records 4 lighting transition targets, 9 lighting keyframe activations,
  1,410 venue `AnimFilter` samples, `MatAnim searchlight_beam_on.mnm` starting
  from venue excitement events with decoded `3.333s` duration, 3 regular camera
  sweeps, 6 `post_switch_cam` moves, and zero miss rows. `frame_00870.bmp`
  remains a coherent arena render after the local-basis and MatAnim changes.
- 2026-06-21 venue MatAnim alpha-space follow-up:
  fest geometry exposes `Rising_Souls.mnm -> rising_souls.mat` with raw keys
  that previously logged as `alpha 100.000 -> 0.800 frames=10.0`. Native's D3D
  renderer consumes material alpha as a `0..1` multiplier, so raw values above
  that range must not participate in interpolation as literal alpha. The
  MatAnim loader now clamps decoded start/end alpha to renderer alpha space, and
  the per-tick sampler clamps the interpolated value before composing material
  visibility. This keeps `100` equivalent to opaque instead of delaying fades
  until the last few frames of a high-to-low interpolation. Validation:
  `analysis/native_validation/fest_matanim_alpha_clamp_20260621_current/`
  reruns `crazyonyou`/fest from `16.0s` with alpha/filter diagnostics; the
  MatAnim load log changes `Rising_Souls.mnm` to `alpha 1.000 -> 0.800
  frames=10.0`, the active `Rising_Soul_Plane.mesh` alpha sample is finite
  (`0.683` at frame 260), `color1.pst` remains at `active_spots=6` with
  `fade_frames=0.000`, and the `excitement_great` biker filters continue
  sampling position/rotation frames through the same venue AnimFilter path.
- 2026-06-21 lighting-overlay event animation follow-up: arena, small1, and
  theatre `*_lighting.milo_ps2` all carry the same authored route
  `smoke_lights.trig` payload `start` -> `smoke_lights.filt` ->
  `smoke_lights.mnm`. Native previously only found `.mnm` refs directly
  embedded in an `EventTrigger`, so this lighting-overlay source route was
  invisible to the runtime. Native now resolves `EventTrigger -> AnimFilter ->
  MatAnim` indirection, loads separate lighting-overlay MatAnim route state from
  the lighting MILO, and applies the authored `start` event when the overlay is
  constructed. The `smoke_lights.mnm` channel is not the alpha-only venue
  geometry shape already decoded for `MatAnim`; unsupported lighting material
  channels are therefore logged rather than mapped to brightness or UV animation
  without trace-backed field semantics.
- 2026-06-21 MatAnim channel schema note: the local community schema
  `config/rnd_objects.dta` describes `MatAnim` as a material-property animator,
  and `config/milo.dta` breaks its editor pages into Color, Alpha, Trans, Scale,
  Rot, and Tex. The three lighting `smoke_lights.mnm` objects match that channel
  order: after the target material/name strings they carry `color_count=0`,
  `alpha_count=0`, then `trans_count=2` with rows equivalent to UV/material
  translation keys at authored frames `0` and `100`, followed by one-key scale
  and rotation channels. Native therefore must implement material texture
  transform sampling for this route; treating it as alpha would incorrectly hide
  `spotlight_default.mat`.
- 2026-06-21 MatAnim texture-transform implementation: native now decodes
  MatAnim channel counts in the PS2 order above instead of assuming the first
  count is alpha. Alpha rows remain value/frame pairs. Trans rows are preserved
  as x/y/z/frame keys and sampled on the song clock as raw texture-transform
  values, not normalized percent values. Geometry evidence backs this: arena
  `sky_clouds.mnm` moves `0 -> 1` over 100 frames, small1 TV animations use
  sub-unit and multi-unit offsets, scale `1.0` is identity, and rotation keys
  such as `0.017453292` are radians. Native therefore routes Trans, Scale, and
  Rot to one renderer material texture-transform override and only switches to
  wrapping while an animated material transform is active.
- 2026-06-21 lighting-to-venue MatAnim route: fest lighting overlay
  `excitement_okay.trig` references `Rising_Souls.mnm`, but that `MatAnim`
  lives in `fest_geom.milo_ps2`, not `fest_lighting.milo_ps2`. The PS2 body
  decodes normally as version 7 with 6 alpha keys and 2 texture-translation
  keys, so the previous native `unsupported channel shape` log was a cross-MILO
  ownership bug rather than an unknown channel schema. `apply_lighting_event`
  now falls back to the decoded venue-geometry MatAnim map and applies the
  animation to the venue renderer when a lighting EventTrigger references a
  geometry-owned material animation. Validation:
  `analysis/native_validation/fest_cross_milo_matanim_rising_souls_20260621_current/`
  reruns hidden stock PS2 `badreputation` from `15.0s` with diagnostic
  autoplay and dynamic venue lights enabled. The log decodes `Rising_Souls.mnm`,
  routes `lighting event excitement_okay: venue MatAnim Rising_Souls.mnm`,
  no longer emits `unsupported channel shape`, still starts the
  `stage_angel`/`stage_gargoyle` LightAnims from `venue_effect`, keeps lighting
  keyframes and regular camera sweeps active, and captures coherent fest frames
  at 180/300.
- 2026-06-21 group-contained MatAnim route: several authored venue events do not
  point directly at `.mnm` objects. `small1` routes `start` /
  `excitement_okay` through groups such as `tv_good.grp`, `tv_bad.grp`, and
  `barrel_smoke.grp`; `arena` routes sky/background material animation through
  `stage_bkg_okay.grp` / `stage_bkg_great.grp`. Native now resolves
  `EventTrigger -> Group -> MatAnim` and `EventTrigger -> AnimFilter -> Group ->
  MatAnim` in the shared MatAnim route loader. Validation:
  `analysis/native_validation/small1_group_matanim_route_20260621_current/`
  logs `tv_*` and `barrel_smoke.mnm` events starting from group refs, and
  `analysis/native_validation/arena_group_matanim_route_20260621_current/` logs
  `sky_clouds.mnm`, `sky_green.mnm`, and `sky_orange.mnm` events from the
  authored sky groups while preserving lighting overlay `smoke_lights.mnm`.
- 2026-06-21 venue EnvAnim route: arena geometry contains `EnvAnim`
  `coplight_blue.enm` and `coplight_red.enm`, both version 4. The body shape is
  the same 25-byte pre-target prefix used by venue `MatAnim`, followed by the
  target `.env` string, a color-key count, then RGBA+frame rows. The coplight
  rows are 0/50/100-frame color cycles for `coplight_blue.env` and
  `coplight_red.env`. Native now resolves `EventTrigger -> AnimFilter -> Group
  -> EnvAnim`, starts active environment color animations from those decoded
  keys, samples them on the song clock, and feeds per-Environ color overrides
  through the existing material `use_environ` gate. Validation:
  `analysis/native_validation/arena_envanim_coplights_great_20260621_current/`
  reruns stock PS2 `shoutatthedevil` from `16.0s` with diagnostic autoplay; at
  streak 10 the log starts both EnvAnims from `excitement_great`, keeps
  regular camera/post-switch and `chorus_great.pst` lighting, and the captured
  arena frames remain coherent after the coplight environment animation starts.
- 2026-06-21 venue ParticleSys first pass: all GH2 venue MILO inventories show
  `ParticleSys` / `ParticleSysAnim` as a broad shared venue-effect class
  (150 `.part` objects and 48 `.panim` objects in the current eight-venue
  inventory). PS2 `ParticleSys` bodies are version 27 with an embedded Trans
  block at raw `+0x19`: Trans version 9, local/world matrices, normal 9-byte
  Trans tail, parent string, then Draw version 3 / Draw body, followed by the
  particle property floats. Native now decodes the authored particle material,
  parent, max particle count, velocity range, lifetime range, and size range,
  resolves `EventTrigger -> AnimFilter -> Group -> ParticleSysAnim ->
  ParticleSys`, and renders active systems through D3D point sprites. This is a
  shared loader/runtime route, not exact PS2 emitter physics yet. Validation:
  `analysis/native_validation/arena_particles_firstpass_20260621_current/`
  runs hidden stock PS2 `shoutatthedevil` from `16.0s` with diagnostic autoplay.
  The log records arena geometry loading `20 particles (20 ok / 0 fail)`,
  `venue ParticleSys routes loaded ...: 10 events`, `excitement_okay` starting
  six persistent smoke/blood particle systems through the normal event path,
  and coherent native frames at 480/600.
- 2026-06-21 ParticleSysAnim key follow-up: extracted PS2 `ParticleSysAnim`
  `.panim` bodies are version 3, target a `.part` object, then carry an
  unaligned key count after the target string plus 8 bytes. Each key row is
  two emission values and an authored frame. Some rows, for example
  `flames_2.panim`, keep their own target particle but copy keys from another
  `.panim`, so native now preserves key-owner references instead of scanning
  for the largest plausible float. The event route stores those key rows,
  transient/persistent particle systems sample them on the same authored
  30 fps clock as other venue animation, and the renderer scales particle count
  and alpha from the sampled emission intensity. Validation:
  `analysis/native_validation/arena_particles_panim_keys_venueeffect_20260621_current/`
  runs hidden stock PS2 `shoutatthedevil` from `100.0s` with diagnostic
  autoplay. The log records real `venue_effect` cues at `102.000s` and
  `103.283s`, starts `flame_1.part`, `flame_2.part`, and `flame_3.part` from
  their 3-key `.panim` routes, samples six keyed flame intensities, keeps
  authored lighting keyframes and regular camera sweeps active, and captures
  coherent arena frames at 120/180 with the flame effect visible.
- 2026-06-22 ParticleSysAnim start-size channel follow-up: the local
  `config/milo.dta` editor pages list `PartAnim Start Size` separately from
  emit rate, and stock PS2 arena flame `.panim` bodies carry a second scalar
  key block immediately after the self animation ref. `flames_1.panim` keeps
  three emit-rate keys through frame `10` plus three start-size keys
  `10 -> 10 -> 0` through frame `30`; `flames_2.panim` and `flames_3.panim`
  copy those owner rows. Native now preserves that second scalar block as
  particle size keys, copies it through `keys_owner`, samples it per tick, and
  feeds authored point-sprite size overrides to the renderer by particle name.
  Validation:
  `analysis/native_validation/arena_particle_size_keys_shout_20260622_current/`
  reruns hidden stock PS2 `shoutatthedevil` from `100.0s` with diagnostic
  autoplay and `GHOGX_DEBUG_VENUE_FILTERS=1`. The log records `venue_effect`
  starting `flame_1.part`, `flame_2.part`, and `flame_3.part` with
  `emit_keys=3 size_keys=3 frames=30.0`, six live flame samples with
  `size=10.000`, zero unsupported material channels, zero misses, active
  lighting keyframes, regular camera/post-switch routes, and coherent arena
  captures at frames 120/180/240. This is shared ParticleSysAnim channel
  support; it does not change the still-gated authored dynamic-light bridge.
- 2026-06-21 venue LightAnim route: extracted PS2 `LightAnim` `.lnm` bodies are
  version 2 with the same 25-byte pre-target prefix used by venue `MatAnim` /
  `EnvAnim`, followed by the target `.lit` string, a color-key count, and
  RGBA+frame rows. Fest `stage_angel.lnm` owns seven keys from frame 0..200;
  `stage_gargoyle.lnm` targets `gargoyle_light.lit` and references
  `stage_angel.lnm` as its key owner. Native now decodes `.lnm` objects, copies
  key-owner rows, resolves `EventTrigger -> AnimFilter/Group -> LightAnim`,
  starts active light color animations on venue events, samples them on the song
  clock, and feeds per-Light color overrides to the venue renderer's authored
  dynamic light route. Validation:
  `analysis/native_validation/fest_lightanim_event_badreputation_20260621_current/`
  runs hidden stock PS2 `badreputation` from `15.0s` with diagnostic autoplay
  and dynamic venue lights enabled. The log records `stage_angel.lnm` /
  `stage_gargoyle.lnm` decode, `venue LightAnim routes loaded ...: 2 events`,
  the real MIDI `venue_effect` cue at `19.400s`, both LightAnims starting
  transiently through that cue, companion MatAnim/ParticleSys/visibility/
  AnimFilter routes firing, active lighting keyframes, regular camera sweeps,
  and coherent native frames at 180/300.
- 2026-06-22 resume validation:
  `analysis/native_validation/fest_lightanim_resume_20260622_current/` reruns
  stock PS2 `badreputation` from `15.0s` with diagnostic autoplay and dynamic
  venue lights enabled. The log decodes the same `stage_angel.lnm` /
  `stage_gargoyle.lnm` LightAnim route, resolves cross-MILO
  `Rising_Souls.mnm` through the venue MatAnim path without the old unsupported
  channel row, then dispatches the real `venue_effect` cue through MatAnim,
  LightAnim, ParticleSys, visibility, and AnimFilter state. Captured frames
  `frame_00180.bmp` and `frame_00300.bmp` are coherent fest-stage authored
  camera renders with band, props, lighting, and venue animation.
- 2026-06-21 venue MeshAnim route: extracted PS2 `MeshAnim` `.msnm` bodies are
  version 1 vertex-frame tables. Direct rows embed a target `.mesh` string,
  then frame and vertex counts followed by `frame_count * vertex_count` local
  XYZ positions; owner rows can instead reference another `.msnm` key source.
  Native now preserves `.msnm` / `.meshanim` refs, resolves `EventTrigger ->
  AnimFilter/Group -> MeshAnim`, samples vertex positions on the same
  AnimFilter clock used by TransAnim, and feeds exact-count mesh position
  overrides to the renderer. Validation:
  `analysis/native_validation/small2_meshanim_youreallygotme_20260621_current/`
  runs hidden stock PS2 `youreallygotme` from `16.0s` with diagnostic autoplay.
  The log decodes `monitor_speaker01.msnm` and `monitor_speaker02.msnm` as
  3-frame / 39-vertex animations, routes both through `floormonitor` and
  `kick_drum`, records 84 live `venue MeshAnim sample` rows from real drum
  events, keeps regular camera sweeps and lighting keyframes active, and the
  captured small2 frames at 180/300 remain coherent with the monitor speakers
  on-screen.
- 2026-06-22 AnimFilter long-frame follow-up:
  fest `zombie_loop.filt` targets `zombie.grp` and its traced filter row stores
  `scale=0.75`, `start=600`, `end=1400`, `type=1`, `offset=0`. Native was
  clamping AnimFilter start/end above `500` back to zero, so intro-end zombie
  MeshAnim routes decoded but sampled frame `0.00` forever. The sanity clamp is
  now widened to keep long authored venue frame windows. Validation:
  `analysis/native_validation/fest_animfilter_long_frame_20260622_current/`
  reruns hidden stock PS2 `badreputation` from `15.0s`; the log records
  `zombie_loop.filt frame 600.00..1400.00` and advancing MeshAnim samples
  `611.25`, `622.50`, `633.75`, and `645.00` on the four zombie `.msnm`
  targets while regular camera, lighting preset, particle, and autoplay hit
  routes continue.
- 2026-06-22 AnimFilter zero-span offset follow-up:
  fest `biker1_ok.filt` and `biker2_ok.filt` target `biker_1.grp` /
  `biker_2.grp` and their traced PS2 filter rows store `scale=1`, `start=0`,
  `end=0`, `type=1`, and authored `offset=7`. Native previously returned
  `start` before applying `offset` when an AnimFilter span was zero, so the
  Motocross TransAnim targets sampled frame `0.00` instead of the PS2-authored
  static frame. Zero-span AnimFilters now still honor the authored offset as a
  static sample frame, while keeping duration at zero rather than inventing a
  playback window.
- 2026-06-21 texture-transform validation pass:
  `analysis/native_validation/small1_tex_xfm_scale_rot_20260621_current/`
  proves small1 group-routed TV/barrel MatAnims are decoded and started with
  authored Trans/Scale/Rot counts (`tv_nuke.mnm` has 3 translation, 3 scale, and
  20 rotation keys; `tv_anarchy.mnm` has 12 translation, 5 scale, and 1 rotation
  key). `analysis/native_validation/arena_tex_xfm_scale_rot_20260621_current/`
  proves arena sky/background routes still expand from authored groups, city
  light alpha events still fire from fret/kick triggers, and the lighting
  overlay `smoke_lights.mnm` starts with 2 translation, 1 scale, and 1 rotation
  key. Native screenshots from both runs render coherently after the full
  material texture-transform path.
- 2026-06-21 AnimFilter field-order correction: extracted PS2 venue filter
  objects show length-prefixed target refs followed by `scale`, `period`,
  `start`, `end`, `type`, and `offset`. Examples:
  `speaker_cone03.filt` has `scale=1`, `period=0`, `start=0`, `end=100`,
  `type=0` (`kAnimRange`), and `offset=0.25`; `searchlights.filt` has
  `type=2` (`kAnimShuttle`) and `offset=5`; `squid_okay.filt` and
  `grim_okay_loop.filt` have `type=1` (`kAnimLoop`). Native previously read
  the offset float as an integer type, producing raw bit-pattern values like
  `1084227584`. The shared AnimFilter sampler now applies the authored offset
  and uses `ANIM_ENUM` modes (`kAnimRange`, `kAnimLoop`, `kAnimShuttle`) when
  sampling TransAnim frame ranges. Validation:
  `analysis/native_validation/arena_animfilter_type_offset_20260621_current/`
  logs `searchlights.filt offset=5.000 type=2`, looped grim/squid filters as
  `type=1`, and coherent sampled frames after the change.
  `analysis/native_validation/small1_animfilter_type_offset_20260621_current/`
  logs speaker cones as `offset=0.250 type=0`, `Bass_amp_bass_hit.filt` as
  `offset=0.150 type=0`, and stable venue screenshots.
- 2026-06-21 small1 venue-animation refresh:
  `analysis/native_validation/small1_venue_anim_probe_20260621_current/` runs
  stock PS2 `psychobilly` from `10.0s` with diagnostic autoplay. The
  translation-only native log from that pass
  native log decodes `small1` geometry with 34 EventTrigger filter routes, 30
  filter mesh-target rows, and 27 routed AnimFilter transform events. The run
  records 2,720 `venue AnimFilter sample` rows from player-fret speaker events,
  including `speaker_cone*.filt -> speaker_cone*.mesh` targets advancing
  through frames `0..45+`. At that point persistent `excitement_okay` /
  `excitement_great` looked visibility-only because native only accepted
  translation keys; the transform-channel follow-up below supersedes that
  interpretation by decoding the same ceiling-swing filters as rotation-key
  animation. The same run keeps 3 regular camera sweeps, 4
  `post_switch_cam` moves, 2 lighting transition targets, and zero miss rows.
  Treat this as evidence that small1 speaker/fret venue animation is active
  through the shared decoded route, not as a reason to synthesize unsupported
  motion.
- 2026-06-21 TransAnim channel follow-up: native venue/drum playback was still
  structurally translation-only even though PS2 `TransAnim` bodies include
  quaternion rotation and vec3 scale key blocks. `crash.tnm` in
  `dw_small1_drums.milo_ps2` is the compact proof: it decodes as
  `pos=0 rot=15 scale=0`, so the old `MeshAnimKey {frame,pos}` path could not
  animate the crash cymbal at all. Native now decodes `TransAnim` into a shared
  `MeshTransformAnim` with translation, rotation, and scale channels; persistent
  venue `AnimFilter` samples and one-shot drum triggers both send
  `MeshTransformSample` to the renderer. The renderer applies translation in
  the target mesh's local basis, then applies local rotation and scale deltas
  relative to the first authored key.
  Validation:
  `analysis/native_validation/small1_transform_channels_20260621_current/`
  reruns stock PS2 `psychobilly` from `10.0s` with diagnostic autoplay and raw
  logging. `run_raw.log` records 44 venue `TransAnim` decodes, 4 drum
  `TransAnim` decodes, 2,836 live venue `AnimFilter` samples, 31 drummer cues,
  3 regular camera sweeps, 4 `post_switch_cam` moves, and zero miss rows. Key
  rows include `speaker_cone5.tnm -> speaker_cone5.mesh pos=5 rot=0 scale=3`,
  live speaker samples with `pos=1 rot=0 scale=1`,
  `light_hanging_cable*.tnm` rows with `pos=0 rot=101 scale=0`, live ceiling
  swing samples with `pos=0 rot=1 scale=0`, `snare.tnm -> snare.mesh
  pos=8 rot=8 scale=0`, and `crash.tnm -> crash.mesh pos=0 rot=15 scale=0`
  followed by `crash_symbal` drum cues. `frame_00700.bmp` is coherent for the
  small1 venue/drum route; it is not a character-hair signoff.
- 2026-06-21 cross-venue post-animation sanity:
  `analysis/native_validation/yyz_theatre_venue_band_post_anim_20260621_current/`
  runs stock PS2 `yyz` from `8.0s` after the local-space TransAnim and MatAnim
  changes. The log loads the theatre route with `funk1`, `metal_bass`,
  `metal_drummer`, and `metal_keyboard`; `BAND KEYS` enters `[play]`, the
  venue-specific `dw_theatre_drums` kit loads, 16 drummer cues fire with mode
  changes, lighting presets advance to `chorus_okay.pst` /
  `chorus_great.pst`, and authored cameras record 4 regular sweeps plus 3
  `post_switch_cam` moves with zero miss rows. `frame_00700.bmp` is a coherent
  distant theatre-stage route-health frame, not camera-composition parity.
  `analysis/native_validation/crazyonyou_fest_female_singer_post_anim_20260621_current/`
  runs stock PS2 `crazyonyou` from `16.0s` in the fest venue. The log loads
  `alterna1`, `metal_bass`, `metal_drummer`, and `female_singer`, starts
  `singer_active_medium_01`, loads `dw_fest_drums`, decodes fest MatAnim and
  EventTrigger visibility routes, advances lighting and authored cameras, and
  records zero miss rows. This window keeps singer FaceFX at `graph=idle`, so
  keep it as fest/female-singer route-health evidence rather than active vocal
  performance proof; the separate `shout_singer_active_close_valid` capture
  remains the current active singer/FaceFX proof.
- 2026-06-22 small1 live venue-animation debug validation:
  `analysis/native_validation/venue_anim_debug_small1_psychobilly_20260622_current/`
  reruns stock PS2 `psychobilly` from `10.0s` with diagnostic autoplay and
  `GHOGX_DEBUG_VENUE_FILTERS=1`. The bounded hidden run exits cleanly after
  480 frames, records 1,425 `venue AnimFilter sample` rows, 96
  `venue ParticleSys sample` rows, 244 `venue event ... AnimFilter` starts,
  22 venue `MatAnim` event rows, 4 active lighting keyframes, 3 active lighting
  presets, 2 regular camera sweeps, 3 `post_switch_cam` moves, and zero misses
  or unsupported channel rows. The sampled paths include persistent
  `excitement_okay` ceiling-swing filters and repeated player-fret
  `speaker_cone*.filt` filters driven by diagnostic autoplay hits. Captured
  frames `frame_00180.bmp`, `frame_00300.bmp`, and `frame_00470.bmp` are
  coherent authored-camera small1 stage renders with band, drum kit, TVs,
  speakers, particles, lighting, and venue animation active.
- 2026-06-22 fest LightAnim live-sampling validation:
  `analysis/native_validation/venue_lightanim_debug_fest_badreputation_20260622_current/`
  reruns stock PS2 `badreputation` from `15.0s` with diagnostic autoplay,
  `GHOGX_DEBUG_VENUE_FILTERS=1`, and the existing opt-in
  `GHOGX_ENABLE_ENVIRON_DYNAMIC_LIGHTS=1` bridge. Native debug logging now
  prints EnvAnim/LightAnim samples on the same half-second cadence used for
  ParticleSys/AnimFilter samples. The run decodes `stage_angel.lnm` and
  `stage_gargoyle.lnm`, dispatches the real MIDI `venue_effect` cue at
  `t=19.400`, starts both LightAnims through that event, records 16
  `venue LightAnim sample` rows with advancing frame/color values from frame
  `0.00` through `105.00`, and keeps companion ParticleSys, MeshAnim,
  AnimFilter, lighting-keyframe, regular-camera, and post-switch routes active.
  Captured frames `frame_00180.bmp`, `frame_00300.bmp`, and `frame_00470.bmp`
  remain coherent fest-stage renders. Because the dynamic-light bridge remains
  opt-in pending final color parity, treat this as route/sampling evidence, not
  a signoff that authored lighting color balance is final.
- 2026-06-22 arena EnvAnim live-sampling validation:
  `analysis/native_validation/venue_envanim_debug_arena_shout_20260622_current/`
  reruns stock PS2 `shoutatthedevil` from `100.0s` with diagnostic autoplay and
  `GHOGX_DEBUG_VENUE_FILTERS=1`. The run decodes `coplight_blue.enm` and
  `coplight_red.enm`, routes them through `excitement_great.trig`, records 30
  `venue EnvAnim sample` rows with cycling color values, and keeps the arena
  fret-light MatAnim routes, flame ParticleSys cues, 3,082 AnimFilter samples,
  15 lighting keyframes, regular cameras, and post-switch cameras active with
  zero misses or unsupported channel rows. Captures `frame_00180.bmp`,
  `frame_00300.bmp`, and `frame_00530.bmp` are coherent authored-camera arena
  renders; use this as current proof that Environ color overrides advance in a
  real song window.
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
- 2026-06-22 solo and forced camera mode follow-up:
  `world_objects_worldbase.dta::pick_regular_camera_shot` filters CamShots to
  `solo (ok never)`, while `pick_solo_camera_shot` filters to `solo (ok only)`
  and does not apply the regular far/behind repeat-distance guard. Native was
  discarding `solo=only` CamShots during load, so authored solo shots such as
  `SOLO_NEAR01` could never be selected. Native now keeps `solo=only` shots,
  switches the shared camera selector into solo mode from the current authored
  section, and logs `mode=solo`/`mode=regular`/`mode=jump` for validation.
  The same pass wires the script-backed forced routes: `band_jump` uses the
  decoded `jump_ok` predicate only above bad excitement, while
  `sync_wag`/`sync_head_bang` force a new shot only above okay excitement.
  Validation in
  `analysis/native_validation/camera_solo_mode_jordan_20260622_current_clean/`
  runs stock PS2 `jordan` from `105.0s`; the log loads `SOLO_NEAR01`, enters
  `[solo_on]`, picks `SOLO_NEAR01` with `mode=solo`, runs
  `post_switch_cam`, and keeps lighting on `request=SOLO/`. Regression
  validation in
  `analysis/native_validation/camera_regular_mode_shout_20260622_current/`
  runs stock PS2 `shoutatthedevil` from `16.0s`; it picks
  `flr_far_rt02x3` with `mode=regular`, keeps the `VERSE/color1` lighting
  route, and runs `post_switch_cam`. The `.lit`/`.env` "no decoded" rows in
  both logs are the existing lighting-graph breadcrumbs documented above, not
  new camera-route failures.

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
  `kStartGuitarist0`. Current native quickplay has one guitarist, so
  `guitarist0` now starts from the decoded `kStartGuitarist0` flag/name route;
  the shared start-waypoint helper still honors decoded flags before falling
  back to waypoint names, so a future second-guitarist route can explicitly
  request the MP flag order instead of treating the multiplayer start as the
  only valid performer root. Singer, keyboardist, bassist, drummer, and the
  drum kit keep their existing `start_flags` routes. Validation probes:
  `engine/out/codex_goal_20260619_start_probe_yyz_after.log` and
  `engine/out/codex_goal_20260619_start_probe_shout_after.log`.
  `engine/out/codex_goal_20260619_start_probe_sweep/summary.txt` extends the
  same hidden 3-frame route check across `small1`, `fest`, `battle`, and
  `arena` songs; every loaded performer reports a decoded start transform.
  `analysis/native_validation/start_flag_fallback_20260622_current/run.log`
  rechecks the current native single-guitarist `shoutatthedevil` route with
  performer-start logging: `guitarist0 start xfm flags=1`, singer flag `4`,
  bassist flag `16`, drummer flag `32`, the arena drum kit, regular camera, and
  lighting keyframe rows all resolve in the same hidden song window.

2026-06-22 crowd-lighter camera route:

- Source route:
  `_community_re/Guitar-Hero-II-Deluxe-Unified/_ark/world/crowd.dta`
  dispatches `crowd_lighters_slow` and `crowd_lighters_fast` by recording the
  old `[lighter]` state, setting the new lighter tempo, and calling
  `world pick_lighter_shot` only when `world get did_lighter_cam` is false and
  the old lighter state was `off`. `crowd_lighters_off` sets `[lighter] off`
  and calls `world force_pick_shot`.
- Source world behavior:
  `_community_re/Guitar-Hero-II-Deluxe-Unified/_ark/world/world_objects_worldbase.dta`
  defines `LIGHTER_SHOT_DURATION` as `5`. `pick_lighter_shot` sets
  `[camera_bars_left]` to that value, sets `[did_lighter_cam] TRUE`, and
  performs `pick_shot LIGHTER`. `force_pick_shot` instead sets
  `[camera_bars_left]` through `get_shot_duration` and then calls
  `pick_new_shot`.
- Native implication: authored `LIGHTER` CamShots must live in the decoded
  camera pool even though the stock PS2 `lighter` CamShot is authored
  `special=1`. The regular, solo, and jump routes reject `lighter` shots; the
  lighter route accepts only `lighter` shots and deliberately checks that
  before the regular `special` rejection.
- Stock PS2 `shoutatthedevil` EVENTS MIDI timing used for the native check:
  `[crowd_lighters_fast]` at `156.941s`, `[band_jump]` at `159.481s`, and
  `[crowd_lighters_off]` with `[sync_wag]` at `161.997s`.
- Current native validation:
  `analysis/native_validation/camera_lighter_shout_20260622_current/`.
  The log loads `regular CamShot lighter ... special=1 ... lighter=1`, then at
  `t=156.950` switches `flr_far_rt02x3 -> lighter` with
  `duration=lighter[5,5] mode=lighter forced=1`. At `t=159.483`,
  `[band_jump]` switches `lighter -> flr_far_rt04` with
  `duration=jump[4,4] mode=jump forced=1`. At `t=162.000`,
  `[crowd_lighters_off]` returns through the normal forced-shot duration path
  and activates `chorus_okay.pst`; `chorus_great.pst` follows at `t=162.567`.
  The known `.lit`/`.env` "no decoded" rows remain lighting graph breadcrumbs,
  not camera route failures.

2026-06-22 CamShot crowd visibility route:

- Source route:
  `_community_re/Guitar-Hero-II-Deluxe-Unified/_ark/world/camshot.dta`
  calls `[crowd] crowd_update` on shot start and then sets the crowd's
  rotate flag from the CamShot `crowd_face_camera` property. The same CamShot
  object schema carries `hide_crowd`, visible in stock PS2 CamShot dumps such
  as `analysis/venue_lighting_audit/small2_gen_camshot_dump_20260622.txt`.
- Native now decodes `hide_crowd` and `crowd_face_camera` with the same packed
  bool reader used for `walk_ok`, `low_excitement_ok`, `starpower_ok`,
  `jump_ok`, and `special`. Regular camera pose variants and direct embedded
  intro CamShot poses carry those flags through `CameraKey`. TransAnim-backed
  intro cameras now preserve the selected intro CamShot's crowd flags through
  the intro selector and stamp them onto the loaded intro camera keys before
  runtime visibility is applied, so the `.tnm` route no longer drops the
  metadata.
- Because native does not yet instantiate the full crowd character system, the
  current source-backed visual bridge applies the two CamShot crowd flags to
  the generic authored crowd mesh set built from group names, mesh names, and
  material names containing `crowd`. `hide_crowd` composes that set into
  `composed_venue_hidden_meshes()`. `crowd_face_camera` now sends the same
  decoded set to the venue renderer, which yaws those meshes toward the active
  camera after authored mesh transforms/animations are sampled. This layers with
  EventTrigger visibility and material-alpha hiding instead of replacing those
  routes, and remains a generic crowd-route bridge rather than a venue-specific
  mesh hack.
- Validation:
  `analysis/native_validation/camera_crowd_flags_shout_20260622_current/`
  reruns the existing stock `shoutatthedevil` lighter-camera window and proves
  the flag decode without changing the lighter/jump route. The longer follow-up
  `analysis/native_validation/camera_crowd_flags_shout_long_20260622_current/`
  runs the same route until the regular director selects
  `flr_near_rt01xbass.shot`; the log records 15 decoded venue crowd meshes,
  `camera crowd visibility: shot=flr_near_rt01xbass.shot hide=1 meshes=15`,
  then `flr_far_lft03 hide=0 meshes=0`. The run exits `0` and records zero
  miss/unsupported material-channel rows.
  `analysis/native_validation/camera_crowd_face_meshes_shout_20260622_current/`
  reruns the known `crowd_lighters_fast` window from `156.0s` with hidden
  rendering and no screenshots. The log records the forced `lighter` camera
  sweep; the renderer bridge row includes
  `shot=lighter hide=0 meshes=0 face_camera=1 face_meshes=15`, then clears the
  face-camera mesh set on the following jump camera. The run exits `0`.

2026-06-23 CamShot hide-list preservation:

- The local `world_objects_ps2.dta::CamShot` schema includes a shot-level
  `hide_list` object array alongside `hide_crowd`, `crowd_face_camera`, and
  `force_char_lod`. Raw stock PS2 Stone CamShot bodies in
  `analysis/camshot_raw_probe_20260623_current/stone_extract/` confirm the
  packed field shape after the shot category string: `band_POV01` stores PS2
  object-array tag `0x17`, count `3`, then `hill01.mesh`,
  `flowers12.mesh`, and `flowersmall04.mesh`; `band_POV03` stores count `4`
  and includes `drummer` plus the same foliage meshes.
- Native now decodes that authored hide list for intro-selected CamShots,
  direct embedded `CamShot:` intro routes, and regular camera shots. Runtime
  visibility applies `.mesh` refs directly, expands `.grp` refs through the
  decoded venue group map, and treats `crowd` refs as the shared crowd mesh set.
  This composes with `hide_crowd`, EventTrigger visibility, and material-alpha
  hiding instead of replacing those routes.
- Validation:
  `analysis/native_validation/stone_camshot_hidelist_20260623_current/` runs
  Stone/`shoutatthedevil` from the diagnostic song-start window. The log
  decodes hide lists on `band_POV01`, `band_POV02`, `band_POV03`,
  `balcony_lft01`, `balcony_lft02`, `balcony_lft03`, `lighter`, and other
  shots, then applies the selected `band_POV01` list with
  `camera crowd visibility: shot=band_POV01 hide=0 hide_list=3 meshes=3`.
  The run exits `0`. This is source-backed CamShot metadata plumbing, but it
  does not resolve the remaining Stone under-stage/foreground camera mismatch;
  that still requires a focused camera transform/composition trace rather than
  ad hoc mesh hiding.

2026-06-22 battle MatAnim color/texture channel route:

- Stock PS2 `config/gen/songs.dtb` maps `rockthistown` to
  `rockabill1` / `lespaul` / `battle`. A hidden validation window from
  `52.0s` initially rendered the route and camera switches cleanly, but the
  log exposed 12 `MatAnim ... unsupported channel shape` rows on battle
  lighting events. Hex dumps of `world/battle/og/gen/battle_lighting.milo_ps2`
  showed these were shared `MatAnim` channel variants, not battle-specific
  exceptions: the first channel is a 20-byte RGBA+frame color-key stream, and
  the later texture channel is `count` followed by repeated
  `Tex` string + frame rows.
- Native now decodes and samples those shared material channels through the
  existing active `VenueMaterialAnim` path. Material color keys feed renderer
  material RGBA overrides; texture keys feed discrete diffuse texture overrides
  by material name. Animated texture names are added to the same MILO texture
  request before `set_scene`, so texture swaps only use decoded PS2 `Tex`
  entries from their owning MILO.
- Validation:
  `analysis/native_validation/battle_matanim_texture_rockthistown_20260622_current/`
  reruns stock PS2 `rockthistown` with diagnostic autoplay, hidden rendering,
  and `GHOGX_DEBUG_VENUE_FILTERS=1`. The bounded run exits cleanly after
  600 frames, loads `rockabill1` / `metal_singer` / `metal_bass` /
  `metal_drummer`, `dw_battle_drums`, and `battle` venue/lighting. The log
  decodes `fire_tex.mnm` with `texture_keys=3`, `ticker_texanim.mnm` with
  `texture_keys=3`, multiple spark/fire color-key MatAnims, and records
  `unsupported channel shape = 0`, `has no supported material channels = 0`,
  `miss= = 0`, 3 regular camera sweeps, 3 `post_switch_cam` moves, 2 active
  lighting keyframes, and 3 active lighting presets. The active fire and ticker
  textures referenced by those routes are present in
  `battle_lighting.milo_ps2`; the lighting texture loader reports `34/35`
  requested textures because one non-blocking requested texture did not decode,
  but the exercised MatAnim routes are no longer unsupported. Captured frames
  `frame_00180.bmp`, `frame_00360.bmp`, and `frame_00590.bmp` remain coherent
  authored-camera battle renders.

2026-06-22 big venue route validation:

- Stock PS2 `config/gen/songs.dtb` maps `hangar18` to
  `metal1` / `sg` / `big`. `analysis/native_validation/big_venue_debug_hangar18_20260622_current/`
  reruns a hidden native window from `80.0s` with diagnostic autoplay and
  `GHOGX_DEBUG_VENUE_FILTERS=1`.
- The run exits cleanly after 600 frames and loads `big_geom.milo_ps2`,
  `big_lighting.milo_ps2`, `metal1`, `metal_singer`, `metal_bass`,
  `metal_drummer`, and `dw_big_drums`. The venue log records 54 particle
  event starts, 25 AnimFilter starts, 360 live `venue ParticleSys sample`
  rows, 8,511 live `venue AnimFilter sample` rows, 4 lighting keyframes,
  4 lighting presets, 3 regular camera sweeps, and 1 `post_switch_cam` row.
  It records zero `unsupported channel shape`, zero
  `has no supported material channels`, zero decode failures, and zero
  `miss=` / `MISS` rows.
- Captures `frame_00180.bmp`, `frame_00360.bmp`, and `frame_00590.bmp` are
  coherent authored-camera `big` venue renders with fan/speaker animation,
  particle effects, drum kit, performers, lighting, and cameras active. Treat
  this as venue-route health evidence only; it is not a final character fidelity
  signoff.

2026-06-22 cross-venue post-MatAnim sweep:

- `analysis/native_validation/cross_venue_route_sweep_20260622_current/`
  reruns one hidden 240-frame diagnostic-autoplay window for each stock GH2
  quickplay venue after the material color/texture channel decode:
  `arena/shoutatthedevil`, `small1/psychobilly`, `fest/badreputation`,
  `theatre/yyz`, `battle/rockthistown`, `big/hangar18`, and
  `small2/youreallygotme`.
- Every route exits with code `0` and records zero
  `unsupported channel shape`, zero `has no supported material channels`, and
  zero `miss=` / `MISS` rows. Live route counts from `summary.csv`:
  `arena` MatAnim 148 / EnvAnim 10 / ParticleSys 30 / AnimFilter 1,312 /
  cameras 2+1 / lighting presets 2 / keyframes 7; `small1` MatAnim 45 /
  ParticleSys 48 / AnimFilter 482 / cameras 1+1 / presets 3 / keyframes 3;
  `fest` MatAnim 18 / ParticleSys 168 / AnimFilter 16 / MeshAnim 28 /
  cameras 1+1 / presets 1 / keyframes 2; `theatre` MatAnim 19 /
  ParticleSys 65 / AnimFilter 54 / cameras 2+1 / presets 2 / keyframes 14;
  `battle` MatAnim 33 / cameras 2+1 / presets 2 / keyframes 2; `big`
  MatAnim 8 / ParticleSys 144 / AnimFilter 1,519 / cameras 2+1 / presets 3 /
  keyframes 3; `small2` MatAnim 17 / MeshAnim 44 / cameras 1+1 / presets 4 /
  keyframes 6.
- Treat this sweep as route-regression coverage for venue material channels,
  particles, AnimFilters, MeshAnim, authored cameras, and lighting preset
  dispatch across all seven stock GH2 quickplay venues. It is not a final
  dynamic-light color-parity signoff; the decoded environment dynamic-light
  bridge remains opt-in until PS2 trace evidence proves the active
  preset/keyframe-to-light brightness math.
- Post camera-parser checkpoint:
  `analysis/native_validation/venue_lighting_anim_health_after_camera_20260622_current/`
  reruns the same seven stock GH2 venue/song routes after the neutral-basis
  CamShot scanner filter. All seven hidden fixed-step runs exit `0` with zero
  unsupported rows, zero `MISS` / `miss=` rows, zero `no decoded` rows, and
  zero unresolved rows. The broad "error" scan only matches benign
  `failed=0` Light/Environ coverage summaries. Live sample rows remain active
  across venue animation and lighting: arena 31,256 venue / 85 lighting,
  small1 12,447 / 101, fest 31,124 / 111, theatre 22,322 / 155, battle
  7 / 1,814, big 58,466 / 80, and small2 7,539 / 1,501. This preserves the
  venue-lighting/animation route health after the camera parser change; it is
  still not dynamic-light color-parity signoff.

2026-06-22 dynamic environment light A/B:

- `analysis/native_validation/dynamic_light_ab_arena_20260622_current/`
  reruns the same `shoutatthedevil` arena window twice at `100.0s`: once with
  default rendering and once with `GHOGX_ENABLE_ENVIRON_DYNAMIC_LIGHTS=1`.
  Both runs exit cleanly and hit the same camera, preset, and keyframe route
  (`chorus_okay.pst`, then `chorus_great.pst` with `yellowish` / `teal`
  keyframes), so the comparison isolates renderer lighting rather than song
  state.
- The default frame stays coherent. The dynamic-light frame still shifts the
  arena city/backdrop area to a peach/yellow wash, matching the older rejected
  probe. This confirms the raw mesh -> `Group.environ` -> `.lit` fixed-function
  bridge is not safe to enable by default. Keep the dynamic-light bridge gated
  until a PS2 trace proves how active lighting preset/keyframe target states
  should scale or select those decoded Light objects.
- Follow-up PS2 trace:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_lighting_color_focus_retry_bg_20260622.json`
  reruns the accepted stock GH2 savestate through background Retry input
  without foregrounding PCSX2. It reproves live changes in the world lighting
  state and mutable render-color rows: `0x00b78418 + 0x60/+0x70/+0x80` advance
  through LightPreset/list pointers, `0x00b78418 + 0xb4` points at
  `0x007fe790`, and `0x007fe790 + 0x10/+0x14/+0x18` moves through normalized
  RGB-like values such as `(0.278,0.082,0.082)`,
  `(0.106,0.106,0.259)`, and `(0.043,0.082,0.408)`. A static comparison
  against decoded Big/Arena LightPreset target-state RGB rows found no direct
  match, so these rows must not be substituted by averaging spotlight target
  colors or by enabling the existing Environ dynamic-light probe. Treat this as
  a sharper trace target for render-light semantics, not an implementation-ready
  color formula.
- Follow-up PS2 argument-snapshot traces:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_lighting_apply_arg_snap_bg_20260622.json`
  and
  `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_lighting_apply_arg_snap_long_bg_20260622.json`
  were run through the no-focus/background PCSX2 path with lowered scratch
  data placement after the first oversized probe exceeded 32 MB EE RAM before
  launching the emulator. The short pass hit `0x00271288` twice and
  `0x00271a08` twice; the longer pass hit `0x00271288` three times and
  `0x00271a08` three times, with zero retained calls to `0x002716b8`,
  `0x00271778`, `0x00271200`, `0x00280f60`, `0x00280fe8`, `0x00281070`,
  `0x002c6808`, or `0x003b50e0`.
- These traces strengthen the set/request half of the lighting route rather
  than the final render-color half. The parent call shape is stable:
  `set_lighting` receives world lighting state `0x00b78418` and feeds
  `0x00271a08` rows such as `blackout`, `color1`, `color2`,
  `section intro`, `section chorus_1`, `chorus`, `music_start`, and
  `sync_wag`. The corresponding script row at `0x006006b0` contains
  `do_lighting_next_keyframe`, `excitement_level`,
  `ignored_last_light_change`, and `lighting_next_keyframe`, matching the
  native MIDI cue/keyframe gate already implemented from the earlier accepted
  trace notes. A separate `0x00271a08` call passes the live world-light
  subobject at `0x00b78460`, whose sampled rows again expose
  `0x00b784cc -> 0x007fe790`.
- Because neither new run reached a same-window keyframe apply helper or final
  color writer, do not use these traces to enable dynamic Environ lights or to
  derive a new RGB formula. They are accepted evidence that native preset
  request/category traversal should stay shared and list-driven; dynamic
  renderer-color parity still needs a trace window that actually hits the
  `0x002716b8 -> 0x00280f60` apply branch or a confirmed downstream consumer of
  `0x007fe790`.
- Follow-up consumer-side trace:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_env_color_consumer_20260622_resume.json`
  reruns the accepted stock active-song state with no-focus/background PCSX2
  input and a prepatched state. The screenshot is active gameplay, the patched
  state was deleted after capture, and the trace records 86
  `color_interp_find_003a8b88`, 86 `color_interp_replace_003a8f80`, 85
  `color_interp_update_003a9170`, and 44 `color_interp_apply_003a8e38` calls.
  The new `0x003a8e38` hits are still called from `0x003a8f80` with the global
  color-list row at `0x00b784c4` and stack color payloads; they are list
  maintenance for the already-accepted color runner path, not a final
  renderer/dynamic-light writer. Keep the native dynamic Environ light bridge
  disabled by default until a later trace finds the real consumer.
- Accepted trace analysis checkpoint:
  `0x003a8e38` is reached only from `0x003a8f80` (`ra=0x003a9154`) in the
  accepted consumer trace. All retained calls pass stack scratch color rows as
  `a0=0x01ffe7d0`, the global color-list row as `a1=0x00b784c4`, and a stack
  destination as `a3=0x01ffe7e0`; the source payloads carry the familiar
  normalized RGB triples such as `(0.106,0.106,0.259)`,
  `(0.350,0.000,0.350)`, and `(0.043,0.082,0.408)`. `0x003a9170` and
  `0x003a8f80` both keep updating the same `0x00b784c4` list family and
  pointer slots such as `0x007fe790`, `0x00782580`, and `0x00845ca0`. This
  strengthens the decision to leave `GHOGX_ENABLE_ENVIRON_DYNAMIC_LIGHTS` as an
  explicit opt-in: sampled `LightAnim` colors may feed decoded `Light` refs
  when the probe is enabled, but no accepted trace has promoted those values to
  default venue brightness.

2026-06-22 Environ fog / preset-animation flag decode:

- The local `rnd_objects.dta` `Environ` schema lists `fog_enable`,
  `fog_color`, `fog_start`, `fog_end`, and `animate_from_preset`. A retained
  byte audit over venue and lighting `Environ__*` bodies shows the payload
  after the light-ref array as: ambient RGBA, `fog_start` at `base + 0x10`,
  `fog_end` at `base + 0x14`, fog color RGBA at `base + 0x18`, then
  `fog_enable` at `base + 0x28` and `animate_from_preset` at `base + 0x29`.
  Common venue Environs carry `00 01 ...` there (no fog, preset-animated);
  small2 `op_Art_projection.env` carries `01 00 ...` with fog color
  `(0.5, 0.0, 0.5, 1.0)` and range `0..3000`; small2 neon array Environs
  commonly carry `00 00 ...` and are driven by explicit `EnvAnim` routes.
- Native now exposes those fields on decoded `EnvironObj` and logs
  `fog=` / `animate_preset=` with the Environ coverage rows. The renderer
  applies authored per-Environ fog only when `fog_enable` is set and the start
  / end range is sane, with `GHOGX_DISABLE_ENVIRON_FOG=1` as the A/B kill
  switch. This is independent of, and does not enable, the still-gated
  dynamic environment light bridge.
- Follow-up validation found that this decode path was not reaching the
  renderer because lighting-overlay scenes are marked additive and
  `MiloSceneRenderer::draw_impl` skipped the regular mesh and particle passes
  whenever `additive_blend_` was true. That was a renderer gate, not a trace or
  object-routing failure: small2 `projections.grp` maps `back_projection.mesh`
  and `trippy_graphic1.mesh` through `op_Art_projection.env`, and
  `trippy_pojection.mat` has `use_environ` set. Native now draws regular
  overlay meshes and active overlay particles through the same additive blend
  state, while still skipping spotlight template meshes in the normal pass.
  Validation:
  `analysis/native_validation/environ_fog_overlay_draw_20260622_resume/`
  logs `Environ fog active: op_Art_projection.env`, live
  `op_art_projection.enm` samples, active dry-ice particle samples, and exits
  after 80 frames. The screenshot sanity run
  `analysis/native_validation/environ_fog_overlay_draw_capture_20260622_resume/`
  retains `frame_00040.bmp` and `frame_00080.bmp` with visible projection /
  floor-light overlay contribution. These frames prove the overlay draw route
  is no longer blank; they are not final lighting color parity.

2026-06-22 diagnostic stone venue override:

- `config/gh2.dta` lists `stone` in the authored venue set and
  `world/world_objects.dta` merges `stone/stone.dta`, but the stock quickplay
  song routes covered by `songs.dtb` do not exercise it. Native now exposes a
  diagnostic-only `--diagnostic-venue <venue>` route that keeps the resolved
  quickplay song, band, guitar, chart, and performer data, then swaps only the
  venue symbol before the normal world/lighting/chars/drums loaders run.
- `analysis/native_validation/stone_diagnostic_venue_override_20260622_current/`
  reruns stock PS2 `shoutatthedevil` with `--diagnostic-venue stone`,
  diagnostic autoplay, hidden rendering, and screenshots at frames 120/240/350.
  The log shows `arena -> stone`, loads `world/stone/og/gen/stone_geom.milo_ps2`,
  `world/stone/og/gen/stone_lighting.milo_ps2`, and
  `char/og/drums/gen/dw_stone_drums.milo_ps2`, then runs authored regular
  camera sweeps, `post_switch_cam`, lighting presets/keyframes, LightAnim,
  MeshAnim, and AnimFilter samples with zero miss rows and zero unsupported
  material-channel rows. Frames 240 and 350 are coherent authored-camera stone
  venue renders, so `stone` is now covered as venue-runtime breadth evidence.

2026-06-22 venue start EventTrigger dispatch:

- The first stone validation exposed decoded `Start.trig` MatAnim and
  ParticleSys routes that did not sample because native initialized only
  start-event visibility before applying the current excitement state. The
  extracted `EventTrigger__Start.trig` body begins with the lower-case packed
  payload label `start`, followed by filter/group refs, matching the same
  EventTrigger route shape already used for later venue cues.
- Native now dispatches `apply_venue_event("start", false)` after the world
  renderer is created and before the persistent excitement event. This keeps
  `start` as a one-shot trigger instead of replacing the latched excitement
  state, while routing the authored start MatAnims, ParticleSys, AnimFilters,
  and visibility through the shared event path.
- `analysis/native_validation/stone_start_event_dispatch_20260622_current/`
  reruns the same hidden `stone` override window. The log records 8
  `venue event start: MatAnim` rows, 3 `venue event start: ParticleSys` rows,
  3 `venue event start: AnimFilter` rows, and 6 live `venue ParticleSys sample`
  rows, while preserving `dw_stone_drums`, 2 regular camera sweeps, 2
  `post_switch_cam` moves, 2 active lighting presets/keyframes, zero miss rows,
  and zero unsupported material-channel rows. Frame 350 remains a coherent
  authored-camera stone venue render.
- `analysis/native_validation/cross_venue_start_event_regression_20260622_current/`
  reruns short hidden diagnostic-autoplay windows for all seven stock quickplay
  venues after this change. All routes exit `0` with zero miss rows and zero
  unsupported material-channel rows. Authored venue-start routes now fire where
  present: `arena` 4 MatAnim / 16 ParticleSys / 2 AnimFilter, `small1` 14 / 3 /
  3, `fest` 8 / 29 / 4, `theatre` 5 / 9 / 3, `big` 0 / 8 / 1, `battle` 0 / 0 /
  1, and `small2` 0 / 0 / 0 for the geometry venue. The same sweep preserves
  regular cameras, `post_switch_cam`, lighting preset/keyframe dispatch, and
  venue animation samples across the stock routes.

2026-06-22 venue MatAnim validation rows:

- The venue material-animation sampler already applied alpha/color/texture/UV
  overrides each frame, but unlike EnvAnim, LightAnim, ParticleSys, and
  AnimFilter it emitted no live sample rows. This made native validation look
  like start MatAnims fired without proving their channels advanced.
- Native now logs `venue MatAnim sample` rows under `GHOGX_DEBUG_VENUE_FILTERS`
  with the sampled frame, alpha (or `-1` when the route has no alpha channel),
  and channel counts for color, texture, translation, scale, and rotation.
- `analysis/native_validation/venue_matanim_sample_logging_small1_20260622_current/`
  reruns stock PS2 `psychobilly`/`small1` hidden with diagnostic autoplay. The
  log records 14 `venue event start: MatAnim` rows and 42 `venue MatAnim sample`
  rows (for example `tv_nuke.mnm`, `tv_bomb_big.mnm`, `tv_snow.mnm`), while
  preserving particles, AnimFilters, regular camera, `post_switch_cam`,
  lighting presets/keyframes, zero miss rows, and zero unsupported
  material-channel rows. The captured frame 180 is a coherent authored-camera
  small1 venue render with TV/material-heavy stage content visible.
- Follow-up: lighting-overlay MatAnim routes now emit matching
  `lighting MatAnim sample` rows under `GHOGX_DEBUG_VENUE_FILTERS`. Big
  `smoke_lights.mnm` already sampled texture transform channels on the lighting
  overlay, but the lack of live sample rows made the route look weaker than the
  venue MatAnim path in validation. This is an evidence/diagnostic bridge for
  the existing decoded route, not a visual lighting-color parity claim.
  Validation:
  `analysis/native_validation/lighting_matanim_sample_big_20260622_current/`
  reruns stock PS2 `hangar18`/Big from `80.0s` with hidden diagnostic autoplay.
  It records the `smoke_lights.mnm -> smoke_lights.mat` lighting start route,
  40 live `lighting MatAnim sample` rows with texture translation/scale/rotation
  channel counts, 10 lighting keyframe rows, 80 camera rows, and zero
  unsupported, miss, or missing-route rows.
- Resume guard:
  `analysis/native_validation/lighting_route_guard_20260622_resume/` reruns
  the same Big/Hangar 18 hidden fixed-step route with no screenshot capture
  after the dynamic-light trace guardrail was added. The current executable
  exits `0`, loads full geometry/proxy/lighting texture coverage including the
  lighting overlay fallback, records 40 `lighting MatAnim sample` rows, 10
  lighting keyframes, regular/solo camera sweeps, 30,300 venue animation sample
  rows, and zero unsupported, miss, or no-decoded route rows. The still-gated
  `GHOGX_ENABLE_ENVIRON_DYNAMIC_LIGHTS` path remains disabled by default.

2026-06-22 venue section message bridge:

- Stock venue DTBs define venue-local section handlers such as arena
  `chorus`, `verse`, and `solo`; those handlers can in turn call authored
  animation routes like `sparks_on` / `sparks_off`. Native already used the
  same EVENTS text (`[verse]`, `[chorus]`, `[solo]`) for lighting category and
  camera-section state, but did not forward the section messages into the
  shared venue-event router.
- Native now maps `[verse]`, `[chorus]`, and `[solo]` to transient venue
  messages with an independent cursor, including diagnostic-seek skipping.
  Script-side state machines are handled by the venue DTB script bridge below;
  decoded asset routes with matching event keys still remain the only render
  side effects.

2026-06-22 venue DTB script handler bridge:

- The arena venue DTB in
  `analysis/venue_lighting_audit/world_dtb_20260622/world_arena_gen_arena.dtb.dta`
  shows that section handlers are not simple filter aliases: `chorus` sets
  `state_chorus` and calls `sparks_on`, while `sparks_on` only animates
  `sparks.filt` / `sparks_on.filt` when both `state_peak` and `state_chorus`
  are true. Native now loads venue-local handlers and initial `state_*` values
  from `world/<venue>/gen/<venue>.dtb`, evaluates the traced `set [state]`,
  `$this handler`, direct `{*.filt animate}`, and `if` over state properties,
  then fires direct filter commands through internal `@filter:<name>` route
  keys. The internal key keeps handler names such as `sparks_on` from
  colliding with direct filter routes, so conditionals stay authoritative.
- Direct filter commands reuse the existing decoded route tables: MatAnim,
  EnvAnim, LightAnim, ParticleSys, and transform/MeshAnim AnimFilter loaders
  now expose each decoded `.filt` under its internal `@filter:` key. This is
  still a shared DTB/MILO bridge, not an arena-specific spark rule.
- Validation:
  `analysis/native_validation/venue_script_bridge_arena_20260622_current/`
  reruns stock PS2 `shoutatthedevil` on arena hidden with
  `GHOGX_DEBUG_VENUE_FILTERS=1`. The log loads 7 arena script handlers and 3
  state values, executes the opening `[verse]` handler, calls `sparks_off`,
  evaluates the two-state gate false, and records zero `@filter:sparks_on` or
  `venue event @filter:sparks_on` rows. The same run exits `0`, records zero
  miss/unsupported rows, and frame 180 is a coherent authored-camera arena
  render.
- `analysis/native_validation/venue_script_bridge_cross_venue_20260622_current/`
  reruns 180-frame hidden windows for arena, small1, fest, theatre, battle,
  big, and small2 after the bridge. All seven routes exit `0` with zero
  miss/unsupported rows; arena is the only stock route in this sweep with
  venue-local script handlers loaded.

2026-06-22 venue script task scheduler bridge:

- Focused PS2 trace evidence in
  `analysis/pcsx2_trace/venue_task_scheduler_20260622_current/` captured the
  shared task callback path rather than a native guess: `script_task_cb_002c5440`
  constructed a live task once, `$task sleep` callback `0x002c64c0` fired 13
  times, `$task loop` callback `0x002c6528` fired 13 times, and the task tick
  callback `0x002c5e78` ran continuously. Static SLUS xrefs in
  `ps2_task_string_xrefs.json` tie the registered `script_task`, `thread_task`,
  `sleep`, `loop`, `units`, `delay`, `name`, and `script` symbols to the same
  task manager registration block.
- Native now parses arena/small venue task forms from DTB data into shared
  runtime steps: `script_task`, `thread_task`, `(units kTaskSeconds)`,
  `(units kTaskBeats)`, numeric and `{random_float min max}` delays,
  `(name task_name)`, nested `(script ...)`, `delete task_name`,
  `delete [state_object]`, `if {exists task_name}`, `$task sleep`,
  `$task loop`, and `$task set_name`. Seconds delays use `song_time_`; beat
  delays convert through the chart tempo map with `ticks_per_beat`, matching the
  source distinction between `kTaskSeconds` and `kTaskBeats`.
- The scheduler keeps task handles as object-state IDs, so arena
  `set [onscript] {script_task ...}` returns a cancellable object and
  `delete [onscript]` cancels it if it is still pending. Named task lookup is
  shared for small1 `throw_task` and small2 `neon_task`; `$task set_name ""`
  clears a running task's name after it begins, as authored in small1.
- Validation:
  `analysis/native_validation/arena_venue_script_task_chorus_20260622_current/`
  runs stock PS2 `shoutatthedevil` through the native app hidden, with
  `GHOGX_DEBUG_VENUE_FILTERS=1`, diagnostic venue `arena`, and diagnostic
  event `peak_on`. The log reaches the authored first `[chorus]` at `t=25.500`,
  sets `state_chorus=1`, evaluates `sparks_on` true, fires
  `@filter:sparks` / `@filter:sparks_on`, schedules `script_task id=1`
  with `state='onscript' delay=4.000 due=29.500 steps=1`, then runs that task
  at `t=29.500` and fires `@filter:sparks_bounce` ParticleSys routes. Later
  `[verse]` schedules the matching delayed `sparks_bounce_RESET` task, which
  runs at `t=50.000`. This validates scheduler timing against authored arena
  script data; object-level neon controller expansion remains separate work.

2026-06-22 venue intro-start EventTrigger bridge:

- Cross-venue PS2 EventTrigger dumps show an authored `intro_start` payload in
  multiple venue geometry and lighting MILOs: small1 `intro_start`, small2
  `intro.trig -> intro_start` / lighting `start.trig -> intro_start`, big
  `Intro_start.trig -> intro_start`, stone geometry and lighting
  `intro_start`, and battle lighting `intro_start`. Native already dispatched
  the separate `start` one-shot and `intro_end`, but geometry triggers whose
  payload was `intro_start` did not run unless their stripped object name also
  happened to be `start`.
- Native now dispatches `apply_venue_event("intro_start", false)` immediately
  after the decoded `start` one-shot and before persistent excitement replay.
  This keeps the route in the shared EventTrigger/AnimFilter/MatAnim/
  ParticleSys path, rather than adding venue-specific rules. Existing
  `intro_end` dispatch remains the close of the intro lifecycle.
- Validation:
  `analysis/native_validation/venue_intro_start_small2_20260622_current/`
  reruns stock PS2 `youreallygotme` on small2 from song start. The log starts
  `fan_anims.filt` from `intro_start`, records 21 live `venue AnimFilter
  sample` rows for `op_fan.op_fan01/02/03.mesh`, keeps lighting presets active,
  exits `0`, and records zero miss rows. A second hidden run,
  `analysis/native_validation/venue_intro_start_big_override_20260622_current/`,
  uses the diagnostic big-venue override and starts `curtain_rising.filt` from
  `intro_start`, with 216 AnimFilter samples, 214 ParticleSys samples, lighting
  presets active, exit `0`, and zero miss rows. The retained frames are coherent
  intro-camera venue renders, not final camera-composition parity.
- The small2 `neon_controller` DTB type is now bridged through the same script
  loader used for WorldDir handlers. The important source fact is that the
  handler bodies live in the preprocessed
  `world/small2/gen/small2.dtb` `ObjectDir -> types -> neon_controller` rows,
  while the concrete object instances and their `env1..env9` property maps live
  in `world/small2/og/gen/small2_lighting.milo_ps2` as `ObjectDir__neon_a` and
  `ObjectDir__neon_b`.
- Evidence:
  `analysis/venue_lighting_audit/world_dtb_20260622/world_small2_gen_small2.dtb.dta`
  exposes the `neon_controller` handlers plus `ENV_ON`/`ENV_OFF` macros that
  call `animate(dest, period)` on `[envN]`. The extracted object bodies in
  `analysis/venue_object_inventory_20260621_current/small2_lighting_extract/`
  map `neon_a.env1..env9` to `neon_array00.enm..neon_array08.enm` and
  `neon_b.env1..env9` to the `_copy` EnvAnims. The matching
  `analysis/venue_lighting_audit/small2_lighting_eventtrigger_dump_20260622.txt`
  route proves `neon_fast_blink.trig` sends `neon_a.fast_blink` and
  `neon_b.fast_blink`.
- Native validation:
  `analysis/native_validation/small2_neon_controller_20260622_082610/` reruns
  `youreallygotme` on diagnostic `small2` with `neon_fast_blink`. The log loads
  `1 object_types` / `12 object_handlers`, two script objects, and nine
  script-message events. It schedules separate `neon_task` thread tasks for
  `neon_a` and `neon_b`, resolves all 18 referenced neon EnvAnims, and drives
  them to frame `10` then back to frame `0` with the authored periods. The
  smaller neon EnvAnim bodies inherit their two color keys from
  `neon_array00.enm` via a generic `.enm` `keys_owner` path, matching the
  object payloads instead of a small2-specific alias.

2026-06-22 direct EventTrigger transform refs:

- `analysis/venue_lighting_audit/small2_geom_eventtrigger_dump_20260622.txt`
  shows `small2` fret/speaker triggers whose payload labels are the normal
  runtime events (`hit_p0_fret1` through `hit_p0_fret5`) but whose action refs
  point straight at `speaker_cone*.tnm` objects, for example
  `speaker_cone02.tnm`, `speaker_cone13.tnm`, and related speaker-cone rows.
  These are not wrapped in `.filt` objects, so the native
  `EventTrigger -> AnimFilter -> TransAnim` bridge never saw them.
- Native now records direct EventTrigger refs to `.tnm`, `.msnm`, `.meshanim`,
  and `.grp` through the same payload-label/object-name route keys used by
  filter events. Direct refs are expanded into synthetic `VenueAnimFilter`
  rows, then sampled by the existing venue transform/MeshAnim runtime. This is
  intentionally a shared object-reference decoder, not a `small2` or speaker
  special case.
- Synthetic direct filters use authored transform/MeshAnim key durations for
  their frame window, so transient fret events expire through the same active
  venue-animation lifecycle as authored filters.
- Validation:
  `analysis/native_validation/venue_direct_tnm_small2_20260622_current/`
  reruns stock PS2 `youreallygotme` on small2 hidden with diagnostic autoplay.
  The log loads 26 direct synthetic filters, starts 28 `hit_p0_fret*`
  `AnimFilter direct_hit_p0_fret*` events, records 607 live fret-triggered
  `venue AnimFilter sample` rows on `speaker_cone*.mesh`, records zero
  `hit_p0_fret*` "no decoded AnimFilter transforms" rows, exits `0`, and keeps
  zero miss rows. The retained frames are route-validation captures only; their
  current small2 camera composition is visibly low/occluded and should not be
  treated as camera parity evidence.

2026-06-22 direct intro CamShot camera route:

- `analysis/venue_lighting_audit/small2_gen_camshot_dump_20260622.txt` and
  `analysis/venue_lighting_audit/small2_gen_objdir_20260622.txt` show that
  `world/small2/gen/small2.milo_ps2` has authored CamShot entries such as
  `Intro01`, `Intro_fast`, and `intro_encore`, but
  `analysis/venue_lighting_audit/small2_gen_transanim_dump_20260622.txt`
  shows zero `TransAnim` entries in that MILO. Native's intro camera selector
  only accepted CamShots containing a `.tnm` ref, then fell back to absent
  `Intro.tnm`; the six-bar intro window therefore kept rendering from
  `default.cam`.
- Native now accepts `Intro*` CamShot names as intro camera candidates and, when
  no intro CamShot candidate has a `.tnm` route, falls back to an explicit
  `CamShot:<name>` path that decodes the embedded CamShot pose rows with the
  existing `decode_camshot_poses` parser. Venues with authored `.tnm` intro
  routes keep those routes. This is a generic direct-CamShot fallback for
  venues with embedded intro poses, not a small2-specific camera.
- Validation:
  `analysis/native_validation/small2_direct_intro_camshot_20260622_current/`
  reruns stock PS2 `youreallygotme` on small2 hidden with camera debug. The log
  records `intro CamShot Intro01 -> CamShot:Intro01`, loads 2 direct poses from
  body `+0x13E`, emits 480 authored camera samples from `Intro01`, exits `0`,
  and keeps zero miss rows. Frames 120, 300, and 470 are coherent stage renders
  with the full band and small2 venue visible, replacing the earlier
  default-camera low/occluded route-validation frames.
- Follow-up validation:
  `analysis/native_validation/small2_intro_to_regular_camera_20260622_current/`
  extends the same song to 820 frames. The run keeps the direct `Intro01`
  route, starts the post-intro regular camera at `flr_far_lft02`, records one
  `post_switch_cam`, 11 lighting preset/keyframe activations, 61 direct
  `hit_p0_fret*` AnimFilter events, exit `0`, and zero miss/unsupported rows.
  Frames 650, 760, and 819 are coherent regular-camera small2 stage renders
  with venue props, drum kit, lights, and band visible. The old blonde hair
  attachment issue is visible in these captures and remains character-loader
  work, not a venue camera regression.
- Priority regression:
  `analysis/native_validation/theatre_intro_regression_after_direct_camshot_fix_20260622_current/`
  reruns `yyz` in theatre and confirms venues with authored TransAnim intro
  cameras still choose `Intro01 -> Camera01.tnm`, decode 94 position keys and 3
  rotation keys, and never enter the `CamShot:` direct fallback. The companion
  `analysis/native_validation/small2_direct_intro_camshot_after_priority_fix_20260622_current/`
  rerun confirms small2 still chooses `Intro01 -> CamShot:Intro01` with 2 direct
  poses where no `.tnm` intro candidate exists.

2026-06-22 lighting-overlay EventTrigger routes:

- The lighting overlay MILOs contain authored EventTrigger routes beyond the
  earlier MatAnim-only `smoke_lights` path. The retained dumps show examples
  such as `world/battle/og/gen/battle_lighting.milo_ps2` `Start.trig` and
  `effects_excitement_great.trig`, whose refs run through `.filt`, `.grp`,
  `ParticleSys`, direct `MeshAnim`, and visibility lists inside the lighting
  overlay itself. Native now loads separate lighting-overlay ParticleSys,
  AnimFilter/MeshAnim, and visibility route maps from the lighting MILO and
  samples them against the lighting renderer, leaving venue-geometry runtime
  state separate.
- Lighting overlay construction now replays both `start` and `intro_start` so
  overlay triggers whose payload is `intro_start` are not lost when the venue
  geometry already dispatched that event before the lighting renderer existed.
  Diagnostic seek resets the overlay particle/filter/visibility state alongside
  the existing overlay material state.
- `battle_lighting.milo_ps2` includes `ticker_glow.mnm` as a zero-channel
  version-7 MatAnim body: it has the normal material/name header and all six
  channel counts are zero. The event MatAnim route loader now treats same-MILO
  zero-channel MatAnims as inert no-ops, while still leaving non-empty unknown
  channel shapes visible as unsupported logs.
- Validation:
  `analysis/native_validation/lighting_overlay_routes_battle_20260622_rerun/`
  reruns stock PS2 `rockthistown` on `battle` hidden with diagnostic autoplay.
  The log loads 16 lighting-overlay ParticleSys route events and 17
  lighting-overlay AnimFilter route events, starts overlay `start`,
  `intro_start`, and `excitement_great` routes, records 35 live
  `lighting ParticleSys sample` rows, 90 live `lighting AnimFilter sample`
  rows, and `lighting MeshAnim sample` rows for `electric_outlet_anim.mesh`,
  exits `0`, and records zero unsupported material rows, zero overlay no-route
  hit-event spam, and zero miss rows. Frames 180 and 300 are coherent authored
  battle-camera renders.

2026-06-22 lighting-overlay EnvAnim/LightAnim routes:

- The same lighting overlay object inventory shows effect routes that are not
  material-only: `stone_lighting.milo_ps2` has 6 `EnvAnim` objects, 1
  `LightAnim`, 64 `AnimFilter` objects, 35 `ParticleSys` objects, and 16
  `EventTrigger` objects. Its retained EventTrigger dump routes
  `start`/`intro_start` through overlay filters/groups that expand to
  `intro_start*.enm` and `bonfire_stack.lnm`. Native now loads separate
  lighting-overlay EnvAnim/LightAnim route maps from the lighting MILO and
  samples them against the lighting renderer's environment/light color override
  state. This deliberately reuses the shared venue EnvAnim/LightAnim decoder and
  sampler structures; it is not a stone-specific lighting hack.
- Overlay reset/diagnostic seek now clears the lighting EnvAnim/LightAnim
  runtime state and reapplies the authored `start` and `intro_start` overlay
  triggers alongside the previously bridged material, particle, transform, and
  visibility state.
- Validation:
  `analysis/native_validation/lighting_overlay_env_light_stone_20260622_current_clean/`
  reruns stock PS2 `shoutatthedevil` with the diagnostic `stone` venue override
  hidden and diagnostic autoplay. The log decodes lighting overlay
  `bonfire_stack.lnm`, records EventTrigger EnvAnim rows for
  `intro_start5.enm`, `intro_start4.enm`, `intro_start3.enm`,
  `intro_start2.enm`, and `intro_start01.enm`, starts those routes from
  `start`/`intro_start`, starts `bonfire_stack.lnm` from `intro_start`, records
  48 live `lighting EnvAnim sample` rows and 16 live
  `lighting LightAnim sample` rows with advancing frame/color values, exits `0`,
  and records zero unsupported rows, zero miss rows, and zero PowerShell
  redirect wrapper noise. Frames 180, 300, and 420 are retained as route
  validation captures; camera composition parity is not claimed from this
  diagnostic override run.
- Stock-route cross-check:
  `analysis/native_validation/lighting_overlay_envanim_small2_20260622_current/`
  reruns PS2 `youreallygotme` in its authored `small2` venue hidden with
  diagnostic autoplay. The lighting overlay EventTrigger decoder records
  `effects_excitement_bad`, `effects_excitement_okay`, and
  `effects_excitement_great` EnvAnim rows for `op_art_projection.enm` and
  `trippy_projection.enm`; the native run starts both from
  `excitement_great`, records 16 live `lighting EnvAnim sample` rows with
  advancing color/frame values, exits `0`, and records zero unsupported rows,
  zero miss rows, and zero PowerShell redirect wrapper noise. This cross-check
  covers a normal stock venue route rather than only the diagnostic stone
  override.

2026-06-22 EnvAnim route-summary evidence:

- The shared EnvAnim EventTrigger loader now emits the same route-count summary
  row as LightAnim and ParticleSys:
  `venue EnvAnim routes loaded <milo>: <events> events`. This does not change
  runtime behavior; it makes native route validation prove EnvAnim coverage
  with the same summary shape used by the other decoded event-route families.
- `analysis/native_validation/envanim_route_summary_small2_20260622_current/`
  reruns PS2 `youreallygotme` in the authored `small2` venue hidden with
  diagnostic autoplay for 900 frames. The log records
  `venue EnvAnim routes loaded world/small2/og/gen/small2_lighting.milo_ps2:
  9 events`, starts `op_art_projection.enm` and `trippy_projection.enm` from
  `excitement_great`, records 42 live `lighting EnvAnim sample` rows, exits
  `0`, and records zero `unsupported channel shape`, zero
  `has no supported material channels`, zero `MISS` / ` miss=` rows, and zero
  FaceFX animation parse failures after the v1200 song `.voc` parser fix.
  `frame_00600.bmp` is retained as a coherent small2 band/venue render; it is
  route-health evidence, not dynamic-light color parity.

2026-06-22 current cross-route FaceFX/venue smoke:

- `analysis/native_validation/current_cross_route_facefx_after_v1200_20260622/`
  reruns seven stock GH2 route/song pairs after the v1200 FaceFX parser fix:
  `arena/shoutatthedevil`, `small1/psychobilly`, `fest/badreputation`,
  `theatre/yyz`, `battle/rockthistown`, `big/hangar18`, and
  `small2/youreallygotme`. All seven runs exit `0` with zero FaceFX parser
  failures, zero unsupported material-channel rows, zero miss rows, and zero
  PowerShell redirect wrapper noise.
- The sweep proves song FaceFX and venue routes coexist in the current build:
  six vocal songs load 25-curve `.voc` animations, including the two v1200
  files (`psychobilly_dryvox_16m` and `YouReallyGotMe_dryvox_16m`), while
  `theatre/yyz` correctly reports no `songs/yyz/yyz.voc` because it is the
  instrumental keyboardist route. The short `psychobilly` window keeps singer
  FaceFX idle, but the v1200 animation still loads cleanly; longer focused
  singer runs remain the FaceFX motion proof.
- Live route counts from `summary.csv`: `arena` MatAnim 218 / EnvAnim 12 /
  ParticleSys 91 / AnimFilter 3,856 / cameras 2+1 / presets 2 / keyframes 7;
  `small1` MatAnim 85 / ParticleSys 69 / AnimFilter 795 / cameras 1+1 /
  presets 3 / keyframes 3; `fest` MatAnim 77 / ParticleSys 358 /
  AnimFilter 1,488 / MeshAnim 60 / cameras 1+1 / presets 1 / keyframes 2;
  `theatre` MatAnim 47 / ParticleSys 108 / AnimFilter 1,765 /
  cameras 2+1 / presets 2 / keyframes 14; `battle` MatAnim 24 /
  ParticleSys 44 / AnimFilter 148 / MeshAnim 22 / cameras 2+1 / presets 2 /
 keyframes 2; `big` MatAnim 4 / ParticleSys 204 / AnimFilter 2,322 /
  cameras 2+1 / presets 3 / keyframes 3; `small2` MatAnim 14 / EnvAnim 20 /
  ParticleSys 144 / AnimFilter 521 / MeshAnim 42 / cameras 1+1 / presets 4 /
  keyframes 6. This is current route-regression evidence, not final visual
  signoff.

2026-06-22 route-aware venue/lighting diagnostics:

- Native event diagnostics now wait for decoded route ownership before
  reporting a missing venue/lighting route. `apply_lighting_event` returns
  whether it applied a decoded lighting route, and `apply_venue_event` emits a
  single combined `no decoded venue/lighting routes` row only when the event is
  present in a decoded venue or lighting route table, or when
  `--diagnostic-venue-event` explicitly requested that event. This keeps
  gameplay-only cues such as fret, drum, and section markers from looking like
  missing venue routes without suppressing real decoded-route failures.
- `analysis/native_validation/route_aware_event_diagnostics_20260622_gated/`
  reruns the same seven route/song smoke used by the current cross-route pass
  for 300 frames hidden with diagnostic autoplay. All seven runs exit `0`, with
  zero unsupported material-channel rows, zero miss rows, and
  `combined_no_route=0` on every route. The remaining `no_decoded` rows are
  entirely the existing lighting-preset `.lit` / `.env` reference breadcrumbs:
  arena `7`, small1 `8`, fest `7`, theatre `8`, battle `8`, big `11`, and
  small2 `6`. Route-family activity remains present in the same runs, including
  MatAnim, ParticleSys, AnimFilter, MeshAnim, EnvAnim, LightAnim, lighting
  presets, and lighting keyframes where authored by each venue.

2026-06-22 symbolic performer/crowd lighting rig refs:

- The remaining lighting-preset `.lit` / `.env` reference breadcrumbs from the
  route-aware diagnostics were audited against loaded PS2 lighting and venue
  object inventories. Names such as `char_*`, `crowd_*`, `drummer_*`,
  `rim_lighting.lit`, `band.env`, `character.env`, and `drummer.env` are
  symbolic performer/crowd rig refs in the preset tables, not decoded
  `Light`/`Environ` objects in the loaded lighting MILOs. When a matching venue
  geometry object does exist, the coverage pass still counts it as
  `matched_venue` before applying this symbolic classification.
- Native now logs those symbolic rig refs separately and keeps true unresolved
  object refs visible as `ref has no decoded Light/Environ object`. This is a
  diagnostic/coverage refinement only: it does not apply performer lighting to
  `CharRenderer`, does not enable the still-gated dynamic Environ bridge, and
  does not invent character-specific visual offsets.
- `analysis/native_validation/lighting_symbolic_rig_refs_20260622_current/`
  reruns the seven stock GH2 route/song windows for 300 frames hidden with
  diagnostic autoplay. All routes exit `0`, with zero unsupported rows, zero
  miss rows, zero `no decoded` rows, zero true `ref has no decoded` rows, and
  zero coverage rows with nonzero `unmatched`. The former reference breadcrumbs
  are now accounted as symbolic rig refs: arena `7`, small1 `8`, fest `7`,
  theatre `8`, battle `8`, big `11`, and small2 `6`.

2026-06-22 unlabeled LightPreset keyframe fallback:

- The current small2 route exposed a concrete lighting decoder gap:
  `sweep.pset` reports `keyframes=1` in the PS2 `LightPreset` body but has no
  decoded description label, so native previously switched to the preset and
  left the previous spotlight targets alive because `preset->keyframes` was
  empty. The local `LightPreset` DTA describes the keyframe array as the
  authored state sequence; a counted single-frame payload should not be
  dropped just because its description string is absent.
- Native now shares the keyframe payload scanner between labeled records and a
  conservative unlabeled fallback. If decoded labels undershoot the authored
  keyframe count and bytes remain, native emits one explicit
  `unlabeled_<index>` keyframe covering the remaining payload, but the
  unlabeled path accepts only packed Spotlight target-state rows. Without a
  description label there is no reliable boundary before the preset-level
  `.spot`/`.env`/`.lit` tail tables, so those object refs stay preset-level
  and are not promoted to per-keyframe refs. This is still a generic
  `LightPreset` decoder rule, not a small2 or `sweep` special case, and it
  does not enable the still-gated dynamic Environ light bridge.
- `analysis/native_validation/lightpreset_unlabeled_target_only_cross_route_20260622_current/`
  reruns the seven-route smoke for 180 frames after the target-only fallback.
  All routes exit `0` with zero unsupported material-channel rows, zero miss
  rows, zero FaceFX parser failures on vocal songs, and zero wrapper noise.
  The summary records `unlabeled_direct_spots=0` for every route, proving that
  preset tail tables are no longer treated as per-keyframe spot refs.
- In that run, stock `small2/youreallygotme` from `16.0s` records
  `LightPreset sweep.pset ... keyframes=1`, switches to
  `lighting preset active: sweep.pset ... keyframes=1 t=16.983`, then applies
  `lighting keyframe active: sweep.pset[0] 'unlabeled_0'` with 16 targets,
  11 target-state rows, 0 direct spot refs, 8 inferred spots, and 9 active
  spots. `battle/rockthistown` still recovers an unlabeled target-state frame
  for `verse_great.pst`, while the previous tail-only `theatre/yyz`
  pseudo-frame is no longer emitted.

2026-06-22 peak event bridge:

- The accepted source/trace evidence separates peak state from direct MIDI
  trigger data. Stock `config/gen/midi_parsers.dtb` only maps the TRIGGERS
  lighting parser pitches for first/next/prev and effect parser pitch 52 for
  `world venue_effect`; it does not define a peak-on/off trigger. The extracted
  worldbase script maps `kExcitementPeak` to the persistent
  `excitement_peak` handler, while the accepted char script and PCSX2 trace
  notes expose `peak_on_player`, `peak_off_player`, `peak_on`, and `peak_off`
  as authored runtime messages.
- Native now bridges only the exact persistent `excitement_peak` transition to
  transient `peak_on`, and any transition from peak back to another persistent
  excitement event to transient `peak_off`. This keeps peak visuals in the
  shared EventTrigger/AnimFilter/ParticleSys/visibility route path and avoids a
  guessed gameplay threshold for when peak should be entered.
- `--diagnostic-venue-event <event>` was added as a one-shot validation hook.
  It applies the requested persistent venue event after the world/lighting
  route tables have loaded, so route evidence can be captured without
  hardcoding gameplay scoring or star-power state.
- `analysis/native_validation/venue_peak_bridge_20260622_current/` validates
  stock `battle/rockthistown` with hidden D3D capture. `run.log` records the
  forced `diagnostic venue event: excitement_peak`, then
  `venue peak bridge excitement_peak -> peak_on`, followed by Battle lighting
  visibility `show=1 hide=0` and the decoded `electric_fire.part` /
  `electric_sparkssmoke1.part` particle routes. `run_long.log` continues with
  diagnostic autoplay until the native player streak enters
  `excitement_great`; it then records
  `venue peak bridge excitement_great -> peak_off`, lighting visibility
  `show=0 hide=1`, and the matching decoded particle routes. Both runs exit
  `0`; `long_frame_0003.bmp` is retained only as a sanity frame for the loaded
  Battle venue and performers.
- Follow-up validation in
  `analysis/native_validation/venue_peak_bridge_resend_20260622_current/`
  tightened resend semantics. `resend_active_venue_event()` now force-reapplies
  the active persistent event without clearing `active_venue_event_`, so
  `resend_excitement: excitement_peak` does not fabricate a second
  `peak_on`. The run records exactly one
  `venue peak bridge excitement_peak -> peak_on`, exactly one later
  `venue peak bridge excitement_great -> peak_off`, and one
  `resend_excitement: excitement_peak`.
- Follow-up validation in
  `analysis/native_validation/lighting_transient_persistence_battle_20260622_replace_match/`
  closes the overlay lifetime gap for the same bridge. `apply_venue_event()`
  now passes the event persistence bit through lighting overlay routes, and
  lighting `ParticleSys` / `AnimFilter` active rows expire with the same
  decoded duration rule as venue rows. Lighting overlay particle replacement
  also matches venue particle replacement by preserving rows from the other
  persistence class, so a one-shot effect cannot erase an unrelated persistent
  effect that shares the same particle asset. The hidden Battle run records
  `peak_on` and `peak_off` lighting particles as `transient`, with only eight
  total `electric_fire.part` / `electric_sparkssmoke1.part` samples before
  expiry instead of the previous indefinite sampling into the end of the
  capture.
- `analysis/native_validation/arena_peak_script_sparks_20260622_current/`
  validates the source-shaped venue script side of the same bridge. The run
  starts stock `shoutatthedevil` shortly before the authored first chorus,
  forces `excitement_peak`, then lets the real `[chorus]` text event fire.
  The log records `venue script event peak_on`, `state_peak=1`, a first
  `sparks_on` test with `result=0`, then `venue script event chorus`,
  `state_chorus=1`, `sparks_on` with `result=1`, and routed
  `@filter:sparks` / `@filter:sparks_on` ParticleSys events with live
  `sparks.part` samples. When diagnostic autoplay later enters
  `excitement_great`, the bridge fires `peak_off`, the script runs
  `sparks_off`, and `state_peak` returns to `0`. This is venue script/particle
  route validation, not camera-composition signoff.
- Follow-up validation in
  `analysis/native_validation/peak_bridge_route_applied_battle_20260622_final/`
  closes a false missing-route diagnostic from the same Battle peak path. The
  persistent `excitement_peak` event is represented by the traced transient
  `peak_on` bridge rather than by its own EventTrigger table entry, and a
  forced `resend_excitement` of an already-active peak state must not fabricate
  a second `peak_on`. Native now counts the bridge, plus same-peak forced
  resend, as satisfying the persistent event. The focused hidden Battle run
  exits `0` with zero `unsupported`, zero `MISS` / `miss=`, zero
  `no decoded venue/lighting routes`, one `peak_on`, one `peak_off`, one
  `resend_excitement: excitement_peak`, 613 lighting animation samples, 94
  camera rows, and 14 active lighting keyframe rows.

2026-06-22 performer band_jump clip bridge:

- The accepted `BAND_JUMP` macro in `char_objects_ps2.dta` plays `kSyncJump`
  dirty and then resumes the current graph-loop mode; the accepted
  `pcsx2_bandjump_downbeat_row_objects_20260611.json` trace identifies the
  authored `band_jump` row and payload. Native already used `[band_jump]` for
  the forced camera route, but the performer sink side was still camera-only.
- Native now loads a shared performer jump clip from the main driver path:
  first the `sync_jump` `CharClipGroup`, then proven direct clip names
  (`band_jump`, `singer_band_jump`, `bassist_band_jump`). It does not invent
  unobserved `drummer_band_jump` or `keyboard_band_jump` names. The jump plays
  as a transient dirty non-loop base pose while the normal active/idle player
  keeps advancing underneath, then clears after the authored clip duration.
  Face and hand overlay lanes remain independent.
- `analysis/native_validation/performer_band_jump_shout_20260622_current/`
  reruns stock PS2 `shoutatthedevil` hidden from `156.0s` with diagnostic
  autoplay. The log loads `sync_jump -> band_jump` for `glam1`,
  `singer_band_jump` for `metal_singer`, and `bassist_band_jump` for
  `metal_bass`; all three fire at tick `120960` / `t=159.483` with a
  `2.167s` authored duration. The same event still forces the authored jump
  camera (`mode=jump`) and the run exits `0` with zero unsupported, miss, or
  no-decoded route rows. `metal_drummer` has no decoded jump clip in this stock
  asset path, so drummer absence is treated as asset-backed rather than filled
  with a guessed clip name.
- Follow-up: the later seven-route sweep still showed native trying the generic
  `band_jump` fallback for drummer on every route and for keyboard on YYZ,
  producing duplicate `[clip] 'band_jump' not found` rows despite the accepted
  evidence above. Native now only uses generic `band_jump` as the guitarist
  fallback; singer and bassist keep their observed role-specific fallbacks, and
  drummer/keyboard only load a jump if an actual decoded `sync_jump`
  `CharClipGroup` supplies one. Validation:
  `analysis/native_validation/bandjump_role_fallback_20260622_current/`
  reruns arena from the known `band_jump` event window and theatre/YYZ for the
  keyboard load path. Both exits are `0`; guitarist, singer, and bassist still
  load/fire the observed jump clips in arena, and drummer/keyboard produce zero
  generic `band_jump` not-found rows.

2026-06-22 material BLEND_ENUM and projection-lighting pass:

- The local PS2 runtime schema in `system/run/config/macros.dta` defines
  `BLEND_ENUM` as `kBlendDest`, `kBlendSrc`, `kBlendAdd`,
  `kBlendSrcAlpha`, `kBlendSrcAlphaAdd`, `kBlendSubtract`, and
  `kBlendMultiply`. Real small2 material bodies prove the `u32` immediately
  after the object metadata is that enum, not a disposable observed field:
  `Mat__spotlight_default.mat` and `Mat__floor_glowred.mat` decode as
  `kBlendAdd`, `Mat__opArt_projection.mat` and `Mat__spot_circle.mat` decode
  as `kBlendSrcAlphaAdd`, and ordinary opaque/source materials decode as
  `kBlendSrc` or `kBlendSrcAlpha`.
- Native now stores that value in `MatObj::blend` before material color decode
  and maps it to per-material D3D blend state in `MiloSceneRenderer`. This
  replaces the older scene-wide `additive_blend_` / material-alpha heuristic:
  regular venue geometry, lighting overlay meshes, and particles all use the
  authored material blend rather than forcing every overlay draw through one
  additive state or dimming alpha materials globally.
- The retained texture-alpha probe in
  `analysis/native_validation/material_blend_texture_alpha_20260622_resume/`
  decoded the wrapped `Tex__spot_circle.tex`, `Tex__smoke_lights.tex`,
  `Tex__opArt_projection.tex`, and floor glow textures. Their decoded alpha
  channels are fully opaque, so the projection and light-mask softness cannot
  be recovered by alpha-cutting those textures. They are authored RGB masks
  whose visibility comes from material blend, material color, environment, and
  route state.
- Spotlight-owned template meshes are now excluded from the normal overlay pass
  by walking each decoded `Spotlight` group recursively and by also excluding
  its target, circle mesh, and direct instance mesh refs. They remain drawable
  only through the active Spotlight path. Validation in
  `analysis/native_validation/spotlight_owned_mesh_skip_small2_20260622_resume/`
  removes the earlier regular-pass `spotlight_random04/05/06.mesh` alpha rows
  while preserving active spotlight logs.
- Follow-up A/B in
  `analysis/native_validation/spotlight_ab_small2_20260622_resume3/` captured
  stock `small2/youreallygotme` with active Spotlights on and with
  `GHOGX_DISABLE_SPOTLIGHT_INSTANCES=1`. Both runs exit `0`; the frame-80
  captures are visually identical for the large white/black fan shapes while
  the normal run logs only active `master_can.mesh` Spotlight draws. That proves
  those visible fan shapes are regular projection/fog overlay meshes
  (`opArt_projection`, `trippy_projection`, floor fog/glow), not active
 Spotlight instances. This is renderer/route evidence, not final RedOctane
 venue visual signoff.

2026-06-22 lighting Spotlight Set coverage:

- Lighting overlay MILOs carry authored `Set` objects in several venues
  (`fest`, `small1`, `stone`, and `theatre`). Retained PS2 bodies show a
  compact collection shape: 13 bytes of object/base data, a little-endian
  spot-ref count at `+0x0d`, then length-prefixed `.spot` refs. Examples:
  `fest_lighting` `STARS.set` expands to four ceiling-front spotlights, and
  `small1_lighting` `Set Front.set` expands to six front light-can spotlights.
- Native now decodes those lighting `Set` bodies while loading
  `LightPreset` data, treats known `.set` names as object refs rather than
  possible keyframe labels, and expands any direct preset/keyframe Set refs
  into the existing `spot_refs` filter. This keeps Set support inside the
  shared LightPreset/Spotlight path; it does not add venue-specific light
  lists and does not enable the still-gated dynamic Environ light bridge.
- Validation:
  `analysis/native_validation/lighting_set_expansion_fest_small1_20260622_current/`
  reruns hidden diagnostic-autoplay windows for stock `badreputation`/fest and
  `psychobilly`/small1. Both routes exit `0` with zero miss rows, zero
  unsupported material-channel rows, and zero missing venue/lighting-route
  rows. Fest decodes 20 lighting Sets and small1 decodes 5. The exercised stock
 LightPreset bodies still report `sets=0`, so current active spotlight
 selection remains target-state driven; this pass is object-format coverage,
 not a claimed visual lighting-color parity change.

2026-06-22 PS2 Spotlight object default-state decode:

- Accepted PCSX2 trace
  `analysis/ps2_trace/pcsx2_color_runner_scale_20260622_current.json`
  records the stock PS2 Battle route calling `0x00275ee0` into the color
  runner `0x0026f378` for eleven named `Spotlight` objects. The runtime source
  vector handed to `0x00275ee0` matches object-authored default rows for
  non-target special spotlights: `square01_spotlight.spot` receives
  `(1.0, 1.0, 0.878431)`, and `SHADOW_light.spot` receives
  `(0.105882, 0.105882, 0.258824)`.
- Raw stock PS2 `world/battle/og/gen/battle_lighting.milo_ps2` confirms those
  values are in the `Spotlight` body after the embedded Trans parent string.
  If the first post-parent payload string is a `.grp`, RGB starts eight bytes
  after that string (`square01`); if it is a performer/object token, RGB starts
  four bytes after that string (`SHADOW_light`). The following float is the
  authored default intensity/alpha. Targeted beam spotlights keep neutral
  native defaults unless a `LightPreset` target-state row supplies runtime
  color; their nearby float runs also include aim/template fields and are not
  promoted as colors.
- Native now decodes that shared `SpotlightObj` default state, carries it
  through `Gameplay::LightingSpotlight`, and seeds the active spotlight state
  from it before applying any `LightPreset::TargetState` override. This keeps
  the fix in the common Spotlight/LightPreset path; it does not enable
  `GHOGX_ENABLE_ENVIRON_DYNAMIC_LIGHTS` and does not add Battle-specific light
  lists.
- Validation:
  `analysis/native_validation/spotlight_default_state_battle_20260622_rerun/`
  runs stock `rockthistown`/Battle hidden with diagnostic autoplay and fixed
  `0.25s` steps. It exits `0`, captures nonblank Battle frames, logs
  `lighting Spotlights decoded: total=11 default_states=4`, and specifically
  logs the decoded defaults for `square01_spotlight.spot` and
  `SHADOW_light.spot`. The same log continues through authored lighting
  presets/keyframes (`verse_okay`, `verse_great`, `color1`) with no
  unsupported, unresolved, or error rows.

2026-06-22 RndDir proxy venue effects:

- Venue geometry MILOs can carry `RndDir` objects that proxy small, separate
  MILO directories instead of ordinary mesh/group children. Retained PS2
  bodies show the owner object contains the proxy `.milo` path and route atoms:
  small1 beer-toss objects point at
  `../small1_geom_bottle_throw_proxy.milo` with type `bottle_throw`, while big
  flashpot objects point at `../big_geom_flashpot_proxy.milo` and include the
  `venue_effect` alias. The actual proxy MILOs are present in the stock PS2
  ARK as same-`gen` `.milo_ps2` entries and contain their own Mesh, Mat,
  TransAnim, MatAnim, MeshAnim, ParticleSys, and ParticleSysAnim bodies.
- Native now loads those proxy RndDirs as hidden overlay scenes, using the same
  PS2 MILO decoders and the venue camera. `RndDir` type handlers are loaded
  from the venue DTB alongside `ObjectDir` handlers, so small1's authored
  `bottle_throw` `start`/`throw`/`stop` script drives `set_showing`,
  `stop_animation`, and `$this animate range 0 63` rather than a hand-coded
  beer effect. EventTriggers can also resolve proxy objects through a Group
  ref, and proxy body event aliases route to a default `start` message; this
  is what lets big's `venue_effect` start both flashpot proxies.
- Validation:
  `analysis/native_validation/rnddir_proxy_routes_20260622_after_routes/`
  runs hidden fixed-step windows for small1/`psychobilly` and big/`hangar18`.
  Small1 logs `1 object_types / 3 object_handlers`, routes
  `excitement_great_bottles.trig` to the four beer RndDirs, then executes
  delayed `showing=1` and `animate 0.00..63.00` rows from the DTB task script.
  Big logs both `big_geom_flashpot_PROXY` objects with 15 TransAnim targets,
  7 MatAnims, 6 particle routes, routes `flashpots.trig` through the group,
  and starts both proxies from the `venue_effect` alias. The final spot-check
  in `analysis/native_validation/rnddir_proxy_routes_20260622_final_spotcheck/`
  confirms small1 no longer has stray aliases while big still fires
  `venue_effect`.
- 2026-06-22 compact MeshAnim UV follow-up:
  `analysis/venue_proxy_rnddir_20260622/big_extract/MeshAnim__flashpot_flash.msnm`
  is not a 3-float vertex-position MeshAnim. It targets
  `flashpot_flash.mesh`, stores two 4-vertex UV frames, and carries a 15-frame
  duration. Native now falls through from false-positive position count pairs
  into a shared compact UV MeshAnim decoder, samples those UV frames alongside
  existing vertex-position MeshAnims, and passes exact-vertex-count UV
  overrides through the common renderer. Validation:
  `analysis/native_validation/meshanim_uv_big_flashpot_20260622_final/`
  runs hidden stock PS2 `hangar18`/Big with diagnostic `venue_effect`;
  `RndDir MeshAnim flashpot_flash.msnm -> flashpot_flash.mesh` logs
  `frames=2 uv_frames=2 verts=4 duration=15.0`, both flashpot proxies report
  `mesh_anims=1`, the authored proxy `animate 0.00..100.00` rows run, and
  captured frames 40/70 render without unsupported, miss, or route-error rows.
  Cross-route regression:
  `analysis/native_validation/meshanim_uv_cross_route_sweep_20260622_current/`
  reruns arena, small1, fest, theatre, battle, big, and small2 in hidden
  fixed-step windows after the shared renderer UV override path. All seven
  exits are `0` with zero unsupported rows and zero miss rows; the only broad
  parser hits are benign `failed=0` Light/Environ coverage summaries.

2026-06-22 lighting overlay geometry texture fallback:

- Native texture diagnostics in
  `analysis/native_validation/lighting_texture_missing_names_20260622_current/`
  showed lighting overlay MILOs requesting textures that were not `Tex`
  entries in the overlay itself: arena `track_light_obj.tex`, small1
  `op_stagelight01.tex`, battle `bat_lampsmall.tex`, big
  `metal_with_light.tex`, and small2 `op_glowblue.tex` / `op_pat01.tex`.
  Direct PS2 ARK object-directory dumps prove each missing overlay texture is
  present in the paired venue geometry MILO. This is a PS2 resource-layout
  dependency, not six venue-specific missing-art cases.
- Native now exposes `asset::load_milo_textures_from_sources`, which decodes
  the first source containing each requested `Tex` while preserving primary
  source precedence. Lighting overlays request textures from
  `[<venue>_lighting.milo_ps2, <venue>_geom.milo_ps2]`, so overlay-local
  textures still win and shared geometry textures only fill real overlay
  misses. The path is generic and has no texture-name special cases.
- Validation:
  `analysis/native_validation/lighting_texture_geom_fallback_20260622_current/`
  reruns the five formerly short overlay routes with
  `GHOGX_DEBUG_TEXTURE_LOAD=1`; all exits are `0` and the affected overlays
  now report full texture coverage from two sources: arena `9/9`, small1
  `6/6`, battle `35/35`, big `6/6`, and small2 `17/17`. The wider sweep in
  `analysis/native_validation/lighting_texture_geom_fallback_route_sweep_20260622_current/`
  reruns arena, small1, fest, theatre, battle, big, and small2 for 120 fixed
  frames. All seven exits are `0`, every lighting overlay reports full
  requested texture coverage, and the parser records `bad_rows=0` for
  unresolved textures, unsupported rows, miss rows, and missing ARK entries.

2026-06-22 lighting overlay geometry material fallback:

- Raw scene scans found the same PS2 resource split at the material-record
  level: lighting overlay meshes can name `.mat` objects that are not stored
  in `<venue>_lighting.milo_ps2` but are present in the paired
  `<venue>_geom.milo_ps2`. Confirmed examples are arena
  `searchlight.mat` / `track light obj.mat`, battle `lightrigface.mat` /
  `lamp.mat`, fest `light_body.mat`, and small2 `op_pat-1.mat`.
- Native now collects exact material references from lighting overlay meshes,
  spotlights, particles, and lighting `MatAnim` targets, then copies only
  those missing `MatObj` records from the already-loaded geometry scene before
  building the lighting texture request list and before `set_scene`. Overlay
  local material records keep precedence, and there are no material-name or
  venue-name special cases.
- Validation:
  `analysis/native_validation/lighting_material_geom_fallback_arena_20260622_current/`
  runs arena `shoutatthedevil` for 260 hidden fixed-step frames with
  screenshots at 80/160/240. It exits `0` and logs:
  `lighting material fallback: borrowed 2 from venue geometry searchlight.mat
  track light obj.mat`, with zero unsupported, miss, no decoded, unresolved,
  and error rows.
  `analysis/native_validation/lighting_material_geom_fallback_crosscheck_20260622_current/`
  reruns battle, fest, and small2 for 100 hidden fixed-step frames. All three
  exits are `0`; battle borrows `lightrigface.mat` / `lamp.mat`, fest borrows
  `light_body.mat`, and small2 borrows `op_pat-1.mat`. Each route again has
  zero unsupported, miss, no decoded, unresolved, and error rows while still
  sampling lighting MatAnim plus route-specific venue MeshAnim/AnimFilter
  activity.

2026-06-22 CamShot force-char-LOD preservation:

- `world/gen/camshot.dta` calls `world set_min_lod [force_char_lod]` when a
  CamShot starts. A fresh stock PS2 audit in
  `analysis/camshot_property_audit_20260622_current/` dumps all eight venue
  CamShot pools and confirms the field is not just schema noise: every venue
  has authored `force_char_lod` values, with the common pattern that `band_POV*`
  shots carry `1` while many normal/intro/near shots carry `-1`.
- Native now preserves that signed packed-MILO int through the shared CamShot
  property reader and carries it on `CameraKey` for selected intro CamShots,
  direct `CamShot:` intro fallback poses, regular shots, and per-pose camera
  variants. Runtime camera logs include `force_char_lod` beside the existing
  `hide_crowd` / `crowd_face_camera` flags.
- Follow-up implementation: the active CamShot now drives
  `CharRenderer::set_min_lod` for every performer. Negative/no-force values
  clamp back to high-detail character meshes; `force_char_lod >= 1` selects the
  decoded `lod1.grp` membership when that group exists. This keeps the route as
  a shared authored LOD-group rule and avoids character-specific mesh lists.
- Validation:
  `analysis/native_validation/camshot_force_lod_small2_20260622_current/`
  runs stock PS2 `youreallygotme` through small2 with hidden fixed-step
  diagnostic autoplay and screenshots at frames 30/60/80. The run exits `0`,
  records intro/direct camera `force_char_lod=-1`, records
  `band_POV01`/`band_POV02`/`band_POV03` with `force_char_lod=1`, and has zero
  unsupported, no decoded, unresolved, missing, `MISS`, `miss=`, or error rows.
  Renderer validation:
  `analysis/native_validation/camshot_force_lod_runtime_small2_20260622_current/`
  reruns the same small2 route from `16.0s` with camera/mesh/filter debug.
  The run exits `0`, selects
  `flr_near_rt04 -> band_POV02 ... force_char_lod=1` at `t=57.266`, emits
  four `[char3d] min_lod active: 1` rows for the active performers, keeps
  `post_switch_cam` on `band_POV02`, records 8,751 venue/lighting sample rows,
  and has no game/runtime unsupported, missing, miss, unresolved, or error
  rows. The only `error` text in the log is PowerShell's redirected native
  stderr wrapper around normal app output.

2026-06-22 resumed venue/lighting focus validation:

- Current `Debug` `ghogx_app` rebuild and `ctest --test-dir
  out/build/engine-vs -C Debug --output-on-failure` pass locally, including the
  venue/band orchestration contract and all character contract guards.
- `analysis/native_validation/resume_venue_lighting_focus_20260622_current/`
  reruns four hidden fixed-step native routes against the stock PS2 `GEN` ARK
  after the camera/lighting/proxy/LOD work: `small2/youreallygotme` for direct
  intro cameras, lighting EnvAnim, direct event refs, and force-char-LOD;
  `big/hangar18` with diagnostic `venue_effect` for RndDir proxy flashpots and
  compact UV MeshAnim; `battle/rockthistown` with diagnostic
  `excitement_peak` for peak bridge, spotlight defaults, overlay particles, and
  camera cuts; and diagnostic `stone/shoutatthedevil` for overlay EnvAnim /
  LightAnim / material fallback coverage.
- All four routes exit `0` with zero unsupported rows, zero miss rows, zero
  combined decoded-route misses, zero unresolved rows, zero missing rows, zero
  nonzero `failed=` coverage rows, and zero real error rows. The only retained
  `no decoded` text is Stone `char.env`, which the symbolic performer/crowd rig
  classifier keeps out of the true unresolved bucket.
- Route evidence remains live in the same sweep: small2 records 340 camera rows,
  17 regular sweeps, 27 `post_switch_cam` moves, 90 lighting keyframes, 340
  lighting EnvAnim samples, 1,990 MeshAnim samples, and 17,358 venue AnimFilter
  samples. Big records 14 RndDir rows, flashpot proxy load/animate rows, 15
  regular sweeps, 4 `post_switch_cam` moves, 18 lighting keyframes, 1,536 venue
  particle samples, and 73,570 venue AnimFilter samples. Battle records the
  exact `peak_on` / `peak_off` bridge, 260 camera rows, 32 lighting keyframes,
  631 lighting particle samples, 1,149 lighting AnimFilter samples, and the
  traced `square01_spotlight.spot` / `SHADOW_light.spot` defaults. Stone records
  220 camera rows, 38 lighting keyframes, 330 lighting EnvAnim samples, 110
  lighting LightAnim samples, 3,824 lighting particle samples, and 7,328 venue
  AnimFilter samples.
- Focused LOD proof in
  `analysis/native_validation/resume_venue_lighting_focus_20260622_current/small2_lod_debug_meshes/`
  reruns the opening small2 `band_POV02` window with `GHOGX_DEBUG_MESHES=1`.
  It records `force_char_lod=1` on the selected CamShot, eight renderer-side
  `[char3d] min_lod active` rows, then a clean return to `min_lod active: 0`
  when the camera switches back to a non-forced shot. This validates the runtime
  handoff from decoded CamShot metadata to shared character LOD selection.

2026-06-23 symbolic `char.env` classification follow-up:

- The fresh hidden venue/lighting probe in
  `analysis/native_validation/venue_lighting_probe_20260623_current/` reran
  small2, battle, big, stone, and theatre routes after the crowd-camera work.
  All five exits are `0`; every route keeps live camera sweeps, lighting
  keyframes, and venue/lighting animation samples with zero unsupported,
  miss, unresolved, missing, or nonzero failed rows. The only true route-health
  false positive was Stone logging
  `lighting preset .env ref has no decoded Environ object: char.env`.
- `char.env` is the same symbolic performer/crowd lighting-rig family as the
  already-classified `band.env`, `character.env`, and `drummer.env`, not a
  decoded `Environ` object to instantiate as a renderer light. Native now keeps
  it in the shared symbolic `.env` classifier so health sweeps do not report it
  as a missing decoded route.
- Validation:
  `analysis/native_validation/symbolic_char_env_probe_20260623_current/`
  reruns diagnostic `stone/shoutatthedevil` hidden for 160 fixed-step frames.
  It exits `0`, records one
  `lighting preset .env performer/crowd rig ref: char.env` row, records zero
  `no decoded` rows and zero unmatched env coverage, and keeps 160 camera rows,
  8 regular sweeps, 36 lighting keyframes, 3,814 lighting samples, and 7,536
  venue samples.

2026-06-23 static scene `Draw.showing` decode:

- The theatre/YYZ native visual probe exposed a pale rectangular slab that
  survived `GHOGX_ONLY_PERFORMER=__none__` and
  `GHOGX_DISABLE_SPOTLIGHT_INSTANCES=1`, so it was not a character, prop, drum
  kit, or active spotlight instance. The systemic mismatch was in the static
  scene Mesh decoder: skinned character meshes and ParticleSys already consume
  the Draw-base `showing` byte, but `milo_scene::decode_mesh` skipped that byte
  as part of the remaining Draw body and defaulted every venue mesh visible.
- Native now decodes `MeshObj::showing` from the same Draw-base byte and seeds
  the existing venue/lighting runtime hidden-mesh sets from meshes authored
  hidden at load time. EventTrigger and script show/hide routes can still
  remove a mesh from those sets later. This is a shared MILO format fix, not a
  theatre mesh-name override; authored theatre alpha/light meshes such as
  `flame_shadow.mesh`, `flame_shadow_2.mesh`, and `shadow_clip_mask.mesh` still
  log and route after the change.
- Validation:
  `cmake --build out/build/engine-vs --config Debug` and
  `ctest --test-dir out/build/engine-vs -C Debug -R milo_scene
  --output-on-failure` pass. The hidden native probe in
  `analysis/native_validation/theatre_slab_showing_runtime_hidden_20260623_current/`
  reruns stock PS2 `yyz` in `theatre` from `16.0s` with frame 80 captured. It
  exits `0`; `frame_00080.bmp` no longer shows the pale slab over the stage,
  while the log still records the expected theatre venue, lighting, AnimFilter,
  and alpha mesh routes.
- Post-change route sweep:
  `analysis/native_validation/showing_decode_route_sweep_20260623_current/`
  runs theatre/`yyz`, Big/`hangar18` with `venue_effect`, Battle/
  `rockthistown` with `excitement_peak`, small2/`youreallygotme`, and Stone/
  `shoutatthedevil` with `excitement_great`. All five exits are `0`; all five
  capture frame 80; log scans show no runtime unsupported, miss, unresolved,
  missing, failed-route, or error rows beyond PowerShell's redirected-stderr
  wrapper. The sweep keeps live venue/lighting samples on every route, proving
  the initial hidden-set seed does not suppress EventTrigger, RndDir,
  AnimFilter, particle, camera, or lighting activity.

2026-06-23 material `prelit` render flag:

- The local `rnd_objects.dta` material schema and the existing PS2 material
  decode both expose `Mat.prelit`, but the renderer still sent every venue mesh
  through the generic fixed-function scene lights. Native now treats decoded
  prelit materials as already carrying authored vertex/texture lighting and
  disables fixed-function relighting for that mesh only. This is a shared
  material flag path, not a venue-specific brightness tweak; it remains A/B-able
  through `GHOGX_DISABLE_PRELIT_MATERIALS=1` and can log the exact mesh/material
  pairs with `GHOGX_LOG_PRELIT_MESHES=1`.
- Validation:
  `analysis/native_validation/prelit_material_renderer_20260623_current/`
  runs hidden A/B captures for stock `shoutatthedevil`/arena and
  `rockthistown`/Battle. Default rendering logs 161 arena and 111 Battle prelit
  mesh/material pairs using the new path; the kill-switch runs log zero by
  design. Both routes keep lighting keyframes active and save frame captures.
  Battle frame 80 demonstrates the effect clearly: the kill-switch frame blows
  the gym walls/props out under the generic light wash, while the decoded prelit
  path keeps the darker authored room tone with the scoreboard, props,
  performers, and lighting still visible. Arena's retained frame still exposes
  the pre-existing camera-composition issue in that shot, so do not use it as a
  camera-parity signoff.

2026-06-23 prelit `use_environ` color handoff:

- A targeted source probe extracted only `world/small2/og/gen/small2_lighting.milo_ps2`
  and `small2_geom.milo_ps2` to a temp directory, inspected the material/group
  bodies, then deleted the temp directory in the same command. The projection
  and glow materials that drive the visually loud small2 overlay are authored
  as both `use_environ=1` and `prelit=1`: `trippy_pojection.mat`,
  `opArt_projection.mat`, `floor_fog_mat.mat`, `neon_glow_*`, and
  `spotlight_default.mat`. The active groups route these meshes through
  decoded Environ objects (`projections.grp` -> `op_Art_projection.env`,
  `op_art_projection.grp` -> `trippy_projection.env`) that are driven by
  `EnvAnim`.
- The previous prelit renderer path correctly disabled fixed-function lighting
  for prelit meshes, but that also meant `Mat.use_environ` color only affected
  non-prelit meshes through `D3DRS_AMBIENT`. Native now preserves the authored
  environment route for prelit materials by folding the current Environ /
  EnvAnim color into the diffuse vertex/material multiplier before disabling
  fixed lighting. This is shared material behavior; it does not name small2,
  projection meshes, or texture assets.
- Validation:
  `analysis/native_validation/prelit_use_environ_small2_20260623_current/`
  reruns stock `youreallygotme` in small2 with diagnostic autoplay,
  `excitement_great`, fixed-step timing, and one retained frame at 80. The app
  exits `0`, the log records the expected `lighting event excitement_great`
  MatAnim/EnvAnim rows for `op_art_projection` and `trippy_projection`, and
  `frame_00080.bmp` shows the projection overlay darkened/tinted by authored
  Environ color instead of rendering as full-strength white masks. The only
  retained `error` text is PowerShell's redirected native stderr wrapper, not a
  runtime route failure. Cleanup audits before and after the run reported zero
  `GH2DXu_PS2_trace_*` staging folders and zero temporary ISO/MDS files.
