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

## Source Coverage Matrix

| Area | ihatecompvir evidence | Native status |
| --- | --- | --- |
| Object and property-tree skip/read shape | `Object.cs`, `DTBNode.Read` | Parser authority; native skips must mirror source enum and logs standalone generic `Object` rows. |
| Character/BandCharacter/RndDir/ObjectDir root body | `rb3-latest` `Character.cpp`, `rndobj/Dir.cpp`, `obj/Dir.cpp` | Native records the root directory revision/name/type and opaque root object body boundary; no root runtime fields are decoded until the exact GH2 revision/body relation is pinned. |
| Character lifecycle and directory sync flow | `rb3-latest` `Character.cpp`, `Character.h` | Native helper ports constructor defaults, poll-state enum order, Enter/Exit/Poll state changes, main-driver discovery, sphere-base replacement, eyes gates, and SyncObjects cleanup/sort flow. |
| Character subsystem init/terminate | `rb3-latest` `Char.cpp`, `Char.h` | Native helper records the source init/terminate order only; it does not install callbacks or alter runtime startup. |
| Character test harness defaults | `rb3-latest` `CharacterTest.cpp`, `CharacterTest.h` | Native helper ports editor/test defaults, draw/poll decisions, `AddDefaults` controller creation names and offsets, walk/teleport/start-end/load gates, and move-self delegation. This is harness evidence only, not a live controller or playback import. |
| Transformable local/world composition | `RndTrans.cs`, `Trans.cpp`, `Trans.h` | Runtime authority for parent/constraint world rows. |
| Transform copy controller | `rb3-latest` `CharTransCopy.cpp` / `CharTransCopy.h` | Native helper ports the complete null-gated local-transform copy and dependency publication behavior; no stock runtime hookup is promoted without rows. |
| Group membership and LOD selection | `RndGroup.cs` | Runtime/draw membership must use decoded object rows. |
| Mesh hide visibility rows | `rb3-latest` `CharMeshHide.cpp` / `CharMeshHide.h` | Native helper ports `HideAll` flag aggregation and `HideDraws` visibility gating; no renderer hookup is promoted until stock rows are proven. |
| Translucent character draw controller | `rb3-latest` `CharTransDraw.cpp` / `CharTransDraw.h`, `Character.h` draw-mode enum | Native helper ports source draw-mode command order only; it does not change renderer sorting or material state. |
| Cuff/accessory deformation rows | `rb3-latest` `CharCuff.cpp` / `CharCuff.h` | Native helper ports constructor defaults, source eccentricity math, and revision defaults; deformation and mesh hookup remain unwired without stock rows. |
| Blend-bone constraints | `rb3-latest` `CharBlendBone.cpp` / `CharBlendBone.h` | Native helper ports constructor/constraint defaults, load field order, and dependency publication; the checked source does not include the blend `Poll` body. |
| Sleeve secondary motion | `rb3-latest` `CharSleeve.cpp` / `CharSleeve.h` | Native helper ports source defaults, poll math, teleport reset, top-sleeve write, and dependency publication; no live runtime hookup is promoted without decoded rows. |
| Mesh palette, offsets, and group sections | `RndMesh.cs`, `Mesh.cpp` | Parser and skinning authority; no palette reshaping. |
| Material render state | `RndMat.cs`, `Mat.cpp`, `Mat.h` | Blend, z write, alpha, wrap, and draw order come from source rows. |
| Texture object row inventory | `rb3-latest` `Tex.cpp` / `Tex.h`, `Bitmap.cpp`, `ChunkStream.cpp`, `FilePath.h`, `BinStream.*` | Decode/log stock `Tex` metadata rows, cached bitmap headers, and source-backed payload byte boundaries; texture upload stays on the existing PS2 image asset path. |
| Rnd utility animation rows | `rb3-latest` `AnimFilter.cpp` / `Anim.cpp` | Decode/log stock `AnimFilter` rows; no trigger or animation runtime hookup. |
| Event trigger row inventory | `rb3-latest` `EventTrigger.*`, `ObjVector.h`, `ObjPtr_p.h`, `BinStream.*` | Decode/log stock source fields only; trigger scheduling and the GH2 v8 four-byte zero tail remain fenced. |
| Fenced stock object rows | RB2 dump `CharWalk.cpp` / `OutfitLoader.cpp`, `DirLoader` `WorldFx` fixup refs | Fenced unless the exact source load path is present. |
| Hair row decode and simulation boundary | `glTFMilo` hair builder, `rb3-latest` `CharHair.*` / `CharCollide.*`, `band3_recomp` symbols | Decode/log source rows and run the checked source poll/reset/sim state path; no point writeback until `Hookup(ObjPtrList<CharCollide>&)` is faithfully ported. |
| Eyes/look-at controllers | `CharEyes.cpp`, `CharLookAt.cpp`, `CharInterest.cpp` / `CharInterest.h`, `CharEyeDartRuleset.cpp` / `CharEyeDartRuleset.h` | Decode/log GH2 rows through the source `CharWeightable` + `source`/`pivot`/`dest` order; native helpers port `CharEyes` poll-child/dependency publication plus `CharInterest` / `CharEyeDartRuleset` data decisions only; no synthetic eye runtime bridge. |
| Character mesh cache | `rb3-latest` `CharMeshCacheMgr.cpp` / `CharMeshCacheMgr.h` | Native helper ports constructor defaults, disabled-state capture, membership checks, bounded `GetVerts`, visible `SyncMesh` index behavior, and mesh-list stuffing. It is bookkeeping-only and does not alter live renderer/cache ownership. |
| FaceFX/lip-sync boundary | `rb3-latest` `CharFaceServo.*`, `CharLipSync.*`, `CharLipSyncDriver.*`; stock GH2 `FaceFxLipSyncServo` inventory | `CharFaceServo` and `CharLipSync` are source context, not matching `FaceFxLipSyncServo` load bodies; native FAC/viseme lookup stays bounded compatibility. |
| Position constraints | `rb3-latest` `CharPosConstraint.cpp` / `CharPosConstraint.h` | Decode/log source, targets, and box rows; native `Poll` ports the source target/source delta clamp and writes target world rows. |
| Waypoint clip/path diagnostics | `rb3-latest` `Waypoint.cpp` / `Waypoint.h` | Native helper ports source defaults/load/copy and `ShapeDeltaBox` / `ShapeDeltaAng` / `Constrain` math for diagnostics; no live camera/path behavior is invented. |
| Bone offsets | `rb3-latest` `CharBoneOffset.cpp` / `CharBoneOffset.h` | Decode/log source destination and offset rows; native helper ports source `Poll`/`ApplyToLocal` math without adding an unproven frame-cadence write. |
| Bone twist controller | `rb3-latest` `CharBoneTwist.cpp` / `CharBoneTwist.h` | Decode/log source bone, targets, and weight rows; native helper ports source target-average twist solve without adding an unproven frame-cadence write. |
| Hand/head/foot IK, IK MIDI, slider MIDI, and IK fingers | `CharIKHand.cpp`, `rb3-latest` `CharIKHead.cpp` / `CharIKHead.h`, `CharIKFoot.cpp` / `CharIKFoot.h`, `CharIKMidi.cpp` / `CharIKMidi.h`, `CharIKSliderMidi.cpp` / `CharIKSliderMidi.h`, `CharIKFingers.cpp` / `CharIKFingers.h` | Native hand IK follows source dataflow; IK head helpers port source defaults, dependency publication, point-chain rebuilding, load gates, and copy flow without inventing the absent `Poll` body; IK foot helpers port source helper-target setup, FSM, load gates, and delegation plan without inventing row hookup; IK MIDI rows decode/log the source `mBone` and revision-gated legacy/anim blend fields; IK slider MIDI helpers port source defaults, dependency publication, setup reset, load gates, and copy flow without inventing the absent `Poll` / `SetFraction` bodies; IK fingers helpers port source defaults and left/right finger transform names only. |
| IK scale controller | `rb3-latest` `CharIKScale.cpp` / `CharIKScale.h` | Native helper ports constructor defaults, source poll gate, capture-before/after scale rows, and dependency publication; the checked source `Poll` body has no implemented scale write. |
| Clip drivers | `rb3-latest` `CharDriver.cpp` / `CharDriver.h`, `CharDriverMidi.cpp` / `CharDriverMidi.h`; `CharWeightable.cpp`; `ObjPtr_p.h`; RB2 dump `CharDriver.cpp` | Decode/log driver inventory, inherited weight owner, default clip pointer, parser rows, and blend override gates. Base `CharDriver::Load`/`Poll` bodies are not present in the available source, so runtime clip selection remains source-fenced. |
| Clip groups | `rb3-latest` `CharClipGroup.cpp` / `CharClipGroup.h` | Native shared loader follows source `CharClipGroup::Load`: `Hmx::Object::Load`, `mClips`, `mWhich`, and revision-gated `mFlags`. Guitarist active group selection now follows source `CharClipGroup::GetClip` cycling. Flagged `GetClip(int)` selection remains fenced because the available body is not decompiled. |
| Clip set preview/editor container | `rb3-latest` `CharClipSet.cpp` / `CharClipSet.h` | Native helper ports reset/default state, group randomize/sort dispatch, pre/post-save preview handling, revision-gated post-load read plan, preview character decisions, frame helpers, and BPM update; it does not promote clip playback runtime. |
| Clip display/task graph diagnostics | `rb3-latest` `CharClipDisplay.cpp` / `CharClipDisplay.h`, `CharTaskMgr.cpp` / `CharTaskMgr.h` | Native helper ports display init, source lookup, text width plus em, bounded start/end bookkeeping, line spacing, and task-graph toggle registration. This is diagnostic/editor-only and does not change runtime clip playback. |
| Clip editor/collision/graph diagnostics | `rb3-latest` `ClipCollide.cpp` / `ClipCollide.h`, `ClipGraphGen.cpp` / `ClipGraphGen.h`, `ClipDistMap.h`, `ClipCompressor.cpp`, `FileMerger.cpp` / `FileMerger.h` | Native helper ports source-visible editor defaults, transition-generation gates, list/test call plans, and merger row defaults only; no live collision, transition graph execution, compression, or file merging behavior is promoted. |
| Weight setters and weight owners | `rb3-latest` `CharWeightable.cpp` / `CharWeightSetter.cpp` | Decode/log source weight rows; full setter `Poll` remains fenced to source driver/evaluate path. |
| Mirror servo controller | `rb3-latest` `CharMirror.cpp` / `CharMirror.h` | Native helper ports constructor defaults, nonzero-weight/nonempty-bones `Poll` gate, servo setter `SyncBones` triggers, dependency publication, load order, and copy flow; `SyncBones` bone rebuilding remains fenced because the body is absent from `rb3-latest`. |
| Rod IK/accessory rods | `rb3-latest` `CharIKRod.cpp` / `CharIKRod.h` | Decode/log source rows; do not synthesize missing destination transforms. |
| Guitar string bend controller | `rb3-latest` `CharGuitarString.cpp` / `CharGuitarString.h`; stock guitar sweep | Native helper ports source `Poll` projection/open-string math and `PollDeps`, but the checked GH2 stock guitar MILOs contain no `CharGuitarString` rows; native does not invent one. |
| Upper/fore/neck twist | `CharUpperTwist.cpp`, `CharForeTwist.cpp`, `rb3-latest` `CharNeckTwist.cpp` / `CharNeckTwist.h` | Native upper/fore passes follow source `Poll` routines; neck twist rows decode/log the source load order and expose helper math, but stock GH2 character inventories currently show zero `CharNeckTwist` rows. |
| Poll groups | `rb3-latest` `CharPollGroup.cpp` | Native helper ports source `Poll`, `ListPollChildren`, and `PollDeps` decision behavior, but stock GH2 base-character inventory contains no `CharPollGroup` rows; native does not invent one. |
| Servo bone driver target | `rb3-latest` `CharServoBone.cpp` / `CharServoBone.h` | Decode/log the `bone.servo` row and `clip_type`; movement remains fenced by clip/CharBones source. |
| Clip sample/output publishing | `rb3-latest` `CharClip` / `CharBones` / `CharBonesSamples` / `CharBone`, `MiloEditor` `RndTrans.cs`, `rb3-retail-old` RB2 dump, `band3_recomp` symbols | Channel naming, compression sizing, sample interpolation wrappers, CharBone output row fields, and partial call flow are source-backed; sample decode/evaluate and broad pose publishing remain fenced where source bodies are incomplete. |
| Hair two-sided rendering | User/project visual override | Two cull passes only; not source evidence for material/depth/sort changes. |

