# Penelope face and Duke lenses — September 16, 2026

The lighting fix was committed and pushed as 9a0c1607. This follow-up repairs
Penelope's facial pose and applies the user's Duke-only opaque-lens preference.

FaceFX composed Casey's absolute neutral and expression data and published it
onto Penelope's differently proportioned skeleton. Gameplay now retargets the
finished typed pose using the existing source/target bind snapshots. It does
not rebase the individual delta poses. Stock characters retain their existing
path. The regression checks changed proportions, expression motion, neutral
return, and unchanged stock behavior. The eye bridge contract also passes.

- [Face before](penelope-before.bmp) and [face repaired](penelope-face-fixed.bmp):
  matching native frame 850, hair temporarily suppressed to expose the face.
- [Normal appearance](penelope-normal.bmp): same corrected face with authored hair.
- [Duke opaque lenses](duke-opaque.bmp): eyes no longer show through the lenses.
- [Installed Quickplay](installed-quickplay.bmp): actual saved-band copy, natural
  venue lighting; original save bytes and timestamp unchanged.

Original RB2 goggles use alpha blending with material alpha 0.8. The user
explicitly chose opaque lenses for Duke only. The installed Duke repair changes
one decompressed material byte (blend 3 to 1); no other character's glasses or
renderer-wide transparency changes. His recipe sets opaque blend and generated
texture alpha 255. Source evidence and model hashes are in the adjacent JSONs.

Individually reviewed three corrected facial frames, three normal-hair frames,
matching before/after close-ups, actual Quickplay and the Manage Band preview.
Both native tests, build, and all four package validations pass. These are
sampled stills with muted diagnostic autoplay, not every-frame or audio review.
The separate necklace alpha issue remains outside this face repair.
