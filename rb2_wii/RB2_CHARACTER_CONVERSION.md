# Rock Band 2 Wii preset-character conversion

## Scope

The first implemented character is the retail Rock Band 2 Wii prefab
`guitar32`, localized as **The Duke of Gravity**. The proof package temporarily
replaces Clive Winston's native GH2 `classic` outfit through the normal loose
DLC catalog. No converted commercial payload is committed to this repository.

## Source facts

`presets/duke_of_gravity.json` records the decoded prefab composition rather
than visual guesses:

- male guitarist, rocker attitude, height `0.75`, weight `0.5`;
- crazyhawk hair using palette colors 38 and 28;
- head 2, chops color 28, goggles colors 14 and 3;
- pleather strap jacket colors 45 and 12;
- robotic codpiece pants colors 12 and 45;
- bare feet, naked hands, Hercules wrists, and Chief rings.

The recipe names every resource directory, selected mesh expression, diffuse
texture, color mask, palette, palette index, transparency behavior, and output
material namespace. `tools/convert_rb2_preset_character.py` reads the extracted
retail objects and carries geometry, weights, bind matrices, transforms,
textures, and render flags into a version-9 mesh bundle.

## Reproducible pipeline

1. Extract only the recipe's required resource MILOs from the user's RB2 Wii
   content and unpack them with `milo_tool extract` into one component root.
2. Run `convert_rb2_preset_character.py` with the component root, recipe, a
   target-transform root when the recipe declares target-only attachment
   transforms, and paths for the mesh bundle and JSON audit.
3. Run `milo_convert_tool build-character-from-meshbundle` to create a
   source-rigged `BandCharacter` donor. Supply the intended GH2 main, strum,
   and fret animation banks so the generated directory contains the standard
   guitarist controller graph.
4. Run `milo_convert_tool merge-character-render-payload` with a decoded,
   generated GH2-compatible template, the donor, `--rebind-template-rig`, and
   `--preserve-donor-hand-mesh-bind-offsets`. This keeps one target skeleton,
   rebases body meshes, and retains the source bind data for meshes connected
   to the finger/hand chains.
5. Package the merged MILO as loose DLC. A manifest may temporarily replace a
   native outfit only when `selection` and `replaces_selection` are identical,
   the character still owns that native outfit, and `replaces` explicitly
   names the base model path.

The raw retail Clive MILO contains opaque retail object revisions that the
standalone `milo_object` merger does not completely decode. The accepted Duke
proof therefore uses the already-generated, source-audited compatible
`tools/milo_convert/gh1-character-models/classic.milo_ps2` rig as the merge
template, then loads Clive's native GH2 animation banks at runtime. This is a
format constraint, not a hand-authored character adjustment.

## Duke audit result

- 95 source render meshes;
- 6,023 vertices and 7,135 faces;
- 14 source-derived materials/textures;
- 345 resolved donor bone slots;
- 235 body slots rebased to the compatible target bind graph;
- 110 hand-connected slots preserving donor bind offsets across 30 meshes;
- 130 bone references mapped to factual ancestors in the target rig;
- maximum bind reconstruction residual `0.00000762939`.

The native proof reports 95 showing meshes, 63 target bones, two upper-arm
twists, two forearm twists, two IK hands, one IK MIDI, three drivers, and two
weight setters. Clive's native GH2 main/strum/fret clips drive the result
without runtime skeleton retargeting.

## Known boundary

RB2's continuous character-creator height and weight deformation channels are
not decoded or baked yet. The current conversion preserves the retail prefab
parts and source bind data but does not claim source-exact body-shape or RB2
facial-animation parity. It is a successful gameplay/rig feasibility result,
not the final general-purpose character-creator baker.
