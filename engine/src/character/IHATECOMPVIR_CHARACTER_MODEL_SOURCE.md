# GH2 Character Model Source Map

This file is the source-backed map for the native GH2 PS2 character model path.
Treat ihatecompvir's repos as the authority; do not use the older
`CHARACTER_FORMAT_NOTES.md` as proof when these sources disagree.

## Source Snapshot

- `ihatecompvir-public-milo-sources/MiloEditor` at `3ebffb1c4391dd83c5765cb428eef433dffaff51`
- `ihatecompvir-public-milo-sources/glTFMilo` at `3c02a5497ede1a5d61023fb066cc8bfbe2e8a8e4`
- `ihatecompvir-public-milo-sources/rb3` at `41719f248995f677ffa39bd394706b5d18ef70c6`
- `ihatecompvir-public-milo-sources/re-notes` at `5c486fd6e5e5186c0797df9c84182b056672b3f0`
- `ihatecompvir-public-milo-sources/re-gh2` at `2aa28d67f7da4d41ae4e3f18129b49b51ffee2fd`

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
    from mesh or material names such as `hair`.
- `rb3/src/system/rndobj/Mat.h`
  - `RndMat` exposes source `GetBlend`, `GetZMode`, and `GetTexWrap` accessors.

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
- `rb3/src/system/char/CharHair.cpp`
  - `operator>>(BinStream&, CharHair::Point&)` is the runtime read authority for
    point fields. For revisions 6, 7, and 8 the extra float is added to both
    `radius` and `outerRadius`. For revisions below 8, `sideLength` is forced to
    `-1.0f`; revisions above 5 consume two ints.
  - `CharHair::SimulateInternal` only writes driven point transforms through the
    runtime collision/hookup path. Native GHOGX must not invent a partial hair
    physics bridge from decoded point rows alone.

## Face Controller Authorities

- `rb3/src/system/char/CharLookAt.cpp`
  - `CharLookAt::Poll` is the available Harmonix source for eye/look-at runtime
    motion. It reads source/pivot/destination transformables and writes the
    pivot through `SetWorldXfm`; it does not synthesize a head-forward source
    row when a GH2 row names the `CharLookAt` object itself.
- `rb3/src/system/char/CharEyes.cpp`
  - `CharEyes` owns `EyeDesc` rows and delegates poll children to the referenced
    `CharLookAt` controllers. It is not evidence for a native bridge that copies
    eye mesh world rows into ad-hoc controller overrides.
- Native GHOGX therefore decodes `CharEyes`/`CharLookAt` rows for inspection but
  does not publish synthetic eye runtime rows until a direct source-backed poll
  port is implemented.

## IK Controller Authorities

- `rb3/src/system/char/CharIKHand.cpp`
  - `CharIKHand::Poll` is the available Harmonix source for hand IK runtime
    motion. It resolves `mHand`, optional `mFinger`, and the target list,
    blends the world destination into `mWorldDst`, calls `IKElbow` when an
    elbow chain is present, and writes the hand through `SetWorldXfm`.
  - Native GHOGX must not retain the older opt-in free two-bone arm solver or
    its `GHOGX_ENABLE_ARM_IK`/stretch/rotation gates. Any hand or elbow solve
    must be translated from the source-backed `CharIKHand` dataflow above.

## Stock GH2 Evidence

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

## Native Rules

- Shared parser fixes are allowed when they follow the source files above.
- Character-name fixes are not source evidence.
- Renderer geometry selection must come from decoded source membership such as
  `RndGroup.objects`; name/palette suppressions like numbered-hair or terminal
  leg-overlay hiding are not source evidence.
- Renderer state such as blend, z write, alpha test, cull, wrap, and draw order
  must come from source material/drawable rows. Hair-name render branches are
  not source evidence.
- If a behavior is not proven by ihatecompvir source or stock asset data, leave
  it decoded/logged and unwritten until the source-backed runtime path is known.