## Character Mesh Cache Helper

`CharMeshCacheMgr` coverage is intentionally fenced to the source behavior
visible in `rb3-latest/src/system/char/CharMeshCacheMgr.cpp` and `.h`:

- `MeshCacher` stores the mesh pointer, zeroes `unk4`, and captures the current
  disabled flag at creation time.
- `Disable(bool)` is only valid before any cache entries exist.
- `HasMesh` and `GetVerts` scan the cache in order by mesh identity.
- `SyncMesh` preserves the visible ihatecompvir index/post-increment scan
  behavior and appends a new cacher when that scan reaches the current cache
  size; native records the null-mesh assertion instead of dereferencing it.
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
    character path, waypoint, and position, then clears `mClip`.
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

## Remaining Character Import Checklist

This is the current answer to "what remains to import" from ihatecompvir's
character sources. Mesh row decode, material state, transform composition,
group draw membership, IK hand, upper twist, fore twist, and position
constraints have source-backed native coverage. The unresolved work is the
connected character animation and controller runtime that turns authored clips
into final transform rows.

1. Whole-character clip and pose stack:
   - Port the missing source-backed bodies for `CharBonesSamples::LoadHeader`,
     `LoadData`, `EvaluateChannel`, and `Relativize`.
   - Port the missing source-backed bodies for `CharBones::ScaleAdd`,
     `RotateBy`, `RotateTo`, `Blend`, and any required identity/mesh
     application helpers.
   - Port the missing source-backed bodies for `CharClip::Load`,
     `ScaleAdd`, `RotateBy`, and related `FacingSet` behavior.
   - Port `CharClipDriver::Evaluate`/poll timing, blend, loop, beat-align,
     and exit behavior from a reviewable source body or direct original-game
     trace. The current `rb3-latest` file only exposes stack construction and
     flag masking; the RB2 dump gives a function map, not enough standalone C++
     body to blindly copy.
   - Port `CharDriver::Load`, `CharDriver::Poll`, and `EvaluateFlags`, plus
     the `CharDriverMidi` parser/message-sink path, before treating source
     drivers as active clip selection. Until then, native viewer clip playback
     is diagnostic/application glue, not proof of the Harmonix driver runtime.

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
   - `CharEyes` and `CharLookAt` can be ported from source poll bodies, but
     native must first respect the GH2 row shape and avoid the removed
     synthetic eye-row bridge.

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
- `rb3-latest/src/system/rndobj/Dir.cpp`
  - `RndDir::PreLoad` reads packed revisions, asserts source revision `0xA`,
    pushes that revision, and delegates to `ObjectDir::PreLoad`.
  - `RndDir::PostLoad` delegates to `ObjectDir::PostLoad`, then loads the
    source superclasses `RndAnimatable`, `RndDrawable`, and revision-gated
    `RndTransformable` before environment/test/postproc rows.
