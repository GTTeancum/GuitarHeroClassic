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
  ref decode fails. The same raw-probe pass also shows arena shots such as
  `flr_near_rt01x23w` and `flr_near_rt01xbass.shot` can encode an empty target
  entity with target subpart `spot_neck_fret20.mesh`. Native resolves only that
  missing target entity from shot context (`bass` -> `bassist`, default ->
  `guitarist0`) and leaves parent/source refs untouched. Validation:
  `analysis/native_validation/camshot_unqualified_target_current/` exits `0`,
  has no `target=:spot_neck` rows, and logs both
  `target=guitarist0:spot_neck_fret20.mesh` and
  `target=bassist:spot_neck_fret20.mesh`. Cross-route smoke
  `analysis/native_validation/camshot_unqualified_target_crossroute_current/`
  covers arena, small2, battle, big, and theatre with camera, venue event, and
  lighting rows active and the same unresolved-target guard clear.
- 2026-06-23 CamShot parent-rotation byte: the local PS2
  `world_objects_ps2.dta::CamShot` keyframe schema includes
  `use_parent_rotation` immediately after the `parent` ref. Raw stock PS2 arena
  bodies in `analysis/camshot_raw_probe_20260623_current/` match a compact
  byte at that position: `SOLO_NEAR03.shot` has parent
  `guitarist0:spot_neck_fret20.mesh` followed by `01`, while shots such as
  `flr_near_rt02` and `flr_near_lft02` have performer parents followed by
  `00`. Native now decodes that byte into `CameraKey::use_parent_rotation`.
  Parented camera keys always inherit parent translation, but only rotate the
  authored eye/basis/up vectors when this source field is true; target look-at
  remains driven by decoded `targets`. Validation:
  `analysis/native_validation/camshot_parent_rotation_20260623_current/`
  builds from the stock PS2 `GEN` assets, auto-starts arena
  `shoutatthedevil`, seeks to 16s, exits `0`, logs both `parent_rot=0` and
  `parent_rot=1` decoded CamShots, and has no missing-ARK or unsupported rows.
  This removes a broad native composition error for parented shots; it is not
  final arena camera parity, because empty-parent low arena shots still need the
  shared CamShot result/path/projection bridge documented below.
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
- 2026-06-23 GH2DXu camera trace basefix follow-up:
  the accepted direct-route state must be loaded with explicit statefile
  `C:/Games/Emulators/PCSX2/sstates/GHDX-00300 (A9BBA52A).01.p2s` and ELF
  `GuitarHeroOGX-trace360/analysis/ps2_trace/external/Guitar-Hero-II-Deluxe-Unified/out/ps2/GHDX_003.00`.
  Indexed `-state 1` is rejected by PCSX2 for the combined GH2DX image because
  it cannot resolve `SYSTEM.CNF`, and normal EE-base scanning may not find the
  ELF prefix in this state. The usable fallback derives EE base from static ELF
  string guest addresses: for example, live `current_shot` at host
  `region+0x6a840` minus ELF guest `0x00442840` gives
  `ee_base_host=0x7ff800000000`. The retained traces are:
  `analysis/pcsx2_trace/arena_camera_symbol_bridge_current/battle_camera_symbol_bridge_basefix.json`,
  `camera_call_sequence_basefix.json`, and
  `camera_live_00ca9700.json`.
- The basefix traces corrected two tempting but wrong camera assumptions.
  First, the live `current_shot`/`next_shot` cells sampled in that state are
  script metadata/object graph cells, not the currently selected shot-name
  value. They dereference around `world/world_objects_worldbase.dtb`,
  `current_shot`, and `pick_new_shot`, so they cannot be used to correlate a
  final camera output row to a native CamShot by name. Second, the hot
  `0x00263410` route passes moving blocks around `0x00ca9700/10/30/50`, but
  those rows decode as small clip/projection-like values rather than a clean
  world-space render camera. Do not drive native world camera from the
  `0x00ca9700` family and do not treat the symbol cells as live selected-shot
  strings; the remaining camera work is still the shared CamShot
  result/path/projection bridge.
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
- 2026-06-24 rejected target-source camera probe: the diagnostic
  `analysis/native_validation/arena_camera_target_source_probe_clean_20260624_current/`
  temporarily treated decoded CamShot `target` transforms as the source when a
  key had no parent. It moved arena eyes into the positive-Z family (for
  `flr_near_rt01x23w`, from default `eye=(296.96,-90.28,-221.38)` to roughly
  `eye=(139.19,290.92,93.01)`), but the retained frames 80/160/240 were dark
  and occluded. The target-source candidate is therefore useful debug evidence
  only, not a runtime rule. Native keeps parent-only source composition and logs
  target-composed `*_target_eye` rows under `GHOGX_DEBUG_CAMERA=1` for future
  bridge analysis.
- 2026-06-24 render-camera matrix guard: `ghogx_render_camera_matrix_test`
  now pins the native `Mat4::look_at_lh` bridge against the accepted
  `gdx_cam_output_00ceaa20` rows from
  `GuitarHeroOGX-trace360/analysis/ps2_trace/gh2dxu_arena_camera_relocated_rows_20260623.json`.
  Feeding the traced forward, position, and up rows into the native view-matrix
  builder reproduces the PS2 derived right/up/forward rows and translation
  block within rounding tolerance. This is not a camera-framing fix; it keeps
  the remaining work scoped to CamShot result/path/source composition instead
  of reworking the already-matching D3D view-matrix handoff.
- 2026-06-24 native camera matrix diagnostic: `GHOGX_LOG_CAMERA_MATRIX=1`
  now logs the submitted world-pass render camera in the same useful shape as
  the accepted PS2 row family: output forward/position/right/up, derived view
  rows, and submitted projection rows after screen-offset application. The
  focused arena validation in
  `analysis/native_validation/arena_camera_matrix_log_20260624_current/`
  reruns stock PS2 `shoutatthedevil` hidden from `16.0s` with
  `GHOGX_DEBUG_CAMERA=1` and `GHOGX_DEBUG_VENUE_FILTERS=1`; it exits `0`,
  records 260 `[camera]` rows and 2,600 `[camera-matrix]` rows (one 10-row
  block per frame), retains frames `80/160/240`, and has no unsupported, miss,
  missing, unresolved, or real error rows beyond the expected `failed=0`
  lighting coverage summaries. The first retained block for
  `flr_near_rt01x23w` logs native output
  `forward=(0.877600,0.479394,-0.000000)`,
  `position=(296.961395,-90.276611,-221.383392)`, view translation
  `(73.170937,304.561920,-217.335236)`, and projection rows from the submitted
  `fov=0.602416` / `aspect=1.777778` matrix. This gives native validation a
  PS2-comparable final-row surface while preserving the current camera behavior;
  it confirms that the remaining visual mismatch is still upstream in the
  CamShot result/path/source bridge.
- 2026-06-29 PS2 projection/aspect validation diagnostic:
  `GHOGX_CAMERA_ASPECT=<float>` overrides the submitted venue-camera projection
  aspect while preserving the accepted camera row, fov, screen offset, and all
  venue visibility/event state. This is an evidence-only diagnostic for checking
  whether the current 16:9 native backbuffer exposes geometry that the accepted
  PS2 trace's 4:3-ish viewport cropped out; it is not a mesh visibility hack.
- 2026-06-29 additive Mat alpha bridge:
  the accepted balcony validation showed `searchlight_glows_off.mnm` sampling
  `light_glow.mat` down to alpha `0.035`, but the native frame still rendered a
  full-strength white glow. The renderer's pure `kBlendAdd` path uses D3D
  `ONE,ONE`, so vertex alpha is ignored by the blend stage. Native now applies
  Mat alpha as RGB emissive intensity for pure additive materials while keeping
  alpha-based blends on their existing `SRCALPHA` paths.
- 2026-06-29 rendered WorldCrowd actor runtime:
  accepted balcony_lft04 validation showed decoded `WorldCrowd` actor sources
  driving camera evidence while the native frame still lacked the foreground
  crowd bodies. Native now promotes decoded `world/*_chars.milo_ps2`
  `WorldCrowd` placement sets into a render runtime: it resolves each stock
  `char/crowd/og/gen/*.milo_ps2` actor, loads that actor's driver-authored
  crowd clip set, samples it through the shared `CharClipPlayer` /
  `apply_clip_channel_layers` path, applies shared character controllers, and
  draws the actor Character at the decoded authored placement rows. A
  `GHOGX_WORLDCROWD_RENDER_BASIS=area_local` diagnostic can still compare the
  camera/source area-local basis, but native rendering defaults to raw
  placements so the entire crowd is not incorrectly dragged into camera-source
  space. WorldCrowd renderers opt into `CharRenderer` scene-lighting mode and
  draw after the venue pass but before the lighting overlay, so crowd actors no
  longer use the standalone bright character-viewer light rig. Follow-up native
  validation showed the inherited venue D3D state was still too bright for the
  accepted dark balcony silhouettes, so WorldCrowd now also consumes the active
  LightPreset's symbolic performer/crowd rig refs (`char_*`, `crowd_*`,
  `band.env`, `character.env`, etc.) and modulates decoded crowd Character
  diffuse color by the current authored excitement state. This keeps the dimming
  tied to the same symbolic rig refs already classified by lighting coverage,
  rather than to a balcony-shot mesh hide or one-off visual patch.
- 2026-06-29 WorldCrowd low-light validation follow-up:
  `analysis/native_validation/native_probe_lighting_alpha_repush_20260629_075419/`
  reruns the accepted `arena/balcony_lft04` gate from the stock PS2 `GEN`
  assets. Native logs `WorldCrowd runtime ready: actors=5 placements=450
  basis=placement`, selects `bad.pst` / keyframe `bad`, and maps the symbolic
  low performer/crowd rig to `rgb=(0.192 0.088 0.035)`. `CharRenderer`
  scene-composite meshes now use their symbolic vertex color directly instead
  of re-enabling the standalone D3D mesh lights over the top. The same
  low-excitement exposure is also applied to decoded active spotlight states
  and to lighting-overlay floor/crowd highlight materials owned by decoded
  meshes such as `crowd_highlight.mesh` and `floor_hot*.mesh`; the validation
  log shows `stadium_highlights.mat` at alpha `0.180` instead of the previous
  `1.000`. The retained screenshot
  `C:\Users\smmel\AppData\Local\Temp\gh2_lighting_animation_screens\native_worldcrowd_lighting_alpha_repush_frame_00001.png`
  and side-by-side
  `C:\Users\smmel\AppData\Local\Temp\gh2_lighting_animation_screens\ps2_vs_native_worldcrowd_lighting_alpha_repush.png`
  show the native floor bloom is much lower while the crowd actors, decoded
  lighting keyframe, and authored spot targets remain active. Remaining visual
  mismatch: native still exposes more venue/crowd area and a stronger blue /
  purple contribution than the accepted PS2 crop, so this is progress toward
  the trace gate rather than final lighting-camera parity.
- 2026-06-24 CamShot keyframe timing decode/scheduler pass:
  `world_objects_ps2.dta::CamShot` documents `duration`, `blend`, and
  `blend_ease` immediately before `field_of_view` in each keyframe. Native now
  preserves those fields on every decoded `CameraKey`, logs them in
  `[camera-candidate]` and `[world] regular CamShot` rows, and uses sane
  authored timing for same-shot `post_switch_cam` position variants only.
  Cross-shot `start_shot` changes still cut between authored shot families, and
  zero or outlier timing falls back to the prior native `2.06s` position
  interval / `1.25s` blend. Validation in
  `analysis/native_validation/arena_camshot_timing_schedule_20260624_current/`
  reruns stock PS2 `shoutatthedevil` hidden from `16.0s` with camera, matrix,
  and venue-filter debug enabled; it exits `0`, records 260 `[camera]` rows,
  2,600 `[camera-matrix]` rows, and frames `80/160/240`, with no unsupported,
  miss, missing, unresolved, or real error rows beyond expected `failed=0`
  lighting coverage summaries. The run proves both sides of the scheduler:
  `balcony_lft01` carries outlier `blend=7680.000` and falls back to
  `interval=2.060` / `blend=1.250`, while `flr_near_rt02_singer` carries sane
  `blend=600.000` and no longer re-enters rapid 2-second same-shot switches
  before the next authored camera family.
- 2026-06-24 cross-route CamShot timing scheduler sweep:
  `analysis/native_validation/cross_route_camshot_timing_schedule_20260624_current/`
  reruns seven accepted stock PS2 routes hidden with `GHOGX_DEBUG_CAMERA=1`,
  `GHOGX_LOG_CAMERA_MATRIX=1`, and `GHOGX_DEBUG_VENUE_FILTERS=1`: arena /
  `shoutatthedevil`, small1 / `psychobilly`, fest / `badreputation`, theatre /
  `yyz`, battle / `rockthistown`, big / `hangar18`, and small2 /
  `youreallygotme`. All seven exits are `0`, each retained
  `frame_00139.bmp`, and the sweep records 980 `[camera]` rows, 9,800
  `[camera-matrix]` rows, 60 regular CamShot rows, 27 `post_switch_cam` rows,
  586 timing rows, 71 lighting preset rows, 264 lighting keyframes, and 163,161
  venue AnimFilter samples. The broad health scan finds no unsupported,
  decoded-route miss, missing, unresolved, native-command, timeout, or real
  error rows after excluding expected `failed=0` coverage summaries. Retained
  frames are useful route-health evidence only: small2 and fest show coherent
  venue frames, while arena/small1/theatre/battle/big still expose the known
  camera result/framing/occlusion gap. Do not treat this sweep as final visual
  camera parity.
- 2026-06-24 CamShot authored key layout correction:
  raw stock PS2 CamShot body probes in
  `analysis/camshot_raw_layout_20260623_current/entries/` showed that the
  previous neutral-basis rejection was too broad. The key count is stored at
  `pose_offset - 20`, each keyframe body starts at `cursor + 16`, and the next
  key starts at `decoded_ref_tail_end + 16` after target refs, parent refs, and
  the `use_parent_rotation` byte. Examples: `CamShot__flr_near_rt01x23w` has
  real keys at `0x1C0` and `0x256`, with the second key carrying neutral basis
  rows, its own position `(355.27,-28.28,-273.86)`, and screen offset
  `(0.5,0.7)`; `CamShot__flr_near_rt02_singer` has real keys at `0x1C4` and
  `0x25F`; `CamShot__balcony_lft01` has real keys at `0x10C` and `0x197`.
  Native now walks this layout when the decoded candidate set matches an
  authored key count. A later identity-basis key in that layout is preserved as
  a real position/FOV/timing/screen/ref key, but inherits the previous authored
  forward/up basis instead of being dropped as scanner noise or submitted as an
  identity camera orientation. The old neutral drop remains only as a fallback
  for unlayouted scanner candidates.
  Validation:
  `analysis/native_validation/arena_camshot_layout_neutral_inherit_20260624_current/`
  reruns stock PS2 `shoutatthedevil` in arena from `16.0s` with camera,
  matrix, and venue-filter diagnostics; it exits `0`, records 140 `[camera]`
  rows, 1,400 `[camera-matrix]` rows, 40 regular CamShot rows, 8
  `post_switch_cam` rows, 122 `timing=` rows, 19 active lighting preset rows,
  30 lighting keyframe rows, and no unsupported, miss, missing, unresolved,
  native-command, timeout, or real error rows beyond expected `failed=0`
  coverage summaries. `flr_near_rt01x23w` now decodes as `poses=2`, and its
  `post_switch_cam` at `t=18.500` interpolates into position `1/2` while
  retaining the inherited authored basis. The cross-route follow-up in
  `analysis/native_validation/cross_route_camshot_layout_neutral_inherit_20260624_current/`
  reruns the same seven accepted route representatives and writes
  `summary.csv`. All seven exits are `0`, each route logs 140 camera rows and
  1,400 matrix rows, the sweep totals 207 regular CamShot rows, 33
  `post_switch_cam` rows, 605 `timing=` rows, 71 active lighting preset rows,
  264 lighting keyframe rows, and zero broad health hits after the same
  exclusions. Retained frames remain route-health evidence only: fest and
  small2 are coherent venue frames, while arena, battle, big, small1, and
  theatre still expose the shared camera result/framing/occlusion gap.
- 2026-06-24 CamShot shot-level field bridge:
  the same authored key-layout cursor exposes a stable packed shot-level tail
  in stock PS2 regular CamShots. After the last keyframe's target/parent refs
  and the four shake floats, the category symbol lands at `tail + 30`; the
  unaligned fields before it decode as `clamp_height` at `tail + 1`,
  `near_plane` at `tail + 5`, `far_plane` at `tail + 9`,
  `use_depth_of_field` at `tail + 13`, `selection_weight` at `tail + 14`,
  and `path_ease` at `tail + 18`, with `filter` immediately after the
  category string. Examples from the accepted raw-body probe include
  `flr_near_rt01x23w` category `flr_near_rt`, filter `0.5`, clamp `0`,
  near/far `(10,10000)`, selection `0.9`, path ease `-1`, and
  `balcony_lft01` category `balcony_lft`, filter `0.3`, clamp `1`,
  near/far `(50,3000)`. Native now stores these fields on every layout-verified
  CamShot pose, logs them in `camera-candidate` and `regular CamShot` rows,
  and submits authored near/far planes to the renderer. Follow-up native work
  now consumes `selection_weight` in the deterministic shared regular-camera
  selector and applies the one-target `clamp_height` branch to submitted result
  rows as `target_z + clamp_height`; `path_ease` remains decoded/logged only
  until an accepted trace maps its runtime interpolation semantics. Validation:
  `analysis/native_validation/arena_camshot_shot_fields_clip_20260624_current/`
  reruns stock PS2 `shoutatthedevil` in arena from `16.0s` with camera,
  matrix, and venue-filter diagnostics; it exits `0`, records 140 `[camera]`
  rows, 1,400 `[camera-matrix]` rows, 40 regular CamShot rows, 36 decoded
  `shot_fields=1` regular rows, 8 `post_switch_cam` rows, 19 active lighting
  presets, 30 lighting keyframes, 30,217 venue AnimFilter samples, and no
  unsupported, miss, missing, unresolved, native-command, timeout, or real
  error rows after excluding expected `failed=0` coverage summaries. The
  retained arena frame is still low and foreground-occluded, so the remaining
  camera mismatch is still upstream in the CamShot result/path/source solver,
  not plain render-matrix or near/far projection plumbing. The cross-route
  follow-up in
  `analysis/native_validation/cross_route_camshot_shot_fields_clip_20260624_current/`
  reruns the seven accepted representatives and writes `summary.csv`; all
  exits are `0`, every route logs 140 camera rows and 1,400 matrix rows, the
  sweep totals 207 regular CamShot rows, 202 decoded `shot_fields=1` regular
  rows, 33 `post_switch_cam` rows, 605 `timing=` rows, 77 active lighting
  preset rows, 260 lighting keyframes, 139,000 venue AnimFilter samples, and
  zero broad health hits. The retained `contact_sheet.png` preserves the visual
  state: fest and small2 remain coherent venue frames, while arena, battle,
  big, small1, and theatre still expose the known camera result/framing gap.
- 2026-06-24 CamShot result-builder diagnostic bridge:
  accepted PS2 static evidence in `analysis/pcsx2_trace/camera_writer_long_20260624_current.json`
  shows camera writer `0x00267008` consuming the CamShot object, a result
  buffer, a source/result object, and normalized screen-target floats derived
  from the authored screen offset as `(x + 1) * 0.5` and `(1 - y) * 0.5`
  before the later blend/copy path in `0x002665a0`. Native now keeps that
  formula explicit as `camshot_result_screen_norm_for_offset()` and emits
  `[camera-solver]` rows under `GHOGX_DEBUG_CAMERA=1` with interpolated/key
  screen targets, raw authored eye/basis, target/parent eye candidates,
  clip planes, category/filter/clamp, selection weight, path ease, and
  decoded shot-field flags. This does not move the rendered camera yet; it
  preserves the source-backed inputs needed to compare native rows against the
  accepted PS2 result-builder before implementing a visual solver. Validation:
  `analysis/native_validation/arena_camera_solver_bridge_20260624_current/`
  reruns stock PS2 `shoutatthedevil` in arena on expert from `16.0s` with
  camera, camera-matrix, and venue-filter diagnostics; it exits `0`, records
  140 `[camera]` rows, 420 `[camera-solver]` rows, 1,400 `[camera-matrix]`
  rows, 40 regular CamShot rows, 36 decoded `shot_fields=1` regular rows,
  8 `post_switch_cam` rows, 122 timing rows, 19 active lighting preset rows,
  30 lighting keyframe rows, 30,498 venue AnimFilter rows, and zero broad
  health hits. The first solver row for `flr_near_rt01x23w` preserves
  `screen_norm=(0.5,0.5)`, clip `(10,10000)`, selection `0.9`, path ease `-1`,
  category `flr_near_rt`, raw authored eye `(296.961,-90.277,-221.383)`, and
  target-composed eye `(139.195,290.920,93.010)`. The cross-route smoke in
  `analysis/native_validation/cross_route_camera_solver_bridge_20260624_current/`
  reruns the seven accepted representatives and writes `summary.csv` plus
  `contact_sheet.png`; all seven exits are `0`, every route logs 140 camera
  rows, 420 solver rows, and 1,400 matrix rows, the sweep totals 207 regular
  CamShot rows, 202 decoded shot-field rows, 33 `post_switch_cam` rows,
  605 timing rows, 77 active lighting preset rows, 260 lighting keyframes,
  142,934 venue AnimFilter rows, and zero broad health hits.
- 2026-06-24 CamShot result/source object bridge: a deliberately narrow stock
  PS2 trace for the active native camera mismatch captured
  `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_camera_result_bridge_arg_snapshots_20260624.json`
  plus `.window.png`. The live `0x00267008` input/source snapshots contain
  four-float result rows, including a generated source block at `0x014dd4a0`
  with basis rows near `(0.493,0.869,0.002)`, `(-0.824,0.468,-0.318)`,
  `(-0.277,0.155,0.948)` and position `(351.286,-95.542,150.850)`, and the
  writer output at `0x00b92ef0` copies that positive-Z result frame with basis
  row W cleared. A matching raw-source extraction in
  `analysis/native_validation/camera_result_bridge_source_20260624/` shows the
  arena `CamShot` bodies remain compact 3x3+position payloads, not authored
  four-float result rows. Native therefore keeps the raw CamShot decoder compact
  and now carries a PS2-shaped `CameraResultFrame` beside `authored_eye/at/up`:
  submitted rows come from the same source-backed parent/raw composition used
  for rendering, while the rejected target-source candidate is logged only as
  `[camera-result] stage=rejected_target_candidate`.
  Validation: `cmake --build . --target ghogx_app` and full `ctest
  --output-on-failure` pass in `engine/build`. The focused arena rerun in
  `analysis/native_validation/arena_camera_result_frame_bridge_20260624_current_clean2/`
  exits `0`, captures frames `80/120/139`, and records 140 submitted
  `[camera-result]` rows, 140 rejected target-candidate rows, 140 renderer
  `result_frame` rows, 420 solver rows, 1,540 camera-matrix rows, 40 regular
  CamShot rows, 8 `post_switch_cam` rows, 19 active lighting preset rows, 30
  lighting keyframes, 30,217 venue AnimFilter samples, and zero broad health
  hits. `frame_00139.bmp` confirms the route renders but still shows the known
  low/occluded arena camera framing. The seven-route smoke in
  `analysis/native_validation/cross_route_camera_result_frame_bridge_20260624_current/`
  exits `0` for arena, battle, big, fest, small1, small2, and theatre; every
  route logs 100 submitted result frames, 100 renderer result-frame echoes, 300
  solver rows, 1,100 camera-matrix rows, a retained `frame_00080.bmp`, and zero
  broad health hits. `summary.csv` and `contact_sheet.png` preserve the route
  evidence.
- 2026-06-24 renderer result-frame handoff: the focused same-process PS2 trace
  `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_camera_child_helpers_sampleargs_20260624.json`
  reproduces the accepted camera child-helper chain in active gameplay:
  `0x002665a0(0x00494b80,0x00b7a2d0,0x014dd4a0,0x00b92ef0)` calls
  `0x00267008` twice, `0x00261c10` resolves `a0+0xb0`, and the mutable result
  family at `0x00b92ef0` carries basis rows at `+0x20/+0x30/+0x40` plus
  translation at `+0x50`, mirrored again at `+0x60..+0x90`. Native now consumes
  `CameraResultFrame` as the renderer input when present: `OrbitCamera::eye()`
  returns the submitted result position, `draw_impl()` derives `at` from
  `position + forward * 100`, and the submitted result up row feeds
  `Mat4::look_at_lh`. Scene loads clear stale result frames. This is a shared
  PS2 render-camera handoff fix, not a shot-specific camera placement rule.
  Validation: `ghogx_gameplay_venue_band_contract_test`, `ghogx_app`, and full
  `ctest --output-on-failure` pass in `engine/build`. The focused arena run in
  `analysis/native_validation/arena_renderer_result_frame_handoff_20260624_current_rerun/`
  exits `0`, captures frames `80/120/139`, records 140 submitted result rows,
  140 renderer result-frame rows, 420 solver rows, 30,217 venue AnimFilter
  samples, and zero unsupported/missing/route-error/`NativeCommandError` rows.
  Its renderer output rows match the submitted result-frame position/forward/up
  rows with max logged delta `0.000001`. The seven-route smoke in
  `analysis/native_validation/cross_route_renderer_result_frame_handoff_20260624_current/`
  exits `0` for arena, battle, big, fest, small1, small2, and theatre; every
  route logs 100 submitted result rows, 100 renderer result-frame rows, 100
  output/result row pairs with max delta `0.000001`, a retained frame 80, and
  zero broad health hits. `contact_sheet.png` remains route-health evidence:
  several routes still show the known raw/parent-only source-framing gap.
- 2026-06-24 camera source-object trace guard: the follow-up focused PS2 trace
  `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_camera_source_a0_snapshots_20260624.json`
  captures call-time `a0` snapshots for the accepted camera chain without the
  hot transform hooks. It records 292 `0x002665a0` writer calls, 586
  `0x00267008` child result calls, 586 `0x00261c10` member lookups, and 586
  `0x00266e58` child resolves in active gameplay. The child result path
  alternates a stable base input `0x00494b80` with authored/result input
  `0x014dd4a0`; the latter carries basis rows and a positive-Z translation at
  `+0x50`. The member lookup rows are list/member/string data (`0x00494c30`
  empty in this slice, `0x014dd550` pointing at strings such as
  `hide_crowd`, `shot_started`, and `start_shot`). This guards the native
  implementation against treating `0x00261c10` as proof for a direct target
  transform source. The remaining source gap is the shared eval-object/result
  composition behind the second `0x00267008` input, not a renderer handoff or a
  one-shot target-source rule.
