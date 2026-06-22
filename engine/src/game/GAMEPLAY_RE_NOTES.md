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
  `ignored_last_light_change`. Native now parses those TRIGGERS notes into
  `Chart::lighting_cues`, applies the four-beat parser offset, and uses the
  cue stream to drive `active_lighting_keyframe_index_`; the older
  duration/beat loop remains only as a fallback for charts without lighting
  cues. Validation:
  `engine/out/codex_goal_20260620_lighting_midi_cues_surrender/` and
  `engine/out/codex_goal_20260620_lighting_midi_cues_surrender_long/` run
  stock PS2 `surrender` with authored camera/venue. The log decodes
  `lighting cues=204`, dispatches early `first`/`next` cues through the
  skip gate, changes presets from `blackout.pst` to `strobe_okay.pst`,
  `verse_okay.pst`, and `color1.pst`, and renders nonblank arena frames.
  This is traced keyframe-dispatch plumbing, not final render-light parity.
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
  `48/49/50` with the four-beat parser offset, and the separation between
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
  the normal `fret_mask` before strum edge detection, matching the accepted
  direct-autoplay trace workflow without bypassing note-hit scoring or venue
  dispatch. Validation:
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