- `rb3-latest/src/system/obj/Dir.cpp`
  - `ObjectDir::PreLoad` reads packed revisions, asserts source revision
    `0x1B`, then consumes revision-gated object/type prefix, reserve/hash,
    inline/proxy, viewport, and subdir state before pushing the revision.
  - `ObjectDir::PostLoad` pops the revision and resolves inlined dirs,
    subdirs, and proxy state.

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
deliverable is inventory only: `dirVersion`, `dirType`, `bodyOffset`,
`bodyBytes`, copied byte count, and head/tail hex proof.

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
- `CopyBoundingSphere` copies sphere, bounding, and the source sphere-base
  pointer when present, otherwise clears the pointer. `RepointSphereBase` only
  looks up by name when the pointer is non-null and only replaces it when the
  directory lookup succeeds. `PreSave` is just `UnhookShadow`.
- Native `source_character_*` helpers port these source-visible runtime flows
  for deterministic tests and future wiring. They do not decode the fenced root
  body bytes above and do not change current renderer/material behavior.
- `MiloEditor/MiloLib/Assets/Rnd/RndTrans.cs`
  - `RndTrans.Read` reads combined revision, optional object fields for
    standalone objects, local matrix, world matrix, old child references for
    revisions below 9, constraint, target, preserve-scale, then parent.