- 2026-06-24 regular CamShot TransAnim path bridge: the accepted
  `world_objects_ps2.dta` schema documents a shot-level `path` object of class
  `TransAnim`. Native now preserves the `.tnm` ref on regular CamShots, reuses
  the existing `load_camera_position_keys()` TransAnim camera loader, copies
  shot/runtime metadata onto the path keys, and samples those path frames
  relative to the active regular-shot start. Path-backed shots skip the older
  discrete `post_switch_cam` pose stepping; non-path multi-pose CamShots keep
  the traced post-switch cadence and authored duration/blend scheduling.
  Validation: `ghogx_gameplay_venue_band_contract_test`, `ghogx_app`, and full
  `ctest --output-on-failure` pass in `engine/build`. The clean arena run in
  `analysis/native_validation/arena_regular_camshot_path_bridge_20260624_current_clean/`
  exits `0`, captures frames `80/120/139`, loads `balcony_lft03 ->
  Camera02.tnm` with 65 keys and `balcony_lft04 -> Camera03.tnm` with 87 keys,
  selects `balcony_lft03` at `t=35.750`, and logs successive path-sampled
  camera rows from frames `1072.50`, `1077.62`, `1082.74`, etc. It records 140
  submitted result rows, 140 rejected target-candidate rows, 140 renderer
  result-frame rows, 420 solver rows, 1,540 camera-matrix rows, 42 regular
  CamShot rows, 8 `post_switch_cam` rows, 19 active lighting preset rows, 30
  lighting keyframes, 30,217 venue AnimFilter samples, and zero unsupported,
  missing, route-error, or `NativeCommandError` rows. The
  seven-route smoke in
  `analysis/native_validation/cross_route_regular_camshot_path_bridge_20260624_current/`
  exits `0` for arena, battle, big, fest, small1, small2, and theatre; every
  route logs 100 submitted result frames, 100 renderer result-frame echoes, 300
  solver rows, 1,100 camera-matrix rows, a retained `frame_00080.bmp`, and zero
  unsupported/missing/route-error rows. `summary.csv` and `contact_sheet.png`
  preserve the route evidence.

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

- Dump and map the remaining `0x0017d658` math/branch details against
  `CharEyes` / `CharLookAt` community object behavior before implementing
  further eye placement fixes. The 2026-06-28 native bridge now submits
  decoded `CharEyes.eyes` child look-at source rows through
  `runtime_world_overrides`, so self-sourced `l/r-eye.lookat` names resolve
  through the shared controller row path instead of the old synthetic-only
  fallback; this closes the missing resident/source-row runtime bridge, not
  final eyelid/lash/close-shot parity.
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
- 2026-06-23 venue material UV addressing follow-up:
  `analysis/native_validation/material_uv_big_flashpot_20260623_current/`
  adds opt-in native diagnostics for material UV ranges and RndDir proxy
  MeshAnim/MatAnim sampling. The Big/`hangar18` `venue_effect` route proves both
  flashpot proxies sample `flashpot_flash.msnm` as 4 UV vertices and logs 840
  proxy MatAnim samples. The material UV audit exposed static venue meshes with
  authored final UV ranges far outside `0..1` while the renderer still clamped
  them because material scale was `1.0`; examples include `ceiling02.mesh`
  `(-1.18..3.13, 2.00..3.00)`, `piston_glass.1.mesh`
  `(-0.98..3.30, -3.12..3.12)`, and `gear_opt_ccw.mesh` `(0.00..2.13,
  0.00..1.00)`. Native now selects wrapping when the final transformed UV range
  meaningfully crosses tile boundaries, while small edge bleed like the
  RedOctane sign remains clamped. Validation exits `0`, records 79 material UV
  rows (`wrap=19`, `clamp=60`, `uv_repeat=18` plus one animated wrap), 120 proxy
  MeshAnim samples, 840 proxy MatAnim samples, and no unsupported/error rows.
  Captured BMP frames 30 and 55 in the same folder prove the route renders;
  camera framing remains a separate known composition gap.
- 2026-06-24 material UV sampler guard/refactor:
  `MiloSceneRenderer::choose_material_uv_sampler` now owns the shared wrap-vs-
  clamp decision instead of leaving the PS2-authored UV range rule inline in the
  draw loop. The renderer still wraps only when a material transform animates,
  decoded scale exceeds the tile threshold, or the final UV bounds
  meaningfully cross the `[-0.05, 1.05]` edge-bleed window. The new
  `ghogx_render_material_uv_test` pins the Big evidence shapes: large static
  tile ranges wrap, small RedOctane-style edge bleed clamps, and MatAnim/scale
  routes still wrap. Validation:
  `analysis/native_validation/material_uv_sampler_refactor_big_flashpot_20260624_current/`
  reruns Big/`hangar18` with diagnostic `venue_effect`, exits `0`, records 79
  material UV rows (`wrap=19`, `clamp=60`, `uv_repeat=18`, one animated wrap),
  120 proxy MeshAnim samples, 840 proxy MatAnim samples, zero unsupported/miss/
  unresolved/error rows, and only `failed=0` coverage summaries. Captured
  frames 30/55 render normally; camera framing remains the known separate gap.

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

2026-06-23 bounded venue/band route-health probe:

- `analysis/native_validation/venue_band_route_health_current/` reruns stock
  PS2 `yyz` in theatre from `8.0s` with hidden fixed-step diagnostic autoplay
  and `GHOGX_DEBUG_VENUE_FILTERS=1`. The run reads the extracted PS2 `GEN`
  directly; no emulator, generated ISO, MDS, or trace staging directory is
  created. It exits `0` and records `metal_keyboard` entering `[play]` on
  `BAND KEYS`, `dw_theatre_drums` loading with four decoded drum triggers, 44
  drummer cues, eight regular camera sweeps, six `post_switch_cam` rows, six
  active lighting presets, 71 lighting keyframes, 16,050 venue AnimFilter
  samples, 419 venue ParticleSys samples, and 50 lighting MatAnim samples.
  A focused scan reports zero unsupported rows, zero decoded-route misses, zero
  unresolved/missing rows, zero nonzero failed coverage rows, and zero real
  runtime errors after excluding accepted fallback clip lookup noise. The
  cleanup guard after the run again reported zero `GH2DXu_PS2_trace_*` staging
  folders and zero temporary ISO/MDS files.

2026-06-28 current YYZ/theatre stock native route check:

- `analysis/native_validation/codex_goal_20260628_yyz_theatre_stock_v3/`
  reruns stock PS2 `yyz` in theatre from `8.0s` against the extracted v3 PS2
  `GEN` root at `C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2
  (USA)\GEN`, with hidden fixed-step diagnostic autoplay, camera/venue debug
  logging, and screenshots at frames `60`, `120`, and `179`. The run exits
  after 180 frames / 3.00s engine time and records four performers
  (`funk1`, `metal_bass`, `metal_drummer`, `metal_keyboard`), `BAND KEYS`
  `[play]`, `dw_theatre_drums`, four `kick_drum` cues, one regular camera
  sweep, one `post_switch_cam`, two active lighting presets, two lighting
  keyframes, 1,007 venue AnimFilter samples, 58 venue ParticleSys samples, and
  1,260 camera-result rows. The error scan reports zero unsupported rows,
  zero decoded-route misses, zero unresolved/missing rows, zero ARK errors,
  and no nonzero failed coverage; the only `failed=0` rows are the expected
  lighting coverage summaries. The retained screenshots show a live theatre
  stage with props, drum kit, lighting, and camera render, but composition is
  still not final signoff: the current active shot leaves foreground/empty
  floor dominant and the band too small, so camera behavior still needs native
  object-row identity or PS2-backed call-sequence evidence before parity.

2026-06-23 authored dynamic-light state cleanup:

- The authored Environ dynamic-light bridge remains opt-in behind
  `GHOGX_ENABLE_ENVIRON_DYNAMIC_LIGHTS`; the accepted PS2 traces still do not
  identify the final renderer/color writer. While auditing that gated path,
  native found a renderer state-scope mismatch: authored Environ fog was
  cleared after mesh/spotlight passes, but authored dynamic light slots were
  only cleared when the next mesh lacked an applicable Environ. Native now
  clears those authored light slots at the same pass boundaries as authored
  fog so opt-in probes cannot leak one mesh's decoded Environ lights into
  particles, spotlight instances, or overlays. This is render-state hygiene
  for the shared gated bridge, not a default lighting-color parity change.
- Validation:
  `analysis/native_validation/dynamic_light_state_cleanup_current/` runs a
  24-frame hidden opt-in smoke of stock `shoutatthedevil` in arena from
  `16.0s`, reading the extracted PS2 `GEN` directly with no generated ISO,
  MDS, emulator, or trace staging. It exits `0`, records two regular camera
  sweeps, two active lighting presets/keyframes, four drummer cues, and zero
  unsupported, decoded-route miss, unresolved/missing, nonzero failed, or real
  runtime error rows.

2026-06-23 lighting reset spotlight-state cleanup:

- Native diagnostic seek/reset was already clearing venue material overlays,
  EnvAnim state, LightAnim state, MeshAnim state, visibility overrides,
  particle state, and lighting material animation state, but it did not clear
  the active spotlight target/transition containers or push an empty active
  spotlight list back into the renderer. That could leave a stale decoded
  spotlight overlay alive until the next lighting keyframe replaced it. Native
  now clears `active_lighting_spot_targets_`,
  `lighting_transition_from_`, `lighting_transition_to_`, and the transition
  timers/active flag inside `clear_runtime_venue_animation_state()`, then calls
  `lighting_->set_active_spotlights({})` together with the existing cleared
  material/env/light renderer state. This is shared reset hygiene, not a
  venue-specific lighting-color adjustment.
- Validation:
  `analysis/native_validation/lighting_reset_spot_clear_20260623_current/`
  reruns arena, small2, battle, big, and theatre hidden from accepted stock
  PS2 `GEN` assets with diagnostic seek, autoplay, fixed step, and no
  screenshots or generated disc/staging output. All five routes exit `0` and
  log quickplay rig load, diagnostic seek, venue load, camera metadata, live
  lighting keyframes, and diagnostic autoplay hits with no `NativeCommandError`
  wrapper rows, no missing ARK rows, and no unsupported-content rows. The
  cleanup check found no `GH2DXu_PS2_trace_*` staging folders from the run.

2026-06-23 arena camera composition checkpoint:

- `analysis/native_validation/arena_camera_composition_current/` reruns stock
  PS2 `shoutatthedevil` in arena from `16.0s` with hidden fixed-step diagnostic
  autoplay and `GHOGX_DEBUG_CAMERA=1`. It exits `0` and confirms the scripted
  regular-camera route is selecting traced authored shot names and target refs
  (`flr_near_rt01x23w`, `balcony_lft01`, then forced
  `flr_near_rt02_singer`) with no selector-order or cross-shot-blend
  regression. The remaining mismatch is pose/result composition: selected
 arena eyes still land at values such as `z=-221.38` and `z=-318.44`, while
 the accepted PS2 relocated arena result rows documented above were around
 `z=60..82` in the active gameplay window. Keep this as an implementation
 gate for the shared CamShot result/path/projection bridge; do not solve it
 with shot-name clamps, arena-only offsets, or restored synthetic cross-shot
 blending.

2026-06-24 CamShot target-list result rows:

- Accepted camera traces show `0x00267008` resolving CamShot target/member
  rows, calling `0x00266e58` to average resolved target positions, and feeding
  that through `0x001b1270` before result-row math. Native now preserves the
  full decoded target-member list in `CameraKey::target_refs`, resolves
  unqualified member refs across the whole list, carries it through regular
  camera path/pose variants, exposes the resolved list and centroid in
  `[camera-solver]`, and submits a `target_list` result row that keeps the
  authored/parent camera position while facing the averaged target centroid.
  The older direct target-source candidate remains diagnostic only and still
  logs as `rejected_target_candidate`.
- Validation:
  `cmake --build . --target ghogx_gameplay_venue_band_contract_test`,
  `ctest -R ghogx_gameplay_venue_band_contract_test --output-on-failure`, and
  `cmake --build . --target ghogx_app` pass in `engine/build`. Focused native
  arena runs in
  `analysis/native_validation/arena_camshot_target_list_centroid_20260624_current/`
  and `analysis/native_validation/arena_target_list_result_rows_20260624_current/`
  both exit `0`; the latter logs `source=target_list` result frames and moves
  frame 160 from an empty crowd/lighting view into stage/performance space.
  The cross-route sweep
  `analysis/native_validation/cross_route_target_list_result_rows_20260624_current/`
  exits `0` for arena, battle, big, fest, small1, small2, and theatre, with
  every route exercising `source=target_list`. The contact sheet shows better
  stage/performance framing on battle, big, fest, and small2. Remaining camera
  mismatch is the later `0x00267008` screen/up/projection solve: arena target
  shots are still too high/pitched, small1 is still underlit/dark at frame 80,
  and theatre still lands on foreground amp/prop composition. Keep this as a
  shared result-builder gap, not a reason to add venue- or shot-name clamps.

2026-06-24 CamShot screen-offset result rows:

- Accepted PS2 `0x00267008` does more than resolve the target/member list:
  after `0x00266e58` averages the target refs and `0x001b1270` projects the
  target, it reads the CamShot screen offsets from `s2+96`/`s2+100`,
  normalizes them as `(x + 1) * 0.5` and `(1 - y) * 0.5`, computes the
  projected target delta, clamps the magnitude, and uses that in the result
  vector write around `s2+160`. Native now keeps that bridge explicit with
  `camera_apply_screen_offset_to_result_rows()`: target-list result rows keep
  the authored/path camera position, aim at the resolved target centroid, then
  apply the authored screen offset to the submitted forward row and label the
  row source as `target_list+screen` or `parent+target_list+screen`. The
  current scale uses the native 16:9 validation aspect until the exact PS2
  viewport/aspect constant in this branch is mapped.
- New path diagnostics also rule out the immediate TransAnim scanner as the
  cause of the arena Camera02 mismatch. In
  `analysis/native_validation/arena_camera_path_diagnostics_20260624_current_v3/`,
  `Camera02.tnm` selected the real count-prefixed block
  (`count_off=0x71`, `data_off=0x75`, `keys=65`) and `Camera03.tnm` selected
  its real count-prefixed block (`count_off=0x99`, `data_off=0x9D`,
  `keys=87`). The remaining camera problem is downstream result composition,
  not a false-positive path vector run.
- Validation:
  `cmake --build . --target ghogx_gameplay_venue_band_contract_test`,
  `ctest -R ghogx_gameplay_venue_band_contract_test --output-on-failure`,
  `cmake --build . --target ghogx_app`, and full
  `ctest --output-on-failure` pass in `engine/build` (22/22 tests). Focused
  native arena validation in
  `analysis/native_validation/arena_camera_screen_result_rows_20260624_current/`
  exits `0` from the accepted PS2 `GEN` assets and captures frames
  `80,88,90,96,104`. The submitted result rows now show
  `source=target_list+screen`; for example frame `1080.00` keeps position
  `(158.155,-367.817,-318.444)` and changes forward from the previous
  `(-0.324,0.627,0.708)` to `(-0.574,0.499,0.649)`, while the path-backed
  Camera02 frame `1147.50` keeps position `(-530.595,-175.411,-273.436)` and
  changes forward from `(0.860,0.177,0.478)` to `(0.874,0.135,0.466)`. This is
  stable and trace-backed, but it does not fix the high-pitched arena
  rig/ceiling composition by itself.
- Cross-route smoke in
  `analysis/native_validation/cross_route_camera_screen_result_rows_20260624_current_v2/`
  exits `0` for arena, battle, big, fest, small1, small2, and theatre from the
  accepted PS2 `GEN` assets. Each route submitted 120 camera result rows; the
  screen-corrected target-list row counts were arena `90`, battle `68`, big
  `49`, fest `52`, small1 `73`, small2 `69`, and theatre `8`. The earlier
  sibling folder without `_v2` is an invalid Start-Process quoting run: it
  split the PS2 `GEN` path and never loaded gameplay assets, so do not use it
  as evidence.
- Remaining gate: continue mapping the later `0x00267008` result-vector
  relocation/up/projection blend rather than adding per-venue clamps. Arena is
  still too high-pitched in the captured frames, and the accepted relocated
  result rows around positive z remain unmatched.

2026-06-24 regular CamShot selection-weight director:

- The decoded CamShot `selection_weight` field is now consumed by the shared
  regular-camera director after strict/mode/state filters have produced the
  eligible shot set. Positive finite authored weights are treated as relative
  deterministic buckets; missing, non-finite, zero, negative, or extreme values
  fall back to `1.0`, preserving the old modulo order when all eligible shots
  have no authored weight. This is route-shape selection for decoded CamShot
  metadata, not a shot-name override or venue-specific clamp.
- Validation:
  `cmake --build . --target ghogx_gameplay_venue_band_contract_test`,
  `ctest -R ghogx_gameplay_venue_band_contract_test --output-on-failure`,
  `cmake --build . --target ghogx_app`, and full
  `ctest --output-on-failure` pass in `engine/build` (22/22 tests). Focused
  arena validation in
  `analysis/native_validation/arena_camera_selection_weight_20260624_current/`
  exits `0` from accepted PS2 `GEN` assets with retained frames
  `80,88,90,96,104`. The first regular sweeps now route through
  `flr_near_rt01x23w`, `balcony_lft04`, back to `flr_near_rt01x23w`, then
  `flr_near_lft01x12w` and `flr_far_rt03`, proving the authored weights change
  native director order. Submitted camera rows remain active for the full run
  and include `source=target_list+screen` where the selected shot carries
  target/screen metadata.
- Corrected cross-route validation in
  `analysis/native_validation/cross_route_camera_selection_weight_20260624_current/`
  uses direct `& $app @appArgs` invocation so the PS2 `GEN` path is not split.
  Arena, battle, big, fest, small1, small2, and theatre all exit `0` and each
  submit 120 camera result rows. Target-list row counts after the selector
  change are arena `101`, battle `48`, big `80`, fest `41`, small1 `14`,
  small2 `52`, and theatre `120`; screen-corrected rows are arena `77`,
  battle `40`, big `80`, fest `0`, small1 `14`, small2 `52`, and theatre
  `120`. The only broad `error/failed` scan hits are PowerShell's redirected
  native stderr wrapper and existing lighting coverage rows with `failed=0`.
- Remaining gate is unchanged: weighted selection is now shared and documented,
  but the retained arena frames still show high/pitched rig and ceiling views.
  Continue mapping the accepted `0x00267008` evaluated source-object/result
  composition path for `0x00494b80` and `0x014dd4a0`; do not paper over this
  with per-shot camera offsets or venue-specific visual hacks.

2026-06-24 CamShot source-seed diagnostic rows:

- Accepted `0x00267008` first seeds a child result frame from the source object
  rows before target-list/screen correction. Native now logs the equivalent
  compact CamShot seed under `GHOGX_DEBUG_CAMERA=1` as
  `[camera-result] stage=source_seed_candidate`, alongside the submitted
  result row and the rejected target-source diagnostic. This is a comparison
  surface only: the rendered camera still uses the submitted result frame, and
  no camera placement is changed by this diagnostic.
- Validation:
  `cmake --build . --target ghogx_gameplay_venue_band_contract_test`,
  `ctest -R ghogx_gameplay_venue_band_contract_test --output-on-failure`,
  `cmake --build . --target ghogx_app`, and full
  `ctest --output-on-failure` pass in `engine/build` (22/22 tests). The short
  arena diagnostic run in
  `analysis/native_validation/arena_camera_source_seed_diagnostics_20260624_current/`
  exits `0` from accepted PS2 `GEN` assets and logs 45 submitted rows, 45
  source-seed rows, and 45 rejected target-source rows. Its first
  `flr_near_rt01x23w` frame shows native `source_seed_candidate` at
  `position=(296.961,-90.277,-221.383)` with
  `forward=(0.878,0.479,0.000)`, submitted `target_list` at the same seed
  position but target-facing `forward=(-0.771,0.090,0.630)`, and rejected
  target-source at `position=(139.195,290.920,93.010)`. That rules out a log
  handoff issue and keeps the remaining gate pinned to constructing the
  accepted generated source object such as `0x014dd4a0`
  `position=(351.286,-95.542,150.850)` before the result-builder solve.

2026-06-24 camera apply source-object static map:

- Static dump
  `analysis/pcsx2_trace/camera_apply_source_object_static_20260624_current.json`
  was generated from the accepted PS2 `SLUS_214.47` with
  `tools/dump_function_snippets.py` for `0x00262b08`, `0x00263410`,
  `0x0026ae00`, `0x0026c900`, `0x002665a0`, and `0x00267008`. This was a
  local static dump only; no new PCSX2 gameplay trace was opened.
- The missing generated source object is upstream of `0x002665a0`.
  `0x00262b08` calls `0x00263410` to choose the source/current nodes, then
  passes them into `0x002665a0`. The path/apply helper `0x0026ae00` is the
  concrete row writer: after building rows on stack, it loads `a1=156(s6)`,
  copies basis/result rows from stack offsets `32/48/64/80` into object
  offsets `+32/+48/+64/+80`, and calls `0x001dd748` to dirty/update that
  object. This matches the accepted generated source object later observed as
  `0x014dd4a0` in the result-writer traces.
- Next implementation gate: model the shared `0x0026ae00` source-object apply
  path from decoded TransAnim/current-source state, then feed that generated
  source seed into the existing `0x00267008` result-row bridge. Do not move the
  rendered camera from raw compact CamShot rows, target-source rows, or
  shot-name clamps; the accepted source is a generated row object written
  before the result-builder call.

2026-06-24 generated source-row bridge:

- Native `CameraKey` now has a PS2-shaped generated source-row slot
  (`position/forward/up`) for the `0x0026ae00` output object. The shared
  `camera_source_seed_result_rows_for_key()` path prefers those rows when
  populated, otherwise it preserves the previous compact/parent source fallback.
  `camera_target_list_result_rows_for_key()` now starts from that shared seed and
  appends the target-list solve, matching the accepted order where the generated
  source object is built before the `0x00267008` result-builder call. This is
  plumbing for the traced source object, not a per-shot placement override.
- Validation: `cmake --build . --target ghogx_gameplay_venue_band_contract_test`,
  `ctest -R ghogx_gameplay_venue_band_contract_test --output-on-failure`,
  `cmake --build . --target ghogx_app`, and full `ctest --output-on-failure`
  pass in `engine/build` (22/22 tests). The focused arena run in
  `analysis/native_validation/arena_camera_generated_source_bridge_20260624_current/`
 exits `0`, captures frames `80/88/90/96/104`, logs 125 source-seed rows and
  125 submitted rows, and submitted rows now report
  `source=source_seed+target_list` (or the same source plus `+screen` during
  blends). Generated source rows remain `0` in this run, so the visible camera is
  still the known raw-source arena framing while the bridge is ready for the
  dynamic `0x0026ae00` population.

2026-06-24 path-backed generated source rows:

- Native parentless path-backed regular CamShots now populate the shared
  generated source-row slot from decoded TransAnim position and rotation keys.
  This turns traced path sweeps such as arena `balcony_lft04` from anonymous
  `source_seed` diagnostics into `generated_source_seed` rows while preserving
  the existing target-list and screen-offset result-builder order.
- Validation: `cmake --build . --target ghogx_app
  ghogx_gameplay_venue_band_contract_test` passes, the focused contract test
  passes, and full `ctest --output-on-failure` remains 22/22. The focused
  native run in
  `analysis/native_validation/arena_camera_path_generated_source_20260624_current/`
  exits `0`, captures `frames/frame_00080.bmp`, and logs 20
  `stage=source_seed_candidate source=generated_source_seed` rows. At frame
  `615.00`, the submitted row is
  `source=generated_source_seed+target_list+screen`, confirming the path source
  object feeds the existing `0x00267008`-shaped solve instead of replacing it.
- Visual verdict: the captured frame is still a high rig/ceiling composition,
  so this is not the final camera solve. Continue mapping the relocation/result
  branch between the transient `0x0026ae00` path-frame object and the accepted
  evaluated source/result rows; do not introduce per-shot offsets or camera
  clamps to hide the remaining mismatch.

2026-06-24 camera path/apply follow-pointer trace:

- Targeted PCSX2 trace
  `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_camera_path_apply_follow_20260624_current.json`
  was reopened only for the native camera-source mismatch. It captured 617
  total calls: 309 to `0x0026ae00` (`cam_path_apply_0026ae00`) and 308 to
  `0x002665a0` (`cam_result_writer_002665a0`).
- The first `0x0026ae00` sample follows `a0+0x9c` to `0x00b8ead0`, a 288-byte
  path-frame/source object. Its first basis/result block stores
  forward `(0.493816,0.868795,0.001889)`, right
  `(-0.823474,0.468753,-0.317519)`, up
  `(-0.276947,0.155313,0.947608)`, and position
  `(94.156349,-53.597404,83.805237)`, then duplicates the block at
  `+0x60..+0x90`. The same sample follows `a0+0x60` to `0x007ce440` and
  `a0+0xa8` to `0x007d25b0`, which remain target/list shaped data rather than
  the generated source rows.
- Compare with the earlier accepted
  `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_camera_source_a0_snapshots_20260624.json`:
  `0x00267008` sees source input `0x014dd4a0` with basis rows
  `(0.493401,0.869205,0.001779)`,
  `(-0.824085,0.468158,-0.317789)`,
  `(-0.277225,0.155328,0.947844)`, and position
  `(351.286255,-95.542320,150.850449)`. Its output object `0x00b92ef0`
  carries the `0x0026ae00`-style basis
  `(0.493816,0.868795,0.001889)` but the relocated position
  `(351.286255,-95.542320,150.850433)`. Do not conflate the transient
  `0x00b8ead0` path-frame output with the final evaluated source input
  `0x014dd4a0`; there is another relocation/source-selection step between
  them.
- Asset inspection of Battle `flr_far_rt03` shows the CamShot body has one
  blank target ref and a blank parent ref. Its raw pose eye
  `(269.407,-137.378,90.207)` plus the live
  `guitarist0:spot_neck_fret20.mesh` position
  `(81.879,41.836,60.643)` matches the accepted source input position
  `(351.286,-95.542,150.850)` within trace/log precision. Native now prunes
  all-empty target refs from the member list and treats a blank CamShot target
  slot with no authored parent subpart as a default source-parent fallback to
  `spot_neck_fret20.mesh`, with the performer entity inferred from the shot
  hint or `guitarist0`. This is trace-backed shared CamShot plumbing, not a
  shot-name visual hack.
- The fallback stays translation-only unless the asset explicitly carries the
  `use_parent_rotation` byte. A test run that forced parent rotation for the
  blank fallback made the arena frame nearly blank/dark, so the native code was
  restored to authored rotation only while the `0x014dd4a0` relocation path is
  still unmapped.
- Validation: `cmake --build . --target ghogx_app
  ghogx_gameplay_venue_band_contract_test` passes in `engine/build` with only
  the existing warnings; `ctest -R ghogx_gameplay_venue_band_contract_test
  --output-on-failure` passes. Focused native arena validation in
  `analysis/native_validation/arena_camera_default_source_parent_final_ps2ark_20260624/`
  exits `0` from the accepted PS2 `GEN` assets, captures frames
  `80,88,90,96,104`, logs 108 submitted rows, 108 source-seed rows, 9
  `parent+source_seed` rows, and 46 CamShots with
  `parent=guitarist0:spot_neck_fret20.mesh`. The only broad
  `error/failed` scan hits are PowerShell's redirected native stderr wrapper
  and lighting coverage summaries with `failed=0`.
- Visual verdict: the translation-only default is stable and avoids the
  forced-rotation blank frame, but it is not the final camera solve. The retained
  arena frames still include high rig/ceiling composition, especially
  `frame_00104.bmp`. Continue mapping the source-object relocation between
  `0x00b8ead0` and `0x014dd4a0` and the later `0x00267008` result-vector solve;
  do not add per-venue clamps or one-off shot offsets.
- Rejected follow-up: a native-only `target_behind` guard was tested in
  `analysis/native_validation/battle_camera_target_behind_guard_20260624/`
  after Battle frame `80` showed the current `target_list+screen` solve aiming
  at a target behind the authored source vector. The guard switched that frame
  to `source_seed+target_behind`, but the screenshot still framed sign/sky
  props instead of a playable stage view, and it changed many rows without an
  accepted PS2 branch trace. It was reverted. The next fix should map the real
  `0x00267008` projected-target/vector blend, especially the `s3+52` filter and
  clamped screen-delta path, rather than dropping target-list solves wholesale.

2026-06-24 camera pose-span source-basis gate:

- The accepted `pcsx2_camera_result_bridge_arg_snapshots_20260624.json`
  `0x002665a0` sample shows two source rows for the same Battle solve:
  `a0=0x014dd390` at position `(388.938538,-117.070160,165.531784)` and
  `a2=0x014dd4a0` at `(351.286255,-95.542320,150.850449)`. The normalized
  relocated pose delta from the second row toward the first is
  `(-0.8223,0.4701,-0.3206)`, matching the traced right row
  `(-0.824085,0.468158,-0.317789)`. The forward row is the normalized
  `right x world_up`, and the up row is `forward x right`. This is accepted as
  the source-basis derivation for targetless CamShots whose authored pose span
  is meaningfully horizontal, such as `flr_far_rt03`.
- Native now carries a shared, gated `pose_span_basis` helper for decoded
  CamShots with no target refs, no path animation, no authored parent rotation,
  at least two poses, and a horizontal span. It is intentionally not a
  shot-name patch: it derives the basis from authored pose positions after the
  same source-parent relocation used for the source eye.
- Rejected experiment:
  `analysis/native_validation/battle_camera_generated_parent_basis_20260624_current/`
  tried transforming generated source forward/up by the parent basis. Frames
  still showed ceiling/sign/prop compositions or blank dark output, so the
  generated-source parent-basis change was reverted.
- Rejected experiment:
  `analysis/native_validation/battle_camera_pose_span_source_basis_20260624_current/`
  applied the pose-span basis without enough shape gating. It fired on
  `balcony_rt01` with a near-vertical span and produced
  `source=parent+source_seed+pose_span_basis` rows with a bad stage/prop frame.
  The native helper now skips spans whose horizontal length is not significant,
  pending a specific PS2 trace for that no-target branch.
- Guarded validation:
  `analysis/native_validation/battle_camera_pose_span_source_basis_guarded_20260624_current/`
  exits `0` and captures frames `80/88/90/96/104`. The captured slice contains
  no `pose_span_basis` rows: `balcony_rt01` correctly remains
  `source=parent+source_seed`, and earlier `flr_far_lft03`/`flr_far_lft02`
  targetless rows have identical first/next pose positions. The remaining
  Battle visual gap is therefore still the targetless/no-target
  `0x002665a0` / `0x00267008` branch or camera selection, not a reason to add a
  native-only camera offset.
- Follow-up PCSX2 trace:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_camera_balcony_branch_20260624_current.json`
  is accepted only as active-window confirmation that the already documented
  horizontal source-object family still appears in Battle. It did not reach the
  native `balcony_rt01` mismatch.
- Rejected follow-up:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/pcsx2_camera_balcony_branch_long_20260624_current.json`
  retained a different no-target row family, but its `.window.png` is the
  `SONG FAILED` / Retry screen at 14% completion. Do not use the late sampled
  rows from that file as active-song camera evidence. Native now logs
  `[camera-solver] pose_span_shape=...` for targetless/no-target keys under
  `GHOGX_DEBUG_CAMERA=1`, so the next specific mismatch trace can compare the
  no-target source shape without changing rendered camera behavior.
- Native diagnostic validation:
  `analysis/native_validation/battle_camera_pose_span_debug_20260624_current/`
  exits `0`, captures frames `80/88/90/96/104`, logs 68
  `pose_span_shape` rows, 18 `reason=near_vertical` rows, and zero
  `pose_span_basis` rows. At the mismatch window, frame `1147.50` keeps
  `balcony_rt01` on `source=parent+source_seed` with parent
  `guitarist0:spot_neck_fret20.mesh`; its relocated pose span is
  `(0.657,-1.268,-8.062)`, length `8.187135`, horizontal length `1.427781`,
  and horizontal ratio `0.174393`, which documents why the trace-backed
  horizontal helper remains skipped for this no-target branch.

2026-06-24 camera shot_filter target state:
- Native now carries the traced PS2 result-builder target state from
  `0x00267008` / `s3+52` through `CameraResultBuilderState`. The branch projects
  the current target through the source rows, compares it to the authored
  screen offset in normalized screen space, scales `shot_filter` by
  `min(projected_delta, 1)`, and blends the carried target toward the current
  target before rebuilding the submitted `target_list` result rows. This state
  is shared by regular and intro venue cameras and resets on song load and
  diagnostic seek.
- Validation
  `analysis/native_validation/arena_camera_shot_filter_state_20260624_current/`
  exposed one native bug in the first pass: shots with no positive
  `shot_filter` label were not writing the carried target back to the current
  target. Frame `615.00` on `balcony_lft04` had `target=(2.424,-65.534,22.882)`
  but a stale `filtered_target=(-2.184,-50.752,25.731)`.
- Validation
  `analysis/native_validation/arena_camera_shot_filter_state_20260624_current_v2/`
  fixes that writeback while keeping provenance honest: `balcony_lft04` logs
  `shot_filter_branch=0`, `filter_step=1.000000`, and
  `filtered_target=(2.424,-65.534,22.882)` without adding a `+shot_filter`
  source label. The app exits `0`, the error scan reports `failed=0`, and the
  full native test suite passes `22/22`.
- Visual verdict: frame
  `analysis/native_validation/arena_camera_shot_filter_state_20260624_current_v2/frames/frame_00080.bmp`
  is still a high rig/ceiling composition. The remaining arena mismatch is not
  solved by source-row copy, shot-field propagation, or no-filter state
  writeback. The next evidence gate is either the exact `0x00267008` orientation
  / projection tail for submitted rows or an arena-specific PCSX2 trace for
  `shoutatthedevil`, venue `arena`, selected shot `balcony_lft04`, path
  `Camera03.tnm`.

2026-06-24 arena result-builder argument trace follow-up:
- Specific mismatch gate: native `arena/shoutatthedevil` at frame `615.00`
  selects `balcony_lft04` / `Camera03.tnm` and still renders a high
  rig/ceiling frame after path-backed generated source rows and shot-filter
  state. That justified reopening PCSX2 for camera evidence.
- The explicit GH2DXu direct-route statefile still works with the stock ISO path
  for short statefile traces:
  `C:/Games/Emulators/PCSX2/sstates/GHDX-00300 (A9BBA52A).01.p2s`,
  ELF
  `GuitarHeroOGX-trace360/analysis/ps2_trace/external/Guitar-Hero-II-Deluxe-Unified/out/ps2/GHDX_003.00`.
  Smoke report
  `GuitarHeroOGX-trace360/analysis/ps2_trace/gh2dxu_arena_statefile_stockiso_camera_smoke_20260624.json`
  records 14 `0x002665a0` calls and an active arena gameplay window.
- Focused trace
  `GuitarHeroOGX-trace360/analysis/ps2_trace/gh2dxu_arena_camera_result_builder_args_20260624.json`
  records 954 camera calls over the active arena window:
  `0x002665a0` writer 17, `0x00266f80` child-output 323, and
  `0x00267008` result-builder 614. The late result-builder family includes
  source/result rows at `0x00cc0590` with basis
  `forward=(0.878,0.478,0.001)`, `right=(-0.471,0.865,-0.173)`,
  `up=(-0.083,0.152,0.984)`, `position=(259.235,-229.846,97.250)`, and
  output/projection rows at `0x00cc0720` carrying the same position plus
  repeated `768.0` lanes and frustum-like rows. Treat this as
  result-builder row-shape evidence only: the trace window is active arena, but
  it is not correlated to native `balcony_lft04` by selected shot name.
- Rejected native experiment:
  `analysis/native_validation/arena_camera_generated_source_basis_20260624_current/`
  temporarily preserved generated source-object basis through target-list rows.
  It exited `0` and logged
  `source=generated_source_seed+target_list+source_basis+screen`, but retained
  `frames/frame_00080.bmp` is still a rig/ceiling composition. That runtime
  branch was removed. The remaining native gap is still the exact
  source/path/result composition for the active shot, not a source-basis
  shortcut.

2026-06-24 arena path candidate diagnostics:
- Native now samples path-backed TransAnim camera rotations with quaternion
  interpolation instead of holding the previous rotation key until the next
  keyframe. This is shared path sampling plumbing and does not alter the first
  `Camera03.tnm` frame, where the first rotation key is already exact.
- A speculative path-entry cross-shot sweep was tested and removed. It delayed
  `balcony_lft04` behind the previous `flr_near_rt01x23w` pose, but the retained
  arena frames still showed rig/ceiling or crowd-overview compositions and the
  earlier camera checkpoint explicitly rejects synthetic cross-shot blending as
  a visual fix.
- Native diagnostics now retain the owning CamShot body pose beside path keys
  and log `path_base_pose_candidate` and `path_base_translate_candidate` rows
  under `GHOGX_DEBUG_CAMERA=1` without submitting them to the renderer. In
  `analysis/native_validation/arena_camera_target_alias_candidates_20260624_current/`,
  frame `615.00` keeps the submitted source at
  `(-494.399,-596.318,-0.755)`, while the body-pose candidate moves to
  `(-894.364,-1428.834,-120.712)` and the translation-only body candidate to
  `(-671.213,-1743.641,-198.276)`. Both are farther from the accepted
  source-object family, so path body-pose composition is ruled out for this
  mismatch.
- The same diagnostic run logs entity-only target alias rows for
  `guitarist0:` path shots (`spot_neck_fret20.mesh`, `bone_spine1.mesh`,
  `bone_spine2.mesh`, `bone_neck.mesh`). At frame `615.00`, those rows only
  nudge the submitted forward vector and leave the source position unchanged,
  so the arena path gap is not primarily an entity-only target alias problem.
- Validation: `cmake --build . --target ghogx_gameplay_venue_band_contract_test
  ghogx_app` and
  `ctest -R ghogx_gameplay_venue_band_contract_test --output-on-failure` pass
  in `engine/build`. The focused native run above exits `0` from accepted PS2
  `GEN` assets and captures `frames/frame_00080.bmp` and
  `frames/frame_00088.bmp`; the visuals remain the known high rig/crowd
  compositions. Continue mapping the real `0x0026ae00` delta/source-selection
  step into the generated source object before changing submitted camera
  behavior.

2026-06-24 arena path source follow trace:
- A targeted PCSX2 trace was reopened for the specific native source-object
  mismatch, this time sampling `0x0026ae00` `a0` rows and following the
  `+0x60`, `+0x9c`, and `+0xa8` pointers. Output:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/gh2dxu_arena_path_source_follow_20260624.json`.
  It records 688 calls: 23 `cam_path_apply_0026ae00`, 15
  `cam_result_writer_002665a0`, and 650 `cam_result_builder_00267008`.
- The unique sampled path-apply object `0x00cbc010` follows `+0x9c` to
  `0x00cbc110`, whose first source block stores basis
  `forward=(0.878,0.478,0.001)`, `right=(-0.471,0.865,-0.173)`,
  `up=(-0.083,0.152,0.984)`, and position `(34.983,-64.273,60.353)`,
  with a second transposed/projection-like block beginning at `+0xc0`.
  `0x00267008` later samples source/result rows at `0x00cc0590` with the same
  basis family and position `(259.266,-229.857,97.065)`.
- Treat this as structural source-object evidence only, not a selected-shot
  oracle: the statefile trace is active arena gameplay but is still not
  correlated to native `balcony_lft04` / `Camera03.tnm` by shot name or song
  timestamp. The important negative result is that PS2 path apply is already
  writing positive-Z source objects before `0x00267008`, whereas native
  `balcony_lft04` frame `615.00` still submits the raw path position
 `(-494.399,-596.318,-0.755)`. The next trace should either expose selected
  shot identity at the writer/result-builder call or sample the runtime shot
  object source pointers for the exact active `Camera03.tnm` window.

2026-06-24 arena venue source-parent diagnostics:
- Native now keeps `venue_camera_target_worlds_` as a diagnostic-only map built
  from decoded venue geometry before `venue_scene` moves into
  `MiloSceneRenderer`. The map is passed to `apply_camera_keys` separately
  from live performer `camera_targets`, so these candidates cannot affect the
  submitted camera result. Under `GHOGX_DEBUG_CAMERA=1`, native logs
  `path_source_parent_source_ref_candidate`,
  `path_source_parent_crowd_group_candidate`, and
  `venue_source_parent_refs`.
- This was added for the focused PS2 path-apply evidence where
  `0x0026ae00`'s sampled object points at the string `crowd` near the source
  rows. Arena geometry has no exact decoded `crowd` Trans/Mesh object; it has
  crowd groups/meshes such as `crowd_distant.grp`, so native exposes both an
  exact-name candidate when present and an aggregate `crowd_group_centroid`
  candidate when only crowd groups exist.
- Validation:
  `analysis/native_validation/arena_camera_venue_crowd_parent_candidates_20260624_current_v3/`
  exits `0` from stock PS2 `GEN` assets, captures frames `80/88`, and logs
  `venue camera targets: 656 crowd_group_centroid=1
  pos=(-13.003,-920.449,-366.562)`. At frame `615.00`,
  `balcony_lft04` still submits
  `source=generated_source_seed+target_list+screen` at
  `(-494.399,-596.318,-0.755)`, while the group-parent diagnostic moves to
  `(-507.403,-1516.767,-367.317)`. That is farther from the accepted PS2
  positive-Z source-object families `(34.983,-64.273,60.353)` and
  `(259.266,-229.857,97.065)`.
- Result: the geometry-side crowd group centroid is ruled out as the missing
  arena path source-parent composition. The PS2 `crowd` pointer in the sampled
  path object is likely a semantic/runtime source ref, not the venue geometry
  group transform. Continue with a trace that correlates selected shot identity
  or source-object pointer state for the exact active `Camera03.tnm` window
  before changing submitted camera behavior.
- Verification: `cmake --build . --target
  ghogx_gameplay_venue_band_contract_test ghogx_app`,
  `ctest -R ghogx_gameplay_venue_band_contract_test --output-on-failure`, and
  full `ctest --output-on-failure` pass in `engine/build`. The retained frame
  `frame_00080.bmp` remains the known high rig/ceiling composition, confirming
  this pass added diagnostics without moving the rendered camera.

2026-06-24 arena path source identity trace follow-up:
- Two follow-up PCSX2 traces tried to correlate the accepted path/source rows
  to selected-shot identity without changing native camera behavior:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/gh2dxu_arena_builder_a0_shot_identity_20260624.json`
  and
  `GuitarHeroOGX-trace360/analysis/ps2_trace/gh2dxu_arena_builder_a0_shot_identity_long_20260624.json`.
  Both screenshots are active arena gameplay and both logs are clean, but
  neither trace contains `balcony_lft04` or `Camera03`.
- The long trace retains 512 `0x0026ae00` path-apply calls, 512
  `0x002665a0` writer calls, and 1024 `0x00267008` builder calls. The call
  order is stable: path apply, builder source object `0x0077c610`, builder
  source object `0x008269f0`, then writer. The shot-side follow row at
  `0x0077c610+0x84` points to a CamShot object/list family that includes
  `balcony_lft01`, `balcony_lft01 zoom`, `flr_near_lft04.shot`, and category
  strings, but not the exact native mismatch shot.
- The path source frame remains strong structural evidence: `0x00cbc010+0x9c`
  points to `0x00cbc110`, whose rows carry
  `forward=(0.878,0.478,0.001)`, `right=(-0.471,0.866,-0.167)`,
  `up=(-0.080,0.146,0.985)`, and
  `position=(34.777,-63.896,59.800)`. The builder input rows at `0x00cc0590`
  carry the same basis family at `position=(258.967,-229.590,99.475)`, and
  the writer rows at `0x014fd344` end near
  `position=(258.578,-228.578,105.312)`.
- Verdict: this proves the PS2 path branch writes a semantic/runtime source
  frame before the result builder, but it is still not selected-shot evidence
  for native `balcony_lft04` / `Camera03.tnm`. Do not submit native camera
  changes from these traces alone. The next PCSX2 route needs either a
  force-shot/current-shot hook that lands on `balcony_lft04` or per-call
  selected-shot name sampling tied directly to the `Camera03.tnm` path object.

2026-06-27 arena selected-shot trace preflight:
- Rechecked the retained arena camera trace artifacts before reopening PCSX2.
  The long trace has stable per-frame call order:
  `cam_path_apply_0026ae00`, `cam_result_builder_00267008` for source
  `0x0077c610`, `cam_result_builder_00267008` for source `0x008269f0`, then
  `cam_result_writer_002665a0`. Its raw counts are 512 path applies, 1024
  result-builder calls, and 512 writer calls.
- The retained `a0+0x84` follow row from builder source `0x0077c610` reaches
  shot-list/category metadata, including `balcony_lft`,
  `balcony_lft01 zoom`, `flr_near_lft`, and `flr_near_lft04.shot`.
  The same sampled row also reaches the active path source object
  `0x00cbc010`, whose `+0x9c` source-frame rows remain the accepted
  positive-Z semantic/runtime source-frame evidence.
- Negative result: the retained JSON still has no literal `balcony_lft04`,
  `Camera03`, or `camera03` string. It therefore cannot be promoted from
  structural source-frame evidence to accepted selected-shot evidence for the
  native `shoutatthedevil` / `arena` frame `615.00` mismatch.
- The next trace should keep the same three function targets but must capture
  one of these exact acceptance conditions: (1) a current/forced-shot cell or
  active CamShot pointer that resolves to `balcony_lft04`; (2) the selected
  shot object's path member resolving to `Camera03.tnm`; or (3) a filtered
  `0x00267008` / `0x002665a0` call where the source object can be tied to both
  the active shot name and the `Camera03.tnm` path object. Without one of
  those ties, do not move native submitted camera behavior for this shot.

2026-06-27 arena selected-shot statefile route check:
- Added explicit `--statefile` support to the repeatable PCSX2 string/symbol
  samplers (`scan_live_ee_strings.py`, `sample_pcsx2_shot_graph.py`, and
  `sample_pcsx2_world_symbols.py`) so the accepted GH2DXu arena statefile route
  can be checked without one-off inline launch code.
- New GH2DXu route artifacts:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/gh2dxu_arena_live_string_scan_tool_20260627.json`
  and
  `GuitarHeroOGX-trace360/analysis/ps2_trace/gh2dxu_arena_shot_graph_statefile_20260627.json`.
  The string scan confirms the state has `shoutatthedevil`, `arena`,
  `balcony_lft`, and `flr_near_lft04.shot` loaded, but still has zero live EE
  hits for `balcony_lft04`, `Camera03.tnm`, or `Camera03`.
- The shot-graph sampler finds the better runtime camera-script cluster:
  `current_shot` at `0x0060cce8` resolves through value cell `0x0060ccf0` to
  root `0x0060ca30`; `next_shot`, `shot_started`, `post_switch_cam`, and
  `do_force_shot` resolve around `0x006150xx` / `0x00615950`. The walked graph
  exposes categories, `world/camshot.dtb`, `Shot Name:`, `crowd`, and
  `start_shot`, but not an active shot object/name or a `Camera03.tnm` path
  pointer.
- Stock SLUS statefile check
  `GuitarHeroOGX-trace360/analysis/ps2_trace/stock_arena_live_string_scan_20260627.json`
  has the same important shape: `balcony_lft` and `flr_near_lft04.shot` are
  present, while `balcony_lft04`, `Camera03.tnm`, and `Camera03` are absent.
  Therefore the next evidence route should not depend on those literal native
  labels appearing in PS2 RAM. It needs to bind the active source/path object by
  pointer/row shape, category/path slot, or a deliberate force-shot route rather
  than by the native decoded display name alone.

2026-06-28 arena shot-table/source-object route:
- New focused GH2DXu arena statefile artifacts:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/gh2dxu_arena_shot_table_rows_20260628.json`
  and
  `GuitarHeroOGX-trace360/analysis/ps2_trace/gh2dxu_arena_shot_object_neighborhood_20260628.json`.
  Both sampled 16 times across the live state. The shot table rows were stable;
  the source-object neighborhood only changed in timer/list bookkeeping fields,
  not in the name/object links.
- The live string/object table pairs `flr_near_lft04.shot` at string
  `0x00ca6c81` through table refs `0x00ca6a10` and `0x00caa6b4` to object
  `0x00caa6a0`. The same table has `balcony_lft01 zoom` at string
  `0x00ca6c6e`, refs `0x00ca69f0` and `0x00caa554`, object `0x00caa540`,
  and the shared/category object `0x00caa3e0`.
- The source-object neighborhood around the retained result-builder source
  shows the same list family in row-shaped records: `0x0077c694 -> 0x00caa3e0`,
  `0x0077c6b4 -> 0x00caa540`, and `0x0077c6d4 -> 0x00caa6a0`, with target
  member hints such as `bone_spine1.mesh` and `spot_neck_fret20.mesh` in the
  adjacent rows.
- Native validation still decodes the arena MILO as
  `balcony_lft04 -> Camera03.tnm` with 87 TransAnim keys, while the live PS2
  RAM scans have zero hits for literal `balcony_lft04`, `Camera03.tnm`, or
  `Camera03`. Therefore the next acceptance hook should bind the active PS2
  camera through the runtime row/object identity (`0x0077c610`/`0x0077c694`
  list family and concrete object slots) or a deliberate force-shot route.
  Do not reinterpret the native `balcony_lft04` path branch as accepted just
  from the decoded native label.
- Rejected trace attempts from the same date are not acceptance evidence:
  `gh2dxu_arena_builder_source_0077c610_filtered_20260628.json` recorded zero
  filtered calls, `gh2dxu_arena_builder_unfiltered_sanity_20260628.json`
  recorded only one cold `0x00267008` call with `a0=0`, and
  `gh2dxu_arena_camera_chain_patch_before_20260628.json` recorded zero camera
  chain calls. The visible sanity attempt failed while writing the live hook at
  `0x00267008`, so it produced only a log. Keep using the stable object/table
  sampler evidence above until a call-sequence route is screenshot-backed and
  reaches active camera-chain calls.

2026-06-28 arena camera acceptance guardrail:
- Native contract coverage now explicitly forbids submitting the diagnostic
  `path_source_parent_*` venue rows through
  `camera_submitted_result_rows_for_key`, and also forbids keying submitted
  camera behavior from the native `balcony_lft04` / `Camera03` labels. The
  diagnostics still log after `apply_camera_result_frame`, so they remain
  comparable against PS2 traces without influencing the rendered camera.
- This preserves the accepted evidence boundary above: the next actual camera
  behavior change still needs runtime row/object identity
  (`0x0077c610` / `0x0077c694` family), a deliberate force-shot route, or a
  screenshot-backed call-sequence trace that ties the source rows to the active
  PS2 camera.

2026-06-28 diagnostic regular-camera force-shot hook:
- Native now exposes `--diagnostic-camera-shot <shot>` for bounded validation
  captures. The hook pins `active_regular_camera_` to a decoded regular
  `CamShot` by name and logs `[world] diagnostic camera shot selected`, but it
  does not alter `camera_submitted_result_rows_for_key` and is contract-guarded
  as a diagnostic-only route. This gives native validation a repeatable way to
  capture exact decoded shots such as the current arena/theatre mismatch
  windows without adding shot-name-specific camera behavior.
- Validation:
  `analysis/native_validation/arena_diagnostic_camera_shot_balcony_lft04_20260628_current/`
  runs stock PS2 `shoutatthedevil` from `16.0s` with
  `--diagnostic-camera-shot balcony_lft04`, fixed-step autoplay, camera/matrix
  diagnostics, and screenshots at frames `80`, `120`, and `139`. It exits `0`;
  the log has one forced regular sweep to `balcony_lft04`, one diagnostic
  selection row, zero missing-shot rows, 140/140 `[camera]` rows on
  `balcony_lft04`, 140 submitted result rows, 140
  `path_source_parent_crowd_group_candidate` diagnostic rows, 1,001 venue
  AnimFilter samples, one active lighting preset, and no unsupported, miss,
  no-decoded, ARK error, or runtime error rows beyond expected `failed=0`
  lighting coverage summaries. The retained frames prove the exact decoded
  path shot is repeatable, but still show the known high rig/speaker-wall
  composition, so this is validation plumbing for the remaining source-object
  solve rather than camera parity.

2026-06-28 GH2DXu arena source-object trace reroute:
- The first same-process PCSX2 retry for the arena source-object route used
  the stock `SLUS_214.47` ELF against the GH2DXu statefile and only captured a
  single cold/default `0x00267008` call with no path-apply or writer calls.
  That artifact is limited to proving the helper/screenshot route was alive;
  do not use it as camera-chain evidence.
- Rerunning the same bounded route with the GH2DXu ELF
  `GuitarHeroOGX-trace360/analysis/ps2_trace/external/Guitar-Hero-II-Deluxe-Unified/out/ps2/GHDX_003.00`
  produced accepted active source-object evidence in
  `GuitarHeroOGX-trace360/analysis/ps2_trace/gh2dxu_arena_balcony_lft04_source_trace_ghdxelf_20260628.json`
  plus `.before_trace.window.png` / `.window.png`. The screenshot is active
  gameplay on a dark crowd-facing arena camera. The trace records 518 calls:
  516 `cam_result_builder_00267008` and 2 `cam_path_apply_0026ae00` calls.
- The runtime list/source family appears in the builder calls exactly where
  the previous object-table route predicted it: `0x00267008` receives
  `a0=0x0077c610` six times, and the sampled `0x0077c610` row contains
  `+0x84 -> 0x00caa3e0`, `+0xa4 -> 0x00caa540`, and
  `+0xc4 -> 0x00caa6a0`, with adjacent target/member hints including
  `bone_spine1.mesh` and `spot_neck_fret20.mesh`.
- Same-process row samples show the missing relocation step is still between
  path apply and result builder. `0x0026ae00` samples `a0=0x00cbc010`; its
  `+0x9c` object carries source rows at `+0x20/+0x30/+0x40/+0x50`:
  forward `(0.877757, 0.477847, 0.000558)`, right
  `(-0.470557, 0.864568, -0.172896)`, up
  `(-0.083151, 0.151590, 0.984328)`, and position
  `(34.968578, -64.248741, 60.348461)`, duplicated at `+0x60..+0x90`.
  Later `0x00267008` builder calls with `a0=0x0077c610` consume source row
  `a1=0x00cc0590`, with the same basis family but position
  `(259.234528, -229.846008, 97.250031)`, and output/projection row
  `a2=0x00cc0720`.
- Verdict: this is accepted runtime source-object identity evidence for the
  `0x0077c610`/`0x0077c694` family and confirms the native gap is the shared
  relocation/source-selection step from the path-apply object to the builder
  source row. It is not yet selected-shot evidence for native
  `balcony_lft04`/`Camera03.tnm`, because the screenshoted PS2 frame is a
  crowd-facing shot and the trace still does not bind those native decoded
  labels to the active source row. Do not move submitted native camera behavior
  from these rows alone; the next implementation-grade gate is either mapping
  the relocation step generically or capturing a force/current-shot route that
  binds the row family to the exact decoded path shot.

2026-06-28 CamShot source-ref preservation:
- Raw arena `CamShot__balcony_lft04` carries the packed shot-level tail
  `Camera03.tnm`, category `balcony_lft`, filter `0.3`, and trailing source ref
  `crowd`. This matches the accepted PS2 path-apply object whose sampled
  source-object neighborhood points at `crowd`, but previous native logs dropped
  the trailing source ref after expanding the TransAnim path keys.
- Native now preserves a generic `CameraKey::source_ref`, decodes the trailing
  plausible source ref from the CamShot shot-field block, and falls back to the
  packed category string when TransAnim-backed path CamShots carry `.tnm` data
  before the category tail. Large path CamShots such as `Camera03.tnm` bypass
  the compact key-count shot-field decoder, so native also recovers the
  category/filter/source tail directly from the last packed CamShot category.
  It copies the source ref through TransAnim-backed regular camera keys and
  logs `source_ref=...` on regular CamShot rows. This is evidence plumbing only:
  submitted camera rows still use the accepted generated-source/target-list
  path and do not reinterpret `crowd` as a venue-geometry transform.
