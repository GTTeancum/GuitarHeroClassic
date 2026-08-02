# Crowd and Crowd-Floor Source Contract Proof

This proof set validates the source-shaped crowd and floor implementation for
all seven converted GH1 venues and all eight native GH2 venues.

## Result

- GH1: 35 `MultiMesh0` objects, 2,265 source transforms, 2,265 submissions,
  zero missing template Meshes.
- GH2: 49 `WorldCrowd6` actors, 4,080 source placements.
- All 15 runs reach `playing`, exit zero, and report zero decode errors.
- Every GH1 venue resolves and binds its decoded `crowd.env` exactly once.
- Zero runs use the removed manufactured crowd-floor path.
- All 15 replacement screenshots visibly frame the crowd. The proof camera is
  diagnostic-only and derives its target and extent from decoded MultiMesh
  instances or WorldCrowd placements.
- All 49 GH2 actors change deterministic sampled-pose digests across the
  captured interval. Every actor uses the source `great` group selected by the
  source `excitement_peak` event, with at least 19 decoded channels.

See `runtime-matrix.tsv` for the per-venue ledger.

## Transparency result

Seven of eight GH1 MultiMesh-bearing packages contain an authored binary-alpha
crowd image. Theatre's source image is fully opaque despite having the same
RGB pixels. Conversion preserves both cases exactly; no black-key or synthetic
alpha is applied. See `crowd-alpha-source.tsv`.

## Screens

The `screens/` directory contains one full-resolution PNG for every tested
venue. Every frame explicitly exposes the crowd; these replace the earlier
smoke frames whose retail camera happened not to face the audience. The seven
`gh1_*.png` files were captured after the final cross-directory `crowd.env`
binding.

These are source-rendering proofs, not matched retail camera/time comparisons.
The camera can sit outside authored venue bounds, but it never changes crowd
content, lighting, alpha, transforms, fullness, or animation.

## Animation

Native GH2 builds live impostor textures from decoded character clips. The
renderer had been issuing every draw while rejecting every billboard face:
the D3D bridge used `D3DCULL_CCW` for retail's `(0,1,2)` source face even
though the normal character path and mirrored projection require
`D3DCULL_CW`. The correction changes only the cull mapping; no venue,
placement, density, material, texture, or lighting exception was introduced.

`gh2-worldcrowd-animation.tsv` records all eight runs. The runtime diagnostic
hashes every sampled source pose channel at 0.25-second intervals, records the
source clip and MILO path, and proves all 49 actors change. The compact
five-frame visuals are in `animation/`.
`gh2-worldcrowd-animation-samples.tsv` preserves the first and last source
beat/digest for every actor.

GH1 venue cards are different: `MultiMesh0` has no Animatable superclass and
the audited venue objects contain zero animation references to their card
Meshes or MultiMeshes. Their card pose is therefore source-static. The GH1
corpus also contains global crowd character clips, but the venue cards do not
reference them; attaching those clips would be fabricated. Crowd-associated
venue `LightAnim` rows remain decoded through the normal source animation
path. `gh1-crowd-animation-source.tsv` records their exact targets and key
counts. Theatre's three crowd light tracks are source-constant one-key rows.

## Deployment

`deployment-hashes.tsv` records the active executable, packed archive, header,
and the eight converted crowd-bearing MILOs read back from the active archive.

## Tests

`test-summary.txt` records the focused test and audit results. Full logs and
bulk conversion trees were intentionally not retained.