- `MiloEditor/MiloLib/Assets/Rnd/RndDrawable.cs`
  - `RndDrawable.Read` reads combined revision, showing, optional sphere, and
    draw order for revisions greater than 2.
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
- `MiloEditor/MiloLib/Assets/Rnd/RndMesh.cs`
  - `RndMesh.Read` calls `base.Read`, embedded `trans.Read`, embedded
    `draw.Read`, material, geom owner, vertices, faces, group sizes, then bone
    transforms.
  - GH2-era meshes are below revision 33. When the source presence check sees a
    bone-transform block, it is exactly four bone-name symbols followed by
    exactly four transform matrices. Empty names remain source slots; they are
    not evidence to reshape or renumber the palette.
  - For last-gen parent directories before revision 25, `RndMesh.Read` then
    reads one `GroupSection` per `groupSizes` row when `groupSizes[0] > 0`.
    Each `GroupSection` is `sectionCount`, `vertCount`, signed section indices,
    then unsigned vertex offsets. Native keeps these rows decoded/loggable
    instead of treating them as anonymous trailing bytes.
- `rb3/src/system/rndobj/Mesh.cpp`
  - `RndMesh::SetBone` is the runtime source for how a bone offset is authored:
    it inverts `t->WorldXfm()` and calls `Multiply(WorldXfm(), inverseBone,
    mBones[i].mOffset)`.
  - GH2-era `RndMesh::PostLoad` reads the same four source bone slots and four
    offsets that the native decoder preserves.
