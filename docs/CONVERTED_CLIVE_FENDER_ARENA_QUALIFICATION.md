# Converted Clive + RB2 Fender + GH1 Arena qualification

## Scope

This document covers the exact all-converted gameplay stack qualified on
2026-07-31:

- GH1 Clive converted to the GH2-native character layout as `classic`
- the Rock Band 2 Wii Fender Stratocaster converted as `guitar_sg`
- the GH1 Arena converted as `gh1_arena`

The target was GH2 gameplay parity, not a model-viewer proof. Clive had to
perform continuously with his whole body, walk rather than slide, keep both
hands attached to the instrument, and complete a song in the converted venue
under normal gameplay cameras.

The visible qualification used `Shout at the Devil` on Expert with diagnostic
autoplay. No camera was pinned and no stock GH1 character, instrument, or
venue asset was substituted.

## Instrument conversion

The Fender conversion is described in detail in:

```text
rb2_wii/FENDER_CONVERSION_GUIDE.md
```

The final package contains:

- the converted RB2 body, neck, fretboard, and headstock;
- all six RB2 strings baked into their bind-pose geometry;
- the RB2 string texture and its alpha/render properties;
- separate body-finish, pickguard, wood, hardware, and string materials;
- corrected UVs for the worn finish without white fretboard/headstock bands;
- a calibrated instrument transform behind the hands; and
- GH2-native attachment targets used by the fret and strum drivers.

The converted instrument package is:

```text
rb2_wii/output/drop_in/char/og/guitars/gen/guitar_sg.milo_ps2
```

Its size is 147,010 bytes and its SHA-256 is:

```text
95E5B2C4FB68937E9E1B4402DB57FE4B419B0473A7F385BE477F86EEC4C808B6
```

## Regression 1: walking motion rendered as sliding

### Symptom

Clive's world position moved between venue waypoints, but the rendered body
held an ordinary standing-performance pose. Only his position changed, so he
appeared to slide.

### Cause

CharWalk already selected valid turn, walk, and stop clips and advanced root
motion. The final pose chooser nevertheless gave the ordinary authored
`main.drv` player priority over an active CharWalk player.

The converted Arena also originally contained only one walk-eligible
guitarist waypoint. A one-node graph cannot supply a route.

### Correction

`active_main_driver_player()` in `engine/src/game/gameplay.cpp` now gives an
active CharWalk player temporary ownership of the main body pose. The normal
performance player resumes ownership after the stop clip completes. Fret and
strum drivers remain layered over either main-body source.

Venue conversion now derives two additional walk targets from the authored
`walk_spot_01` transform, offsets them along its local X axis, and connects all
three guitarist waypoints:

```text
start_guitarist0.way
walk_guitarist0_left.way
walk_guitarist0_right.way
```

The route generation is in:

```text
tools/milo_convert/gh1_venue_placement_conversion.cpp
tools/milo_convert/milo_convert_test.cpp
```

The reusable command added to
`tools/milo_convert/milo_convert_tool.cpp` is:

```text
milo_convert_tool rebuild-venue-waypoints <bundle-dir> <venue>
    --out <venue_chars.milo_ps2>
```

It rebuilds a converted venue's character-placement MILO without changing the
other converted venue archives.

The complete pose-ownership analysis is also retained in:

```text
docs/CHARWALK_POSE_OWNERSHIP_FIX.md
```

## Regression 2: arms moving over a frozen body

### Symptom

Clive entered a performance pose and remained there for long periods. The
independent fret and strum drivers continued moving his arms, which made the
rest of his body appear frozen.

### Cause

GH1 Clive's authored `main.drv` received performance-group requests such as
`normal`, but the compatibility runtime allowed the selected clip to exhaust
and clamp. It also initially saved the selected clip as the continuation
node, which would only replay the same clip.

### Correction

For non-intro guitarist performance groups (`normal`, `idle`, `extreme`, and
`solo`) that do not specify a loop mode, the runtime now applies
`kCharPlayNodeLoop`.

The continuation saves the requested group node rather than the initially
selected clip. Re-entering the group therefore advances the authored
`CharClipGroup::mWhich` selector. Intro requests and explicit loop modes are
unchanged.

The implementation and source contracts are in:

```text
engine/src/game/gameplay.cpp
engine/src/game/gameplay_venue_band_contract_test.cpp
```

## Rebuilding and deploying the Arena route

Build the `milo_convert` tool and its tests through the repository's normal
CMake build, then run:

```powershell
.\milo_convert_tool.exe rebuild-venue-waypoints `
  ..\out\native-bundle-proof2 gh1_arena `
  --out ..\out\native-bundle-proof2\world\gh1_arena\gen\gh1_arena_chars.milo_ps2
```

Overlay the output at:

```text
world/gh1_arena/gen/gh1_arena_chars.milo_ps2
```

Read the active ARK entry back immediately and compare hashes. The generated
file and active-ARK readback are both 903 bytes with SHA-256:

```text
CA838299F0E2AD48A33F71C71A434BEF92763F59406BB0C67A8A9BE7C6F357A8
```

The manifest and readback are:

```text
proofs/full-conversion-stack-visible/arena-waypoint-overlay.tsv
proofs/full-conversion-stack-visible/gh1_arena_chars.readback.milo_ps2
```

## Deployed executable

The executable used for the complete visible run is:

```text
gh2_ps2_hybrid_assets/ghogx_app.exe
```

Size: 5,962,752 bytes.

SHA-256:

```text
3F740BC3C1E707939DBF04209096B22B53D1EE00ED712247EF3401EED50C6C0E
```

The converted Clive model and its main, fret, and strum animation archives had
already passed exact active-ARK readback before the lifecycle correction.

## Preflight verification

The route converter unit test passes with six total placement waypoints and a
connected three-node guitarist route.

The focused post-fix body preflight recorded:

- 81 sampled guitarist states;
- six distinct body clips;
- seven `normal` group selections;
- zero clip-duration overruns;
- zero terminal-frame samples; and
- zero autoplay misses or overstrums.

The focused logs are:

```text
proofs/full-conversion-body-preflight3.stderr.log
proofs/full-conversion-stack-visible/milo-route-test.log
proofs/full-conversion-stack-visible/body-continuation-build3.log
```

The broad venue source-contract executable still reports unrelated,
pre-existing camera/crowd/order drift. The focused converter and clip-driver
checks pass, and none of the newly added contract strings is absent.

## Complete visible qualification

The visible run was launched from `gh2_ps2_hybrid_assets` with:

```powershell
.\ghogx_app.exe `
  --ark-dir .\GEN `
  --auto-start `
  --require-native-assets `
  --song shoutatthedevil `
  --difficulty 3 `
  --diagnostic-character classic `
  --diagnostic-guitar guitar_sg `
  --diagnostic-venue gh1_arena `
  --diagnostic-autoplay `
  --show-window
```

The user watched the complete run and confirmed that Clive was working
correctly. The song completed at 207.94 seconds with:

```text
final score: 178082
final streak: 518
misses: 0
overstrums: 0
```

The runtime trace proves:

- 220 sampled Clive body states;
- 14 distinct non-walk Clive clips;
- 10 distinct turn/walk/stop clips;
- zero Clive clip-duration overruns;
- seven completed walks;
- zero expired walk requests;
- 28 normal venue-camera sweeps;
- 25 distinct camera targets; and
- zero fatal, exception, or crash entries.

Each walk temporarily reports `main_source=gh1_walk`, keeps both hand-driver
weights at 1.0, and returns to the authored performance player after the stop
clip.

The complete stdout/stderr evidence is in:

```text
proofs/full-conversion-stack-visible-final/
```

## Separate stock-band to-do

During the same run, the stock GH2 `metal_singer` sometimes exhausted a direct
active/idle clip and visually clamped until the next band MIDI event selected
another clip. The user explicitly accepted this as a separate to-do rather
than part of this converted Clive/Fender/Arena correction.

It is recorded with exact trace intervals and acceptance criteria in:

```text
engine/src/game/PERFORMER_ANIMATION_LIFECYCLE_TODO.md
```

## Batch conversion remains a plan

The all-guitars-and-basses plan remains unexecuted:

```text
rb2_wii/RB2_INSTRUMENT_BATCH_PLAN.md
```

Every successful import must use its own RB2-authored store price. There is no
uniform 550-price rule.