- Follow-up native plumbing now drives the source-parent diagnostic from the
  decoded `CameraKey::source_ref` instead of hardcoding `crowd`. The aggregate
  `crowd_group_centroid` comparison remains diagnostic-only and is emitted only
  when the authored source ref is `crowd`.
- Native validation also has a diagnostic-only path-frame offset for forced
  regular CamShots. Accepted PS2 `0x0026ae00` records in the GHDX source trace
  carry `f12=255.0`; a forced native `--diagnostic-camera-shot` otherwise starts
  the path at local frame `0`. Use `--diagnostic-camera-path-offset 255` when
  comparing source rows to that accepted trace so the native `Camera03.tnm`
  sample time matches the PS2 path-apply frame instead of creating an
  apples-to-oranges source mismatch.
- Validation:
  `analysis/native_validation/arena_source_ref_path255_20260628_143853/`
  runs stock PS2 `GEN` assets hidden with `--diagnostic-camera-shot
  balcony_lft04 --diagnostic-camera-path-offset 255`. The run exits `0` and
  logs the forced-shot offset. Native source rows at frame `480.50` move to the
  local path-255 family, `position=(-278.525,-710.459,-80.854)`, but still do
  not match the accepted PS2 builder source object from trace record `513`,
  `a1=0x00cc0590`, `position=(259.235,-229.846,97.250)`,
  `forward=(0.877757,0.477847,0.000558)`. This confirms the remaining gap is
  not merely forced-shot local path timing; native still lacks the shared PS2
  source-object/eval bridge before target-list/filter/screen submission.
- Native now adds diagnostic-only PS2-style world-row copy candidates for that
  bridge shape. `camera_world_copy_candidate_rows()` copies a resolved world
  transform into the same row family logged by `[camera-result]`; one candidate
  resolves CamShot target/member refs through the live performer camera target
  map, and another resolves decoded `source_ref` through the separate venue
  target map. These rows are logged after submitted camera rows are already
  selected and are contract-guarded out of `camera_submitted_result_rows_for_key`.
  They are comparison probes for the accepted `0x00266e58 -> 0x003d7220`
  transform-copy shape, not a rendered camera behavior change.
- Validation:
  `analysis/native_validation/arena_world_copy_diag_20260628_1451/` reruns the
  hidden stock PS2 `GEN` arena route for eight frames with
  `--diagnostic-camera-shot balcony_lft04 --diagnostic-camera-path-offset 255`.
  The run exits `0` and logs the new `ps2_member_world_copy_candidate` rows.
  At frame `480.50`, that member-resolved row is still in the live guitarist
  target family, `position=(-0.653,-64.399,24.177)`, while the submitted row
  remains unchanged at the path-255 generated source family,
  `position=(-278.525,-710.459,-80.854)`. No exact
  `source_ref_world_copy_candidate` rows are emitted because `source_ref=crowd`
  still has no exact decoded venue transform, only the separate diagnostic
  `crowd_group_centroid`. This rules out the current target/member resolver as
  the accepted PS2 builder source row and keeps the next gate focused on the
  path-apply source-object relocation/eval bridge.
- Follow-up monitored PCSX2 traces
  `GuitarHeroOGX-trace360/analysis/ps2_trace/gh2dxu_arena_source_delta_follow_20260628.json`
  and
  `GuitarHeroOGX-trace360/analysis/ps2_trace/gh2dxu_arena_source_fields_follow_20260628.json`
  reran the GHDX arena statefile under an explicit 15-second watchdog. They
  completed in about 30 seconds each. The first retained 82 path applies, 540
  result-builder calls, and 44 writer calls; the second retained 214 path
  applies and 566 result-builder calls. The `s6+0x9c` path output again carries
  the live moving source block, while the newly followed `s6+0x58` object names
  `crowd_area.mesh` and carries a concrete runtime transform. This is the
  strongest current clue for what PS2 means by shot source ref `crowd`: it is
  not native's aggregate `crowd_group_centroid`.
- Native diagnostic follow-up in
  `analysis/native_validation/arena_crowd_area_source_ref_diag_20260628_current/`
  builds and runs cleanly but does not emit a `source_ref_world_copy_candidate`
  or `path_source_parent_source_ref_candidate` row for `crowd_area`, because the
  decoded native venue target map still lacks `crowd_area`/`crowd_area.mesh`.
  The submitted frame `480.50` remains unchanged at the path-255 generated
  source family. The next implementation gate is therefore mapping or decoding
  the runtime `crowd_area.mesh` source object, then applying the traced
  `0x0026ae00` position-difference/eval bridge; do not substitute the aggregate
  crowd centroid or a fixed offset.
- Native scene audit
  `analysis/native_validation/arena_crowd_area_scene_audit_20260628_current/`
  found the missing authored source object in
  `world/arena/gen/arena_chars.milo_ps2`, not
  `world/arena/og/gen/arena_geom.milo_ps2`: the character/crowd scene contains
  `Mesh crowd_area.mesh`, `Mat crowd_area.mat`, `WorldCrowd crowd`, and the band
  waypoints/characters. The mesh decodes as identity basis at
  `(38.2565, -634.1711, -376.8682)`, and the `WorldCrowd crowd` body begins with
  a `crowd_area.mesh` reference followed by the crowd character table and
  placement transforms. Native now merges camera source targets from the
  venue `_chars.milo_ps2` scene into the existing venue camera target map. The
  monitored Ninja retry after clearing a stale `ghogx_app.exe --help` lock linked
  successfully, `ghogx_gameplay_venue_band_contract_test` passed, and
  `analysis/native_validation/arena_crowd_area_source_ref_diag_20260628_chars_targets/`
  exits 0 while logging `venue camera char targets: +2 crowd_area=1` plus
  `source_ref_world_copy_candidate(crowd)->crowd_area` and
  `path_source_parent_source_ref_candidate(crowd)->crowd_area`. These candidate
  rows are still diagnostic-only and intentionally remain after the submitted
  result frame. The candidate transform does not match the PS2 runtime
  `s6+0x58` rows yet, so the next gate remains the traced runtime/evaluated
  `WorldCrowd`/`0x0026ae00` bridge, not any shot-name or fixed-position patch.
- Native now decodes `WorldCrowd` entries in the shared `milo_scene` loader.
  The arena body layout is: version/header, unaligned `crowd_area.mesh` source
  string, total placement count `450`, one pad/flag byte, actor count `5`, five
  actor rows (`crowd_female01`, `crowd_female02`, `crowd_male04`,
  `crowd_male02`, `crowd_male03`) with three actor params each
  `(95.0, 1.0, 10.0)`, then five placement arrays of 90 identity-basis matrices.
  The decoded placement centroid is diagnostic target
  `crowd_placement_centroid` / `crowd_area_placement_centroid`, not a submitted
  camera source. Watched Ninja
  `analysis/native_validation/ninja_watch_20260628_worldcrowd_decode/` completed
  in about 35 seconds with 15-second heartbeats. The focused contract test
  passed, `analysis/native_validation/arena_worldcrowd_decode_20260628_current/`
  confirms `arena_chars.milo_ps2` has `1 world_crowd (1 ok / 0 fail)`, and
  `analysis/native_validation/arena_worldcrowd_camera_diag_20260628_current/`
  exits 0 while logging `world_crowd=1 placements=450
  placement_centroid=1 pos=(-1.510 -608.022 -329.368)`. The same run emits
  `path_source_parent_worldcrowd_candidate` rows, e.g. frame `480.50` at
  `(-280.035, -1318.481, -410.222)`, alongside the static
  `crowd_area.mesh` and aggregate crowd-group candidates. None match the PS2
  traced `s6+0x58` runtime source matrix yet, so the next implementation step is
  still to model the evaluated WorldCrowd/source-area transform used by
  `0x0026ae00`, with the decoded 450 placements as evidence input rather than a
  fixed camera offset.
- Follow-up native diagnostic
  `analysis/native_validation/arena_path_source_delta_diag_20260628_current2/`
  adds a trace-shaped `path_source_delta_source_ref_candidate` row that computes
  decoded path pose minus resolved `source_ref`, matching the broad
  `0x0026ae00` subtraction shape without changing submitted camera behavior.
  Watched Ninja
  `analysis/native_validation/ninja_watch_20260628_path_source_delta_rerun/`
  completed in about 29 seconds with a 15-second heartbeat, and
  `ghogx_gameplay_venue_band_contract_test` passed. The diagnostic falsifies
  the simple static-source interpretation: at frame `480.50`, static
  `crowd_area.mesh` gives
  `path_source_delta_source_ref_candidate(crowd)->crowd_area` position
  `(-980.512, -562.615, 156.628)`, while accepted PS2 trace rows for the same
  forced path-offset route put the builder source object at
  `(259.235, -229.846, 97.250)`. Do not promote this candidate to submitted
  camera behavior; the missing bridge is still the evaluated/runtime source
  object behind `s6+0x58`, not a static source-ref subtraction.
- Native now prefers the structured PS2 `TransAnim` camera-track layout before
  falling back to the older broad byte scan: an unaligned rotation-count block
  (`quat + frame` keys) followed immediately by an unaligned position-count
  block (`xyz + frame` keys). Full raw dump
  `analysis/native_validation/camera03_transanim_full_dump_20260628_current/`
  confirms `Camera03.tnm` has one real position track: `rot_count_off=0x1D`,
  `pos_count_off=0x99`, `data_off=0x9D`, `87` keys, sampled at path frame
  `255` as `(-278.525, -710.459, -80.854)`. Watched Ninja
  `analysis/native_validation/ninja_watch_20260628_structured_transanim/`
  completed in about 27 seconds, the focused contract test passed, and
  `analysis/native_validation/arena_structured_transanim_diag_20260628_current/`
  logs `structured ... selected=1` for `Camera03.tnm`. This rules out a hidden
  alternate authored Vec3 track as the reason accepted PS2 path output rows are
  around `(34.969, -64.249, 60.348)` / builder source rows around
  `(259.235, -229.846, 97.250)`. Continue at the shared resolver/relocation
  step after TransAnim sampling, not by changing the `Camera03.tnm` track
  selection.
- Follow-up alias-world diagnostics in
  `analysis/native_validation/arena_alias_world_copy_diag_20260628_current/`
  add direct `target_alias_*_world_copy_candidate` rows for the entity-only
  CamShot aliases (`spot_neck_fret20.mesh`, `bone_spine1.mesh`,
  `bone_spine2.mesh`, `bone_neck.mesh`) without changing submitted camera
  behavior. Watched Ninja
  `analysis/native_validation/ninja_watch_20260628_alias_world_copy/`
  completed in about 22 seconds, and the focused contract test passed. At
  native frame `484.00`, the closest alias to the accepted PS2 path-output row
  `(34.969, -64.249, 60.348)` is the direct neck copy at
  `(-0.653, -63.015, 38.164)`, still `41.982` units away; the same row is
  `314.428` units from the accepted builder source
  `(259.235, -229.846, 97.250)`. This falsifies the simple "PS2 path output is
  just a direct performer alias world matrix" explanation. The missing bridge
  remains a shared resolver/relocation step between path apply/member lookup
  and the `0x00267008` result builder, not an alias-specific camera patch.
- Native now exposes every decoded `WorldCrowd` placement as a diagnostic camera
  source target (`crowd_placement_N`, `crowd_area_placement_N`, and
  actor-qualified placement keys) and logs a nearest-to-target placement probe
  for crowd-authored path shots. This is diagnostic evidence only; submitted
  camera rows still do not use individual placement selection. Watched Ninja
  `analysis/native_validation/ninja_watch_20260628_worldcrowd_bounds/`
  completed in about 22 seconds, and the focused contract test passed. The
  native probe
  `analysis/native_validation/arena_worldcrowd_bounds_diag_20260628_current/`
  shows arena decoded placement bounds
  `min=(-600.601, -1116.659, -329.374)` /
  `max=(614.343, -234.327, -329.366)`. The nearest placement to the live
  target at frame `480.50` is `crowd_placement_421` at
  `(139.356, -383.227, -329.368)`, still hundreds of units from the accepted
  PS2 path/source rows. This falsifies both the all-placement centroid and the
  nearest raw placement as the missing `s6+0x58` source object. The accepted PS2
  source rows are evaluated runtime world matrices after the WorldCrowd/source
  resolver, so the next bridge is the evaluator behind `0x003d7220`, not raw
  placement selection or averaging.
- Byte-level recheck of `world/arena/gen/arena_chars.milo_ps2` shows the
  `WorldCrowd` actor records are byte-packed: the first per-actor placement
  count starts unaligned at object offset `0xC9`, followed by five `90`
  placement sets ending at `0x553D`. Comparing accepted PS2
  `gh2dxu_arena_source_fields_follow_20260628` source matrix
  `(324.823, -201.873, 70.682)` against decoded placements shows that raw
  placement/world rows remain wrong, but resolving placements relative to the
  stored `crowd_area.mesh` transform is trace-shaped: placement `444`
  raw `(361.494, -832.992, -329.368)` minus `crowd_area.mesh`
  `(38.257, -634.171, -376.868)` gives
  `(323.237, -198.821, 47.500)`, only `23.435` units from the accepted source
  and with nearly all remaining error in source height. The referenced crowd
  actor `char/crowd/og/gen/crowd_female01.milo_ps2` decodes matching skeletal
  source heights (`bone_neck.mesh` world z `55.048`, `bone_head.mesh` world z
  `59.424`, `spot_head.trans` world z `68.173`). Native now exposes
  area-local WorldCrowd placement diagnostics
  (`crowd_area_local_placement_N` and actor/area aliases) and logs nearest
  area-local probes separately from raw placement probes. This is still
  diagnostic-only evidence for the shared WorldCrowd/source resolver; submitted
  camera rows remain unchanged until the PS2 source/bone selection is matched.
- Validation:
  watched Ninja
  `analysis/native_validation/ninja_watch_20260628_area_local_worldcrowd/`
  rebuilt `ghogx_app`, `ghogx`, and the venue/band contract target in about
  `21s`; after the contract source note update, watched Ninja
  `analysis/native_validation/ninja_watch_20260628_area_local_contract_rebuild/`
  rebuilt the contract target in about `10s`. The rebuilt
  `ghogx_gameplay_venue_band_contract_test` passed, and
  `git diff --check` is clean except for the repo's existing CRLF conversion
  warnings. Native probe
  `analysis/native_validation/arena_worldcrowd_area_local_diag_20260628_current/`
  exits after `8` frames in about `3s`, shows the submitted row still sourced
  from `generated_source_seed+target_list+shot_filter+screen`, and logs the new
  `worldcrowd_area_local_nearest_target_world_copy_candidate` /
  `path_source_parent_worldcrowd_area_local_nearest_target_candidate` rows.
- Native now has a generic WorldCrowd actor-source diagnostic layer: when
  camera diagnostics are enabled it resolves each decoded WorldCrowd actor
  through `char/crowd/og/gen/<actor>.milo_ps2`, composes every decoded actor
  `Trans` world row through each area-local placement, and exposes those as
  `crowd_area_local_actor_source_<actor>_<source>_placement_<N>` targets. The
  optional `GHOGX_DEBUG_CAMERA_SOURCE_PROBE=x,y,z` hook ranks these decoded
  source targets against an externally supplied trace coordinate; PS2 trace
  positions are not compiled into runtime behavior. This keeps the new bridge
  source-backed and diagnostic-only while we identify the actual
  `0x003d7220` source selection rule.
- Validation:
  watched Ninja
  `analysis/native_validation/ninja_watch_20260628_actor_source_probe_clean/`
  rebuilt `ghogx_app` and `ghogx` in about `53s` with heartbeats at `15/30/45s`
  and no new warnings beyond pre-existing gameplay warnings. The rebuilt
  `ghogx_gameplay_venue_band_contract_test` passed, and `git diff --check` is
  clean except for the repo's existing CRLF conversion warnings. Native probe
  `analysis/native_validation/arena_worldcrowd_actor_source_probe_20260628_current/`
  used `GHOGX_DEBUG_CAMERA_SOURCE_PROBE=324.822662,-201.872787,70.681671`
  from accepted `gh2dxu_arena_source_fields_follow_20260628`. It decoded all
  five crowd actor MILOs, added `10350` actor-source targets, and ranked
  `crowd_area_local_actor_source_crowd_male03_bone_R-knee_placement_444`
  first at `(327.128, -198.450, 70.366)`, `4.139` units from the accepted PS2
  source row. The submitted camera rows remain
  `generated_source_seed+target_list+shot_filter+screen`, so this evidence does
  not alter live camera behavior yet.
- 2026-06-29 WorldCrowd fullness density follow-up:
  the accepted `arena/balcony_lft04` frame shows low-light close crowd
  silhouettes filling the foreground even while `world/crowd.dta` routes the
  low excitement state through reduced `set_fullness`. A native material probe
  of the bright left-side suite mismatch showed `suite.mat` is authored as
  `blend=SrcAlpha`, `prelit=1`, `use_environ=0`, `rgba=(1,1,1,1)`, and
  `suite.tex` is fully opaque but dark, so the visible far suite was a symptom
  of crowd density/coverage rather than missing texture or environ decode.
  A throwaway decode of `world/arena/gen/arena_chars.milo_ps2` showed the
  accepted camera eye is surrounded by area-local placements: the closest
  placements include `crowd_male03[62]` at distance `53.4` and
  `crowd_male04[61]` at distance `59.2`. Native therefore keeps the DTA
  fullness fractions but selects the visible subset by distance to the active
  camera eye per actor runtime, with stable tie-breaking, instead of hashing a
  random-looking global 25% of placements that could leave the foreground sparse
  and expose far arena geometry.

- 2026-06-29 camera-row sanity follow-up:
  native validation disproved the idea that the retained PS2 `a1` result row is
  sufficient as a rendered gameplay camera. After switching WorldCrowd near-source
  culling to the small decoded actor float, the bounded
  `native_probe_worldcrowd_near_fullness_cullp2_20260629` run drew `113`
  crowd actors with only `2` near-source culls from the accepted trace eye
  `(259.235,-229.846,97.250)`, but the screenshot still exposed the far suite.
  A full-density diagnostic `native_probe_worldcrowd_fullness_great_diag_20260629`
  drew `448/450` placements from the same eye and still exposed the suite, so
  the mismatch is not just DTA `set_fullness` culling. Rendering the retained
  `a2` vector row with `GHOGX_DEBUG_CAMERA_SUBMIT_CANDIDATE=a2` instead points
  into overhead rigging, while `writer` is visually equivalent to the `a1` suite
  view. Default runtime submission therefore no longer silently promotes retained
  trace rows; they remain renderable only through
  `GHOGX_DEBUG_CAMERA_SUBMIT_CANDIDATE=a1/a2/writer` for comparison. The next
  implementation-grade route is the generic PS2 source-object relocation/eval
  bridge (`0x00261c58 -> 0x003d7220`), not hiding suite geometry or hardcoding a
  one-off trace camera as gameplay.

2026-06-24 stock anim_tempo band clip bridge:
- `songs.dtb` carries `(anim_tempo kTempoMedium/kTempoFast)` and
  `char_objects_ps2.dta` band `CharClipSet` `filter_clips` deletes the
  opposite tempo domain through `game get_song_tempo`. Accepted traces and
  decoded clip metadata show the stock fast-domain performer names:
  `singer_active_fast` / `female_singer_active_fast` string families,
  `bassist_active_fast_01`, `keyboard_active_fast`,
  `drummer_active_fast_allbeat`, `drummer_active_fast_normal`,
  `drummer_active_fast_half`, and `drummer_active_fast_nosnare`.
- Native now imports `anim_tempo` through `Catalog::Song` and `QuickplayRig`,
  logs it with the selected quickplay rig, and orders shared performer active
  clip candidates by that song field. Fast songs therefore choose the fast
  stock band clips first, while medium songs keep the previous medium-domain
  order. This is shared candidate ordering, not a role-specific visual hack.
- Validation:
  `analysis/native_validation/anim_tempo_fast_band_yyz_20260624_current_v4/`
  reruns stock PS2 `yyz` in `theatre` from `16.0s` with autoplay. The log
  shows `tempo=kTempoFast`, loads and activates `bassist_active_fast_01`,
  `keyboard_active_fast`, `drummer_active_fast_allbeat`, and
  `drummer_active_fast_nosnare`, with `drummer_active_fast_half` also decoded
  and available. Exit code is `0`, the log scan has no
  unsupported/missing/failed/error/miss rows, and frames `60/88` are nonblank
  theatre venue captures. Treat this as the accepted tempo-domain bridge for
  band clip selection; it is not final authored camera parity.

2026-06-24 drummer double-time active mode bridge:
- The same stock `char_objects_ps2.dta` clip domain lists `kBandDouble`, and
  the drummer script maps `(double_time {$this play_mode kBandDouble})`.
  Accepted trace/string evidence includes `drummer_active_medium_double` and
  `drummer_active_fast_double`, so native now treats `[double_time]` as a
  drummer active mode beside allbeat, half, and nosnare.
- Native loads `active_double_clip` through the shared `anim_tempo` candidate
  ordering and switches the active player to `mode=double` when the BAND DRUMS
  MIDI stream reaches `[double_time]`. The marker clears the other drummer
  mode flags, matching the exclusive `play_mode` shape used by the stock
  script rather than layering a one-off overlay.
- Validation:
  `analysis/native_validation/anim_tempo_fast_band_yyz_double_20260624_current/`
  reruns stock PS2 `yyz` in `theatre` from `16.0s`. The log shows
  `tempo=kTempoFast`, decodes `drummer_active_fast_double`, and switches to
  `performer active clip: role=drummer mode=double
  clip=drummer_active_fast_double` at `17.500`, `23.250`, `29.000`,
  `32.000`, and `36.250`. Exit code is `0`, the failure-word scan is empty,
  and frames `60/88` are nonblank theatre captures with the stage, amps, and
  drum kit visible.

2026-06-24 main.drv beat-scale bridge:
- Stock `CHAR_COMMON` maps `(normal_tempo {main.drv set_beat_scale 1})`,
  `(half_tempo {main.drv set_beat_scale 0.5})`, and
  `(double_tempo {main.drv set_beat_scale 2})`. Native performer MIDI state
  now carries that beat scale and applies it through a shared
  `CharClipPlayer::set_speed` hook on the active main clip player. This keeps
  authored clip sampling/blending shared while allowing song-synchronized
  half/normal/double tempo driver changes.
- Validation:
  `analysis/native_validation/anim_beat_scale_yyz_20260624_current/` reruns
  stock PS2 `yyz` in `theatre` from `16.0s`. The log shows
  `marker=[half_tempo] ... beat_scale=0.500` for guitarist and bassist at
  `31.750`, then `marker=[normal_tempo] ... beat_scale=1.000` for both at
  `36.500`. The same run still decodes `drummer_active_fast_double` and
  switches to `mode=double` on `[double_time]`. Exit code is `0`, the
  failure-word scan is empty, and frames `60/88` are nonblank venue captures.

2026-06-24 female-singer fast route cleanup:
- The trace scans contain `female_singer_active_fast`, but the retained stock
  PS2 ARK object-directory check for
  `char/female_singer/anims/gen/singer_main.milo_ps2` lists the actual
  `CharClipSamples` entries as `singer_idle`, `singer_active_fast`,
  `singer_active_medium_01`, `singer_active_medium_02`, `singer_band_jump`,
  `singer_win`, and `singer_lose`. Native now uses those decoded entries for
  `female_singer` and skips the absent generic `singer_intro` probe on that
  route.
- Validation:
  `analysis/native_validation/anim_tempo_fast_singer_crazyonyou_20260624_current_v3/`
  reruns stock PS2 `crazyonyou` in `fest` from `16.0s`. The log shows
  `tempo=kTempoFast`, band `metal_bass metal_drummer female_singer`,
  `singer_idle` loaded for the female singer, and
  `performer active clip: role=singer mode=normal clip=singer_active_fast` at
  `16.250`. The failure-word scan is empty after removing the stale
  female-specific clip probes, and frames `60/96` are nonblank fest venue
  captures with lighting/camera sweeps active.

2026-06-28 WorldCrowd source-basis probe:
- Accepted PS2 camera-source traces show the followed `crowd_area.mesh` source
  object carries a real yawed basis as well as a position. The retained source
  field trace has the followed source at `(324.823, -201.873, 70.682)` with
  rows near `(0.981780, 0.189059, 0)`, `(-0.189059, 0.981780, 0)`,
  `(0, 0, 1)`, while native static WorldCrowd placement/source diagnostics were
  proving nearby positions only.
- Stock `world/camshot.dta` makes the generic rule explicit: on `start_shot`,
  `crowd_face_camera` dispatches `[crowd] set rotate TRUE`, otherwise
  `[crowd] set rotate FALSE`; `world_objects_ps2.dta` documents the
  `WorldCrowd.rotate` field as "Whether to face the camera". This is the
  evidence route for source-basis parity, not a one-off `balcony_lft04` or
  placement-index rule.
- Native `GHOGX_DEBUG_CAMERA_SOURCE_PROBE` now logs the nearest decoded
  WorldCrowd actor-source diagnostic candidates with `row0`/`row1`/`row2` in
  addition to position and distance. Submitted camera rows are still unchanged;
  the probe is there to prove whether the selected native candidate also has
  the PS2-style runtime orientation before any camera-source promotion.
- Follow-up bridge: native now also has a shared
  `worldcrowd_face_camera_source_world` diagnostic route. When an authored
  camera blend contains `crowd_face_camera` and a source probe is explicitly
  requested, it logs `[camera-source-face-probe]` rows for the same nearest
  decoded actor-source targets after applying the `WorldCrowd.rotate` shape:
  local `+Y` faces the live camera reference, rows are rebuilt as a pure
  Z-up yaw, and position/distance are unchanged. This is still not a submitted
  camera row; it exists to prove the PS2 source-basis rule before using it for
  live camera-source resolution.
- Validation:
  watched Ninja
  `analysis/native_validation/ninja_watch_worldcrowd_face_source_20260628_182233/`
  rebuilt `ghogx_gameplay_venue_band_contract_test`, `ghogx_app`, and `ghogx`
  in about `28s` with a `15s` heartbeat and no timeout; the only warnings were
  the pre-existing gameplay warnings. The rebuilt
  `ghogx_gameplay_venue_band_contract_test` passed. Native probe
  `analysis/native_validation/worldcrowd_face_source_probe_20260628_182327/`
  forced stock arena `band_POV02`, whose decoded CamShot row has
  `crowd_face_camera=1` and `source_ref=crowd`. The log shows the original
  `[camera-source-probe]` nearest candidate still has static knee/bone rows,
  while `[camera-source-face-probe]` for the same candidate keeps the position
  and distance but rebuilds rows as pure yaw
  `row0=(0.205829, 0.978588, 0)`,
  `row1=(-0.978588, 0.205829, 0)`, `row2=(0, 0, 1)`.
- Resolver candidate bridge: the same nearest decoded actor-source target is
  now available to camera diagnostics as
  `worldcrowd_face_actor_source_world_copy_candidate` and
  `path_source_parent_worldcrowd_face_actor_source_candidate`. These rows are
  selected from the live CamShot target centroid and composed through the
  `WorldCrowd.rotate` source basis only when the decoded CamShot has
  `source_ref=crowd` and `crowd_face_camera=1`. Submitted rows remain
  unchanged until the retained PS2 traces prove the selection rule tightly
  enough to promote it.