- `rb3/src/system/rndobj/Mat.cpp`
  - `RndMat` runtime defaults are source state: blend `kSrc`, texture wrap
    `kRepeat`, and z mode `kNormal`.
  - Native render state must come from decoded `RndMat`/`RndDrawable` rows, not
    from mesh or material names such as `hair`, except for the project-level
    hair two-sided cull rule below.
- `rb3/src/system/rndobj/Mat.h`
  - `RndMat` exposes source `GetBlend`, `GetZMode`, and `GetTexWrap` accessors.
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
- `rb3/src/system/rndobj/Trans.cpp`
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
- `glTFMilo/Source/glTFMilo/Core/NodeProcessor.cs`
  - `ProcessCharHair` builds `CharHair` strands from weighted hair-bone chains.
    The newer source splits strands at branches, matching how the decompiled
    runtime expects hair to be structured.
  - `ProcessHairCollides` can emit empty `CharCollide` objects for hair meshes,
    but its own source comment says this is inferred from decomp and "could be
    wrong". Treat those rows as exporter/format hints, not proof of GH2 runtime
    collision hookup.
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
  - `FreezePoseRaw` stores current point positions back into `unk5c` in the
    root-parent local basis. `FreezePose` performs a source `Hookup()`, simulates
    200 loops at 60 Hz, restores the previous simulate flag, then freezes those
    rows. Native `source_char_hair_freeze_pose_plan` ports the call order and
    restore behavior, and `source_char_hair_freeze_pose_raw` ports the raw
    local-row write. Full `FreezePose` remains bounded by the unresolved source
    `Hookup(ObjPtrList<CharCollide>&)` path.
  - `SetName` detects whether the owning directory is a `Character` or
    `WorldDir` and enables post-process FPS emulation accordingly. `GetFPS`
    returns the post-process emulated rate when available, otherwise 60 Hz.
    Native ports the constructor constants, SetName ownership branch, and the
    `GetFPS` branch into deterministic helpers so those source defaults remain
    explicit without guessing a post-process runtime.
  - `SimulateLoops` is gated by `mSimulate` and a non-empty strand list, runs
    collide-list maintenance, then calls `SimulateInternal` for each requested
    loop. Native `source_char_hair_simulate_loops_plan` ports that gate and
    call count as a deterministic plan. A decoded `CharHair` row alone is
    therefore not enough evidence for a native writeback path.
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
- `rb3-latest/src/system/char/CharCollide.cpp` and
  `rb3-latest/src/system/char/CharCollide.h`
  - `CharCollide::Load` reads `Hmx::Object`, `RndTransformable`, shape,
    radius/length/flags rows, optional current radius, optional second
    radius/length rows, an internal transform, mesh pointer, eight mesh sphere
    rows, SHA1 digest, and mesh-y-bias.
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
  - `CharHair::SimulateInternal` calls `CharCollide::GetRadius` and
    `CharCollide::Axis`, but `GetRadius` depends on cached collision fields
    (`unk18c`, `unk190`, `unk194`, and `unk1a0`). The latest source exposes the
    inline `GetRadius` formula, while the older RB2 dump only names
    `ComputeRadius` / `SyncRadius` without a usable body. Native GHOGX ports the
    inline formula as `source_char_collide_get_radius`, but it requires an
    explicit `SourceCharCollideRadiusCache` and is not wired into live hair
    simulation. Native therefore keeps collision response disabled until the
    cached-field updates are sourced instead of reconstructed by guesswork.
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
    exact data behavior for deterministic coverage; they do not enable
    procedural eye darts in GH2 runtime.
