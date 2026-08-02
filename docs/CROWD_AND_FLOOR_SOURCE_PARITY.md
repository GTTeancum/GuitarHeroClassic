# Crowd and Crowd-Floor Source Parity

## Scope and result

This work closes the crowd and crowd-floor rendering subcase for all seven
converted Guitar Hero 1 venues and all eight native Guitar Hero 2 venues. It
does not claim full matched venue-presentation parity.

The implementation now has two deliberately separate source paths:

1. GH1 `MultiMesh0` crowd cards are decoded, owned, transformed, shaded, and
   submitted as authored.
2. GH2 `WorldCrowd6` actors and placement meshes retain the native GH2 path.

No venue, mesh, material, or texture name is used to invent visibility,
flooring, alpha, camera-facing behavior, or placement.

## GH1 retail contract

### MultiMesh drawing

All 41 `MultiMesh0` objects in the packed GH1 corpus are crowd-card
archetypes. A `MultiMesh0` contains a Drawable block, a template Mesh
reference, and an ordered array of serialized instance transforms.

The target renderer follows `RndMultiMesh::DrawShowing`: for each source
instance it installs the serialized transform on the referenced template Mesh
and draws that Mesh. The template is not also submitted once at its root
transform. No synthetic camera-facing transform, foreground cull, or guessed
bounds test is applied.

GH1 `Arena::Crowd` owns these MultiMeshes independently of the current View
drawable list. The converter therefore emits a deterministic native owner
Group named `__gh1_runtime_multimeshes.grp`. That Group keeps source order and
contains the MultiMesh references; it is not a venue-specific visibility
exception.

### Crowd region

Retail GH1's `crowd_region` selects a subset of the original MultiMesh
instances. Construction classifies each authored instance against the
`crowd_limitsNN.mesh` volumes; changing region copies accepted original
instances into an active list. It does not modify transforms, materials, or
animation.

The target keeps the complete serialized instance arrays and lets ordinary
frustum and depth behavior reject unseen cards. This preserves all source
content without reproducing a redundant retail draw-list optimization.

### Environment ownership

GH1 retail stores `arena::crowd.env` at `Arena + 0x9C`. Static evidence is
`SLUS_212.24` `0x001685DC..0x0016862C`; the pointer is subsequently read from
that field by the crowd path.

Converted crowd packages cannot serialize an unresolved cross-directory
ObjectPtr to an environment held by another venue section. After all native
venue sections have merged, runtime resolves the decoded `crowd.env` and
assigns it to `__gh1_runtime_multimeshes.grp`. Normal Group traversal then
propagates that environment to each MultiMesh template Mesh. This is a
source-ownership bridge, not a brightness correction.

### Texture alpha

The eight GH1 packages containing MultiMeshes were audited from the source
MILO and again after conversion.

- Seven packages use the same 128x256 indexed crowd image. It decodes to
  25,220 transparent pixels and 7,548 opaque pixels, with no partial-alpha
  pixels. Source and converted bitmap payloads match.
- Theatre's same-sized `crowd01.tex` has the same RGB pixels but its source
  CLUT marks all 32,768 pixels opaque. Its source `crowd.mat` uses SrcAlpha,
  `useEnviron=true`, `cull=true`, `zMode=1`, `alphaCut=false`, and
  `alphaWrite=false`. The converted bitmap digest remains identical to the
  source digest.

Consequently, seven crowd-card sets become silhouettes through authored
texture alpha. Theatre remains opaque because that is what its source package
encodes. There is no black-key, texture-name rule, or fabricated alpha.

## GH2 retail contract

Native GH2 uses `WorldCrowd6`, not GH1 MultiMesh crowds. The decoded
WorldCrowd owns character billboards, an authored placement Mesh, count, and
environment.

`WorldCrowd::BuildBillboard` constructs the four-vertex character billboard.
The placement Mesh supplies positions; it is not a visible floor surface.
`WorldCrowd::CleanUpCrowdFloor` discards placement-mesh vertex data outside
the editor after crowd construction. The compatibility runtime therefore
never renders those placement meshes as flooring and never substitutes a
guessed `tile_dark.mat` or `street_asphalt.mat`.

Venue floors remain ordinary authored venue Mesh objects. The previous
diagnostic path that manufactured visible floor geometry from crowd
placements has been removed.

## Implementation points

- `milo_scene` decodes exact `MultiMesh0` template references and transforms.
- `milo_convert` emits the deterministic GH1 runtime owner Group and prevents
  its MultiMeshes or template Meshes from being quarantined as unreachable.
- `gameplay` merges MultiMesh objects across venue sections, preserves exact
  Group references, binds the decoded GH1 `crowd.env` after the merge, and
  leaves GH2 WorldCrowd placement meshes non-draw.
- `milo_scene_renderer` expands source MultiMesh transforms in authored order
  and propagates Group environment state to their template Meshes.
- Contract tests reject the removed crowd-name heuristic, synthetic culls,
  fake floor material substitutions, and dangling environment ownership.

## Validation

The input-free runtime matrix covers 15 venues:

- GH1: 35 MultiMesh objects, 2,265 authored instances, 2,265 submitted, zero
  missing template Meshes, and one source environment bind in every venue.
- GH2: 49 WorldCrowd actors and 4,080 decoded placements, with no GH1
  MultiMesh path used.
- Every run reaches `playing`, exits zero, and reports zero decode errors.
- No run reports a manufactured visible crowd-floor row.

The full converter audit covers 105 assets with 105 complete, zero incomplete,
13,115 converted objects, 815 source-derived synthesized objects, zero
blocked objects, and 13,930 emitted semantic objects.

Focused `milo_convert_test` and `ghogx_milo_scene_test` pass. The broad
gameplay contract runner retains 64 unrelated stale baseline failures, while
all new crowd source-contract assertions pass.

The compact evidence set is in
`proofs/gh1-native-conversion-parity/crowd-floor-source-contract/`.