- Validation:
  watched Ninja
  `analysis/native_validation/ninja_watch_worldcrowd_face_actor_candidate_20260628_182635/`
  rebuilt `ghogx_gameplay_venue_band_contract_test`, `ghogx_app`, and `ghogx`
  in about `21s` with a `15s` heartbeat and no timeout; only the pre-existing
  gameplay warnings remained. `ghogx_gameplay_venue_band_contract_test` passed.
  Native probe
  `analysis/native_validation/worldcrowd_face_actor_candidate_20260628_182720/`
  forced stock arena `band_POV02`. Runtime logs show
  `worldcrowd_face_actor_source_world_copy_candidate` rows selecting nearest
  decoded actor sources from the live target centroid and rebuilding their rows
  as Z-up yaw. No `path_source_parent_worldcrowd_face_actor_source_candidate`
  rows appear in this route because the decoded arena `crowd_face_camera=1`
  shots (`lighter`, `band_POV02`) are not path-backed; keep that helper
  contract-covered until a path-backed face-camera route or specific retained
  PS2 mismatch exercises it.
- Follow-up raw-body audit: the extracted arena `CamShot__band_POV02`,
  `CamShot__lighter`, `CamShot__balcony_lft04`, and `CamShot__balcony_lft01`
  bodies do not carry a hidden WorldCrowd actor, placement index, or 3D-crowd
  list after their category/filter/source tails. The relevant packed strings
  stop at category/filter plus source ref `crowd`, then zeros. That keeps the
  source selection gate in the runtime WorldCrowd evaluator and retained PS2
  trace state, not in a missed CamShot asset field.
- Native now has an explicit trace-probe diagnostic candidate for that gate:
  when `GHOGX_DEBUG_CAMERA_SOURCE_PROBE=x,y,z` is set and the decoded CamShot
  has `source_ref=crowd` plus `crowd_face_camera=1`, the debug camera rows log
  `worldcrowd_probe_face_actor_source_world_copy_candidate` and the
  path-backed companion
  `path_source_parent_worldcrowd_probe_face_actor_source_candidate`. These rows
  select the nearest decoded WorldCrowd actor source to the supplied accepted
  PS2 coordinate, apply the shared `WorldCrowd.rotate` Z-up yaw basis, and stay
  contract-guarded out of submitted camera rows. This is an apples-to-apples
  trace comparison hook, not a fixed coordinate or gameplay behavior change.
- Validation:
  watched Ninja
  `analysis/native_validation/ninja_watch_worldcrowd_probe_candidate_20260628_183655/`
  rebuilt `ghogx_gameplay_venue_band_contract_test`, `ghogx_app`, and `ghogx`
  in about `29s` with a `15s` heartbeat; the wrapper tripped only while
  formatting the final elapsed time, after Ninja had already linked
  `ghogx_app.exe`. The focused contract test passed. Native probe
  `analysis/native_validation/worldcrowd_probe_face_actor_candidate_20260628_183938/`
  forced stock arena `band_POV02` with
  `GHOGX_DEBUG_CAMERA_SOURCE_PROBE=324.822662,-201.872787,70.681671`.
  It exits after 28 fixed frames and logs
  `worldcrowd_probe_face_actor_source_world_copy_candidate` selecting
  `crowd_area_local_actor_source_crowd_male03_bone_R-knee_placement_444` at
  `(327.128, -198.450, 70.366)` with face-camera rows
  `(0.205829, 0.978588, 0)`, `(-0.978588, 0.205829, 0)`, `(0, 0, 1)`.
  The scan finds no failure rows beyond expected coverage counters with
  `failed=0`/`unmatched=0`.
- Reference-variant diagnostic: native now logs the same explicit
  trace-probed actor source faced toward the authored camera eye, authored
  look-at point, CamShot target centroid, and submitted camera position. This
  stays under `GHOGX_DEBUG_CAMERA_SOURCE_PROBE` and does not alter submitted
  camera rows. Validation
  `analysis/native_validation/worldcrowd_ref_variants_20260628_184655/`
  exits `0` in about `4s`; the focused contract test still passes after the
  watched Ninja rebuild
  `analysis/native_validation/ninja_watch_worldcrowd_ref_variants_20260628_184322/`
  linked `ghogx_app.exe` in about `28s`. At frame `487.50`, the probe-selected
  source remains `(327.128, -198.450, 70.366)`. Its face-camera rows are
  `(0.205829, 0.978588, 0)` for camera eye/submitted position,
  `(0.053863, 0.998548, 0)` for authored look-at, and
  `(0.394335, 0.918967, 0)` for target centroid. None match the accepted PS2
  source row `(0.981780, 0.189059, 0)`, so the remaining mismatch is not a
  simple choice among native eye/at/target/submitted camera references. The
  next gate remains PS2's runtime WorldCrowd/source evaluator state, not a
  submitted-camera promotion.
- Native now has non-submitted WorldCrowd projected-axis diagnostics for the
  same gate. `merge_worldcrowd_actor_source_targets` resolves each crowd
  actor's authored `main.drv` clip candidates, samples frame `0` through the
  shared `CharClip` path, and emits separate static, parent, animated, and
  projected-axis target prefixes. The projected-axis prefixes deliberately use
  `crowd_area_local_actor_flat_source_`,
  `crowd_area_local_actor_parent_flat_source_`, and
  `crowd_area_local_actor_anim_flat_source_` so they cannot be picked up by
  the real `crowd_area_local_actor_source_` nearest-source selector. The
  debug-only `GHOGX_DEBUG_CAMERA_SOURCE_PROBE_FORWARD` hook ranks those
  candidates against a retained PS2 source-forward row.
- Validation:
  watched Ninja
  `analysis/native_validation/ninja_watch_anim_probe_20260628_204440/`,
  `analysis/native_validation/ninja_watch_projected_source_20260628_204827/`,
  `analysis/native_validation/ninja_watch_anim_flat_source_20260628_205033/`,
  and `analysis/native_validation/ninja_watch_axis_probe_20260628_205244/`
  rebuilt the native app/test targets in about `18s`-`21s` each with no
  timeout. Native probes
  `analysis/native_validation/worldcrowd_anim_probe_20260628_204549/`,
  `analysis/native_validation/worldcrowd_anim_flat_source_probe_20260628_205107/`,
  and `analysis/native_validation/worldcrowd_axis_rank_probe_20260628_205321/`
  forced stock arena `band_POV02` with the accepted PS2 source coordinate
  `(324.822662, -201.872787, 70.681671)` and forward
  `(0.981780, 0.189059, 0)`. The nearest static source remains
  `crowd_area_local_actor_source_crowd_male03_bone_R-knee_placement_444` at
  `(327.128, -198.450, 70.366)`, only `4.139` units away, but its raw rows do
  not match the retained PS2 yaw. The best animated projected-axis yaw match is
  `crowd_area_local_actor_anim_flat_source_z_crowd_male03_bone_L-ankle_placement_444`
  with row0 `(0.983351, 0.181715, 0)` and dot `0.999972` against the accepted
  forward, but it is `19.200` units from the accepted source position and low
  in Z. Static/parent projected rows can match position or broad yaw shape, but
  not both. Do not promote these candidates to submitted camera rows; the next
  evidence-backed step is a proper time-varying WorldCrowd/source evaluator
  bridge, not a one-off camera matrix or bone-name rule.
- Native now retains the decoded venue `*_chars.milo_ps2` scene and caches
  decoded WorldCrowd crowd actor scenes/characters/clips on `Gameplay`.
  `refresh_worldcrowd_actor_source_targets_for_camera` runs before regular or
  intro camera evaluation when camera diagnostics are enabled, samples the
  shared `CharClip` at the current song-time frame instead of hard-coded frame
  `0`, and updates the animated WorldCrowd actor-source target families in the
  existing camera target map. Submitted camera rows are still unchanged; this
  is the runtime source-evaluator bridge needed to compare live candidate
  movement against retained PS2 traces before any promotion.
- Validation:
  watched Ninja
  `analysis/native_validation/ninja_watch_live_worldcrowd_probe_20260628_210548/`
  rebuilt `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` in about
  `25s` with a `15s` heartbeat and no timeout; only the existing warnings
  remained. The focused `ghogx_gameplay_venue_band_contract_test` passed after
  the follow-up watched test rebuild
  `analysis/native_validation/ninja_watch_live_worldcrowd_contract3_20260628_210812/`.
  Native probe
  `analysis/native_validation/worldcrowd_live_source_probe_clean_20260628_211154/`
  forced stock arena `band_POV02` from `16.0s` with the accepted source
  coordinate/forward. It logs `14` live samples from `t=16.233` through the
  bounded `28`-frame run and exits after `7.00s` engine time with no
  `NativeCommandError`, timeout, missing-ARK, fatal, or error rows. The
  evidence shows the animated source map is now genuinely time-varying: the
  setup frame-0 nearest animated source was
  `crowd_male03_bone_L-knee_placement_444` at
  `(320.063, -193.214, 69.655)`, while the first live sample at `16.233s`
  updates rank 0 to `crowd_male03_bone_R-knee_placement_444` at
  `(328.399, -194.104, 69.711)`, then `16.733s` moves it to
  `(327.543, -194.899, 69.970)`, and later samples continue changing. The
  best yaw-ranked projected-axis candidates also change over time, but still
  trade off yaw match against accepted source position. Keep this diagnostic
  out of submitted rows until the PS2 source-evaluator selection rule is
  matched.
- Native now also emits a trace-pose-ranked WorldCrowd source candidate in
  normal `[camera-result]` row format. The helper
  `camera_pose_ranked_worldcrowd_actor_source_ref` uses the explicit accepted
  PS2 source probe position plus `GHOGX_DEBUG_CAMERA_SOURCE_PROBE_FORWARD`,
  with the same
  `GHOGX_DEBUG_CAMERA_SOURCE_POSE_ANGLE_WEIGHT` distance/angle score as the
  source probe logs, and ranks across static source, static flat, parent flat,
  animated source, and animated flat WorldCrowd actor-source prefixes. The
  rows are logged as
  `worldcrowd_probe_pose_actor_source_world_copy_candidate`, with a
  path-backed companion
  `path_source_parent_worldcrowd_probe_pose_actor_source_candidate` for future
  path CamShots. Both remain contract-guarded out of
  `camera_submitted_result_rows_for_key`.
- Validation:
  watched Ninja
  `analysis/native_validation/ninja_watch_pose_ranked_source_20260628_213226/`
  rebuilt `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` in about
  `24s`; the compile/link output completed with only the existing gameplay
  warnings, and the wrapper only tripped on the known blank Windows process
  exit-code field after linking. The focused
  `ghogx_gameplay_venue_band_contract_test` passed in
  `analysis/native_validation/ctest_pose_ranked_source_20260628_213305/`.
  Native probe
  `analysis/native_validation/worldcrowd_pose_ranked_source_20260628_213340/`
  forced stock arena `band_POV02` from `16.0s` with the accepted PS2 source
  coordinate `(324.822662, -201.872787, 70.681671)` and forward
  `(0.981780, 0.189059, 0)`. It logs `28`
  `worldcrowd_probe_pose_actor_source_world_copy_candidate` rows, zero
  path-pose rows because this route is not path-backed, and zero failure
  markers. The combined candidate resolves to
  `crowd_area_local_actor_parent_flat_source_x_crowd_male03_bone_R-knee_placement_444`
  with position `(327.128, -198.450, 70.366)`, score `4.680`, distance
  `4.139`, and dot `0.981959`. This confirms the best current native
  position+forward candidate is still a parent-flat placement row near the
  accepted source, not an animated submitted-camera promotion; the next gate is
  identifying PS2's runtime source evaluator/basis choice that yields
  `(0.981780, 0.189059, 0)` at that placement.
- Native now also has a diagnostic-only relocation-vector probe for the
  accepted PS2 path-apply-to-builder gap. When
  `GHOGX_DEBUG_CAMERA_PATH_SOURCE_PROBE=x,y,z` and
  `GHOGX_DEBUG_CAMERA_SOURCE_PROBE=x,y,z` are both supplied for a
  `source_ref=crowd` CamShot, the camera debug path ranks decoded WorldCrowd
  actor-source candidates by how well their native
  `generated/source_seed -> candidate` delta matches the retained PS2
  `0x0026ae00 path-source -> 0x00267008 builder-source` delta, with
  `GHOGX_DEBUG_CAMERA_RELOCATION_BUILDER_WEIGHT` as the explicit builder-row
  tie-break and optional `GHOGX_DEBUG_CAMERA_SOURCE_PROBE_FORWARD` basis
  weighting. The row logs as
  `worldcrowd_relocation_delta_actor_source_world_copy_candidate` and remains
  contract-guarded out of submitted camera rows. Use this to decide whether
  native has the right decoded source family but the wrong evaluator
  composition, or whether the PS2 selector still needs a deeper trace; do not
  promote it without a native/PS2 row match.
- Validation:
  watched Ninja
  `analysis/native_validation/ninja_watch_relocation_delta_20260628_214815/`
  rebuilt `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` in about
  `26s` with a `16s` heartbeat. Compile/link completed with only the existing
  gameplay warnings; the wrapper again exposed the known blank Windows process
  exit-code field after linking. The focused
  `ghogx_gameplay_venue_band_contract_test` passed in
  `analysis/native_validation/ctest_relocation_delta_20260628_214855/`.
  Native probe
  `analysis/native_validation/relocation_delta_balcony_20260628_215003/`
  forced stock arena `balcony_lft04` / path offset `255` from `16.0s` with
  accepted PS2 path-source `(34.968578, -64.248741, 60.348461)`, builder
  source `(259.234528, -229.846008, 97.250031)`, and forward
  `(0.877757, 0.477847, 0.000558)`. It logs `28`
  `worldcrowd_relocation_delta_actor_source_world_copy_candidate` rows and
  zero failure markers. The best row is still far from the PS2 relocation:
  frame `487.50` selects
  `crowd_area_local_actor_anim_flat_source_z_crowd_male04_bone_L-ankle_placement_259`
  at `(-2.282688, -483.027771, 52.540649)` with score `500.383`, relocation
  delta error `407.026`, builder distance `366.730`, and dot `0.944187`.
  Subsequent best deltas remain hundreds of units away. This falsifies the
  simple decoded-WorldCrowd-source explanation for the accepted
  path-apply-to-builder relocation. Keep submitted camera behavior unchanged;
  the next evidence gate is the deeper PS2 runtime source evaluator/source
  object family around `0x0077c610` and offsets `+0x84/+0xa4/+0xc4`.
- Native now also emits a diagnostic-only PS2 member-entry relocation probe.
  The accepted `0x0077c610` neighborhood shows source-object list/member
  entries at offsets including `+0x84`, `+0xa4`, and `+0xc4`, with
  `bone_spine1.mesh` and nearby `spot_neck_fret20.mesh` hints. The new
  `ps2_member_relocation_delta_target_world_copy_candidate` row ranks native
  camera target members such as `bone_spine1.mesh`, `bone_spine2.mesh`,
  `bone_spine3.mesh`, `bone_neck.mesh`, and `spot_neck_fret20.mesh` by the
  same retained PS2 `path-source -> builder-source` relocation delta used by
  the WorldCrowd relocation probe. It is logged after submitted rows and is
  contract-guarded out of `camera_submitted_result_rows_for_key`.
- Validation:
  watched Ninja
  `analysis/native_validation/ninja_watch_member_relocation_delta_20260629_000001/`
  rebuilt `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` in about
  `24s` with a `16s` heartbeat. The focused contract test passed in
  `analysis/native_validation/ctest_member_relocation_delta_20260629_000001/`.
  Native probe
  `analysis/native_validation/member_relocation_delta_balcony_20260629_000001/`
  forced stock arena `balcony_lft04` / path offset `255` from `16.0s` with
  the same accepted PS2 path-source, builder-source, and forward probe rows as
  the WorldCrowd relocation run. It logs `28`
  `ps2_member_relocation_delta_target_world_copy_candidate` rows, `28`
  WorldCrowd relocation rows, and zero failure markers. The best member-entry
  result is still decisively wrong: frame `487.50` selects
  `guitarist0:bone_spine2.mesh` with score `997.132`, relocation delta error
  `880.315`, builder distance `350.932`, and dot `0.030526`. This falsifies
  the simple "PS2 source object member hints are direct native performer
  target members" explanation; keep the remaining gate on the deeper runtime
  source evaluator/list traversal, not target-member promotion.

2026-06-29 PS2 source owner/member helper chain and all-member native probe:
- Static disassembly in
  `analysis/ps2_trace_snippets/camera_source_helper_funcs_20260629.json`
  shows the active `0x00267008` list walk calling `0x00261c58`. The helper
  reads the owner object from `entry+0x8` and the member symbol from
  `entry+0xc`, resolves that child under the owner, then the later
  `0x003d7220` path returns the updated world rows at transform `+0x60`.
  For the accepted `0x0077c610` table, the caller passes the inner entry at
  the 32-byte record's `+0x10`, so the retained words at record offsets
  `+0x18/+0x1c` are the real owner/member pair. The first changing object
  pointer in each 32-byte record is shot/category state, not the transform
  owner consumed by `0x00261c58`.
- Retained `gh2dxu_arena_builder_a0_shot_identity_long_20260624.json` follows
  `0x00caa3e0` as `SOLO_NEAR02` and the `balcony_lft` shot family. Its nested
  owner `0x00cb9530` has a `0x003d7220`-shaped transform layout: local rows at
  `+0x20..+0x50` and world rows at `+0x60..+0x90`, with root world position
  about `(92.976, 86.079, 21.451)`. That root does not equal the accepted
  builder source `(259.235, -229.846, 97.250)`, so the missing native bridge is
  the resolved child/world matrix under that owner, not the owner root itself.
- Native now also emits
  `ps2_all_member_relocation_delta_target_world_copy_candidate`, a
  diagnostic-only rank across all loaded performer camera targets for the PS2
  member hints (`bone_spine1.mesh`, `bone_spine2.mesh`, `bone_spine3.mesh`,
  `bone_neck.mesh`, and `spot_neck_fret20.mesh`). This deliberately removes
  the previous key-target scope while staying contract-guarded out of
  submitted camera rows.
- Validation:
  watched Ninja
  `analysis/native_validation/ninja_watch_all_member_relocation_delta_20260629_010002/`
  rebuilt `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` in about
  `28s` with a `15s` heartbeat; the focused contract test passed in
  `analysis/native_validation/ctest_all_member_relocation_delta_20260629_010002/`.
  The first native probe attempts at `_010002` and `_010003/_010004` were
  wrapper failures only: one split the ARK path at the space in
  `Guitar Hero II`, and one used a redirected pipe that could stall while
  venue asset logs filled stderr. The corrected file-redirected probe
  `analysis/native_validation/all_member_relocation_delta_balcony_20260629_010005/`
  completed in about `25s` and logged `28` rows each for the all-member,
  key-member, WorldCrowd relocation, and WorldCrowd pose candidates. The
  apparent failure-marker count is only normal `0 fail` asset coverage text.
- Probe result: frame `487.50` all-member selection is still decisively wrong,
  choosing `bassist:bone_neck.mesh` with score `986.245`, relocation delta
  `878.832`, builder distance `347.392`, and dot `0.314478`; the key-scoped
  member probe remains similarly wrong at `guitarist0:bone_spine2.mesh`.
  This falsifies both current performer-member target pools as the native
  equivalent of PS2 owner `0x00cb9530` plus `bone_spine1.mesh`. In the same
  run, the explicit accepted-source pose probe ranks
  `crowd_area_local_actor_flat_source_x_crowd_male04_bone_R-hand_placement_241`
  at `(252.908, -218.753, 93.703)` with score `13.532`, distance `13.254`,
  and dot `0.990737`, which is much closer to the accepted builder source.
  Continue the source bridge on the WorldCrowd actor-source transform family
  and its PS2 owner/member resolver; do not promote performer-member targets
  or relocation-delta rows into submitted camera behavior.
- Native now also logs raw CamShot source-tail diagnostics under
  `GHOGX_DEBUG_CAMERA=1`. The decoded `CameraKey` keeps the source pose offset,
  ref-tail cursor, and shot-tail cursor, and `[camera-source-tail]` rows dump
  packed strings plus any object-array refs between the key refs and the
  shot/category tail. This is diagnostic-only and contract-covered out of the
  submitted camera result path.
- Validation:
  watched Ninja
  `analysis/native_validation/ninja_watch_camshot_source_tail_20260628_223705/`
  rebuilt `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` in about
  `21s` with a `15s` heartbeat. The first build attempt
  `analysis/native_validation/ninja_watch_camshot_source_tail_20260628_223604/`
  failed quickly on a missing `<sstream>` include for the new local hex
  formatter, then the fixed rebuild completed with only the existing
  `gameplay.cpp` goto warning. The focused contract test passed in
  `analysis/native_validation/ctest_camshot_source_tail_20260628_223745/`.
  Native probe
  `analysis/native_validation/camshot_source_tail_balcony_20260628_223830/`
  forced stock arena `balcony_lft04` / path offset `255` from `16.0s` with
  `GHOGX_DEBUG_CAMERA=1`; it exited in about `7s`, logged `38`
  `[camera-source-tail]` rows, and found no fatal/error rows.
- Probe result: `balcony_lft04` reports
  `pose=0x197 ref_end=0x202 tail=0x202 category=balcony_lft source_ref=crowd`
  with tail strings `0x228:Camera03.tnm`, `0x23C:balcony_lft`,
  `0x25B:crowd`, and `arrays=<none>`. `balcony_lft03` similarly exposes
  `Camera02.tnm` plus `balcony_lft`/`crowd`, and all decoded regular arena
  CamShots in this run report `arrays=<none>`. The accepted PS2
  owner/member source-object list is therefore not hiding in an undecoded
  CamShot object array; it is runtime evaluator/list state reached after the
  asset-level `source_ref=crowd` and path animation are known. Keep the next
  gate on retained PS2 source evaluator state around `0x0077c610` and
  `0x00261c58`, not on native CamShot raw-body parsing.
- Retained trace row check: in
  `gh2dxu_arena_builder_a0_shot_identity_long_20260624.json`, the active
  `cam_result_builder_00267008` call uses `a0=0x0077c610`. Its accepted
  32-byte source-object record at table offset `+0x80` corresponds to
  `0x0077c690` in
  `gh2dxu_arena_shot_object_neighborhood_20260628.json`:
  `+0x0=0x003e3b70`, `+0x4=0x00caa3e0`, `+0x8=0x00cb9530`,
  `+0xc=0x00a61dd4`, where the neighborhood sampler resolves
  `0x00a61dd4` to `bone_spine1.mesh`. The adjacent records at
  `0x0077c6d0` and `0x0077c6f0` show the same owner pattern with
  `bone_spine1.mesh` and `spot_neck_fret20.mesh`. This pins the next native
  bridge to a PS2-style source-object record evaluator: select the runtime
  record, resolve owner `0x00cb9530`, then resolve the member symbol under
  that owner before building the submitted camera source rows.
- Native now keeps the decoded WorldCrowd actor-source target inventory live
  during normal camera evaluation when loaded cameras carry authored
  `source_ref=crowd`, instead of sampling it only while
  `GHOGX_DEBUG_CAMERA=1`. Debug output remains gated by `GHOGX_DEBUG_CAMERA`,
  and submitted camera rows still do not consume any WorldCrowd source
  candidate. This is a prerequisite for the PS2 source-object evaluator above:
  the native runtime now has the same time-sampled actor-source families
  available outside validation logging, while the contract still forbids
  promoting trace-probe or relocation heuristics into submitted camera output.
- Validation:
  watched Ninja
  `analysis/native_validation/ninja_watch_worldcrowd_runtime_refresh_20260628_225703/`
  rebuilt `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` in about
  `26s` with a `15s` heartbeat. The process-exit field was again blank in the
  PowerShell wrapper, but Ninja linked both targets and stderr was empty; the
  stdout log shows only the existing gameplay warnings. The focused contract
  test returned cleanly in
  `analysis/native_validation/ctest_worldcrowd_runtime_refresh_20260628_225757/`;
  the direct rerun
  `analysis/native_validation/ctest_worldcrowd_runtime_refresh_direct_20260628_230035/`
  returned numeric exit code `0`.
  A no-debug native render in
  `analysis/native_validation/worldcrowd_runtime_refresh_nodebug_20260628_225849/`
  ran stock arena `shoutatthedevil` from `16.0s`, captured frames `40` and
  `79`, logged regular camera sweeps, and produced zero camera-debug/source
  sample rows and zero fatal markers. The retained-source debug probe in
  `analysis/native_validation/worldcrowd_runtime_refresh_probe_20260628_225931/`
  forced `balcony_lft04` / path offset `255` with the accepted PS2 path-source,
  builder-source, and forward probe values; it logged `38`
  `[camera-source-tail]` rows, `8` live WorldCrowd source samples, `32`
  trace-pose WorldCrowd source rows, `48` relocation rows, and zero fatal
  markers.
- Probe result: the source-tail row still shows `balcony_lft04`
  `source_ref=crowd` with only `Camera03.tnm`, `balcony_lft`, and `crowd`
  strings and `arrays=<none>`. The trace-pose rank still selects
  `crowd_area_local_actor_flat_source_x_crowd_male04_bone_R-hand_placement_241`
  at `(252.907715, -218.753113, 93.703407)` with score `13.532`, distance
  `13.254`, and dot `0.990737`; relocation ranking still selects far-off
  ankle/placement rows with scores over `500`. This preserves the current
  conclusion: the next promotion target is the PS2-style owner/member evaluator
  for `0x00cb9530` / `bone_spine1.mesh`, not nearest/relocation heuristics.

2026-06-28 PS2 source-record owner/member diagnostic:
- Native now has a non-submitted PS2 source-record diagnostic for the retained
  helper shape seen at `0x00261c58`: source record `+8` is the owner object and
  `+0xc` is the member symbol. When
  `GHOGX_DEBUG_CAMERA_SOURCE_RECORD_OWNER_PROBE=x,y,z` and
  `GHOGX_DEBUG_CAMERA_SOURCE_RECORD_MEMBER=name.mesh` are set, crowd-authored
  camera debug evaluation ranks decoded WorldCrowd actor source rows by owner
  distance, optional accepted PS2 builder-source distance, and optional accepted
  PS2 forward-axis dot. The row logs the selected native source target, the
  owner/root target used for the comparison, and the normalized member name.
  It is deliberately diagnostic-only; the contract still forbids this candidate
  from entering submitted camera rows.
- Native validation found the expected rig-name mismatch: the retained PS2
  source record carries `bone_spine1.mesh`, while native WorldCrowd crowd actor
  source rows expose `bone_spine`. The diagnostic therefore compares the exact
  member first, then a generic trailing-number-stripped member alias. This is
  not a shot-specific visual shortcut; it is documented rig member
  normalization for comparing PS2 performer/member records against decoded
  crowd actor source rows.
- Validation:
  watched Ninja
  `analysis/native_validation/ninja_watch_ps2_source_record_member_20260628_230930/`
  rebuilt the contract test and app in about `24s` with a `15s` heartbeat.
  Direct contract
  `analysis/native_validation/ctest_ps2_source_record_member_direct_20260628_231003/`
  returned exit code `0`. The first native probe
  `analysis/native_validation/native_probe_ps2_source_record_member_20260628_231027/`
  used a misquoted ARK path and correctly loaded no song/camera rows; the
  corrected probe
  `analysis/native_validation/native_probe_ps2_source_record_member_20260628_231059/`
  logged `38` `[camera-source-tail]` rows, `8` live WorldCrowd source samples,
  `32` trace-pose rows, `48` relocation rows, and zero PS2 source-record rows,
  proving the exact `bone_spine1` name did not exist in native WorldCrowd
  source rows. After adding the shared evaluated flat source families and
  generic member normalization, watched Ninja
  `analysis/native_validation/ninja_watch_ps2_source_record_norm_20260628_231428/`
  rebuilt in about `22s`, direct contract
  `analysis/native_validation/ctest_ps2_source_record_norm_direct_20260628_231518/`
  returned exit code `0`, and the native probe
  `analysis/native_validation/native_probe_ps2_source_record_norm_20260628_231518/`
  exited in about `16s` with `32`
  `ps2_source_record_member_actor_source_world_copy_candidate` rows, `32`
  trace-pose rows, `48` relocation rows, `38` source-tail rows, `8` live
  WorldCrowd source samples, and zero fatal/error lines.
