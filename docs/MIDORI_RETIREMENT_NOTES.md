# Midori conversion retirement notes

## Final verdict

This experiment is retired after a hard rejection of its final
GuitarHeroClassic gameplay proof. The visible failures are:

- both legs float above the stage;
- the arms are stretched;
- the overall playing pose is unnatural; and
- prior visual assessments that described the result as coherent were wrong.

The objective was not achieved. The archived package must not be presented as
a working Midori conversion or used to authorize a retail GH2 overlay.

## Preserved state

The retirement archive lives on branch
`archive/midori-conversion-pre-5.7`. It starts from the existing Midori commits
on `main-ui-engine` and adds the rejected proof, reports, complete handoff, and
these notes without mixing in unrelated working-tree changes.

The final rejected screenshot is:

`midori_casey_big_medium04_s143_f30_yawm040_wristweights_gameplay.bmp`

Its SHA-256 is
`60EFC436916BB2609D2B42E09EC433AB4E1D7B8DCC8E24FCBB4775C05753072D`.
The bound runtime log SHA-256 is
`4A809F2E06B21A0861168A0D9742DF2FECAA23C3D385C26AFBA0B93E28A54D4F`.

The package at retirement contains 45 active Midori meshes, 169 total meshes,
115 Casey transforms, and 20 Casey controllers. It covers all 157 Casey clip
calls and all 30 stock groups. The packaged model SHA-256 is
`C8B7BE6DEF202AB60B0E71563924B11C687DD8C947A7535436E19D81B8CEA286`;
the main animation bank SHA-256 is
`F7B202330E9233378BA55C896845DEEF1923C885174B7C0CF49097C115C17002`.

These facts prove reproducibility and runtime call coverage. They do not prove
correct skinning, grounding, retargeting, or natural motion.

## Retail state

The five authenticated payloads were staged at Casey's projected retail paths,
and the source archive was audited read-only. The retail apply gate was never
opened. No copied retail ARK was produced, no ISO was built or mounted, and no
emulator was used. A rejected proof is deliberately ineligible for retail
apply.

The final focused contract suite passes 17 of 17 tests. A real rejected-proof
apply attempt failed on `human_visual_acceptance`; the output `MAIN.HDR` and
`MAIN_0.ARK` were absent both before and after that check.

## Restart point

Treat all earlier claims that the lower body or torso were solved as
unproven. Resume from data, not from the last screenshot:

1. Diagnose pelvis/root grounding and lower-body world transforms across
   multiple animation frames. Quantify foot contact from posed geometry.
2. Trace the arm stretch through bind matrices, skin weights, and IK/controller
   transforms from clavicle to fingers. Compare both rigs in GLB or equivalent
   structured form and validate numerically before rendering.
3. Establish natural torso, neck, and head orientation over multiple clips.
4. Keep the face static, use outfit 1 only, and defer guitar attachment until
   pelvis-through-head and both arms are coherent.
5. Revisit animation naming and DTB call conventions before any retail test.

Use GuitarHeroClassic for runtime proofs. Keep CPU priority at Idle with one
worker, generate at most one screenshot per review turn, and do not use an ISO,
mounted disc image, PCSX2, or commercial source assets at runtime.

## Archived evidence

The `proofs/midori-conversion-retired` directory contains the rejected image,
its runtime log and proof record, the retail staging reports, and package and
animation-call validation reports. The complete final handoff is preserved at
`docs/MIDORI_HIERARCHY_HANDOFF_ARCHIVE.md`. Commercial GH2/GH3 source archives
and intermediate MILO payloads are intentionally not duplicated here.
