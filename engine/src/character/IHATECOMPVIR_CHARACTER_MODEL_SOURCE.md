# GH2 Character Model Source Map

This file is the source-backed map for the native GH2 PS2 character model path.
Treat ihatecompvir's repos as the authority; do not use the older
`CHARACTER_FORMAT_NOTES.md` as proof when these sources disagree.

## Source Snapshot

The small `third_party/ihatecompvir-public-milo-sources` tree is an
in-worktree reference snapshot, not a full mirror. Its local `README.md`
records the upstream commits for the copied files:

- `ihatecompvir-public-milo-sources/MiloEditor` at `3ebffb1c4391dd83c5765cb428eef433dffaff51`
- `ihatecompvir-public-milo-sources/glTFMilo` at `3c02a5497ede1a5d61023fb066cc8bfbe2e8a8e4`
- `ihatecompvir-public-milo-sources/rb3` at `41719f248995f677ffa39bd394706b5d18ef70c6`
- `ihatecompvir-extra/re-gh2` at `2aa28d67f7da4d41ae4e3f18129b49b51ffee2fd`
- `ihatecompvir-extra/band3_recomp` at `c51944bd13dfd4cb6df918159fb7136c20f74fb0`
- `ihatecompvir-extra/rb3-latest` is a shallow sparse source copy of
  `ihatecompvir/rb3` master at `41719f248995f677ffa39bd394706b5d18ef70c6`.
- `ihatecompvir-extra/rb3-retail-old` is a shallow sparse source copy of
  `ihatecompvir/rb3` tag `retail-old` at
  `ae71945afad4d3b12bb34f4d71aecc4750334105`.
- `ihatecompvir-extra/grim` is a narrow source copy of `ihatecompvir/grim`
  commit `1c05ca3d00eaafb4b522435bbb1b8a554c0484bb` containing the
  `char_bones_samples`, `char_clip`, and `char_clip_samples` loaders.
- `ihatecompvir-extra/grim-dev` is a narrow source copy of `ihatecompvir/grim`
  branch `dev` commit `40dd809e46390a3c2952affdcc19b5c48857fb03`
  containing the GH2 `char_hair` loader plus matching `char_clip` parser rows.
- `ihatecompvir-extra/re-notes` is a narrow source copy of
  `ihatecompvir/re-notes` commit `5c486fd6e5e5186c0797df9c84182b056672b3f0`
  containing GH2 `CharClipSamples` notes and the matching 010 templates.
- 2026-07-12 upstream checks still match the local source snapshot:
  `MiloEditor` main `3ebffb1c4391dd83c5765cb428eef433dffaff51`,
  `glTFMilo` main `3c02a5497ede1a5d61023fb066cc8bfbe2e8a8e4`,
  `rb3` master `41719f248995f677ffa39bd394706b5d18ef70c6`,
  `rb3` tag `retail-old` `ae71945afad4d3b12bb34f4d71aecc4750334105`,
  `re-gh2` main `2aa28d67f7da4d41ae4e3f18129b49b51ffee2fd`, and
  `band3_recomp` main `c51944bd13dfd4cb6df918159fb7136c20f74fb0`,
  `grim` main `1c05ca3d00eaafb4b522435bbb1b8a554c0484bb`, and
  `grim` dev `40dd809e46390a3c2952affdcc19b5c48857fb03`, and
  `re-notes` main `5c486fd6e5e5186c0797df9c84182b056672b3f0`.
  The imported `grim` loader snapshot adds reviewable Rust source for
  `CharClipSamples`, `CharClip`, and `CharBonesSamples` file-structure details
  that were missing from the checked C++ bodies.
  The imported `grim-dev` snapshot adds reviewable Rust source for GH2
  `CharHair` version 2 file-structure details that were missing from the
  checked C++ bodies.

The contract test verifies that every copied
`ihatecompvir-extra/rb3-latest/src/system/char` `.cpp`/`.h` file is named in
this map. If another ihatecompvir character source file is added to the local
snapshot, document it here before treating its behavior as understood.

The same contract also guards the selected `rb3-latest/src/system/rndobj`
source files that make up the character-model assembly boundary:
`Draw.cpp`, `Draw.h`, `Group.cpp`, `Group.h`, `Mat.cpp`, `Mat.h`,
`Mesh.cpp`, `Mesh.h`, `MeshAnim.cpp`, `MeshAnim.h`, `MeshDeform.cpp`,
`MeshDeform.h`, `MultiMesh.cpp`, `MultiMesh.h`, `MultiMeshProxy.cpp`,
`MultiMeshProxy.h`, `Poll.cpp`, `Poll.h`, `PollAnim.cpp`, `PollAnim.h`,
`Trans.cpp`, `Trans.h`, `TransAnim.cpp`, `TransAnim.h`, `TransProxy.cpp`,
`TransProxy.h`, and `TransRemover.h`. Listing a file here keeps it inside the
source-truth map; it is not a claim that every body in that file is promoted to
native runtime behavior.

Copied headers that primarily declare inheritance, fields, constants, or
missing runtime bodies are accounted for explicitly here: `CharBoneDir.h`,
`CharBones.h`, `CharBonesBlender.h`, `CharBonesMeshes.h`,
`CharBonesSamples.h`, `CharClip.h`, `CharClipDriver.h`, `CharEyes.h`,
`CharForeTwist.h`, `CharIKHand.h`, `CharLookAt.h`, `CharPollable.h`, and
`CharUpperTwist.h`. Header declarations are evidence for row shape and
ownership only; they do not promote an undecompiled `Poll`, `Load`, or pose
writer body.

## ihatecompvir Character Implementation Inventory

This inventory is the current ownership map for the copied
`rb3-latest/src/system/char/*.cpp` source files. `ported-visible-source` means
the checked concrete body is mirrored by native helpers or runtime code.
`fenced-runtime-gap` means concrete source was ported where present, but the
class still depends on a missing/declaration-only source body or an unresolved
runtime publisher. `diagnostic-only` means the source is understood only as
editor, audit, or row-shape evidence. `absence-evidence` means the checked
source proves there is no usable runtime class/body to port from that file.

| ihatecompvir file | Native owner | Status |
| --- | --- | --- |
| `Char.cpp` | `ghogx_character_character_source_test` | `ported-visible-source` |
| `Character.cpp` | `ghogx_character_character_source_test` | `ported-visible-source` |
| `CharacterTest.cpp` | `ghogx_character_character_test_source_test` | `diagnostic-only` |
| `CharBlendBone.cpp` | `ghogx_character_blend_bone_source_test` | `fenced-runtime-gap` |
| `CharBone.cpp` | `ghogx_character_char_bones_source_test` | `ported-visible-source` |
| `CharBoneDir.cpp` | `ghogx_character_char_bones_source_test` | `fenced-runtime-gap` |
| `CharBoneOffset.cpp` | `ghogx_character_bone_offset_source_test` | `ported-visible-source` |
| `CharBones.cpp` | `ghogx_character_char_bones_source_test` | `fenced-runtime-gap` |
| `CharBonesBlender.cpp` | `ghogx_character_char_bones_source_test` | `fenced-runtime-gap` |
| `CharBonesMeshes.cpp` | `ghogx_character_char_bones_source_test` | `fenced-runtime-gap` |
| `CharBonesSamples.cpp` | `ghogx_character_char_bones_source_test` | `fenced-runtime-gap` |
| `CharBoneTwist.cpp` | `ghogx_character_bone_twist_source_test` | `ported-visible-source` |
| `CharClip.cpp` | `ghogx_character_clip_driver_flags_test` | `fenced-runtime-gap` |
| `CharClipDisplay.cpp` | `ghogx_character_clip_display_source_test` | `diagnostic-only` |
| `CharClipDriver.cpp` | `ghogx_character_clip_driver_flags_test` | `fenced-runtime-gap` |
| `CharClipGroup.cpp` | `ghogx_character_clip_set_source_test` | `fenced-runtime-gap` |
| `CharClipSet.cpp` | `ghogx_character_clip_set_source_test` | `fenced-runtime-gap` |
| `CharCollide.cpp` | `ghogx_character_char_collide_source_test` | `ported-visible-source` |
| `CharCuff.cpp` | `ghogx_character_cuff_source_test` | `fenced-runtime-gap` |
| `CharDriver.cpp` | `ghogx_character_clip_driver_flags_test` | `fenced-runtime-gap` |
| `CharDriverMidi.cpp` | `ghogx_character_clip_driver_flags_test` | `fenced-runtime-gap` |
| `CharEyeDartRuleset.cpp` | `ghogx_character_eye_dart_ruleset_source_test` | `ported-visible-source` |
| `CharEyes.cpp` | `ghogx_character_eyes_source_test` | `fenced-runtime-gap` |
| `CharFaceServo.cpp` | `ghogx_character_face_servo_source_test` | `fenced-runtime-gap` |
| `CharForeTwist.cpp` | `ghogx_character_fore_upper_twist_source_test` | `ported-visible-source` |
| `CharGuitarString.cpp` | `ghogx_character_guitar_string_source_test` | `ported-visible-source` |
| `CharHair.cpp` | `ghogx_character_char_hair_source_test` | `fenced-runtime-gap` |
| `CharIKFingers.cpp` | `ghogx_character_ik_fingers_source_test` | `fenced-runtime-gap` |
| `CharIKFoot.cpp` | `ghogx_character_ik_foot_source_test` | `fenced-runtime-gap` |
| `CharIKHand.cpp` | `ghogx_character_ik_hand_source_test` | `ported-visible-source` |
| `CharIKHead.cpp` | `ghogx_character_ik_head_source_test` | `fenced-runtime-gap` |
| `CharIKMidi.cpp` | `ghogx_character_ik_midi_source_test` | `fenced-runtime-gap` |
| `CharIKRod.cpp` | `ghogx_character_ik_rod_source_test` | `fenced-runtime-gap` |
| `CharIKScale.cpp` | `ghogx_character_ik_scale_source_test` | `fenced-runtime-gap` |
| `CharIKSliderMidi.cpp` | `ghogx_character_ik_slider_midi_source_test` | `fenced-runtime-gap` |
| `CharInterest.cpp` | `ghogx_character_interest_source_test` | `fenced-runtime-gap` |
| `CharLipSync.cpp` | `ghogx_character_lip_sync_source_test` | `fenced-runtime-gap` |
| `CharLipSyncDriver.cpp` | `ghogx_character_lip_sync_source_test` | `fenced-runtime-gap` |
| `CharLookAt.cpp` | `ghogx_character_lookat_source_test` | `fenced-runtime-gap` |
| `CharMeshCacheMgr.cpp` | `ghogx_character_mesh_cache_source_test` | `fenced-runtime-gap` |
| `CharMeshHide.cpp` | `ghogx_character_mesh_hide_source_test` | `ported-visible-source` |
| `CharMirror.cpp` | `ghogx_character_mirror_source_test` | `fenced-runtime-gap` |
| `CharNeckTwist.cpp` | `ghogx_character_neck_twist_source_test` | `ported-visible-source` |
| `CharPollGroup.cpp` | `ghogx_character_poll_group_source_test` | `ported-visible-source` |
| `CharPosConstraint.cpp` | `ghogx_character_pos_constraint_source_test` | `ported-visible-source` |
| `CharServoBone.cpp` | `ghogx_character_char_bones_source_test` | `fenced-runtime-gap` |
| `CharSleeve.cpp` | `ghogx_character_sleeve_source_test` | `ported-visible-source` |
| `CharTaskMgr.cpp` | `ghogx_character_clip_display_source_test` | `diagnostic-only` |
| `CharTransCopy.cpp` | `ghogx_character_trans_copy_source_test` | `ported-visible-source` |
| `CharTransDraw.cpp` | `ghogx_character_trans_draw_source_test` | `ported-visible-source` |
| `CharUpperTwist.cpp` | `ghogx_character_fore_upper_twist_source_test` | `ported-visible-source` |
| `CharUtl.cpp` | `ghogx_character_char_utl_source_test` | `ported-visible-source` |
| `CharWeightable.cpp` | `ghogx_character_weight_setter_source_test` | `ported-visible-source` |
| `CharWeightSetter.cpp` | `ghogx_character_weight_setter_source_test` | `fenced-runtime-gap` |
| `ClipCollide.cpp` | `ghogx_character_clip_editor_source_test` | `diagnostic-only` |
| `ClipCompressor.cpp` | `ghogx_character_clip_editor_source_test` | `absence-evidence` |
| `ClipGraphGen.cpp` | `ghogx_character_clip_editor_source_test` | `diagnostic-only` |
| `FileMerger.cpp` | `ghogx_character_clip_editor_source_test` | `diagnostic-only` |
| `Waypoint.cpp` | `ghogx_character_waypoint_source_test` | `fenced-runtime-gap` |

## Source Coverage Matrix

| Area | ihatecompvir evidence | Native status |
| --- | --- | --- |
| Object and property-tree skip/read shape | `Object.cs`, `DTBNode.Read` | Parser authority; native skips must mirror source enum and logs standalone generic `Object` rows. |
| Character/BandCharacter/RndDir/ObjectDir root body | `rb3-latest` `Character.cpp`, `rndobj/Dir.cpp`, `obj/Dir.cpp`, `obj/Dir.h` | Native helper ports visible Character/RndDir/ObjectDir loader, sync, find, subdir, copy, handler, and prop-row contracts while keeping the raw GH2 root byte span opaque until the exact GH2 revision/body relation is pinned. |
| Character lifecycle and directory sync flow | `rb3-latest` `Character.cpp`, `Character.h` | Native helper ports constructor defaults, poll-state enum order, Enter/Exit/Poll state changes, main-driver discovery, sphere-base replacement, eyes gates, and SyncObjects cleanup/sort flow. |
| Character subsystem init/terminate | `rb3-latest` `Char.cpp`, `Char.h` | Native helper records the source init/terminate order only; it does not install callbacks or alter runtime startup. |
| Character test harness defaults | `rb3-latest` `CharacterTest.cpp`, `CharacterTest.h` | Native helper ports editor/test defaults, draw/poll decisions, `AddDefaults` controller creation names and offsets, walk/teleport/start-end/load gates, and move-self delegation. This is harness evidence only, not a live controller or playback import. |
| Transformable local/world composition | `RndTrans.cs`, `Trans.cpp`, `Trans.h` | Runtime authority for parent/constraint world rows. |
| Transform proxy attachment | `rb3-latest/src/system/rndobj/TransProxy.cpp` / `TransProxy.h` | Native helper ports source defaults, load gates, sync parent-resolution flow, setter/save/copy/handler/prop rows; it does not create live proxy attachments. |
| Transform animation rows | `rb3-latest/src/system/rndobj/TransAnim.cpp` / `TransAnim.h` | Native helper ports source defaults, load gates, key-owner copy/replace decisions, SetFrame/SetKey call flow, handlers, and prop rows; `MakeTransform` remains fenced because the checked body is an assertion stub. |
| Mesh vertex animation rows | `rb3-latest/src/system/rndobj/MeshAnim.cpp` / `MeshAnim.h` | Native helper ports source defaults, key-count sizing, load/copy/replace, SetFrame interpolation/sync gates, shrink helpers, handlers, and prop rows; it does not enable live vertex animation in the character renderer. |
| Animation base and task rows | `rb3-latest/src/system/rndobj/Anim.cpp` / `Anim.h` | Native helper ports source defaults, load gates, rate-unit mapping, frame conversion, copy/handler/property rows, `OnAnimate` mode selection, and `AnimTask` range/timing setup; it does not enable the full task scheduler. |
| Poll animation cadence | `rb3-latest/src/system/rndobj/Poll.cpp` / `Poll.h`, `PollAnim.cpp` / `PollAnim.h` | Native helper ports source poll message rows, poll-animation child lifecycle, rate-to-frame mapping, load/copy/handler/prop rows; it does not change runtime scheduling. |
| Property animation rows | `rb3-latest/src/system/rndobj/PropAnim.cpp` / `PropAnim.h`, `PropKeys.cpp` / `PropKeys.h` | Native helper ports source property-key load/copy/frame/key/path/value/handler/property-sync contracts; it does not enable live property animation playback. |
| Drawable visibility, bounds, and draw gates | `rb3-latest/src/system/rndobj/Draw.cpp` / `Draw.h`, `MiloEditor` `RndDrawable.cs` | Native helper ports source defaults, revision ceiling, load gates, draw/budget culling, copy, collision, handler, and property rows; no material sort/depth override is inferred from drawable state. |
| Transform copy controller | `rb3-latest` `CharTransCopy.cpp` / `CharTransCopy.h` | Native helper ports the complete null-gated local-transform copy and dependency publication behavior; no stock runtime hookup is promoted without rows. |
| Group membership and LOD selection | `RndGroup.cs`, `rb3-latest/src/system/rndobj/Group.cpp` / `Group.h` | Runtime/draw membership must use decoded object rows. Native helper ports source defaults, copy, replace, handler, and prop-sync rows without changing live draw order. |
| Mesh hide visibility rows | `rb3-latest` `CharMeshHide.cpp` / `CharMeshHide.h` | Native helper ports `HideAll` flag aggregation and `HideDraws` visibility gating; no renderer hookup is promoted until stock rows are proven. |
| Translucent character draw controller | `rb3-latest` `CharTransDraw.cpp` / `CharTransDraw.h`, `Character.h` draw-mode enum | Native helper ports source draw-mode command order only; it does not change renderer sorting or material state. |
| Mesh deformation rows | `rb3-latest/src/system/rndobj/MeshDeform.cpp` / `MeshDeform.h` | Native helper ports visible defaults, vertex-array resize/clear, `SetMesh`, and handler rows; load/copy/reskin bodies remain fenced because they are declared but not visible in the checked source. |
| Multi-mesh instancing/proxy rows | `rb3-latest/src/system/rndobj/MultiMesh.cpp` / `MultiMesh.h`, `MultiMeshProxy.cpp` / `MultiMeshProxy.h` | Native helper ports visible defaults, load/copy, `SetMesh`, handlers, prop-sync, proxy draw/update, and proxy failure rows; it does not create live instanced rendering or proxy ownership. |
| Cuff/accessory deformation rows | `rb3-latest` `CharCuff.cpp` / `CharCuff.h` | Native helper ports constructor defaults, source eccentricity math, revision defaults, and the source `AddBoneChildren` bone-prefix recursion rule; deformation and mesh hookup remain unwired without complete source bodies/stock rows. |
| Blend-bone constraints | `rb3-latest` `CharBlendBone.cpp` / `CharBlendBone.h` | Native helper ports constructor/constraint defaults, load field order, and dependency publication; the checked source does not include the blend `Poll` body. |
| Sleeve secondary motion | `rb3-latest` `CharSleeve.cpp` / `CharSleeve.h` | Native helper ports source defaults, poll math, teleport reset, top-sleeve write, and dependency publication; no live runtime hookup is promoted without decoded rows. |
| Mesh palette, offsets, and group sections | `RndMesh.cs`, `Mesh.cpp` | Parser keeps raw source rows; runtime-active skinning palette follows RB3 `RndMesh` null/invalid bone trimming. |
| Material render state | `RndMat.cs`, `Mat.cpp`, `Mat.h` | Blend, z write, alpha, wrap, and draw order come from source rows. |
| Texture object row inventory | `rb3-latest` `Tex.cpp` / `Tex.h`, `Bitmap.cpp`, `ChunkStream.cpp`, `FilePath.h`, `BinStream.*` | Decode/log stock `Tex` metadata rows, cached bitmap headers, and source-backed payload byte boundaries; texture upload stays on the existing PS2 image asset path. |
| Rnd utility animation rows | `rb3-latest` `AnimFilter.cpp` / `Anim.cpp` | Decode/log stock `AnimFilter` rows; no trigger or animation runtime hookup. |
| Event trigger row inventory | `rb3-latest` `EventTrigger.*`, `ObjVector.h`, `ObjPtr_p.h`, `BinStream.*` | Decode/log stock source fields only; trigger scheduling and the GH2 v8 four-byte zero tail remain fenced. |
| Fenced stock object rows | RB2 dump `CharWalk.cpp` / `OutfitLoader.cpp`, `DirLoader` `WorldFx` fixup refs | Native records opaque row names, types, sizes, and byte prefixes, but does not decode or run them unless the exact source load path is present. |
| Hair row decode and simulation boundary | `glTFMilo` hair builder, `rb3-latest` `CharHair.*` / `CharCollide.*`, `band3_recomp` symbols | Decode/log source rows and run the checked source poll/reset/sim state path; no point writeback until `Hookup(ObjPtrList<CharCollide>&)` is faithfully ported. |
| Eyes/look-at controllers | `CharEyes.cpp`, `CharLookAt.cpp`, `CharInterest.cpp` / `CharInterest.h`, `CharEyeDartRuleset.cpp` / `CharEyeDartRuleset.h` | Decode/log GH2 rows through the source `CharWeightable` + `source`/`pivot`/`dest` order; native helpers port `CharLookAt` poll gating, `CharEyes` load/copy/state/dependency/handler/property rows, plus `CharInterest` / `CharEyeDartRuleset` data decisions; no synthetic eye runtime bridge. |
| Character mesh cache | `rb3-latest` `CharMeshCacheMgr.cpp` / `CharMeshCacheMgr.h` | Native helper ports constructor defaults, disabled-state capture, membership checks, bounded `GetVerts`, visible `SyncMesh` index behavior, and mesh-list stuffing. It is bookkeeping-only and does not alter live renderer/cache ownership. |
| FaceFX/lip-sync boundary | `rb3-latest` `CharFaceServo.*`, `CharLipSync.*`, `CharLipSyncDriver.*`; stock GH2 `FaceFxLipSyncServo` inventory | `CharFaceServo` and `CharLipSync` are source context, not matching `FaceFxLipSyncServo` load bodies; native FAC/viseme lookup stays bounded compatibility. |
| Position constraints | `rb3-latest` `CharPosConstraint.cpp` / `CharPosConstraint.h` | Decode/log source, targets, and box rows; native `Poll` ports the source target/source delta clamp and writes target world rows. |
| Waypoint clip/path diagnostics | `rb3-latest` `Waypoint.cpp` / `Waypoint.h` | Native helper ports source defaults/load/copy, prop sync, handlers, and `ShapeDeltaBox` / `ShapeDeltaAng` / `Constrain` math for diagnostics; no live camera/path behavior is invented. |
| Bone offsets | `rb3-latest` `CharBoneOffset.cpp` / `CharBoneOffset.h` | Decode/log source destination and offset rows; native helper ports source `Poll`/`ApplyToLocal` math without adding an unproven frame-cadence write. |
| Bone twist controller | `rb3-latest` `CharBoneTwist.cpp` / `CharBoneTwist.h` | Decode/log source bone, targets, and weight rows; native helper ports source target-average twist solve without adding an unproven frame-cadence write. |
| Hand/head/foot IK, IK MIDI, slider MIDI, and IK fingers | `CharIKHand.cpp`, `rb3-latest` `CharIKHead.cpp` / `CharIKHead.h`, `CharIKFoot.cpp` / `CharIKFoot.h`, `CharIKMidi.cpp` / `CharIKMidi.h`, `CharIKSliderMidi.cpp` / `CharIKSliderMidi.h`, `CharIKFingers.cpp` / `CharIKFingers.h` | Native hand IK follows source dataflow; IK head helpers port source defaults, dependency publication, point-chain rebuilding, load gates, and copy flow without inventing the absent `Poll` body; IK foot helpers port source helper-target setup, FSM, load gates, and delegation plan without inventing row hookup; IK MIDI rows decode/log the source `mBone` and revision-gated legacy/anim blend fields and now expose source Enter/PollDeps/copy/handler/prop-sync plans while fencing the absent `Poll` / `NewSpot` bodies; IK slider MIDI helpers port source defaults, dependency publication, setup reset, load gates, and copy flow without inventing the absent `Poll` / `SetFraction` bodies; IK fingers helpers port source defaults, left/right finger transform names, setup completeness, visible SetFinger/ReleaseFinger state writes, load gates, and copy flow without promoting the incomplete `Poll` / `MeasureLengths` path. |
| IK scale controller | `rb3-latest` `CharIKScale.cpp` / `CharIKScale.h` | Native helper ports constructor defaults, source poll gate, capture-before/after scale rows, and dependency publication; the checked source `Poll` body has no implemented scale write. |
| Clip drivers | `rb3-latest` `CharDriver.cpp` / `CharDriver.h`, `CharDriverMidi.cpp` / `CharDriverMidi.h`; `CharWeightable.cpp`; `ObjPtr_p.h`; RB2 dump `CharDriver.cpp` | Decode/log driver inventory, inherited weight owner, default clip pointer, parser rows, and blend override gates. Base `CharDriver::Load`/`Poll` bodies are not present in the available source, so runtime clip selection remains source-fenced. |
| Clip groups | `rb3-latest` `CharClipGroup.cpp` / `CharClipGroup.h` | Native shared loader follows source `CharClipGroup::Load`: `Hmx::Object::Load`, `mClips`, `mWhich`, and revision-gated `mFlags`. Handler and prop-sync row plans now mirror the visible source rows. Guitarist active group selection now follows source `CharClipGroup::GetClip` cycling. Flagged `GetClip(int)` selection remains fenced because the available body is not decompiled. |
| Clip set preview/editor container | `rb3-latest` `CharClipSet.cpp` / `CharClipSet.h` | Native helper ports reset/default state, group randomize/sort dispatch, load/pre/post-load, pre/post-save preview handling, handler rows, preview character decisions, frame helpers, and BPM update; it does not promote clip playback runtime. |
| Clip display/task graph diagnostics | `rb3-latest` `CharClipDisplay.cpp` / `CharClipDisplay.h`, `CharTaskMgr.cpp` / `CharTaskMgr.h` | Native helper ports display init, source lookup, text width plus em, bounded start/end bookkeeping, line spacing, and task-graph toggle registration. This is diagnostic/editor-only and does not change runtime clip playback. |
| Clip editor/collision/graph diagnostics | `rb3-latest` `ClipCollide.cpp` / `ClipCollide.h`, `ClipGraphGen.cpp` / `ClipGraphGen.h`, `ClipDistMap.h`, `ClipCompressor.cpp`, `FileMerger.cpp` / `FileMerger.h` | Native helper ports source-visible editor defaults, transition-generation gates, list/test call plans, and merger row defaults only; no live collision, transition graph execution, compression, or file merging behavior is promoted. |
| Weight setters and weight owners | `rb3-latest` `CharWeightable.cpp` / `CharWeightSetter.cpp` | Decode/log source weight rows; full setter `Poll` remains fenced to source driver/evaluate path. |
| Mirror servo controller | `rb3-latest` `CharMirror.cpp` / `CharMirror.h` | Native helper ports constructor defaults, nonzero-weight/nonempty-bones `Poll` gate, servo setter `SyncBones` triggers, dependency publication, load order, and copy flow; `SyncBones` bone rebuilding remains fenced because the body is absent from `rb3-latest`. |
| Rod IK/accessory rods | `rb3-latest` `CharIKRod.cpp` / `CharIKRod.h` | Decode/log source rows; do not synthesize missing destination transforms. |
| Guitar string bend controller | `rb3-latest` `CharGuitarString.cpp` / `CharGuitarString.h`; stock guitar sweep | Native helper ports source `Poll` projection/open-string math and `PollDeps`, but the checked GH2 stock guitar MILOs contain no `CharGuitarString` rows; native does not invent one. |
| Upper/fore/neck twist | `CharUpperTwist.cpp`, `CharForeTwist.cpp`, `rb3-latest` `CharNeckTwist.cpp` / `CharNeckTwist.h` | Native upper/fore passes follow source `Poll` routines; neck twist rows decode/log source load order and port the source `Poll` gates, dependency order, and quaternion-effect angle math, but stock GH2 character inventories currently show zero `CharNeckTwist` rows. |
| Poll groups | `rb3-latest` `CharPollGroup.cpp` | Native helper ports source `Poll`, `ListPollChildren`, and `PollDeps` decision behavior, but stock GH2 base-character inventory contains no `CharPollGroup` rows; native does not invent one. |
| Servo bone driver target | `rb3-latest` `CharServoBone.cpp` / `CharServoBone.h` | Decode/log the `bone.servo` row and `clip_type`; movement remains fenced by clip/CharBones source. |
| Clip sample/output publishing | `rb3-latest` `CharClip` / `CharBones` / `CharBonesSamples` / `CharBone`, `grim` `char_bones_samples/io.rs` / `mod.rs`, `char_clip/io.rs`, `char_clip_samples/io.rs`, `re-notes` `char_bones_samples.bt` / `char_clip_samples.bt` / `charclipsamples.txt`, `MiloEditor` `RndTrans.cs`, `rb3-retail-old` RB2 dump, `band3_recomp` symbols | Channel naming, compression sizing, sample interpolation wrappers, Grim GH2 `CharClipSamples` version gates, legacy full/one/duplicate header order, version-gated weights, raw sample-byte sizing, serialized bone-order sample walking, re-notes active template order `.pos` / `.quat` / `.rotz`, CharBonesSamples load/prop-sync row plans, CharBone output row fields, and partial call flow are source-backed; `.scale`, `.rotx`, `.roty`, sample evaluate, and broad pose publishing remain fenced where source bodies are incomplete. |
| Hair two-sided rendering | User/project visual override | Culling disabled only; not source evidence for material/depth/sort changes. |

## Character Mesh Cache Helper

`CharMeshCacheMgr` coverage is intentionally fenced to the source behavior
visible in `rb3-latest/src/system/char/CharMeshCacheMgr.cpp` and `.h`:

- `MeshCacher` stores the mesh pointer, zeroes `unk4`, and captures the current
  disabled flag at creation time.
- `Disable(bool)` is only valid before any cache entries exist.
- `HasMesh` and `GetVerts` scan the cache in order by mesh identity.
- `SyncMesh` preserves the visible ihatecompvir index/post-increment scan
  behavior and appends a new cacher when that scan reaches the current cache
  size. The source `mask` argument is carried through the native helper, but the
  checked file only exposes it as input to an unknown inlined `MeshCacher` body,
  so native records that body as not visible instead of inventing mesh-cache
  deformation or vertex writeback. Native also records the null-mesh assertion
  instead of dereferencing it.
- `StuffMeshes` publishes cached meshes in stored order.

The native helper uses mesh names and integer vertex tokens only so the
contract can be deterministic. It is not a live renderer cache and does not
change mesh upload, material state, bone posture, or hair/cloth draw behavior.

## Clip Diagnostic Helpers

`CharClipDisplay` and `CharTaskMgr` coverage is diagnostic/editor-only:

- `CharClipDisplay::Init` stores the directory pointer and takes `sEm` from the
  empty-string draw height.
- `FindSource` scans message sources and returns the first source whose sink
  object matches the requested object.
- `SetText` copies the display text and stores drawn text width plus `sEm`.
- `SetClip` sets the clip, then calls `SetText` and `SetStartEnd` with the
  source clip start/end beats and caller flag. The checked source declares
  `SetStartEnd` but does not expose its body, so native records only the
  bounded call inputs.
- `LineSpacing` is exactly `sEm * 2`.
- `CharTaskMgr::Init` registers `toggle_char_task_graph`, and the callback
  toggles the static graph flag and returns the new value.

This slice does not promote clip playback, driver scheduling, overlay drawing,
or runtime task graph rendering.

## Clip Editor/Graph Diagnostic Authorities

- `rb3-latest/src/system/char/ClipGraphGen.cpp` and
  `rb3-latest/src/system/char/ClipGraphGen.h`
  - `GeneratePair` first removes any existing transition nodes from clip A to
    clip B, captures clip A's type definition, and only reaches the
    `on_transition` script branch when a type definition exists, both clips have
    the same type, and `(clipA->mPlayFlags & 0xF0) != 0x10`.
  - In the script branch it clears `mDmap`, publishes `a_clip` and `b_clip`,
    stores `mClipA` and `mClipB`, executes `on_transition`, clears `mDmap`
    again, and calls `SetNodes` only when the script produced a `ClipDistMap`.
    Native `source_clip_graph_generate_pair_step` ports that source-visible
    branch plan; it does not execute data scripts or build a transition graph.
  - `OnGenerateTransitions` uses default values from the source body:
    `max_error=1e30`, `beat_align=0`, `blend_width=1`, `max_facing=0`,
    `max_dist=0`, and `end_dist=0`. It extracts the high play-flag nibble from
    both clips, raises `beat_align` to the lower of those two values, constructs
    `ClipDistMap` with stride `3`, calls `FindDists(max_facing * DEG2RAD,
    restrict)`, then calls `FindNodes(max_error, max_dist, end_dist)`. Native
    `source_clip_graph_on_generate_transitions` ports those parameter decisions
    only.
- `rb3-latest/src/system/char/ClipDistMap.h`
  - The checked source exposes the constructor, `FindDists`, `FindNodes`, and
    `SetNodes` signatures plus storage fields, but not method bodies. Native
    therefore treats `ClipDistMap` as a named destination for the graph helper
    plan, not as implemented transition-distance math.
- `rb3-latest/src/system/char/ClipCollide.cpp` and
  `rb3-latest/src/system/char/ClipCollide.h`
  - Constructor defaults are source-visible: position starts at `front`,
    `mClip` is null, `mWorldLines=false`, `mMoveCamera=true`, `mCharPath` is
    empty, and the graph object is acquired through `RndGraph::Get`.
  - `Load` accepts source revisions through 1, reads `Hmx::Object`, character,
    character path, waypoint, and position, then clears `mClip`. Native
    `source_clip_collide_load_plan` records this exact row order and reset.
  - `SetTypeDef` calls the base object setter only when the type definition
    changes. When the new type definition is non-null, source requires a
    `modes` array and copies `modes[1][0]` into `mMode`. Native
    `source_clip_collide_set_type_def_step` ports that decision without
    executing scripts or mutating a live editor object.
  - `ValidWaypoint` and `ValidClip` send `valid_waypoint` / `valid_clip`
    messages and treat an unhandled response as valid. `ValidClip` returns true
    immediately when no waypoint is selected. Native
    `source_clip_collide_valid_waypoint` and
    `source_clip_collide_valid_clip` port those handler-default decisions.
  - `SyncChar` only calls `SetProxyFile` when a character exists, the selected
    path is non-empty, and it differs from the current proxy; it always follows
    with `SyncWaypoint`. Native `source_clip_collide_sync_char_step` ports that
    decision.
  - `Demonstrate` requires character, waypoint, and clip. When complete, it
    syncs the waypoint and calls `Play(mClip, 2, -1.0f, 1e30f, 0.0f)`. Native
    `source_clip_collide_demonstrate_step` ports those call parameters without
    starting playback.
  - `TestClips` runs every valid clip through the four source directions
    `front`, `back`, `left`, and `right`. Native
    `source_clip_collide_test_clips_plan` records that deterministic call plan.
  - `OnListClips` and `OnListWaypoints` count valid objects, allocate that
    count, write a null first slot, then write valid entries starting at index
    one. Native `source_clip_collide_list_objects_plan` preserves this
    source-visible allocation/index plan as diagnostic evidence rather than
    using it as a runtime container implementation. `OnListReport` is separate
    and allocates `reports + 1`.