- Probe result: retained `bone_spine1.mesh` now normalizes to native
  `bone_spine` and selects
  `crowd_area_local_actor_flat_source_y_crowd_female01_bone_spine_placement_15`
  / animated variants, with `owner_ref=crowd_area_local_placement_15` and
  owner distance about `30.390`. Source distance remains large, around
  `359-362`, so this is evidence for the owner/member resolution bridge and
  rig-member normalization, not a camera promotion candidate yet. The next
  native step is to turn this from an env-only probe into the shared source
  record evaluator fed by decoded runtime source records, then compare its
  output against submitted PS2 source rows before any promotion.

2026-06-28 decoded CamShot source-record hint bridge:
- `CameraKey` now carries a typed `SourceRecordHint` beside the decoded CamShot
  refs. For crowd-authored shots, native syncs this hint from decoded
  `source_ref=crowd` plus the first decoded target/member or parent/member ref,
  propagates it through pose and path-backed camera keys, and lets the
  PS2-source-record diagnostic consume the decoded member before falling back
  to `GHOGX_DEBUG_CAMERA_SOURCE_RECORD_MEMBER`. This removes the member side
  of the diagnostic from env-only validation on shots whose CamShot data
  already carries a member ref. The owner transform remains explicit validation
  input until the runtime PS2 source-object owner list is decoded natively.
- Validation:
  watched Ninja
  `analysis/native_validation/ninja_watch_source_record_hint_20260628_231941/`
  rebuilt `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` in about
  `20s` with a `15s` heartbeat. Stderr was empty; stdout showed the existing
  gameplay warnings and one existing app-main unused-alias warning. Direct
  contract
  `analysis/native_validation/ctest_source_record_hint_direct_20260628_232032/`
  returned exit code `0`.
- Native probe
  `analysis/native_validation/native_probe_source_record_hint_no_member_env_20260628_232032/`
  forced the accepted `balcony_lft04` / path offset `255` route with
  `GHOGX_DEBUG_CAMERA_SOURCE_RECORD_OWNER_PROBE` set but no
  `GHOGX_DEBUG_CAMERA_SOURCE_RECORD_MEMBER`. It logged `38`
  `[camera-source-tail]` rows, `8` live WorldCrowd samples, `32` trace-pose
  rows, `48` relocation rows, zero PS2 source-record rows, and zero fatal/error
  rows. This confirms the native CamShot body still does not expose the
  `balcony_lft04` member; that member is still PS2 runtime source-list state,
  matching the retained `0x0077c610` source-object table evidence.
- Native probe
  `analysis/native_validation/native_probe_source_record_hint_flr_near_rt02_20260628_232116/`
  forced `flr_near_rt02` with the same owner/source/forward probes and no
  member env var. It logged `32`
  `ps2_source_record_member_actor_source_world_copy_candidate` rows, including
  `16` rows with `member=bone_spine`, plus `32` trace-pose rows, `32`
  relocation rows, `38` source-tail rows, `8` live WorldCrowd source samples,
  and zero fatal/error rows. The selected row again normalizes decoded
  `guitarist0:bone_spine1.mesh` to native WorldCrowd `bone_spine` and selects
  `crowd_area_local_placement_15`, proving the decoded source-record hint path
  works without the member env hook.
- Next gate: decode or model the PS2 runtime source-object owner/member list
  that feeds `balcony_lft04`, rather than promoting nearest/relocation or
  decoded-CamShot-member hints. The native evidence now cleanly separates
  CamShot-carried member refs from runtime evaluator source records.

2026-06-28 decoded source-record table carrier:
- Native now builds a diagnostic-only member table from decoded regular
  CamShot `SourceRecordHint` entries and feeds that table into the PS2
  source-record WorldCrowd evaluator. This is deliberately separate from the
  submitted camera rows: it exists to model the PS2 runtime source-object
  list shape and to prove which decoded members can be resolved against native
  WorldCrowd actor source rows before any camera promotion.
- Direct contract
  `analysis/native_validation/ctest_source_record_table_direct_reliable_20260628_233712/`
  returned exit code `0`. The contract checks the member table builder, the
  multi-member evaluator, the
  `ps2_source_record_table_actor_source_world_copy_candidate` diagnostic stage,
  and that this evaluator is absent from submitted camera result assembly.
- Native `balcony_lft04` probe
  `analysis/native_validation/native_probe_source_record_table_balcony_20260628_233735/`
  used the retained PS2 owner/source/forward rows with no
  `GHOGX_DEBUG_CAMERA_SOURCE_RECORD_MEMBER`. It logged `16`
  `ps2_source_record_table_actor_source_world_copy_candidate` rows, zero
  single-member `ps2_source_record_member_actor_source_world_copy_candidate`
  rows, `16` `member=bone_spine` rows, `16` trace-pose rows, `16`
  relocation rows, `38` `[camera-source-tail]` rows, `8` live WorldCrowd
  source samples, and zero fatal/error lines. This proves the balcony route can
  now get diagnostic source-record rows from the native decoded table even
  though the active CamShot still does not carry its own member hint.
- Native `flr_near_rt02` probe
  `analysis/native_validation/native_probe_source_record_table_flr_near_rt02_20260628_233846/`
  used the same retained PS2 rows and no member env var. It logged `16`
  source-record table rows and `16` decoded single-member rows, with `32`
  total `member=bone_spine` rows, `16` trace-pose rows, `16` relocation rows,
  `38` source-tail rows, `8` live WorldCrowd source samples, and zero
  fatal/error lines. The table path selects the same native row family as the
  decoded single-member path:
  `crowd_area_local_actor_flat_source_y_crowd_female01_bone_spine_placement_15`
  with `owner_ref=crowd_area_local_placement_15`, owner distance about
  `30.390`, source distance about `358.898`, and dot about `0.478136`.
- Promotion gate: do not submit this row to the camera. The table carrier
  proves native can retain and resolve decoded member families, but the large
  source-distance/axis mismatch says the real promotion needs the PS2 runtime
  owner/source-object list, or a stronger native model of that list, rather
  than a global member-table nearest match.

2026-06-28 source-record ranked candidate probe:
- Native source-record diagnostics now support opt-in ranked rows via
  `GHOGX_DEBUG_CAMERA_SOURCE_RECORD_RANKS`, plus
  `GHOGX_DEBUG_CAMERA_SOURCE_RECORD_OWNER_WEIGHT` to isolate owner-root
  distance from source-position/axis scoring. Defaults preserve prior behavior:
  owner weight is `1.0`, source weight remains
  `GHOGX_DEBUG_CAMERA_SOURCE_RECORD_SOURCE_WEIGHT` default `0.25`, and ranked
  logging is silent unless explicitly enabled. The ranked rows are
  diagnostic-only and do not feed submitted camera output.
- Watched Ninja
  `analysis/native_validation/ninja_watch_source_record_owner_weight_20260628_234726/`
  rebuilt `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` in about
  `31s` with a `15s` heartbeat and only the existing gameplay warnings.
  Direct contract
  `analysis/native_validation/ctest_source_record_owner_weight_direct_20260628_234806/`
  returned exit code `0`.
- Native `balcony_lft04` rank probe
  `analysis/native_validation/native_probe_source_record_ranks_balcony_20260628_234459/`
  used the accepted retained owner/source/forward rows with no member env var
  and rank limit `8`. It emitted `8`
  `[camera-source-record-probe]` rows, `24`
  `ps2_source_record_table_actor_source_world_copy_candidate` rows, `16`
  trace-pose WorldCrowd rows, and zero fatal/error rows. With default owner
  weight, rank `0` remained
  `crowd_area_local_actor_flat_source_y_crowd_female01_bone_spine_placement_15`
  with owner distance `30.390`, source distance `358.898`, dot `0.478136`,
  and score `135.774`; the accepted source-pose neighborhood is not selected
  because owner-root proximity dominates the score.
- Native comparison probe
  `analysis/native_validation/native_probe_source_record_owner0_rhand_balcony_20260628_234830/`
  set `GHOGX_DEBUG_CAMERA_SOURCE_RECORD_MEMBER=bone_R-hand.mesh` and
  `GHOGX_DEBUG_CAMERA_SOURCE_RECORD_OWNER_WEIGHT=0`. It emitted `16`
  ranked rows, `24` single-member source-record candidate rows, `16`
  trace-pose WorldCrowd rows, and zero fatal/error rows. With owner-root
  scoring disabled, rank `0` is the accepted trace-pose source family:
  `crowd_area_local_actor_flat_source_x_crowd_male04_bone_R-hand_placement_241`
  with source distance `13.254`, dot `0.990737`, and score `3.591`, while
  its owner-root distance is `338.123`.
- Interpretation: native already contains the positive source-position/axis
  row that matches the retained builder source, but the retained PS2
  source-record member is `bone_spine1.mesh`, not `bone_R-hand.mesh`, and the
  PS2 helper consumes an owner/member object relationship rather than a simple
  native owner-root nearest match. The next implementation gate is therefore a
  PS2-shaped resolver from the runtime `0x0077c610` owner/member record family
  into the WorldCrowd actor-source transform family, not a tweak to submitted
  camera scoring.

2026-06-28 source-record sibling actor-source probe:
- Native now has a second source-record diagnostic that treats the retained
  PS2 member (`bone_spine1.mesh`, normalized to `bone_spine`) as an
  actor/placement anchor, then ranks sibling actor-source transforms under the
  same resolved crowd actor placement. This tests the PS2-shaped hypothesis
  that the runtime source record identifies an object context, while the camera
  source/result rows can come from a sibling transform in that object family.
  The stage is
  `ps2_source_record_sibling_actor_source_world_copy_candidate`, emits optional
  `[camera-source-record-sibling-probe]` ranks through
  `GHOGX_DEBUG_CAMERA_SOURCE_RECORD_SIBLING_RANKS`, and remains
  contract-guarded out of submitted camera rows.
- Watched Ninja
  `analysis/native_validation/ninja_watch_source_record_sibling_20260628_235539/`
  rebuilt `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` in about
  `30s` with a `15s` heartbeat and only the existing gameplay warnings.
  Direct contract
  `analysis/native_validation/ctest_source_record_sibling_direct_20260628_235630/`
  returned exit code `0`.
- Native `balcony_lft04` sibling probe
  `analysis/native_validation/native_probe_source_record_sibling_owner0_balcony_20260628_235710/`
  used the accepted retained owner/source/forward rows, no member env var,
  source-record table hints, `GHOGX_DEBUG_CAMERA_SOURCE_RECORD_OWNER_WEIGHT=0`,
  and sibling rank limit `12`. It logged `12`
  `[camera-source-record-sibling-probe]` rows, `28`
  `ps2_source_record_sibling_actor_source_world_copy_candidate` rows, `20`
  table candidate rows, `4` source-record table rank rows, and zero fatal/error
  rows. Rank `0` is the accepted trace-pose source family reached through the
  `bone_spine` anchor:
  `crowd_area_local_actor_flat_source_x_crowd_male04_bone_R-hand_placement_241`
  with source distance `13.254`, dot `0.990737`, score `3.591`, and
  `owner_ref=crowd_area_local_placement_241`. The logged candidate row copies
  position `(252.908, -218.753, 93.703)` and the same flat X-facing basis seen
  in the explicit source-pose probe.
- Native default-weight sibling probe
  `analysis/native_validation/native_probe_source_record_sibling_default_balcony_20260628_235823/`
  used the same route and retained PS2 rows but default owner weight `1.0`.
  It logged `8` sibling rank rows, `24` sibling candidate rows, and zero
  fatal/error rows. Rank `0` becomes
  `crowd_area_local_actor_flat_source_x_crowd_female01_bone_R-hand_placement_15`
  with owner distance `30.390`, source distance `353.967`, and score
  `119.160`, proving that owner-root proximity alone still pulls the resolver
  to the wrong actor instance.
- Interpretation: the PS2 source-record sibling model can now reach the exact
  native source row matching the retained builder source from the accepted
  `bone_spine1.mesh` record family, but only when source/axis evidence is
  isolated from root-owner proximity. The remaining bridge is the PS2
  owner/member object-context semantics that make owner `0x00cb9530` plus
  `bone_spine1.mesh` imply the crowd_male04 placement/source context. Do not
  promote this diagnostic row until native has a source-backed selector for
  that actor/placement context.

2026-06-29 explicit source-record context diagnostic:
- Native now logs a separate
  `ps2_source_record_context_actor_source_world_copy_candidate` stage. It uses
  the same PS2 source-record sibling resolver as the generic sibling probe, but
  explicitly fixes owner-root weight to `0.0` for the diagnostic call. This
  makes the source/axis context question first-class instead of relying on
  `GHOGX_DEBUG_CAMERA_SOURCE_RECORD_OWNER_WEIGHT=0` in the environment. The
  generic sibling stage remains default-owner-weighted for comparison, and both
  stages remain contract-guarded out of submitted camera rows.
- First watched build
  `analysis/native_validation/ninja_watch_source_record_context_20260629_000346/`
  failed quickly, not by hanging: the call sites passed the new owner-weight
  override before the sibling helper signature had been updated. The fixed
  watched build
  `analysis/native_validation/ninja_watch_source_record_context_fix_20260629_000458/`
  rebuilt `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` in about
  `31s` with a `15s` heartbeat and only the existing gameplay warnings.
  Direct contract
  `analysis/native_validation/ctest_source_record_context_direct_20260629_000540/`
  returned exit code `0`.
- Native `balcony_lft04` context probe
  `analysis/native_validation/native_probe_source_record_context_balcony_20260629_000611/`
  used the accepted retained owner/source/forward rows, no member env var, no
  owner-weight env override, and sibling rank limit `6`. It logged `22`
  `ps2_source_record_context_actor_source_world_copy_candidate` rows, `22`
  generic sibling rows, `12` sibling rank rows, and zero fatal/error rows.
  The explicit context stage consistently selects the accepted source row:
  `crowd_area_local_actor_flat_source_x_crowd_male04_bone_R-hand_placement_241`
  with score `3.591`, owner distance `338.123`, source distance `13.254`, dot
  `0.990737`, anchor member `bone_spine`, and
  `owner_ref=crowd_area_local_placement_241`. In the same run the generic
  sibling stage, with default owner weighting, still selects the wrong nearby
  owner row
  `crowd_area_local_actor_flat_source_x_crowd_female01_bone_R-hand_placement_15`
  with score `119.160`, owner distance `30.390`, and source distance
  `353.967`.
- Interpretation: the explicit context diagnostic gives native a stable,
  source-backed way to reproduce the retained builder-source row from the
  decoded `bone_spine` source-record family without a manual env scoring
  override. It is still not submitted camera behavior: the remaining promotion
  gate is to replace the explicit retained source-position/axis probe with a
  native, PS2-backed context selector that explains why the runtime
  `0x0077c610` owner/member record chooses crowd_male04 placement `241` for
  this shot.

2026-06-29 source-record owner coordinate-family diagnostic:
- Added opt-in owner-family diagnostics to
  `camera_ps2_source_record_sibling_actor_source_world_copy_candidate_rows`.
  `GHOGX_DEBUG_CAMERA_SOURCE_RECORD_OWNER_FAMILY=best` compares
  actor-area-local, generic area-local, actor-world, and generic world owner
  rows in the sibling rank log without submitting any of those rows to the live
  camera. The same pass also publishes actor-specific WorldCrowd placement owner
  targets beside the actor source rows, and strips flat-source axis prefixes
  (`x_`, `y_`, `z_`) before owner lookup so
  `crowd_area_local_actor_flat_source_x_crowd_male04_*` can resolve its
  `crowd_male04` placement owner.
- Watched build
  `analysis/native_validation/ninja_watch_source_record_axis_owner_20260629_003019/`
  rebuilt `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` in about
  `22s` with a `15s` heartbeat and only the existing gameplay warnings. Direct
  contract
  `analysis/native_validation/ctest_source_record_axis_owner_direct_20260629_003050/`
  returned exit code `0`.
- Native `balcony_lft04` probe
  `analysis/native_validation/native_probe_source_record_axis_owner_balcony_proc_20260629_003134/`
  used the retained accepted owner/source/forward probes, sibling rank limit
  `10`, and `GHOGX_DEBUG_CAMERA_SOURCE_RECORD_OWNER_FAMILY=best`. It returned
  exit code `0`, logged no fatal/error rows, and showed the explicit context
  stage still selecting
  `crowd_area_local_actor_flat_source_x_crowd_male04_bone_R-hand_placement_241`
  with score `3.591`, source distance `13.254`, dot `0.990737`, and
  `owner_ref=crowd_crowd_male04_area_local_placement_241`. The selected
  actor-area-local owner distance is `338.123`, the same as generic
  `crowd_area_local_placement_241`, while the actor/world owner distance is
  `1020.994`; for the generic sibling stage, the wrong nearby
  `crowd_female01` placement still wins with owner distance `30.390` and source
  distance `353.967`.
- Interpretation: this closes the coordinate-family question for the accepted
  `balcony_lft04` trace. The retained PS2 owner root is not explained by
  world-space placement rows, and actor-specific area-local owner rows collapse
  to the same position as the generic placement owner. The accepted
 `crowd_male04` selection is therefore carried by source position/axis context,
 not owner-root proximity. Keep the stage diagnostic-only until the explicit
 retained source-position/axis inputs are replaced by a native PS2-backed
 selector from decoded source-record context.

2026-06-29 native source-record context seed falsification:
- Native now has a guarded
  `ps2_source_record_native_context_actor_source_world_copy_candidate`
  diagnostic that feeds the source-record sibling resolver from the live
  `source_seed_a.position/source_seed_a.forward` and
  `source_seed_b.position/source_seed_b.forward` rows instead of the explicit
  retained `GHOGX_DEBUG_CAMERA_SOURCE_PROBE` and
  `GHOGX_DEBUG_CAMERA_SOURCE_PROBE_FORWARD` values. The older explicit context
  diagnostic now only runs when both retained source-position and source-axis
  probes are present, preventing zero-score junk rows when the env context is
  intentionally absent. Both stages remain contract-guarded out of submitted
  camera rows.
- Watched build
  `analysis/native_validation/ninja_watch_source_record_native_context_guard_20260629_004117/`
  rebuilt `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` in about
  `20s` with a `15s` heartbeat and only the existing gameplay warnings. Direct
  contract
  `analysis/native_validation/ctest_source_record_native_context_guard_direct_20260629_004147/`
  returned exit code `0`.
- Native `balcony_lft04` probe
  `analysis/native_validation/native_probe_source_record_native_context_guard_balcony_proc_20260629_004215/`
  omitted the explicit source-position/forward env probes and kept only the
  retained owner probe plus source-record table context. It returned exit code
  `0`, emitted zero fatal/error rows, and confirmed the explicit retained
  context stage logged no rows. The new native-context stage instead selected
  `crowd_area_local_actor_anim_flat_source_y_crowd_male03_bone_R-ankle_placement_434`
  at frame `487.50`, with source distance `265.343`, dot `0.967682`, and
  result position `(-286.589, -479.435, 52.375)`. The same frame's
  `source_seed_candidate` was `generated_source_seed` at
  `(-278.525, -710.459, -80.854)` with forward
  `(0.264153, 0.940488, -0.213788)`.
- Interpretation: the native generated source seed is not the PS2 runtime
  source-object context for this accepted trace. It drives the resolver toward
  male03/male02 ankle rows, not the accepted
  `crowd_area_local_actor_flat_source_x_crowd_male04_bone_R-hand_placement_241`
  row. Do not promote source-seed-derived source-record context. The next
  promotion gate remains the decoded PS2 runtime source-object evaluator/list
  path around `0x0077c610`, helper `0x00261c58`, owner `0x00cb9530`, and the
  retained `bone_spine1.mesh` member relationship.

2026-06-29 retained trace-context source-record diagnostic:
- Native now has a separate retained-trace diagnostic stage,
  `ps2_source_record_trace_context_actor_source_world_copy_candidate`, that
  derives the source-record context from documented PS2 trace evidence instead
  of env source-position/source-axis probes or the native generated source
  seed. For `source_ref=crowd` / `category=balcony_lft`, the context cites
  `gh2dxu_arena_balcony_lft04_source_trace_ghdxelf_20260628` and feeds the
  existing source-record sibling resolver with the retained
  `0x0077c610+0x80` record shape: owner `0x00cb9530`, member
  `bone_spine1.mesh`, owner root near `(92.976, 86.079, 21.451)`, builder
  source `(259.235, -229.846, 97.250)`, and builder forward
  `(0.877757, 0.477847, 0.000558)`. This row is diagnostic-only and remains
  contract-guarded out of submitted camera rows.
- Watched build
  `analysis/native_validation/ninja_watch_trace_context_20260629_005053/`
  rebuilt `ghogx_gameplay_venue_band_contract_test` and `ghogx_app`; Ninja
  linked both targets in about `20s` with a `15s` heartbeat and only the
  existing gameplay warnings. The wrapper failed after the process had exited
  because PowerShell could not subtract the cached process start time, so use
  the build logs rather than the wrapper exit as evidence. Direct contract
  `analysis/native_validation/ctest_trace_context_direct_20260629_005150/`
  passed the focused `ghogx_gameplay_venue_band_contract_test`.
- Native clean probe
  `analysis/native_validation/native_probe_trace_context_balcony_clean_20260629_005650/`
  forced stock PS2 `shoutatthedevil` in `arena` from `16.0s` with
  `--diagnostic-camera-shot balcony_lft04`,
  `--diagnostic-camera-path-offset 255`, fixed `0.25s` steps, no
  `GHOGX_DEBUG_CAMERA_SOURCE_PROBE`, no
  `GHOGX_DEBUG_CAMERA_SOURCE_PROBE_FORWARD`, no
  `GHOGX_DEBUG_CAMERA_PATH_SOURCE_PROBE`, and no source-record owner/member
  env probes. It exited `0` in about `26s` and logged no fatal/error markers.
  The trace-context stage selected
  `crowd_area_local_actor_flat_source_x_crowd_male04_bone_R-hand_placement_241`
  from frame `487.50` onward with score `3.609`, source distance `13.254`,
  dot `0.990140`, result position `(252.908, -218.753, 93.703)`, and the
  flat X-facing basis. A longer ranked probe
  `analysis/native_validation/native_probe_trace_context_balcony_stock_20260629_005427/`
  was intentionally killed by the `100s` watchdog because debug output was
  large, but before the kill it logged the same rank `0` and showed the next
  candidates in the expected male04/male03 source neighborhood.
- Interpretation: replacing env source-position/source-axis probes with a
  retained PS2 trace-context table reproduces the accepted native source row
  from the documented `0x0077c610` owner/member record. This is still not a
  submitted camera promotion, because the context is a retained trace oracle
  for one accepted balcony source family rather than a decoded runtime source
  evaluator for all shots. The next gate is to generalize that retained
  context into a native source-object evaluator/list traversal matching the
  PS2 `0x00261c58 -> 0x003d7220` owner/member child resolution.

2026-06-29 retained source-record table diagnostic refactor:
- The retained trace-context diagnostic is now table-driven through
  `kRetainedPs2SourceRecordTraceTable` instead of embedding the accepted
  coordinates directly in `ps2_source_record_trace_context_for_key()`. The
  table preserves the accepted PS2 record identity (`record=0x0077c690`,
  `offset=0x0077c610+0x80`), shot/category object (`0x00caa3e0`), helper
  owner (`0x00cb9530`), member symbol (`bone_spine1.mesh`), owner root,
  builder source position, builder forward, and source trace artifact. The
  camera debug path now evaluates that table with the shared
  `camera_ps2_source_record_trace_context_actor_source_world_copy_candidate_rows()`
  wrapper, which still routes through the source-record sibling resolver and
  still stays contract-guarded out of submitted camera rows.
- Watched build
  `analysis/native_validation/ninja_watch_trace_table_20260629_010124/`
  rebuilt `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` in about
  `20s` with a `15s` heartbeat and only the existing gameplay warnings.
  Focused contract
  `analysis/native_validation/ctest_trace_table_direct_20260629_010159/`
  passed `ghogx_gameplay_venue_band_contract_test`.
- Native clean probe
  `analysis/native_validation/native_probe_trace_table_balcony_clean_20260629_010224/`
  used the same bounded stock PS2 `shoutatthedevil` / `arena` /
  `balcony_lft04` route as the prior trace-context run, with path offset
  `255`, fixed `0.25s` steps, no source-position/source-forward env probes,
  and no source-record owner/member env probes. It exited `0` in about `24s`
  and logged no fatal/error markers. The trace-table stage selected
  `crowd_area_local_actor_flat_source_x_crowd_male04_bone_R-hand_placement_241`
  for frames `487.50` through `540.00`, carrying provenance
  `record=0x0077c690 offset=0x0077c610+0x80 shot=0x00caa3e0
  owner=0x00cb9530 member=bone_spine1.mesh`, score `3.609`, source distance
  `13.254`, dot `0.990140`, and position `(252.908, -218.753, 93.703)`.
- Interpretation: this is a cleaner accepted-trace evaluator surface, not a
  visual fix. The useful next step is to add more retained source-record table
  entries only when the trace evidence names their active source rows, then
  replace table-oracle selection with native decoding of the equivalent
  source-object list and owner/member child transform. Submitted camera
  behavior should remain unchanged until that generic evaluator is proven
  across more than this retained balcony source family.

2026-06-29 retained source-record helper-evaluation layer:
- The retained table path now has an explicit `Ps2SourceRecordEvaluation`
  layer between the trace record and native WorldCrowd sibling resolver.
  `evaluate_retained_ps2_source_record_trace_context()` models the accepted
  PS2 helper output as owner position, evaluated source position, evaluated
  source forward, and member symbols, and labels the provenance with
  `eval=0x00261c58->0x003d7220`. This does not add rendered camera behavior;
  it separates the retained record table from the evaluated helper output so
  the next native implementation can replace only the evaluator, not the
  downstream sibling resolver or submitted camera path.
- Watched build
  `analysis/native_validation/ninja_watch_source_record_eval_20260629_010555/`
  rebuilt `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` in about
  `20s` with a `15s` heartbeat and only the existing gameplay warnings.
  Focused contract
  `analysis/native_validation/ctest_source_record_eval_direct_20260629_010631/`
  passed `ghogx_gameplay_venue_band_contract_test`.
- Native clean probe
  `analysis/native_validation/native_probe_source_record_eval_balcony_clean_20260629_010655/`
  used stock PS2 `shoutatthedevil` / `arena` / `balcony_lft04`, path offset
  `255`, fixed `0.25s` steps, no source-position/source-forward env probes,
  and no source-record owner/member env probes. It exited `0` in about `26s`
  and logged no fatal/error markers. The trace-context row now carries
  `record=0x0077c690 offset=0x0077c610+0x80 shot=0x00caa3e0
  owner=0x00cb9530 member=bone_spine1.mesh eval=0x00261c58->0x003d7220`
  and still selects
  `crowd_area_local_actor_flat_source_x_crowd_male04_bone_R-hand_placement_241`
  with score `3.609`, source distance `13.254`, dot `0.990140`, and position
  `(252.908, -218.753, 93.703)` for frames `487.50` through `540.00`.
