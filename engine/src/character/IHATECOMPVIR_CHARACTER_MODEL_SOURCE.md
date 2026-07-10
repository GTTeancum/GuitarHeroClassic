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
| Object and property-tree skip/read shape | `Object.cs`, `DTBNode.Read` | Parser authority; native skips must mirror source enum. |
| Transformable local/world composition | `RndTrans.cs`, `Trans.cpp`, `Trans.h` | Runtime authority for parent/constraint world rows. |
| Group membership and LOD selection | `RndGroup.cs` | Runtime/draw membership must use decoded object rows. |
| Mesh palette, offsets, and group sections | `RndMesh.cs`, `Mesh.cpp` | Parser and skinning authority; no palette reshaping. |
| Material render state | `RndMat.cs`, `Mat.cpp`, `Mat.h` | Blend, z write, alpha, wrap, and draw order come from source rows. |
| Rnd utility animation rows | `rb3-latest` `AnimFilter.cpp` / `Anim.cpp` | Decode/log stock `AnimFilter` rows; no trigger or animation runtime hookup. |
| Event trigger row inventory | `rb3-latest` `EventTrigger.*`, `ObjVector.h`, `ObjPtr_p.h`, `BinStream.*` | Decode/log stock source fields only; trigger scheduling and the GH2 v8 four-byte zero tail remain fenced. |
| Remaining stock object rows | `rb3-latest` `Tex.*`, RB2 dump `CharWalk.cpp` / `OutfitLoader.cpp`, `DirLoader` `WorldFx` fixup refs | Fenced unless the exact source load path is present. |
| Hair row decode and simulation boundary | `glTFMilo` hair builder, `rb3-latest` `CharHair.*` / `CharCollide.*`, `band3_recomp` symbols | Decode/log source rows; no runtime writeback until `Hookup(ObjPtrList<CharCollide>&)` and simulation are faithfully ported. |
| Eyes/look-at controllers | `CharEyes.cpp`, `CharLookAt.cpp` | Decode/log old GH2 rows; no synthetic eye runtime bridge. |
| Position constraints | `rb3-latest` `CharPosConstraint.cpp` / `CharPosConstraint.h` | Decode/log source, targets, and box rows; runtime `Poll` remains fenced until source transform writeback is ported. |
| Hand IK | `CharIKHand.cpp` | Native hand IK follows source dataflow. |
| MIDI clip drivers | `rb3-latest` `CharDriverMidi.cpp` / `CharDriverMidi.h`; `CharWeightable.cpp`; `ObjPtr_p.h` | Decode/log source revision, inherited weight owner, default clip pointer, parser rows, and blend override gates. Runtime clip selection remains source-fenced. |
| Weight setters and weight owners | `rb3-latest` `CharWeightable.cpp` / `CharWeightSetter.cpp` | Decode/log source weight rows; full setter `Poll` remains fenced to source driver/evaluate path. |
| Rod IK/accessory rods | `rb3-latest` `CharIKRod.cpp` / `CharIKRod.h` | Decode/log source rows; do not synthesize missing destination transforms. |
| Upper/fore twist | `CharUpperTwist.cpp`, `CharForeTwist.cpp` | Native twist passes follow source `Poll` routines. |
| Servo bone driver target | `rb3-latest` `CharServoBone.cpp` / `CharServoBone.h` | Decode/log the `bone.servo` row and `clip_type`; movement remains fenced by clip/CharBones source. |
| Clip sample/output publishing | `rb3-latest` `CharClip` / `CharBones` / `CharBonesSamples`, `rb3-retail-old` RB2 dump, `band3_recomp` symbols | Layout and call-flow evidence exists; native math/application is still fenced where source bodies are incomplete. |
| Hair two-sided rendering | User/project visual override | Two cull passes only; not source evidence for material/depth/sort changes. |

## Binary Layout Authorities

- `MiloEditor/MiloLib/Assets/Object.cs`
  - `ObjectFields.Read` reads a combined object revision, subtype `Symbol`,
    root DTB parent tree, and an optional note `Symbol` when object revision is
    greater than zero.
  - `DTBNode.Read` defines the property-tree node payloads. The native readers
    only skip these trees, but the skip table must mirror this enum exactly.
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
  - `CharHair::Poll` re-runs `Hookup()` while the owning `Character` is syncing,
    resets after teleports, skips simulation for higher LODs, then calls
    `DoReset`, `SimulateLoops`, or `SimulateZeroTime` depending on runtime state.
  - `CharHair::DoReset` seeds each point from `unk5c` transformed by the root
    parent world row, then calls `SimulateLoops(reset, GetFPS())` with inertia
    and friction temporarily forced to zero.
  - `CharHair::Strand::SetRoot` builds the strand from the root transform's
    first-child chain, caches the root base matrix, assigns each point's bone,
    copies child `LocalXfm().v.y` into point length, and seeds point positions
    from source world rows.
  - `CharHair::SetCloth` assigns `sideLength` from the matching point in the
    next strand when cloth mode is enabled and otherwise forces `sideLength` to
    `-1.0f`.
  - `CharHair::SimulateInternal` only calls `SetWorldXfm` for a point inside the
    `thisPoint.collides.size() != 0` branch. Native GHOGX must not invent a
    partial hair physics bridge from decoded point rows alone.
  - The latest source includes `CharHair.h`, `CharCollide.h`, default
    `CharHair::Hookup()` gathering all `CharCollide` objects from the object
    directory, and the `CharCollide` shape/radius header plus load path.
    However, the overloaded `Hookup(ObjPtrList<CharCollide>&)` body is still
    declared but not implemented in the checked source. Native GHOGX therefore
    keeps decoded hair rows logged and unwritten until that hookup filter and
    the simulation path are ported from source, not guessed.