- `rb3-latest/src/system/char/CharInterest.cpp` and `CharInterest.h`
  - The constructor defaults are source data: max view angle `20.0`,
    priority `1.0`, look time `1.0..3.0`, refractory period `6.1`, no dart
    override, category flags `0`, no min-target-distance override, and min
    target distance override `35.0`. `SyncMaxViewAngle` stores
    `cos(maxViewAngle * 0.017453292)`.
  - `Load` accepts source revisions through 6 and reads the object,
    transformable, timing, priority, optional dart override, category flags,
    and min-target-distance override fields in the source order.
  - `IsMatchingFilterFlags` returns true only when the category mask overlaps
    and the category flags are non-zero. Native `source_char_interest_*`
    helpers port these data decisions and the copy-time max-view-angle resync.
    `ComputeScore` includes runtime vectors and `RandomFloat`; it stays fenced
    from native runtime until the surrounding source eye-interest path is ported
    or traced.
- `rb3-latest/src/system/char/CharTransCopy.cpp` and
  `CharTransCopy.h`
  - `CharTransCopy::Poll` returns immediately when either `mSrc` or `mDest` is
    missing. With both refs present, it calls `mDest->SetLocalXfm(mSrc->mLocalXfm)`.
  - `CharTransCopy::PollDeps` appends `mDest` to the `change` list and `mSrc`
    to `changedBy`, matching the source dependency direction.
  - Native `source_char_trans_copy_poll` and
    `source_char_trans_copy_poll_deps` port those complete source behaviors as
    isolated helpers. This does not imply active character runtime wiring unless
    stock `CharTransCopy` rows are decoded or another source-backed owner path
    is proven.
- `rb3-latest/src/system/char/CharPollGroup.cpp` and
  `CharPollGroup.h`
  - `CharPollGroup::Poll` iterates `mPolls` only when the source weight owner
    weight is nonzero. Zero weight skips every child.
  - `ListPollChildren` appends every poll child in list order.
  - `PollDeps` uses the explicit `mChangedBy` / `mChanges` pair when either
    pointer exists; otherwise it delegates dependency collection to each child
    pollable in list order.
  - Native `source_char_poll_group_poll_order`,
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
  - Native `source_char_sleeve_*` helpers port this source-visible simulation
    and dependency behavior for deterministic tests. They do not attach it to
    live character rendering until stock rows and owner ordering are decoded.
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
- `rb3-latest/src/system/char/CharLipSync.cpp` and
  `rb3-latest/src/system/char/CharLipSync.h`
  - `CharLipSync::Generator` initializes the lip-sync pointer to null,
    `mLastCount` to zero, and the weight list empty.
  - `CharLipSync` initializes `mPropAnim` to null and `mFrames` to zero.
  - `CharLipSync::Load` accepts source revisions through 1, reads
    `Hmx::Object`, then viseme names, frame count, raw data, and only reads
    `mPropAnim` when the revision is non-zero.
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
    defaults/load gates/dependency publication as source context only and do
    not promote any live GH2 mouth or viseme controller behavior.