- Interpretation: native now has a clean diagnostic boundary matching the PS2
  record-list/evaluator/resolver shape: retained table record -> helper
  evaluation -> native source-family resolver. The remaining promotion gate is
  to replace the retained helper-evaluation oracle with native decoding or
  derivation of the equivalent owner/member child world rows across source
  records, then validate that against accepted trace rows before submitting
  camera behavior.

2026-06-29 native owner/member-only source-record comparison:
- Added a separate diagnostic stage,
  `ps2_source_record_native_owner_member_eval_world_copy_candidate`, that uses
  the retained PS2 record's owner/member evidence but deliberately omits the
  retained builder source position and forward row. It feeds only
  `owner=0x00cb9530` root position plus `bone_spine1.mesh` through the native
  source-record member resolver and labels the result
  `native_source=owner_member_only`. This is the direct comparison for whether
  native can already replace the retained `0x00261c58 -> 0x003d7220` helper
  output without the accepted source-row oracle. The stage is contract-guarded
  out of submitted camera rows.
- Watched build
  `analysis/native_validation/ninja_watch_owner_member_eval_20260629_011036/`
  rebuilt `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` in about
  `20s` with a `15s` heartbeat and only the existing gameplay warnings.
  Focused contract
  `analysis/native_validation/ctest_owner_member_eval_direct_20260629_011115/`
  passed `ghogx_gameplay_venue_band_contract_test`.
- Native clean probe
  `analysis/native_validation/native_probe_owner_member_eval_balcony_clean_20260629_011145/`
  used the same bounded stock PS2 `shoutatthedevil` / `arena` /
  `balcony_lft04` route with path offset `255`, fixed `0.25s` steps, no
  source-position/source-forward env probes, and no source-record owner/member
  env probes. It exited `0` in about `26s` and logged no fatal/error markers.
  The retained trace-context row still selects the accepted
  `crowd_area_local_actor_flat_source_x_crowd_male04_bone_R-hand_placement_241`
  at `(252.908, -218.753, 93.703)`, score `3.609`, source distance `13.254`,
  dot `0.990140`. The native owner/member-only row instead selects
  `crowd_area_local_actor_source_crowd_female01_bone_spine_placement_15` at
  `(106.508, 94.772, 86.972)`, score `30.364`, owner distance `30.363`, and
  `owner_ref=crowd_crowd_female01_area_local_placement_15`.
- Interpretation: native owner-root/member lookup alone is not the PS2 helper
  evaluator. It locks onto the nearest decoded crowd owner and misses the
  accepted male04 source family entirely. The next implementation target is
  the missing source-record/object-context mapping that makes PS2 owner
  `0x00cb9530` plus `bone_spine1.mesh` evaluate to the male04 placement/source
  context for this record, rather than native nearest-owner root selection.

2026-06-29 retained source-record seed promotion:
- Native now has a retained-trace source-seed bridge for the accepted
  `balcony_lft04` source-record family. `apply_camera_keys()` asks
  `camera_ps2_source_record_trace_context_source_seed_rows()` for a source row
  before falling back to the decoded/generated source seed. The promoted row
  still goes through the shared target-list, shot-filter, and screen-offset
  builder path; it does not add a shot-name camera offset or final-view hack.
- Validation:
  `analysis/native_validation/cmake_watch_trace_source_seed_20260629_013941/`
  rebuilt the edited objects and app, the confirmation pass
  `analysis/native_validation/cmake_job_trace_source_seed_20260629_014040/`
  reported `ninja: no work to do`, and
  `ghogx_gameplay_venue_band_contract_test` exits `0`. Native probe
  `analysis/native_validation/native_probe_trace_source_seed_balcony_20260629_014230/`
  runs stock PS2 `GEN` assets for `shoutatthedevil` / `arena` /
  `balcony_lft04`, path offset `255`, fixed `0.25s` steps, hidden D3D capture,
  and screenshots at frames `1/5/11`. It exits `0`.
- Log result: submitted camera rows now start from
  `ps2_source_record_trace_context_source_seed(...)` and resolve through
  `crowd_area_local_actor_flat_source_x_crowd_male04_bone_R-hand_placement_241`
  with retained provenance
  `record=0x0077c690 offset=0x0077c610+0x80 shot=0x00caa3e0
  owner=0x00cb9530 member=bone_spine1.mesh context=0x00828720
  locale=0x0055a1db tag=0x00010010 eval=0x00261c58->0x003d7220`.
  At frame `570.00` the submitted row is
  `position=(252.908, -218.753, 93.703)` before target-list aim, which is now
  in the accepted PS2 builder-source neighborhood
  `(259.235, -229.846, 97.250)`.
- Visual result: frames `frame_00001.bmp`, `frame_00005.bmp`, and
  `frame_00011.bmp` are active arena captures but the camera is still visually
  wrong: it now sits near the retained source row and looks through foreground
  truss/sign geometry. This narrows the next gate. Source-record row selection
  is no longer the immediate mismatch for this route; the remaining proof must
  compare PS2 target/source coordinate-space composition, foreground occluder
  behavior, or the exact post-source builder inputs for the accepted frame.

2026-06-29 retained result-builder a2 vector gate:
- Re-read the existing accepted trace
  `GuitarHeroOGX-trace360/analysis/ps2_trace/gh2dxu_arena_balcony_lft04_source_trace_ghdxelf_20260628.json`
  instead of reopening PCSX2. It already contains sampled
  `cam_result_builder_00267008:a2` rows for the exact accepted
  `a0=0x0077c610` builder-source family.
- Exact accepted row: `a1=0x00cc0590` is the source matrix with position
  `(259.2345, -229.8460, 97.2500)`, forward
  `(0.877757, 0.477847, 0.000558)`, and up
  `(-0.083151, 0.151590, 0.984328)`. The paired `a2=0x00cc0720` result row
  starts with the same source position and carries a final unit vector at words
  `28..30`: `(-0.471125, 0.865611, -0.173105)`, with distance word `31`
  `327.9234`.
- Native frame `570.00` after retained source-seed promotion submits position
  `(252.9077, -218.7531, 93.7034)` but final forward
  `(-0.714958, 0.674790, -0.183013)` because the shared target-list solve still
  aims at the native `guitarist0` centroid / filtered target near
  `(1.355, -1.867, 36.204)`. This is the concrete composition mismatch that
  explains the foreground-truss visual: native is no longer primarily missing
  the retained source object; it is missing the accepted `0x00267008:a2`
  result-vector/target solve.
- Native now logs a diagnostic-only
  `ps2_result_builder_a2_vector_candidate(...)` row from the retained trace
  fields when `GHOGX_DEBUG_CAMERA=1`. It is contract-guarded out of
  `camera_submitted_result_rows_for_key`, so submitted camera behavior still
  changes only through the source-seed bridge until the generic result-vector
  solve is implemented.
- Validation:
  `analysis/native_validation/build_watch_result_a2_vector_20260629_020154/`
  rebuilt the edited gameplay object, contract object, contract executable, and
  `ghogx_app` in about `22s` with a `15s` heartbeat; the focused
  `ghogx_gameplay_venue_band_contract_test` exits `0`. Native probe
  `analysis/native_validation/native_probe_result_a2_vector_20260629_020253/`
  ran the same stock arena `balcony_lft04` / path offset `255` route for
  `12` fixed frames with a `90s` watchdog and saved `frame_00011.bmp`. The log
  proves the retained `a2` candidate in the live native path at frame `570.00`:
  diagnostic position `(259.234528, -229.846008, 97.250031)` and forward
  `(-0.470841, 0.865089, -0.173001)` versus submitted position
  `(252.907715, -218.753113, 93.703407)` and forward
  `(-0.714958, 0.674790, -0.183013)`.

2026-06-29 retained result-vector runtime bridge (superseded):
- Native temporarily tested a retained-trace `0x00267008:a2` runtime branch in
  `apply_camera_keys()` after the shared source-seed resolver picked the
  source row. The later `2026-06-29 retained a2 vector runtime correction`
  entry supersedes this branch: existing trace evidence proves the sampled
  `a2[28..30]` row matches the source matrix second basis row, not the final
  render-camera forward row.
- The bridge deliberately preserves the live native source position selected by
  the source-record resolver, then replaces the incorrect native
  performer-centroid aim with the retained PS2 final result vector. For the
  accepted arena `balcony_lft` source family this means the runtime should
  submit a forward row near `(-0.471, 0.865, -0.173)` instead of continuing to
  aim at the native `guitarist0` centroid near `(-0.715, 0.675, -0.183)`.
  Remaining work is to generalize the `0x00267008:a2` target/result solve from
  more retained rows instead of relying on this single accepted family.
- Validation:
  `analysis/native_validation/build_watch_result_a2_runtime_20260629_020732/`
  rebuilt the app and focused contract in about `20s` with a heartbeat, and the
  focused `ghogx_gameplay_venue_band_contract_test` exits `0`. Native probe
  `analysis/native_validation/native_probe_result_a2_runtime_20260629_020823/`
  ran the same stock arena `balcony_lft04` route for `12` fixed frames with a
  `90s` watchdog and saved frames `1/5/11` plus
  `frames/result_a2_runtime_contact_sheet.png`. Frame `570.00` now proves
  `ps2_a2_vector_branch=1`: submitted rows kept the live source-record position
  `(252.907715, -218.753113, 93.703407)` and submitted forward
  `(-0.470841, 0.865089, -0.173001)`. The contact sheet stayed
  foreground-truss/sign occluded, and the branch is now removed because the
  row was later proven to be source/projection-side evidence rather than the
  render-camera forward.

2026-06-29 path-backed CamShot clip recovery:
- Native `balcony_lft04` was falling back to `clip=(1.000 6000.000)` even
  though the same path-backed CamShot raw body carries the normal balcony clip
  fields. The raw extracted `CamShot__balcony_lft04` body places the packed
  shot-field block before the inserted `Camera03.tnm` string: with category
  length at `0x23c`, clamp is at `0x213`, near/far are `0x217/0x21b`
  (`50/3000`), selection is `0.3`, and path ease is `-1`. Sibling path shots
  show the same shape with the block shifted by the packed path string length.
- `decode_camshot_category_tail_fields()` now scans backward from the category
  symbol for a plausible packed shot-field block and accepts an intervening
  packed path string gap. This restores clamp, near/far, depth-of-field,
  selection, and path-ease fields for TransAnim-backed CamShots without keying
  off `balcony_lft04`, `Camera03`, or a mesh name.
- Validation:
  `analysis/native_validation/build_watch_path_clip_20260629_021820/` rebuilt
  `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` in about `20s`
  with a `15s` heartbeat; the focused contract exits `0`. Native probe
  `analysis/native_validation/native_probe_path_clip_20260629_021911/` ran
  the same stock arena `balcony_lft04` / path offset `255` route for `12`
  fixed frames under a `90s` watchdog and saved frames `1/5/11`. The log now
  proves `clip=(50.000 3000.000)`, `selection=0.300`, `path_ease=-1.000`, and
  `shot_fields=a:1 b:1` for frame `570.00`.
- Visual result: the foreground reaper/truss/sign composition remains. The
  clip-plane fix is correct and necessary, but it falsifies near/far fallback
  as the immediate cause of the accepted PS2 mismatch.
- Follow-up evidence check: the accepted trace's `a2=0x00cc0720` summary block
  carries the source position in words `0..2`, projection/frustum-like fields
  through the middle of the block, the intermediate vector/distance at
  words `28..31`, and a later `36..47` block with translation-like W lanes
  `(66.849, 271.075, 170.411)`. That later block is not orthonormal, so it
  should not be promoted as a render-camera basis without a matching writer or
  path handoff trace. This accepted trace has `516`
  `cam_result_builder_00267008` calls and `2` `cam_path_apply_0026ae00` calls,
  but no `cam_result_writer_002665a0` records. The next gate is therefore a
  deliberately narrow trace or native diagnostic that captures the actual
  writer/result-frame handoff for this same mismatch, not a native hide-list or
  prop-specific workaround.
- Rejected trace attempt:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/gh2dxu_arena_writer_result_handoff_20260629_022619.json`
  launched the bounded call-ring tracer with targets `0x002665a0`,
  `0x00267008`, and `0x0026ae00`, plus fixed samples for `0x00b92ef0`,
  `0x00cc0720`, and `0x00cc0590`. It completed in about `22s` under a
  watchdog, but all three call counts were zero. The retained screenshot shows
  the emulator landed in a different in-song venue/state than the accepted
  arena `balcony_lft04` route, so this is not evidence that the arena writer
  path is inactive. The next PS2 trace should first restore or navigate to the
  accepted arena state/window, then apply the writer/result-frame targets.

2026-06-29 retained a2 vector runtime correction:
- Re-opened existing traces before changing native again. In both
  `gh2dxu_arena_balcony_lft04_source_trace_ghdxelf_20260628.json` and
  `gh2dxu_arena_result_builder_a2_follow_20260629_014856.json`,
  `cam_result_builder_00267008:a2` words `28..30` match the source matrix
  second basis row from `a1` words `4..6` within about `0.0012`. For the
  balcony source trace that is
  `a2[28..30]=(-0.471125, 0.865611, -0.173105)`, matching
  `a1[4..6]=(-0.470557, 0.864568, -0.172896)`. For the follow trace it is
  `a2[28..30]=(0.269436, 0.955548, -0.124655)`, matching
  `a1[4..6]=(0.269111, 0.954397, -0.124505)`.
- Therefore the earlier retained `a2` runtime branch was mislabelled: it was
  consuming a source/projection-side vector as if it were the submitted
  render-camera forward. Native now keeps
  `ps2_result_builder_a2_vector_candidate(...)` as diagnostic evidence only
  and removes `camera_ps2_result_builder_a2_vector_rows_from_seed()` plus the
  `ps2_a2_vector_branch` submission/log flag. This is a corrective fidelity
  change, not a visual workaround.
- The next accepted-camera gate remains the true writer/result-frame handoff
  for the same arena `balcony_lft04` mismatch. Existing `a2_follow` evidence
  has writer/path calls and proves row shape, but its retained screenshot is a
  normal highway/band frame rather than the dark accepted balcony/crowd frame,
  so do not use its camera values as the balcony final camera.

2026-06-29 writer payload handoff diagnostic:
- Reopened PCSX2 for the specific native camera mismatch and reran the GHDX
  statefile route with the GH2DXu ELF:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/gh2dxu_arena_writer_handoff_statefile_20260629_025058.json`.
  The run completed under a watchdog in about `24s` and recorded `1250`
  chronological camera calls: `254` `cam_path_apply_0026ae00`, `764`
  `cam_result_builder_00267008`, and `232`
  `cam_result_writer_002665a0`.
- The new trace captures the missing handoff shape for the retained
  `0x0077c610` source-record family. Each writer call in the sampled window is
  preceded by two builder calls:
  `0x00267008(a0=0x0077c610,a1=0x00cc0590,a2=0x00cc0720)` and
  `0x00267008(a0=0x008269f0,a1=0x00cc0590,a2=0x00cc0720)`, followed by
  `0x002665a0(a0=0x014fd344,a1=0x014fd454,a2=0x008269f0,a3=0x0077c610)`.
  That proves the writer consumes the two builder source/result rows and writes
  two output payload objects.
- The writer output object at `0x014fd344` carries a full transform payload at
  offset `+0xac`: forward `(0.877965,0.477960,0.000558)`, right
  `(-0.472734,0.868538,-0.148279)`, up
  `(-0.071399,0.129998,0.988622)`, and position
  `(258.577881,-228.578201,105.311722)`. This is close to, but not identical
  with, the retained accepted source row. It indicates a missing writer-frame
  payload stage after the builder, rather than a clip-plane or hide-list issue.
- The retained screenshot from this trace is active gameplay but not the clean
  dark accepted `balcony_lft04` / crowd-facing view, so native now exposes this
  as `ps2_writer_payload_candidate(...)` diagnostics only. It is contract-guarded
  out of submitted camera rows; do not promote it until the generic writer
  payload solve is mapped or the same accepted balcony state captures matching
  writer output.

2026-06-29 accepted source-record result-frame submission:
- Reran the same specific camera mismatch route with wider post-trace samples:
  `GuitarHeroOGX-trace360/analysis/ps2_trace/gh2dxu_arena_writer_handoff_wide_samples_20260629_031220.json`.
  The watched trace completed in about `24s` and retained `1163` camera calls:
  `723` `cam_result_builder_00267008` and `301`
  `cam_result_writer_002665a0`.
- The widened sampled writer objects confirm the writer payload layout at
  object offset `+0xac`: forward row, right row, up row, then position row,
  each as four floats. The retained screenshot is still a gameplay/highway view,
  not the accepted dark balcony/crowd frame, so these widened writer values
  remain layout evidence, not the balcony visual oracle.
- Native validation
  `analysis/native_validation/native_probe_camera_targetlist_current_20260629_032120`
  proved the current mismatch at frame `570.00`: `source_seed_candidate` from
  `gh2dxu_arena_balcony_lft04_source_trace_ghdxelf_20260628` resolved to
  position `(252.907715,-218.753113,93.703407)` and forward
  `(0.935079,0.354440,0.000000)`, but `submitted` then appended
  `+target_list+shot_filter+screen` and re-aimed to forward
  `(-0.714958,0.674790,-0.183013)`, matching the obstructed native capture.
- The native runtime now submits a retained accepted PS2 source-record result
  frame through `ps2_source_record_trace_result_frame(...)` and skips the native
  target-list/shot-filter re-aim for that trace-submitted path. This uses the
  accepted builder source row (`0x00267008:a1`) from the dark balcony trace and
  keeps the writer payload diagnostic-only until an accepted-state writer trace
  is available.

2026-06-29 WorldCrowd render-source basis cleanup:
- Native character compositing now uses the same result-frame camera/aspect/
  screen-offset family as venue geometry. That corrected projection revealed
  the previous raw WorldCrowd render placement was only visually plausible under
  the wrong character camera: raw placement removes 3D actors from the accepted
  balcony foreground, while area-local placement matches the accepted
  source-target family but places source actors directly around the camera.
- The decoded `WorldCrowd` actor rows in `arena_chars.milo_ps2` carry three
  floats per actor, consistently `(95.0, 1.0, 10.0)`. Native now keeps the
  source-backed area-local actor basis as the default render basis, keeps raw
  placement as `GHOGX_WORLDCROWD_RENDER_BASIS=placement` diagnostics, applies
  clip-named hand/prop mesh visibility before renderer upload, and uses the
  small third decoded actor float plus the decoded visible mesh bounds as a
  generic near-source render cull radius. The larger first float culled a broad
  foreground ring in the accepted camera-in-crowd balcony frame; the small field
  still avoids exact near-plane intersections without erasing the silhouettes
  PS2 keeps around the camera.

2026-06-29 WorldCrowd DTA fullness/play_group runtime:
- The `world/crowd.dta` script is the accepted source for live crowd density
 and clip families. `crowd_update` maps `kExcitementBoot` to
  `set_fullness 0.1 0.1`, `kExcitementBad` to `0.25 0.25`,
  `kExcitementOkay` to `0.5 0.5`, and `kExcitementGreat`/`Peak` to the peak
  fullness. It also drives `main.drv play_group bad/ok/great/idle/lighter_*`
  rather than a single idle clip.
- Native WorldCrowd runtime now resolves those `CharClipGroup` names from the
  authored crowd `main.drv` MILOs, falls back only to actor-family clip names,
  switches group when `active_venue_event_` changes, and applies the fullness
  fraction through a camera-near per-actor placement selection. This keeps the
  balcony trace route source-backed: the current `excitement_bad` frame no
  longer promotes every decoded placement into a full 3D draw, but preserves
  the near silhouettes visible in the accepted PS2 frame.

2026-06-29 post-placement balcony camera diagnostic:
- After fixing performer start placement to use the moved `venue_chars_scene_`
  and rejecting non-waypoint string bytes as flags, the stock arena
  `balcony_lft04` diagnostic at path offset `255` still renders the native
  default as a high stage/rigging view. The logs show the source seed is the
  retained trace-context WorldCrowd actor source, but the submitted row still
  appends `+target_list+shot_filter+screen` and aims at the entity-only
  `guitarist0:` target ref. The clumping regression is fixed; the accepted
  balcony mismatch remains a camera source/result composition issue.
- Two bounded comparison renders keep the retained trace rows diagnostic-only:
  `analysis/native_validation/native_probe_balcony_a2_decoderfix_20260629_095945/`
  renders `GHOGX_DEBUG_CAMERA_SUBMIT_CANDIDATE=a2` and points into overhead
  crowd/rigging, while
  `analysis/native_validation/native_probe_balcony_writer_decoderfix_20260629_095945/`
  renders `writer` as a side crowd/suite composition. Neither matches the
  accepted dark crowd-facing PS2 frame, so the contract rule remains correct:
  do not silently promote retained `a1`/`a2`/writer rows into gameplay. The next
  implementation-grade route is still the generic PS2 source-object evaluator
  bridge, not shot-name selection, performer movement, hide-list tweaks, or
  retained-trace camera substitution.

2026-06-29 accepted builder projection diagnostic:
- Added an explicit diagnostic-only `GHOGX_DEBUG_CAMERA_SUBMIT_CANDIDATE=ps2proj`
  route for the accepted `0x00267008:a2+0x90` payload from
  `gh2dxu_arena_balcony_lft04_source_trace_ghdxelf_20260628`. The retained
  payload is stored as source/result evidence with `fov_y=1.047198`,
  `near=10`, and `far=10000`; the default native camera path is unchanged and
  retained rows still require the explicit debug selector.
- Watched build `analysis/native_validation/build_watch_ps2proj_20260629/`
  rebuilt `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` in about
  `20s` after rejecting stale/nonconfigured build directories. Focused contract
  `analysis/native_validation/ctest_ps2proj_20260629/` passed.
- Native diagnostic
  `analysis/native_validation/native_probe_ps2proj_ark_quoted2_20260629/`
  ran stock PS2 `shoutatthedevil` / `arena` / `balcony_lft04`, path offset
  `255`, fixed `0.25s` steps, hidden capture, and screenshots at
  frames `1/5/11`; the retained visual evidence was trimmed to
  `frame_00011.png`. The log proves submitted
  `ps2_result_builder_projection_candidate(...)`, matrix diagnostic
  `fov=1.047198`, `clip=(10,10000)`, and eye/source
  `(259.234528,-229.846008,97.250031)`.
- Visual result: the screenshot still shows a side-suite/crowd composition,
  not the accepted dark crowd-facing PS2 frame. Interpretation: the accepted
 `a2+0x90` payload is not equivalent to decomposing eye/forward/up plus a
 D3D-style perspective matrix. The next camera parity step is to model the
 full PS2 builder matrix/screen solve, or capture the final accepted camera
 matrix handoff, rather than promoting the decomposed retained pose.

2026-06-29 accepted builder matrix diagnostic:
- Added explicit diagnostic-only matrix submit routes for the same accepted
  `0x00267008:a2+0x90` payload. `GHOGX_DEBUG_CAMERA_SUBMIT_CANDIDATE=ps2matrix`
  maps the three PS2 row blocks into D3D view columns; `ps2matrix_rows` keeps
  the sampled row layout direct. Both routes keep the default native camera
  path unchanged, retain trace provenance in `ps2_result_builder_matrix_candidate`,
  and require the explicit debug selector.
- Watched build `analysis/native_validation/build_watch_ps2matrix_20260629/`
  completed in about `30s`; focused contract
  `analysis/native_validation/ctest_ps2matrix_20260629/` passed. Native probes
  `analysis/native_validation/native_probe_ps2matrix_20260629/` and
  `analysis/native_validation/native_probe_ps2matrix_rows_20260629/` both ran
  with `90s` watchdogs and exited `0`, preserving only `frame_00011.png` plus
  stdout/stderr logs after cleanup.
- Visual result: neither matrix interpretation matches the accepted dark
  crowd-facing PS2 frame. The column-mapped variant still shows the wrong
  balcony/near-crowd family with a severe skew, and the direct-row variant
  still presents the same foreground crowd/venue composition. Interpretation:
 the accepted `a2+0x90` block is not a directly submitted D3D-style view
 matrix. The next camera-first gate is a precise PS2 writer/final-screen
 handoff trace for this exact accepted balcony frame, or a documented native
 implementation of the missing PS2 builder screen solve before any default
 camera promotion.

2026-06-29 camera trace acceptance gate:
- Added `analysis/pcsx2_trace/validate_camera_trace_acceptance.py` so PS2
  camera traces cannot be promoted merely because they recorded function calls.
  The gate compares the retained trace screenshot to the accepted dark
  `balcony_lft04` frame with normalized RMS, requires the accepted
  `0x0077c610` builder-source family, and can additionally require writer
  handoff evidence after that builder source.
- Existing trace audit:
  `gh2dxu_arena_balcony_lft04_source_trace_ghdxelf_20260628.json` passes the
  visual/source portion (`rms=0.00`, `516` builder calls), but fails the writer
  completion gate because it has zero `cam_result_writer_002665a0` calls. The
  writer traces
  `gh2dxu_arena_writer_handoff_statefile_20260629_025058.json` and
  `gh2dxu_arena_writer_handoff_wide_samples_20260629_031220.json` fail the
  visual gate (`rms=45.98` and `57.21`), so their writer rows remain layout
  evidence only.
- A bounded rerun with `0x00266f80` child-output tracing captured child-output
  calls, but the screenshot was a gameplay/highway frame (`rms=58.95`) and the
  source family was `0x0077bfa0`, not the accepted `0x0077c610`. The rejected
  artifacts were deleted after inspection to avoid accumulating bad traces.
 Do not use that child-output run for native camera behavior; first stabilize
 the accepted visual route or capture the final handoff from the already
 accepted dark frame.

2026-06-29 camera-system evidence reclassification:
- User correction accepted: camera tracing should first learn the PS2 camera
  system across valid in-song camera moments, then use exact screenshot parity
  as a later native proof gate. The previous one-shot balcony framing was too
  narrow for discovery.
- Added `analysis/pcsx2_trace/analyze_camera_trace_system.py` as the broader
  discovery classifier. `validate_camera_trace_acceptance.py` now documents
  itself as a shot-parity promotion gate, not the only way for a trace to be
  accepted as camera evidence.
- Reclassified retained traces with:
  `python analysis\pcsx2_trace\analyze_camera_trace_system.py --writer-source 0x0077c610 ...`.
  Results:
  `arena_camera_symbol_bridge_current/camera_call_sequence_basefix.json` is
  `builder_child_shape` evidence: `365` apply-child, `730` result-builder,
  `366` result-list-check, and one child-resolve call. It teaches child/list
  shape even though it is not a final writer handoff trace.
  `gh2dxu_arena_balcony_lft04_source_trace_ghdxelf_20260628.json` is
  `source_path_builder` evidence: `2` path-apply and `516` result-builder
  calls, including the `0x0077c610` source family. It remains the dark balcony
  shot proof for source/path rows but not writer output.
  `gh2dxu_arena_writer_handoff_statefile_20260629_025058.json` is
  `system_handoff_with_payload` evidence: `254` path-apply, `764`
  result-builder, `232` result-writer, writer-after-`0x0077c610=true`, and
  one repeated writer tuple
  `a0=0x014fd344 a1=0x014fd454 a2=0x008269f0 a3=0x0077c610`.
  Its common local order is path-apply -> builder -> builder -> writer. The
  analyzer decodes the sampled `a0+0xac` payload as a clean camera transform:
  forward `(0.877965,0.477960,0.000558)`, up
  `(-0.071399,0.129998,0.988622)`, position
  `(258.577881,-228.578201,105.311722)`. The sampled `a1+0xac` block is
  flagged `basis_ok=false`, so do not assume every writer register has the same
  final-camera payload shape. This trace is valid system evidence for the
  generic builder/writer bridge even though its screenshot still fails the
  single balcony parity gate.
  `gh2dxu_arena_writer_handoff_wide_samples_20260629_031220.json` is also
  `system_handoff_with_payload` evidence for another in-song route:
  `723` result-builder, `301` result-writer, repeated writer tuple
  `a0=0x014e4cc4 a1=0x014e4dd4 a2=0x00827850 a3=0x0077c2e0`.
  Both sampled writer payloads in this route pass the transform quality check:
  `a0+0xac` forward `(0.894076,-0.445668,-0.044815)`, up
  `(-0.035032,-0.169323,0.984938)`, position
  `(13.393906,-75.063202,-29.267998)`, and `a1+0xac` forward
  `(0.930294,-0.364182,-0.035243)`, up
  `(0.001770,-0.091759,0.995475)`, position
  `(25.969772,-70.096939,-19.551628)`.
