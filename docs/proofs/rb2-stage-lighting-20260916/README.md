# Penelope and Duke stage lighting — September 16, 2026

Fixed the shared mesh-bundle builder: generated materials now enable venue
lighting instead of forcing unlit diffuse textures. Rebuilt the converter and
verified an actual generated material with the native tools.

Repaired the live and repository DLC packages without rebuilding either rig.
Only 24 decompressed bytes changed for Penelope (12 materials) and 30 for Duke
(15 materials). Already-lit donor materials are untouched. Native MILO verify,
compression round trips, and all four package indexes pass.

Runtime lighting: Penelope 141/141 meshes lit; Duke 95/95 meshes lit. Both
previously rendered with all these meshes unlit. Ten focused tests pass.

Visual evidence, all inspected individually:
- [Penelope before](../quickplay-buckets-20260916/actual-profile/frame_01350.bmp)
- [Penelope repaired, matching frame](penelope-after/frame_01350.bmp)
- [Duke before](duke-before/frame_01350.bmp)
- [Duke repaired, matching frame](duke-after/frame_01350.bmp)
- [Penelope, later light cue](penelope-later/frame_01350.bmp)
- [Duke, later light cue](duke-later/frame_01350.bmp)

These are native renderer captures from the installed game using its normal
Setlists and DLC paths. Penelope uses a copy of the actual saved profile;
Duke uses the same settings with only character/outfit changed. The original
save is untouched. The later captures seek to 60 seconds instead of 30;
no artificial lights or color adjustments are used. Navigation/autoplay is
process-local, window hidden, audio muted. No desktop automation or capture.

Both now show red stage illumination, yellow/green highlights, and shadowed
surfaces. This verifies the material-lighting defect; existing facial/rig
issues and exhaustive lighting/hardware behavior remain outside this repair.
See per-run verification JSON and the two material-repair reports.

Build/extraction/test scratch was removed. Only source changes, updated
converter/DLC deliverables, profiles for reproducible proofs, and compact
verification evidence remain from this turn.
