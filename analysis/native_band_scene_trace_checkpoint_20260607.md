# Native Band Scene Trace Checkpoint - 2026-06-07

Scope: evidence only. This checkpoint is the current handoff for moving from
Rexglue traces into the native PS2-asset band scene. Do not treat any native
animation code as approved from this note alone; the current instruction is to
check in before writing more native animation code.

## Rexglue Trace Route Fix

The earlier headless Quick Play route sent song-list navigation too early. The
script label `A_select_quickplay` was not the real song-list entry point for
the active Rexglue build. JSON input-edge tracing proved the only `DPAD_DOWN`
near that point occurred before `ui/gen/sel_song_quickplay.milo_xbox` and
`ui/gen/list_song2.milo_xbox` loaded, so `ss_song.lst selected_pos` stayed at
0 and every run committed `set_song_index = 0`.

Corrected route:

- Wait until the step that actually opens `sel_song_quickplay`.
- Then send song-list `DPAD_DOWN` offsets.
- Verify with `input.guitar_edge`, `DataNode_Resolve`, `DataNode_AsInt`, and
  final song asset opens.

Trace-only instrumentation added:

- `hmx_GuitarPort_RemapButtons` logs `input.guitar_edge`.
- `sub_82316428` logs selection/list property dispatch.
- `hmx_DataNode_Resolve` and `hmx_DataNode_AsInt` log the resolved
  `selected_pos` / `set_song_index` values.
- `sub_82291428` and `sub_82291B70` were instrumented as candidate list
  handlers but did not fire on this Quick Play path.

Functions seen in the committed selection chain include:

- `sub_822D4D48`
- `sub_82377300`
- `sub_82328988`
- `sub_82329478`
- `sub_822CD180`

Corrected trace evidence:

| Trace | Committed Index | Final Song Assets | Character Families |
| --- | ---: | --- | --- |
| `trace_1780804828.jsonl` | 1 | `songs/possum/possum.{mid,mogg,voc}` | `alterna1`, `metal_singer`, `metal_bass`, `metal_drummer`, crowd/shared |
| `trace_1780804929.jsonl` | 2 | `songs/heartshapedbox/heartshapedbox.{mid,mogg,voc}` | `rock1`, `rock2`, `metal_singer`, `metal_bass`, `metal_drummer`, crowd/shared |
| `trace_1780805014.jsonl` | 6 | `songs/mother/mother.{mid,mogg,voc}` | `goth1`, `goth2`, `metal_singer`, `metal_bass`, `metal_drummer`, crowd/shared |

Each corrected run captured `320` weighted and `320` unweighted animation apply
events in the current hook window.

## Native PS2-Asset Runtime Check

Native visual test used the PS2 v3-readable asset root:

`C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\GEN`

The raw/encrypted-looking roots whose HDR begins `D9 02 D5 1C` are not readable
by the current `ArkV3Reader`; they produce `ARK HDR version 483721945 not
supported`.

`possum` is not present in the PS2 asset set, so the native runtime check used
`shoutatthedevil`, which is present in the PS2 ARK.

Command:

```powershell
& "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX\engine\out\build\codex-vs\src\app\RelWithDebInfo\ghogx_app.exe" `
  --ark-dir "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\GEN" `
  --auto-start --song shoutatthedevil --difficulty 0 `
  --fixed-dt 0.0166667 --frames 180 `
  --screenshot "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX\engine\out\native_shout_world.png" `
  --screenshot-frame 150