- `rb3-latest/src/system/char/FileMerger.cpp` and
  `rb3-latest/src/system/char/FileMerger.h`
  - The checked source exposes constructor/default row behavior and property
    sync rows, but not the load/merge bodies. Native
    `source_file_merger_default_state`,
    `source_file_merger_merger_default_state`, and
    `source_file_merger_merger_copy_plan` port only the visible defaults and
    member copy list. This is not live async loading, filtering, or merge
    behavior.
- `rb3-latest/src/system/char/ClipCompressor.cpp`
  - The checked source contains only `unusedclipcompressor()` calling
    `MakeString("%s %f %f", ...)`. Native
    `source_clip_compressor_evidence` records that absence so compression is not
    inferred from the filename.

## Character Test Harness Helper

`CharacterTest` is useful source evidence for Harmonix's editor/test setup, but
it is not the live character playback path:

- Constructor defaults set `mShowDistMap=none`, zero transition/test toggles,
  `mCycleTransition=true`, and request the `char_test` overlay.
- Destructor clears the overlay callback, hides the overlay, and restarts the
  timer only when the overlay callback is this test object.
- `Draw` highlights the driver when either clip pointer is set, uses
  `bone_head` when present, and otherwise falls back to the character itself.
- `Poll` only enters the clip branch when the driver has a clip directory and
  `mClip1` is set. The helper records the click-cue load/restore, metronome
  edge, `PlayNew` decisions, zero-travel servo regulate clear, and recenter
  request without creating a runtime polling bridge.
- `AddDefaults` creates `main.drv`, `bone.servo`, `foreTwist_L.ik`,
  `foreTwist_R.ik`, `upperTwist_L.ik`, and `upperTwist_R.ik` only when the
  corresponding source object is missing and the source bones are present.
  Source foretwist offsets are `+90` for left and `-90` for right.
- `SetStartEndBeat`, `SetMoveSelf`, and `Load` are represented as deterministic
  source decisions. The checked source still lacks bodies for `PlayNew`,
  `Recenter`, `UpdateOverlay`, `Handle`, and `PropSync`, so native does not
  infer those behaviors.

## Character LOD Row Authority

- `rb3-latest/src/system/char/Character.cpp` and
  `rb3-latest/src/system/char/Character.h`
  - `Character::Lod` defaults `mScreenSize` to `0.0f` and initializes both
    group pointers empty. Its copy constructor and assignment operator copy
    only `mScreenSize`, `mGroup`, and `mTransGroup`, and assignment returns the
    destination row.
  - Property sync exposes exactly `screen_size`, `group`, and `trans_group`.
    Native `source_character_lod_*` helpers record those source rows as
    deterministic data-contract evidence only; they do not select LODs, change
    draw membership, or alter renderer behavior.

## Remaining Character Import Checklist

This is the current answer to "what remains to import" from ihatecompvir's
character sources. Mesh row decode, material state, transform composition,
group draw membership, IK hand, upper twist, fore twist, and position
constraints have source-backed native coverage. The unresolved work is the
connected character animation and controller runtime that turns authored clips
into final transform rows.

1. Whole-character clip and pose stack:
   - GH2-era `CharClipSamples` / `CharBonesSamples` file loading now follows
     Grim's source-backed version gates, full/one/duplicate header order,
     version-gated weights, raw sample-byte sizing, and serialized bone-order
     sample walking. Grim's visible decode body publishes `.pos`, `.quat`, and
     `.rotz`; re-notes' active `Sample` template also counts `.pos`, `.quat`,
     and `.rotz` while exposing only a generic scalar `RotSample`. Native
     consumes `.scale`, `.rotx`, and `.roty` sample bytes without pose
     writeback, and treats `.rotx` / `.roty` as requiring the still-missing
     `EvaluateChannel` / pose body before they can be considered fully
     understood. The remaining missing
     source-backed bodies are `CharBonesSamples::EvaluateChannel`, `Relativize`,
     and the complete pose application path. `rb3-latest` declares those
     functions, while the RB2 dump maps names/ranges and locals but does not
     expose a statement-level body.
   - `ghogx_character_clip_audit` now reports raw `.pos`, `.scale`, `.quat`,
     `.rotx`, `.roty`, and `.rotz` header counts plus `fencedRaw`, so stock
     clips can prove which rows are consumed-only under the current source
     boundary.
   - Port the missing source-backed bodies for `CharBones::ScaleAdd`,
     `RotateBy`, `RotateTo`, `Blend`, and any required identity/mesh
     application helpers.
     Current evidence is also not enough to copy these: `rb3-latest`
     declares the pose writers but only implements the `ScaleAdd(CharClip*)`
     delegation, while the RB2 dump maps broad pose writer names and locals
     without statement-level bodies.
   - Port the missing source-backed bodies for `CharClip::Load`,
     `ScaleAdd`, `RotateBy`, and related `FacingSet` behavior.
   - Port `CharClipDriver::Evaluate`/poll timing, blend, loop, beat-align,
     and exit behavior from a reviewable source body or direct original-game
     trace. The current `rb3-latest` file only exposes stack construction and
     flag masking; the RB2 dump gives a function map, not enough standalone C++
     body to blindly copy.
   - Port base `CharDriver::Load`, `CharDriver::Poll`, and `EvaluateFlags`
     before treating source drivers as active clip selection. The visible
     `CharDriverMidi` parser/message-sink/load/copy/property decisions are
     already ported as deterministic helpers, but they still delegate to the
     missing base driver and clip/pose stack for actual playback. Until those
     base bodies exist, native viewer clip playback is diagnostic/application
     glue, not proof of the Harmonix driver runtime.

2. Pose-dependent controller publishing:
   - `CharWeightSetter::Poll` depends on `CharDriver::EvaluateFlags`; do not
     promote it beyond decode/log and the bounded live-performer fallback until
     the driver path above is source-backed.
   - `CharServoBone` movement and broad `CharBonesMeshes` writes depend on the
     same clip/bone output path. Keep them fenced until whole-character
     `CharBones` application is source-backed.
   - `CharFaceServo` is useful source context, but GH2 stock rows are
     `FaceFxLipSyncServo`; do not infer eye or mouth transforms from it unless
     a matching GH2 source body or direct trace is available.
   - `CharLookAt::Poll` has reviewable source coverage for gate/branch math and
     native deterministic helpers already port the visible yaw-weight,
     source-radius, smoothing, range, and no-roll pieces. Live eye/look-at
     publishing remains fenced because stock GH2 `CharLookAt` rows currently
     decode with `mDest=<none>`, and `CharEyes::Poll` still has only RB2
     range/local evidence. Native must respect the GH2 row shape and avoid the
     removed synthetic eye-row bridge.

3. Hair and cloth writeback:
   - Decode, reset, and simulation-state coverage is source-backed, but point
     world-row writeback still needs the overloaded
     `CharHair::Hookup(ObjPtrList<CharCollide, ObjectDir>&)` body, point
     collide-list population, and `SimulateZeroTime` behavior from reviewable
     source or direct original-game trace. The source `Hookup()` wrapper and
     `SimulateLoops` gate are already ported as deterministic plans.
   - The project hair rule is two-sided culling only. It is not permission to
     change depth priority, material sorting, or material state from mesh names.

4. Stock evidence still needed after imports:
   - Re-run the 24 base stock character screenshot/log sheet after each clip or
     controller import.
   - Specifically re-check Rock1/Rock2 side profile arm and neck posture,
     Rockabill2 eyes/teeth/mouth pieces, and visible hair/cloth on characters
     with decoded `CharHair` rows.
   - Keep `metal_keyboard` out of broad pose signoff until the source driver
     runtime is ported, but do not treat it as a sample decode failure. The
     focused clip audit found its real `keyboard_main` clips under
     keyboard-specific names; the old `idle_medium_01`/`stand_medium_01` miss is
     a viewer route issue.

## Binary Layout Authorities

- `MiloEditor/MiloLib/Assets/Object.cs`
  - `ObjectFields.Read` reads a combined object revision, subtype `Symbol`,
    root DTB parent tree, and an optional note `Symbol` when object revision is
    greater than zero.
  - `DTBNode.Read` defines the property-tree node payloads. The native readers
    only skip these trees, but the skip table must mirror this enum exactly.
  - Native generic `Object` rows decode the same `ObjectFields` prefix and log
    unread tails instead of promoting any runtime behavior.
- `rb3-latest/src/system/char/Character.cpp`
  - `Character::PreLoad` reads packed revisions, asserts source revision
    `0x11`, and for revisions greater than 1 delegates to `RndDir::PreLoad`.
  - `Character::PostLoad` delegates to `RndDir::PostLoad`, then reads
    character-owned LOD, shadow, self-shadow, sphere base, bounding, frozen,
    min LOD, trans group, and test rows behind source revision gates.
  - Native `source_character_load_plan` records the `PreLoad` / `PostLoad`
    branch order for source revisions `0..0x11`, including the legacy
    `somerev` path, proxy-only test loading, pre-revision-7 rate default,
    pre-revision-8 LOD screen-size scaling, and the old `lod%d.grp` rename
    branch. This helper is a deterministic source contract only; it does not
    decode or apply the root `Character` byte span.
  - Native `source_character_copy_plan` records the checked
    `BEGIN_COPYS(Character)` body: copy `RndDir`, create the destination row,
    copy members only when `ty != kCopyFromMax`, and preserve the exact source
    member order, including the duplicated `mMinLod` copy before
    `mTransGroup`. It is a copy-contract helper only and does not run a native
    `Character` copy.
- `rb3-latest/src/system/rndobj/Dir.cpp`
  - `RndDir::PreLoad` reads packed revisions, asserts source revision `0xA`,
    pushes that revision, and delegates to `ObjectDir::PreLoad`.
  - `RndDir::PostLoad` delegates to `ObjectDir::PostLoad`, then loads the
    source superclasses `RndAnimatable`, `RndDrawable`, and revision-gated
    `RndTransformable` before environment/test/postproc rows.
  - `RndDir::SyncObjects` clears anim/poll rows, skips the rest for subdirs,
    otherwise syncs drawables, gathers animatables and pollables, removes
    child-owned rows, sorts polls, optionally chains a message-source parent,
    and then calls `ObjectDir::SyncObjects`. `SyncDrawables` similarly clears
    draw rows, skips subdirs, gathers drawables, updates pre-clear state,
    removes draw children, and sorts draws.
  - Native `source_rnddir_load_plan`,
    `source_rnddir_sync_objects_plan`,
    `source_rnddir_sync_drawables_plan`, `source_rnddir_copy_plan`,
    `source_rnddir_handler_plan`, and `source_rnddir_prop_sync_plan` record
    those visible source decisions without creating a live native `RndDir`.
- `rb3-latest/src/system/obj/Dir.cpp` and `Dir.h`
  - `ObjectDir::PreLoad` reads packed revisions, asserts source revision
    `0x1B`, then consumes revision-gated object/type prefix, reserve/hash,
    inline/proxy, viewport, and subdir state before pushing the revision.
  - `ObjectDir::PostLoad` pops the revision and resolves inlined dirs,
    subdirs, and proxy state.
  - `ObjectDir::ObjectDir` defaults `mProxyOverride=false`,
    `mInlineProxy=true`, null loader/current camera/path/inline hash, clears
    `mIsSubDir`, and sets `mInlineSubDirType=kInlineNever`.
  - `FindObject` searches the local hash, then subdirs, then self name, then
    parent dirs, then main dir. `SetSubDir(true)` clears name and type, while
    `AddedSubDir` / `RemovingSubDir` publish nested object additions/removals.
  - Native `source_object_dir_default_state`,
    `source_object_dir_preload_plan`, `source_object_dir_postload_plan`,
    `source_object_dir_find_object_plan`, and `source_object_dir_subdir_plan`
    record those concrete source contracts without replacing the existing
    native MILO object table.

## Character Root Body Boundary

The native MILO parser already identifies the root directory's own object body:
the bytes after the entry name list and before the first `0xADDEADDE`
terminator. For `Character`/`BandCharacter` roots this body belongs to the
source loader chain above (`Character::PreLoad` -> `RndDir::PreLoad` ->
`ObjectDir::PreLoad`, and the matching `PostLoad` chain). Native now carries
this bounded byte span on `Character` and logs it as `[dir-entry]` under
`ghogx_character_bind_audit --types`.

Do not decode or apply root `Character`, `RndDir`, or `ObjectDir` runtime fields
from this byte span until the GH2-era root revision/body relationship is proven
from ihatecompvir source or equivalent trace evidence. The current source-backed
deliverable for the raw byte span is inventory only: `dirVersion`, `dirType`,
`bodyOffset`, `bodyBytes`, copied byte count, and head/tail hex proof. The
source helper contracts above describe the visible ihatecompvir loader and sync
flow; they do not claim the opaque GH2 root body is now field-decoded.

2026-07-10 proof log:
`engine/out/source_truth_dir_entry_20260710/stock_character_dir_entry_inventory.log`.

## Character Runtime Flow

- `rb3-latest/src/system/char/Char.cpp` initializes the character subsystem in
  this order: `Character::Init`, `CharBonesObject::Init`,
  `CharBoneOffset::Init`, `PreloadSharedSubdirs("char")`,
  `CharBoneDir::Init`, `CharUtlInit`, then
  `TheDebug.AddExitCallback(CharTerminate)`. `CharTerminate` removes that exit
  callback, terminates `Character`, then terminates `CharBoneDir`. Native
  `source_char_lifecycle_plan` records this order only; it does not install a
  callback, preload assets, or change app startup.
- `rb3-latest/src/system/char/Character.h` defines `PollState` as
  `kCharCreated = 0`, `kCharSyncObject = 1`, `kCharEntered = 2`,
  `kCharPolled = 3`, and `kCharExited = 4`.
- `Character::Character` source defaults include `mLastLod = 0`,
  `mMinLod = 0`, `mDriver = 0`, `mSphereBase = this`,
  `mPollState = kCharCreated`, `mFrozen = false`,
  `mDrawMode = kCharDrawAll`, `mTeleported = true`, and empty
  `mInterestToForce`.
- `Enter` sets entered state, min LOD `-1`, clears frozen, resets last LOD,
  marks teleported, clears forced interest, then delegates to `RndDir::Enter`.
  `Exit` sets exited state and delegates to `RndDir::Exit`.
- `Poll` does nothing while frozen. Otherwise it delegates to `RndDir::Poll`,
  clears `mTeleported`, and records `kCharPolled`.
- `AddedObject` only assigns `mDriver` when the object is both `CharPollable`
  and `CharDriver` and its name is exactly `main.drv`. `RemovingObject` clears
  the current driver before delegating to `RndDir::RemovingObject`.
- `Replace` always delegates to `RndDir::Replace`; if the replaced object is the
  sphere base it uses the replacement transform, or falls back to `this` when
  the replacement is not transformable.
- `SetSphereBase` defaults a null input to `this`, builds the current world
  sphere, multiplies it by the chosen transform world matrix, calls `SetSphere`,
  then stores the transform as `mSphereBase`.
- `SetInterestObjects` is fully gated by `GetEyes`. When eyes exist it clears
  all current interest objects, validates each candidate with either the
  override directory or the candidate's own directory, and only adds validated
  candidates.
- `AddShadowBone` returns null for a null transform, returns an existing
  `ShadowBone` with the same parent when present, otherwise appends one new
  row. `UnhookShadow` deletes every shadow bone row.
- `SyncShadow` always unhooks first. When `mShadow` exists and the old graphics
  mode is active, meshes with source bones retarget each bone through
  `AddShadowBone`; meshes without bones get a shadow parent through
  `SetTransParent(AddShadowBone(mesh))`. The shadow drawable is then removed
  from draws when `mShadow` exists.
- `SyncObjects` records `kCharSyncObject`, converts bones to transforms only
  when `bone_pelvis.mesh` exists, delegates to `RndDir::SyncObjects`, removes
  the trans group plus each LOD group and trans group from draws, syncs shadow,
  and sorts character polls.
- `CharPollableSorter::ChangedBy` / `ChangedByRecurse` are concrete for the
  dependency reachability test used by poll ordering: same dep pointers return
  false without incrementing `sSearchID`; other queries increment the search id,
  recurse through `changedBy`, mark each visited dep with that id, return true
  before marking the target dep, and use the search id to suppress cycles.
  Native `source_char_pollable_sorter_changed_by` ports that reachability body
  only. The broader `Sort` / `AddDeps` object-to-dep construction remains
  fenced because the checked source has incomplete bodies there.
- `CopyBoundingSphere` copies sphere, bounding, and the source sphere-base
  pointer when present, otherwise clears the pointer. `RepointSphereBase` only
  looks up by name when the pointer is non-null and only replaces it when the
  directory lookup succeeds. `PreSave` is just `UnhookShadow`.
- Native `source_character_handler_plan` records the source
  `BEGIN_HANDLERS(Character)` order: `teleport`, `play_clip`,
  `calc_bounding_sphere`, `copy_bounding_sphere`, `find_interest_objects`,
  `force_interest`, `force_interest_named`, `enable_blink`, the debug-only
  `list_interest_objects` / `mTest` rows, the `RndDir` superclass, and
  `HANDLE_CHECK(0x57B)`.
- Native `source_character_prop_sync_plan` records the source property rows:
  set rows `sphere_base`, `shadow`, and `driver`; direct rows `lods`,
  `force_lod`, `trans_group`, `self_shadow`, `bounding`, and `frozen`;
  modifying row `interest_to_force`; debug-only rows
  `debug_draw_interest_objects` and `CharacterTesting`; and the `RndDir`
  superclass.
- Native `source_character_on_play_clip` ports the visible source decision only:
  without `mDriver` it returns false; with `mDriver` it uses message arg 3 as
  play flags only when the message has more than three nodes, otherwise it uses
  `4`, asserts when the message has more than four nodes, and calls
  `mDriver->Play` with blend width `-1.0`, end beat `1e+30`, and start beat
  `0.0`. This still delegates actual clip playback to the source-fenced driver
  runtime.
- Native `source_character_*` helpers port these source-visible runtime flows
  for deterministic tests and future wiring. They do not decode the fenced root
  body bytes above and do not change current renderer/material behavior.
- `MiloEditor/MiloLib/Assets/Rnd/RndTrans.cs`
  - `RndTrans.Read` reads combined revision, optional object fields for
    standalone objects, local matrix, world matrix, old child references for
    revisions below 9, constraint, target, preserve-scale, then parent.
  - Shared native `source_rndtrans_load_plan` records the same gates: standalone
    rows read Object fields while embedded bases do not; local/world matrices
    are always read; revision `< 9` reads the old child list, using
    null-terminated strings for parent directories `<= 6` and symbols
    otherwise; revision `> 6` reads constraint and preserve-scale; revision
    `> 5` reads target; parent is always read.
  - The shared `milo_scene` decoders and the character mesh/collider decoder
    now pass the actual parent directory revision into their embedded
    `RndTrans` reader. The legacy `ReadUTF8` child-list branch is decoded only
    for the source `revision < 9 && parent.revision <= 6` gate; GH2 PS2
    character directories still take the Symbol path, but the reader no longer
    hardcodes that common case.
- `rb3-latest/src/system/rndobj/TransProxy.cpp` and
  `rb3-latest/src/system/rndobj/TransProxy.h`
  - `RndTransProxy::RndTransProxy` nulls `mProxy` and leaves `mPart` default.
    Native `source_rndtrans_proxy_default_state` records those defaults.
  - `RndTransProxy::Load` accepts revisions `0..1`, reads `Hmx::Object`,
    reads `RndTransformable` only when `gRev != 0`, then reads `mProxy`,
    `mPart`, and calls `Sync`. Native `source_rndtrans_proxy_load_plan`
    records those gates.
  - `RndTransProxy::Sync` first clears its transform parent. If `mProxy` is
    present and `mPart` is null, it tries to use the proxy itself as a
    `RndTransformable` parent and returns on success. Otherwise, when `mProxy`
    is present, it searches that object directory for `mPart` as a
    `RndTransformable` and parents to that on success. If neither branch
    resolves a parent, it clears the parent again. Native
    `source_rndtrans_proxy_sync_plan` ports that branch order, including the
    source fallthrough from a null part to a directory lookup when the proxy
    object is not transformable.
  - `SetProxy` and `SetPart` assign and call `Sync` only when the value changes;
    `PreSave` clears the parent and `PostSave` calls `Sync`. Native
    `source_rndtrans_proxy_setter_plan` and `source_rndtrans_proxy_save_plan`
    record those source decisions.
  - `BEGIN_COPYS`, `BEGIN_HANDLERS`, and `BEGIN_PROPSYNCS` copy
    `Hmx::Object`/`RndTransformable`, copy `mProxy` and `mPart`, call `Sync`,
    expose only superclass handlers with check `0x6A`, and sync `proxy` and
    `part` with `Sync`. Native `source_rndtrans_proxy_copy_plan`,
    `source_rndtrans_proxy_handler_plan`, and
    `source_rndtrans_proxy_prop_sync_plan` record those rows without attaching
    runtime model parts.
- `rb3-latest/src/system/rndobj/TransAnim.cpp` and
  `rb3-latest/src/system/rndobj/TransAnim.h`
  - `RndTransAnim::RndTransAnim` nulls `mTrans`, clears trans/scale/rot spline
    flags, defaults `mKeysOwner` to itself, and clears repeat/follow flags.
    Native `source_rndtrans_anim_default_state` records these defaults.
  - `RndTransAnim::Load` accepts revisions up to `7`, reads `Hmx::Object` only
    for revisions above `4`, always reads `RndAnimatable`, dumps old
    `RndDrawable` rows below revision `6`, reads `mTrans`, skips
    rot/trans-key rows only for revision `2`, reads `mKeysOwner`, defaults a
    null owner to `this`, reads a legacy int below revision `3`, reads
    `mFollowPath` above revision `1` or inherits it from the key owner, reads
    `mRotSlerp` above revision `3`, and reads `mRotSpline` above revision `6`.
    Native `source_rndtrans_anim_load_plan` records these gates and keeps
    `mScaleKeys` as copy/runtime evidence only because the visible load body
    does not read scale-key rows.
  - `SetKeysOwner` asserts a non-null owner and assigns it. `Replace` delegates
    to `Hmx::Object::Replace`; only when `mKeysOwner == from` does it set owner
    to `this` for null replacement or copy the replacement `RndTransAnim` key
    owner. Native `source_rndtrans_anim_set_keys_owner_plan` and
    `source_rndtrans_anim_replace_plan` record those decisions.
  - `RndTransAnim::Copy` copies `Hmx::Object`, `RndAnimatable`, and `mTrans`.
    Shallow copies, plus Max copies whose source key owner is external, copy the
    key-owner pointer. Otherwise the destination owns copied trans/rot/scale
    keys, spline flags, repeat/follow flags, `mRotSlerp`, and `mRotSpline`.
    Native `source_rndtrans_anim_copy_plan` records that split.
  - `SetFrame` calls `RndAnimatable::SetFrame`, and only with `mTrans` does it
    copy the local transform, call `MakeTransform(frame, tf, false, blend)`, and
    write the local transform back. The checked `MakeTransform` body asserts
    false, so native `source_rndtrans_anim_set_frame_plan` records call flow
    without promoting transform interpolation.
  - `SetKey` only runs when `mTrans` is present: add a translation key from the
    local translation, normalize the local matrix, add a rotation quaternion key,
    derive scale with `MakeScale`, and add a scale key. Native
    `source_rndtrans_anim_set_key_plan` records this source-visible key path.
  - `BEGIN_HANDLERS(RndTransAnim)` exposes `trans`, `splice`, `linearize`,
    `set_trans`, key removal/count/add rows, spline setters, superclass
    handlers, and check `489`. `BEGIN_PROPSYNCS` exposes `keys_owner` through
    `SetKeysOwner` and `RndAnimatable`. Native
    `source_rndtrans_anim_handler_plan` and
    `source_rndtrans_anim_prop_sync_plan` record those tables without running a
    transform animation solve.
- `rb3-latest/src/system/rndobj/Anim.cpp`
  - `RndAnimatable::Load` accepts revisions `0..4`; the constructor defaults
    frame to `0.0` and rate to `k30_fps`. Revisions above 1 read frame,
    revisions above 3 read integer rate, revision 3 reads the legacy byte-rate
    row, and revision 0 reads old filter rows plus the old anim-list conversion
    branch.
  - Shared native `source_rndanimatable_load_plan` records these gates for
    embedded `RndAnimatable` bases such as `RndGroup` without promoting the
    legacy revision-0 conversion branch into runtime behavior.
- `rb3-latest/src/system/rndobj/Poll.cpp` and
  `rb3-latest/src/system/rndobj/Poll.h`
  - `RndPollable` derives virtually from `Hmx::Object`. `Poll` and
    `ListPollChildren` are empty virtual defaults in the header.
  - `BEGIN_HANDLERS(RndPollable)` exposes action rows `enter` and `poll`, a
    static action row `exit`, and check `0x1A`. `Enter` handles `enter_msg`;
    `Exit` handles `exit_msg`. Native `source_rndpollable_handler_plan` and
    `source_rndpollable_base_plan` record those source rows.
- `rb3-latest/src/system/rndobj/PollAnim.cpp` and
  `rb3-latest/src/system/rndobj/PollAnim.h`
  - `RndPollAnim` derives virtually from `RndAnimatable`, `RndPollable`, and
    `Hmx::Object`; its constructor initializes `mAnims` with no-null list
    ownership. `StartAnim`, `EndAnim`, and `SetFrame` are empty source bodies.
    Native `source_rndpollanim_default_state` and
    `source_rndpollanim_empty_body_plan` record these facts.
  - `EndFrame` returns the maximum child `EndFrame`. `ListAnimChildren`
    publishes every child in `mAnims`. `Enter` calls `StartAnim` on every child,
    and `Exit` calls `EndAnim` on every child. Native
    `source_rndpollanim_end_frame_plan`,
    `source_rndpollanim_child_list_plan`,
    `source_rndpollanim_enter_plan`, and `source_rndpollanim_exit_plan` record
    those loops.
  - `Poll` maps each child `RndAnimatable::Rate` to a frame before calling
    `SetFrame(frame, 1.0f)`: `k30_fps` uses `30 * TheTaskMgr.Seconds`,
    `k480_fpb` uses `480 * TheTaskMgr.Beat`, `k30_fps_ui` uses
    `30 * TheTaskMgr.UISeconds`, `k1_fpb` uses `TheTaskMgr.Beat`, and
    `k30_fps_tutorial` uses `30 * TheTaskMgr.TutorialSeconds`. Unknown/default
    rate leaves the source local frame at `0.0`. Native
    `source_rndpollanim_rate_frame_plan` and `source_rndpollanim_poll_plan`
    record this mapping without changing runtime scheduling.
  - `Load` accepts only revision `0`, reads `Hmx::Object`, `RndAnimatable`,
    `RndPollable`, and `mAnims`. `Copy` copies the same superclasses plus
    `mAnims`. `BEGIN_HANDLERS` delegates to `RndAnimatable`, `RndPollable`, and
    `Hmx::Object` with check `0x8B`. `BEGIN_PROPSYNCS` syncs `anims`, then
    returns `RndAnimatable::SyncProperty` if handled, otherwise falls back to
    `RndPollable::SyncProperty`. Native `source_rndpollanim_load_plan`,
    `source_rndpollanim_copy_plan`, `source_rndpollanim_handler_plan`, and
    `source_rndpollanim_prop_sync_plan` record those rows.
- `rb3-latest/src/system/rndobj/PropAnim.cpp`,
  `rb3-latest/src/system/rndobj/PropAnim.h`,
  `rb3-latest/src/system/rndobj/PropKeys.cpp`, and
  `rb3-latest/src/system/rndobj/PropKeys.h`
  - `RndPropAnim` derives from `RndAnimatable`. Its constructor defaults
    `mLastFrame` to `0.0`, clears `mInSetFrame`, and clears `mLoop`; its
    destructor calls `RemoveKeys`. Native
    `source_rndpropanim_default_state` records those defaults.
  - `RndPropAnim::Load` accepts revisions `0..0xD`, calls
    `SetPropKeysRev`, loads `Hmx::Object` and `RndAnimatable`, captures
    `mLastFrame` from the current animation frame, removes existing keys, uses
    `LoadPre7` below revision 7, otherwise reads a key count, key type per
    entry, each `PropKeys` row, and `mLoop` above revision `0xB`. Native
    `source_rndpropanim_load_plan` and
    `source_rndpropanim_pre7_load_plan` record those gates.
  - `Copy` copies `Hmx::Object` and `RndAnimatable`, refreshes `mLastFrame`
    from `GetFrame`, removes old keys, copies every property-key row, and
    copies `mLoop`. `StartFrame` starts at `0.0` and takes the minimum child
    start; `EndFrame` starts at `0.0` and takes the maximum child end.
    `AdvanceFrame` only applies `ModRange` when looped, then calls
    `RndAnimatable::SetFrame(frame, 1.0f)`. Native
    `source_rndpropanim_copy_plan`,
    `source_rndpropanim_start_frame_plan`,
    `source_rndpropanim_end_frame_plan`, and
    `source_rndpropanim_advance_frame_plan` record those rules.
  - `SetFrame` is guarded by `mInSetFrame`, advances the frame, scans
    `kDirEvent` object keys for event triggers between `mLastFrame` and the new
    frame, sets every key frame, updates `mLastFrame`, and clears the guard.
    `SetKey`, `StartAnim`, `RemoveKeys`, `FindKeys`, `ChangePropPath`,
    `ValueFromFrame`, and `ValueFromIndex` are recorded by bounded native
    helpers without promoting property animation into live character playback.
  - `PropKeys` defaults the target from its owner arguments, nulls the property
    and transform cache, sets `mLastKeyFrameIndex` to `-2`, defaults key type
    to float, interpolation to linear, exception id to none, and clears the
    trailing bit. `PropKeys::Load` is invalid before revision 7, reads key
    type, target, property, revision-gated interpolation, interpolation
    handler, exception id, and the trailing bit, then calls
    `SetPropExceptionID`. Native `source_propkeys_default_state` and
    `source_propkeys_load_plan` record those rows.
  - `PropExceptionID` maps `rotation`, `scale`, and `position` on transform
    subclasses to transform exception ids, and maps `event` on object
    directories to `kDirEvent`. `SetPropExceptionID` gives a non-null
    interpolation handler priority, preserves macro exception ids, otherwise
    derives the property exception and refreshes the transform cache for
    transform exceptions. Native `source_propkeys_exception_plan` and
    `source_propkeys_set_prop_exception_plan` record those branches.
  - `BEGIN_HANDLERS(RndPropAnim)` exposes remove/has/add/set key paths,
    interpolation query/set paths, target iteration, keyframe/frame
    replacement, index/frame/value query paths, then `RndAnimatable` and
    `Hmx::Object` with check `0x43C`. `BEGIN_PROPSYNCS` exposes `loop` and
    `RndAnimatable`. Native `source_rndpropanim_handler_plan` and
    `source_rndpropanim_prop_sync_plan` record those rows.
  - This remains a source contract only. It does not enable live `RndPropAnim`
    playback, and it does not authorize property-animation guesses for eyes,
    mouth, hair, cloth, or accessory placement.
- `rb3-latest/src/system/rndobj/MeshAnim.cpp` and
  `rb3-latest/src/system/rndobj/MeshAnim.h`
  - `RndMeshAnim` derives from `RndAnimatable`. Its constructor nulls `mMesh`
    and sets `mKeysOwner` to itself. Native
    `source_rndmeshanim_default_state` records those defaults.
  - `NumVerts` takes the maximum non-zero key count across vertex points,
    normals, UVs, and colors. Native `source_rndmeshanim_num_verts_plan`
    records the checked source order.
  - `Replace` delegates to `Hmx::Object::Replace`; only when `mKeysOwner ==
    from` does it set the owner to `this` for null replacement or copy the
    replacement `RndMeshAnim` key owner. Native
    `source_rndmeshanim_replace_plan` records that decision.
  - `Load` accepts revisions `0..2`, reads `Hmx::Object` only when revision is
    non-zero, always reads `RndAnimatable`, `mMesh`, point keys, UV keys, color
    keys, and `mKeysOwner`, reads normal keys only for revisions above `1`, and
    defaults a null key owner to `this`. Native
    `source_rndmeshanim_load_plan` records those gates.
  - `Copy` copies `Hmx::Object`, `RndAnimatable`, and `mMesh`. Shallow copies,
    plus Max copies whose source key owner is external, copy the key-owner
    pointer. Otherwise the destination owns copied point, normal, UV, and color
    key streams. Native `source_rndmeshanim_copy_plan` records that split.
  - `EndFrame` returns the maximum last frame across the four key streams.
    `SetFrame` calls `RndAnimatable::SetFrame`, does nothing without `mMesh`,
    warns when the mesh mutable mask has no low `0x1F` bits, otherwise
    evaluates each non-empty key stream through `InterpVertData` and calls
    `mMesh->Sync(0x1F)` when any stream was applied. Native
    `source_rndmeshanim_end_frame_plan`,
    `source_rndmeshanim_interp_plan`, and
    `source_rndmeshanim_set_frame_plan` record that flow without enabling live
    vertex animation in the character renderer.
  - `SetKey` is an empty body in the checked source. `ShrinkVerts` resizes the
    stored value vectors for each key in each stream; `ShrinkKeys` resizes each
    non-empty key stream itself. Native `source_rndmeshanim_set_key_plan`,
    `source_rndmeshanim_shrink_verts_plan`, and
    `source_rndmeshanim_shrink_keys_plan` record those behaviors.
  - `BEGIN_HANDLERS(RndMeshAnim)` exposes `num_verts`, `shrink_verts`,
    `shrink_keys`, superclass handlers for `RndAnimatable` and `Hmx::Object`,
    and check `0x207`. `BEGIN_PROPSYNCS` exposes `mesh` and `RndAnimatable`.
    Native `source_rndmeshanim_handler_plan` and
    `source_rndmeshanim_prop_sync_plan` record those rows.
