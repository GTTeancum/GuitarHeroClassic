# Quickplay buckets — September 16, 2026

Implemented and installed JSON setlists with the hierarchy bucket → section →
songs, with a default venue on each section. The shared setup JSON holds GH1
and GH80s as separate buckets. The installed catalog is GH2 64, GH1 47, GH80s 30.

Native renderer evidence:

- [GH1 bucket](gh1.png): bucket name, Basement section and source song order.
- [Installed GH80s bucket](gh80s-installed.png): reached with two Yellow presses
  using the live executable and its default relative `Setlists`/`DLC` paths.
- [Section navigation](section-header.png): Iron Man is selected beneath Small
  Club; the bucket caption and all five footer actions remain readable.
- [JSON venue override](json-venue-override.png): a temporary JSON change alone
  sends Iron Man to `gh1_big_club`, with live notes, scoring and the GH1 rig.
- [Test preferences](saved-preferences.png): Blue launches the same song with
  an artificial test profile (Arena, Xavier and Flying V). This is not the user's
  saved band. See the actual-profile evidence below.

Green's normal Small Club launch and GH80s gameplay were also visually
inspected. All six native runs exited successfully. Yellow wraps GH2 → GH1 →
GH80s → GH2 and restarts selection at the first song. Native preview logs show
the corresponding Shout at the Devil, I Love Rock & Roll and Metal Health
audio paths and authored preview intervals.

Validation: 17 native data/provider checks and 27 installer unit tests pass;
release executable builds successfully. Installer checks cover the shared
file, both song counts, required venues, repeat installation and preserving
user edits. Setup's frozen build now includes the Setlists resource directory.

Review used selected native stills, not every frame. Diagnostic audio was
muted: routing/playback state was checked, but audible output was not listened
to. Physical Xbox/guitar controls, a fresh full-media installer run and every
song/venue combination are outside this verification. No desktop input or
external capture was used.

[Verification metadata](verification.json) records hashes, selected log
evidence, commands, reviewed frames and limits. Source and setup behavior are
documented in [Quickplay setlists](../../QUICKPLAY_SETLISTS.md).

Actual saved-profile verification (September 16 follow-up):

- Same live installation used in the September 11 menu/crowd milestone:
  `gh2_ps2_hybrid_assets/ghogx_app.exe`, working directory its installation folder.
- [Quickplay](actual-profile/frame_00360.bmp) uses default installed Setlists/DLC
  paths. GH1 Small Club contains Iron Man and the Yellow Setlist action.
- [Actual saved band](actual-profile/frame_01350.bmp) shows Penelope in GH1 Big
  Club after Blue/My Band. The native handoff records saved Penelope/default
  outfit, Jaguar03/Wood Maple selection and gh1_big_club. Guitar mesh/skin
  fidelity is not certified by this still.
- [Run metadata](actual-profile/verification.json) records the executable hash,
  source profile hash, command and native handoff. The game loaded a byte-identical
  copy of the actual saved profile; the original bytes and timestamp are unchanged.
- Both native stills were visually inspected. Scripted process-local navigation
  and muted diagnostic autoplay were used; no desktop input or external capture.
