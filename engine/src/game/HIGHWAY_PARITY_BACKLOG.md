# Highway Parity Backlog

This backlog is for the highway/gameplay fidelity fork. It is intentionally
small and evidence-first: every fix should have a 4:3 proof capture, and 16:9
should only be checked against one of the supported aspect modes.

## To-Do, Parked: Whammy Sustain Tails

- Keep the current PCSX2 body-width and blob evidence.
- Status: parked while smaller highway/gameplay parity wins are resolved.
- Do not continue guessing tail-width behavior from screenshots.
- The whammy ripple/thickening behavior needs a from-the-data implementation:
  held sustain bodies stay solid, while whammy input creates local blobs/ripples
  down the tail rather than globally changing tail width.
- Before touching this again, isolate which native tail layer owns the visible
  solid core, then derive the ripple deformation from PCSX2 trace data.

## Small Wins First

- Verify the highway root/camera rig keeps the surface, notes, rails, hit
  effects, flames, and fret buttons in one shared coordinate space.
  Current hook: `GHOGX_DEBUG_HIGHWAY_ALIGNMENT=1` emits 4:3 screen-space lane
  root rows for the strike point, fret-target smasher point, far-end point,
  hit flash, and held-fret state.
- Lock 4:3 as the primary proof mode, with 16:9 as the only secondary mode.
- Re-check note, sustain, rail, fret-button, and hit-effect alignment in 4:3.
- Re-check the far-end fade and highway angle/perspective against captured GH2
  evidence.
- Keep the broken track-explode mesh/particle debris purged from the highway.
  It produced large tan spike/square artifacts and must not be exposed by
  runtime flags until a source-true replacement is traced and rebuilt.
- Re-check wrong-pick, missed-note silence, star phrase completion stinger, and
  star meter gain behavior with short focused captures.