- `rb3-latest/src/system/rndobj/Draw.cpp`, `Draw.h`, and
  `MiloEditor/MiloLib/Assets/Rnd/RndDrawable.cs`
  - `RndDrawable::RndDrawable` defaults `mShowing` true, zeroes `mSphere`, and
    clears `mOrder`; the source revision ceiling is `DRAW_REV = 3`.
    Native `source_rnddrawable_default_state` records these defaults.
  - `RndDrawable.Read` / `RndDrawable::Load` read combined revision, showing,
    optional old drawable list, optional sphere, and draw order for revisions
    greater than 2. Native `source_rnddrawable_load_plan` records the same
    gates and rejects revisions outside `0..3`; the older local rev4
    clip-plane interpretation is not source-backed by `Draw.cpp`.
  - Native `source_rnddrawable_draw_plan` and
    `source_rnddrawable_budget_plan` port the visible source culling gates:
    hidden drawables stop before sphere work, and visible drawables call
    `DrawShowing` / `DrawShowingBudget` only when there is no world sphere or
    the current camera does not cull it.
  - Native `source_rnddrawable_copy_plan` records the source copy split:
    normal copies include `mShowing`, `mOrder`, and `mSphere`; `kCopyFromMax`
    copies only the sphere and only when both source and destination radii are
    non-zero.
  - Native `source_rnddrawable_collide_plan` records the visible
    `CollideSphere` and `CollidePlane` gates, while
    `source_rnddrawable_handler_plan` and `source_rnddrawable_prop_sync_plan`
    record the source handler/property rows.
  - The shared `RndGroup` decoder now passes the actual parent directory
    revision into embedded `RndDrawable` rows and uses the source
    `ReadUTF8`/Symbol split for old drawable lists. GH2 PS2 character groups
    still take the Symbol path, but the reader no longer assumes that for older
    parent directories.
- `MiloEditor/MiloLib/Assets/Rnd/RndMat.cs`
  - For material revisions above 21, `RndMat.Read` reads `useEnviron`,
    `preLit`, `zMode`, `alphaCut`, optional `alphaThreshold`, `alphaWrite`,
    `texGen`, `texWrap`, texture transform, diffuse texture, next pass,
    intensify, cull, and emissive multiplier in that order.
- `MiloEditor/MiloLib/Assets/Rnd/RndGroup.cs`
  - `RndGroup.Read` reads Object fields for revisions above 7, then
    `RndAnimatable`, embedded `RndTrans`, embedded `RndDrawable`, then the
    explicit object `Symbol` list for revisions above 10. Native LOD and draw
    membership must come from this source object list, not broad string scans.
  - Shared native `source_rndgroup_load_plan` records the revision gates:
    object fields for revisions above 7; regular object lists for revisions
    above 10; environ for `> 10 && < 16`; draw-only for revisions above 12; LOD
    for `> 11 && < 16`; legacy object rows for revision 4; revision-7 LOD
    dimensions; and sort-in-world for revisions above 13.
- `rb3-latest/src/system/rndobj/Group.cpp` and
  `rb3-latest/src/system/rndobj/Group.h`
  - `RndGroup::RndGroup` initializes the object list with owner-control
    semantics, nulls `mEnv`, `mDrawOnly`, and `mLod`, zeros
    `mLodScreenSize`, clears `mSortInWorld`, and clears `unkf8`.
    Native `source_rndgroup_default_state` records these defaults.
  - `RndGroup::Copy` copies `Hmx::Object`, `RndAnimatable`, `RndDrawable`, and
    `RndTransformable`, then copies `mEnv`, `mDrawOnly`, `mLod`,
    `mLodScreenSize`, and `mSortInWorld`. Deep copies copy `mObjects`; Max
    imports call `Merge`; all paths call `Update`. Native
    `source_rndgroup_copy_plan` records that exact decision table.
  - `RndGroup::Replace` always delegates to `RndTransformable::Replace`, scans
    `mObjects`, and only for a found source object calls `AddObject(to, from)`,
    sets `gInReplace`, calls `RemoveObject(from)`, then clears `gInReplace`.
    Native `source_rndgroup_replace_plan` ports the membership branch without
    mutating live group lists.
  - `BEGIN_HANDLERS(RndGroup)` exposes actions `sort_draws`, `add_object`,
    `remove_object`, and `clear_objects`, query handlers `get_draws` and
    `has_object`, then superclasses `RndAnimatable`, `RndDrawable`,
    `RndTransformable`, and `Hmx::Object`. Native
    `source_rndgroup_handler_plan` records this source-visible table and check
    value `0x29B`.
  - `BEGIN_PROPSYNCS(RndGroup)` exposes `objects`, `environ`, `draw_only`,
    `lod`, `lod_screen_size`, and custom `sort_in_world`; `objects` and `lod`
    call `Update`, and `lod_screen_size` calls `UpdateLODState`. Native
    `source_rndgroup_prop_sync_plan` records those rows without changing
    character draw membership at runtime.
- `MiloEditor/MiloLib/Assets/Rnd/RndMesh.cs`
  - `RndMesh.Read` calls `base.Read`, embedded `trans.Read`, embedded
    `draw.Read`, material, geom owner, vertices, faces, group sizes, then bone
    transforms.
  - GH2-era meshes are below revision 33. When the source presence check sees a
    bone-transform block, the raw MILO rows are exactly four bone-name symbols
    followed by exactly four transform matrices. Native keeps those raw rows
    for audit.
  - For last-gen parent directories before revision 25, `RndMesh.Read` then
    reads one `GroupSection` per `groupSizes` row when `groupSizes[0] > 0`.
    Each `GroupSection` is `sectionCount`, `vertCount`, signed section indices,
    then unsigned vertex offsets. Native keeps these rows decoded/loggable
    instead of treating them as anonymous trailing bytes.
- `rb3/src/system/rndobj/Mesh.cpp`
  - `RndMesh::SetBone` is the runtime source for how a bone offset is authored:
    it inverts `t->WorldXfm()` and calls `Multiply(WorldXfm(), inverseBone,
    mBones[i].mOffset)`.
    Native `source_rndmesh_set_bone_plan` ports that assignment/offset rule as
    a deterministic helper: every call assigns the bone pointer, and only the
    source `b` flag recomputes the offset as mesh world times inverse bone
    world.
  - Native `source_rndmesh_scale_bones` ports the visible `ScaleBones` helper:
    only each offset transform's translation vector is multiplied by the scale;
    rotation rows are not altered.
  - GH2-era `RndMesh::PostLoad` reads the same four source bone slots and four
    offsets that the native decoder preserves as raw rows, then trims the
    active runtime bone list at the first null bone pointer and calls
    `RemoveInvalidBones`. Native therefore keeps `raw_bone_palette` /
    `raw_bind` for row proof, while `bone_palette` / `bind` follow the
    runtime-active source list used for skinning.
  - Native `source_rndmesh_bone_tail_plan` records that split explicitly:
    revisions above `0x1c` read a newer bone vector and clamp it to
    `MaxBones()`, while GH2 rev28 takes the older `gRev > 0xd` / `gRev > 0x16`
    path: read the first bone pointer, resize to four slots, read slots 1-3,
    read four offsets, trim at the first null slot, then call
    `RemoveInvalidBones`. If the first old pointer is null, source clears the
    bone list. Rev28 still runs the old `SetZeroWeightBones` gate, but that
    does not create serialized per-vertex bone indices.
  - ihatecompvir's RB3 `operator>>(BinStream&, RndMesh::Vert&)` reads explicit
    `boneIndices[0..3]` only for mesh revisions above `0x1c`, while
    MiloEditor reads explicit vertex bone indices in older pre-GH2 layouts and
    newer indexed layouts, not in GH2 revision 28. Native
    `source_rndmesh_skin_index_plan` records that GH2 rev28 is a legacy
    slot-weight layout with no serialized per-vertex bone-index rows. The RB3
    `SetZeroWeightBones` pass is therefore source evidence for zero-weight
    index cleanup when indices exist, not permission to synthesize fake GH2
    rev28 bone indices for hair, face, neck, or hand fixes.
  - `MiloEditor/MiloLib/Assets/Rnd/RndMesh.cs` records the source field gates
    around the same `RndMesh` rows: `mat2` exists only at mesh revision 27,
    `altGeomOwner` before 13, `transParent` before 15, the two unknown transform
    references before 14, mutable flags from 16 onward, volume after 17, BSP
    after 18, modern direct `mPatches` after `0x17`, the legacy
    `count -> ui + ushort-vector + uint-vector` patch loop for revisions
    `0x16..0x17`, legacy direct `mPatches` for revisions `0x11..0x15`,
    old four-slot bone names/offsets before revision 33, vector bone transforms
    at 33+, `keepMeshData` after 34, `hasAOCalculation` after `0x25`, `noQuant`
    when alt revision is above 1, and `unkBool3` when alt revision is above 3.
    The parent-dir gate for legacy `GroupSection` rows is
    `groupSizesCount > 0 && groupSizes[0] > 0 && parent.revision < 25`. Native
    `source_rndmesh_field_gate_plan` ports those gates so GH2 PS2 revision 28
    remains a source-backed material/geom-owner/mutable/volume/BSP/group-size
    and old-four-bone layout, not an inferred newer indexed layout.
  - `glTFMilo/Source/glTFMilo/Program.cs` is source evidence for exporter-side
    skin packing, not GH2 rev28 runtime decoding. It writes the four weights in
    influence order, writes bone slots in influence order for normal vertex
    layouts, reverses the bone slot order for compressed vertex layouts, and
    repairs invalid remapped bones by reusing the last valid slot. Native
    `source_gltf_milo_pack_skin_slots` ports that exact packing contract so the
    exporter rule is documented without being mistaken for stock runtime skin
    order.
  - `AddVertexToChunkMesh` is the per-vertex consumer for that exporter-side
    contract. It skips already-emitted source vertex indices, copies position,
    optional UV, optional normal, optional tangent, optional vertex color,
    always writes up to four influence weights, writes chunk-local bone slots
    only when a skin exists, applies the AO color override to `255,255,255,255`,
    and throws when the chunk grows past `ushort.MaxValue` vertices. Native
    `source_gltf_milo_add_vertex_to_chunk_mesh` mirrors those deterministic
    rows for contract coverage only; it does not change stock GH2 runtime mesh
    decode or manufacture per-vertex bone-index rows.
  - The glTFMilo material creation pass creates a `Mat` row for every logical
    material and, when a base-color texture exists, wires `diffuseTex`,
    `stencilMode=kStencilIgnore`, per-pixel/prelit/point/projected lighting,
    `fog=false`, `cull=!material.DoubleSided`, `_skin` / `_hair` shader
    variation selection, source blend/z/wrap/alpha decisions, and a matching
    diffuse `Tex` row. Native `source_gltf_milo_material_base_plan` mirrors
    those deterministic exporter rows for evidence only; live GH2 character
    material behavior still comes from decoded stock `RndMat` rows plus the
    separate project hair two-sided culling override.
  - `CreateBaseMesh` allocates `RndMesh.New(selectedGame.ModelRevision, 0, 0,
    0)`, sets object-fields revision 2, embeds a revision-9 `RndTrans` parented
    to the MILO filename, embeds revision-3 `RndDrawable` with radius `0`,
    sets triangle volume, keeps mesh data, and clears AO by default. It copies
    the glTF node local/world transforms, turns on next-gen vertex layout only
    for Rock Band 3 or Dance Central 1 on `xbox` (`compressionType=1`,
    `vertexSize=36`) or `ps3` (`compressionType=2`, `vertexSize=40`), binds
    `<material>.mat` when a material exists, warns when diffuse is missing, and
    enables AO calculation when a normal map exists. Native
    `source_gltf_milo_create_base_mesh_plan` mirrors that exporter-side mesh
    setup as a contract; it does not change stock GH2 runtime mesh decode.
  - `NodeProcessor.ProcessBoneNode` skips `neutral_bone`, skips RB3 skeleton
    bones only when exporting a `character`, otherwise emits a revision-9
    `Trans` row with object-fields revision 2, local/world matrices copied from
    the glTF node, and parent chosen from `GetParentBoneName(...) ??
    fallbackParent`. `NodeProcessor.ProcessGroupNode` skips only `Armature`,
    creates a `Group` row named `<node>.grp`, allocates group/trans/draw/anim
    rows from the selected game's revisions, copies local/world transforms,
    appends all non-null descendant names, calls `MiloExtras.AddToGroup`, and
    emits the group entry. Native `source_gltf_milo_process_bone_node_plan` and
    `source_gltf_milo_process_group_node_plan` mirror those exporter hierarchy
    decisions only; they do not alter GH2 stock transform solving or renderer
    group membership.
  - `NodeProcessor.ProcessLightNode` creates a `Light` row named
    `<node>.lit`, sets the selected game's light revision and object-fields
    revision 2, copies range and RGB color from `node.PunctualLight` with
    alpha forced to `1.0`, sets `colorOwner` to the same `.lit` name, maps
    point/spot/directional glTF light types to `RndLight` types with unknowns
    falling back to point, creates a selected-revision `RndTrans`, copies
    local/world transforms, calls `MiloExtras.AddToObject`, and emits the
    directory entry. Native `source_gltf_milo_process_light_node_plan` records
    that exporter object-shape contract only.
  - For each glTF logical animation whose channels all target only
    `translation`, `rotation`, or `scale`, glTFMilo creates a revision-7
    `TransAnim`, assigns selected-game anim/draw revisions, uses 30fps,
    zeroes the draw sphere, targets `<firstTargetNode>.mesh`, stores keys under
    `<animName>.tnm`, and emits a `TransAnim` directory entry with that same
    name. If channels target different nodes, source logs an error but still
    builds the `TransAnim` row from the first channel's node. Native
    `source_gltf_milo_export_trans_anim_plan` records that exporter contract
    only; it does not promote animation playback or pose publishing.
  - The same glTFMilo source validates skin influences before packing: missing
    or non-finite weights/joints are ignored, zero or negative weights are
    skipped, non-integral/out-of-range joints are rejected, excluded joints such
    as `neutral_bone` are ignored, influences are stably sorted by descending
    weight, more than four influences are trimmed, and the kept four weights are
    normalized. Native `source_gltf_milo_validate_skin_influences` ports that
    deterministic pre-pack contract only; it does not alter stock GH2 mesh
    decode or synthesize per-vertex bone indices.
  - `GetVertexSkinInfluences` gathers one vertex's `JOINTS_0/WEIGHTS_0` XYZW
    slots first, then `JOINTS_1/WEIGHTS_1` XYZW slots when the second accessor
    pair exists, before applying the validation, trim, and normalization rules
    above. Native `source_gltf_milo_get_vertex_skin_influences_plan` records
    that eight-slot collection order for exporter parity without promoting a
    guessed runtime skinning path.
  - `ValidateSkinAccessorSet` accepts only paired `JOINTS_n` / `WEIGHTS_n`
    accessors whose counts match each other and the `POSITION` accessor. Missing
    pairs, mismatched pairs, or wrong-position-count pairs are cleared before
    influence extraction. Native `source_gltf_milo_validate_skin_accessor_set`
    ports that gate.
  - `BuildSourceTriangles` emits sequential triangles when no index accessor is
    present, warns and drops trailing vertices/indices that cannot form a full
    triangle, and ignores indexed triangles whose indices fall outside the
    position accessor. Native `source_gltf_milo_build_source_triangles` ports
    that pre-chunk triangle contract.
  - `SplitTrianglesIntoMeshChunks` greedily partitions exporter triangles
    under `MaxMeshInfluencingBones = 40` and `MaxMeshVertices = ushort.MaxValue`.
    It precomputes each triangle's unique joint set in vertex order, rejects a
    triangle that cannot fit within the 40-bone palette, returns the full mesh
    as one chunk when it already fits, otherwise seeds each chunk with the
    unassigned triangle using the most joints, grows through shared-edge
    adjacency by fewest added joints then fewest added vertices, and finally
    does a global fill pass. Native `source_gltf_milo_split_mesh_chunks` ports
    this deterministic exporter chunking rule and records the emitted chunk
    triangle indices, joint palette order, and unique vertex count. It is
    exporter/format evidence for palette layout, not a change to stock GH2
    runtime skin decoding.
  - `PopulateMeshChunk` builds `jointIndexToLocalBoneIndex` from the chunk joint
    list, clears the target mesh vertices/faces, calls `AddVertexToChunkMesh` in
    each triangle's `idx0` / `idx1` / `idx2` order, and writes each face from
    the resulting `originalIndexToNewIndex` map. Native
    `source_gltf_milo_populate_mesh_chunk_plan` records that chunk-local vertex
    and face remapping contract without changing the live GH2 mesh decoder.
  - After `PopulateMeshChunk`, glTFMilo collects each chunk joint whose name
    passes `IsHairBone` (`bone_hair_`, case-insensitive), rebuilds
    `mesh.groupSizes` as repeated `255` entries plus a final remainder from the
    emitted face count, calls `MiloExtras.AddToMesh`, suffixes split chunk
    filenames as `.<chunkIndex:00>` before the extension, creates a `Mesh`
    directory entry, and assigns `mesh.geomOwner = entry.name`. The same pass
    records hair-collision meshes when extras `ObjectType` is `CharCollide` or
    the entry/node name ends in `.coll` / `.collide` or contains
    `hair_collide`. Native `source_gltf_milo_finalize_mesh_chunk_plan` ports
    these finalization rules around the still-external `MiloExtras.AddToMesh`
    hook; it is source evidence for mesh ownership, naming, group-size rows,
    and hair-related mesh classification only.
  - `PopulateMeshChunk` then builds `jointIndexToLocalBoneIndex` from each
    chunk's `jointIndices` in order and emits exactly one `RndMesh::BoneTransform`
    per chunk joint, because vertex bone indices point into this list by
    position. The source writes the bone name from the joint node or
    `joint_{index}`, warns and uses an identity inverse for non-invertible joint
    world matrices, then writes `inverse(jointNode.WorldMatrix) *
    node.WorldMatrix`. Native `source_gltf_milo_build_bone_transforms` ports
    that per-chunk transform-list rule.
  - Native `source_rndmesh_vert_load_plan` records the visible vertex stream
    gates: position, normal, color, and UV are read for all revisions in this
    source path; revisions `>= 0x25` read a separate weight vector; revisions
    `> 0x1c` read explicit bone indices; revisions `> 0x1d` read the later
    trailing vector. For skinned revisions below `0x25`, PostLoad copies
    `color.fr/fg/fb/fa` into `boneWeights` and clears the color. Therefore GH2
    rev28 character vertices use the color payload as the source-backed skin
    weight carrier, with no serialized per-vertex bone index rows.
  - Native `source_rndmesh_set_zero_weight_bones` ports the exact visible
    `SetZeroWeightBones` rule as a deterministic contract only: if the active
    source bone count is below two, it does nothing; otherwise zero weights in
    slots 1, 2, or 3 copy slot 0's bone index into that zero-weight slot.
  - The visible RB3 `RndMesh` ownership helpers are now mirrored as source
    plans: `MaxBones()` is 40 from ihatecompvir's `MAX_BONES`; `Sync(mask)` ORs `0x200` only while
    `mKeepMeshData` is true; `ClearCompressedVerts()` releases the compressed
    buffer and zeros `mNumCompressedVerts`; `SetNumVerts` and `SetNumFaces`
    resize their respective arrays and call `Sync(0x3f)`; and
    `SetKeepMeshData(false)` clears verts, faces, and patches only when the
    keep flag actually changes. These helpers are bookkeeping contracts, not
    permission to alter character material state or synthesize skinning data.
  - Native `source_rndmesh_copy_bones` and
    `source_rndmesh_copy_geometry_from_owner` port the visible ownership
    helpers: `CopyBones(nullptr)` clears the source bone list, otherwise it
    copies the source mesh's bones; `CopyGeometryFromOwner` copies geometry with
    volume and calls `Sync(0x3f)` only when the geometry owner is not `this`.
  - Native `source_rndmesh_set_geom_owner_plan`,
    `source_rndmesh_copy_geometry_plan`, and `source_rndmesh_replace_plan`
    preserve the adjacent source rules: `SetGeomOwner` asserts a non-null
    owner before assignment; `CopyGeometry` makes the destination its own
    geometry owner, copies verts/faces/patches from the source geometry owner,
    optionally copies volume, copies bones from the source mesh, and clears
    striper results; `Replace` rewires `mGeomOwner` only when it matched the
    replaced object, taking the replacement mesh's geometry owner or falling
    back to `this`.
  - Native `source_rndmesh_copy_plan` records the visible `RndMesh::Copy`
    decision table: normal copies copy keep-mesh-data/mutable state, copy full
    geometry with volume, copy AO state, and finish with `Sync(0xbf)`; shallow
    copies copy the geometry owner and bones instead; `kCopyFromMax` ORs
    mutable state, skips keep-mesh-data, and only takes the owner/bones branch
    when the source mesh itself has an external geometry owner.
  - Native `source_rndmesh_collide_showing_plan` ports the checked
    `RndMesh::CollideShowing` branch contract as deterministic evidence only:
    skinned meshes or `sRawCollide` use the incoming segment, static non-raw
    meshes invert `WorldXfm()` and multiply the segment endpoints into mesh
    space, BSP hits multiply the returned plane by `WorldXfm()`, triangle
    volume hits skin vertices only for `IsSkinned() && !sRawCollide`, update
    the running segment end/fraction and last-face index, and multiply the
    triangle plane by `WorldXfm()` only when raw collision is off. This does
    not promote a native point-collision or hair writeback path.
  - Native `source_rndmesh_update_sphere_plan` ports the adjacent checked
    `RndMesh::UpdateSphere` branch: meshes without bones call
    `MakeWorldSphere(s, true)`, invert `WorldXfm()`, multiply the sphere back
    into local space, and publish it through `RndDrawable::SetSphere`; meshes
    with bones zero the sphere before the same publish step.
  - Native `source_rndmesh_get_distance_to_plane` ports the checked
    `RndMesh::GetDistanceToPlane` selection rule: empty vertex buffers return
    zero, otherwise each vertex position is multiplied by `WorldXfm()`, dotted
    against the plane, and the vertex with the strictly smallest absolute dot
    replaces the current result.
  - Native `source_rndmesh_set_volume_plan` records the checked
    `RndMesh::SetVolume` boundary: non-self geometry owners forward the volume
    request, self-owned meshes assign `mVolume` and release `mBSPTree`, and
    only nonempty vertex/face geometry enters the `kVolumeBox` or `kVolumeBSP`
    branches. The source-visible box branch grows a box and allocates a
    `BSPNode`, but both box/BSP branch bodies are incomplete in the checked
    source and must not be treated as a native BSP implementation.
  - Native `source_rndmesh_face_load_plan` and
    `source_rndmesh_face_center` port the checked face-row support bodies:
    `operator>>(RndMesh::Face&)` reads three face indices and reads one legacy
    vector only when `RndMesh::gRev < 1`; `FaceCenter` sums the three indexed
    vertex positions and multiplies by `0.33333333f`.
  - Native `source_rndmesh_handler_plan`,
    `source_rndmesh_point_collide_plan`,
    `source_rndmesh_attach_mesh_plan`,
    `source_rndmesh_configure_mesh_plan`,
    `source_rndmesh_vertex_edit_plan`, `source_rndmesh_face_edit_plan`, and
    `source_rndmesh_unitize_normals_plan` port the checked editor/accessor
    surface: `OnPointCollide` gets the BSP tree, builds a vector from message
    floats 2-4, multiplies by `WorldXfm()`, and returns
    `tree && Intersect(v, tree)`; `OnAttachMesh` reads mesh arg 2, calls
    `AttachMesh(this, m)`, deletes that temporary mesh pointer, and returns
    zero; `OnConfigureMesh` warns for non-configurable meshes, otherwise reads
    `left`, `right`, and `height`, assigns four vertex positions, and calls
    `Sync(0x3f)`; vertex `norm` / `pos` / `uv` getters and setters assert the
    index, setters call `Sync(31)`, face getters/setters assert the face index,
    `OnSetFace` calls `Sync(32)`, and `OnUnitizeNormals` normalizes every
    vertex normal. These helpers document the source row surface only; they do
    not mutate live renderer geometry or add a native collision/attachment
    runtime path.
  - Native `source_rndmesh_vert_vector_resize_plan` and
    `source_rndmesh_vert_vector_reserve_plan` port the checked
    `RndMesh::VertVector` storage rules: `resize` stores the incoming `unka`
    flag, uses a fixed-capacity path when `mCapacity` is nonzero, releases
    vertices on resize-to-zero, otherwise allocates a new vertex buffer, copies
    `Min(newCount, oldCount)` rows, and deletes the old buffer. `reserve`
    asserts that capacity grows past both current capacity and current count,
    clears capacity, fails above `0xFFFF` before resizing, otherwise delegates
    to `resize`, then restores `mCapacity` and the previous vertex count.
  - Native `source_rndmesh_pre_load_vertices_plan` and
    `source_rndmesh_post_load_vertices_plan` record the visible vertex-load
    support path: alternate revisions above `4` create a front `FileLoader`,
    `PostLoadVertices` releases that loader into a temporary buffer stream,
    revisions above `0x22` read the compressed flag, and uncompressed vertices
    resize with `!(mMutable & 0x1F) && !mKeepMeshData` before reading each row.
    Compressed vertex data is still fenced because the visible source asserts
    platform support and immediately fails unsupported compression; stale
    compressed data is skipped by byte count, while zero metadata stores the
    compressed size and would read chunks after a debug failure when nonzero.
  - Native `source_rndmesh_create_multi_mesh_plan` and
    `source_rndmesh_cache_strips_plan` mirror the checked support helpers:
    `CreateMultiMesh` allocates an owner multimesh only when missing, sets that
    mesh to the owner, clears instances, and returns the owner multimesh;
    `CacheStrips` returns true only for cached Wii streams on self-owned meshes
    with nonempty faces/verts and without mutable bit `0x20`.
- `rb3-latest/src/system/rndobj/MeshDeform.cpp` and
  `rb3-latest/src/system/rndobj/MeshDeform.h`
  - `RndMeshDeform::VertArray::VertArray` starts with size `0`, data `0`, and
    the provided parent pointer. `Clear` delegates to `SetSize(0)`.
    `SetSize` only acts when the size changes: it stores the new size, frees
    existing data, and allocates the requested byte count. Native
    `source_rndmesh_deform_vert_array_default_state`,
    `source_rndmesh_deform_vert_array_set_size_plan`, and
    `source_rndmesh_deform_clear_plan` record these decisions.
  - `RndMeshDeform::RndMeshDeform` nulls `mMesh`, parents `mBones` and
    `mVerts` to itself, clears `mSkipInverse`, and clears `mDeformed`. Native
    `source_rndmesh_deform_default_state` records those defaults.
  - `RndMeshDeform::SetMesh` assigns `mMesh` and clears `mVerts`. Native
    `source_rndmesh_deform_set_mesh_plan` records that bounded behavior.
  - `BEGIN_HANDLERS(RndMeshDeform)` only exposes `Hmx::Object` and
    `HANDLE_CHECK(0x2A1)`. Native `source_rndmesh_deform_handler_plan` records
    the visible handler table.
  - The header declares `CopyWeights`, `Reskin`, `FindDeform`, `Load`, and
    `Copy`, but the checked source file does not expose statement bodies for
    those routines. Native `source_rndmesh_deform_body_availability` keeps those
    bodies fenced; do not infer a skinning/deformation runtime from the class
    name alone.
- `rb3-latest/src/system/rndobj/MultiMesh.cpp` and
  `rb3-latest/src/system/rndobj/MultiMesh.h`
  - `RndMultiMesh` derives from `RndDrawable`. Its constructor nulls `mMesh`
    through the `ObjPtr` owner and clears `unk9p4`; `Instance::Instance`
    resets its transform. Native `source_rndmultimesh_default_state` and
    `source_rndmultimesh_instance_default_state` record those defaults.
  - `RndMultiMesh::Load` accepts revisions `0..4`, skips `Hmx::Object::Load`
    only for revision `0`, always loads `RndDrawable` and `mMesh`, reads and
    returns after a legacy transform-list dump below revision `2`, otherwise
    reads `mInstances`, and reads one legacy byte below revision `4`. Native
    `source_rndmultimesh_load_plan` records those gates.
  - `RndMultiMesh::Copy` copies `Hmx::Object` and `RndDrawable`, copies `mMesh`
    except for `kCopyFromMax`, always copies `mInstances`, then calls
    `UpdateMesh`. Native `source_rndmultimesh_copy_plan` records that split.
  - `SetMesh` only assigns `mMesh` and calls `UpdateMesh`. `BEGIN_HANDLERS`
    exposes the transform-list mutation/query handlers, `mesh`, action
    `set_mesh`, then `RndDrawable` and `Hmx::Object`, with an explicit warning
    for unhandled messages. `BEGIN_PROPSYNCS` only syncs `RndDrawable`. Native
    `source_rndmultimesh_set_mesh_plan`,
    `source_rndmultimesh_handler_plan`, and
    `source_rndmultimesh_prop_sync_plan` record those rows.
  - `OnSetPos` advances the instance iterator by the requested index, reads
    floats in source order `z`, `y`, then `x`, and writes translation in
    `x`, `y`, `z` order. Native `source_rndmultimesh_set_pos_plan` records the
    visible row order without adding bounds checks not present in the source.
- `rb3-latest/src/system/rndobj/MultiMeshProxy.cpp` and
  `rb3-latest/src/system/rndobj/MultiMeshProxy.h`
  - `RndMultiMeshProxy` derives from `RndTransformable` and `RndDrawable`.
    Its constructor nulls `mMultiMesh` and zeros `mIndex`.
  - `SetMultiMesh` clears `mMultiMesh` first, copies the selected instance
    transform into local transform only when a mesh is provided, then assigns
    `mMultiMesh` and `mIndex`. Native `source_rndmultimesh_proxy_set_plan`
    records those decisions.
  - `DrawShowing` only acts when both `mMultiMesh` and its `mMesh` are present:
    it sets the mesh world transform from the selected instance transform, then
    draws the mesh. `UpdatedWorldXfm` writes the selected instance transform
    from `WorldXfm()` when `mMultiMesh` is present; the checked source also
    exposes the `VERSION_SZBE69_B8` spelling variant. Native
    `source_rndmultimesh_proxy_draw_plan` and
    `source_rndmultimesh_proxy_updated_world_plan` record that behavior.
  - Proxy `Load`, `Save`, and `Copy` all fail by design; its handler table only
    has `HANDLE_CHECK(0x3F)`, and its prop-sync table is empty. Native
    `source_rndmultimesh_proxy_failure_plan`,
    `source_rndmultimesh_proxy_handler_plan`, and
    `source_rndmultimesh_proxy_prop_sync_plan` record those fences. This is not
    a live character rendering import.
- `rb3/src/system/rndobj/Mat.cpp`
  - `RndMat` runtime defaults are source state: blend `kSrc`, texture wrap
    `kRepeat`, z mode `kNormal`, cull true, white RGBA, environment enabled,
    alpha-cut/write/per-pixel/point/fog/fadeout/color-adjust disabled,
    emissive multiplier `1.0`, texture transform reset, color-mod array reset
    to three rows, and dirty state `3`.
  - Native render state must come from decoded `RndMat`/`RndDrawable` rows, not
    from mesh or material names such as `hair`, except for the project-level
    hair two-sided cull rule below.
  - Shared native `source_rndmat_load_plan` records the material load gates used
    by character rendering: blend and color are first, revisions above 21 read
    `use_environ`, `prelit`, `z_mode`, `alpha_cut`, optional
    `alpha_threshold` for revisions above `0x25`, `alpha_write`, `tex_gen`,
    `tex_wrap`, `tex_xfm`, `diffuse_tex`, `next_pass`, `intensify`, `cull`,
    and `emissive_multiplier` in source order. GH2 PS2 v27 therefore has no
    serialized alpha-threshold row. Native now reads `diffuse_tex`,
    `next_pass`, `intensify`, `cull`, and `emissive_multiplier` directly from
    that source cursor instead of scanning forward for a filename-shaped
    `.tex` row.
  - Shared native `source_rndmat_default_state`,
    `source_rndmat_accessors`, and `source_rndmat_setter_plan` record the
    checked constructor, direct member accessors, and setter dirty-bit rules:
    alpha/color setters dirty color state with bit `1`, transform/render-state
    setters dirty bit `2`, while `SetAlphaThreshold` and `SetPointLights` write
    their member without changing `mDirty` in the checked header.
- `rb3/src/system/rndobj/Mat.h`
  - `RndMat` exposes source `GetBlend`, `GetZMode`, and `GetTexWrap` accessors.
  - The source setters directly write members; they do not encode hair/string
    special cases, sorting rules, or texture-alpha overrides.
- `rb3/src/system/rndobj/Tex.cpp`
  - `RndTex::Load` is `PreLoad` followed by `PostLoad`.
  - `PreLoad` reads packed revisions, `Hmx::Object` fields for revisions above
    8, width/height, `SetPowerOf2`, bits-per-pixel, and `FilePath`.
  - `PostLoad` reads the legacy cubemap mask for revisions below 5, one-byte
    legacy bools for revisions 1 and 2, `mipMapK` as either float or fixed
    integer/16, type flags, the post flag for revisions above 7, and
    `optimize_for_ps3` for revisions above 10.
  - When the stream is cached, `PostLoad` delegates the remaining stream to
    `RndBitmap::Load`; native records that boundary instead of treating the
    remaining bytes as anonymous padding.
  - Native `source_rndtex_load_plan` records the same `PreLoad` / `PostLoad`
    revision gates and cached-stream branches, and `decode_rnd_tex` consumes
    its rows instead of repeating raw revision comparisons inline.
- `rb3/src/system/rndobj/Bitmap.cpp`
  - `RndBitmap::LoadHeader` reads bitmap revision, bpp, order, mip count,
    width, height, row bytes, and the fixed padding row before pixel chunks.
    Native logs that header for cached character texture rows but does not
    decode pixel chunks in the character model decoder.
  - `RndBitmap::PaletteBytes` returns `(1 << bpp) * 4` only for bpp <= 8 when
    neither the `0x38` nor `0x80` order masks are set.
  - `RndBitmap::Load` reads palette bytes first, then reads exactly
    `mRowBytes * mHeight` base pixel bytes through `ReadChunks`; its mip loop
    halves width/height before reading each mip row. The source `LoadSafely`
    check documents the row-byte relation as `mBpp * mWidth / 8`.
- `rb3/src/system/utl/ChunkStream.cpp`
  - `ReadChunks` repeatedly reads `Min(total_len - curr_size, max_chunk_size)`
    until exactly `total_len` bytes have been consumed. This is the source for
    the passive cached bitmap payload-size validation.
- `rb3/src/system/rndobj/Tex.h`
  - `RndTex::Type` flag values are source state: `Regular=1`, `Rendered=2`,
    `Movie=4`, `BackBuffer=8`, `FrontBuffer=0x18`, and the later render target
    flags. Do not remap `Regular` to zero.
- `rb3/src/system/utl/FilePath.h` and `BinStream.cpp`
  - `FilePath` rows are read through `ReadString(buf, 0x100)` and then
    `SetRoot`, so the native row inventory reads the same bounded string shape.