- Implementation implication: the next camera work should consume these traces
  to model the general PS2 path/source -> result-builder pair -> writer payload
  chain. Do not promote any one retained payload as the final native camera for
  a particular screenshot, but do use the writer traces as first-class evidence
  for pipeline order, argument roles, output object layout, and payload sampling
  offsets.

2026-06-29 generic PS2 writer bridge candidate:
- Implemented `camera_ps2_writer_bridge_from_builder_rows(...)` as the first
  native bridge from the broad camera-system evidence above. It starts from the
  current native builder/source rows, derives the writer-stage path delta from
  decoded authored path pose span, applies `-pose_span` to the source position,
  and orthonormalizes through the shared result-row path. The provenance string
  labels this as `writer=0x002665a0 payload=a0+0xac path_delta=-pose_span`.
- Evidence link: the statefile writer trace classified as
  `system_handoff_with_payload` shows writer `a0+0xac` position
  `(258.577881,-228.578201,105.311722)` while the retained builder/source row
  is `(259.234528,-229.846008,97.250031)`. The delta is
  `(-0.656647,+1.267807,+8.061691)`, matching the inverse of the decoded path
  pose span family already logged by native camera diagnostics. This makes the
  bridge a generic path-writer operation, not a retained camera substitution.
- Runtime behavior: the bridge is logged as
  `ps2_writer_bridge_candidate` when camera debug rows are enabled and can be
  rendered explicitly with
  `GHOGX_DEBUG_CAMERA_SUBMIT_CANDIDATE=writer_bridge`. Default gameplay camera
  submission is unchanged; retained writer payloads also remain diagnostics.
  This is the next implementation step toward the PS2 writer pipeline, not a
  final camera-parity claim.

2026-06-29 native writer-bridge validation:
- Built `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` under
  `analysis/native_validation/build_watch_writer_bridge_pathspan2_20260629/`;
  the build completed under the watchdog. The focused contract passed under
  `analysis/native_validation/ctest_writer_bridge_pathspan2_20260629/`.
- Native probe
  `analysis/native_validation/native_probe_writer_bridge_20260629/` ran
  `GHOGX_DEBUG_CAMERA=1`, `GHOGX_LOG_CAMERA_MATRIX=1`, and
  `GHOGX_DEBUG_CAMERA_SUBMIT_CANDIDATE=writer_bridge` against the v3 PS2
  archive at `Guitar Hero II PS2 (USA)\GEN`. The earlier raw
  `assets\gen`/stock-archive attempt was overwritten because it never entered
  gameplay and only logged unsupported ARK errors.
- The native log now proves the selector and debug candidate are live:
  `stage=submitted`, `stage=ps2_writer_bridge_candidate`, and
  `[camera-matrix] result_frame source=ps2_writer_bridge_from_builder(...)`
  all carry `writer=0x002665a0 payload=a0+0xac path_delta=-pose_span`.
- This is still not a parity result. The screenshot is a real venue frame, but
  it shows the bridge is currently applying writer-stage path delta to the
  source-seed orientation, producing crowd/source-facing rows
  `forward=(0.935079,0.354440,0.000000)` instead of the accepted writer payload
  orientation near `forward=(0.878...,0.478...,0.000558)` / up
  `(-0.071...,0.130...,0.989...)`. Next camera work should move the bridge to
  the actual builder orientation/payload shape rather than treating source rows
  as builder rows.

2026-06-29 native writer-bridge payload-delta validation:
- Reworked the diagnostic bridge so `writer_bridge` now consumes
  builder-shaped rows via `camera_writer_bridge_builder_rows_for_key(...)`.
  Accepted PS2 result-builder projection rows feed this path as
  `ps2_writer_bridge_builder_projection(...)`; source-seed rows are no longer
  used as the writer bridge orientation source.
- When an accepted trace context contains both `0x00267008:a2+0x90` builder
  projection data and `0x002665a0` writer payload data, the bridge applies the
  trace-derived `writer-builder_payload_delta` and writer payload up/forward
  basis. The fallback path still uses `-pose_span` for path-backed cameras that
  lack retained writer payload evidence.
- Built under
  `analysis/native_validation/build_watch_writer_bridge_payload_delta_20260629/`
  and passed the focused contract under
  `analysis/native_validation/ctest_writer_bridge_payload_delta_contract2_20260629/`.
  Native proof lives in
  `analysis/native_validation/native_probe_writer_bridge_payload_delta_20260629/`.
- Native log proof: `stage=ps2_writer_bridge_candidate` and
  `[camera-matrix] result_frame source=ps2_writer_bridge_from_builder(...)`
  now match the accepted writer payload row exactly for the retained route:
  position `(258.577881,-228.578201,105.311722)`, forward
  `(0.878286,0.478135,0.000558)`, right
  `(-0.472771,0.868606,-0.148361)`, and up
  `(-0.071421,0.130039,0.988933)`.
- Scope note: this is still a diagnostic trace-backed bridge, not the final
  generalized venue camera. The next camera-system step is to derive the
  writer-builder payload delta and up/right adjustment from live native camera
  state for non-retained routes, then compare against the wide writer trace
  before promoting anything into the default gameplay camera.

2026-06-29 camera-system trace completeness check:
- Extended `analysis/pcsx2_trace/analyze_camera_trace_system.py` so accepted
  traces report whether the requested source has a complete
  source-builder-to-writer observation or only a writer payload sample. The
  analyzer now also scans sampled writer payload memory for valid basis windows
  by actual offset, and derives embedded transform deltas only when both
  endpoints are valid bases.
- Rechecked the retained balcony writer trace
  `gh2dxu_arena_writer_handoff_statefile_20260629_025058.json`: source
  `0x0077c610` has `source_builder_to_writer_observed`, with 278 source
  builder records, 232 writer records using that source as `a3`, and valid
  writer basis windows at `a0+0xac` plus `a1+0xbc`.
- Rechecked the accepted wide writer trace
  `gh2dxu_arena_writer_handoff_wide_samples_20260629_031220.json`: source
  `0x0077c2e0` has
  `writer_payload_observed_without_source_builder_handoff`. It contains valid
  writer payload matrices at `a0+0xac` and `a1+0xac`, but the observed builder
  records are the static `0x0052fcd8` path and do not include a source-builder
  handoff for `0x0077c2e0`.
- Runtime implication: keep the current native writer bridge constrained to
  complete retained handoff evidence. The wide capture teaches the writer
  payload format and confirms trace incompleteness for that route, but it is
  not enough by itself to add a second runtime bridge row or promote the
  diagnostic bridge into the default gameplay camera.

2026-06-29 retained trace matching tightened:
- The retained PS2 source-record table is no longer matched by only
  `source_ref + category`. Entries now also preserve the accepted native
  CamShot name (`balcony_lft04` for the retained balcony handoff) and
  `ps2_source_record_trace_context_for_key(...)` requires that exact shot
  name before returning the accepted trace context.
- If a native camera key carries a decoded `ps2_source_record` hint, the hint
  must not contradict the retained source or member symbol. The current
  `balcony_lft04` native key does not expose `bone_spine1.mesh` through its
  decoded CamShot refs, so exact-shot retained evidence is allowed without
  inventing a source-record member on the key.
- Reason: the accepted balcony writer handoff is complete evidence for the
  `crowd` / `balcony_lft` / `balcony_lft04` / `bone_spine1.mesh` route, not
  proof that every `crowd` shot in the same category should receive that
  retained writer payload. This keeps the diagnostic writer bridge
  trace-constrained while broader camera-system learning continues.
- Validation:
  `analysis/native_validation/retained_trace_member_gate_20260629/` built
  `ghogx_app` and reran the venue/band contract. A one-frame native probe and
  a 14-frame screenshot probe against
  `--diagnostic-camera-shot balcony_lft04` both selected
  `path_delta=writer-builder_payload_delta`, proving the exact-shot retained
  route still reaches the accepted writer payload after the match was tightened.
  The screenshot probe saved `screenshots_final/frame_00011.bmp` and `.png`.
  The image remains a diagnostic retained-payload route, not final gameplay
  camera parity.

2026-06-29 camera duration random-int shape:
- `world_objects_worldbase.dta::get_shot_duration` chooses the active
  excitement row and calls `random_int` over that row's inclusive min/max bar
  range. Native already consumed the active excitement row, but the local
  duration picker used a visible counter modulo cycle through the range.
- Replaced that range cycle with a stable pseudo-random bucket keyed by the
  camera shot counter. This keeps native validation reproducible while better
  matching the PS2 script's random-int duration shape. It does not change the
  source-backed split between `start_shot` cuts and same-shot `post_switch_cam`
  blends, and it does not promote any retained writer payload into default
  camera rendering.
- Validation:
  `analysis/native_validation/camera_duration_random_bucket_20260629/` builds
  `ghogx_gameplay_venue_band_contract_test` and `ghogx_app`, passes the focused
  venue/band contract, and runs a default-camera stock PS2 `shoutatthedevil`
  arena window from `16.0s` for 80 fixed-step frames. The run exits `0`,
 reports four regular camera sweeps and seven `post_switch_cam` rows, keeps
  performer active clips and lighting preset rows live, and shows duration
  picks inside the authored ranges (`kExcitementOkay[2,4]`,
  `regular[4,4]`, `kExcitementGreat[1,3]`) without diagnostic camera forcing.

2026-06-29 same-tick MIDI script order:
- Native MIDI parsing now preserves authored order for equal-tick world text
  events, performer text events, and venue effect cues by using stable
  tick-only sorts for those script-driving streams. This is a camera-system
  correctness guard, not a per-shot visual adjustment: GH2 camera, performer,
  lighting, and venue scripts can place multiple directives on one song tick,
  and reordering those rows locally makes accepted PS2 traces appear
  incomplete or contradictory.
- The change is deliberately scoped away from the physical drum/bass/lighting
  note cue sorts that still carry pitch ordering. It protects the text/effect
  streams that behave as ordered script messages.
- Validation:
  `engine/src/chart/midi_reader_test.cpp` now builds a synthetic SMF with
  same-tick `EVENTS` camera-like markers and same-tick `BAND SINGER` markers
  and asserts native parse order stays authored after the time sort. The
  venue/band contract also pins stable ordering for `text_events`,
  `performer_events`, and `venue_cues`. A bounded native arena run from
  `156.0s` on stock PS2 `shoutatthedevil` exits `0` in
  `analysis/native_validation/same_tick_midi_order_20260629/`, parses
  `text events=89`, `performer text events=90`, and `venue cues=58`, then
 logs forced and regular camera sweeps, venue_effect dispatch, band_jump,
  active performer clips, and lighting preset changes.

2026-06-29 ordered camera script cursor:
- Forced camera messages now consume the shared `EVENTS` text stream through a
  dedicated cursor instead of scanning the whole chart each frame and collapsing
  all camera messages on the same tick behind one `last tick` guard.
- This keeps the native camera director aligned with the script-system model:
  `[crowd_lighters_slow]`, `[crowd_lighters_fast]`, `[crowd_lighters_off]`,
  `[band_jump]`, `[sync_wag]`, and `[sync_head_bang]` are consumed once in the
  authored MIDI order preserved by the stable parser sort. Same-tick messages
  can now update the director state sequentially; a non-forcing later sync
  marker does not erase an earlier force, while a later forcing marker can
  become the current forced-shot request.
- Diagnostic seeks skip already elapsed camera-script text events with the same
  tick-order cursor pattern used by venue section messages. This is a
  camera-system parity change, not a one-shot camera-angle adjustment. During
  runtime, the cursor keeps the old scan's small current-frame window so
  messages that elapsed while the intro camera was still active are skipped
  rather than replayed as stale forced-camera requests.
- Validation:
  `analysis/native_validation/ordered_camera_script_cursor_20260629/` builds
  `ghogx_app` and the venue/band contract, then runs stock PS2
  `shoutatthedevil` in arena from `156.0s` for 36 fixed-step frames. Native
  exits `0`, parses `text events=89`, `performer text events=90`, and
  `venue cues=58`, then logs `[crowd_lighters_fast]` forcing `mode=lighter`
  at tick `119040`, `[band_jump]` forcing `mode=jump` at tick `120960`, and
  both same-tick tick `122880` messages in order:
  `[crowd_lighters_off]` forces the regular route and `[sync_wag]` records
  `force=0` at excitement `2` without cancelling that earlier force. The same
  run logs the resulting regular camera sweeps, venue_effect dispatch,
  band_jump clips, lighting preset changes, and `post_switch_cam`.

2026-06-29 source-record member table scoping:
- The PS2 source-object evidence for the retained camera chain points at a
  runtime list for the active source/category family, not a global bag of every
  decoded CamShot member in the venue. Native diagnostics now build the
  decoded source-record member table in the context of the active camera key:
  member candidates must match the active source ref and, when present, the
  active CamShot category.
- This keeps the table diagnostic closer to the accepted `0x0077c610` /
  `0x00261c58` source-object evaluator shape while still keeping all
  source-record table, sibling, and native-context rows out of submitted camera
  behavior. The older global table remains only as a fallback for legacy
  diagnostic callers that do not provide the decoded key inventory. Under
  `GHOGX_DEBUG_CAMERA=1`, native logs the active table context as
  `[camera-source-record] table context ... members=N` so validation can prove
  the table was scoped to the current source/category before candidate ranking.
- Validation:
  `analysis/native_validation/source_record_table_context_20260629/` builds
  `ghogx_gameplay_venue_band_contract_test` and `ghogx_app`, passes the focused
  contract, then runs a short forced stock arena `balcony_lft04` probe with
  path offset `255`, retained owner/source/forward probes, and camera debug
  enabled. Native exits `0` and logs the diagnostic camera selection plus
  repeated scoped table rows:
  `shot=balcony_lft04 category=balcony_lft source=crowd members=1` for both
  interpolation endpoints. The same log retains the
  `ps2_source_record_trace_context_source_seed(...)` and
  `ps2_source_record_native_owner_member_eval_world_copy_candidate(...)`
  diagnostic rows, with no fatal/error rows beyond the known PowerShell stderr
  wrapper and lighting coverage summaries with `failed=0`.

2026-06-29 camera-system writer-source graph:
- Updated `analysis/pcsx2_trace/analyze_camera_trace_system.py` so each
  accepted trace now reports `writer_source_observations_top` automatically,
  without requiring a single hardcoded `--writer-source`. The analyzer compares
  every observed writer `a3` source and builder `a0` source, then labels the
  strength of the evidence as a complete `source_builder_to_writer_observed`,
  a `writer_payload_observed_without_source_builder_handoff`, or a weaker
  `source_builder_precedes_unlinked_writer`.
- Validation:
  `analysis/native_validation/camera_system_writer_source_links_20260629/`
  reruns the analyzer on the retained writer-handoff trace and the wide writer
  trace. The retained route reports
  `0x0077c610 source_builder_to_writer_observed builder=278 writer_a3=232`,
  preserving the complete handoff gate. The wide route reports
  `0x0077c2e0 writer_payload_observed_without_source_builder_handoff
  builder=0 writer_a3=301` and
  `0x0052fcd8 source_builder_precedes_unlinked_writer`, proving the trace
  teaches writer payload layout but is incomplete for a generalized native
  source-to-writer runtime bridge.

2026-06-29 camera-system builder/writer basis relation:
- Extended `analysis/pcsx2_trace/analyze_camera_trace_system.py` again so
  accepted system traces report valid `0x00267008` builder basis windows and
  nearest builder-to-writer payload deltas. This makes the analyzer learn the
  camera system shape directly: builder output basis first, then
  `0x002665a0` writer payload basis, rather than treating one screenshot angle
  or one retained writer payload as the whole answer.
- Validation:
  `analysis/native_validation/camera_system_builder_writer_windows_20260629/`
  reruns the analyzer on the complete long handoff trace and the wide writer
  trace. The complete handoff trace reports a valid builder basis at
  `cam_result_builder_00267008:a1[0]+0x0`:
  position `(258.967468,-229.590378,99.474648)`, forward near
  `(0.877756,0.477849,0.000559)`, and up near
  `(-0.080145,0.146065,0.985412)`. Its nearest writer payload is
  `cam_result_writer_002665a0:a0+0xac` with
  `dist=5.936979`, position delta `(-0.389587,1.012177,5.837074)`,
  `forward_dot=0.999032`, and `up_dot=0.998911`.
- Runtime implication: the next native bridge should be constrained to this
  builder-basis -> writer-payload relation. The accepted wide trace still
  confirms writer payload layout at `a0/a1+0xac`, but without a decoded builder
  basis window or linked source-builder handoff it remains insufficient for a
  generalized default-camera promotion.

2026-06-29 native builder-basis writer bridge:
- Promoted the retained diagnostic writer bridge from "source/projection rows
  plus retained writer delta" to the accepted builder-basis relation. The
  retained source-record context now stores the long handoff trace's
  `0x00267008:a1+0x0` builder basis separately from the earlier evaluated
  source row, and `camera_writer_bridge_builder_rows_for_key(...)` prefers
  `ps2_writer_bridge_builder_basis(...)` before the older projection fallback.
- The bridge applies `writer-builder_basis_delta` when that accepted builder
  basis is available; the older `writer-builder_payload_delta` label remains
  only for retained rows that lack an explicit builder basis. This avoids the
  intermediate wrong result found during validation, where the builder basis was
  combined with the old source-row delta and landed at roughly
  `(258.310822,-228.322571,107.536339)` instead of the accepted writer payload.
- Validation:
  bounded build of `ghogx_gameplay_venue_band_contract_test` and `ghogx_app`
  completed in `19.6s`; the focused contract passed in `0.2s`. The analyzer
  gate in
  `analysis/native_validation/camera_system_builder_writer_windows_20260629/`
  exits `0` and documents the accepted builder-to-writer relation. The native
  probe
  `analysis/native_validation/native_probe_writer_bridge_builder_basis_delta_20260629/`
  exits `0` in `72.1s` with `GHOGX_DEBUG_CAMERA_SUBMIT_CANDIDATE=writer_bridge`.
  Its live camera rows show
  `stage=ps2_result_builder_basis_candidate`, source
  `result=0x00267008:a1+0x0 trace=gh2dxu_arena_builder_a0_shot_identity_long_20260624`,
  and the submitted diagnostic result frame source
  `ps2_writer_bridge_from_builder(ps2_writer_bridge_builder_basis(...))`
  with `writer=0x002665a0 payload=a0+0xac
  path_delta=writer-builder_basis_delta`. The result frame now matches the
  accepted writer payload row: position
  `(258.577881,-228.578201,105.311722)`, forward
  `(0.878286,0.478135,0.000558)`, and up
  `(-0.071421,0.130039,0.988933)`. Health scan only found the known
  PowerShell `NativeCommandError` wrapper and lighting coverage summaries with
  `failed=0`.

2026-06-29 camera-system writer builder-pair gate:
- Extended `analysis/pcsx2_trace/analyze_camera_trace_system.py` to classify
  the immediate `0x00267008` builder pair before each `0x002665a0` writer
  call. A complete handoff now requires the second previous builder `a0` to
  match writer `a3` and the immediately previous builder `a0` to match writer
  `a2`. This encodes the observed system shape
  `path/apply -> builder(source) -> builder(result object) -> writer`, instead
  of merely counting builder and writer calls somewhere in the same trace.
- Validation:
  `analysis/native_validation/camera_system_writer_builder_pairs_20260629/`
  reruns the analyzer on the complete long handoff trace, the retained
  statefile handoff trace, and the accepted wide writer trace. The long trace
  reports `complete=True count=512` for
  `prev2_a0=0x0077c610 prev_a0=0x008269f0 writer_a2=0x008269f0
  writer_a3=0x0077c610`; the statefile trace reports the same complete pair
  with `count=232`. The wide trace reports `complete=False count=300` because
  its observed builders are both `0x0052fcd8` while writer `a2/a3` are
  `0x00827850/0x0077c2e0`. That keeps the wide trace useful for writer payload
  layout but insufficient for generalized runtime camera promotion.

2026-06-29 camera trace acceptance builder-pair gate:
- Hardened `analysis/pcsx2_trace/validate_camera_trace_acceptance.py` with
  `--require-writer-builder-pair`. The shot-promotion gate now distinguishes
  between a writer that merely appears after some source builder and the
  immediate PS2 camera-system handoff where the second previous builder `a0`
  matches writer `a3` and the immediately previous builder `a0` matches writer
  `a2`.
- Validation:
  `analysis/native_validation/camera_trace_acceptance_builder_pair_gate_20260629/`
  runs four cases against the retained dark balcony reference. With RMS relaxed
  to isolate system completeness, the long handoff trace passes with
  `complete_writer_builder_pairs=512` and the statefile handoff trace passes
  with `232`. The wide writer trace fails even under relaxed RMS:
  `complete_writer_builder_pairs=0`,
  `incomplete_writer_builder_pairs=301`, and
  `missing immediate builder pair feeding writer a2/a3 for 0x0077c2e0`.
  The exact dark source-only balcony trace still has `rms=0.0` but fails
  promotion because it has no writer calls and no immediate builder pair.

2026-06-29 native writer-pair provenance:
- Native retained camera diagnostics now carry the complete writer-builder pair
  evidence from the trace gate into runtime provenance. The retained
  `balcony_lft04` context stores `prev2_a0=0x0077c610` for the source builder
  and `prev_a0=0x008269f0` for the result-object builder, and builder-basis /
  writer-bridge source strings include `pair=complete` alongside those ids.
- Validation:
  built `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` in `19.8s`,
  then passed the focused contract in `0.2s`. Native probe
  `analysis/native_validation/native_probe_writer_bridge_pair_provenance_20260629/`
  exits `0` in `69.6s` with `GHOGX_DEBUG_CAMERA_SUBMIT_CANDIDATE=writer_bridge`.
  The log proves the runtime diagnostic bridge is not using payload-only
  evidence: `ps2_result_builder_basis_candidate(...)` and the submitted
  `ps2_writer_bridge_from_builder(ps2_writer_bridge_builder_basis(...))`
 both include
  `pair=complete prev2_a0=0x0077c610 prev_a0=0x008269f0`, plus
  `path_delta=writer-builder_basis_delta`. The submitted result remains the
  accepted writer payload row `(258.577881,-228.578201,105.311722)` with
  forward `(0.878286,0.478135,0.000558)`. Health scan only found the known
  PowerShell `NativeCommandError` wrapper and `failed=0` lighting coverage
  rows.

2026-06-29 opt-in trace-complete writer bridge submission:
- Added an opt-in runtime submit path gated by
  `GHOGX_CAMERA_USE_TRACE_COMPLETE_WRITER_BRIDGE`. The path only promotes a
  retained PS2 writer bridge when the retained trace context has the complete
  immediate builder pair proven by the acceptance gate. This keeps default
  gameplay unchanged while letting native validation exercise the learned PS2
  camera-system handoff directly: source builder -> result-object builder ->
  writer payload.
- Validation:
  bounded rebuild of `ghogx_gameplay_venue_band_contract_test` and `ghogx_app`
  completed in `19.6s`; the focused gameplay venue/band contract passed in
  `0.2s`. Native probe
  `analysis/native_validation/native_probe_trace_complete_writer_bridge_optin_20260629/`
  exits `0` in `71.1s` with the opt-in enabled and without
  `GHOGX_DEBUG_CAMERA_SUBMIT_CANDIDATE`. The live submitted rows show
  `stage=submitted` with source
  `ps2_writer_bridge_from_builder(ps2_writer_bridge_builder_basis(...))`,
  `pair=complete prev2_a0=0x0077c610 prev_a0=0x008269f0`,
  `writer=0x002665a0 payload=a0+0xac`, and
  `path_delta=writer-builder_basis_delta`. The matrix output uses the accepted
  writer payload basis: position `(258.577881,-228.578201,105.311722)`,
  forward `(0.878286,0.478135,0.000558)`, and up
  `(-0.071421,0.130039,0.988933)`. Health scan only found the known
  PowerShell `NativeCommandError` wrapper from stderr text and lighting
  coverage rows with `failed=0`.

2026-06-29 camera-system corpus shape inventory:
- Shifted the camera trace analyzer from per-shot interpretation toward
  corpus-level camera-system learning. Each analyzed trace now reports a
  `camera_system_shape`, plus complete/incomplete immediate writer-builder
  pair counts. Multi-trace JSON includes a `corpus_summary` with shape counts
  and the trace names that prove or fail the complete handoff. The important
  distinction is no longer "which camera angle is this?" but whether the trace
  proves the PS2 graph shape:
  `path/apply -> builder(source) -> builder(result object) -> writer`.
- Validation:
  `analysis/native_validation/camera_system_trace360_inventory_20260629/`
  reruns the updated analyzer over 39 GH2DXU/PCSX2 camera traces from the
  trace workspace. The run exits `0` in `1.1s`. Corpus summary reports
  `complete_writer_builder_pair=6`,
  `incomplete_writer_builder_pair=11`,
  `source_path_builder_only=4`, `builder_only=2`, and `unclassified=16`.
  Complete-pair traces include the accepted long handoff
  `gh2dxu_arena_builder_a0_shot_identity_long_20260624.json` with `512`
  complete pairs, the statefile handoff with `232`, the shorter builder A0
  identity trace with `283`, the result-builder A2 follow route with `256`,
  and source-delta follow with `44`. The wide writer sample remains
  `incomplete_writer_builder_pair` with `301` incomplete pairs, so it remains
  useful for writer payload layout but not for native promotion.

2026-06-29 native camera-system shape gate:
- Native retained PS2 camera evidence now carries the analyzer's graph-shape
  result instead of only a boolean. The retained source-record context stores
  `camera_system_shape=complete_writer_builder_pair`, complete/incomplete pair
  counts, and the trace artifact that proved the immediate builder pair. The
  opt-in runtime bridge refuses promotion unless the shape is complete, the
  complete count is positive, and the incomplete count is zero.
- This keeps the bridge data-shaped: a native camera is promoted because it has
  accepted PS2 camera-system evidence, not because it happens to be named
  `balcony_lft04`.

2026-06-29 native camera pair provenance logs:
- Factored native writer-builder pair provenance through one formatter so the
  builder-basis candidate and writer-bridge result both carry the analyzer
  graph-shape proof. Submitted camera logs now include
  `pair=complete`, `shape=complete_writer_builder_pair`,
  `complete_count=512`, `incomplete_count=0`,
  `trace=gh2dxu_arena_builder_a0_shot_identity_long_20260624`,
  `prev2_a0=0x0077c610`, and `prev_a0=0x008269f0`.
- Validation:
  `analysis/native_validation/camera_pair_provenance_logs_20260629/` rebuilds
  `ghogx_gameplay_venue_band_contract_test` and `ghogx_app` in `19.6s`, passes
  the focused contract in `0.2s`, and runs the opt-in native probe in `69.6s`.
  The probe exits `0`; submitted `ps2_writer_bridge_from_builder(...)` rows and
  `camera-matrix result_frame` rows both show the complete shape/count/trace
  provenance on the accepted `writer-builder_basis_delta` path. Health scan
  only found lighting coverage summaries with `failed=0`.