```

Verified native load evidence:

- Chart: `songs/shoutatthedevil/shoutatthedevil.mid`
- Audio: `songs/shoutatthedevil/shoutatthedevil.vgs`
- Quickplay rig: `character=glam1`, `guitar=flying_v`, `venue=arena`
- Band: `metal_singer`, `metal_bass`, `metal_drummer`
- Venue geometry: `world/arena/og/gen/arena_geom.milo_ps2`
- Lighting overlay: `world/arena/og/gen/arena_lighting.milo_ps2`
- Character placement scene: `world/arena/gen/arena_chars.milo_ps2`
- Guitarist: `char/glam1/og/gen/glam1.milo_ps2`
- Guitar prop: `char/og/guitars/gen/flyingv_v2.milo_ps2`
- Singer: `char/metal_singer/og/gen/metal_singer.milo_ps2`
- Bassist: `char/metal_bass/og/gen/metal_bass.milo_ps2`
- Bass prop: `char/og/guitars/gen/bass_music_black.milo_ps2`
- Drummer: `char/metal_drummer/og/gen/metal_drummer.milo_ps2`
- Drum kit: `char/og/drums/gen/dw_arena_drums.milo_ps2`
- Intro camera: `Intro.tnm`
- Regular camera shots: decoded from arena CamShot data.
- Lighting preset at t=0.017: `blackout.pst`, request `VERSE/blackout`.

Screenshot produced:

`C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX\engine\out\native_shout_world.png`

Observed current visual state:

- Arena renders.
- Lighting overlay and venue event filters activate.
- Band performers and instruments load and are visible.
- Drum kit loads.
- The shot is still a wide/early camera proof, not a finished 1:1 band scene.
- Native completion remains unproven: correct animation blending, final camera
  sweeps, and lighting exactness still require Rexglue-derived implementation
  work and visual comparison.

## Trace-Only Character RE Addendum

Scope guard: no native animation code was written for this addendum. The user
explicitly requested trace/static RE only until a check-in proves the animation
system has been traced enough.

Local-only sources used:

- PS2 files: `C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)`
- PS2 ISO: `C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso`
- PCSX2 install: `C:\Games\Emulators\PCSX2`
- Local Ghidra install: `C:\Programming\ghidra_11.3.2_PUBLIC`
- Local community DTA files under `_community_re\Guitar-Hero-II-Deluxe-Unified`

### Rexglue Runtime Evidence

Three later clean Rexglue song traces captured the venue/band update stack over
different songs and performer families:

| Trace | Song Offset | Song | Character Families |
| --- | ---: | --- | --- |
| `trace_1780806552.jsonl` | 1 | `possum` | `alterna1`, `metal_singer`, `metal_bass`, `metal_drummer` |
| `trace_1780806304.jsonl` | 2 | `heartshapedbox` | `rock1`, `rock2`, `metal_singer`, `metal_bass`, `metal_drummer` |
| `trace_1780806656.jsonl` | 5 | `mother` | `goth1`, `goth2`, `metal_singer`, `metal_bass`, `metal_drummer` |

The contaminated `trace_1780806445.jsonl` should not be used because parallel
writers polluted that capture.

Trace-only hooks confirmed periodic/stack activity for:

- `anim.apply.weighted`
- `anim.apply.unweighted`
- `lighting.preset.update`
- `camera.camshot.update`
- `camera.camshot.blend`
- `crowd.world.update`
- `char.ik.update`

Important stack evidence:

- `char.ik.update`: `sub_8214CD88 <- __imp__sub_821D1190 <- __imp__sub_821D1710 <- __imp__sub_8214CAC8 <- __imp__sub_8214A8D0 <- __imp__sub_8214C610 <- __imp__sub_8214A838`
- `camera.camshot.update`: `sub_822F5A70 <- __imp__sub_822F60B0 <- __imp__sub_822A12B8 <- __imp__sub_823763D8 <- __imp__sub_82378D88`
- `camera.camshot.blend`: `sub_822F6B58 <- __imp__sub_822F7260 <- __imp__sub_822F76D0 <- __imp__sub_822F7EA0 <- __imp__sub_826A14F0 <- __imp__sub_826A3418 <- __imp__sub_826A6370 <- __imp__sub_826A2380 <- __imp__sub_822F9050 <- __imp__sub_822E5A48`
- `crowd.world.update`: `sub_822CC848 <- __imp__sub_823763D8 <- __imp__sub_82378D88 <- __imp__sub_8236B460`
- `lighting.preset.update`: `sub_821BCAE8 <- __imp__sub_821B2800 <- __imp__sub_821DB520 <- __imp__sub_821B2800 <- __imp__sub_821B8ED0 <- __imp__sub_8236D3B0`

### PS2 SLUS Static Anchors

PCSX2 boot logging proved the local PS2 executable loads:

- CRC: `0x2A6C845B`
- ELF entry point: `0x00100BF0`
- Boot target: `cdrom0:\SLUS_214.47;1`

Runtime PINE/socket tracing was not proven. `pcsx2.exe --nogui --noguiprompt`
booted hidden far enough to log the SLUS, but no TCP endpoint or named pipe was
observed. Do not claim live PS2 function tracing is working yet.

Ghidra default ELF import produced useful string xrefs. A forced MIPS32 import
gave the expected processor but lost direct string-xref function evidence, so
the default import is the current static map of record.

Static string/function anchors found in `SLUS_214.47` include:

- Character animation: `CharClipSamples`, `CharClipSet`, `CharClipGroup`, `CharClipFilter`, `CharDriver`, `CharDriverMidi`
- IK and driven body systems: `CharIKHand`, `CharIKFoot`, `CharIKMidi`, `CharDriverMidi`, `CharIKRod`
- Facing/recenter bones: `bone_facing.pos`, `bone_facing_delta.pos`, `bone_facing.rotz`, `bone_facing_delta.rotz`, `bone_pelvis`, `bone_pelvis.pos`, `bone_pelvis.mesh`
- Camera systems: `CamShot`, `CamAnim`, `free_camera`, `play_camera_sequence`, `pick_shot`, `force_shot`, `current_shot`
- Lighting systems: `LightPreset`, `LightAnim`, `lighting`, `lighting_change`, `lighting_next_keyframe`, `lighting_prev_keyframe`, `lighting_first_keyframe`
- Venue/crowd systems: `Performer`, `PerformerGroup`, `WorldCrowd`
- MIDI systems: `MidiParser`, `MidiReceiver`, `midi_file`, `midi_parser`, `midi_parsers`, `midi_drums`

Useful static call anchors:

- `FUN_00335d30` registers `15CharClipSamples`.
- `FUN_00337740` registers `11CharClipSet`.
- `FUN_0033f4b0` registers `10CharIKFoot`.
- `FUN_0033f818` registers `10CharIKHand`.
- `FUN_003a4f58` registers `7CamShot`.
- `FUN_003a7888` registers `10WorldCrowd`.
- `FUN_003b7520` registers `11LightPreset`.
- `FUN_00163a58` resolves `bone_pelvis.mesh`.

### DTA Semantics From Local Community Files

The local DTA descriptions provide a useful Rosetta stone for class intent:

- `CharClipSamples`: sample data grouped by time and interpolated.
- `CharClipSet`: clip container; `recenter` can target bones such as
  `bone_pelvis`.
- `CharDriver`: schedules and blends CharClips as a stack.
- `CharDriverMidi`: uses parser event lists to play animation on the character.
- `CharIKHand`: pins the hand bone to a destination transform with orientation
  and stretch options.
- `CharIKFoot`: foot-skate IK.
- `CharIKMidi`: moves a bone to another spot over time.
- `CharForeTwist`: forearm twist interpolation; documented left/right offset
  rotations are normally `90` and `-90` degrees.
- `CharHair`: hair physics rooted to named bones/transforms with collisions.
- `WorldCrowd`: 3D crowd placement and crowd characters.
- `BandCamShot`: `CamShot` subclass with target object plus `CharClipGroup`
  links, `play_group`, `start_shot`, `check_shot`, focus flags, timing, and
  crowd/postprocess controls.

### PS2 Asset Audit

Archive listing from `GEN\MAIN.HDR` / `GEN\MAIN_0.ARK`:

- ARK version: `3`
- Entry count: `1585`
- `.milo_ps2`: `488`
- `.vgs`: `473`
- `.dtb`: `179`
- `.mid`: `73`

Character body audit report:

`analysis\ps2_trace\body_audit\character_body_pollables_summary.txt`

Selected object-payload reference report:

`analysis\ps2_trace\body_audit\selected_character_payload_refs.txt`

Clip channel report:

`analysis\ps2_trace\animation_samples\clip_channel_summary.txt`

Decoded body-object audit:

`analysis\ps2_trace\re_anim_body_audit.json`

Decoded animation-object audit:

`analysis\ps2_trace\re_anim_clip_audit.json`

Focused exact-body field report:

`analysis\ps2_trace\re_anim_focused_body_only_fields.txt`

The local `tools\re_anim_audit.py` parser consumed all targeted objects without
failures:

- Body MILOs: `91` entries, `0` failures.
- Body objects decoded: `CharDriver=82`, `CharDriverMidi=76`,
  `CharEyes=19`, `CharForeTwist=116`, `CharHair=93`, `CharIKHand=76`,
  `CharIKMidi=38`, `CharIKRod=6`, `CharLookAt=38`, `CharServoBone=80`,
  `CharUpperTwist=134`, `CharWalk=38`, `CharWeightSetter=76`,
  `FaceFxLipSyncServo=42`.
- Animation MILOs: `72` entries, `0` failures.
- Animation objects decoded: `CharBone=4903`, `CharClipFilter=72`,
  `CharClipGroup=329`.

Important body-driver findings:

- Standard selectable guitarists such as `glam1`, `rock2`, `deathmetal1`, and
  `rockabill1` carry `CharDriver`, two `CharDriverMidi`, `CharForeTwist` L/R,
  `CharUpperTwist` L/R, `CharIKHand` L/R, `CharIKMidi`, `CharWeightSetter`
  L/R, `CharEyes`, `CharLookAt` L/R, `CharHair`, and `FaceFxLipSyncServo`.
- `rock2` does not own a unique animation folder in the PS2 archive; its body
  drivers explicitly reference `../../../rock1/anims/rock1_main.milo`,
  `rock1_fret.milo`, `rock1_strum.milo`, and `rock1_viseme.milo`.
- `rockabill1` owns `rockabill1_main.milo`, `rockabill1_fret.milo`, and
  `rockabill1_strum.milo`; its wrong-facing/cross-leg failure should be traced
  through that set, not assumed to be shared with Rock1/Rock2.
- `metal_bass` is structurally different from guitarists: body audit found
  `CharDriver`, `CharServoBone`, and `CharUpperTwist` L/R only. Its main driver
  references `../../anims/bass_main.milo`. No `CharHair` object appears in the
  extracted `metal_bass` body MILO, so hair/attachment problems there are likely
  mesh/bone attachment or another layer, not the same `CharHair` path as glam,
  rock, or deathmetal.

Arm/spaghetti evidence:

- Guitarist bodies explicitly define `CharForeTwist__foreTwist_L.ik` from
  `bone_L-hand.mesh` to `bone_L-foreTwist2.mesh`.
- Guitarist bodies explicitly define `CharForeTwist__foreTwist_R.ik` from
  `bone_R-hand.mesh` to `bone_R-foreTwist2.mesh`.
- They also define `CharUpperTwist` from upper arm to upper twist bones.
- The likely cause of spaghetti arms is missing, wrong-order, or wrong-offset
  `CharForeTwist` / `CharUpperTwist` execution, not just clip channel decode.

Hand/guitar evidence:

- `CharDriverMidi__left_hand.drv` references `left.weight`, `bone.servo`, and
  the character fret animation MILO.
- `CharDriverMidi__right_hand.drv` references `right.weight`, `bone.servo`, and
  the character strum animation MILO.
- `CharIKHand__left_hand.ik` maps `left.weight`, `bone_L-hand.mesh`, and
  `bone_fret_hand.mesh`.
- `CharIKHand__right_hand.ik` maps `right.weight`, `bone_R-hand.mesh`, and
  `bone_strum_hand.mesh`.
- `CharIKMidi__fret.ik` references `bone_fret.mesh`.
- Body-only decoded fields show `right.weight` uses flags `0x800000`, while
  `left.weight` uses flags `0x400000`.
- `rock2` has `CharIKHand` serialized weights of `0.0`, but the matching
  `CharWeightSetter` objects are `1.0`; do not drop those IK records based only
  on the serialized IK object weight.

Eye evidence:

- `glam1` and similar guitarists define `CharEyes.eyes`, `l-eye.lookat`,
  `r-eye.lookat`, and `FaceFxLipSyncServo`.
- `glam1` uses `eye-L.mesh` / `eye-R.mesh`.
- `rockabill1` uses `l-eye.mesh` / `r-eye.mesh` and has extra upper-lid content
  in `CharEyes.eyes`.
- Body-only decoded fields show Glam1 eye look-at limits are asymmetric:
  left `(-20,25,-10,15)`, right `(-25,20,-10,15)`.
- Body-only decoded fields show Rockabill1 eye look-at limits are wider:
  left `(-30,40,-10,18)`, right `(-40,30,-10,18)`, plus
  `bone_R-upperlid.mesh`.
- Glam eye placement must be traced through `CharEyes`, `CharLookAt`, and
  `FaceFxLipSyncServo`; arbitrary eye transform fixes are not grounded.

Hair evidence:

- `glam1`: one `CharHair` object, `hair.hair`, rooted through hair/bang bones
  and `bone_neck.mesh` references.
- `rock2`: `hair_front.hair` and `hair_back.hair`, rooted through
  `bone_hair-front.mesh`, `bone_R-hair01.mesh`, `bone_L-hair01.mesh`,
  `bone_head.mesh`, and `bone_neck.mesh`.
- `deathmetal1`: `hair_back.hair` and `hair_front.hair`, rooted through
  `bone_hair01.mesh`, `bone_hair-L01.mesh`, and `bone_hair-R01.mesh`.
- `rockabill1`: one `hair.hair`, rooted through `bone_hair.mesh`.
- Body-only decoded `CharHair` globals differ by character; for example Glam1
  uses `0.08,0.10,0.80,1.0,1.0,0.30`, Rock2 front hair uses
  `0.04,0.40,0.80,1.0,1.0,0.15`, and Rockabill1 uses
  `0.33,0.90,0.70,1.0,0.5,0.30`. Treat these as authored behavior inputs,
  not cosmetic constants.

Leg/facing evidence:

- Extracted `CharClipSamples` payloads for guitarists and bassist contain
  `bone_facing.pos`, `bone_facing.rotz`, `bone_pelvis.pos`,
  `bone_pelvis.quat`, thigh/ankle/knee/toe channels, and instrument-position
  bones such as `bone_pos_guitar` or `bone_pos_gutbass`.
- Walk/turn groups are present in main clip sets (`walk_walk`, `walk_turn`,
  `walk_stop`) and include named turn clips like `turn_left_90_medium` and
  `turn_right_90_medium`.
- The wide stance and Rockabill facing/cross-leg failures should be traced
  through CharClipSet recenter/facing/pelvis application and CharDriver
  scheduling before any pose-space fix is attempted.

### Current Proof Gaps

- PCSX2 runtime function tracing is not operational yet; only boot/logging is
  proven.
- Rexglue runtime tracing has update stacks and event counts, but not enough
  per-object field dumps for `CharForeTwist`, `CharUpperTwist`, `CharHair`,
  `CharEyes`, or `CharClipSet` recenter execution order.
- Native animation implementation should not resume until those exact systems
  are either traced in Rexglue or statically mapped far enough in PS2/Rexglue to
  prove field layout and update order.
- PCSX2 INI files were restored from the backups made before trace setup:
  `PCSX2.ini.codex_trace_backup_20260607` and
  `PCSX2_vm.ini.codex_trace_backup_20260607`.