- `rb3/src/system/rndobj/Trans.h`
  - `RndTransformable::Constraint` is the runtime enum authority:
    `kNone`, `kLocalRotate`, `kParentWorld`, `kLookAtTarget`,
    `kShadowTarget`, billboard variants, and `kTargetWorld`.
  - Source local setters (`ResetLocalXfm`, `SetLocalXfm`, local rotation,
    local position, and `DirtyLocalXfm`) call `SetDirty` and directly write or
    return the local transform row. They do not contain character-name,
    material-name, or mesh-name correction logic.
- `rb3/src/system/rndobj/Trans.cpp`
  - `RndTransformable` constructor defaults are source state: parent and target
    null, constraint `kNone`, preserve scale false, local/world transforms reset,
    a `DirtyCache` allocated, and the cache initialized to itself.
  - `SetTransParent` asserts the new parent is not `this`; when the parent is
    unchanged it only marks dirty. Otherwise, the preserve-world branch computes
    the old-parent to new-parent delta, multiplies it into `mLocalXfm`, and
    calls `TransformTransAnims`; then it removes the old parent links, assigns
    the new parent, updates the cache parent flags, adds new parent links, and
    marks dirty.
  - `SetWorldXfm` writes the full world transform, clears the dirty bit, calls
    `UpdatedWorldXfm`, and dirties children. `SetWorldPos` writes only the world
    translation, calls `UpdatedWorldXfm`, and dirties children, but the checked
    source body does not clear the cache dirty bit.
  - `RndTransformable::WorldXfm_Force` is the runtime transform-composition
    authority. With no parent, world equals local. `kParentWorld` copies the
    parent world row. `kLocalRotate` transforms only the local translation by
    parent world and keeps the local rotation rows. Otherwise local is
    multiplied by parent world, then dynamic constraints are applied.
  - `ApplyDynamicConstraint` replaces world with the target world for
    `kTargetWorld`; billboard/look-at/shadow constraints require the source
    camera/target path and must not be approximated from mesh names.
  - Native GHOGX implements the non-dynamic source cases plus target-world when
    a target row is present. Other dynamic constraints log
    `[source-xfm-unsupported]` with `runtimeWriteback=0` and keep the decoded
    base transform until the matching source runtime path is ported.
  - Shared native `source_rndtransformable_default_state`,
    `source_rndtransformable_set_dirty_plan`,
    `source_rndtransformable_set_parent_plan`,
    `source_rndtransformable_world_write_plan`, and
    `source_rndtransformable_local_write_plan` record those concrete source
    behaviors as deterministic contracts for character/bone transform work.

## Rnd Texture Row Authority

Native now decodes `Tex` object rows as passive source inventory using
`RndTex::Load`/`PreLoad`/`PostLoad`, `RndTex::Type`, `RndBitmap::LoadHeader`,
`RndBitmap::PaletteBytes`, `ReadChunks`, `FilePath`, and `BinStream` evidence
from ihatecompvir's `rb3-latest` source snapshot. The decoded row records packed
low/high revisions, object metadata for `gRev > 8`, width/height/bpp, source
`SetPowerOf2` state, filepath, legacy cubemap mask, mip-map coefficient, source
type flags, post bool, PS3 optimize bool, cached bitmap payload/header evidence,
and whether the payload byte count exactly matches source `PaletteBytes`,
base `rowBytes * height`, and any mip rows from the source mip loop.

This does not replace the native PS2 texture-image path. Texture payloads are
still handled by `asset/milo_image.*` and selected from material diffuse texture
names. The `Tex` rows are format evidence and audit coverage so character
texture metadata is no longer an anonymous stock row class.

The focused inventory at
`engine/out/source_truth_tex_inventory_20260710/stock_character_tex_inventory.log`
records 160 stock `Tex` rows with source `RndBitmap::LoadHeader` fields for
the cached bitmap payloads. The inventory includes two stock mip textures
(`metal_keyboard_mip.tex` and `metal_singer_belt_mip.tex`), and all 160 stock
rows report `payloadSizeMatch=1`.

`ghogx_character_tex_source_test` builds synthetic source-shaped rows for a
current revision cached texture, a legacy cubemap-suffix texture, and a
revision-0 bitmap header. It proves the native decoder follows
`RndTex::PreLoad`/`PostLoad` and `RndBitmap::LoadHeader`/payload sizing without
touching renderer upload, material sorting, or runtime texture selection.

## Generic Object Row Authority

Native now decodes generic `Object` rows as passive source inventory using the
same ihatecompvir `ObjectFields.Read` row already used by embedded superclass
metadata: combined low/high object revisions, subtype `Symbol`, root DTB parent
presence/id/child count, and revision-gated note `Symbol`. It does not attach
runtime behavior to these rows.

The focused inventory at
`engine/out/source_truth_object_inventory_20260710/stock_character_object_inventory.log`
records 19 stock `Object` rows. All 19 rows are `expression_task`, all are
version `0` / alt version `0`, all have no subtype, no root property tree, no
note, and all report `unreadBytes=0`.

## Skinning Authority

- `glTFMilo/Source/glTFMilo/Program.cs`
  - Mesh bone transforms are generated per chunk joint, in vertex palette order.
  - The transform written for a palette bone is:
    `inverse(jointNode.WorldMatrix) * node.WorldMatrix`.
- `rb3/src/system/rndobj/Mesh.cpp`
  - Runtime-authored offsets are `meshWorld * inverse(boneWorld)`. Native GHOGX
    therefore consumes GH2 stock offsets as `vertex * storedOffset *
    currentBoneWorld`.
  - Do not use the glTFMilo writer formula by itself to reverse runtime skinning
    order; a local visual check on stock Rock1/Rock2/Rockabill2 proved that
    order explodes the character mesh.

## Hair Authorities

- `glTFMilo/Source/glTFMilo/Program.cs`
  - Current exporter source identifies hair bones by names beginning with
    `bone_hair_`.
  - The same source defines mesh skin export packing and the compressed-layout
    bone-slot reversal now recorded by `source_gltf_milo_pack_skin_slots`.
  - During node traversal, the exporter only tries to discover `CharHairExtras`
    on `bone_hair_` nodes with non-null extras, only deserializes extras whose
    JSON contains the exact `milo_hair_` marker, keeps the first valid settings
    object, and catches bad extras because hair extras are optional. Native
    `source_gltf_milo_detect_hair_settings_plan` records that discovery path as
    source-backed gating for physics settings; it does not invent default
    physics values absent from the checked source.
- `glTFMilo/Source/glTFMilo/Core/NodeProcessor.cs`
  - `ProcessCharHair` starts by building a case-insensitive weighted hair-bone
    set and returns without a `CharHair` object when the set is empty. When the
    set is non-empty, it creates a revision 11 `CharHair` with object revision
    2, `simulate = true`, copies stiffness/torsion/inertia/gravity/weight/
    friction from `CharHairExtras`, and uses `CharHairExtras.DefaultWind` when
    the requested wind string is empty. It only adds a `CharHair` directory
    entry after at least one strand exists, and names that entry `hair.hair`.
    Native `source_gltf_milo_process_char_hair_plan` records those top-level
    exporter gates and defaults without turning them into a runtime placement
    guess.
  - `ProcessCharHair` builds `CharHair` strands from weighted hair-bone chains.
    The newer source splits strands at branches, matching how the decompiled
    runtime expects hair to be structured. Native
    `source_gltf_milo_collect_hair_chains_split_at_branches` ports that
    root-climb and branch-split rule as deterministic segment evidence:
    weighted hair bones climb to their top `bone_hair_` ancestor, non-hair bone
    children under hair bones are warnings, branch points end the current
    segment, and weighted descendants keep child segments alive.
  - The same source keeps the old `CollectHairChains` fallback for
    `--disable-splitting`. Native
    `source_gltf_milo_collect_hair_chains_without_splitting` ports that
    fallback exactly: it walks full root-to-leaf hair-bone paths, emits only
    paths containing a weighted hair bone, warns when a hair bone branches
    while splitting is disabled, and therefore duplicates shared root/trunk
    bones across multiple strands. This documents the fallback; it is not a
    reason to prefer the fallback over the source-backed branch splitter.
  - The same `ProcessCharHair` row emits each point from visible exporter
    rules: non-tip points use the next bone's world position, tip points extend
    along the current bone's world Y axis with a UnitY fallback, point length
    comes from next-bone distance, previous-point length, parent distance, then
    `5.0f`, and non-tip radius/outer-radius taper by point index. Native
    `source_gltf_milo_export_hair_point` ports those deterministic point rows,
    including `sideLength = -1.0f` and `unk5c` as the generated point
    transformed by the strand root parent's inverse world matrix. This is
    exporter/format evidence for segment construction, not proof that GH2
    runtime simulation or collision hookup is complete.
  - `ProcessEmptyHairCollides` can emit empty `CharCollide` objects for hair
    meshes, but its own source comment says this is inferred from decomp and
    "could be wrong". Treat those rows as exporter/format hints, not proof of
    GH2 runtime collision hookup. Native
    `source_gltf_milo_process_empty_hair_collides`
    ports the exporter row shape only: unique hair mesh names become `.coll`
    entries, existing `CharCollide` names are skipped case-insensitively, and
    the generated row uses `CharCollide` revision 7, object revision 2, sphere
    shape, zero flags, mesh reference, no mesh-y bias, identity unknown
    transform, and eight empty structs.
- `rb3-latest/src/system/char/CharHair.cpp` and
  `rb3-latest/src/system/char/CharHair.h`
  - `operator>>(BinStream&, CharHair::Point&)` is the runtime read authority for
    point fields. For revisions 6, 7, and 8 the extra float is added to both
    `radius` and `outerRadius`. For revisions below 8, `sideLength` is forced to
    `-1.0f`; revisions above 5 consume two ints.
  - `CharHair::Load` defaults `minSlack`/`maxSlack` only when `gRev < 8`.
    Revision 8 and newer rows read both floats before the strand list. It
    asserts source revisions through 11, always reads `simulate` after the
    strand list, and reads `wind` only when `gRev > 10`. Native exposes
    `decode_hair` for deterministic row tests and follows those same gates.
    `source_char_hair_load_plan`, `source_char_hair_strand_load_plan`, and
    `source_char_hair_point_load_plan` record the same source read order and
    revision gates as deterministic format evidence for hair segment/controller
    rows.
  - Grim `dev` `char_hair/io.rs` independently records GH2/GH2 360
    `CharHair` version 2 parser order: version, object metadata, stiffness,
    torsion, inertia, gravity, weight, friction, strand count, strand
    root/angle, point count, point vector/bone/length/collide
    type/collision/distance/align distance, base/root matrices, and simulate.
    Native `source_grim_char_hair_load_plan` records that separate Grim row
    order and maps `unknown_floats`, `distance`, and `align_dist` to the RB3
    revision-2 `pos`, `radius`, and `outerRadius` fields. This is parser
    evidence only; it does not promote collision hookup, physics, or
    simulation writeback into solved runtime behavior.
    The deterministic `decode_hair` coverage includes a synthetic revision-2
    row that preserves `collide_type`, `collision`, `distance`, and
    `align_dist` while leaving `side_length` and strand hookup flags at their
    source-gated defaults.
  - `CharHair::Save` uses source save id `0x41B`. Native
    `source_char_hair_save_plan` records that row id only; it does not imply
    any additional runtime writer.
  - Revisions below 3 consume a legacy `int` and string, and revision 3 consumes
    a legacy `int`, but the reader then calls `pt.collides.clear()`. Native may
    log these legacy inline fields for stock GH2 evidence, but they are not a
    resolved runtime `ObjPtrList<CharCollide>` and must not be promoted into one
    without the missing source hookup body.
  - `CharHair::Poll` re-runs `Hookup()` while the owning `Character` is syncing,
    resets after teleports, skips simulation for higher LODs, then calls
    `DoReset`, `SimulateLoops`, or `SimulateZeroTime` depending on runtime state.
    Native `source_char_hair_poll_decision` ports this branch order as a
    deterministic decision helper. It records when source `Poll` would call
    `Hookup`, `DoReset`, `SimulateLoops`, or `SimulateZeroTime`, while keeping
    the unresolved overloaded hookup body and zero-time simulation body fenced.
  - `CharHair::Enter` sets `mReset = 1`, delegates to `RndPollable::Enter`,
    then runs `Hookup()`. Native `source_char_hair_enter_plan` records that
    exact call order and reuses the bounded hookup plan; it does not resolve
    the missing overloaded hookup body.
  - `CharHair::DoReset` seeds each point from `unk5c` transformed by the root
    parent world row, temporarily forces `mSimulate=true`, `mInertia=0`, and
    `mFriction=0`, then calls `SimulateLoops(reset, GetFPS())` before restoring
    the previous simulate/inertia/friction values. Native reset follows that
    forced-simulate lane even when the decoded `simulate` flag is false.
    Native `source_char_hair_do_reset_plan` records the checked reset flow,
    including the point-row reset steps, temporary simulate/inertia/friction
    override, `GetFPS` simulate-loop call, restore behavior, and final
    `mReset=0` write.
  - `FreezePoseRaw` stores current point positions back into `unk5c` in the
    root-parent local basis. `FreezePose` performs a source `Hookup()`, simulates
    200 loops at 60 Hz, restores the previous simulate flag, then freezes those
    rows. Native `source_char_hair_freeze_pose_plan` ports the call order and
    restore behavior, and `source_char_hair_freeze_pose_raw` ports the raw
    local-row write. Full `FreezePose` remains bounded by the unresolved source
    `Hookup(ObjPtrList<CharCollide>&)` path.
  - `SetName` detects whether the owning directory is a `Character` or
    `WorldDir` and enables post-process FPS emulation accordingly. `GetFPS`
    returns the post-process emulated rate when available, otherwise 60 Hz. The
    inline `SetManagedHookup` setter only assigns `mManagedHookup`.
    Native ports the constructor constants, SetName ownership branch, the
    `SetManagedHookup` state change, and the `GetFPS` branch into
    deterministic helpers so those source defaults remain explicit without
    guessing a post-process runtime.
  - `SimulateLoops` is gated by `mSimulate` and a non-empty strand list, runs
    collide-list maintenance, then calls `SimulateInternal` for each requested
    loop. Native `source_char_hair_simulate_loops_plan` ports that gate and
    call count as a deterministic plan. A decoded `CharHair` row alone is
    therefore not enough evidence for a native writeback path.
  - Native `source_char_hair_simulate_internal_scalars` ports the concrete
    beginning of `CharHair::SimulateInternal`: `sixtyover = 60 / fps`,
    `f19 = (1 / fps) * sixtyover`, stiffness decay as
    `pow(1 - mStiffness, sixtyover * sixtyover)`, wind contribution only when
    both a wind object and strand root exist, and gravity as
    `mGravity * f19 * -3.858268`.
  - Native `source_char_hair_simulate_internal_cloth_pair` ports the cloth
    side-length pair constraint from `SimulateInternal` over explicit point
    rows. It preserves the source min-slack branch and the checked max-slack
    condition exactly as written (`maxslacklen > maxslacklensq`), even though
    that condition means larger GH2-length rows usually do not enter the
    max-slack adjustment. This helper is deterministic evidence for cloth math
    only; it does not populate collide lists or call `SetWorldXfm`.
  - Native `source_char_hair_simulate_internal_length_step` ports the next
    concrete point step: cache `v140`, add the point force and external force
    to `pos`, derive `m128.y` from point-to-root, compute the source reciprocal
    length and `rsalen`, optionally add the previous-point force delta when
    `j > 0`, correct point position along `m128.y`, and compute the source
    target `v158 = root + rootY * length`.
  - Native `source_char_hair_simulate_internal_collision_step` ports the
    explicit collision branch inside `SimulateInternal` over caller-supplied
    `CharCollide` results: the `collides.size() != 0` gate, plane push-out,
    outside sphere/cigar, inside sphere/cigar, tapered-radius `lastZ`
    overrides/interpolation, and the basis rebuild that precedes source
    `SetWorldXfm`. This helper does not discover or populate a point's
    collide-list; the unresolved `Hookup(ObjPtrList<CharCollide>&)` body still
    gates live hair transform publishing.
  - Native `source_char_hair_simulate_internal_force_step` ports the later
    force/friction/inertia update in the collision-resolved branch:
    `force = v158 - pos`, `lastFriction - force`, store `lastFriction`, scale
    force by `1 - powed`, apply `-mFriction`, then add `(pos - v140) *
    mInertia`. This is still deterministic point math only; it does not imply
    the unresolved collision-list hookup or live `SetWorldXfm` writeback has
    been solved.
  - `CharHair::Strand::SetRoot` builds the strand from the root transform's
    first-child chain, caches the root base matrix, assigns each point's bone,
    copies child `LocalXfm().v.y` into point length, and seeds point positions
    from source world rows. Native ports this as
    `source_char_hair_strand_set_root` over an explicit first-child transform
    chain, preserving the old terminal length when present and using the source
    single-root fallback length of `5.0f`.
  - `CharHair::Strand::SetAngle` stores the angle, builds a rotation around X
    from `angle * DEG2RAD`, and multiplies that by `mBaseMat` into `mRootMat`.
    Native exposes this exact formula through
    `source_char_hair_strand_set_angle` and the bind audit uses that shared
    helper for `setAngleRootErr`.
  - `CharHair::Strand::Strand` initializes `mBaseMat` and `mRootMat` to
    identity. Native `CharHairStrand` defaults now match that constructor so
    helper-created strands do not start from zero rotation matrices.
  - `CharHair::SetCloth` assigns `sideLength` from the matching point in the
    next strand when cloth mode is enabled and otherwise forces `sideLength` to
    `-1.0f`. Native ports this exactly as `source_char_hair_set_cloth`; it is
    a deterministic side-length helper only, not a guessed hair placement or
    writeback path.
  - `BEGIN_HANDLERS(CharHair)` exposes only `reset`, `hookup`, `set_cloth`,
    and `freeze_pose`, then delegates to `RndPollable` and `Hmx::Object`.
    Native `source_char_hair_handler_plan` records those source-visible message
    rows as a deterministic contract only.
  - `BEGIN_CUSTOM_PROPSYNC(CharHair::Point)`,
    `BEGIN_CUSTOM_PROPSYNC(CharHair::Strand)`, and
    `BEGIN_PROPSYNCS(CharHair)` expose point rows (`bone`, `length`,
    `collides`, `radius`, `outer_radius`, `side_length`), strand rows
    (`root` through `SetRoot`, `angle` through `SetAngle`, `points`,
    `hookup_flags`, `show_spheres`, `show_collide`, `show_pose`), and hair rows
    (`stiffness`, `torsion`, `inertia`, `gravity`, `weight`, `friction`,
    `min_slack`, `max_slack`, `strands`, `simulate`, `wind`). Native
    `source_char_hair_prop_sync_plan` records those property rows without
    promoting editor/property mutation into the runtime renderer.
  - `CharHair::SimulateInternal` only calls `SetWorldXfm` for a point inside the
    `thisPoint.collides.size() != 0` branch. Native GHOGX must not invent a
    partial hair physics bridge from decoded point rows alone.
  - The latest source includes `CharHair.h`, `CharCollide.h`, default
    `CharHair::Hookup()` gathering all `CharCollide` objects from the object
    directory, and the `CharCollide` shape/radius header plus load path. Native
    `source_char_hair_hookup_plan` ports the managed-hookup early return and
    directory collide collection order, then records that the overloaded hookup
    would be called. However, the overloaded
    `Hookup(ObjPtrList<CharCollide>&)` body is still declared but not
    implemented in the checked source. Native GHOGX therefore runs the checked
    source poll/reset/sim state path for persistent point position, force,
    friction, and `lastZ`, but keeps point rows unwritten until that hookup
    filter and point collide-list population are ported from source, not
    guessed.
  - `rb3-retail-old/doc/rb2_dump/rockband2/system/src/char/CharHair.cpp`
    confirms the missing overload is a real runtime body at
    `0x80360284 -> 0x80360BE0`. Its local inventory names a `vector collides`,
    `ObjDirItr`, nested loop counters, `CharCollide* c`, `delta`, `rootDist`,
    `d`, `length`, and `maxRadius`, with references to `Hmx::Object` and
    `CharCollide` RTTI. Native `source_char_hair_hookup_dump_evidence` records
    those facts and also records `has_statement_body=false`: the dump is enough
    to prove Hookup is doing geometric collide filtering, but not enough to copy
    the filtering statements or live point collide-list population.
  - The same RB2 dump confirms `CharHair::SimulateZeroTime` is a real runtime
    body at `0x8035FC8C -> 0x80360144`. Its local inventory names an outer loop
    counter, `Transform t`, an `ObjVector` point list, an inner loop counter,
    and `Matrix3 m`. Native
    `source_char_hair_simulate_zero_time_dump_evidence` records those facts and
    also records `has_statement_body=false`: the dump is enough to prove
    zero-delta hair has its own transform path, but not enough to copy that path
    or use it for live native point writeback.
  - The RB2 dump also maps `CharHair::PollDeps` at
    `0x80360144 -> 0x80360284` and `CharHair::Copy` at
    `0x803616E8 -> 0x8036181C`. Native
    `source_char_hair_rb2_mapped_body_evidence` records those ranges and the
    visible locals, while explicitly marking both as lacking statement bodies.
    Do not infer dependency rows or copy-member behavior from those ranges
    without a reviewable source body or direct original-game trace.
- `rb3-latest/src/system/char/CharCollide.cpp` and
  `rb3-latest/src/system/char/CharCollide.h`
  - `CharCollide::Load` reads `Hmx::Object`, `RndTransformable`, shape,
    radius/length/flags rows, optional current radius, optional second
    radius/length rows, an internal transform, mesh pointer, eight mesh sphere
    rows, SHA1 digest, and mesh-y-bias.
    Native `source_char_collide_load_plan` records this exact read order and
    revision-gated branch behavior for deterministic format coverage; it does
    not imply the still-missing hair point collide-list hookup is solved.
  - `CharCollide::Load` uses `ASSERT_REVS(7, 0)`. Native GHOGX rejects
    `CharCollide` rows outside that source revision range before consuming the
    superclass payload.
  - Native GHOGX decodes and logs `CharCollide` rows using this source order so
    hair hookup/collision work can be audited from stock data. It does not yet
    apply collision or write hair world rows from these decoded objects.
  - Native GHOGX retains the internal transform, all eight mesh sphere rows, and
    the SHA1 digest from source `CharCollide` rows. These fields remain decoded
    evidence for later source-backed collision hookup, not an invented native
    collision response.
  - Native GHOGX ports `CharCollide::CopyOriginalToCur` and
    `CharCollide::SyncShape` / `CharCollide::NumSpheres` exactly for decoded
    rows. The decoder now calls the named copy helper at the same revision
    gates as source load.
  - Native `source_char_collide_clear_mesh` ports the inline
    `CharCollide::ClearMesh()` body: it only clears the mesh pointer/string and
    does not change collision radii, shape, or hair point hookup state.
  - `CharCollide::CharCollide` initializes `mShape` to `kSphere`, zeroes
    flags/radius/length state, copies original radius/length into current
    radius/length, clears all eight mesh collision rows, resets the internal
    transform, and leaves mesh-y-bias false. Native GHOGX records that source
    constructor contract as `source_char_collide_default_state` and checks the
    decoded row defaults against it.
  - `CharCollide::Copy` copies `Hmx::Object`, `RndTransformable`, shape, flags,
    original/current radius and length arrays, the internal transform,
    mesh-y-bias, and mesh pointer. The checked source copy-member list does not
    name `mDigest` or `unk_structs`; native GHOGX records that as copy-plan
    evidence only, not as a replacement for a complete runtime copy system.
  - Native `source_char_collide_handler_plan` and
    `source_char_collide_prop_sync_plan` record the source handlers and
    property rows: handlers forward only to `RndTransformable` and
    `Hmx::Object`, while mutable shape/radius/length/mesh rows all call
    `SyncShape`. This keeps hair collision row edits source-backed instead of
    name-driven.
  - Native `source_char_collide_highlight_plan` records the diagnostic draw
    dispatch from source `Highlight`: planes draw one plane, sphere variants
    draw original/current spheres, cigar variants draw original/current cigars,
    and a present mesh draws `NumSpheres() * 2` tiny vertex spheres. This is
    diagnostic evidence only; it does not change character rendering.
  - `CharCollide::Save` uses source save id `0x58`. Native
    `source_char_collide_save_plan` records that row id alongside the decoded
    collision shape/load/copy/property plans used by the hair boundary.
  - `CharCollide::Deform` is an empty source body. Native
    `source_char_collide_deform_plan` records that no-op explicitly so hair or
    accessory collision fixes do not invent unsupported mesh deformation.
  - `CharHair::SimulateInternal` calls `CharCollide::GetRadius` and
    `CharCollide::Axis`, but `GetRadius` depends on cached collision fields
    (`unk18c`, `unk190`, `unk194`, and `unk1a0`). The latest source exposes the
    inline `GetRadius` formula, while the older RB2 dump only names
    `ComputeRadius` / `SyncRadius` without a usable body. Native GHOGX ports the
    inline formula as `source_char_collide_get_radius`, records the mapped
    cache/update boundary as `source_char_collide_radius_runtime_evidence`, but
    still requires an explicit `SourceCharCollideRadiusCache` and is not wired
    into live hair simulation. Native therefore keeps collision response
    disabled until the cached-field updates are sourced instead of reconstructed
    by guesswork.
- `ihatecompvir-extra/band3_recomp`
  - The current config exposes `CharHair::GetFPS` and `CharHair::Simulate`
    symbols only. It does not provide a decompiled `CharHair` body or
    `CharCollide` hookup implementation for native runtime writeback.

## Face Controller Authorities

- `rb3/src/system/char/CharLookAt.cpp`
  - `CharLookAt::Poll` is the available Harmonix source for eye/look-at runtime
    motion. It reads source/pivot/destination transformables and writes the
    pivot through `SetWorldXfm`; it does not synthesize a head-forward source
    row when a GH2 row names the `CharLookAt` object itself.
  - Native GHOGX now decodes the GH2 revision-2 row in source order:
    `Hmx::Object`, `CharWeightable`, `mSource`, `mPivot`, `mDest`,
    `mHalfTime`, yaw/pitch limits, weight-yaw limits, and weight-yaw speed.
    Native enforces the source `CharLookAt` revision ceiling
    (`ASSERT_REVS(5, 0)`) and the nested `CharWeightable` revision ceiling
    (`ASSERT_REVS(2, 0)`) before consuming those fields. The earlier
    `flags/source/target/driven` labels were the same bytes read under old
    local names, not source truth.
  - `CharLookAt::SyncLimits` clamps yaw and pitch limits to the source
    `[-80, 80]` degree range, computes `mBounds.mMin.y` from the largest
    absolute yaw/pitch limit, sets `mBounds.mMax.y` to `1.0E+29f`, then derives
    yaw Z and pitch X bounds with `tan`. Native ports this as
    `source_char_lookat_sync_limits` for deterministic tests and future
    source-backed `Poll` work.
  - Native `source_char_lookat_set_min_yaw`,
    `source_char_lookat_set_max_yaw`, `source_char_lookat_set_min_pitch`, and
    `source_char_lookat_set_max_pitch` port the four concrete setter bodies:
    each stores the requested angle and immediately re-runs source
    `SyncLimits`, including the source clamp of the stored angle value.
  - Native `source_char_lookat_load_plan` and
    `source_char_lookat_copy_plan` record the source row and copy contracts:
    `Load` accepts revisions 0-5, reads `Hmx::Object`, `CharWeightable`,
    source/pivot/dest, half-time, yaw/pitch limits, revision-gated yaw-weight
    fields, `mAllowRoll`, jitter fields, and source radius, then calls
    `SyncLimits`; `Copy` copies both superclasses, the checked transform/limit/
    jitter/source-radius member list, then also calls `SyncLimits`.
  - Native `source_char_lookat_enter` and `source_char_lookat_poll_deps` port
    the concrete `Enter` reset and `PollDeps` dependency bodies: entering resets
    the smoothed direction row to `(1.0E+29, 0, 0)` and requests a pivot-local
    identity reset only when a pivot exists; dependency publication uses
    `GetSource()` semantics, so an empty source falls back to the pivot, then
    publishes destination as changed-by and pivot as changed. These helpers do
    not claim the full `Poll` transform write because `mBounds.Clamp` and the
    final pivot transform path still need a complete source-backed port.
  - Native `source_char_lookat_poll_plan` ports the checked `Poll` gate and
    branch order as a deterministic plan: missing destination/pivot/source,
    missing pivot parent, or negative delta time keep the row inert; nonzero
    source weight then controls the source yaw-weight, source-radius, pivot
    world write, parent-space clamp, smoothing, test/show range, jitter, and
    roll/no-roll local write branches. This remains a branch contract only; it
    does not synthesize the missing final pivot transform math.
  - Native `source_char_lookat_source_radius_offset` ports the concrete
    source-radius branch inside `CharLookAt::Poll`: when `mSourceRadius > 0`,
    positive delta blends `vec80` 10% toward source world Y, computes `v108 =
    sourceY - vec80`, converts source radius degrees to radians, and clamps
    `v108` length to that radius. This preserves the pre-parent-space offset
    only; it does not subtract the transformed offset from a live look-at
    vector.
  - Native `source_char_lookat_yaw_weight_step` ports the concrete
    `mMinWeightYaw >= 0` branch inside `CharLookAt::Poll`: it normalizes the
    source world Y row before flattening Z, computes the same clamped dot,
    applies the source `mMaxWeightYaw - acos(dot) / (mMaxWeightYaw -
    mMinWeightYaw)` formula, then mirrors the one-sided `MinEq` speed limit
    before multiplying the row weight and updating the stored yaw-weight row.
    This is deterministic math coverage only; it does not create a native eye
    destination or publish a pivot transform for GH2 rows whose `mDest` is
    `<none>`.
  - Native `source_char_lookat_smooth_dir` ports the concrete half-time
    smoothing branch: when the previous direction row is initialized and
    `mHalfTime != 0`, interpolate by `delta / (delta + mHalfTime)` and store
    the smoothed row. Native `source_char_lookat_range_dir` ports the following
    source test/show range override: `mTestRange` wins over `mShowRange`,
    interpolates pitch X and yaw Z from bounds, while `mShowRange` forces row
    weight to one and selects one of the eight source switch vectors, including
    the case-1 `Set(0, mBounds.mMin.z, mBounds.mMax.x)` assignment exactly.
  - Native `source_char_lookat_no_roll_axes` ports the concrete no-roll local
    write branch inside `CharLookAt::Poll`: interpolate the pivot local Y row
    toward the bounded parent-space direction by row weight, seed Z to
    `(-1, 0, 0)`, normalize Y, rebuild X with `Cross(Y, Z)`, rebuild Z with
    `Cross(X, Y)`, and preserve the source invalid-`x.x` identity fallback.
    This is deterministic row math only; it does not publish live eye/look-at
    transforms.
  - Current stock GH2 `CharLookAt` rows observed in the base characters have
    `mDest=<none>`, so the source poll gate would be inert. Native therefore
    keeps these rows decoded/logged and does not publish look-at world rows or
    fabricate a destination.
    `engine/out/source_lookat_20260711/lookat_source_decode_audit.log`
    rechecks Alterna1, Rock2, Rockabill2, and Funk1 with source-shaped fields;
    all sampled rows report `version=2`, `weightableVersion=2`,
    source/pivot on the eye mesh, `dest=<none>`, and `unreadBytes=0`.
- `rb3/src/system/char/CharEyes.cpp`
  - Native `source_char_eyes_default_state` ports the concrete constructor
    defaults for the source-owned data fields: null eye/interest/object refs,
    zero filter flags, `max_extrapolation=19.5`, `min_target_dist=35`,
    upper/lower lid track defaults, focus priority `-1`, timer sentinels, the
    `unkec=1.0` / `unk15d=true` flags, `unkb8=cos(0.52359879)`, and the
    `eye_status` overlay lookup. Fields not initialized by the source
    constructor remain outside this helper rather than being assigned invented
    defaults.
  - Modern revisions read `EyeDesc` rows, but GH2-era revision 3 uses the older
    branch: `CharEyes::Load` reads an `ObjPtrList<CharLookAt>` and converts each
    reference into an eye entry. For revisions 3 and 4 it then consumes one old
    `RndTransformable` pointer. Native GHOGX decodes the GH2 row as that
    look-at reference list plus trailing old transformable (`legacyTransform`),
    not as hidden eye offsets or blink/upper-lid data. The native decoder now
    enforces the source revision range and only consumes that legacy transform
    in source revisions 3 or 4; older leftover bytes remain unread evidence
    instead of being mislabeled as the transform pointer.
    `engine/out/source_chareyes_20260711/chareyes_source_decode_audit.log`
    rechecks Alterna1, Rock2, Rockabill2, and Funk1; all sampled rows report
    `version=3`, two look-at refs, `legacyTransform=<none>`, and
    `unreadBytes=0`.
  - Native `source_char_eyes_eye_desc_load_plan`,
    `source_char_eyes_load_plan`, and `source_char_eyes_copy_plan` expose the
    source row format directly: `EyeDesc` reads eye/upper-lid, lower-lid above
    revision 6, and blink lids above revision 15; `CharEyes::Load` accepts
    revisions 0-18 and preserves the legacy look-at-list, old transform pointer,
    old interest rows, and revision 15/16 lower-lid padding branches; `Copy`
    copies `Hmx::Object`, `CharWeightable`, and the checked eye/filter/lid
    member list.
  - Native `source_char_eyes_handler_plan`,
    `source_char_eyes_prop_sync_plan`, and
    `source_char_eyes_default_interest_categories_sync` record the checked
    handler/property surface: `add_interest`, `force_blink`, the debug
    `toggle_force_focus` / `toggle_interest_overlay` handlers, custom `EyeDesc`
    and `CharInterestState` property rows, the direct `CharEyes` property rows,
    the conditional debug properties, and the source bitfield rule for
    `default_interest_categories`. The helper takes an already-resolved bit mask;
    macro lookup remains source context, not a native parser invention.
  - `CharEyes::ListPollChildren` delegates poll children to the referenced
    `CharLookAt` controllers. It is not evidence for a native bridge that copies
    eye mesh world rows into ad-hoc controller overrides.
  - `CharEyes::PollDeps` publishes same-directory interest objects as
    `changedBy`, publishes `GetHead()` / `GetTarget()` only when the eye list is
    non-empty, then publishes `mHeadLookAt` and `mFaceServo` when present.
    Native `source_char_eyes_*` helpers port these graph/dependency decisions
    plus the concrete `GetHead`, `GetCurrentInterest`, `SetFocusInterest`, and
    `ForceBlink` state bodies for deterministic coverage: view direction wins
    over the first eye source parent, focus interest wins over current interest,
    lower-priority focus requests are rejected, clearing focus resets priority
    to `-1`, and force blink stores the current task time while incrementing
    the blink count by one.
    The checked source does not include a reviewable `CharEyes::GetTarget`
    body. Native `source_char_eyes_runtime_dump_evidence` therefore marks
    `latest_source_has_get_target_body=false`; the `PollDeps` target row is a
    source-visible dependency edge, not evidence for a native target transform
    resolver or an eye accessory placement fix.
  - Native `source_char_eyes_interest_*` helpers port the concrete
    `CharEyes::CharInterestState` refractory timer body: construction/reset set
    the start time to `-1`, beginning a refractory period stores the current
    task time, and both active/remaining queries return inactive/zero when the
    interest pointer is absent or the timer has not been started.
  - Native `source_char_eyes_eye_desc_*` and interest-list helpers port the
    concrete `EyeDesc` constructor/copy/assignment and
    `ClearAllInterestObjects` / `AddInterestObject` bodies: eye and lid refs
    default to null, copy/assignment preserve all five refs, clearing drops the
    interest vector, and add ignores null interests while pushed interests start
    with a reset refractory timer.
  - Native `source_char_eyes_copy_state` ports the concrete `BEGIN_COPYS`
    member list as a constructor-default destination plus only the source
    `COPY_MEMBER` fields. Runtime-only fields intentionally reset instead of
    copying: focus, blink, overlay, current-interest, and interest-filter state
    remain constructor defaults unless the source copy list includes them.
  - Native `source_char_eyes_enter_state` and `source_char_eyes_exit_state`
    port the concrete `Enter` / `Exit` reset bodies as data-only state rows:
    enter zeroes the source unknown counters/flags, sets the source `-1` timer
    sentinels, copies `mDefaultFilterFlags` into `mInterestFilterFlags`,
    normalizes the head world Y row when `GetHead()` resolves, and records the
    number of eye children and interest rows that receive `Enter` / reset.
    Exit clears focus, resets focus priority to `-1`, clears interests, and
    records the eye children that receive `Exit`.
  - Native `source_char_eyes_toggle_force_focus` and
    `source_char_eyes_toggle_interest_overlay` port the concrete handler bodies
    as decisions: force-focus toggling delegates to the same
    `SetFocusInterest` priority gate, and interest overlay toggling only flips
    `mShowing` / restarts the timer when the source overlay pointer exists.
  - Native `source_char_eyes_either_eye_clamped` ports the concrete
    `EitherEyeClamped` query: scan the eye descriptors and return true only
    when a present `CharLookAt` eye has its clamp flag set. It does not invent
    clamp state for missing eye refs.
