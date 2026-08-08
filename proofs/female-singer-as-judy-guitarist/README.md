# Female singer as Judy Nails guitarist proof

This is a hidden, input-free diagnostic run against the deployed hybrid ARK.
The rendered model is the converted GH1 `female_singer`; the animation owner,
driver program, and clips are the converted GH1 Judy Nails package (`alterna`).
The target occupies the `guitarist0` waypoint. The male singer remains the
vocalist; the female singer's source-authored mic descendants are suppressed
because her diagnostic role is guitarist.

The two captures are from the same deterministic run configuration at 1.10
and 4.10 seconds. They show body/head motion while the GH2 guitar remains on
the live Judy-derived `bone_pos_guitar.mesh` attachment chain:

- `frame-020-t1.10.png`
- `frame-080-t4.10.png`
- `playing-action-t8.00-t12.92.mp4`

The video is a five-second live-playing segment, not the entrance or idle
hold. The source `PART GUITAR` track enters `[play]` at 4.482 seconds; the
capture spans 8.00 through 12.92 seconds and ends with four chart misses. Judy's
`stand_fast`/`stand_medium` body clips drive the singer throughout. The singer
has no `CharIKHand`/`CharIKMidi` graph and no complete finger chains, so the
runtime truthfully skips the separate hand map rather than fabricating finger
or fret-hand motion.

Runtime facts:

```text
model=char/female_singer/og/gen/female_singer.milo_ps2
animationOwner=char/alterna/og/gen/alterna.milo_ps2
animationArchive=char/alterna/anims/gen/alterna_main.milo_ps2
role=guitarist0 waypointFlags=65 position=(36.2,52.0,20.7)
clips=alterna_intro_02,alterna_idle_medium
matchedOutputs=30/63 nonfinite=0
attachment=bone_pos_guitar.mesh parent=bone_pelvis.mesh
suppressedMicDescendants=1
```

This proves the cross-character direction and the generic bind-delta path. It
does not claim universal retarget parity: transforms without matching names
remain deliberately unmapped, and future external character configuration
must describe any factual non-identical bone mapping.