- `rb3-latest/src/system/char/CharCollide.cpp` and
  `rb3-latest/src/system/char/CharCollide.h`
  - `CharCollide::Load` reads `Hmx::Object`, `RndTransformable`, shape,
    radius/length/flags rows, optional current radius, optional second
    radius/length rows, an internal transform, mesh pointer, eight mesh sphere
    rows, SHA1 digest, and mesh-y-bias.
  - Native GHOGX decodes and logs `CharCollide` rows using this source order so
    hair hookup/collision work can be audited from stock data. It does not yet
    apply collision or write hair world rows from these decoded objects.
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
- `rb3/src/system/char/CharEyes.cpp`
  - Modern revisions read `EyeDesc` rows, but GH2-era revision 3 uses the older
    branch: `CharEyes::Load` reads an `ObjPtrList<CharLookAt>` and converts each
    reference into an eye entry. For revisions 3 and 4 it then consumes one old
    `RndTransformable` pointer. Native GHOGX decodes the GH2 row as that
    look-at reference list plus trailing old transformable, not as hidden eye
    offsets.
  - `CharEyes::ListPollChildren` delegates poll children to the referenced
    `CharLookAt` controllers. It is not evidence for a native bridge that copies
    eye mesh world rows into ad-hoc controller overrides.
- Native GHOGX therefore decodes `CharEyes`/`CharLookAt` rows for inspection but
  does not publish synthetic eye runtime rows until a direct source-backed poll
  port is implemented.

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
  - Native GHOGX currently decodes and logs these rows only. It does not publish
    constraint world writes from this class until that source transform
    writeback path is integrated without disturbing unrelated character pose
    controllers.

## IK Controller Authorities

- `rb3-latest/src/system/char/CharWeightable.cpp` and
  `rb3-latest/src/system/char/CharWeightable.h`
  - `CharWeightable::Load` reads a source revision, `mWeight`, and
    `mWeightOwner` when the revision is greater than 1.
  - `CharWeightable::Weight()` returns the owner row's weight, not merely the
    object's local serialized value. Native therefore keeps `weight_owner` as a
    named source row instead of treating it as a generic UI property.
- `rb3-latest/src/system/char/CharWeightSetter.cpp` and
  `rb3-latest/src/system/char/CharWeightSetter.h`
  - `CharWeightSetter::Load` reads `Hmx::Object`, then `CharWeightable` for
    revisions above 1, followed by `driver`, `flags`, revision-gated
    `offset`/`scale`, old owner lists for revisions below 2, `base_weight` and
    `beats_per_weight` above revision 4, optional `base` above revision 5, and
    min/max setter refs through the revision 7/8 single-pointer rows or the
    revision 9 lists.
  - `CharWeightSetter::Poll` derives `base_weight` from either
    `mDriver->EvaluateFlags(mFlags)` or `mBase->Weight()`, applies
    `scale`/`offset`, clamps through min/max setter rows, and then either snaps
    or beat-smooths `mWeight`.
  - Native GHOGX decodes/logs these source fields. Full `Poll` behavior is not
  reimplemented as a visual shortcut; the active performer path may consume
  explicit live song/MIDI weights and use decoded setter weights only as the
  bounded fallback the current hand path already had.
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
  - `rb3-latest/src/system/obj/ObjPtr_p.h` proves `mDefaultClip.Load` reads one
    `0x80`-bounded source string. Native GHOGX therefore decodes/logs that slot
    as `midiDefaultClip` for revision-below-7 rows before applying the
    parser/flag/blend gates.
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
    those source-declared fields so the active asset data can prove which
    branches are actually present.
  - Native GHOGX must not retain the older opt-in free two-bone arm solver or
    its `GHOGX_ENABLE_ARM_IK`/stretch/rotation gates. Any hand or elbow solve
    must be translated from the source-backed `CharIKHand` dataflow above.
  - The native CharIKHand pass now runs from the decoded controller order and is
    not wrapped in the older hand-IK A/B switches, name-based fret/strum
    reordering, or the hand-local `.pos` escape hatch. Hand `.pos` rows stay out
    of local FK for real hand bones; the live hand reaches its target through
    the CharIKHand world-row write after clip sampling.
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
- `rb3/src/system/char/CharForeTwist.cpp`
  - `CharForeTwist::Load` reads `offset`, `hand`, `twist2`, an old revision-2
    dummy int, and `bias` for revisions above 3.
  - `CharForeTwist::Poll` derives the twist angle from the hand world Z row and
    the hand parent world X/Y rows, applies authored `offset` and `bias`, writes
    the `twist2` parent transform, then interpolates toward the hand position
    using `twist2.local.x / hand.local.x` and writes `twist2` itself.
  - `CharIKHand::Poll` does not inline or consume `CharForeTwist` rows. Native
    GHOGX therefore runs decoded `CharForeTwist` controllers as their own source
    poll pass after hand IK instead of marking them handled inside the hand IK
    bridge.
  - Native GHOGX does not keep approximate or PS2-row twist writers in the
    runtime path. The standalone twist controller path follows these
    ihatecompvir source `Poll` routines.

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
  - Native GHOGX decodes and logs the source `CharServoBone` row and
    `clip_type`. It does not port `MoveToFacing`, `MoveToDeltaFacing`, or
    broad `CharBonesMeshes` movement until the connected clip/bone source path
    is implemented as a whole.
