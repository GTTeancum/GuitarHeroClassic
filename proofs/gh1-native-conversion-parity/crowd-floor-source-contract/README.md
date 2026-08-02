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

See `runtime-matrix.tsv` for the per-venue ledger.

## Transparency result

Seven of eight GH1 MultiMesh-bearing packages contain an authored binary-alpha
crowd image. Theatre's source image is fully opaque despite having the same
RGB pixels. Conversion preserves both cases exactly; no black-key or synthetic
alpha is applied. See `crowd-alpha-source.tsv`.

## Screens

The `screens/` directory contains one fixed-time PNG for every tested venue.
The seven `gh1_*.png` files were captured after the final cross-directory
`crowd.env` binding. Native GH2 has no GH1 MultiMeshes, so the five-line
environment propagation correction cannot change its `WorldCrowd6` path.

These frames are runtime smoke proofs, not matched retail camera/time
comparisons. Opening lighting can intentionally leave a crowd dark, and a
camera may not face the audience in a particular frame.

## Deployment

`deployment-hashes.tsv` records the active executable, packed archive, header,
and the eight converted crowd-bearing MILOs read back from the active archive.

## Tests

`test-summary.txt` records the focused test and audit results. Full logs and
bulk conversion trees were intentionally not retained.