- Native GHOGX therefore decodes `CharEyes`/`CharLookAt` rows for inspection but
  does not publish synthetic eye runtime rows until a direct source-backed poll
  port has real source data to drive it.
- Native `source_char_eyes_runtime_dump_evidence` records the RB2 dump ranges
  for the missing runtime body and adjacent source-visible helpers: `Poll`
  `0x80354D64 -> 0x80355480`, `NextLook`
  `0x8035559C -> 0x80355A74`, `Replace`
  `0x80355A74 -> 0x80355DCC`, `ListPollChildren`
  `0x80355DCC -> 0x80355E84`, and `PollDeps`
  `0x80355E84 -> 0x80356030`. The visible `Poll` local inventory includes
  `h`, `camWeight`, `blinkWeight`, `blink`, `delta`, `cang`, `sec`, `d`,
  `dest`, `weight`, `srcCam`, two `Transform t` locals, `it`, and `height`.
  This remains range/local evidence only: the checked latest source lacks a
  reviewable `CharEyes::Poll` body, so native keeps
  `safe_to_publish_eye_runtime_rows=false` and
  `safe_to_infer_facefx_rows=false`.
- `rb3-latest/src/system/char/CharEyeDartRuleset.cpp` and
  `CharEyeDartRuleset.h`
  - `EyeDartRulesetData::ClearToDefaults` sets the source defaults:
    radius `0.5..3.0`, on-target angle `5.0`, dart counts `2..5`,
    dart timing `0.25..0.65`, sequence timing `1.0..2.0`,
    `scaleWithDistance=true`, and reference distance `70.0`.
  - `Load` accepts source revisions through 1 and reads the same fields in
    order after `Hmx::Object`.
  - The source `Copy` body copies `mMinRadius`, then assigns destination
    `mMaxRadius` from the source `mMinRadius` rather than the source
    `mMaxRadius`. Native `source_char_eye_dart_ruleset_*` helpers preserve this
    exact data behavior plus load/copy/prop/handler row plans for deterministic
    coverage; they do not enable procedural eye darts in GH2 runtime.
- `rb3-latest/src/system/char/CharInterest.cpp` and `CharInterest.h`
  - The constructor defaults are source data: max view angle `20.0`,
    priority `1.0`, look time `1.0..3.0`, refractory period `6.1`, no dart
    override, category flags `0`, no min-target-distance override, and min
    target distance override `35.0`. `SyncMaxViewAngle` stores
    `cos(maxViewAngle * 0.017453292)`.
  - `Load` accepts source revisions through 6 and reads the object,
    transformable, timing, priority, refractory period, then follows the
    source `u32 temp = gRev + 0x10000` gate. Revisions 2, 3, 4, and 5 read a
    legacy object pointer; revisions 0, 1, and 6 read `mDartOverride`.
    Revisions above 2 read `mCategoryFlags`, revision 3 also consumes one
    legacy byte, and revisions above 4 read the min-target-distance override
    fields before `SyncMaxViewAngle`.
  - `IsMatchingFilterFlags` returns true only when the category mask overlaps
    and the category flags are non-zero. Native `source_char_interest_*`
    helpers port these data decisions, `source_char_interest_load_plan` records
    the concrete source load row order, `source_char_interest_copy_plan`,
    `source_char_interest_prop_sync_plan`,
    `source_char_interest_category_flags_prop_plan`, and
    `source_char_interest_handler_plan` record the source copy/property/handler
    rows, `source_char_interest_is_within_view_cone` ports the nonzero-vector
    source view-cone decision, and the copy helper keeps the max-view-angle
    resync.
    `ComputeScore` includes runtime vectors and `RandomFloat`; live eye-target
    scoring stays fenced until the surrounding source eye-interest path is
    ported or traced. Native `source_char_interest_compute_score_plan` records
    the gate and scoring steps, and
    `source_char_interest_compute_score_deterministic` ports the concrete math
    with the random jitter supplied by the caller: category/default-category
    gate, normalized viewer-to-interest direction, view and interest dot gates,
    distance contribution with the source `NaN -> 0.2` fallback, `-0.99`, the
    nonnegative-score jitter gate, and final priority multiply. This is
    deterministic source-contract coverage only; it is not a live target-picker.
  - `CharInterest::Highlight` is a debug/authoring draw path. It always adds a
    red sphere at the interest position, adds a name string only when
    `WorldToScreen` returns a positive depth using the source `x -= 30`,
    `y += 15` offsets, and draws dart override min/max spheres only when both
    properties exist. Native `source_char_interest_highlight_plan` records that
    draw plan without promoting interest scoring or debug drawing into the live
    runtime target picker.
- `rb3-latest/src/system/char/CharTransCopy.cpp` and
  `CharTransCopy.h`
  - `CharTransCopy::Load` accepts source revisions `0..1`, loads
    `Hmx::Object`, then reads `mSrc` and `mDest`.
  - `CharTransCopy::Copy`, handlers, and prop-sync rows are source-visible:
    copy duplicates `mSrc` and `mDest`, handlers dispatch through
    `RndPollable` then `Hmx::Object` with check `0x4C`, and properties expose
    `src` and `dest`.
  - `CharTransCopy::Poll` returns immediately when either `mSrc` or `mDest` is
    missing. With both refs present, it calls `mDest->SetLocalXfm(mSrc->mLocalXfm)`.
  - `CharTransCopy::PollDeps` appends `mDest` to the `change` list and `mSrc`
    to `changedBy`, matching the source dependency direction.
  - Native `source_char_trans_copy_load_plan`,
    `source_char_trans_copy_copy_plan`,
    `source_char_trans_copy_handler_plan`,
    `source_char_trans_copy_prop_sync_plan`,
    `source_char_trans_copy_poll`, and
    `source_char_trans_copy_poll_deps` port those complete source behaviors as
    isolated helpers. This does not imply active character runtime wiring unless
    stock `CharTransCopy` rows are decoded or another source-backed owner path
    is proven.
- `rb3-latest/src/system/char/CharPollGroup.cpp` and
  `CharPollGroup.h`
  - `CharPollGroup::Load` accepts source revisions `0..3`, loads
    `Hmx::Object`, loads `CharWeightable` only above revision 2, always reads
    `mPolls`, and reads `mChangedBy` / `mChanges` only above revision 1.
  - `CharPollGroup::Copy`, `SortPolls`, handlers, and prop-sync rows are
    source-visible. The `kCopyFromMax` branch appends missing poll refs only;
    normal copy duplicates `mPolls`, `mChangedBy`, and `mChanges`.
    `SortPolls` delegates ordering to `CharPollableSorter`, the handler table
    exposes `sort_polls` with check `0xA2`, and prop-sync exposes `polls`,
    `changed_by`, `changes`, then `CharWeightable`.
  - `CharPollGroup::Enter` and `CharPollGroup::Exit` iterate every child in
    `mPolls` list order.
  - `CharPollGroup::Poll` iterates `mPolls` only when the source weight owner
    weight is nonzero. Zero weight skips every child.
  - `ListPollChildren` appends every poll child in list order.
  - `PollDeps` uses the explicit `mChangedBy` / `mChanges` pair when either
    pointer exists; otherwise it delegates dependency collection to each child
    pollable in list order.
  - Native `source_char_poll_group_load_plan`,
    `source_char_poll_group_copy_plan`,
    `source_char_poll_group_handler_plan`,
    `source_char_poll_group_prop_sync_plan`,
    `source_char_poll_group_sort_plan`,
    `source_char_poll_group_enter_order`,
    `source_char_poll_group_exit_order`,
    `source_char_poll_group_poll_order`,
    `source_char_poll_group_list_children`, and
    `source_char_poll_group_poll_deps` port those source decisions for tests and
    future decode work. Stock GH2 base-character inventory still proves zero
    `CharPollGroup` rows, so no hidden runtime poll group is synthesized.
- `rb3-latest/src/system/char/CharIKScale.cpp` and
  `CharIKScale.h`
  - The constructor defaults are source state: `mScale = 1.0f`,
    `mBottomHeight = 0.0f`, `mTopHeight = 0.0f`, and `mAutoWeight = false`.
  - `Poll` only enters its body when `mDest` exists and `Weight()` is nonzero;
    the checked source body contains no implemented scale write after that
    gate.
  - `CaptureBefore` stores `mDest->mLocalXfm.v.z` into `mScale` when `mDest`
    exists. `CaptureAfter` stores `mDest->mLocalXfm.v.z / mScale`, with no
    source zero guard beyond the missing-destination return.
  - `PollDeps` pushes `mDest` and each secondary target into the `change` list,
    then pushes `mDest` into `changedBy`.
  - Native `source_char_ik_scale_*` helpers port those complete source-visible
    behaviors only. They do not invent the absent scale-write body.
- `rb3-latest/src/system/char/CharTransDraw.cpp`,
  `CharTransDraw.h`, and `Character.h`
  - `Character::DrawMode` source values are `kCharDrawNone`,
    `kCharDrawOpaque`, `kCharDrawTranslucent`, and `kCharDrawAll` in that
    order.
  - `CharTransDraw::Load` reads `Hmx::Object`, `RndDrawable`, then `mChars`,
    and immediately sets every referenced character to `kCharDrawOpaque`.
  - `CharTransDraw::Copy`, handlers, and prop-sync rows are source-visible:
    copy duplicates `mChars` after the `Hmx::Object` and `RndDrawable`
    superclasses, handlers delegate to `RndDrawable` then `Hmx::Object` with
    check `0x5E`, and prop-sync exposes `chars` then `RndDrawable`.
  - `CharTransDraw::~CharTransDraw` restores every referenced character to
    `kCharDrawAll`.
  - `DrawShowing` skips hidden characters. For each showing character it sets
    draw mode to `kCharDrawTranslucent`, calls `Draw`, then restores
    `kCharDrawOpaque`.
  - Native `source_char_trans_draw_*` helpers port that command order for tests
    and future source-backed wiring only. They do not alter material state,
    depth behavior, sort order, or the project-level hair two-sided rule.
- `rb3-latest/src/system/char/CharCuff.cpp` and `CharCuff.h`
  - The constructor seeds three source shape rows:
    `offset/radius = -2.9/1.9`, `0.0/2.6`, and `2.0/3.5`; `mOuterRadius`
    defaults to the middle radius plus `0.5`; `mOpenEnd` defaults false;
    `mEccentricity` defaults `1.0`.
  - `CharCuff::Eccentricity` computes
    `sqrt((y*y + x*x) / (y*y * (1 / eccentricity^2) + x*x))`.
  - `CharCuff::Load` accepts source revisions through 8. Older revisions
    default `outer_radius`, `open_end`, `bone` from `TransParent`, eccentricity,
    category, and ignore rows behind the exact source gates.
    Native `source_char_cuff_load_plan` records the source read order, revision
    gates, and old-row warning branch.
  - `CharCuff::Copy` copies `Hmx::Object`, `RndTransformable`, the three shape
    rows, `mOuterRadius`, `mOpenEnd`, `mBone`, `mEccentricity`, `mCategory`,
    and `mIgnore`. Native `source_char_cuff_copy_plan` records that source copy
    list without adding a live deformation path.
  - Native `source_char_cuff_handler_plan` and
    `source_char_cuff_prop_sync_plan` port the visible source handler chain,
    check value `0x1FE`, direct shape/outer/open/bone/eccentricity/category/
    ignore property rows, and `RndTransformable` superclass.
  - Source `AddBoneChildren` only appends a transform when the current
    transform name starts with `bone_`, then recurses into that transform's
    children. It does not scan below a null or non-`bone_` root. Native
    `source_char_cuff_add_bone_children` ports that exact collection rule for
    deterministic accessory/deformation diagnostics.
  - Native `source_char_cuff_*` helpers port those complete source-visible data
    rules only. The deformation path, bone mask helper, and mesh callbacks are
    not promoted without source-backed stock rows or a complete runtime owner.
- `rb3-latest/src/system/char/CharBlendBone.cpp` and
  `CharBlendBone.h`
  - The constructor defaults `mTransX`, `mTransY`, `mTransZ`, and `mRotation`
    to false and leaves source pointers empty.
  - `ConstraintSystem` defaults `mWeight` to `0.5`.
  - `Load` reads `mTargets`, `mSrc1`, `mSrc2`, `mTransX`, `mTransY`,
    `mTransZ`, and `mRotation` after `Hmx::Object`.
    `ConstraintSystem` load reads `mTarget` then `mWeight`.
    Native `source_char_blend_bone_load_plan` and
    `source_char_blend_bone_constraint_load_plan` record those exact source
    read orders and revision gates.
  - `Copy` copies `Hmx::Object`, `mTargets`, `mSrc1`, `mSrc2`, `mTransX`,
    `mTransY`, `mTransZ`, and `mRotation`. Native
    `source_char_blend_bone_copy_plan` records that list without adding a blend
    solve.
  - Native `source_char_blend_bone_handler_plan`,
    `source_char_blend_bone_constraint_prop_sync_plan`, and
    `source_char_blend_bone_prop_sync_plan` port the visible source handler
    chain, check value `0x8F`, constraint `target`/`weight` rows, and
    `targets`/`src_one`/`src_two`/translation/rotation rows.
  - `PollDeps` pushes `mSrc1` and `mSrc2` into `changedBy`, then pushes every
    constraint target into `change`.
  - Native `source_char_blend_bone_*` helpers port those source-visible data and
    dependency rules only. The checked source marks `Poll` but does not include
    its body, so native must not invent blend output math from the field names.
- `rb3-latest/src/system/char/CharSleeve.cpp` and `CharSleeve.h`
  - The constructor defaults `mLastDT = 0`, `mInertia = 0.5`, `mGravity = 1`,
    `mRange = 0`, `mNegLength = 0`, `mPosLength = 0`, and
    `mStiffness = 0.02`.
  - `SetName` records the owning `Character` when the object directory is a
    character.
  - `Poll` gates on `mSleeve` and its parent. It uses task delta seconds,
    source stiffness decay, optional teleport reset from the owner character,
    inertia from `mLastPos`/`mLastDT`, gravity, range clamp, and the checked
    source length/interp block before writing the sleeve world transform. When
    `mTopSleeve` exists, it removes the parent X projection and writes a second
    top-sleeve transform.
  - `PollDeps` pushes the sleeve parent into `changedBy`, then pushes `mSleeve`
    and `mTopSleeve` into `change` only when `mSleeve` exists.
  - `Load` accepts only source revision 0, delegates to `Hmx::Object::Load`,
    then reads `mSleeve`, `mTopSleeve`, `mInertia`, `mGravity`, `mStiffness`,
    `mRange`, `mNegLength`, and `mPosLength`. `Copy` copies `Hmx::Object` and
    the same eight sleeve data members in source order.
  - `BEGIN_HANDLERS(CharSleeve)` delegates to `Hmx::Object` and checks
    `0x112`. `BEGIN_PROPSYNCS(CharSleeve)` exposes `sleeve`, `top_sleeve`,
    `inertia`, `gravity`, `stiffness`, `range`, `neg_length`, and
    `pos_length`. Native `source_char_sleeve_handler_plan` and
    `source_char_sleeve_prop_sync_plan` record those rows as source metadata
    only.
  - Native `source_char_sleeve_*` helpers port this source-visible simulation
    dependency, row-load, and copy behavior for deterministic tests. They do
    not attach it to live character rendering until stock rows and owner
    ordering are decoded.
- `rb3-latest/src/system/char/CharMeshHide.cpp` and
  `CharMeshHide.h`
  - `CharMeshHide::HideAll` first ORs the incoming flag word with every
    `CharMeshHide::mFlags` owner row, then calls `HideDraws` on each owner with
    the combined result.
  - `CharMeshHide::HideDraws` only mutates rows with a valid drawable pointer;
    their stored `show` state becomes `((combinedFlags & rowFlags) == 0) &
    drawable->Showing()`. Rows without a drawable are left untouched.
  - Native `source_char_mesh_hide_all` / `source_char_mesh_hide_draws` ports
    that complete flag and drawable-showing behavior as a deterministic helper.
    This is visibility-row source behavior only; renderer wiring must wait for
    proven stock `CharMeshHide` rows or an equivalent source-backed data path.
- `rb3-latest/src/system/char/CharFaceServo.cpp` and
  `CharFaceServo.h`
  - `CharFaceServo::Load` is the available source for modern face servo rows:
    it reads `Hmx::Object`, an `ObjectDir` clip-set pointer, an optional
    revision-above-3 clip-type `Symbol`, blink clip name symbols behind
    revision gates, then calls `SetClips` and `SetClipType`.
  - `CharFaceServo::Poll` scales down the base clip, applies identity,
    rotates the base clip into the servo, and poses meshes. This is useful
    source context for a future source-backed face-servo port.
  - Native `source_char_face_servo_load_plan`,
    `source_char_face_servo_copy_plan`, `source_char_face_servo_handler_plan`,
    `source_char_face_servo_prop_sync_plan`,
    `source_char_face_servo_enter_plan`,
    `source_char_face_servo_set_clips_plan`,
    `source_char_face_servo_set_clip_type_plan`,
    `source_char_face_servo_poll_plan`,
    `source_char_face_servo_procedural_weights_plan`, and
    `source_char_face_servo_poll_deps_plan` record the visible source
    row/control-flow contracts for `CharFaceServo`: revisions `0..4`, legacy
    clip-type derivation from the clip directory, blink clip name revision
    gates, `SetClips` lookup names, changed clip-type bone rebuild, base-clip
    poll order, procedural blink gate, property rows, and handler check
    `0x119`. These plans remain source context and do not promote
    `FaceFxLipSyncServo` rows into `CharFaceServo` behavior.
  - Native `source_char_face_servo_apply_procedural_weights` ports the concrete
    `ApplyProceduralWeights` blink math: positive unapplied procedural weight
    first consumes `TryScaleDown`, left blink applies `(1 - mBlinkWeightLeft) *
    mProceduralBlinkWeight` when a left clip exists, right blink applies
    `(1 - mBlinkWeightRight) * mProceduralBlinkWeight` only when a right clip
    exists and is not the same object as the left clip, and the source marks
    procedural blink applied after the gated update.
  - Native `source_char_face_servo_scale_add_blink` ports the bounded,
    complete blink-weight part of `CharFaceServo::ScaleAdd`: non-relative clips
    do not enter the source update path; accepted relative clips first consume
    the source `TryScaleDown` reset, then add left/right blink weight only when
    the clip matches the left/left2 or right/right2 blink rows, clamping each
    side to `[0, 1]`. This is source context for blink accumulation only, not a
    runtime face mesh bridge.
  - Stock GH2 PS2 base characters use rows named `FaceFxLipSyncServo`, not
    `CharFaceServo`. The checked ihatecompvir snapshots do not expose a
    matching `FaceFxLipSyncServo::Load` body. Native GHOGX therefore treats its
    `FaceFxLipSyncServo` decoder as bounded GH2 compatibility for locating
    `.fac` files, viseme MILOs, and target object/property rows; it is not
    source evidence for synthetic eye rows, mouth offsets, or a face-controller
    runtime bridge.
  - The old native `FaceFxEyeProperties` bridge and gameplay
    eye-register-name inference were removed. `CharEyes::Poll` and
    `FaceFxLipSyncServo::Load` are not available from the checked
    ihatecompvir sources, so gameplay must not synthesize FaceFX registers from
    decoded eye rows. FaceFX runtime registers remain limited to sampled
    FaceFX animation curves until a source-backed producer exists.
- `rb3-latest/src/system/char/CharLipSync.cpp` and
  `rb3-latest/src/system/char/CharLipSync.h`
  - `CharLipSync::Generator` initializes the lip-sync pointer to null,
    `mLastCount` to zero, and the weight list empty.
  - `CharLipSync` initializes `mPropAnim` to null and `mFrames` to zero.
  - `CharLipSync::Load` accepts source revisions through 1, reads
    `Hmx::Object`, then viseme names, frame count, raw data, and only reads
    `mPropAnim` when the revision is non-zero.
  - `CharLipSync::Save` uses source save id `0x155`. The checked
    `CharLipSync.cpp` snapshot does not include `BEGIN_COPYS`,
    `BEGIN_HANDLERS`, or `BEGIN_PROPSYNCS` rows, so native coverage records
    the save/load/default boundary only.
- `rb3-latest/src/system/char/CharLipSyncDriver.cpp` and
  `rb3-latest/src/system/char/CharLipSyncDriver.h`
  - `CharLipSyncDriver` inherits `RndHighlightable`, `CharWeightable`, and
    `CharPollable`. Its constructor initializes all object pointers to null,
    `mSongOffset` to `0.0`, `mLoop` false, `mSongPlayer` null,
    `mTestWeight` to `1.0`, `mOverrideWeight` to `0.0`, and
    `mApplyOverrideAdditively` false.
  - The checked `PollDeps` body appends only `mBones` to the changed row list.
  - The checked source declares `Poll`, `Enter`, `SetClips`, `SetLipSync`,
    `Load`, and `Copy`, but this snapshot only includes the constructor and
    `PollDeps` body. Native `source_char_lip_sync_*` helpers therefore port
    defaults/save id/load gates/dependency publication as source context only
    and do not promote any live GH2 mouth or viseme controller behavior.

## Rnd Utility Row Authorities

- `rb3-latest/src/system/rndobj/Anim.cpp` and
  `rb3-latest/src/system/rndobj/Anim.h`
  - `RndAnimatable::Load` reads a source revision, optional `mFrame`, then
    `mRate` for revisions above 3 or a legacy byte rate for revision 3. Revision
    0 branches into an old anim-filter/object-list conversion path.
  - `RndAnimatable` defaults to frame `0.0` and `k30_fps`. The source rate
    tables map `k30_fps`, `k30_fps_ui`, and `k30_fps_tutorial` to `30` frames
    per second-style task units, `k480_fpb` to `480` frames per beat, and
    `k1_fpb` to `1` frame per beat. `ConvertFrames` divides by the selected
    frames-per-unit row and reports conversion for non-beat units.
  - The source handler row is `set_frame`, `frame`, `set_key`, `end_frame`,
    `start_frame`, `animate`, `stop_animation`, `is_animating`, and
    `convert_frames`; the property row exposes `rate` and `frame` through
    `SetFrame`.
  - `OnAnimate` starts from source defaults (`StartFrame`, `EndFrame`, `Loop`,
    `Units`, `FramesPerUnit`) and then applies `range`, `loop`, `dest`, and
    `period` rows before creating an `AnimTask`. Named tasks require `DataThis`;
    wait mode only waits cleanly when the blend task uses the same rate.
  - `AnimTask` records min/max, direction scale, offset, loop, and blend period,
    scans the animation target refs for a prior task, marks the old task as
    blending when needed, calls `StartAnim`, and computes time-until-end from
    the active frame and frames-per-unit.
  - Native GHOGX decodes the revisioned frame/rate fields, exposes
    `source_rndanimatable_*` and `source_anim_task_*` helpers for shared
    embedded-base tests, and fences the revision-0 object-list branch until the
    relevant object-list serialization path is source-backed in this decoder.
- `rb3-latest/src/system/rndobj/AnimFilter.cpp` and
  `rb3-latest/src/system/rndobj/AnimFilter.h`
  - `RndAnimFilter::Load` accepts source revisions through 2. It loads
    `Hmx::Object`, then `RndAnimatable`, then reads `mAnim`, `mScale`,
    `mOffset`, `mStart`, and `mEnd`. Nonzero revisions read `mType` and
    `mPeriod`; revision 0 reads a legacy loop byte. Revisions above 1 read
    `mSnap` and `mJitter`.
  - Native GHOGX decodes and logs this row for stock-model evidence only. It
    does not schedule `RndAnimFilter`, attach it to `EventTrigger`, or mutate
    `RndAnimatable` playback.

## Event Trigger Row Authority

- `rb3-latest/src/system/rndobj/EventTrigger.cpp` and
  `rb3-latest/src/system/rndobj/EventTrigger.h`
  - `EventTrigger::Load` uses `LOAD_REVS`, `Hmx::Object`, optional
    `RndAnimatable` for revisions above `0x0f`, then revision-gated rows:
    trigger events, anims, sounds, shows, hide delays, enable/disable/wait
    events, next link, proxy calls, trigger order, reset triggers, reset-self
    bitfield, animation trigger, animation frame, and part launchers.
  - `EventTrigger::Anim` reads `mAnim`, `mBlend`, `mWait`, and `mDelay`.
    Revisions above 9 also read `mEnable`, `mRate`, start/end/period/type, and
    scale. Older revisions reset those later fields to source defaults.
  - `EventTrigger::ProxyCall` reads proxy and call rows; revisions above 10
    then load the optional event through the source object pointer path.
  - `EventTrigger::HideDelay` reads hide object, delay, and rate.
- `rb3-latest/src/system/obj/ObjVector.h`
  - `ObjVector` serialization is count-prefixed, then each row is loaded with
    its `operator>>`. Native uses this shape for EventTrigger anim, proxy-call,
    and hide-delay row vectors.
- `rb3-latest/src/system/obj/ObjPtr_p.h`
  - `ObjPtr::Load` and `ObjOwnerPtr::Load` read one `0x80`-bounded source
    string. `ObjPtrList::Load` reads a count and then one `0x80`-bounded source
    string per row. Native logs those names and does not require object
    resolution for passive inventory.
- `rb3-latest/src/system/utl/BinStream.h` and
  `rb3-latest/src/system/utl/BinStream.cpp`
  - `std::vector` rows are count-prefixed. `Symbol` rows are serialized through
    `ReadString`, and `bool` rows are one byte.
- Native `source_event_trigger_load_plan` records the ihatecompvir
  `EventTrigger::Load` revision gates, legacy branches, nested
  `EventTrigger::Anim` / `ProxyCall` / `HideDelay` row order, and final cleanup
  calls as deterministic contract evidence. This helper is passive: it does
  not register events, trigger animations, play sounds, hide/show drawables,
  launch particles, or schedule tasks.
- Native `source_event_trigger_default_state` and
  `source_event_trigger_copy_plan` record the checked source constructor and
  copy contract: source construction registers events and defaults
  `mAnimFrame`, `mTriggerOrder`, `mAnimTrigger`, `unkde`, `unkdf`, `mEnabled`,
  and `mEnabledAtStart`; copy runs `UnregisterEvents`, copies the explicit
  source member list, then runs `RegisterEvents` and `CleanupHideShow`.
  Runtime-only fields such as spawned tasks and enabled state are documented as
  not copied by that source body.
- Native `source_event_trigger_handler_plan` and
  `source_event_trigger_prop_sync_plan` record the checked handler and property
  rows: `trigger`, `enable`, `disable`, `wait_for`, `proxy_calls`,
  `supported_events`, `basic_cleanup`, the `Anim` / `ProxyCall` / `HideDelay`
  custom property rows, event-list unregister/register wrapping, and
  `RndAnimatable` superclass handoff. This remains row-surface evidence only,
  not event execution.
- Native GHOGX currently decodes EventTrigger rows as passive source inventory
  only. It does not register events, trigger animations, play sounds, hide/show
  drawables, or schedule tasks.
- Focused stock proof at
  `engine/out/source_truth_eventtrigger_20260710/metal_drummer_eventtrigger_inventory.stdout.log`
  records the only stock row as `char=metal_drummer name=game_over.trig
  version=8`, with `triggerEvents=1 event=game_over`, `anims=1
  anim=crash_static.filt`, `unreadBytes=4`, and `tailHex=00:00:00:00`.
  That four-byte zero tail is logged and left unread until an ihatecompvir
  source path identifies it.

## Position Constraint Authorities

- `rb3-latest/src/system/char/CharPosConstraint.cpp` and
  `rb3-latest/src/system/char/CharPosConstraint.h`
  - `CharPosConstraint::Load` accepts source revisions through 2. It reads
    `Hmx::Object`, then the `targets` object list, then the `source`
    transformable pointer. For revisions greater than 1 it reads `mBox`;
    older rows default to min `(1, 1, 0)` and max `(-1, -1, 1000)`.
    Native `source_char_pos_constraint_load_plan` records that read order,
    revision gate, and old-box default branch, and the decoder rejects rows
    outside the source revision range before consuming superclass payload.
  - `CharPosConstraint::Copy` copies `Hmx::Object`, `mTargets`, `mSrc`, and
    `mBox`. `PollDeps` publishes `mSrc` as an input dependency and publishes
    each target as both changed-by and changed rows. Native
    `source_char_pos_constraint_copy_plan` and
    `source_char_pos_constraint_poll_deps_plan` record those source-visible
    contracts.
  - The source header defines `mBox` as a `Box` with `mMin` and `mMax` vector
    members; native therefore decodes the revision-2 box row as min vector
    followed by max vector. The exact `Geo.h` serialization body is not present
    in the checked sparse source snapshot, so this is backed by the source
    member layout and `Poll` field usage rather than an operator body.
  - `CharPosConstraint::Poll` copies each target's current world transform,
    clamps target-source position deltas independently for any axis whose
    `mMin` is less than or equal to `mMax`, then writes the target through
    `SetWorldXfm`.
  - Native GHOGX ports this `Poll` path directly: it resolves the source and
    target current world rows, clamps target-source deltas on enabled axes, and
    publishes the target through the runtime world-row writer. The shared
    `source_char_pos_constraint_target_position` helper is the deterministic
    source-body slice used by both focused tests and the runtime controller pass.

## Waypoint Clip/Path Diagnostic Authorities

- `rb3-latest/src/system/char/Waypoint.cpp` and
  `rb3-latest/src/system/char/Waypoint.h`
  - `Waypoint` constructor defaults are `mFlags=0`, `mRadius=12`,
    `mYRadius=0`, `mAngRadius=0`, `mStrictAngDelta=0`,
    `mStrictRadiusDelta=0`, and an owned `mConnections` vector.
  - `Waypoint::Load` accepts source revisions through 5. Native
    `source_waypoint_load_plan` records the same row order: `Hmx::Object`, a
    legacy drawable for revisions below 5, `RndTransformable`, `mFlags`,
    `mConnections`, optional radius, optional `mYRadius`/`mAngRadius`, and
    optional strict radius/angle deltas.
  - `Waypoint::Copy` copies `Hmx::Object`, `RndTransformable`, `mFlags`,
    `mConnections`, `mRadius`, `mYRadius`, `mAngRadius`,
    `mStrictRadiusDelta`, and `mStrictAngDelta`.
  - Native `source_waypoint_handler_plan` records the source handler
    superclass order (`RndTransformable`, `Hmx::Object`) and check `524`.
    Native `source_waypoint_prop_sync_plan` records direct props (`flags`,
    `radius`, `y_radius`, `strict_radius_delta`, `connections`), set props
    (`ang_radius`, `strict_ang_delta`), and the `RndTransformable` superclass.
  - `ShapeDeltaBox` has two source branches. With a positive Y radius it clamps
    local X and Y dot products against the waypoint world rows and returns only
    the clamped X/Y correction. Without a positive Y radius it pulls the
    subject toward the circular radius in the XY plane and zeroes Z.
  - `ShapeDeltaAng` returns
    `LimitAng(GetZAngle(WorldXfm().m) - subject_z_angle) -
    Clamp(-radius, radius, limited)`.
  - `Constrain` applies strict radius correction only when
    `mStrictRadiusDelta > 0` and strict angle correction only when
    `mStrictAngDelta > 0`, writing the adjusted transform rows.
  - Native helpers are source-only deterministic diagnostics for decoded
    waypoint semantics. They do not create live camera, navigation, or path
    behavior.

## Bone Offset Authorities

- `rb3-latest/src/system/char/CharBoneOffset.cpp` and
  `rb3-latest/src/system/char/CharBoneOffset.h`
  - `CharBoneOffset::Load` accepts source revisions through 1. It reads
    `Hmx::Object`, then the destination transform pointer, then the offset
    vector.
  - `CharBoneOffset::Poll` returns immediately when the destination or its
    parent transform is missing. Otherwise it copies the destination local
    transform, adds `mOffset` to the local translation row, multiplies that
    adjusted local row by the parent world transform, and writes the destination
    through `SetWorldXfm`.
  - `CharBoneOffset::ApplyToLocal` adds the same offset directly to the
    destination local translation row.
  - Native GHOGX now decodes and audits `CharBoneOffset` rows, and
    `source_char_bone_offset_poll_world` /
    `source_char_bone_offset_apply_to_local` port those source math paths as
    deterministic helpers. It does not add a live frame-cadence write until
    stock data or source poll ordering proves where that controller should run.

## Bone Twist Authorities