- `rb3-latest/src/system/char` exposes source files for `CharClip`,
  `CharClipDriver`, `CharDriver`, `CharBones`, `CharBonesSamples`,
  `CharBonesMeshes`, and related clip runtime classes. The previous local note
  that public ihatecompvir source had no clip/sample layer is obsolete.
- `rb3-retail-old/doc/rb2_dump/rockband2/system/src/char` exposes RB2-era dump
  entries for `CharClipSamples`, `CharBonesSamples`, `CharClip`,
  `CharClipDriver`, and `CharDriver`. These files are useful source-backed
  function maps: they identify `FrameToSample`, `ScaleAdd`, `RotateBy`,
  `RotateTo`, `FacingSet`, `CharClipDriver::Evaluate`, and the driver-to-bone
  application flow.
- The checked source is still incomplete for the exact math bodies needed to
  blindly replace native clip playback: `CharBonesSamples::LoadHeader`,
  `LoadData`, `EvaluateChannel`, `CharClip::ScaleAdd`, and
  `CharClipDriver::Evaluate` are declared or function-mapped, but not fully
  implemented as reviewable C++ bodies in the current public source.
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
- Four base characters have no decoded `CharHair` rows in this stock set:
  `metal_bass`, `metal_drummer`, `metal_keyboard`, and `metal_singer`. The
  other 20 base character MILOs expose 31 decoded `CharHair` rows total.
- The focused refreshed controller inventory at
  `engine/out/source_truth_controller_inventory_20260710/expanded_stock_characters_controller_posconstraint_inventory.log`
  shows five `CharPosConstraint` rows total: one each in `female_singer`,
  `grim`, `metal_bass`, `metal_keyboard`, and `metal_singer`. All are revision
  2. `female_singer` and `metal_singer` target `shadow.mesh`; `metal_bass` and
  `metal_keyboard` have zero targets; Grim's `hems.pcon` names `source=grim`
  and has zero targets. Native logs those rows as source data and keeps runtime
  `Poll` writeback fenced.
- The focused refreshed controller inventory at
  `engine/out/source_truth_controller_inventory_20260710/expanded_stock_characters_controller_driver_midi_inventory.log`
  shows 38 `CharDriverMidi` rows across the stock guitarist set. Every row is
  `midiVersion=3` with `midiDefaultClip=<none>`, `midiUnreadBytes=0`,
  `midiParser=<none>`, `midiFlagParser=<none>`, and
  `midiBlendOverridePct=1.0000`. Under the ihatecompvir `CharDriverMidi::Load`
  gates, GH2 stock rows are before the parser/flag/blend fields and their
  revision-below-7 default clip slot is the source-backed empty `ObjPtr` string.
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
- `EventTrigger`: one stock row, on `metal_drummer`. Native now decodes and
  logs the source-backed field prefix using `EventTrigger::Load`,
  `ObjVector`, `ObjPtrList`, and `BinStream` evidence. It still does not run
  trigger scheduling, and the GH2 revision-8 four-byte zero tail remains logged
  as unresolved source evidence rather than consumed by guesswork.
- `Object`: 19 stock generic object rows. ihatecompvir `Object.cs` already
  backs the shared object/property-tree skip path; the remaining rows have no
  character-model runtime behavior to promote.
- `Tex`: 160 stock texture rows. `rb3-latest` `RndTex::Load`/`PreLoad`/
  `PostLoad` is source-backed, but native texture payloads are already handled
  by the PS2 texture asset path (`asset/milo_image.*`) keyed from material
  diffuse texture names. These rows are not promoted into the character
  controller graph.
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