## Rnd Utility Row Authorities

- `rb3-latest/src/system/rndobj/Anim.cpp` and
  `rb3-latest/src/system/rndobj/Anim.h`
  - `RndAnimatable::Load` reads a source revision, optional `mFrame`, then
    `mRate` for revisions above 3 or a legacy byte rate for revision 3. Revision
    0 branches into an old anim-filter/object-list conversion path.
  - Native GHOGX decodes the revisioned frame/rate fields and fences the
    revision-0 object-list branch until the relevant object-list serialization
    path is source-backed in this decoder.
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
  - `Waypoint::Load` accepts source revisions through 5. It reads
    `Hmx::Object`, dumps a legacy drawable for revisions below 5, reads
    `RndTransformable`, then reads `mFlags`, `mConnections`, optional radius,
    optional `mYRadius`/`mAngRadius`, and optional strict radius/angle deltas.
  - `Waypoint::Copy` copies `Hmx::Object`, `RndTransformable`, `mFlags`,
    `mConnections`, `mRadius`, `mYRadius`, `mAngRadius`,
    `mStrictRadiusDelta`, and `mStrictAngDelta`.
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
    constructor/`Enter` state reset, load gates, `PollDeps`, and copy-member
    list. `Enter` clears current/new spots, spot-changed state, interpolation
    fractions, and both local transforms; `PollDeps` publishes `mBone` as both
    changed-by and changed, plus `mCurSpot` as changed-by; `Copy` copies
    `Hmx::Object`, `mBone`, `mAnimBlender`, and `mMaxAnimBlend`.
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
    move-forward, and destination-offset fields by revision.
  - `SetName` resolves hard-coded left/right hand, forearm, upper-arm, finger
    joint, and fingertip transform names. The source then marks setup false only
    if a finger joint/tip is missing; it does not require hand/forearm/upperarm
    in that final completeness loop. Native `source_char_ik_fingers_*` helpers
    port these data decisions and raw source matrices before `Normalize`.
  - `SetFinger`, `ReleaseFinger`, `MeasureLengths`, and the real `Poll` math
    remain fenced; the checked source includes incomplete transform math and
    should not be promoted into live fretting-finger behavior from this data
    slice.
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
    `PlayGroup` only requests playback when both `mClips` and the named group
    resolve; `PollDeps` publishes `mBones` in the change list.