- `rb3-latest/src/system/char/CharBoneTwist.cpp` and
  `rb3-latest/src/system/char/CharBoneTwist.h`
  - `CharBoneTwist::Load` accepts source revision 0. It reads `Hmx::Object`,
    then `CharWeightable`, then the driven bone pointer and target transform
    list.
  - `CharBoneTwist::Poll` returns when the driven bone is missing or the target
    list is empty. Otherwise it averages target world positions, computes the
    target direction from the driven bone to that average, removes the
    component along the driven bone's X row, normalizes the remaining direction,
    interpolates the driven bone's Y row toward it by `Weight()`, normalizes Y,
    rebuilds Z from `Cross(X, Y)`, scales Z by the original X-row length, and
    writes the driven bone through `SetWorldXfm`.
  - Native GHOGX now decodes and audits `CharBoneTwist` rows, including the
    source `CharWeightable` revision, weight, optional weight owner, driven
    bone, and target list. `source_char_bone_twist_weight` and
    `source_char_bone_twist_poll_world` port the source weight lookup and world
    row solve as deterministic helpers. Native does not add a live frame-cadence
    write until stock data or source poll ordering proves where that controller
    should run.

## IK Controller Authorities

- `rb3-latest/src/system/char/CharWeightable.cpp` and
  `rb3-latest/src/system/char/CharWeightable.h`
  - `CharWeightable::Load` reads a source revision, `mWeight`, and
    `mWeightOwner` when the revision is greater than 1.
  - `CharWeightable::Weight()` returns the owner row's weight, not merely the
    object's local serialized value. Native therefore keeps `weight_owner` as a
    named source row instead of treating it as a generic UI property.
  - Native `source_char_weightable_default_state`,
    `source_char_weightable_set_weight`,
    `source_char_weightable_set_weight_owner`,
    `source_char_weightable_replace`, and `source_char_weightable_copy` port the
    concrete source state behavior: constructor `mWeight=1.0f` and
    `mWeightOwner=this`, null owners fall back to `this`, `Replace` swaps the
    owner only when it matches and again falls back to `this` on null, shallow
    copies keep the source owner, and non-shallow copies own themselves while
    copying the source owner's current weight.
  - Native `source_char_weightable_load_plan` and
    `source_char_weightable_copy_plan` record the source load revision range
    and copy branches: revisions 0-2 read `mWeight`, revisions above 1 also
    read `mWeightOwner`; shallow copy preserves the source owner, while
    non-shallow copy owns itself and copies the source owner's current weight.
  - Native `source_char_weightable_handler_plan` and
    `source_char_weightable_prop_sync_plan` record the visible source handler
    chain, check value `0x43`, `weight`/`weight_owner` property rows,
    `SetWeight`/`SetWeightOwner` set branches, get branches, and the source
    `0x40` blocked-op behavior.
- `rb3-latest/src/system/char/CharMirror.cpp` and
  `rb3-latest/src/system/char/CharMirror.h`
  - `CharMirror` inherits `CharWeightable` and `CharPollable`; its constructor
    initializes null `mServo`/`mMirrorServo`, empty `mBones`, and empty `mOps`.
  - `Poll` reads `Weight()` and only calls `mBones.ScaleDown(*mServo.Ptr(),
    1.0f - weight)` when the weight is nonzero and `mBones.TotalSize()` is not
    zero. Native `source_char_mirror_poll` ports that exact gate and scale
    weight without inventing a servo null guard.
  - `SetServo` and `SetMirrorServo` only assign and call `SyncBones` when the
    pointer value changes. `PollDeps` appends `mServo` to `change`.
  - `Load` accepts source revision 1, loads `Hmx::Object`, loads
    `CharWeightable`, reads `mMirrorServo`, reads `mServo`, and calls
    `SyncBones`. `Copy` copies `Hmx::Object` and `CharWeightable`, then routes
    both servo pointers through the source setters.
  - The actual `SyncBones` rebuilding body is not present in `rb3-latest`
    `CharMirror.cpp`; native records the source call sites but does not claim
    mirrored bone output until that body is imported from an authoritative
    source.
- `rb3-latest/src/system/char/CharWeightSetter.cpp` and
  `rb3-latest/src/system/char/CharWeightSetter.h`
  - Native `source_char_weight_setter_default_state` and
    `source_char_weight_setter_set_weight` port the concrete constructor and
    `SetWeight` source bodies: `mBase`/`mDriver` are null, min/max lists are
    empty no-null lists, `mFlags=0`, `mOffset=0`, `mScale=1`,
    `mBaseWeight=0`, `mBeatsPerWeight=0`, and `SetWeight` writes both
    `mBaseWeight` and inherited `mWeight`.
  - `CharWeightSetter::Load` reads `Hmx::Object`, then `CharWeightable` for
    revisions above 1, followed by `driver`, `flags`, revision-gated
    `offset`/`scale`, old owner lists for revisions below 2, `base_weight` and
    `beats_per_weight` above revision 4, optional `base` above revision 5, and
    min/max setter refs through the revision 7/8 single-pointer rows or the
    revision 9 lists. Native GHOGX enforces that source revision range and logs
    the row tail byte count.
  - Native `source_char_weight_setter_load_plan` and
    `source_char_weight_setter_copy_plan` now expose that exact revision-gated
    source read order and copy list for deterministic tests. This records the
    legacy revision 3 invert-bool branch and the revision 7/8 single min/max
    pointer branch without activating driver-backed `EvaluateFlags`.
  - Native `source_char_weight_setter_handler_plan` and
    `source_char_weight_setter_prop_sync_plan` port the visible source
    `Hmx::Object` handler chain, check value `0xF4`, direct property rows
    (`driver`, `flags`, `base`, `offset`, `scale`, `base_weight`,
    `beats_per_weight`, `min_weights`, `max_weights`), and
    `CharWeightable` superclass.
  - `CharWeightSetter::Poll` derives `base_weight` from either
    `mDriver->EvaluateFlags(mFlags)` or `mBase->Weight()`, applies
    `scale`/`offset`, clamps through min/max setter rows, and then either snaps
    or beat-smooths `mWeight`.
  - Native `source_char_weight_setter_poll` ports the source non-driver path:
    `CharWeightable::Weight()` owner lookup, optional `base` weighting,
    min/max setter clamps, snap, and `beats_per_weight` smoothing. Rows with
    `driver` set remain logged/skipped until a source-backed
    `CharDriver::EvaluateFlags` body is available.
  - Native `source_char_weight_setter_poll_deps` ports the concrete
    `CharWeightSetter::PollDeps` dependency publication: `mDriver`, `mBase`,
    every `mMinWeights` row, and every `mMaxWeights` row are appended to
    `changedBy`; reverse `Refs()` owners are appended to `change` only when the
    ref owner is a `CharWeightable` whose `mWeightOwner` is this setter.
  - Native `source_char_weight_setter_runtime_dump_evidence` records the RB2
    dump ranges around the same runtime surface: `Poll`
    `0x8039D368 -> 0x8039D500`, `PollDeps`
    `0x8039D500 -> 0x8039D73C`, `Load`
    `0x8039D83C -> 0x8039DC40`, and `Copy`
    `0x8039DC40 -> 0x8039DDA0`. The visible local inventory is limited to
    `Poll` local `delta`, `PollDeps` locals `it`/`w`, and `Load` locals
    `w`/`it`. This evidence object keeps `safe_to_run_driver_branch=false`
    and `safe_to_publish_driver_weight=false`: the latest source shows the
    driver call site, but native still cannot evaluate the driver-backed branch
    without a source-backed `CharDriver::EvaluateFlags` body.
  - `engine/out/source_weightsetter_20260711/stock_weightsetter_controllers.stdout.log`
    refreshes stock proof against the current decoder: all 38 stock
    `CharWeightSetter` rows are `version=2`, use `CharWeightable` revision 2,
    and report `unreadBytes=0`.
- `rb3-latest/src/system/char/CharIKHead.cpp` and
  `rb3-latest/src/system/char/CharIKHead.h`
  - `CharIKHead` inherits `RndHighlightable`, `CharWeightable`, and
    `CharPollable`. Constructor defaults are source data: null head/spine/
    mouth/target/offset pointers, head filter `(0,0,0)`, target radius `0.75`,
    head mat `0.5`, offset scale `(1,1,1)`, update-points true, and null
    character pointer.
  - `SetName` delegates to `Hmx::Object::SetName` and stores the owning
    `Character` only when the supplied directory is a `Character`.
  - `PollDeps` appends mouth, head, and target to `changedBy`; if
    `GenerationCount(mSpine, mHead)` is non-zero it appends each transform from
    head through the spine-parent boundary to `change`; it always appends
    offset to `change`.
  - `UpdatePoints` only enters when forced or `mUpdatePoints` is true. It then
    clears the dirty flag and point rows; when the generation count is non-zero
    it builds `gencnt + 1` point rows from head upward, stores each local-vector
    length, sums `mSpineLength`, and assigns each point's normalized remaining
    chain length.
  - `Load` accepts source revisions through 3, reads `Hmx::Object`, then
    `CharWeightable`, then head/spine/mouth/target; revisions above 1 read
    target radius and head mat; revisions above 2 read offset and offset scale;
    load always marks update-points true.
  - Native `source_char_ik_head_*` helpers port these concrete source behaviors
    for tests and future row wiring. The checked file does not include a
    reviewable `CharIKHead::Poll` body, so native does not invent head IK
    solving from these data helpers.
- `rb3-latest/src/system/char/CharIKFoot.cpp` and
  `rb3-latest/src/system/char/CharIKFoot.h`
  - `CharIKFoot` inherits `CharIKHand`. Its constructor creates a private
    helper `RndTransformable`, resets that helper's local transform, initializes
    the FSM state to zero, and leaves `mData` / `mDataIndex` at their source
    defaults.
  - `Enter` resets the FSM state and release distance. `SetName` delegates to
    `Hmx::Object::SetName` and stores the owning `Character` only when the
    supplied directory is a `Character`.
  - `DoFSM` is the foot-specific concrete body: teleports reset the FSM state,
    negative delta time clamps to zero, the helper target copies the finger
    world matrix and finger Z position, `mData->mLocalXfm.v[mDataIndex]`
    selects the planted/release branch, planted travel is clamped to `0.125`,
    and release distance decays by `deltaSeconds * 25.0`.
  - `Poll` is a source delegation wrapper: it requires finger, hand, and data
    refs; clears `mTargets`; pushes the helper target with extent zero; runs
    `DoFSM`; calls inherited `CharIKHand::Poll`; then clears `mTargets` again.
    Native `source_char_ik_foot_*` helpers port that foot-specific plan and FSM
    for tests. They do not add a decoded `CharIKFoot` row hookup or a separate
    live foot IK pass.
  - `Load` accepts source revisions through 6, loads the `CharIKHand`
    superclass, consumes a legacy symbol below revision 6, consumes up to three
    legacy ints below revision 5, and reads `mData` plus `mDataIndex` for
    revision 5 and newer. `Copy` copies the `CharIKHand` superclass, `mData`,
    and `mDataIndex`.
- `rb3-latest/src/system/char/CharIKMidi.cpp` and
  `rb3-latest/src/system/char/CharIKMidi.h`
  - `CharIKMidi::Load` accepts source revisions through 5, reads
    `Hmx::Object`, then `mBone`. Revisions below 3 read a legacy spot vector;
    revisions 2 and 3 read a legacy string; revisions above 4 read
    `mAnimBlender` and `mMaxAnimBlend`.
  - Native GHOGX decodes/logs the same source-gated fields as passive row
    inventory and enforces the source revision range. The viewer/gameplay fret-target helper remains diagnostic application glue until `CharIKMidi::NewSpot` / `Poll` bodies are available from source or trace.
  - Native `source_char_ik_midi_*` helpers record the checked source
    constructor/`Enter` state reset, load gates, `PollDeps`, copy-member list,
    `new_spot` handler row, and prop sync rows. `Enter` clears current/new
    spots, spot-changed state, interpolation fractions, and both local
    transforms; `PollDeps` publishes `mBone` as both changed-by and changed,
    plus `mCurSpot` as changed-by; `Copy` copies `Hmx::Object`, `mBone`,
    `mAnimBlender`, and `mMaxAnimBlend`.
    `engine/out/source_ikmidi_20260711/ikmidi_source_decode_audit.log`
    rechecks Rock1, Rock2, Glam1, Funk1, and Rockabill2; each sampled row is
    `version=4`, `bone=bone_fret.mesh`, `legacySpots=0`,
    `legacyString=<none>`, `animBlender=<none>`, and `unreadBytes=0`.
    `engine/out/source_ikmidi_20260711/stock_ikmidi_controllers.stdout.log`
    refreshes stock proof against the current decoder: all 19 stock
    `CharIKMidi` rows are `version=4`, target `bone_fret.mesh`, and report
    `unreadBytes=0`.
- `rb3-latest/src/system/char/CharIKSliderMidi.cpp` and
  `rb3-latest/src/system/char/CharIKSliderMidi.h`
  - `CharIKSliderMidi` inherits `RndHighlightable`, `CharWeightable`, and
    `CharPollable`. Constructor defaults are source data: null target,
    first-spot, second-spot, and character refs; target percentage `1.0`;
    percentage-changed false; reset-all true; tolerance `0.0`; then it calls
    `Enter`.
  - `Enter` clears `mPercentageChanged`, `mFrac`, and `mFracPerBeat`, then
    delegates to `RndPollable::Enter`. `SetName` delegates to
    `Hmx::Object::SetName` and stores the owning `Character` only when the
    supplied directory is a `Character`.
  - `SetupTransforms` only marks `mResetAll` true. Property sync calls it when
    `target`, `first_spot`, or `second_spot` changes.
  - `PollDeps` appends `target` to `change`, then appends `target`,
    `first_spot`, and `second_spot` to `changedBy`.
  - `Load` accepts source revisions through 2, reads `Hmx::Object`, reads
    `CharWeightable` only for revisions above 1, then reads target, first spot,
    second spot, and tolerance. `Copy` copies `Hmx::Object`,
    `CharWeightable`, target, first spot, second spot, and tolerance.
  - Native `source_char_ik_slider_midi_*` helpers port these concrete source
    behaviors for deterministic coverage. The checked source declares but does
    not include reviewable `Poll` or `SetFraction` bodies, so native does not
    invent slider target solving from these helpers.
- `rb3-latest/src/system/char/CharIKFingers.cpp` and
  `rb3-latest/src/system/char/CharIKFingers.h`
  - Constructor defaults are source data: five fingers, reset-hand flags true,
    curled length `0.85`, keyboard offset `(0.3, -6.0, 0.4)`, hand move
    forward `1.0`, pinky rotation `-0.06`, thumb rotation `0.23`, hand
    destination offset `-0.4`, right-hand default true, move-hand false, and
    setup false.
  - `Load` accepts source revisions through 5 and gates `is_right_hand`,
    `output_trans`, `keyboard_ref_bone`, keyboard offset, thumb/pinky rotation,
    move-forward, and destination-offset fields by revision. Native
    `source_char_ik_fingers_load_plan` and
    `source_char_ik_fingers_copy_plan` record that revision-gated row order and
    the source copy member list.
  - `SetName` resolves hard-coded left/right hand, forearm, upper-arm, finger
    joint, and fingertip transform names. The source then marks setup false only
    if a finger joint/tip is missing; it does not require hand/forearm/upperarm
    in that final completeness loop. Native `source_char_ik_fingers_*` helpers
    port these data decisions and raw source matrices before `Normalize`.
  - Native `source_char_ik_fingers_set_finger_plan` and
    `source_char_ik_fingers_release_finger_plan` port the visible source state
    flips: valid finger range, vector assignment, active/dirty flags,
    `mBlendInFrames=5`, per-finger blend counters, and the `finger01` by
    current-hand transform step marker. `MeasureLengths`, the missing transform
    math inside `SetFinger`, and the real `Poll` solve remain fenced and must
    not be promoted into live fretting-finger behavior from this data slice.
  - `BEGIN_HANDLERS(CharIKFingers)` delegates to `CharWeightable` then
    `Hmx::Object` and checks `0x3AB`. `BEGIN_PROPSYNCS(CharIKFingers)` exposes
    `is_right_hand`, `output_trans`, `keyboard_ref_bone`,
    `hand_keyboard_offset`, `hand_thumb_rotation`, `hand_pinky_rotation`,
    `hand_move_forward`, and `hand_dest_offset`, then delegates to
    `CharWeightable`. Native `source_char_ik_fingers_handler_plan` and
    `source_char_ik_fingers_prop_sync_plan` record those rows without adding
    any new finger solver behavior.
- `rb3-latest/src/system/char/CharDriver.cpp` and `CharDriver.h`
  - The header/source expose the base driver object members and runtime helper
    surface: `mBones`, `mClips`, `mDefaultClip`, `mBlendWidth`, `mClipType`,
    `mApply`, `mPlayMultipleClips`, `Enter`, `Play`, `PlayGroup`, and
    `PollDeps`. `Enter` can play `mDefaultClip`; `Play` builds
    `CharClipDriver` nodes; `PollDeps` depends on `mBones`.
  - This source snapshot does not expose a decompiled base `CharDriver::Load`
    body or base `CharDriver::Poll` body. The RB2 dump also has only an empty
    `CharDriver::Load` body, plus named `PreLoad`/`PostLoad` and runtime
    function ranges. Native GHOGX must not claim a full source-backed base
    driver load/poll port from these files.
  - Native GHOGX therefore treats base `CharDriver` rows as passive controller
    inventory unless a connected source-backed runtime path is present. The
    existing viewer-side clip playback is a diagnostic/application helper; it
    must not be used as proof that source `CharDriver::Poll` has been ported.
  - Native `source_char_driver_starved` ports the concrete source
    `CharDriver::Starved` body: an empty stack is starved, any stack with a
    next clip is not starved, and a single `0x10` no-loop clip is not starved.
    `CharClipPlayer::source_starved` reports that helper from the native layer
    stack, but this does not promote default-clip playback without the missing
    `Poll` body.
  - Native `source_char_driver_resolve_blend_width` ports the concrete
    `CharDriver::Play` sentinel rule: requested blend width `-1.0f` resolves
    to the driver's `mBlendWidth`; other values are left as supplied.
    `CharClipPlayer` now defaults its source driver blend width to the source
    constructor default `1.0f` instead of falling back to the clip row.
  - Native `source_char_driver_should_start_clip` ports the concrete
    `CharDriver::Play` duplicate-clip gate: when `mPlayMultipleClips` is true,
    attempting to play a clip already in the stack returns without starting a
    new node. The source constructor default remains false.
  - Native `source_char_driver_play_decision` ports the visible
    `CharDriver::Play(CharClip*)` branch order as one deterministic state
    decision: a missing clip only emits the source warning path, a present clip
    writes `mLastNode` before resolving the `-1.0f` blend-width sentinel and
    before the duplicate-clip gate, and a non-duplicate creates the new source
    stack head. It does not allocate a live `CharClipDriver` or run the missing
    driver `Poll` path.
  - Native `source_char_driver_play_node_decision` ports the checked
    `CharDriver::Play(const DataNode&)` wrapper: it copies the requested node,
    resolves the clip through `FindClip(node, true)`, delegates to the
    `Play(CharClip*)` branch above, then restores `mLastNode` to the requested
    node even when no clip/driver was created. This keeps default/play-node
    state bookkeeping source-backed without claiming runtime clip evaluation.
  - Native `source_char_driver_first_playing_index` ports the concrete
    `CharDriver::FirstPlaying` stack scan over `mFirst` / `mNext`, returning
    the first node with nonzero `mBlendFrac`. It is intentionally a source-stack
    helper only until the missing `CharClipDriver::Evaluate` body supplies
    source `mBlendFrac` values.
  - Native `SourceCharDriverState` records the checked `CharDriver`
    constructor defaults: null `mBones`/`mClips`/clip pointers, empty `mFirst`,
    empty `mLastNode`, `mOldBeat=1e+30f`, `mBeatScale=1.0f`,
    `mBlendWidth=1.0f`, empty `mClipType`, `mApply=kApplyBlend`, no
    `mInternalBones`, and `mPlayMultipleClips=false`.
  - Native `source_char_driver_clear`, `source_char_driver_transfer`,
    `source_char_driver_set_clips`, and `source_char_driver_set_bones` port the
    concrete source state edits from `Clear`, `Transfer`, `SetClips`, and
    `SetBones`. `SetClips` only resets `mLastNode` when the clip directory
    changes.
    `source_char_driver_transfer_plan` records the exact `Transfer` boundary:
    source copies `mClips`, `mLastNode`, `mRealign`, `mBeatScale`,
    `mBlendWidth`, and conditionally clones `mFirst`; it does not copy bones,
    test/default clips, default-play/starved state, `mOldBeat`, clip type,
    apply mode, internal bones, play-multiple state, or `unk89`.
  - Native `source_char_driver_set_apply`, `source_char_driver_set_clip_type`,
    and `source_char_driver_sync_internal_bones` port the concrete
    `SyncInternalBones` gate: every changed apply/clip-type value clears the
    stack and resets `mLastNode`; internal bones are deleted when the clip type
    is null, allocated only for `kApplyBlendWeights` plus non-null clip type,
    then cleared and stuffed from `CharBoneDir::StuffBones`.
  - Native `source_char_driver_enter`, `source_char_driver_set_starved`,
    `source_char_driver_set_blend_width`,
    `source_char_driver_play_group_decision`, and
    `source_char_driver_poll_deps` port the remaining concrete source helper
    decisions available in `CharDriver.cpp`/`.h`: `Enter` clears the stack,
    resets `mLastNode`, `mOldBeat`, and `mBeatScale`, then requests default-clip
    playback with `(1, -1.0f, 1e+30f, 0.0f)` when `mDefaultClip` exists;
    `SetStarved` stores the handler symbol; `SetBlendWidth` stores `mBlendWidth`;
    `PlayGroup` warns and returns when `mClips` is missing, warns and returns
    when the named group is missing, and otherwise calls `CharClipGroup::GetClip`
    before forwarding that clip to `Play`; `PollDeps` publishes `mBones` in the
    change list.
  - Native `source_char_driver_runtime_dump_evidence` records the RB2 dump
    ranges and locals for driver runtime functions whose statement bodies are
    still unavailable in the checked sources: `PlayIfSafe`
    `0x8034D8A4 -> 0x8034DB54`, `SetBeatScale`
    `0x8034DBB4 -> 0x8034DC4C`, `EvaluateFlags`
    `0x8034DC4C -> 0x8034DD64`, `Last`
    `0x8034DD64 -> 0x8034DD88`, `Before`
    `0x8034DD88 -> 0x8034DDAC`, `MostPlaying`
    `0x8034DDD4 -> 0x8034DF00`, `PreLoad`
    `0x8034E0E0 -> 0x8034ED68`, and `PostLoad`
    `0x8034ED68 -> 0x8034F008`. The visible local inventory includes
    `EvaluateFlags` locals `weight`, `flagWeight`, `cd`, and `w`.
    The same helper keeps `safe_to_evaluate_flags=false` and
    `safe_to_import_poll=false` because the RB2 dump maps ranges and locals, not
    a reviewable `EvaluateFlags` or `Poll` implementation.
- `rb3-latest/src/system/char/CharDriverMidi.cpp` and
  `rb3-latest/src/system/char/CharDriverMidi.h`
  - `CharDriverMidi::Load` reads the subclass revision, accepts revisions
    through 7, loads `CharDriver`, then loads `mDefaultClip` for revisions below
    7. Revision 2 carries a legacy string; revisions above 3 read `mParser`,
    above 4 read `mFlagParser`, and above 5 read `mBlendOverridePct`.
    Native `source_char_driver_midi_load_plan` records this exact load order
    and revision gate as a deterministic source contract only.
  - `CharDriverMidi::Enter` attaches the object as a sink to `mParser` and
    `mFlagParser`. `OnMidiParser`, `OnMidiParserFlags`, and
    `OnMidiParserGroup` are the source-backed runtime route for MIDI/note-driven
    clip selection and blend width scaling. `Poll` and `PollDeps` add no
    MIDI-specific behavior and delegate to `CharDriver`.
  - Native `source_char_driver_midi_default_state`,
    `source_char_driver_midi_enter`, `source_char_driver_midi_exit`,
    `source_char_driver_midi_poll_plan`,
    `source_char_driver_midi_poll_deps`,
    `source_char_driver_midi_on_parser_flags`,
    `source_char_driver_midi_on_parser`, and
    `source_char_driver_midi_on_parser_group` port the concrete source
    decisions without activating the runtime scheduler: constructor defaults,
    parser sink add/remove decisions, clip-flag updates, default-clip selection
    when `!unk89 && mDefaultClip`, normal and real-time blend-width conversion,
    `OnMidiParserGroup` use of `grp->GetClip(mClipFlags)` when the default
    clip branch is not active, and group-message assignment of the returned
    node's `mBlendWidth`.
  - Native `source_char_driver_midi_copy_plan` records the checked source copy
    list: copy `CharDriver`, `unk89`, `mParser`, `mFlagParser`, and
    `mBlendOverridePct`. The source copy body does not name `mClipFlags`, so
    native records that absence as copy-plan evidence only.
  - Native `source_char_driver_midi_handler_plan` records the checked message
    rows `midi_parser`, `midi_parser_group`, and `midi_parser_flags` before the
    `CharDriver` superclass. Native `source_char_driver_midi_prop_sync_plan`
    records the checked property rows `parser`, `flag_parser`, and
    `blend_override_pct` before the `CharDriver` superclass. These are row
    contracts, not active message-sink or property-editor wiring.
  - `rb3-latest/src/system/obj/ObjPtr_p.h` proves `mDefaultClip.Load` reads one
    `0x80`-bounded source string. Native GHOGX therefore decodes/logs that slot
    as `midiDefaultClip` for revision-below-7 rows before applying the
    parser/flag/blend gates, and enforces the source MIDI-driver revision range.
  - `engine/out/source_chardrivermidi_20260711/stock_chardrivermidi_controllers.stdout.log`
    refreshes stock proof against the current decoder: all 38 stock
    `CharDriverMidi` rows are `midiVersion=3`, have no default clip, and report
    `midiUnreadBytes=0`.
  - Native GHOGX still does not run `Enter`, attach parser message sinks, choose
    clips from MIDI parser messages, or play `mDefaultClip`. The row is passive
    inventory until the connected clip driver and CharBones runtime are ported.
  - Native GHOGX decodes/logs the inherited `CharWeightable` revision, weight,
    and owner rows for drivers. It keeps `weight_prop` as a compatibility alias
    but source-facing logs name the row `weightOwner`.
- `rb3/src/system/char/CharIKHand.cpp`
  - `CharIKHand::Poll` is the available Harmonix source for hand IK runtime
    motion. It resolves `mHand`, optional `mFinger`, and the target list,
    blends the world destination into `mWorldDst`, calls `IKElbow` when an
    elbow chain is present, and writes the hand through `SetWorldXfm`.
  - `CharIKHand::Load` gates the serialized fields by revision: `mHand`,
    optional `mFinger`, legacy single target or target list, `orientation`,
    `stretch`, `scalable`, `move_elbow`, `elbow_swing`, `always_ik_elbow`,
    `constrain_wrist`, `wrist_radians`, optional revision-9 padding, then
    revision-12 `elbow_collide` and `clockwise`. Native GHOGX decodes and logs
    those source-declared fields, enforces the source revision range, and
    records the row tail byte count so the active asset data can prove which
    branches are actually present.
  - Native `source_char_ik_hand_load_plan`,
    `source_char_ik_hand_copy_plan`, `source_char_ik_hand_handler_plan`, and
    `source_char_ik_hand_prop_sync_plan` now record the visible source rows:
    load revisions through `0xC`, legacy target expansion with extent `0`,
    revision-9 padding, the final `SetHand(mHand)` call, copy superclasses
    `Hmx::Object` then `CharWeightable`, source copy member order including the
    duplicated `mTargets` row and the visible omission of `mFinger`, handler
    row `measure_lengths`, custom `IKTarget` properties `target` / `extent`,
    and the `CharWeightable` prop-sync superclass. These helpers do not add any
    new live IK solve or promote the fenced branches below.
  - Native GHOGX must not retain the older opt-in free two-bone arm solver or
    its `GHOGX_ENABLE_ARM_IK`/stretch/rotation gates. Any hand or elbow solve
    must be translated from the source-backed `CharIKHand` dataflow above.
  - The native CharIKHand pass now runs from the decoded controller order and is
    not wrapped in the older hand-IK A/B switches, name-based fret/strum
    reordering, or the hand-local `.pos` escape hatch. Hand `.pos` rows stay out
    of local FK for real hand bones; the live hand reaches its target through
    the CharIKHand world-row write after clip sampling.
  - Native `source_char_ik_hand_measure_lengths` and
    `source_char_ik_hand_elbow_cosine` port the source `MeasureLengths` fields
    used by `IKElbow`: `1 / (2 * handLen * parentLen)`, `parentLen^2 +
    handLen^2`, `handLen + parentLen`, and the source `ClampEq(-1, 1)` cosine
    step. The runtime single-target slice now uses this named source scalar
    instead of an inline local elbow clamp. Native
    `source_char_ik_hand_update_measure_lengths` mirrors `SetHand` /
    `UpdateHand`: the hand length cache starts dirty, non-scalable hands measure
    once, and scalable hands remeasure each poll. Runtime hand IK now keeps that
    per-controller cache and feeds the cosine helper from the cached source
    fields without pre-clamping target distance.
  - Native `source_char_ik_hand_multi_target_blend` ports the concrete
    multi-target weighting branch from `CharIKHand::Poll`: present targets get
    `144 / max(0.001, LengthSquared(worldPos))`, positive extents either use
    the source `0.001` floor when `extent < -worldPos.z` or zero `z` before the
    length calculation, low total weight scales the row weight down, and the
    final destination position blends the original target world positions by
    normalized source weights. The same helper now ports the source
    multi-target orientation path for deterministic data: when orientation is
    requested, caller-supplied target quaternions are weighted by the same
    normalized target weights, summed, and normalized once at the end, matching
    the `ScaleAddEq(quat, q268, weight / sumfloat)` and final `Normalize`
    branch without adding a hemisphere correction.
  - Native `source_char_ik_hand_finger_target` ports the visible `mFinger`
    branch from `CharIKHand::Poll` as a deterministic transform helper: build a
    target transform from the blended destination vector/quaternion, invert the
    finger world transform, multiply `handWorld * inverse(fingerWorld)`, then
    multiply that by the target transform and use the resulting position and
    rotation. This does not publish live hand transforms.
  - `CharIKHand::PullShoulder` is source-real but not yet source-importable:
    `CharIKHand.cpp` calls it from `IKElbow`, and
    `ihatecompvir-extra/band3_recomp/band3_config.toml` exposes a
    `CharIKHand__PullShoulder` symbol, but the available ihatecompvir C++ only
    declares/calls the method and does not include its body. Native GHOGX
    therefore must not rederive that shoulder offset or claim a full IKElbow
    port until the function body is source-backed.
  - The current runtime solver is the bounded GH2 single-target slice. Source
    branches for live multi-target publishing, `PullShoulder`, `mElbowSwing`,
    wrist constraint, and elbow-collision correction remain fenced unless an
    asset log proves they are present and the matching ihatecompvir source
    branch is ported.
- `rb3-latest/src/system/char/CharIKRod.cpp` and
  `rb3-latest/src/system/char/CharIKRod.h`
  - `CharIKRod::Load` reads revision 2 rows as `left_end`, `right_end`,
    `dest_pos`, `side_axis`, `vertical`, `dest`, then the stored source
    transform `mXfm`.
  - `CharIKRod::Poll` first calls `ComputeRod`. The source `ComputeRod` returns
    false unless `dest`, `left_end`, and `right_end` all resolve, so native code
    must not fabricate a destination transform for an incomplete stock row.
  - `ComputeRod` interpolates the left/right endpoint world positions into the
    destination position, uses either a fixed vertical X row or an interpolated
    endpoint X row, takes the optional side-axis Z row or the left-right vector,
    rebuilds an orthonormal matrix, then `Poll` multiplies that result by
    `mXfm` before writing `dest`.
  - Native `source_char_ik_rod_default_state`,
    `source_char_ik_rod_load_plan`, `source_char_ik_rod_copy_plan`,
    `source_char_ik_rod_handler_plan`,
    `source_char_ik_rod_prop_sync_plan`, and
    `source_char_ik_rod_poll_deps` port the visible constructor defaults,
    revision-gated row order, copy member order, `Hmx::Object` handler chain,
    check value `0xAF`, `SyncBones` property-modify rows, and dependency
    publication order.
  - Native `source_char_ik_rod_compute_world` ports that `ComputeRod` / `Poll`
    path and publishes the resulting `mXfm * computedRod` world row only when
    the source-required `dest`, `left_end`, and `right_end` transforms resolve.
    Stock Grim rows with `dest=<none>` therefore remain logged/inert instead of
    receiving a substitute destination.

## Twist Controller Authorities

- `rb3/src/system/char/CharUpperTwist.cpp`
  - `CharUpperTwist::Load` reads three object pointers and its property sync
    maps them as `upper_arm`, `twist1`, and `twist2`. ihatecompvir's member
    names are intentionally odd: `upper_arm` syncs to the member used as the
    source transform in `Poll`, while `twist1` and `twist2` are the driven
    output transforms.
  - `CharUpperTwist::Poll` reads the source transform's parent world row and
    current world row, builds a rotation from parent X to source X, rotates the
    parent Y row through that quaternion, interpolates toward the source Y row
    at `0.333` and `0.666`, runs `LookAt` on the matrix rows, and writes the two
    driven transforms through `SetWorldXfm`.
  - Native `source_char_upper_twist_poll_world` ports that world-row `Poll`
    behavior directly: it returns the two source `SetWorldXfm` matrices with
    the source transform's X row, the previous driven positions, and the exact
    `0.333f` / `0.666f` interpolation constants. Runtime code only resolves
    decoded object names and converts those source world rows back to local
    rows.
- `rb3/src/system/char/CharForeTwist.cpp`
  - `CharForeTwist::Load` reads `offset`, `hand`, `twist2`, an old revision-2
    dummy int, and `bias` for revisions above 3.
  - `CharForeTwist::Poll` derives the twist angle from the hand world Z row and
    the hand parent world X/Y rows, applies authored `offset` and `bias`, writes
    the `twist2` parent transform, then interpolates toward the hand position
    using `twist2.local.x / hand.local.x` and writes `twist2` itself.
  - Native `source_char_fore_twist_poll_world` ports that world-row `Poll`
    behavior directly: it computes the source angle, applies one third of the
    final angle to the `twist2` parent, applies the same rotation again to
    `twist2`, and keeps the source position ratio
    `twist2.local.x / hand.local.x` instead of inserting a native fallback.
  - `CharIKHand::Poll` does not inline or consume `CharForeTwist` rows. Native
    GHOGX therefore runs decoded `CharForeTwist` controllers as their own source
    poll pass after hand IK instead of marking them handled inside the hand IK
    bridge.
  - Native keeps `CharUpperTwist` after `CharHair`, matching the accepted
    Rock1/Rock2 PS2 cadence documented in `CHARACTER_FORMAT_NOTES.md`.
    Moving `CharUpperTwist` before hair was rejected because it contradicted
    that trace-backed order and regressed the Rock1/Rock2 posture review.
    Corrected proof captures and logs are under
    `engine/out/rock_regression_corrected_20260710/`.
  - Native GHOGX does not keep approximate or PS2-row twist writers in the
    runtime path. The standalone twist controller path follows these
    ihatecompvir source `Poll` routines, and
    `ghogx_character_fore_upper_twist_source_test` now covers the focused helper
    math.
