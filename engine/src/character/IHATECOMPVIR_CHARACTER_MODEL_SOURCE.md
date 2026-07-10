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

## Skinning Authority

- `glTFMilo/Source/glTFMilo/Program.cs`
  - Mesh bone transforms are generated per chunk joint, in vertex palette order.
  - The transform written for a palette bone is:
    `inverse(jointNode.WorldMatrix) * node.WorldMatrix`.
  - The native render path therefore consumes the stored transform as a source
    offset row, then composes it with the current source transform for the named
    bone. It must not replace this with a guessed inverse-bind fallback.

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

## Native Rules

- Shared parser fixes are allowed when they follow the source files above.
- Character-name fixes are not source evidence.
- If a behavior is not proven by ihatecompvir source or stock asset data, leave
  it decoded/logged and unwritten until the source-backed runtime path is known.