- `rb3-latest/src/system/char/CharDriverMidi.cpp` and
  `rb3-latest/src/system/char/CharDriverMidi.h`
  - `CharDriverMidi::Load` reads the subclass revision, accepts revisions
    through 7, loads `CharDriver`, then loads `mDefaultClip` for revisions below
    7. Revision 2 carries a legacy string; revisions above 3 read `mParser`,
    above 4 read `mFlagParser`, and above 5 read `mBlendOverridePct`.
  - `CharDriverMidi::Enter` attaches the object as a sink to `mParser` and
    `mFlagParser`. `OnMidiParser`, `OnMidiParserFlags`, and
    `OnMidiParserGroup` are the source-backed runtime route for MIDI/note-driven
    clip selection and blend width scaling.
  - Native `source_char_driver_midi_default_state`,
    `source_char_driver_midi_enter`, `source_char_driver_midi_exit`,
    `source_char_driver_midi_on_parser_flags`,
    `source_char_driver_midi_on_parser`, and
    `source_char_driver_midi_on_parser_group` port the concrete source
    decisions without activating the runtime scheduler: constructor defaults,
    parser sink add/remove decisions, clip-flag updates, default-clip selection
    when `!unk89 && mDefaultClip`, normal and real-time blend-width conversion,
    and group-message assignment of the returned node's `mBlendWidth`.
  - Native `source_char_driver_midi_copy_plan` records the checked source copy
    list: copy `CharDriver`, `unk89`, `mParser`, `mFlagParser`, and
    `mBlendOverridePct`. The source copy body does not name `mClipFlags`, so
    native records that absence as copy-plan evidence only.
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
  - `CharIKHand::PullShoulder` is source-real but not yet source-importable:
    `CharIKHand.cpp` calls it from `IKElbow`, and
    `ihatecompvir-extra/band3_recomp/band3_config.toml` exposes a
    `CharIKHand__PullShoulder` symbol, but the available ihatecompvir C++ only
    declares/calls the method and does not include its body. Native GHOGX
    therefore must not rederive that shoulder offset or claim a full IKElbow
    port until the function body is source-backed.
  - The current runtime solver is the bounded GH2 single-target slice. Source
    branches for multi-target weighting, `mFinger`, `PullShoulder`,
    `mElbowSwing`, wrist constraint, and elbow-collision correction remain
    fenced unless an asset log proves they are present and the matching
    ihatecompvir source branch is ported.
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
  - `CharNeckTwist::PollDeps` publishes `head` as the changed-by row and
    `twist` as the changed row. Native `source_char_neck_twist_poll_deps`
    mirrors that data behavior.
  - `CharNeckTwist::Poll` walks the head local transform chain up to the twist
    parent, derives a rotation from the resulting X/Y rows, and rotates the
    twist row about local X by half of `LimitAng(atan2(z, y))`. Native exposes
    only the source final scalar helper in this slice; it does not add a live
    neck write.
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
  - `CharBoneDir::GetContextFlags` lazily rebuilds cached context symbols only
    while `mContextFlags` is still the constructor integer node. The source
    scans `CharClip` type rows with `(resource ...)` entries matching the
    directory name, de-duplicates `resource[2]`, sorts the symbols, and returns
    the cached result on later calls. Native
    `source_char_bone_dir_get_context_flags_step` preserves that table scan,
    including the source loop's `cfg->Size() - 1` allocation boundary, as a
    deterministic helper only.
  - `CharBoneDir::SyncFilter` clears `mFilterBones` and republishes every
    `CharBone` whose position, scale, or non-`TYPE_END` rotation context
    intersects `mFilterContext`. Native `source_char_bone_dir_sync_filter`
    ports that selection rule for decoded output-bone rows without installing a
    live editor filter list.
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
    `.roty`, and `.rotz`.
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
  - `SetVer` is the separate legacy source gate and asserts `ver < 13`.
    Native `source_char_bones_samples_set_ver_known` records that older
    pre-load boundary separately from the serialized `Load` range.
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
  - Native `source_char_clip_get_context` ports the concrete `GetContext`
    fallback: a type definition with a `resource` array returns the resource
    macro context value; missing type/resource data returns zero.
  - Native `source_char_clip_get_resource` ports the concrete `GetResource`
    lookup shape: inspect the type definition's `resource` array, request the
    named `CharBoneDir` resource, and warn when no resource is resolved. It
    records the lookup decision only; it does not load or synthesize resources.
  - Native `source_char_clip_transitions_*` helpers port the concrete
    `Transitions` constructor, `Size`, and `Clear` bodies: the node range starts
    empty with an owner, `Size` counts transition node-vector entries, and
    `Clear` releases one clip per entry before resizing the range to zero. This
    does not claim `Resize`, `RemoveNodes`, or transition graph evaluation.
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
- Project override: hair polygons/textures render two-sided. Native therefore
  forces no backface culling for shared hair-token mesh/material/texture
  surfaces. This is a visual policy override, not inferred source evidence, and
  is implemented as back-side then front-side cull passes. It must not affect
  source blend, depth write, alpha test, texture wrap, material color, or draw
  sort, and it must not be used to invent hair blend/depth/alpha/sort behavior.
- If a behavior is not proven by ihatecompvir source or stock asset data, leave
  it decoded/logged and unwritten until the source-backed runtime path is known.