- `rb3-latest/src/system/char/CharNeckTwist.cpp` and
  `rb3-latest/src/system/char/CharNeckTwist.h`
  - `CharNeckTwist::Load` accepts source revisions through 1, loads
    `Hmx::Object`, then reads `head` and `twist` transform references.
  - `CharNeckTwist::Copy`, handlers, and prop-sync rows are source-visible.
    Copy duplicates `mHead` and `mTwist`, the handler table only delegates to
    `Hmx::Object` with check `0x65`, and prop-sync exposes `head` and `twist`.
  - `CharNeckTwist::PollDeps` publishes `head` as the changed-by row and
    `twist` as the changed row. Native `source_char_neck_twist_poll_deps`
    mirrors that data behavior.
  - `CharNeckTwist::Poll` walks the head local transform chain up to the twist
    parent, derives a rotation from the resulting X/Y rows, and rotates the
    twist row about local X by half of `LimitAng(atan2(z, y))`. Native
    `source_char_neck_twist_poll_plan` ports the source gates, parent-chain
    local-matrix multiplication, the source `MakeRotQuatUnitX` call effect
    using the same row-vector quaternion convention as the imported twist
    helpers, and final dirty-local rotate intent. The upstream snapshots still
    do not expose the standalone `MakeRotQuatUnitX` helper body, so no live
    neck write is promoted.
  - `engine/out/source_necktwist_20260711/stock_necktwist_inventory.log`
    refreshes the current stock GH2 hybrid inventory: every one of the 24 base
    character MILOs reports `neckTwist=0`. This source slice is therefore not
    evidence that Rock1/Rock2 neck posture is fixed or caused by a missing
    `CharNeckTwist` row.

## Clip Runtime Boundary

- `rb3-latest/src/system/char/CharServoBone.cpp` and
  `rb3-latest/src/system/char/CharServoBone.h`
  - `CharServoBone::Load` accepts source revisions through 2. It reads
    `Hmx::Object` fields, then a `clip_type` `Symbol` only when the row
    revision is greater than 1.
  - `SetClipType` clears existing bones and calls `CharBoneDir::StuffBones`.
    `ReallocateInternal` then looks up `bone_facing_delta.pos`,
    `bone_facing.pos`, `bone_pelvis`, `bone_facing.rotz`, and
    `bone_facing_delta.rotz` from the source bone data.
  - `rb3-latest/src/system/char/CharBoneDir.cpp` is the current source
    authority for resource lookup, clip-type bone stuffing, and character bone
    directory load/copy/property rows.
  - `CharBoneDir::ListBones` adds `bone_facing.pos` and
    `bone_facing.rotz` when `mMoveContext` intersects the requested mask, adds
    `bone_facing_delta.pos` and `bone_facing_delta.rotz` when the caller asks
    for delta facing rows, then delegates to every `CharBone::StuffBones`.
    Native `source_char_bone_dir_list_bones` ports that list-building behavior
    for decoded `CharClip::OutputBone` rows without claiming the still-fenced
    `CharBones::AddBones` packed insertion path.
  - `FindResourceFromClipType`, `StuffBones(CharBones&, Symbol)`, and
    `GetClipTypes` are table-driven over `CharClip` type rows. Native
    `source_char_bone_dir_find_resource_from_clip_type`,
    `source_char_bone_dir_stuff_bones_symbol_step`, and
    `source_char_bone_dir_get_clip_types` port the concrete branch flow:
    missing type, missing `(resource ...)` field, missing loaded resource, and
    successful resource/context handoff. These helpers do not perform runtime
    MILO loading; they model the source table/resource decisions after the rows
    are known.
  - `CharBoneDir::Init` creates the shared `char_resources` directory, reads
    `objects/CharBoneDir resource_path`, reads `objects/CharClip types`, then
    scans clip-type rows starting at source index one. For every row with a
    `(resource ...)` array it skips resources already present under
    `char_resources`; otherwise it builds `<resource_path>/<resource>.milo`,
    loads it on the `char` heap, and names successful loads under
    `char_resources`. The checked source leaves
    `DataRegisterFunc("get_clip_types", GetClipTypes)` commented out. Native
    `source_char_bone_dir_init_plan` ports this startup preload as a
    deterministic plan only; it does not perform live resource loading or
    replace the runtime `ObjectDir` ownership path.
  - `CharBoneDir::Terminate` only deletes the shared `sResources` directory; the
    checked source does not clear that static pointer afterward. Native
    `source_char_bone_dir_terminate_plan` records those exact lifecycle
    semantics without changing app shutdown.
  - `CharBoneDir::FindResource` delegates directly to
    `sResources->Find<CharBoneDir>(name, false)`. Native
    `source_char_bone_dir_find_resource` ports that exact-name lookup over a
    supplied resource list only; warnings and clip-type resource handling stay
    in the `FindResourceFromClipType` helper.
  - `CharBoneDir::GetContextFlags` lazily rebuilds cached context symbols only
    while `mContextFlags` is still the constructor integer node. The source
    allocates a temporary result array at `cfg->Size() - 1`, then scans source
    rows starting at index `1` while `i < arr->Size()`. That skips the source
    row-zero entry and the final source row. Matching `(resource ...)` rows for
    the directory name de-duplicate `resource[2]`, sort the symbols, and return
    the cached result on later calls. Native
    `source_char_bone_dir_get_context_flags_step` preserves that table scan,
    including the source loop's `cfg->Size() - 1` allocation boundary, as a
    deterministic helper only.
  - `CharBoneDir::SyncFilter` clears `mFilterBones` and republishes every
    `CharBone` whose position, scale, or non-`TYPE_END` rotation context
    intersects `mFilterContext`. Native `source_char_bone_dir_sync_filter`
    ports that selection rule for decoded output-bone rows without installing a
    live editor filter list.
  - `CharBoneDir::MergeCharacter` has a concrete checked prefix and then an
    omitted body. The prefix loads a character MILO, warns and returns when the
    load fails, scans transformables from the loaded directory while skipping
    the directory object itself, requires `CharUtlIsAnimatable`, and collects
    only names beginning with `bone_` or `exo_`. Native
    `source_char_bone_dir_merge_character_plan` ports only that source-visible
    selection prefix; the actual merge body remains fenced.
  - `CharBoneDir` construction, load, and copy are source-visible in the same
    file. Construction starts `mMoveContext` at `0`, `mBakeOutFacing` at `1`,
    `mContextFlags` as integer `0`, `mFilterContext` at `0`, no-null recenter
    target/average lists, and a no-null `mFilterBones` list. `PreLoad` accepts
    source revisions `0..4`, pushes packed alt/HMX revisions, and delegates to
    `ObjectDir::PreLoad`; `Load` delegates to `ObjectDir::Load`; `PostLoad`
    restores revisions, reads a legacy bool before revision 2 or `mMoveContext`
    otherwise, reads another legacy bool before revision 3, always reads
    `mRecenter`, and reads `mBakeOutFacing` only after revision 3. `Copy` copies
    `ObjectDir`, `mMoveContext`, `mRecenter`, and `mBakeOutFacing`. Native
    `source_char_bone_dir_default_state`,
    `source_char_bone_dir_load_plan`, and
    `source_char_bone_dir_copy_plan` port those deterministic source facts only.
    Native `source_char_bone_dir_save_plan` records the checked
    `SAVE_OBJ(CharBoneDir, 0x18C)` row id; it does not imply a native writer.
    Native `source_char_bone_dir_handler_plan`,
    `source_char_bone_dir_recenter_prop_sync_plan`, and
    `source_char_bone_dir_prop_sync_plan` record the checked handler and
    property tables: `get_context_flags`, `ObjectDir` superclass dispatch,
    check value `0x1D1`, recenter rows `targets`/`average`/`slide`, direct
    rows for recenter/move/bake/filter lists, `merge_character` as a set prop,
    and `filter_context` as a modify prop that calls `SyncFilter`.
  - Native GHOGX decodes and logs the source `CharServoBone` row and
    `clip_type`, enforces the source revision range, and records the row tail
    byte count. Native exposes bounded source helpers for `ZeroDeltas`,
    `MoveToFacing`, and `MoveToDeltaFacing`, but does not call them from the
    live model path or port broad `CharBonesMeshes` movement until the
    connected clip/bone source path is implemented as a whole.
  - Native GHOGX also records the checked `CharServoBone` constructor,
    `SetClipType`, `Enter`, `SetMoveSelf`, and `Copy` decision flow:
    constructor pointer members and dirty flags start empty/false; changed
    clip type clears/refills bones through `CharBoneDir::StuffBones`; `Enter`
    zeroes deltas, clears `regulate`, clears `delta_changed`, and mirrors the
    presence of `bone_facing_delta.pos` into `move_self`; `SetMoveSelf` only
    marks `delta_changed` when the requested value differs; `Copy` copies
    `Hmx::Object`, `mMoveSelf`, and calls `SetClipType`.
  - Native `source_char_servo_bone_load_plan`,
    `source_char_servo_bone_save_plan`,
    `source_char_servo_bone_handler_plan`, and
    `source_char_servo_bone_prop_sync_plan` port the visible source
    revision-gated load row order, save row id
    `SAVE_OBJ(CharServoBone, 0x14A)`, `SetClipType` call, `CharPollable` /
    `Hmx::Object` handler chain, check value `0x16E`, `clip_type` /
    `move_self` property setters, `delta_changed` / `regulate` direct rows,
    and `CharBonesMeshes` superclass.
  - Native `source_char_servo_bone_runtime_dump_evidence` records the RB2 dump
    ranges for runtime bodies whose full statements are not in the checked
    latest source: `Poll` `0x8038F4A0 -> 0x8038F820`,
    `RegulateOverride` `0x8038FD74 -> 0x8038FF30`, `Regulate`
    `0x8038FF30 -> 0x803901BC`, and `PollDeps`
    `0x803901BC -> 0x803901C8`. The visible local inventory includes `Poll`
    locals `world`, two `worldPelv` rows, and two `invPelv` rows;
    `RegulateOverride` locals `names`/`pred`; and `Regulate` locals
    `before`, `w`, `pred`, `rawDf`, `df`, `dt`, `pos`, `delta`, and `dang`.
    This evidence does not make live servo motion source-backed:
    `safe_to_run_poll=false`, `safe_to_run_regulate=false`, and
    `safe_to_publish_servo_motion=false`.
  - `rb3-latest/src/system/char/CharBonesMeshes.cpp` is concrete for mesh-slot
    ownership and target resolution. Native
    `source_char_bones_meshes_replace_step`,
    `source_char_bones_meshes_reallocate_step`, and
    `source_char_bones_meshes_stuff_meshes` port the source-visible behavior:
    `Replace` scans only when `from != mDummyMesh`, replaces the first matching
    mesh with `to` when it is transformable or `mDummyMesh` otherwise;
    `ReallocateInternal` calls `CharBonesAlloc::ReallocateInternal`, resizes
    `mMeshes` to `mBones.size()`, resolves each row with `CharUtlFindBoneTrans`,
    substitutes `mDummyMesh` for misses while suppressing missing logs for
    `bone_facing*`, and calls `AcquirePose` only when the mesh vector is
    non-empty; `StuffMeshes` appends mesh slots in source order. This still does
    not port broad `PoseMeshes` transform writes.
  - Native `source_char_bones_meshes_pose_dump_evidence` records the checked
    `PoseMeshes` boundary: latest source exposes only an incomplete body with
    uninitialized `angle` rotations, while the RB2 dump maps `PoseMeshes`
    `0x80321520 -> 0x80321A64` and property sync
    `0x80321B48 -> 0x80321C20`. The RB2 local inventory names `bone`, `pend`,
    `p`, `qend`, `q`, `a`, `xend`, `yend`, `end`, `send`, `s`, and
    `blendScale`, but does not expose statements. Native therefore keeps
    `safe_to_pose_meshes=false` and
    `safe_to_publish_mesh_transforms=false`.
- `rb3-latest/src/system/char/CharClipSet.cpp` and
  `rb3-latest/src/system/char/CharClipSet.h`
  - `CharClipSet` is an `ObjectDir`, `RndDrawable`, and `RndAnimatable`
    preview/editor container for clip rows. Its constructor calls
    `ResetPreviewState` and sets `mRate = k1_fpb`.
  - `ResetPreviewState` deletes the preview character, clears preview/still
    clips, clears the character file root, resets filter flags to zero, sets
    BPM to `90`, and clears preview-walk.
  - `RandomizeGroups` and `SortGroups` iterate each `CharClipGroup` in source
    directory order and call the matching group method.
  - `PreSave` clears the preview character name when one exists. Cached saves
    call both `ResetPreviewState` and `ResetEditorState`; `PostSave` delegates
    to `ObjectDir::PostSave`, then restores `preview_character`, enters it, and
    sends `update_objects` to `milo` when the object exists.
  - Native `source_char_clip_set_load_plan` records the source `Load` body as
    an `ObjectDir::Load` delegation. Native `source_char_clip_set_handler_plan`
    records the visible handler rows: `randomize_groups`, `sort_groups`,
    `recenter_all`, `load_character`, `list_clips`, superclass `ObjectDir`,
    and check `0x2F0`.
  - Native `source_char_clip_set_post_load_plan` ports the full
    revision-gated `PostLoad` read plan through source revision `0x18`,
    including the proxy early return, legacy int/string/list reads,
    filter-clips handler gate, transition-graph warning gate, and modern
    `mCharFilePath` / `mPreviewClip` / filter / BPM / walk / still-clip gates.
  - `LoadCharacter`, `DrawShowing`, `StartFrame`, `EndFrame`, `SetBpm`, and
    `RecenterAll` are ported as deterministic decision helpers. They do not
    execute editor-only loading or promote clip playback runtime.
- `rb3-latest/src/system/char/CharClipGroup.cpp` and
  `rb3-latest/src/system/char/CharClipGroup.h`
  - `CharClipGroup::Load` reads the object prefix through
    `Hmx::Object::Load`, then the stored `mClips` vector, `mWhich`, and
    `mFlags` only for source revisions above 1.
  - Native `source_char_clip_group_load_plan` records the source load gate:
    revisions 0-2 are accepted, `LOAD_REVS`, `Hmx::Object`, `mClips`, and
    `mWhich` are read in that order, and `mFlags` is read only for revisions
    above 1; earlier revisions default `mFlags` to zero.
  - Native `source_char_clip_group_handler_plan` records the exact source
    handler rows: `get_clip`, `delete_remaining`, `get_size`, `has_clip`,
    `find_clip`, `add_clip`, `set_clip_flags`, and `randomize_index`, followed
    by superclass `Hmx::Object` and check `0x179`.
  - Native `source_char_clip_group_prop_sync_plan` records the exact source
    prop-sync rows: `clips` and `flags`.
  - `CharClipGroup::GetClip` advances `mWhich`, wraps it against
    `mClips.size()`, and returns the stored clip pointer. Native
    `char_clip_group_get_clip_index` ports that exact cycling step and mutates
    the stored source `which` state.
  - Native `source_char_clip_group_num_flag_duplicates` ports the complete
    `CharClipGroup::NumFlagDuplicates` masked comparison body: compare the
    selected clip's flags with every other clip and count rows whose masked
    flags match.
  - Native `source_char_clip_group_sorted_names` ports the complete
    `CharClipGroup::Sort` ordering rule: clip names are sorted by the source
    `strcmp(i->Name(), j->Name()) < 0` comparator.
  - Native `source_char_clip_group_add_clip` ports the concrete
    `CharClipGroup::AddClip` duplicate gate: append the requested clip only
    when `HasClip` would report it absent, preserving the existing source
    order.
  - Native `source_char_clip_group_remove_clip` ports the visible
    `CharClipGroup::RemoveClip` iterator behavior as written in
    `CharClipGroup.cpp`: a matching row is erased and the returned iterator is
    then advanced by the loop increment, while a non-match advances once inside
    the `else` branch and once again by the loop. Deterministic coverage keeps
    that source skip behavior explicit instead of replacing it with a cleaner
    remove-all helper.
  - Gameplay routes authored `CharClipGroup` resolution through the shared
    character helper so the same source-backed reader feeds both WorldCrowd and
    performer sync group lookup.
  - The older native graph/stance continuity chooser is not source behavior and
    is removed from guitarist0 `normal` group selection. Runtime still does not
    claim the full `CharDriver::PlayGroup` or `CharDriverMidi` scheduling path;
    this slice ports only the concrete `GetClip()` group advance.
- `rb3-latest/src/system/char` exposes source files for `CharClip`,
  `CharClipDriver`, `CharDriver`, `CharBones`, `CharBonesSamples`,
  `CharBonesMeshes`, and related clip runtime classes. The previous local note
  that public ihatecompvir source had no clip/sample layer is obsolete.
- `rb3-latest/src/system/char/CharBone.cpp` and
  `rb3-latest/src/system/char/CharBone.h` are concrete for clip output
  `CharBone` rows:
  - `CharBone::Load` reads `Hmx::Object`, a legacy
    `RndTransformableRemover` block for revisions below 9, then source-gated
    position context, scale context, rotation type, legacy integers, rotation
    context, target, weights, trans pointer, and bake-out flag.
  - Native clip output rows now decode and log those fields instead of treating
    the bytes after the output transform parent as opaque. The embedded
    transform bytes follow the ihatecompvir `MiloEditor` `RndTrans.Read`
    serialization order already used by the shared native transform decoder:
    local/world matrices, legacy child list, constraint, target, preserve-scale,
    and parent.
  - Native `source_char_bone_find_weight_index`,
    `source_char_bone_get_weight`, `source_char_bone_clear_context`, and
    `source_char_bone_stuff_bones` port the complete source helper bodies for
    decoded `CharBone` output rows. Weight lookup returns the first weight row
    whose context intersects the requested mask, or `1.0f` when no row matches.
    `ClearContext` masks position, scale, and rotation contexts. `StuffBones`
    appends `.pos`, `.scale`, and rotation channels in source order when their
    contexts match, using `CharBones::ChannelName` and `GetWeight`.
  - Native `source_char_bone_copy_members` ports only the concrete
    `BEGIN_COPYS(CharBone)` member list: rotation context, scale context,
    position context, rotation type, target, weights, trans pointer, and
    bake-out flag. Native decoder-only fields such as embedded legacy
    transform bytes, source revision markers, and unread byte counts remain
    outside this helper.
  - This is row decode and diagnostic evidence only. It does not promote broad
    `CharBone` pose publishing; the existing output bridge remains bounded until
    the connected `CharClip` / `CharBonesSamples` evaluation path is fully
    source-backed.
- `rb3-latest/src/system/char/CharUtl.cpp` and
  `rb3-latest/src/system/char/CharUtl.h` are concrete for character bone lookup
  utility behavior:
  - `CharUtlFindBone` rewrites the requested object name's final suffix to
    `.cb` and searches for a `CharBone` row with that name.
  - `CharUtlFindBoneTrans` uses the same `.cb` lookup first and returns the
    matching `CharBone::mTrans` when present. Only when that row is absent does
    it search for `.trans`, then `.mesh`, as `RndTransformable` fallbacks.
  - `CharUtlIsAnimatable` rejects skinned meshes, cameras, `CharCollide`,
    `CharCuff`, `RndDir`, and names beginning with `spot_`; plain transforms
    and unskinned meshes remain animatable.
  - Native `source_char_utl_name_with_suffix`,
    `source_char_utl_find_bone`, `source_char_utl_find_bone_trans`, and
    `source_char_utl_is_animatable` port those deterministic utility rules for
    decoded object inventories. This is a source lookup contract only; it does
    not invent new pose writes or renderer-side character fixes.
  - `CharUtlMergeBones` scans source `CharBone` rows and repeatedly resolves
    the same-named destination row through `GrabBone` / `CharUtlFindBone`.
    Targets are assigned only when the destination target is empty and the
    source target resolves in the destination dir; mismatched existing targets
    warn. Position contexts OR in the caller mask. The checked source's
    scale-context branch calls `SetPositionContext(bone->ScaleContext() | i)`;
    native preserves that exact source behavior in
    `source_char_utl_merge_bones` as a documented contract rather than
    "correcting" it. Rotation contexts OR in the caller mask only when the
    destination rotation type is unset or matches the source; mismatches warn.
  - `CharUtlBoneSaver` snapshots local transforms for transformables whose
    names begin with `bone_` and restores them in the same filtered iteration.
    `CharUtlResetTransform` resets only top-level transformables with no
    parent, and `CharUtlResetHair` calls `Enter()` for every `CharHair` row
    under the character. Native deterministic helpers cover those selection
    rules without changing runtime pose or render behavior.
  - `CharUtlInit` registers only two data functions in the checked source:
    `reset_hair` and `char_merge_bones`. `reset_hair` reads argument one as a
    `Character` and calls `CharUtlResetHair`; `char_merge_bones` reads a
    `FilePath` from argument one, loads that directory, reads the destination
    `ObjectDir` from argument two and context mask from argument three, calls
    `CharUtlMergeBones`, then deletes the loaded directory. Native
    `source_char_utl_init_plan` records those registration and handler call
    shapes only; it does not run a loader or mutate character resources.
  - `ClipPredict` binds `bone_facing.rotz` and `bone_facing.pos`. Its
    `Predict` body evaluates the two requested frames, rotates the second-minus-
    first position delta by `LimitAng(mAng - firstAngle)`, adds that to `mPos`,
    records the second frame as `mLastPos` / `mLastAng`, then advances `mAng`
    by `LimitAng(secondAngle - firstAngle)` and wraps it again. Native
    `source_char_utl_clip_predict` ports that math against explicit sampled
    frame inputs; it does not claim the still-fenced `CharClip::EvaluateChannel`
    body.
- `rb3-latest/src/system/char/CharBones.cpp` is concrete for channel identity
  and byte layout:
  - `CharBones::TypeOf` maps suffixes `.pos`, `.scale`, `.quat`, `.rotx`,
    `.roty`, and `.rotz`. Source scans every dot in the symbol until one of
    those suffixes is found; native now matches that instead of only checking
    the first dot.
  - `SuffixOf` and `ChannelName` round-trip the source channel suffixes.
  - Native `source_char_bones_type_of`, `source_char_bones_suffix_of`,
    `source_char_bones_channel_name`, and `source_char_bones_type_size` now
    port those source helpers directly. `ghogx_character_char_bones_source_test`
    covers all six source suffixes, first-dot channel replacement, and every
    source compression-mode byte size.
  - Native channel classification is constrained to those six source types.
    The old `.d?x` / `.d?y` / `.d?z` category extension, file-order sample
    decode switch, and no-pi axis-rotation switch are not present in
    ihatecompvir's source model and are removed from the current decoder.
    Rejected clip-pose reinterpretation switches for relative/transpose/swap/
    invert/world quaternions and channel dropping are also removed; only the
    clip's decoded `relative` flag may request relative quaternion application.
  - `CompressionType` defines five source modes: `kCompressNone`,
    `kCompressRots`, `kCompressVects`, `kCompressQuats`, and `kCompressAll`.
    Native recognizes the full source enum range instead of capping the row at
    mode 3.
  - `TypeSize` defines the per-channel byte sizes for uncompressed vectors,
    compressed vectors, float quats, short quats, byte quats, and rotations.
    `kCompressQuats` and `kCompressAll` use 4-byte `ByteQuat` rows for
    quaternion channels; native refuses those lists for now because the checked
    source snapshot and RB2 dump identify `ByteQuat` storage but do not expose
    the exact `ByteQuat` -> `Quat` conversion body.
  - A focused upstream check of `ihatecompvir/rb3` master found the current
    `src/system/math/Mtx.h` `Hmx::Quat` declaration and `TransformNoScale`
    `ShortQuat` storage, plus old `doc/src-old/rb3/world/shortquat.hpp`
    declaring `ShortQuat::ToQuat`; the accessible tree does not include a
    matching `ByteQuat` type, header, or conversion implementation. Keep
    `ByteQuat` decode fenced unless new ihatecompvir source or original-game
    trace evidence provides that body.
  - `GHOGX_DEBUG_CLIP=1` logs accepted source `CharBonesSamples` list
    compression modes, sample counts, channel counts, source frame byte counts,
    and whether a list would require the fenced `ByteQuat` path.
  - 2026-07-11 stock default-viewer proof at
    `analysis/source_charbones_stock_compression_20260711/stock_clip_compression_summary.txt`
    ran all 24 stock base character MILOs through the native app with
    `GHOGX_DEBUG_CLIP=1` and individual screenshots. All 24 app runs exited 0.
    Twenty-three characters produced accepted `CharBonesSamples` rows; all 192
    accepted rows used `1(kCompressRots)` and `byteQuat=0`. `metal_keyboard`
    rendered and logged its source driver, but the default viewer clip names
    `idle_medium_01` and `stand_medium_01` were not found in its
    `keyboard_main` clip MILO, so that route produced zero accepted sample
    rows and remains an inventory gap rather than proof of another compression
    mode.
  - The focused read-only clip inventory helper
    `ghogx_character_clip_audit` expands exact `.milo_ps2` names or ARK path
    prefixes, lists each `CharClipSamples` row in a MILO, and loads each named
    clip through the same bounded native decoder. The 2026-07-11 run at
    `analysis/source_clip_inventory_20260711/rock_metal_keyboard_clip_inventory.stdout.log`
    audited `char/metal_keyboard/`, `char/rock1/`, and `char/rock2/` prefixes.
    It found 14 MILOs, 165 `CharClipSamples` rows, 165 accepted rows, and zero
    rejected rows. `keyboard_main` contains six accepted keyboard-named clips:
    `keyboard_lose`, `keyboard_win`, `keyboard_active_fast`, `keyboard_idle`,
    `keyboard_band_jump`, and `keyboard_active_medium`. This proves the earlier
    `metal_keyboard` default-viewer miss was clip-route selection, not sample
    layout or compression. The same pass found source chord-named fret clips in
    `rock1_fret` (`finger_chord_*` and `finger_powerchord_*`), which is data
    inventory only; visual chord-shape verification remains separate.
  - The same audit found no Rock2-specific animation MILOs under the
    `char/rock2/` prefix in this stock ARK slice; Rock2's own prefix contributed
    base, horse, and UI character MILOs with no `CharClipSamples`. Do not infer
    a Rock2 decode failure from absent `char/rock2/anims` rows without tracing
    the source driver/clip directory route.
  - A broader follow-up pass at
    `analysis/source_clip_inventory_20260711/stock_24_character_clip_inventory.stdout.log`
    audited the 24 documented stock base character prefixes. The helper visited
    135 MILOs; 68 MILOs contained clips; all 1,903 `CharClipSamples` rows loaded
    through the bounded native decoder; zero rows were rejected and stderr had no
    failure/status rows. This proves the currently found stock PS2 character
    clip rows are broadly readable by the native decoder. It does not prove
    source `CharDriver` selection, clip blending, or final pose publishing.
  - That broad pass also proves several visual variants do not have private
    animation clips under their own prefix in this ARK slice:
    `deathmetal2`, `glam2`, `goth2`, `metal2`, `punk2`, and `rock2` had zero
    local `CharClipSamples` rows. `alterna2` had only fret/strum/viseme rows,
    and `rockabill2` had only a fret row set. Treat these as clip-directory
    routing evidence to resolve through source driver data, not as parser
    failures or permission to fabricate copied animation paths.
  - The matching controller-route audit at
    `analysis/source_clip_inventory_20260711/stock_24_base_controller_driver_routes.stdout.log`
    confirms the shared routes are authored `CharDriver` data. It decoded 63
    driver rows across the same 24 base MILOs: 25 base `midi=0` rows and 38
    left/right hand `midi=1` rows. The zero-local-clip variants route all three
    driver clips to their sibling set: `deathmetal2 -> deathmetal1`,
    `glam2 -> glam1`, `goth2 -> goth1`, `metal2 -> metal1`,
    `punk2 -> punk1`, and `rock2 -> rock1`. `alterna2` routes only
    `main.drv` to `alterna1_main` while keeping local `alterna2_strum` and
    `alterna2_fret`; `rockabill2` routes `main.drv` and `right_hand.drv` to
    `rockabill1_main`/`rockabill1_strum` while keeping local
    `rockabill2_fret` for `left_hand.drv`.
- `rb3-latest/src/system/char/CharBone.cpp` and `CharBone.h` define the
  authored `CharBone` output rows consumed by `CharBoneDir` and `CharClip`
  resource bones:
  - `CharBone::Load` accepts revisions 0-10, reads `Hmx::Object`, legacy
    `RndTransformableRemover` data below revision 9, bool or int context rows
    depending on revision, rotation type, legacy ints, optional target, the
    revision-6 shared context row, weights above revision 7, transform above
    revision 8, and `mBakeOutAsTopLevel` above revision 9. Native
    `source_char_bone_load_plan` records this source row order and the legacy
    default branches (`mScaleContext=0`, rotation bump, rotation clamp,
    default rotation context, and revision-6 shared-context application).
  - Native `source_char_bone_copy_plan` records the source copy contract:
    `Hmx::Object`, then rotation/scale/position contexts, rotation type,
    target, weights, transform, and bake-out flag.
  - Native `source_char_bone_handler_plan` records the checked handler table:
    action `clear_context`, handler `get_context_flags`,
    `Hmx::Object` superclass dispatch, and source check value `0x152`.
    Native `source_char_bone_weight_context_prop_sync_plan`,
    `source_char_bone_prop_sync_plan`, `source_char_bones_bone_prop_sync_plan`,
    and `source_char_bones_object_prop_sync_plan` record the checked
    `WeightContext`, `CharBone`, `CharBones::Bone`, and `CharBonesObject`
    property rows, including `preview_val`'s `gPropBones->StringVal` lookup and
    the `bones` custom branch.
  - Native `source_char_bone_copy_members`, `source_char_bone_find_weight_index`,
    `source_char_bone_get_weight`, `source_char_bone_clear_context`, and
    `source_char_bone_stuff_bones` port the visible source helpers without
    claiming broad pose output publishing.
  - `FindOffset`, `FindPtr`, `RecomputeSizes`, and `SetCompression` establish
    the source packed-row offset model. Native `source_char_bones_recompute_layout`
    now ports the safe data-layout core of `RecomputeSizes`: cumulative
    per-type counts, per-type byte sizes, offsets, and 16-byte aligned total
    size. Native `source_char_bones_set_compression` ports the source
    `SetCompression` guard: only changed compression values update state and
    recompute the packed layout. Native `source_char_bones_empty_state`,
    `source_char_bones_clear`, `source_char_bones_set_weights`, and
    `source_char_bones_list_bones` port the complete source constructor,
    `ClearBones`, `SetWeights`, and `ListBones` state behavior:
    default compression, counts, offsets, and total size are zero; clear drops
    all bone rows and resets layout/compression; setting weights preserves bone
    names and writes the requested weight to every row; listing bones appends
    the stored rows to the caller-owned output collection in source order. This
    helper slice also ports `FindOffset` over native source-state rows: it
    derives the channel type, scans only the corresponding cumulative type
    range, advances by the source packed type size, and returns `-1` for absent
    rows. Native `source_char_bones_find_ptr` preserves the concrete `FindPtr`
    decision shape as a found offset: source would return `0` for `-1`, or
    `&mStart[offset]` for a hit. It still does not expose a live native pointer
    into sample storage until source sample evaluation bodies are ported.
    Native `source_char_bones_zero` ports the concrete `Zero` byte span:
    `memset(mStart, 0, mTotalSize)`.
    Native `source_char_bones_add_bones_steps` ports the visible `AddBones`
    wrapper flow: call `AddBoneInternal` once for each input row, then call
    `ReallocateInternal` even when the row list is empty. This remains a
    call-flow helper because the checked source declares but does not define
    `AddBoneInternal`.
    Native `source_char_bones_alloc_reallocate_step` ports the concrete
    `CharBonesAlloc::ReallocateInternal` allocation shape: free the old
    `mStart`, allocate `mTotalSize` bytes, and assign the result back to
    `mStart`.
  - `ScaleAdd(CharClip*, ...)` delegates back to `CharClip::ScaleAdd`; it is a
    call-flow hook, not a standalone pose evaluator. Native
    `source_char_bones_scale_add_clip_step` records that delegation and preserves
    the same three float arguments.
  - `rb3-latest` declares `ScaleAdd(CharBones&, float)`, `RotateBy`,
    `RotateTo`, `Blend`, `ScaleDown`, and `ScaleAddIdentity`, but does not
    provide reviewable bodies for those pose writers. The RB2 dump maps
    `ScaleDown`, `ScaleAdd`, `RotateBy`, `RotateTo`, and `ScaleAddIdentity`
    with local inventories, but not copyable statements. Native
    `source_char_bones_pose_body_boundary` records this boundary: packed-row
    layout helpers remain source-backed, but applying pose math to live
    transforms remains fenced until a real body or direct trace is available.
    Native `source_char_bones_runtime_dump_evidence` records the exact RB2
    ranges and visible locals for those mapped pose writers and explicitly
    records that the checked RB2 dump does not map `CharBones::Blend`. That
    helper is evidence for future trace/source import targets only; it is not
    permission to apply pose math from local-name inventories.
- `rb3-latest/src/system/char/CharBonesBlender.cpp` is concrete for the
  animation-bone blender's control flow:
  - Native `source_char_bones_enter_step` ports the inline `CharBones::Enter`
    sequence: `Zero()`, then `SetWeights(0)`.
  - Native `source_char_bones_blender_poll_step` ports `Poll`: return early when
    there are no local bones or no destination; otherwise call `Blend(*mDest)`
    and then `CharBones::Enter`.
  - Native `source_char_bones_blender_set_dest_step` ports `SetDest`: only a
    changed destination writes `mDest`, and only a non-null destination receives
    `AddBones(mBones)`.
  - Native `source_char_bones_blender_set_clip_type_step` ports `SetClipType`:
    changed clip types assign the symbol, clear local bones, then repopulate via
    `CharBoneDir::StuffBones(*this, mClipType)`.
  - Native `source_char_bones_blender_reallocate_step` ports
    `ReallocateInternal`: call `CharBonesAlloc::ReallocateInternal`, add bones
    to the destination when present, then call `CharBones::Enter`.
  - Native `source_char_bones_blender_load_plan` ports `Load`: accepted source
    revisions are `0..2`, the row order is `Hmx::Object`, destination
    `boneObjPtr`, and revision-gated `mClipType`, then the source calls
    `SetClipType` before `SetDest`.
  - Native `source_char_bones_blender_save_plan` records the checked
    `SAVE_OBJ(CharBonesBlender, 0x58)` row id; it does not imply a native
    writer.
  - Native `source_char_bones_blender_copy_plan`,
    `source_char_bones_blender_handler_plan`, and
    `source_char_bones_blender_prop_sync_plan` port the visible source
    copy-member setter order, `CharPollable` / `Hmx::Object` handler chain,
    check value `0x81`, and `dest` / `clip_type` property setters above the
    `CharBonesObject` superclass.
  - This slice is still call-flow only; it does not claim the missing low-level
    `CharBones::Blend` math.
