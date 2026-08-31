# Rejected Midori conversion evidence

This directory preserves the final evidence for the retired Midori-to-Casey
experiment. The screenshot is a hard failure: both legs float, the arms are
stretched, and the overall pose is unnatural. It is retained as a rejected
artifact, not as a success image.

`midori_casey_wristweights_gameplay.validation.json` binds the screenshot and
runtime log to the packaged model and main animation bank. Runtime and hash
checks pass, but both visual review fields are `rejected`. The retail apply
tool accepts only an explicit `accepted` verdict, so this archive cannot open
the retail gate.

The JSON and TSV reports capture the last reproducible structural state. They
demonstrate call coverage, package identity, and read-only retail staging; they
do not demonstrate correct deformation. No commercial source archive, ISO,
copied retail ARK, emulator state, or intermediate MILO is stored here.

Read `docs/MIDORI_RETIREMENT_NOTES.md` and
`docs/MIDORI_HIERARCHY_HANDOFF_ARCHIVE.md` before resuming.
