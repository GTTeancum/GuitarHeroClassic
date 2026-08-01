# CharWalk pose-ownership fix

## Symptom

During GH2 gameplay the guitarist moved between venue waypoints while retaining
the ordinary standing performance pose. The world transform advanced, so the
character appeared to slide across the floor instead of walking.

## Root cause

The CharWalk controller was already working:

- it loaded the character's authored turn, walk, and stop clip groups;
- it selected valid Classic clips;
- it advanced the root-motion predictor along the venue waypoint graph; and
- it published the retail Going and Stopping states.

The final pose-layer chooser was wrong for characters with an authored
`main.drv`. It always selected the normal performance player before checking
the active CharWalk player. Root motion therefore came from CharWalk while the
rendered skeleton came from the stationary performance clip.

## Correction

`active_main_driver_player()` now gives an active CharWalk player temporary
ownership of the main body pose. When CharWalk clears, ownership returns to the
authored performance player. Intro behavior is unchanged, and the left/right
hand drivers remain layered over the walking body pose.

The implementation is in:

`engine/src/game/gameplay.cpp`

A source contract in
`engine/src/game/gameplay_venue_band_contract_test.cpp` protects the required
CharWalk-before-performance ordering.

## Verification

The full-render 60 Hz Classic/Fender run is in:

`proofs/charwalk-pose-ownership-runtime6-fullrate/`

It proves:

- first walk: request 29.317, turn 29.317, walk 31.883, stop 46.400,
  complete 48.467;
- pose source is `gh1_walk` throughout turn/walk/stop;
- pose source returns to `active` immediately after completion;
- two additional walks complete in the same run;
- guitar hand drivers stay active at weight 1.0;
- autoplay reports zero misses and no failure;
- steady performance is 59.800 FPS; and
- the active game executable byte-matches the verified build.

Focused character utility, hand-driver weight, clip-driver flag, gameplay-rule,
and new CharWalk pose-ownership checks pass. The broad venue contract executable
still reports unrelated pre-existing source-contract drift; this change adds no
new failure there.

## Deployment

Verified executable SHA-256:

`5F0F62223D9132E401B27B46939EAAAAE562F57794E7D8F7392419339EA44A04`

Deployed path:

`gh2_ps2_hybrid_assets/ghogx_app.exe`