- `rb3-latest/src/system/char/CharBonesSamples.cpp` is concrete for sample
  ownership and interpolation wrappers:
  - `Set`/`Clone` allocate `mRawData` from `AllocateSize()`.
  - `RotateBy`, `RotateTo`, and `ScaleAddSample` select
    `mRawData[mTotalSize * sample]` and split weight between sample `i` and
    `i + 1` by `frac`.
  - `Load` reads `gVer`, asserts the public source range `13..16`, then
    delegates to `LoadHeader` and `LoadData`. Native
    `source_char_bones_samples_load_version_known` ports that exact range and
    the clip parser rejects out-of-range `CharBonesSamples` entries before
    scanning sample-list headers.
  - Native `source_char_bones_samples_load_plan` records the checked `Load`
    delegation: valid serialized versions read `gVer`, then call
    `LoadHeader`, then call `LoadData`; invalid versions expose no read plan.
  - Native `source_char_bones_samples_prop_sync_plan` records the checked
    prop-sync rows from `BEGIN_PROPSYNCS(CharBonesSamples)`: direct rows
    `num_samples` and `frames`, set rows `preview_sample` and `compression`,
    and the custom `bones` branch through `PropSync(mBones, ...)`.
  - `rb3-latest` declares `LoadHeader`, `LoadData`, `EvaluateChannel`, and
    `Relativize`, but does not provide reviewable statement bodies for them.
    The RB2 dump maps the same names and ranges, but only as a function/local
    inventory. Native `source_char_bones_samples_body_boundary` records this
    source boundary: decoding/logging rows is allowed, but broad pose publishing
    and channel evaluation remain fenced until a real body or direct trace is
    available.
  - Native `source_char_bones_samples_runtime_dump_evidence` records the exact
    RB2 ranges and visible locals for the missing low-level sample bodies:
    `FracToSample`, `EvaluateChannel`, `RotateBy`, `RotateTo`,
    `ScaleAddSample`, `Relativize`, `Load`, `ReadCounts`, `LoadHeader`,
    `LoadData`, and `SyncProperty`. The helper keeps `LoadHeader`, `LoadData`,
    `EvaluateChannel`, and `Relativize` marked as lacking statement-level
    bodies, so it is evidence for decoding/logging and future tracing targets,
    not permission to publish broad pose output.
  - `SetVer` is the separate legacy source gate and asserts `ver < 13`.
    Native `source_char_bones_samples_set_ver_known` records that older
    pre-load boundary separately from the serialized `Load` range.
  - Grim's `CharBonesSamples::get_type_of` uses the first dot in a sample
    channel name and matches exact suffix strings only. That differs from the
    C++ `CharBones::TypeOf` helper, which scans later dots. Native
    `source_grim_char_bones_samples_get_type_of` preserves Grim's first-dot
    rule and the `CharClipSamples` parser now uses it for serialized sample
    rows, while keeping `source_char_bones_type_of` for C++ `CharBones`
    layout/utility contracts.
  - Grim and re-notes decode compressed scalar rotation samples and short
    quaternion components as `i16 / 32767.0` clamped to `-1.0`; uncompressed
    scalar rotations stay as the stored float. Native
    `source_grim_char_bones_samples_decode_snorm16`,
    `source_grim_char_bones_samples_decode_short_quat`, the `CharClipSamples`
    scalar channel reader, and the short quaternion reader preserve that raw
    source value; no pi-scale is applied unless a future source body proves one
    belongs in a later pose-application step.
  - Grim's `load_char_bones_samples_data` computes each packed sample row by
    walking the serialized bone channel list, summing `get_type_size2` for each
    recognized channel, and aligning that per-sample byte count to a 4-byte
    boundary only for versions above 11. Native
    `source_grim_char_bones_samples_data_plan` ports that stride rule and the
    GH2 clip parser now uses it for `BoneList.frame_bytes`; it still refuses
    unsupported channel names and does not evaluate or publish those channels
    as final pose output.
  - Grim's `recompute_sizes` separately derives `computed_sizes` from the
    cumulative count rows using `get_type_size` rather than `get_type_size2`,
    then 16-byte-aligns `computed_flags`. Native
    `source_grim_char_bones_samples_recompute_sizes` ports that distinction and
    the GH2 parser now validates `CharBonesSamples` count rows against it
    without replacing the separate native `CharBones` layout helper.
  - Grim's `decode_samples` groups decoded channel rows under mesh target names
    by replacing `.pos`, `.quat`, and `.rotz` suffixes with `.mesh`. Native
    `source_grim_char_bones_samples_channel_mesh_name` ports that exact
    conversion, and the GH2 clip parser now emits Grim-style `.mesh` channel
    targets instead of a local bare-name stripping rule.
  - Grim then sorts decoded `CharBoneSample` rows by `symbol`. Native
    `source_grim_char_bones_samples_sort_decoded_channels` mirrors that as a
    stable per-frame sort by decoded target name before returning parser frames,
    preserving same-target channel order while matching the source output order.
  - The same Grim `decode_samples` body only decodes `.pos`, `.quat`, and
    `.rotz`; all other transform types hit the unsupported `t@_` panic arm.
    Native `source_grim_char_bones_samples_decode_plan` and
    `source_grim_char_bones_samples_decodes_channel_type` expose that decode
    set, and the GH2 clip parser uses it for the publish/skip decision while
    still consuming unsupported channel bytes to keep packed sample rows aligned.
  - Grim stores each serialized channel as `CharBone { symbol, weight }` and
    attaches `bone.weight` to the decoded `.pos`, `.quat`, or `.rotz` sample
    vector. Native `ClipChannel::source_weight` and
    `source_grim_char_bones_samples_channel_weight` now retain that authored
    channel weight as decode metadata; broad pose weighting remains fenced to
    the missing `CharBonesSamples`/`CharBones` runtime bodies.
  - `SetPreview` clamps the preview sample and points `mStart` at the selected
    packed row.
  - Native `source_char_bones_samples_allocate_size`,
    `source_char_bones_samples_set`, `source_char_bones_samples_clone`,
    `source_char_bones_samples_set_preview`, and
    `source_char_bones_samples_split_steps` port those complete state/offset
    bodies for valid sample rows: allocation is `totalSize * numSamples`,
    `Set` clears previous sample state, applies the source compression guard to
    the prepared bone layout, stores the sample count, records the new raw-data
    allocation size, and clears frames; `Clone` repeats `Set` and copies the
    frame vector. This does not claim the still-missing `AddBoneInternal` body
    or expose a native `mRawData` pointer. Preview stores the clamped sample and
    selected row offset, and split steps report the source `i` / `i + 1` row
    offsets and `(1 - frac)` / `frac` weights. Native
    `source_char_bones_samples_rotate_by_offset`,
    `source_char_bones_samples_rotate_to_steps`, and
    `source_char_bones_samples_scale_add_steps` are named wrappers for the
    source `RotateBy`, `RotateTo`, and `ScaleAddSample` call shapes only; they
    do not add new pose math beyond the source row selection and upstream
    split-step flow.
    Empty sample rows remain fenced because the source body assumes an
    allocated packed row buffer.
  - The RB2 dump also maps `CharClipSamples` runtime-side functions that sit
    above the packed `CharBonesSamples` rows: `FacingBones::Set`,
    `FacingSet::ScaleAdd`, `FrameToSample`, `GetChannel`, both
    `EvaluateChannel` overloads, `RotateBy`, `RotateTo`, both `ScaleAdd`
    overloads, `Relativize`, `SetRelative`, and `Load`. Native
    `source_char_clip_samples_runtime_dump_evidence` records those exact
    ranges and visible locals, including the `FacingSet::ScaleAdd`
    `curPos`/`curAng`/`lastAng` locals and the `CharClipSamples::Load`
    `CharBonesSamples delta` local. This is a source-backed boundary only: the
    dump exposes ranges and local inventories, not statement-level C++ bodies,
    so native must not use it to publish broad pose, channel, or facing output.
  - Grim's newer `CharClipSamples` path reads extra-bone rows after the `full`
    and `one` `CharBonesSamples` blocks: `bone_count`, then repeated prefixed
    name and float weight rows. The checked source marks those values with a
    TODO and discards `_name`/`_weight`; native
    `source_grim_char_clip_samples_extra_bones_plan` records the row shape but
    keeps `stores_runtime_rows=false`. Do not drive pose, hair, or accessory
    behavior from these rows until a source body proves how they are consumed.
- `rb3-latest/src/system/char/CharClip.cpp` is concrete for clip resource
  context, `StuffBones`, `PoseMeshes`, play/clip flags, beat-event loading, and
  `full`/`one` property sync. It declares or calls the broad pose math, but the
  checked file does not include reviewable bodies for `CharClip::ScaleAdd`,
  `CharClip::RotateBy`, `CharClip::Load`, or channel evaluation.
  - Native `source_char_clip_default_state` records the complete checked
    constructor defaults: 30 FPS, zero flags/play flags/range, dirty true,
    compression allowed, `unk42 = -1`, and one beat-track key at frame/value
    zero.
  - Native `source_char_clip_beat_align_string` ports the concrete
    `CharClip::BeatAlignString` body for the `0xF600` play-flag group:
    `RealTime`, `UserTime`, `BeatAlign1`, `BeatAlign2`, `BeatAlign4`,
    `BeatAlign8`, and `NoAlign`.
  - Native `source_char_clip_beat_event_*` helpers port the concrete
    `BeatEvent` constructor/copy/assignment and load row order: event symbol
    first, beat float second, with the default beat at `0.0`.
  - Native `source_char_clip_prop_sync_plan` records the checked property rows
    for `CharGraphNode`, `CharClip::NodeVector`, `CharClip::BeatEvent`, and
    `CharClip`, including the special `full` and `one` `CharBonesSamples`
    subobject branches. This is property-row evidence only; it does not provide
    the missing clip sample evaluation or pose publishing bodies.
  - Native `source_char_clip_get_context` ports the concrete `GetContext`
    fallback: a type definition with a `resource` array reads slot 2 through
    `DataGetMacro(found->Str(2))->Int(0)` and returns that macro context value;
    missing type/resource data returns zero. The companion
    `source_char_clip_get_context_lookup` keeps the macro symbol visible in
    deterministic tests so native does not collapse the source `resource` row
    into an untraceable integer.
  - Native `source_char_clip_get_resource` ports the concrete `GetResource`
    lookup shape: inspect the type definition's `resource` array, request the
    named `CharBoneDir` resource, and warn when no resource is resolved. It
    records the lookup decision only; it does not load or synthesize resources.
  - Native `source_char_clip_transitions_*` helpers port the concrete
    `Transitions` constructor, `Size`, and `Clear` bodies: the node range starts
    empty with an owner, `Size` counts transition node-vector entries, and
    `Clear` releases one clip per entry before resizing the range to zero. This
    does not claim `Resize`, `RemoveNodes`, or transition graph evaluation.
    The RB2 dump maps `RemoveNodes`, `ResizeNodes`, and `AddNode` at
    `0x803286D0 -> 0x80328774`, `0x80328774 -> 0x803288A4`, and
    `0x803288A4 -> 0x80328A1C`, respectively, with local inventories but no
    statement-level bodies. Native
    `source_char_clip_transitions_dump_evidence` records those ranges and keeps
    `has_statement_bodies=false` so transition mutation is not fabricated from
    names alone.
  - Native `source_char_clip_runtime_dump_evidence` records the adjacent RB2
    `CharClip` runtime map: `FindNodes`, `FindFirstNode`, `FindLastNode`,
    `FindNode`, `Replace`, `ClearAllNodes`, `Load`, `CheckStick`, and
    `SyncProperty`, including the repeated `CharClip::Load` locals and the
    `CheckStick` stick/arm/bones/down-vector/angle locals. This is still a
    range/local inventory only. It is not a source body for `CharClip::Load`,
    `CheckStick`, or `SyncProperty`, and native must not import those behaviors
    until statement-level source or direct trace evidence exists.
  - Native `source_char_clip_stuff_bones` and
    `source_char_clip_pose_meshes_steps` port the concrete `StuffBones` /
    `PoseMeshes` call order only: list clip bones, append them into the target
    `CharBones`, create temporary `tmp_viseme_bones`, scale down at `0.0`, call
    `ScaleAdd(meshes, 1.0, frame, 0.0)`, then pose meshes. This records source
    dataflow without claiming the still-missing pose math bodies.
  - Native `source_char_clip_set_flags` and
    `source_char_clip_set_play_flags` port the complete `SetFlags` and
    `SetPlayFlags` dirty-state bodies: unchanged values preserve the incoming
    dirty state, while changed values store the requested flag value and mark
    the clip dirty.
  - Native `source_char_clip_shares_groups` ports the complete
    `CharClip::SharesGroups` ownership query over the clip's `Refs()` list:
    scan source ref owners in reverse order, ignore non-`CharClipGroup`
    owners, and return true when any owning clip group contains the candidate
    clip. This helper records the source group-membership rule only; it does
    not claim the missing clip-driver evaluator.
- `rb3-latest/src/system/char/CharClipDriver.cpp` is concrete for clip-driver
  stack construction, mask application to default blend/loop/beat-align flags,
  clip deletion, exit events, and sync animation cleanup. It does not include a
  reviewable `Evaluate` or `Poll` body.
  - Native `char_clip_driver_masked_play_flags` ports the constructor mask
    application exactly: low mode bits, loop bits, and the `0xF600` real-time /
    beat-align group override the clip's stored play flags only when present in
    the source mask. `CharClipPlayer::play` uses this masked value when it
    starts a native layer. `ghogx_character_clip_driver_flags_test` covers the
    mask groups and the source `CharDriver::Starved` helper branches.
  - Native `source_char_clip_driver_construct`,
    `source_char_clip_driver_delete_stack_order`,
    `source_char_clip_driver_exit_decision`,
    `source_char_clip_driver_delete_clip_result`, and
    `source_char_clip_driver_should_execute_event` port the remaining concrete
    stack-management decisions in `CharClipDriver.cpp`: initialized constructor
    fields, tail-first `DeleteStack`, `Exit(true)` recursive stack teardown,
    `Exit(false)` returning the next node, `DeleteClip` removing the first
    matching node, and the `ExecuteEvent` guard requiring both a non-null symbol
    and a clip type definition. These helpers do not claim `Evaluate`.
  - Native `source_char_clip_driver_runtime_dump_evidence` records the exact
    RB2 dump ranges and local inventories currently available for the missing
    runtime bodies: copy constructor `0x8032D060 -> 0x8032D168`,
    `Evaluate` `0x8032D33C -> 0x8032DA1C`, `ScaleAdd`
    `0x8032DA1C -> 0x8032DB3C`, `RotateTo`
    `0x8032DB3C -> 0x8032DC90`, `AlignToFrame`
    `0x8032DC90 -> 0x8032DDD0`, and `PlayEvents`
    `0x8032DDD0 -> 0x8032DFB4`. The helper also records the visible locals
    (`nextWeight`, `rt`, `ut`, `rampDelta`, `oldFrame`, `delta`, `dfrac`,
    `length`, `w`, plus the smaller function-specific locals) and keeps
    `safe_to_import_runtime=false` because this is still a range/local map, not
    a statement body.
- `rb3-retail-old/doc/rb2_dump/rockband2/system/src/char` exposes RB2-era dump
  entries for `CharClipSamples`, `CharBonesSamples`, `CharClip`,
  `CharClipDriver`, and `CharDriver`. These files are useful source-backed
  function maps: they identify `FrameToSample`, `ScaleAdd`, `RotateBy`,
  `RotateTo`, `FacingSet`, `CharClipDriver::Evaluate`, and the driver-to-bone
  application flow.
- The checked source is still incomplete for the exact bodies needed to blindly
  replace native clip playback: `CharBonesSamples::LoadHeader`, `LoadData`,
  `EvaluateChannel`, `Relativize`, `CharBones::ScaleAdd`, `RotateBy`,
  `RotateTo`, `Blend`, `CharClip::ScaleAdd`, `RotateBy`, `Load`, and
  `CharClipDriver::Evaluate` are either declared, called, or function-mapped,
  but not fully implemented as reviewable C++ bodies in the current public
  source.
- `band3_recomp` currently contributes symbol-table names such as
  `CharClip::SyncProperty` and `CharBones::ScaleAddIdentity`, not a decompiled
  runtime implementation for applying output bones to the live character pose.
- Native GHOGX may decode and log `CharClipSamples`, `CharBonesSamples`, and
  `CharBone` rows. It may use explicitly selected hand-driver output rows
  needed by the authored fret/strum overlay path only when that path stays
  bounded to source-named hand-driver semantics. Broad body, face, lower-body,
  or full CharBone output publishing remains opt-in diagnostic behavior until a
  source-backed implementation or direct original-game trace proves it.
- Current Rock2 evidence is bounded: static/bind rendering does not show the
  long-neck read, while `idle_medium_01` applies active `bone_head`,
  `bone_neck`, and spine clip rows. That points to clip/controller application,
  but it is not evidence for a native neck offset or named-character posture
  correction.

## Stock GH2 Evidence

The expanded stock controller/hair inventory at
`engine/out/source_truth_controller_inventory_20260710/expanded_stock_characters_controller_hair_inventory.log`
loads 24 base character MILOs from the stock GH2 PS2 ARK:
`alterna1`, `alterna2`, `classic`, `deathmetal1`, `deathmetal2`,
`female_singer`, `funk1`, `glam1`, `glam2`, `goth1`, `goth2`, `grim`,
`metal1`, `metal2`, `metal_bass`, `metal_drummer`, `metal_keyboard`,
`metal_singer`, `punk1`, `punk2`, `rock1`, `rock2`, `rockabill1`, and
`rockabill2`. Its controller rows prove:

- All 38 decoded `CharIKHand` rows are source revision 2, legacy
  single-target rows. They have `finger=<none>`, `targets=1`,
  `elbowSwing=0`, `alwaysElbow=0`, `constrainWrist=0`,
  `elbowCollide=<none>`, and `clockwise=0`.
  `engine/out/source_ikhand_20260711/stock_ikhand_controllers.stdout.log`
  refreshes that proof against the current decoder: all 38 stock `CharIKHand`
  rows are `version=2`, have no finger, and report `unreadBytes=0`.
- The focused weight-setter inventory at
  `engine/out/source_truth_controller_inventory_20260710/expanded_stock_characters_controller_inventory_weightsetter.log`
  proves all 38 stock `CharWeightSetter` rows are source revision 2 with
  `CharWeightable` revision 2, `offset=0`, `scale=1`, `base=<none>`,
  `minWeights=0`, and `maxWeights=0`. Nineteen rows carry
  `flags=0x00400000` for `left.weight`; nineteen carry `flags=0x00800000` for
  `right.weight`.
- Those stock rows therefore support the bounded GH2 single-target
  `CharIKHand` runtime slice; they do not justify porting or approximating
  source branches for multi-target weighting, finger offsets, elbow swing,
  wrist constraint, or elbow-collision correction unless another asset or
  trace proves those fields are present.
- The same inventory finds zero separate `CharCollide` objects in these 24
  base character MILOs. GH2 hair rows still carry authored inline collision
  targets such as `bone_head.mesh`, `bone_neck.mesh`, `bone_pelvis.mesh`,
  thigh bones, `hair.trans`, and `spot_hairsphere*.trans`.
- `metal_drummer` contains one revision-1 foretwist row with a missing
  `twist2` pointer. Native source-poll code must continue to skip incomplete
  controller refs instead of inventing a target.
- Only Grim exposes decoded `CharIKRod` rows in this 24-character base set:
  `rknee.rod` and `lknee.rod`. Both are source revision 2 with valid
  left/right knee endpoints and `bone_pelvis.mesh` as the side axis, but both
  have `dest=<none>`. Under ihatecompvir `CharIKRod::ComputeRod`, those rows
  are inert unless another asset or source path supplies a real `dest`.
- The stock type inventory at
  `engine/out/source_truth_controller_inventory_20260710/stock_character_type_inventory.log`
  proves all 24 base character MILOs contain one `CharServoBone` row. Native
  decodes/logs those rows so `CharDriver target=bone.servo` is explicit source
  data rather than an implied name.
  `engine/out/source_truth_controller_inventory_20260710/grim_charikrod_servo_inventory_after.log`
  records Grim's decoded row as `version=1 clipType=<none>`, matching the
  source `clip_type` gate.
  `engine/out/source_charservobone_20260711/stock_charservobone_controllers.stdout.log`
  refreshes that proof against the current decoder: all 24 stock rows are
  `version=1`, have no `clipType`, and report `unreadBytes=0`.
- Four base characters have no decoded `CharHair` rows in this stock set:
  `metal_bass`, `metal_drummer`, `metal_keyboard`, and `metal_singer`. The
  other 20 base character MILOs expose 31 decoded `CharHair` rows total.
  Native hair audits now summarize each decoded `CharHair` row with source-data
  point totals, missing driven-bone counts, inline collision reference counts,
  side-length/`unk5c` presence, wind, and unread byte tails. This is diagnostic
  inventory only; it does not publish guessed hair physics or placement.
  Fresh proof at
  `engine/out/source_truth_hair_digest_20260710/stock_character_hair_digest.log`
  records all 31 decoded hair rows with `unreadBytes=0`.
- The focused refreshed controller inventory at
  `engine/out/source_truth_controller_inventory_20260710/expanded_stock_characters_controller_posconstraint_inventory.log`
  shows five `CharPosConstraint` rows total: one each in `female_singer`,
  `grim`, `metal_bass`, `metal_keyboard`, and `metal_singer`. All are revision
  2. `female_singer` and `metal_singer` target `shadow.mesh`; `metal_bass` and
  `metal_keyboard` have zero targets; Grim's `hems.pcon` names `source=grim`
  and has zero targets. Native now polls these rows through the source clamp
  path; rows with zero decoded targets naturally produce no writes.
- The focused refreshed controller inventory at
  `engine/out/source_truth_controller_inventory_20260710/expanded_stock_characters_controller_driver_midi_inventory.log`
  shows 38 `CharDriverMidi` rows across the stock guitarist set. Every row is
  `midiVersion=3` with `midiDefaultClip=<none>`, `midiUnreadBytes=0`,
  `midiParser=<none>`, `midiFlagParser=<none>`, and
  `midiBlendOverridePct=1.0000`. Under the ihatecompvir `CharDriverMidi::Load`
  gates, GH2 stock rows are before the parser/flag/blend fields and their
  revision-below-7 default clip slot is the source-backed empty `ObjPtr` string.
- The refreshed stock type inventory at
  `engine/out/source_truth_controller_inventory_20260710/stock_character_type_inventory_latest.log`
  also records 25 base `CharDriver` rows across the 24 base character MILOs.
  Every listed character has one base driver row except Grim, which has two.
  Native logs these rows so targets such as `bone.servo` and clip directories
  are visible source data, but the missing base `CharDriver::Load`/`Poll` source
  bodies above keep base driver runtime behavior fenced.
- `rb3-latest/src/system/char/CharPollGroup.cpp` provides a real source
  authority when such rows exist: `Load` reads `Hmx::Object`, optional
  `CharWeightable` data for revisions above 2, `mPolls`, and revision-above-1
  `mChangedBy`/`mChanges`; `Poll` iterates `mPolls` only when the weight owner
  is nonzero. The focused stock type inventory at
  `engine/out/source_truth_poll_inventory_20260710/stock_character_type_inventory.log`
  finds no `CharPollGroup` rows across the 24 base character MILOs listed
  above, so native GH2 must not synthesize a poll group or use it as a hidden
  controller-order source for these assets.
- The same focused stock type inventory records 21 `FaceFxLipSyncServo` rows.
  They are present on all listed base characters except `metal_bass`,
  `metal_drummer`, and `metal_keyboard`. Because no matching ihatecompvir
  `FaceFxLipSyncServo::Load` body is checked in, these rows remain a bounded
  GH2 compatibility decoder rather than source-backed face-servo controller
  authority.
- The refreshed type inventory at
  `engine/out/source_truth_controller_inventory_20260710/stock_character_type_inventory_latest.log`
  shows one stock `AnimFilter` row, on `metal_drummer`. Native now decodes/logs
  that row using the ihatecompvir `RndAnimFilter::Load` order.
  `engine/out/source_truth_controller_inventory_20260710/stock_character_animfilter_inventory.log`
  records it as `char=metal_drummer name=crash_static.filt version=1
  animatableVersion=4 anim=<none> ... unreadBytes=0`. The same type inventory
  shows 19 `CharWalk` rows, but the available ihatecompvir RB2 dump exposes
  class members and runtime names while `CharWalk::Load` itself has no
  decompiled body; native therefore keeps `CharWalk` decode fenced rather than
  guessing its serialized layout.
- The focused EventTrigger inventory at
  `engine/out/source_truth_eventtrigger_20260710/metal_drummer_eventtrigger_inventory.stdout.log`
  shows the single stock `EventTrigger` row on `metal_drummer` as
  `name=game_over.trig version=8`, source trigger event `game_over`, one anim
  row pointing at `crash_static.filt`, and a still-unexplained zero tail
  `tailHex=00:00:00:00`.

## Remaining Stock Type Boundary

The refreshed stock type inventory at
`engine/out/source_truth_controller_inventory_20260710/stock_character_type_inventory_latest.log`
still reports undecoded non-mesh rows in stock character MILOs. These are
bounded as follows:

- `CharWalk`: 19 stock rows. The RB2 dump includes `CharWalk.cpp` and runtime
  function names, but `CharWalk::Load` only exposes `Debug TheDebug` and
  `gRev` references, not a field read order. Native does not decode or run
  these rows.
- `OutfitLoader`: 20 stock rows. The RB2 dump exposes the loader/change-outfit
  runtime surface, while `OutfitLoader::Load` is an empty/bodyless dump row and
  `PreLoad` belongs to broader loader state. Native does not treat these rows
  as character mesh or controller data.
- `CharPollGroup`: zero stock rows in the focused 24-character base-MILO type
  inventory. ihatecompvir source is sufficient to decode and poll a future
  verified row, but the current GH2 stock base-character data does not justify
  adding native poll-group behavior.
- `EventTrigger`: one stock row, on `metal_drummer`. Native now decodes and
  logs the source-backed field prefix using `EventTrigger::Load`,
  `ObjVector`, `ObjPtrList`, and `BinStream` evidence. It still does not run
  trigger scheduling, and the GH2 revision-8 four-byte zero tail remains logged
  as unresolved source evidence rather than consumed by guesswork.
- `Object`: 19 stock generic object rows. Native now decodes and logs the
  source-backed standalone `ObjectFields` row using ihatecompvir `Object.cs`.
  The focused object inventory records all 19 as `expression_task` rows with
  version `0`, no root tree, and `unreadBytes=0`; no character-model runtime
  behavior is promoted.
- `Tex`: 160 stock texture rows. Native now decodes and logs the
  source-backed metadata rows using `RndTex::Load`/`PreLoad`/`PostLoad`,
  `RndTex::Type`, `RndBitmap::LoadHeader`, `RndBitmap::PaletteBytes`,
  `ReadChunks`, `FilePath`, and `BinStream` evidence. It records the cached
  bitmap header and verifies the cached bitmap payload byte boundary,
  but does not change texture upload or material binding; native texture
  payloads are already handled by the PS2 texture asset path
  (`asset/milo_image.*`) keyed from material diffuse texture names.
- `WorldFx`: 99 stock rows. The available ihatecompvir evidence is only
  `DirLoader::FixClassName`/symbol references for `WorldFx`; there is no
  checked `WorldFx::Load` source body. Native keeps these rows as inventory
  evidence only.

The larger `rb3-latest/src/system/rndobj` source snapshot includes many
render/effect classes that are real ihatecompvir source, but the focused GH2
base-character inventories above show zero stock character rows for them. In
this character-model slice, do not count `Cam`, `CamAnim`, `Env`, `EnvAnim`,
`Lit`, `LitAnim`, `Flare`, `Fur`, `Wind`, `Part`, `PartAnim`,
`PartLauncher`, `TexRenderer`, `TexBlendController`, `TexBlender`, `CubeTex`,
`ColorXfm`, `Line`, `PostProc`, `ScreenMask`, or `SoftParticles` as remaining
character-model implementation unless a later stock inventory proves such rows
exist in the character MILOs. This keeps the implementation queue tied to rows
the GH2 assets actually contain instead of the full RB3 engine source tree.

Native `OpaqueObjectRow` inventory records any unhandled directory entry after
the source-backed decoder table declines it. The record is limited to entry
name, class symbol, body byte count, and first/last byte prefixes. This is for
stock-audit evidence only and is not a class-specific loader, controller, mesh,
or material behavior path.
Fresh proof at
`engine/out/source_truth_opaque_rows_20260712/stock_character_opaque_type_inventory.log`
records 138 opaque stock rows across the 24-character base set: 19 `CharWalk`,
20 `OutfitLoader`, and 99 `WorldFx`, all tagged
`note=no-source-loader-body`.

The local stock-asset audit log at
`analysis/ihatecompvir_source_truth_20260710/stock_hair_bone_inventory.log`
confirms the PS2 assets use source-style hair bones and authored collision
targets:

- `rock1`: `hair_back.hair` drives `bone_hair01.mesh` through
  `bone_hair04.mesh`; `hair_front.hair` drives left/right hair chains.
- `rock2`: `hair_front.hair` and `hair_back.hair` both resolve to authored
  `bone_hair*` / side hair bones and collision targets.
- `rockabill2`: `chain.hair` drives chain bones with thigh collision; `hair.hair`
  drives `bone_hair.mesh` and has no collision target.

The local stock material audit log at
`analysis/ihatecompvir_source_truth_20260710/source_rndmat_order_material_audit.log`
confirms native material rows now decode with the `RndMat.cs` source order:
`useEnviron` before `preLit`. This flips the older local note interpretation
for common flag bytes like `01 00`; those rows now decode as
`use_env=1 prelit=0`, not `prelit=1 use_env=0`.

The local stock group audit log at
`analysis/ihatecompvir_source_truth_20260710/source_rndgroup_character_audit.log`
confirms character `lod0.grp` and `lod1.grp` membership is decoded from
`RndGroup.objects` rows, including Rock1/Rock2 hair cards, Rockabill2 hair and
teeth meshes, Funk1 LOD groups, and Grim accessory/body segments.

The focused stock guitar sweep at
`engine/out/source_guitarstring_20260711/guitar_sweep/guitar_sweep.csv`
records `HasCharGuitarString=0` for every checked
`char/og/guitars/gen/*.milo_ps2` entry. `rb3-latest` `CharGuitarString::Poll`
projects the target transform onto the nut-to-bridge segment, clamps that
projection to `[0, 1]`, forces open strings to the nut, and writes the bend
transform. Native `source_char_guitar_string_*` helpers port that math and
`PollDeps` for deterministic coverage and later assets with real
`CharGuitarString` rows, but this is not the active GH2 stock guitar/left-hand
or string-transparency route and must not be promoted as an implicit fix.

The same source also exposes deterministic row metadata: constructor `mOpen=0`,
revision-0 load rows (`Hmx::Object`, `mNut`, `mBridge`, `mBend`, `mTarget`),
copy rows (`mTarget`, `mNut`, `mBridge`, `mBend`), the `set_open` handler,
`Hmx::Object` superclass/check `0x70`, and prop-sync rows (`nut`, `bridge`,
`bend`, `target`). Native `source_char_guitar_string_default_state`,
`source_char_guitar_string_load_plan`, `source_char_guitar_string_copy_plan`,
`source_char_guitar_string_handler_plan`, and
`source_char_guitar_string_prop_sync_plan` record those rows as source metadata
only.

The local stock mesh detail audit log at
`analysis/ihatecompvir_source_truth_20260710/source_rndmesh_group_sections.log`
confirms GH2 PS2 mesh group-section tails decode through the same
`RndMesh.GroupSection` rows for inspected stock character meshes.

The local stock transform constraint audit log at
`analysis/ihatecompvir_source_truth_20260710/trans_constraint_audit/stock_character_constraints.log`
records only source constraint ids `0` (`kNone`) and `2` (`kParentWorld`) for
the sampled stock playable characters in that pass. Native transform
composition is still contracted to the full `RndTransformable` source enum; any
future stock row using dynamic constraints needs the matching source runtime
path, not a visual approximation.

The current Rockabill2 face/attachment proof at
`engine/out/rockabill2_face_current_20260710/` records the native path for the
previous eye/teeth concern. The screenshot keeps `top-teeth.mesh`,
`lower-teeth.mesh`, and both eye meshes on the head/mouth instead of down near
the collar. The logs show teeth/tongue meshes have no bone palette and draw
through the generic `source-trans-world` path, while `l-eye.mesh` and
`r-eye.mesh` are no-palette children of `bone_head.mesh` using source world
rows. This is evidence for the shared source transform path, not for a
Rockabill2-specific face attachment override.

## Native Rules

- Shared parser fixes are allowed when they follow the source files above.
- Character-name fixes are not source evidence.
- Renderer geometry selection must come from decoded source membership such as
  `RndGroup.objects`; name/palette suppressions like numbered-hair or terminal
  leg-overlay hiding are not source evidence.
- LOD visibility must come from source group membership (`lod0.grp` /
  `lod1.grp`), not `_lod1` or `lod_` name prefixes.
- Renderer state such as blend, z write, alpha test, wrap, and draw order must
  come from source material/drawable rows.
- Broad CharBone output bridges for full body, face, or lower body are
  diagnostic-only unless/until a source `CharBones` publisher is ported. They
  must require explicit enable switches and must not have default-on disable
  switches masquerading as source behavior.
- Project override: hair polygons/textures render two-sided. Native therefore
  forces no backface culling for shared hair-token mesh/material/texture
  surfaces. This is a visual policy override, not inferred source evidence, and
  is implemented as a single draw with culling disabled so texture alpha/blend
  contribution is not doubled. It must not affect source blend, depth write,
  alpha test, texture wrap, material color, or draw sort, and it must not be
  used to invent hair blend/depth/alpha/sort behavior.
- If a behavior is not proven by ihatecompvir source or stock asset data, leave
  it decoded/logged and unwritten until the source-backed runtime path is known.
