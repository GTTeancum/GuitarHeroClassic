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
  Current hook: `GHOGX_HIGHWAY_TAIL_LAYER_ONLY=<source>` isolates labels such
  as `held_body_detail`, `held_body_fill`, `held_lane`, `held_tight`,
  `held_star`, `held_whammy_source_line`, and `burn` for 4:3 screenshot
  tracing. The legacy `held_body` value remains an alias for both normal body
  sublayers.
- Normal gameplay keeps the authored solid sustain body during whammy. The
  unverified global whammy-width deformation is parked behind
  `GHOGX_DEBUG_HIGHWAY_WHAMMY_TAIL_DEFORMATION` or
  `GHOGX_DEBUG_HIGHWAY_WHAMMY_LINE_ONLY` until the local blob/ripple formula is
  derived from PCSX2 trace data.

## Small Wins First

- PCSX2 small-window source refresh path: `tools/capture_pcsx2_window_sequence.py`
  can now drive the current PCSX2 qt input mapping without global keystrokes.
  The important mapping discovery is `Cross = Keyboard/L`, so `cross`/`confirm`
  are helper aliases for `L`, while `f8` triggers PCSX2's own screenshot hotkey.
  Current proof: `proofs/pcsx2_4x3_small_window_emucap_20260716_01`. It launches
  slot 1, sends Cross to leave the fail screen, sends F8 during live gameplay,
  and copies two clean PCSX2 emulator screenshots into the proof folder. A
  conservative red-sustain trace is stored at `trace_emucap_red_frame1`; it is
  accepted as fresh stock gameplay context and capture-path proof only, not as a
  replacement for the corrected GS/tubed red-tail width constants because the
  visible moment is dominated by the cyan/white core and has a narrow lane-body
  sample.
- Fresh PCSX2 4:3 emulator-capture visual refresh:
  `proofs/pcsx2_4x3_emucap_refresh_20260717_02` reruns the small-window PCSX2
  path with HWND-only input, `Keyboard/L` for Cross, and PCSX2's own F8
  screenshot hotkey. The retained proof keeps selected emulator snapshots only:
  a clean empty-highway/fade frame, a yellow normal sustain frame, a red normal
  sustain frame, trace overlays, normalized comparison plots, compact summaries,
  and capture manifests. The emulator snap output is `441x331`, effectively
  4:3. Finding: the fresh screenshots are useful visual references for the
  stock solid normal sustain body and far highway fade, but the traced red body
  contains a mid-tail contamination spike and the near rows are note/cap heavy.
  Red-vs-current normalized body median absolute error is `2.308 px`, far-body
  median `0.849 px`, and cap/effect median `3.899 px` with high near-row
  outliers; yellow-vs-current shows the same general split with body median
  `2.124 px` and cap/effect median `16.304 px`. Decision: no renderer patch is
  promoted from this fresh emulator capture. Keep using it as visual context
  and capture-path proof only; do not replace the GS-dump tail constants or
  tune cap/join geometry from this low-resolution screenshot trace.
- Fresh PCSX2 mid-rail projection refresh:
  `proofs/pcsx2_4x3_emucap_projection_refresh_20260716_01` traces only the four
  clean middle rail rows from the emulator-captured 4:3 screenshot above. It
  rejects the performer/sustain-obscured far rows and HUD-covered bottom row,
  then compares the retained points against the current 960x720 4:3 renderer
  projection: width RMS 4.969305 px, center RMS 2.032997 px over four low-res
  samples. This is accepted only as a gross angle/perspective regression check;
  it is not a far-fade, strike-line width, tail-shape, whammy, or audio proof.
  No camera/root patch is justified from this refresh.
- Normal held-tail synthetic-U A/B:
  `proofs/native_4x3_tail_synth_u_ab_20260717_01` adds a diagnostic-only
  `GHOGX_DISABLE_HIGHWAY_TAIL_SYNTH_U` switch around the existing `tail02.mesh`
  cross-tail UV reconstruction. Default rendering is unchanged. In the 4:3
  Shout Expert frame-60 capture, disabling synthetic U changes 10,054 / 691,200
  pixels in the full composite, localized to RGB bbox `[317,349,633,638]`,
  but the isolated `held_body` layer is pixel-identical (`0` changed pixels).
  Finding: the active normal sustain body's middle is owned by the
  `held_body_detail` + `held_body_fill` composition, not by the unproven
  `tail02.mesh` synthetic-U workaround. The near cap/tail-foot join remains the
  next visible normal-tail mismatch; no texgen=5 visual rewrite is promoted
  without exact source/PCSX2 ownership evidence.
- Tail source-UV default correction:
  `proofs/native_4x3_tail_source_uv_default_20260717_01` promotes the safer
  source-backed side of the synthetic-U A/B. `tail02.mesh` decodes with raw
  `u=[0,0]` and cross-tail `v=[0,0.6]`, while the broad GH2 tail materials use
  source `texgen=5`; the exact environment-coordinate math is still unproven.
  The renderer now keeps decoded source UVs by default and gates the older
  reconstructed cross-tail U path behind
  `GHOGX_EXPERIMENT_HIGHWAY_TAIL_SYNTH_U`. The fresh strict 4:3 Shout Expert
  frame-60 A/B shows the default-vs-experiment full composite differs in
  10,054 / 691,200 pixels (`1.454572%`, bbox `[317,349,632,637]`) while the
  isolated active `held_body` layer remains pixel-identical (`0` changed
  pixels). This removes an unproven guessed UV rewrite from normal rendering
  without changing the held-body middle; the exact `texgen=5` implementation
  and lower cap/effect owner remain open.
- Current 4:3 projection/fade guard after synthetic-U diagnostic:
  `proofs/native_4x3_projection_fade_current_20260717_03_synth_u_guard`
  recaptures the same Shout Expert 16.0s frame-60 window in strict 4:3 after
  the tail synthetic-U guard. The accepted PCSX2 4:3 projection reference remains
  `proofs/highway_4x3_projection_measure_20260715_04/projection_trace.json`.
  The fresh native overlay still measures rail width RMS `0.621769 px`, width
  max abs `0.937 px`, center RMS `0.080742 px`, center max abs `0.176 px`, and
  far fade target screen Y `326.666631 px` / world Y `164.477297` with alpha
  distance `71.67506`. No renderer projection/fade patch is promoted; the
  wrong-angle/far-fade regression stays closed while normal tail cap/join work
  remains open.
- Current 4:3 projection/fade guard after color pass:
  `proofs/native_4x3_projection_fade_guard_20260717_04` reruns the same strict
  4:3 Shout Expert 16.0s frame-60 capture after the recent note/button/tail
  color work. The PCSX2 reference is still
  `proofs/highway_4x3_projection_measure_20260715_04/projection_trace.json`.
  The fresh overlay remains unchanged at rail width RMS `0.621769 px`, width
  max abs `0.937 px`, center RMS `0.080742 px`, center max abs `0.176 px`, and
  far fade target screen Y `326.666631 px` / world Y `164.477297` with alpha
  distance `71.67506`. The compact trace also rechecks strict `960x720` 4:3
  output, `surface_dy == note_dy == -191.163`, and `same_pace=1`. No renderer
  projection/fade patch is promoted; this is a guard that the color pass did
  not reintroduce the wrong-angle or no-fade highway regression.
- Native 4:3 tail source contract guard after color pass:
  `proofs/native_4x3_tail_source_contract_guard_20260717_01` is a no-patch
  audit for the improved color state. It adds a fresh strict 4:3 Shout Expert
  frame-1/30/60 visual contact sheet (`960x720`, `exit=0`, final
  `state=playing`, `diff=3`, `t=17.500`, `score=100`, `hits=1`, `misses=0`)
  and copies the previous accepted guard for continuity. It also records the
  source boundary that blocks further normal-tail color/shape guessing:
  ihatecompvir/Milo source exposes `texgen=5` / environ texgen, but the exact
  GH2 broad-tail environment-coordinate path and `Tail::UpdateVerts` body are
  still not present as actionable source.
  The current renderer therefore keeps broad `tail02.mesh` synthetic cross-U
  behind `GHOGX_EXPERIMENT_HIGHWAY_TAIL_SYNTH_U`, keeps only the PCSX2-proven
  tight-tail cross-U default, and keeps active normal held sustains as source
  line/detail plus a lane-colored solid fill. Focused validation passes:
  `ghogx_gameplay_venue_band_contract_test`. No renderer patch is promoted;
  continue with lower cap/effect source ownership or stronger GS blend/depth
  attribution before touching normal-tail color/width/fill again.
- Current 4:3 cap owner/status split:
  `proofs/native_4x3_cap_owner_status_20260717_02` reruns the strict 4:3
  Shout Expert 16.0s frame-60 window as full, caps-disabled, sustains-disabled,
  and sustains+caps-disabled captures after the synthetic-U diagnostic. The
  regenerated proof keeps PNGs/contact sheets/metrics/summary/compact trace
  only; raw BMPs and logs were deleted after distillation. The current native
  active cap isolates to a short smasher-derived plate at bbox
  `[326,583,623,610]`, while the native sustain stack without that cap covers
  `[317,349,632,648]`. The PCSX2 frame-17 selected component remains the taller
  traced red/cyan tail-cap column at 640x480 bbox `[212,245,296,370]`, about
  `[318,367.5,444,555]` scaled to 960x720. Decision: no renderer patch is
  promoted from this pass. Do not promote smasher rim/add/cyan flare/tail color
  changes from this evidence; continue with lower cap/effect texture, raster,
  or source-owner tracing.
- Normal tail near-cap guard:
  `proofs/native_4x3_tail_near_cap_guard_20260717_01` is a compact
  measurement-only proof that reuses the current strict 4:3 native and stock
  PCSX2 GS traces without copying raw captures. It records that the current
  post-cross-U steady normal held body/core remains close to the corrected
  PCSX2 red-tail trace (`held_body` body/core median errors around `0.8..1.0`
  px and core median error around `1.0` px), while the largest remaining
  mismatch is still localized to near-cap/effect rows. It also preserves two
  rejected paths as guardrails: broadening the red source-line/detail path
  worsened stable body metrics, and the more aggressive tight-join refinement
  worsened mean/median near-cap width error. No renderer patch is promoted.
  Next native changes should target lower cap/effect ownership or source order,
  not steady tail body color, width, or another guessed flare.
- Normal tight-layer visibility audit:
  `proofs/native_4x3_tight_layer_visibility_audit_20260717_01` reruns the
  Shout Expert 16.0s frame-60 window in strict 4:3 with the explicit stock PS2
  `main.hdr`/`main_0.ark` pair after an earlier attempt accidentally stayed on
  the title path. The valid captures all report `state=playing`, `960x720`,
  loaded GH2 ingame bank/source track graphics, and the isolated layer logs.
  The no-tail baseline has zero tail rows, while `held_tight_edge` and
  `held_tight_join` each log 96 draw rows. Pixel diffs confirm visible output:
  `held_tight_edge` changes 950 pixels at bbox `[355,349,563,625]` and the
  lane-constrained cyan trace selects a long `203 px`-tall strip with median
  width `2 px`; `held_tight_join` changes 603 pixels at bbox
  `[317,618,631,637]` and the lane trace selects the short near-end cyan join
  with median width `22 px`. Decision: no renderer patch is promoted. This
  corrects the interpretation of the prior colorpass retrace where isolated
  tight-layer positive diffs looked empty; the layers render when captured with
  the explicit ARK and should not be "fixed" by adding a guessed cyan overlay.
  The next visual target remains lower cap/effect ownership or source order.
- PCSX2 lower cap UV/texture owner lead:
  `proofs/pcsx2_lower_cap_uv_owner_trace_20260717_01` reruns the stock GH2
  frame-17 GS trace with high vertex retention and the tracer now records
  per-context GS `CLAMP` state. The accepted red/cyan/white lower component is
  the tall 640x480 bbox `[212,245,296,370]` (`85x126`, 1813 pixels). Exact
  triangle coverage still finds 88 draw groups touching the component, proving
  that geometry overlap alone is not a patch basis. The strongest colored,
  in-range texture-footprint lead is consistently `tbp0=14208/cbp=13810`
  across transfers `236/1258/2280/3302`, with 100% component coverage and a
  recovered PSMT4 footprint at ST about `0.0386..0.0466` /
  `0.0282..0.0387` sampling RGBA mean `[65.833,195.5,136.167,34.0]`. The
  black fullscreen/page candidates such as `tbp0=11424` also cover 100% by
  geometry but sample black, so they are rejected as visual owners. Decision:
  no renderer patch is promoted yet; the next source-backed step is to map
  `tbp0=14208` back to the source object/material or emulate enough GS
  blend/depth/texture state to prove it owns the visible lower cap/join.
  Retained files are the compact summary, trace text, and contact sheet only;
  the high-retention raw vertex JSON was deleted after distillation.
  Multi-frame refresh: `proofs/pcsx2_lower_cap_multiframe_owner_trace_20260717_01`
  repeats the distill on stock GH2 frames 15/16/17 using the GS dumps'
  embedded 640x480 4:3 screenshots, not the external 960x540 captures. The
  trace tool now rejects frame PNGs whose size does not match the dump-embedded
  screenshot after that mismatch produced false component picks. Forced accepted
  component boxes overlap at `1.0` for frame 15 (`45x29`, 243 pixels), frame 16
  (`29x35`, 338 pixels), and frame 17 (`85x126`, 1813 pixels). `tbp0=14208`
  recurs as the colored PSMT4 texture-footprint lead on all three components
  (frame 15 sample `[82.0,203.0,154.0,34.0]`, frame 16
  `[49.25,191.75,161.25,34.0]`, frame 17
  `[65.833,195.5,136.167,34.0]`), while `tbp0=11424` still covers the same
  components geometrically but samples black. Decision remains unchanged: this
  is a stronger owner lead, not a native renderer patch; source-object mapping
  or GS blend/depth attribution is still required before changing cap/join
  rendering.
  Reconciliation proof
  `proofs/pcsx2_lower_cap_texture_candidate_reconciliation_20260717_01`
  puts the multi-frame coverage beside the existing `14208/13810` PSMT4/CLUT
  recovery and source-material map. It keeps the key distinction explicit:
  `14208` is a recurring colored footprint over the accepted component, but the
  recovered texture is low-alpha/noisy, its closest extracted-source matches are
  weak star/smasher/gem-style masks rather than a clean tail/cap material, and
  the source map still points normal tails to `tail_%s.mat`/`gem_star.tex`,
  held detail to `tail_glow_%s.mat`/`line01.tex`, and the active cap/rim to
  `gem_smasher.view`. Do not change cap/tail geometry, color, or order from
  the `14208` lead alone.
  State-refresh guard proof
  `proofs/pcsx2_lower_cap_state_refresh_guard_20260717_01` reruns stock GH2
  frame 17 after `trace_gsdump_gif_vertices.py` was tightened to refresh
  pre-vertex group state when in-tag AD `PRIM`, `TEX0`, `CLAMP`, `ALPHA`, or
  `TEST` registers arrive before vertices. The recurring `14208/13810`
  candidates still cover the forced lower-cap component, and the repeated
  candidates now retain `ALPHA {a=0,b=1,c=0,d=1,fix=0}` plus
  `TEST {ate=1,atst=6,aref=0,afail=2,zte=1,ztst=3}`. The first occurrence
  still has no attached `ALPHA`, and the sampled footprint remains low-alpha
  and noisy (`mean alpha 34` over six colored-alpha pixels), so this improves
  trace quality only; it still does not justify a native cap/tail renderer
  patch without source-object ownership or fuller GS attribution. The 193 MB
  raw vertex trace was deleted after distillation; retained files are the
  contact sheet, compact owner summary/trace, and
  `lower_cap_state_refresh_summary.json`.
  Local-owner negative proof:
  `proofs/pcsx2_lower_cap_local_owner_negative_20260717_01` distills the
  frame-17 lower-cap owner evidence into a compact guardrail. The forced PCSX2
  lower-cap component remains `[212,245,296,370]`, but the 88 covering groups
  and top 24 ranked groups still do not produce any `<512x512` local owner.
  The only `<=512x512` candidates are one no-texture draw and three broad
  `tbp0=11424` draws that sample black; the stronger texture leads
  (`14208`, `13824`, `15616`, `15744`) remain broad clipped/page-level
  candidates rather than source-owned lower-cap objects. No renderer patch is
  promoted. Do not enable smasher rim/add/cyan flare or move native cap/tail
  geometry from a presumed local owner until source-object attribution or a
  fuller GS blend/depth/raster proof exists.
- Ordered visibility guard:
  `proofs/pcsx2_lower_cap_ordered_visibility_guard_20260717_01` distills the
  stock GH2 frame-17 lower-cap state-refresh proof and local-owner negative
  proof into one compact contact sheet, JSON summary, and trace. Ordered by GS
  transfer/draw, the recurring `tbp0=14208/cbp=13810` rows remain the strongest
  colored full-coverage lead (`sample mean RGBA [65.833,195.5,136.167,34.0]`),
  but every top visibility row is still broad/page-level (`local=False`,
  `broad_penalty=2.0`) rather than a source-owned lower-cap object. The
  local-owner negative side remains unchanged: no top-24 `<512x512` owner, and
  the only `<=512x512` candidates are one no-texture draw plus three
  `tbp0=11424` black-sampling draws. Decision: no renderer patch is promoted;
  keep `14208/13810` as an attribution lead only, and do not move native
  cap/tail geometry or enable guessed flare/rim/additive effects from this
  evidence.
- `14208/13810` source-texture rank guard:
  `proofs/pcsx2_14208_source_texture_rank_guard_20260717_01` independently
  ranks the recovered PCSX2 `tbp0=14208/cbp=13810` PSMT4 page against the
  extracted `track.milo_ps2` source textures and material map. This is a
  negative attribution guard, not a patch basis: the recovered page is a noisy
  low-alpha VRAM page, low RGB difference alone is not ownership, the ordered GS
  trace still classifies the lead as broad/page-level, and the targeted upload
  scan still finds zero image payload hits for the target texture/palette
  blocks. The source material map remains unchanged: normal tail body is
  `tail_%s.mat` -> `gem_star.tex`, held/tight glow is `line01.tex` /
  `tail_tight.tex`, and active cap/rim is `gem_smasher.view` ->
  `gem_smasher.mesh` + `smasher_rim.mesh`, with no separate lower-cap mesh
  found. Decision: no renderer patch is promoted; do not use `14208/13810` to
  change lower cap/effect geometry, color, order, cyan flare, smasher add/ring,
  or tail width without a source-object owner or stronger GS texture-alpha
  raster attribution.
- Fresh current 4:3 visual guard after the `14208/13810` rejection:
  `proofs/native_4x3_current_visual_guard_20260717_01` reruns a bounded hidden
  native app capture against `gh2_ps2_hybrid_assets/gen` with strict `--aspect
  4:3`, fixed timestep, Shout Expert at 16.0s, and chart-driven input. The first
  attempted capture against the sibling `GuitarHeroOGX/assets/gen` ARK was
  rejected and deleted because that HDR version is unsupported by this v3
  reader and stayed on the title screen. The accepted rerun reaches
  `state=playing`, saves PNG frames 1/30/60 at `960x720`, logs the far board
  fade at `y=164.480` with `fade=0.000` and screen Y about `326.67 px`, and
  keeps the sampled lane roots shared in 4:3. Decision: no renderer patch is
  promoted from this guard; it is a current-state visual proof that the source
  texture rejection did not change the highway, and the broken plank/no-fade
  regression is not present in this capture.
- Verify the highway root/camera rig keeps the surface, notes, rails, hit
  effects, flames, and fret buttons in one shared coordinate space.
  Current hook: `GHOGX_DEBUG_HIGHWAY_ALIGNMENT=1` emits 4:3 screen-space lane
  root rows for the strike point, fret-target smasher point, far-end point,
  hit flash, and held-fret state.
- Re-check note, sustain, rail, fret-button, and hit-effect alignment in 4:3.
  Current proof:
  `proofs/native_4x3_hit_effect_alignment_current_20260716_06_fill_comp_early_hits_meshlog`.
  It refreshes the 4:3 hit-effect/fret-button alignment proof after the
  normal-tail fill-comp fix, with 55 sampled alignment rows, 13 active hit
  rows, all sampled rows sharing the highway root, active-hit
  strike-to-smasher X drift still at or below 0.02 px, and a lower-highway
  overlay sheet showing hit feedback on the logged lane/fret centers. It also
  logs the source `hit_flame` mesh as the active base hit feedback. This is an
  alignment regression proof, not a PCSX2 flame-animation parity signoff.
  Current refresh after the active normal-tail material pass:
  `proofs/native_4x3_hit_effect_alignment_current_20260716_07_active_tail_current`.
  It uses the same 30.0..34.0s 4:3 Shout diagnostic window and records 135
  alignment rows, 28 active-hit rows on lanes 1 and 3, all sampled rows sharing
  the highway root, max active-hit strike-to-smasher X drift 0.02 px, source
  `hit_flame` mesh active, no flat fallback, and authored origin, transform
  anim, and color anim active on every hit-feedback row. This still does not
  close PCSX2 hit-flame animation parity; it proves current native
  flame/fret-center alignment did not regress.
  Current refresh after the active sustain cap and source color recheck:
  `proofs/native_4x3_hit_effect_alignment_current_20260717_01_active_cap_color`.
  It uses the same 30.0..34.0s 4:3 Shout diagnostic window, records 55 sampled
  alignment rows and 13 active-hit rows on lanes 1 and 3, keeps all sampled
  rows on the shared highway root, and still caps active-hit
  strike-to-smasher X drift at 0.02 px. The source `hit_flame` mesh remains
  active with no flat fallback, authored origin/transform/color animation active
  on every hit-feedback row, and the proof sheet marks the logged active lane
  roots over the lower-highway screenshots. This is still only an alignment
  regression proof; PCSX2 hit-flame animation/spread parity remains open.
  Current source-animation trace:
  `proofs/native_4x3_hit_feedback_anim_trace_20260717_01` reruns the same
  Shout 30.0s strict 4:3 window after adding a bounded
  `GHOGX_DEBUG_HIGHWAY_HIT_FEEDBACK` diagnostic row for the sampled source
  flame animation frame and transform. The run exits 0, saves a distilled PNG
  contact sheet from frames `1,2,4,6,8,10,12,14,16,20,24`, and logs 13
  `[highway-hit]` rows plus 26 `[highway-hit-anim]` rows across lanes 1 and
  3. Both `hit_flame` and `star_collect` paths sample the authored 8-frame
  transform sequence from frame `0.000` through `7.800`; every sampled row has
  source rotation and scale, no translation, and MatAnim color active. This is
  accepted as a stronger native/source measurement hook than the previous
  `base_anim=1` proof, not as PCSX2 hit-flame animation parity. Focused
  contract guard passes: `ghogx_gameplay_venue_band_contract_test`. Visual
  proof: `hit_feedback_anim_trace_contact_sheet.png`; compact trace:
  `hit_feedback_anim_trace.txt`; summary:
  `hit_feedback_anim_trace_summary.json`.
- Lock 4:3 as the primary proof mode, with 16:9 as the only secondary mode.
  Current 16:9 supported-aspect proof:
  `proofs/native_16x9_current_aspect_signoff_20260716_04_active_tail_current`
  renders exactly 1280x720 after the active normal-tail material/color pass,
  enters live gameplay at the 16.000s diagnostic seek, captures frames
  1/60/120/180, and still matches the previous 16:9 aspect audit's far-fade and
  strike-lane samples with 0.0 px logged deltas. This is not a PCSX2 rail-parity
  trace; no 16:9 PCSX2 source trace is present in the repo.
- Incoming sustain core/body pass: normal/star-phrase incoming tails now stack
  the authored tight `tail_glow_tight.mat`/`tail_tight.tex` core over the broad
  lane/star phrase layer, and ordinary incoming tails keep a filled center body
  instead of collapsing into two rails or a skinny line. Current focused 4:3
  proof: `proofs/native_4x3_incoming_normal_tail_body_20260716_05_broad_fill_scaled`.
  In the repeated yellow-tail frame-360 ROI, the collapsed current pass measured
  5 px wide; the scaled fill pass measures 30 px against the older broad
  authored reference at 29 px, with full-stack occupancy 0.909 versus 0.903 and
  isolated body occupancy at 1.0. This is a regression fix against the prior
  authored incoming-tail proof; whammy ripple/blob deformation remains parked
  for PCSX2 trace-derived work.
- Far-end fade/angle pass: track surface, side rails, lane lines, and star-power
  glow now fit their far edge to the PCSX2-measured fade-top world row while the
  track mask remains un-stretched. Proof:
  `proofs/native_4x3_fade_profile_horizon_fit_20260716_01`. The current
  combined 4:3 proof
  `proofs/native_4x3_current_combined_parity_20260716_08_fill_comp_current`
  refreshes the PCSX2 rail projection comparison after the clean held-body
  fill-comp fix:
  width RMS 0.621769 px, center RMS 0.080742 px over nine rail samples.
- Note/button color pipeline pass: GH2 PS2 `RndMat` rev 27 predates the
  ihatecompvir `mPointLights` field, so `use_environ` alone must not force D3D
  point lighting on note/smasher meshes. Runtime meshes now carry the decoded
  point-light bit separately and only enable source lighting when that authored
  bit is present. Dedicated A/B proof:
  `proofs/native_4x3_point_light_material_fix_20260716_01`, where the fixed
  path matches the diagnostic lighting bypass exactly for the pressed smasher
  luma samples. Current combined 4:3 regression proof:
  `proofs/native_4x3_current_combined_parity_20260716_09_point_light_fix_current`,
  still at 960x720 with rail width RMS 0.621769 px and center RMS 0.080742 px
  over nine samples. This is not complete color parity for every gameplay item;
  it closes the synthetic darkening pipeline bug and keeps the 4:3 highway
  projection/fade locked.
- Current 4:3 color/material checkpoint:
  `proofs/native_4x3_color_current_probe_20260716_01` captures frames
  1/18/60/120 at 960x720 in the same Shout Expert 16.000s diagnostic window,
  with safe rock state and chart-driven input. The runtime log reports sampled
  lane colors `e12cc947/e1f32f3a/e1fbe200/e16eacfb/e1f86917`, `track.env`
  ambient `0,0,0,1` with one point light present but GH2 note meshes still
  gated at `point_lights=0`, red/blue standard gems as `gem.tex blend=3
  zmode=1 use_env=1 prelit=0 intensify=1`, red lane tail as
  `gem_star.tex blend=3 zmode=1 prelit=1 intensify=1 mat=0.970,0.190,0.230`,
  red held detail as `line01.tex blend=4 zmode=0 prelit=1 mat=1.000,0.412,0.102`,
  and active smasher rows drawing both source ring-add and RGB-mask add layers.
  The contact sheet shows the current red/blue note, tail, and fret-button
  state without low-rock rail contamination. No renderer color tweak was
  promoted from this checkpoint because the source-material chain is coherent;
  complete PCSX2 color parity still needs a tighter source-vs-native color
  trace.
- Current combined 4:3 proof after the active normal-tail material pass:
  `proofs/native_4x3_current_combined_parity_20260716_10_active_tail_current`
  refreshes frames 1/60/120/180 at 960x720 with the same diagnostic chart
  script and no live keystrokes. The PCSX2 rail projection regression is still
  width RMS 0.621769 px / center RMS 0.080742 px over nine samples, with
  fade-top target y 326.666631 px, world top y 164.477297, and alpha distance
  71.67506. Its full-frame red-tail trace is accepted only as current visual
  context; like the prior combined trace, it is too contaminated by the visible
  glow/cap stack for clean sustain-tail width tuning.
- Current combined 4:3 proof after the shifted tight-edge normal-tail pass:
  `proofs/native_4x3_current_combined_parity_20260716_11_tight_edge_scaled`
  refreshes the same 16.000s Shout diagnostic window at 960x720, with frames
  1/60/120/180, after the active normal sustain stack gained the PCSX2-measured
  tight-edge offset and 0.6x edge-width fit. The broken pale/spiked highway does
  not reproduce in this pass: the board is back on the source surface, the
  far-end fade is active, and the 4:3 rail projection remains locked to the
  accepted PCSX2 trace at width RMS 0.621769 px / center RMS 0.080742 px over
  nine samples. Runtime fade diagnostics record fade-top world Y 164.480,
  alpha distance 71.675, fade start world Y 92.805, and board fade-top center
  at screen Y 326.67. This is the current broad visual regression checkpoint,
  not complete parity: normal sustain edges still look crisper/straighter than
  the PCSX2 tubed reference, whammy ripple remains parked for trace-derived
  work, and audio/16:9 PCSX2 parity are not covered by this proof.
- Rail color-state capture pass: the red side rails in the previous color proof
  were a valid low-rock warning state caused by missed notes during the
  diagnostic seek, not a normal-state source color. The focused 4:3 proof
  `proofs/native_4x3_safe_rail_color_state_20260716_01` runs Surrender Expert
  with diagnostic autoplay and `--diagnostic-rock 0.85`; its log emits
  `rock=0.851 warning=0.000 side=0.000` for the first healthy rows while all
  three screenshots stay out of the red warning rail state. This does not close
  lane-line/rail visual parity; it only prevents normal-color review from using
  contaminated low-rock captures.
- Normal sustain burn suppression pass: the decoded GH2 source still contains
  `burn_tail_base.part` and `burn_tail_big.part`, but the corrected PCSX2
  normal red sustain reference has no comparable orange/yellow burst at the
  fret line. Focused isolation proof
  `proofs/native_4x3_burn_isolation_20260716_01` disables hit particles/rings
  and shows the intrusive flame sprites come from the sustain burn layer
  itself, not from one-shot hit feedback. Runtime now suppresses that burn layer
  for normal non-whammy, non-bonus sustains by default while preserving
  `GHOGX_FORCE_HIGHWAY_NORMAL_SUSTAIN_BURN=1` for source diagnostics and
  leaving star/whammy/bonus paths untouched for later traces. Fixed 4:3 proof:
  `proofs/native_4x3_normal_burn_suppressed_20260716_01`, where frames 40 and
  60 keep the normal red/blue sustain bodies and fret targets without the
  orange burn burst. Full-scene regression proof:
  `proofs/native_4x3_full_scene_after_normal_burn_20260716_01`, which captures
  the same normal red/blue sustain window with venue and HUD active and no
  return of the orange burn burst. Follow-up context trace
  `trace_red_full_scene_frame60` and comparisons in that folder were rejected
  for tail-width tuning because the full-scene ROI measures the visible glow/cap
  stack instead of the clean no-tail-subtracted body layer; the clean split and
  fill-comp trace folders remain authoritative for normal body width constants.
  This does not close hit-flame or whammy burn parity.
- Active normal sustain material/width pass: active held normal sustains now
  render the visible `tail_glow_%s.mat`/`line01.tex` detail through the
  source-style line path (`Tail::mTailGlow` is an `RndLine` in the RB2 dump)
  while keeping the lane `tail_%s.mat` material RGB for the solid center fill.
  The older `tail02.mesh` detail path remains only as a fallback behind
  `GHOGX_DISABLE_HIGHWAY_NORMAL_TAIL_SOURCE_LINE=1`. Current 4:3 proof:
  `proofs/native_4x3_normal_tail_source_line_default_20260716_01`, with
  contact sheet `normal_tail_source_line_default_contact_sheet.png` and summary
  `normal_tail_source_line_default_summary.json`. Against
  `proofs/pcsx2_4x3_red_normal_tail_corrected_20260716_01`, the no-tail-
  subtracted source-line detail improves silhouette body median absolute error
  from the mesh-detail A/B's 3.883 px to 1.164 px, and the default full stack
  improves lane-body body median absolute error from the old mesh full stack's
  8.126 px to 1.5 px. This fixes the old striped/twin-rail active normal
  sustain look and the broad warm detail bleeding into the lane-body trace, but
  it does not close full-stack silhouette parity: the default full silhouette
  body median absolute error is still 5.271 px, so whammy and remaining tail
  shape work stay open. Rejected historical mesh-detail follow-up:
  `proofs/native_4x3_active_tail_detail_curve_20260716_01` tried replacing the
  single 1.67 detail response with a per-row compensation curve, but it
  worsened the isolated detail median abs error from 3.883 px to 4.071 px,
  worsened the full-silhouette median abs error from 3.293 px to 3.856 px, and
  introduced visible warm haze around the active tail; the renderer was restored
  to the proven constant-scale path.
  Source-state audit:
  `proofs/native_4x3_tail_source_state_audit_20260716_01` records the decoded
  GH2 assets used for this path. `tail_glow_red.mat` binds `line01.tex` with
  blend 4, zmode 0, texgen 0, wrap 1, flags `-P---WB`, and RGB
  `[1.000, 0.412, 0.102]`; `tail_red.mat` binds `gem_star.tex` with blend 3,
  zmode 1, texgen 5, UV scale `[0.5, 1.0]`, and RGB
  `[0.970, 0.190, 0.230]`; `tail02.mesh` has 12 verts, 10 faces, bbox X
  `-0.43..0.43`, Y `0..30`, Z `0..0.32`, and raw UV V `0..0.6`.
  Rejected UV A/B:
  `proofs/native_4x3_tail02_center_v_diag_20260716_01` forced `tail02.mesh`
  sampling to center V=0.5 as a diagnostic only. It did not materially improve
  the corrected PCSX2 comparison: detail body median abs moved 3.883 -> 3.764
  px, but RMS stayed 4.617 -> 4.598 px, fill-to-body stayed identical at 1.5 px
  median abs, and full silhouette RMS worsened 5.036 -> 5.133 px. No renderer
  change was promoted from this test.
  Fresh retrace after the color/smasher material passes:
  `proofs/native_4x3_normal_tail_silhouette_retrace_20260716_01` re-captures
  no-tail, `held_body_detail`, `held_body_fill`, combined held-body, and full
  4:3 highway-only layers at frame 60 with the same corrected PCSX2 red-tail
  reference. The solid fill remains close to the PCSX2 lane-body target
  (fill-to-body body median abs 1.5 px), while the combined/full silhouette is
  still row-shape limited (full silhouette body median abs 3.293 px, mean
  -2.237 px). A measured per-row detail compensation experiment was rejected
  again because it centered the silhouette mean but worsened the lane-body
  classification and visible source-detail bleed. The promoted source-line pass
  above supersedes this as the current default; remaining normal-tail work should
  focus on the full-stack silhouette/cap mismatch rather than restoring the
  broad mesh detail.
  Follow-up source sampler pass:
  `proofs/native_4x3_tail_source_sampler_wrap_20260716_01` changes runtime
  mesh sampler setup to honor decoded `RndMat::tex_wrap` before the UV
  repeat/scale fallback. Runtime diagnostics now show `red_held_tail`,
  `held_tight_tail`, `star_held_tail`, and `whammy_held_tail` as
  `sampler=wrap`, matching the source `tail_glow_%s.mat` wrap=1 rows, while
  the lane body `red_tail` remains `sampler=clamp` against
  `tail_red.mat` wrap=0. This is accepted as material-state fidelity only; it
  does not close the remaining normal-tail silhouette row-shape mismatch or
  whammy deformation.
  Follow-up active normal tight-underlay pass:
  `proofs/native_4x3_normal_tail_tight_underlay_fillwidth_20260716_01` draws
  the authored `tail_glow_tight.mat` / `tail_tight.tex` width below the active
  normal lane body, then uses the corrected PCSX2 lane-body width directly for
  the solid red/blue body fill. This replaces the old centered skinny tight
  core for active normal sustains while leaving star/whammy tight width
  unchanged. Against the corrected PCSX2 red normal-tail trace, normalized full
  silhouette body median absolute error improves from the source-line default's
  5.271 px to 2.168 px. The screen-y overlap guard also improves silhouette
  median abs error from 7.5 px to 5.0 px while keeping lane-body overlap at
  1.5 px median abs. Contact proof:
  `normal_tail_tight_underlay_fillwidth_contact_sheet.png`; visual A/B crop:
  `normal_tail_pcsx2_before_underlay_fillwidth_crop_zoom.png`. This is a
  promoted normal-tail structure/color improvement, not a whammy ripple fix and
  not final full-stack parity.
  Follow-up shifted tight-edge pass:
  `proofs/native_4x3_normal_tail_tight_edge_scaled_20260716_01` keeps the
  underlay/body stack above, then redraws only the tight edge over the body using
  the PCSX2-measured core/body center offset (`-8.5 px` at 720p) and a 0.6x
  width fit from the first shifted-edge trace (`~5 px` native core vs `~3 px`
  PCSX2 core). This preserves the lane-body overlap metric at 1.5 px median abs
  while improving screen-y silhouette median abs from the underlay/body pass's
  5.0 px to 2.5 px and core median abs from 2.5 px to 0.5 px. Normalized
  full-silhouette body median abs improves again to 1.5 px. Contact proof:
  `normal_tail_tight_edge_scaled_contact_sheet.png`; crop proof:
  `normal_tail_pcsx2_before_after_tight_edge_scaled_crop_zoom.png`; screen-y
  guard: `screen_y_overlap_tail_compare.json`. Remaining mismatch: native edge
  sampling is still crisper than the PCSX2 capture, and whammy ripple/body
  deformation remains parked for trace-derived work.
  Follow-up source-line/body-fill probe:
  `proofs/native_4x3_normal_tail_line_detail_profile_probe_20260717_01`
  isolates the active `held_body_detail` source-line layer from a no-tail
  baseline. Against the corrected PCSX2 red normal-tail silhouette, the stable
  body-region median absolute error is 1.164 px, so the source-line detail is
  not the owner of the remaining straight/clean body read. A rejected source
  mesh center-fill test is retained at
  `proofs/native_4x3_normal_tail_source_mesh_fill_20260717_01`: drawing the
  measured `held_body_fill` through the authored `tail_%s.mat` /
  `gem_star.tex` lane-tail mesh reopens the hollow/two-rail failure in the
  isolated body-fill screenshot, so that path is locked out by contract and was
  not promoted. Color statistics from the same pass are mixed: the full native
  core is brighter than PCSX2, but the no-tail-subtracted core is close, so no
  alpha/color tweak was promoted without a stronger source-backed isolation.
  Follow-up tight-layer alpha pass:
  `proofs/native_4x3_normal_tail_tight_layer_probe_20260717_01` isolates the
  normal `held_tight_underlay` and `held_tight_edge` layers. The shifted edge is
  close to the corrected PCSX2 core width (core-body median abs 1.55 px), while
  the full-width underlay by itself is much wider than the PCSX2 core
  (core-body median abs 13.0 px). A tested core-width underlay experiment at
  `proofs/native_4x3_normal_tail_tight_underlay_corewidth_20260717_01` was
  rejected because it narrowed the full composite core too far: screen-y core
  median abs worsened from 0.297 px to 2.0 px even though the isolated underlay
  matched the core. The promoted pass instead keeps the traced widths and
  reduces the normal active tight underlay/edge alpha by the measured
  PCSX2/native full-core luma ratio (`182.3 / 227.5 ~= 0.80`):
  `proofs/native_4x3_normal_tail_tight_alpha_20260717_01`. Full-image core
  median luma moves from 227.5 to 200.9 toward the PCSX2 182.3 target, full
  silhouette body median abs improves from the prior shifted-edge pass's
  1.5 px to 1.0 px in the normalized profile, and lane-body screen-y overlap
  stays at 1.312 px median abs. Tradeoff: strict core-width screen-y median abs moves
  from 0.297 px to 1.781 px, so this is a brightness/color improvement, not
  final tubed-core parity. Broad 4:3 regression proof:
  `proofs/native_4x3_current_combined_parity_20260717_01_tight_alpha`, with
  rail projection still locked at width RMS 0.621769 px and center RMS
  0.080742 px over nine samples.
  Rejected residual-alpha follow-up:
  `proofs/native_4x3_normal_tail_tight_residual_alpha_20260717_01` tested a
  second data-derived alpha reduction from the remaining core-luma error,
  lowering normal tight underlay/edge alpha to 162/168. It was not promoted:
  the no-tail-subtracted 4:3 trace left core-body median abs essentially flat
  (1.08 px -> 1.0 px), worsened silhouette body mean abs (2.614 px -> 2.926
  px), and over-dimmed the traced native core median luma (200.9 -> 153.6)
  instead of landing near the PCSX2 target. The renderer was restored to the
  accepted 196/204 alpha values.
  Rejected body-fill alpha follow-up:
  `proofs/native_4x3_normal_tail_body_alpha_20260717_01` tested the apparent
  full-frame lane-body luma ratio by reducing the solid center-fill alpha from
  245 to 218. It was not promoted: the full composite lane-body median luma
  moved away from PCSX2, 94.9 -> 102.6 versus the PCSX2 84.6 target, because
  the lower alpha lets more underlying glow/board contribution through. Width
  metrics stayed effectively unchanged, and the visual body became less solid,
  so the normal active solid fill remains at alpha 245.
- Moving note z-mode pass: runtime highway meshes now retain decoded
  `RndMat::ZMode`, and moving-note z-write follows the same source rule already
  used by the venue renderer: Normal/Force/Decal can write depth, while
  transparent/effect z-modes, additive/subtract/multiply blends, and faded
  alpha suppress z-write. This removes the previous shortcut where every
  `SrcAlpha` note layer was treated as non-writing despite source note bodies
  decoding as `blend=3 zmode=1`. Proof:
  `proofs/native_4x3_note_zmode_source_20260716_01`, with
  `gem_note_materials.txt`, `gem_template_group.txt`, note mesh diagnostics
  proving `red_gem`, `blue_gem`, and `top` as `blend=3 zmode=1`, plus a 4:3
  current-frame contact sheet of the red/blue sustain-note window. This is a
  source material/depth pipeline fix; broader note/button color parity remains
  open.
- Moving note/source UV matrix pass: highway runtime meshes now apply the full
  decoded `RndMat::tex_xfm` matrix, matching the venue renderer's source UV
  rule instead of collapsing materials to diagonal scale plus offset. Proof:
  `proofs/native_4x3_note_uv_matrix_source_20260716_01`, with material dump
  rows exposing non-identity/off-axis transforms on `gem2.mat` and `star2.mat`
  plus a 4:3 red/blue sustain-note and star-note capture. This is a source
  material-coordinate cleanup, not a complete note/button color signoff.
- Moving note/tail intensify pass: highway runtime meshes now retain decoded
  `RndMat::intensify` and use the source 2x texture combine for textured meshes
  whose materials set it. Follow-up correction: the synthetic solid tail body
  fill now uses the authored `RndMat` RGB directly instead of applying that 2x
  base-map boost a second time. Proof:
  `proofs/native_4x3_material_intensify_source_20260716_01`, with
  `gem_materials_intensify.txt`, `tail_materials_intensify.txt`, runtime
  diagnostics proving `gem.mat`/`top.mat`/HOPO/top-star and lane body tails at
  `intensify=1`; the solid-fill correction is covered by the current contract
  guard and the next 4:3 proof pass. Broader button/color parity remains open.
- Moving note/tail material sampler pass: highway runtime meshes now seed the
  sampler from decoded `RndMat::tex_wrap`, then fall back to wrapping when the
  transformed UV range repeats or the material scale exceeds 1.01. The original
  source-sampler cleanup removed the old global-wrap shortcut and proved
  repeated `star2` as `sampler=wrap` while lane body tails remained clamped:
  `proofs/native_4x3_material_sampler_source_20260716_01`. The follow-up tail
  source pass fixes the remaining authored-wrap miss on the held-tail line
  materials: `proofs/native_4x3_tail_source_sampler_wrap_20260716_01`, where
  `tail_glow_%s.mat` rows with wrap=1 log as `sampler=wrap` and
  `tail_red.mat` wrap=0 still logs as `sampler=clamp`. This is source
  material-state fidelity; broader note/button color parity remains open.
- Material flag decode-order correction: the MiloEditor RndMat reader and RB3
  native `RndMat::Load` both read `use_environ` before `prelit` after material
  RGBA. The prior flag-order pass used the Grim Rust order and flipped those
  meanings for GH2 gameplay materials: source-environment gems/buttons became
  prelit raw draws, while some tail layers became environment-lit. Native now
  restores the `use_environ`/`prelit` order and leaves color values untouched.
  Follow-up color-pipeline correction: GH2 rev27 does not carry the later
  ihatecompvir `mPointLights` byte, so the highway renderer must not invent D3D
  point/spot lighting for every `use_environ` material. Source lighting is now
  gated on the decoded `point_lights` bit; stock GH2 note/button materials keep
  their source texture/material combine without a misplaced runtime light.
  Proof is captured in the follow-up
  `native_4x3_material_flag_order_source_20260716_02` pass.
- Pressed smasher ring-add source layer: `track.milo_ps2` contains
  lane-authored `now_ring_*_1.mat` materials for the fret-button rim mesh.
  Native now loads `smasher_rim.mesh` with those exact `_1` materials and draws
  additive source materials directly. First proof:
  `proofs/native_4x3_smasher_ring_add_source_20260716_01`, with a 4:3
  before/after sheet, material dump, and smasher diagnostics proving
  `ring_add=1`. Green/yellow/blue draw their source blend-2 ring-add layers and
  gain lower-button ROI mean luma by +10.2, +10.7, and +7.0 respectively.
  Red/orange/bonus source-alpha `_1` materials now bind a separate
  `#smasher_ring_add_black_key` alias whose alpha is derived from the authored
  RGB mask intensity instead of the decoded all-255 alpha. Accepted proof:
  `proofs/native_4x3_ring_alpha_key_pipeline_20260716_01`, with material dump,
  smasher diagnostics showing `ring_add_blend=3 ring_add_draw=1` for red/orange
  and a before/after sheet proving the old black-fill failure is gone without a
  guessed additive override. A naive global `use_environ` lighting toggle made
  pressed buttons visibly darker and was rejected; broader note/button color
  parity still needs a PCSX2/material-lighting trace.
- Pressed smasher body-add source blend: `gem_smasher_*_1.mat` is now loaded
  as texture plus blend instead of texture-only. The lane add materials decode
  to blend 3 (`SrcAlpha`) with `smasher_add_<lane>.tex`, so runtime binds those
  opaque RGB masks through a separate `#smasher_add_rgb_mask` alias and derives
  alpha from RGB intensity. `gem_smasher_bonus_1.mat` remains its authored
  blend-2 `smasher_add.tex` path. Proof:
  `proofs/native_4x3_smasher_add_source_blend_20260716_01`, with
  `gem_smasher_materials.txt`, smasher diagnostics showing all five pressed
  lanes at `add_blend=3 add_rgbmask=1`, and a 4:3 before/after crop. The button
  ROIs show a small consistent luma pullback after removing the guessed
  `D3DCOLOR_ARGB(180,...)` additive overlay. This is a pipeline cleanup for the
  fret buttons, not a full note/button color-parity signoff.
- Pressed smasher body-add material mesh cleanup: the source blend pass still
  reduced `gem_smasher_*_1.mat` to texture plus blend at draw time. Native now
  also converts `gem_smasher.mesh` with each `_1` material, including
  `gem_smasher_bonus_1.mat`, and draws the pressed add overlay through that
  source-bound mesh when available. The old texture/blend path remains only as
  a missing-source fallback. Proof:
  `proofs/native_4x3_smasher_add_material_mesh_20260716_01`, with 4:3 frames
  1/18/60/120, contact sheet, summary JSON, and smasher diagnostics showing
  `base=5 add=5 add_mesh=5 ring=5`, five smasher add RGB-mask aliases, and
  pressed red/blue rows at `add_mesh=1 add_blend=3 add_zmode=1 add_rgbmask=1`.
  This is source-material-state cleanup for fret buttons; it is not a complete
  PCSX2 color parity signoff.
- Active star/whammy tail source-color cleanup: `tail_glow_star.mat` and
  `tail_glow_whammy.mat` already carry their authored cyan/teal RGB in
  `track.milo_ps2`, so active star sustains now use neutral runtime RGB tint
  and only preserve the existing alpha. Proof:
  `proofs/native_4x3_star_tail_source_tint_20260716_01`, with a before/after
  4:3 screenshot sheet and source material dumps. Runtime diagnostics moved
  `held_star` from hard-coded `argb=f537e1ff` to neutral `argb=f5ffffff`,
  changing the effective pre-texture star-tail RGB from about `[5,183,255]` to
  the decoded material RGB `[21,207,255]`. This is a material-pipeline fix, not
  a whammy ripple/blob implementation.
- Active bonus-highway sustain source-color cleanup: `tail_bonus.mat` already
  carries the authored bonus-tail RGB in `track.milo_ps2` (`gem_bonus.tex`,
  blend 3, zmode 1, texgen 5, color `[0.000,0.690,0.750,1.000]`), so active
  held bonus sustains now use neutral runtime RGB tint when the source mesh is
  present and keep the previous cyan tint only for the emergency flat fallback.
  Proof: `proofs/native_4x3_bonus_tail_source_tint_20260716_01`, with decoded
  material dump, runtime diagnostics proving `bonus_tail=1` and
  `argb=f5ffffff`, plus a 4:3 screenshot contact sheet. This moves the
  effective pre-texture RGB from the double-tinted `[0,155,191]` path back to
  the decoded material RGB `[0,176,191]`. Broader note/button color parity
  remains open; this only closes the bonus-sustain tint pipeline bug.
- Keep the broken track-explode mesh/particle debris purged from the highway.
  It produced large tan spike/square artifacts and must not be exposed by
  runtime flags until a source-true replacement is traced and rebuilt.
- Re-check wrong-pick, missed-note silence, star phrase completion stinger, and
  star meter gain behavior with short focused captures.
  Current proof:
  `proofs/native_4x3_gameplay_audio_feedback_current_20260716_04_explicit_ark`
  refreshes the scripted wrong-pick/miss/recovery and clean star-phrase windows
  against the explicit PS2 `main.hdr`/`main_0.ark` pair after the normal-tail
  center-order renderer build. It proves `miss_gtr`, guitar stem mute/unmute,
  three `sp_gemhit` routes, one `sp_awarded` phrase-completion route, zero
  misses in the clean phrase, and star meter gain to 0.250. The earlier
  `_03_center_order_build` folder is rejected because it omitted `--hdr/--ark`
  and stayed on the title path. Waveform-level PCSX2 audio comparison remains
  out of scope for this proof.
- Star/whammy sustain audit after the normal-tail split:
  `proofs/native_4x3_whammy_star_audit_current_20260716_02_after_silhouette_tail`.
  The default 4:3 scripted star sustain records one hit, zero actual misses,
  200 star-power whammy gain ticks, 15 whammy-pitch-active samples, final star
  meter around 0.113, and visible `held_star`, `held_lane`, and `held_tight`
  tail layers. The source-line-only full-venue diagnostic crashed before
  screenshots twice; a no-crowd retry and the measured-full diagnostic prove the
  source line path can render, but this remains diagnostic-only. Whammy
  local blob/ripple deformation remains parked for a PCSX2 data-derived pass.
- Normal non-whammy sustain tails now use the corrected PCSX2 4:3 red-tail
  layer split instead of treating the tail as one width: the lane-colored body
  uses the traced `lane_body` profile, the source-shaped mesh/detail uses the
  broader traced `silhouette` profile, and the thin held-tight core stays on
  top. The detail/silhouette now draws before the measured lane-colored fill so
  opaque source-detail pixels cannot reopen the hollow/two-rail middle. Latest
  focused 4:3 proof:
  `proofs/native_4x3_normal_tail_fill_comp_20260716_01`, with contact sheet
  `normal_tail_fill_comp_contact_sheet.png` and metric summary
  `normal_tail_fill_comp_summary.json`. Its no-tail-subtracted red held-body
  trace uses the authored held-detail mesh plus a 0.88x solid fill compensation
  and moves body-region median absolute error to 0.5 px against the corrected
  PCSX2 4:3 lane-body trace; the tight-core body-region median absolute error
  remains 0.5 px. The earlier center-order proof
  `proofs/native_4x3_normal_tail_center_order_patch_20260716_01`
  before/after center-fill scan moves the prior red body-only frame from a
  median largest red cluster of 3 px / median total red body of 7 px to a
  median largest red cluster of 17 px / median total red body of 20 px, while
  keeping the tight core as a separate thin layer. The previous visual proof is
  retained at `proofs/native_4x3_normal_tail_silhouette_patch_20260716_01`.
  The PCSX2
  trace source remains
  `proofs/pcsx2_4x3_red_normal_tail_corrected_20260716_01`, where the red
  normal tail measures `lane_body` median 8 px at 480p / 12 px scaled to 720p,
  `silhouette` median 14.5 px at 480p / 21.75 px scaled to 720p, and `core`
  median 2 px at 480p / 3 px scaled to 720p. The renderer also hard-gates
  broad `held_lane` glow for normal non-star held sustains so it cannot re-open
  the hollow/two-rail failure. Current combined 4:3 proof after this split:
  `proofs/native_4x3_current_combined_parity_20260716_08_fill_comp_current`.
  That refresh uses the same 16.0..20.0s scripted Shout at the Devil window
  after the fill-comp fix, still renders 960x720, and keeps the
  PCSX2 rail projection regression at width RMS 0.621769 px / center RMS
  0.080742 px over nine samples. Its full-scene red tail trace is retained as
  a context regression check only; the visible stack includes silhouette/detail
  plus cap/hit effects, so clean tail-width signoff remains the focused
  tail-only, center-order, and fill-comp proof folders.
  Whammy local ripple/blob deformation is still parked for data-derived work.
- Follow-up normal held-body width pass: the scene-backed native trace was
  contaminated by venue/highway pixels, so the focused proof now uses
  `GHOGX_DEBUG_HIGHWAY_ONLY_CAPTURE=1` plus a no-tail subtraction to isolate
  only the sustain body. Current focused proof:
  `proofs/native_4x3_normal_tail_highway_only_trace_20260716_02_scaled_from_diff`.
  The clean 960x720 tail-only trace moves the native held-body median from
  8.0 px to 12.0 px against the corrected PCSX2 4:3 red-tail lane-body median
  scaled to 12.0 px. Body-region median absolute error improves from 3.744 px
  to 1.5 px, core-body median absolute error improves from 5.152 px to
  1.339 px, and the tight-core median absolute error stays at 0.5 px with
  0.0 px core-body error. This proof is retained as the clean lane-body width
  calibration; the newer fill-comp proof above is the current visual/body-width
  pass and confirms the active normal tail no longer reads as two hollow rails.
  Both remain focused non-whammy tail passes rather than complete tail/gameplay
  signoff.
- Normal held-tail split-layer measurement hook: current proof
  `proofs/native_4x3_tail_layer_split_20260716_01` captures
  `held_body_detail`, `held_body_fill`, their legacy `held_body` alias, and a
  no-tail baseline in 4:3 highway-only mode. The no-tail-subtracted comparison
  shows the alias combined silhouette at 3.128 px body median absolute error
  versus the corrected PCSX2 red normal-tail silhouette, while the isolated
  solid fill lands at 1.713 px body median absolute error versus the PCSX2
  `lane_body` trace. Raw full-frame traces were rejected because the
  smasher/note-cap region contaminated the profile rows. This proof is a
  measurement hook for the next tail-width pass, not a rendering signoff.
- Active normal sustain note-head cap pass: PCSX2 4:3 reference frame
  `proofs/pcsx2_4x3_red_normal_tail_corrected_20260716_01/pcsx2_4x3_frame17_red_normal_tubed_source.png`
  shows a separate held note head at the near end of an active normal sustain,
  distinct from the fret/smasher button. The runtime had active tails and
  pressed smashers, but consumed sustains no longer drew the source note head.
  Normal active sustains now redraw the authored lane gem mesh plus the standard
  `top.mesh` as a highway-root child, limited to non-star/normal sustains and
  gated by `GHOGX_DISABLE_HIGHWAY_ACTIVE_SUSTAIN_CAPS` for A/B traces. The first
  placement pass anchored the note-head origin to the source `nowbar_tail_clip`
  near-tail point (`kStrikeY + nowbar_tail_clip_`) rather than the fret-center,
  matching the active-tail near clip. Focused proof:
  `proofs/native_4x3_active_sustain_cap_clip_anchor_20260717_01`, with cap-on,
  cap-off, diff, trace overlays, and contact sheet
  `active_sustain_cap_clip_anchor_contact_sheet.png`. The red full-frame
  `lane_body` comparison against the corrected PCSX2 trace improves near
  mean-absolute error from 12.093 px to 6.899 px and cap/effect max-absolute
  error from 37.771 px to 13.376 px. The full-silhouette classifier is unchanged,
  so this pass is accepted only as a near cap/lane-body correction, not as a
  complete outer-silhouette or whammy-ripple solution. Broad 4:3 proof:
  `proofs/native_4x3_current_combined_parity_20260717_02_active_cap/current_4x3_active_cap_contact_sheet.png`.
  That run saved frames 1/60/120/180 at 960x720 and showed no return of the
  broken plank/highway artifact. The refreshed 4:3 projection/fade regression
  for frame 60 is
  `proofs/native_4x3_current_combined_parity_20260717_02_active_cap/projection_compare.json`
  with `projection_overlay.png`; rail width RMS remains 0.621769 px, center
  RMS remains 0.080742 px over nine PCSX2 trace samples, and the measured fade
  top stays at 164.477 world-y with alpha distance 71.675. The bounded app
  process still returned a nonzero exit code after saving the frames, so the
  exit behavior remains a harness cleanup item rather than a rendering
  acceptance signal.
- Active normal sustain cap placement follow-up:
  `proofs/native_4x3_active_sustain_cap_placement_trace_20260717_01` isolates
  the cap with tails/rings hidden and compares it to the same PCSX2 4:3 red
  normal-tail reference. Source mesh bounds show `nowbar_tail_clip=1.5`,
  `red_gem min_y=-1.625`, and standard `top.mesh min_y=-3.298`. The accepted
  placement now draws the cap origin at
  `kStrikeY + nowbar_tail_clip_ - cap_min_local_y`, so the authored note-head
  stack floor lands on the source near-tail clip instead of sinking through the
  fret row. Contact sheet:
  `active_sustain_cap_placement_contact_sheet.png`; summary:
  `active_sustain_cap_placement_summary.json`. The measured red cap-center
  minus red ring-center gap moved from native current `-7.841 px` to `-21.754 px`
  at 720p; the rejected midpoint/floor-at-strike candidate was `-15.897 px`.
  The PCSX2 reference scaled to 720p measures `-32.644 px`, so this is accepted
  only as a source-backed placement improvement. Cap shape/scale, star/whammy
  cap behavior, and full normal-tail body parity remain open.
- Active normal sustain cap shape audit:
  `proofs/native_4x3_active_sustain_cap_shape_audit_20260717_01` recaptures
  the current final-code 4:3 active normal cap in full-context and isolated
  modes, then compares it to the same PCSX2 red normal-tail crop. The audit
  confirms the next visible mismatch is not just vertical placement: the PCSX2
  cap mask scales to roughly 186 px wide at 720p with a cap/ring width ratio of
  1.051 and a cyan/flare fraction of 0.161, while the current native isolated
  cap measures roughly 164 px wide, cap/ring ratio 0.916, and cyan fraction
  0.0. The center gap still remains short in this mask (`-18.956 px` native vs
  `-32.644 px` PCSX2 scaled). However, the runtime source log in the same proof
  reports `native note meshes: ... glow=1` but `moving note stack: standard
  top=1 glow=0`, and the source recheck shows the loaded cyan `star_base` /
  `gem_glow.tex` layer belongs to the star stack, not the standard normal note.
  No renderer patch was promoted from this audit: adding `gem_glow_mesh_`,
  `star_base_mesh_`, or another cyan flare to normal active sustain caps would
  be a screenshot guess until a PCSX2/source trace proves GH2 uses that layer
  for normal held caps. Contact sheet:
  `active_sustain_cap_shape_audit_contact_sheet.png`; summary:
  `active_sustain_cap_shape_audit_summary.json`.
- Active star sustain cap source-stack pass:
  `proofs/native_4x3_star_active_sustain_cap_source_20260717_01` uses the
  first Shout at the Devil Expert star sustain (`tick=4440`, red+blue,
  `5.929s`) as a 4:3 A/B window. The before trace confirmed the moving star
  stack from `gem_star.view` is `star_base.mesh`, lane `*_star.mesh`,
  `star2.mesh`, and `top_star_black.mesh`, while active sustains were skipping
  `star_power_tail` entirely. Star active sustain caps now draw that same
  authored stack at the source near-tail clip and sample the existing
  `star_base.tnm` animation for the base layer. The after trace logs both lanes
  as `base=1 star_mesh=1 overlay=1 top=1`, with `min_y=-2.460` and
  `y=3.960`; normal active caps remain on the normal gem/top stack. Visual
  proof: `star_active_sustain_cap_contact_sheet.png`; source trace:
  `star_active_sustain_cap_source_trace.txt`.
- Current normal-tail/cap retrace:
  `proofs/native_4x3_normal_tail_current_retrace_20260717_01` recaptures the
  Shout at the Devil Expert normal red+blue sustain (`tick=12120`, `16.111s`)
  after the active-cap and star-cap passes, using 4:3 960x720 full/no-tail
  frames and the same red-lane ROI/centerline as the corrected PCSX2 trace. The
  stable tail body remains close enough that no width/color patch was promoted:
  body median absolute error is `0.644 px` for silhouette, `2.218 px` for
  lane-body, and `1.0 px` for core at the normalized 720p target. The cap/effect
  region is still the visible mismatch (`8.366 px` silhouette median abs,
  `8.928 px` lane-body, `2.316 px` core), and the current native active normal
  cap still reads like a full moving gem stack beside the PCSX2 reference's
  smaller tail-end plate. Source inspection with
  `ghogx.exe mesh/groups --milo-path track/gen/track.milo_ps2` found
  `gem_template.view` contains `red_gem.mesh` and `top.mesh`, and found no
  separate normal sustain-cap mesh, so no renderer cap-shape patch was promoted
  from this pass. Next normal-cap work needs stronger PCSX2/GS evidence for how
  GH2 clips/reuses the standard gem/top stack at the held-tail end, not a
  hand-shaped replacement. Contact sheet:
  `normal_tail_current_retrace_contact_sheet.png`; summary:
  `normal_tail_current_retrace_summary.json`.
- Normal active sustain cap smasher-source pass:
  `proofs/pcsx2_normal_cap_gs_trace_20260717_01/frame17_tail_extract`
  extracts the embedded 640x480 frame from the stock PCSX2 GS dump
  `frame_17_Guitar Hero II_SLUS-21447_20260715100717.gs.zst` and traces the
  visible held-tail/cap component at `x=212..296`, `y=245..370` (`85x126 px`).
  That frame and the corrected PCSX2 red normal-tail crop both show the active
  held join as a red/white plate with cyan tail flare, not as the full round
  `red_gem.mesh + top.mesh` moving-note stack. The native pass
  `proofs/native_4x3_active_sustain_smasher_cap_20260717_01` switches normal
  active sustain caps to the authored `gem_smasher.mesh` body with the lane
  `gem_smasher_%s.mat` / `smasher_%s.tex` material, anchored to
  `nowbar_tail_clip_` and drawn at the flatter fixed now-ring height. The
  pressed-button `_1` smasher add layer was explicitly rejected for this cap
  because it produced a yellow burst absent from the PCSX2 reference. Runtime
  trace confirms the promoted branch logs `smasher=1 body=1 add=0` for lanes
  1/3 at tick 12120, while the A/B fallback logs the old `gem=1 top=1 glow=0`
  path. Visual proof: `active_sustain_smasher_cap_contact_sheet.png`; summary:
  `active_sustain_smasher_cap_summary.json`. This is a source-backed shape
  correction, not final cap parity; exact vertical/scale tuning still needs a
  stronger PCSX2/GS measurement before adding offsets.
- Normal active sustain cap measured fit:
  `proofs/native_4x3_active_sustain_smasher_cap_fit_20260717_02` keeps the
  source-backed `gem_smasher.mesh` normal active-cap body and applies the
  measured placement/scale correction derived from the previous isolated
  native cap diff plus the corrected PCSX2 cap/ring target. The unfitted
  native cap measured `+4.478 px` cap-center minus ring-center at 720p and
  `0.788` cap/ring width ratio, against PCSX2 target `-32.644 px` and `1.051`.
  The promoted renderer constants are `+6.26` world-y offset and `1.456` cap
  scale. The clean fitted proof measures the active cap from isolated on/off
  diff (`bbox=326,583..432,610`) and the ring from a matching cap-off frame
  minus isolated ring/tail-off frame (`bbox=312,580..417,669`), avoiding the
  earlier cap-contaminated ring denominator. The fitted gap is `-35.932 px`
  (`-3.288 px` from target) and the fitted width ratio is `1.009` (`-0.042`
  from target). Contact sheet:
  `active_sustain_smasher_cap_fit_contact_sheet.png`; summary:
  `active_sustain_smasher_cap_fit_summary.json`. This is accepted only as a
  measured cap placement/scale pass. The PCSX2 cyan flare/tail join, exact
  active-tail near clipping, and whammy ripple deformation remain open and
  must not be guessed from screenshots.
- Rejected normal active sustain join source-line profile:
  `proofs/native_4x3_active_sustain_join_audit_20260717_01` compares the
  current accepted cap-fit renderer against the corrected PCSX2 normal-tail
  trace in the same 4:3 red-lane ROI, then documents a rejected experiment from
  `proofs/native_4x3_active_sustain_join_line_profile_20260717_01`. The
  experiment tried applying the PCSX2 near-cap flare rows to the current normal
  `tail_glow_%s` source-line detail path. It was rejected because the output
  widened the wrong red layer: composite silhouette body median-absolute error
  moved from `1.0 px` to `1.594 px`, lane-body body median-absolute error moved
  from `3.0 px` to `5.505 px`, and the core cap/effect under-width was
  essentially unchanged (`2.078 px` median-absolute error). The current
  accepted code therefore remains on the measured cap-fit renderer without this
  line-profile change. Next normal join work should target a separate
  source-backed cyan/white join layer or draw-order/clip evidence, not broader
  red source-line detail. Contact sheet:
  `active_sustain_join_audit_contact_sheet.png`; summary:
  `active_sustain_join_audit_summary.json`.
- Normal active sustain tight-join pass:
  `proofs/native_4x3_active_tight_join_probe_20260717_01` first isolated the
  accepted current tight layers and showed the owner mismatch: native
  `held_tight_edge` stayed a 2-3 px straight cyan/white line through the near
  cap, while the stock PCSX2 GS dump in
  `proofs/pcsx2_normal_cap_gs_trace_20260717_01/frame17_tail_extract/tail_trace.json`
  widens the cyan/white component sharply in the last near-cap rows. The
  promoted patch adds a short normal-only `held_tight_join` layer using the
  existing `tail_glow_tight` source mesh/material, a 4.0 world-y span
  (`1.5..5.5`, about 24 px at 720p), and the GS near-cap width buckets scaled
  from 480p to 720p (`26.25 -> 58.5 -> 93.75 -> 45.0 -> 18.0 px`). It does
  not change the red lane body, incoming sustain width, star/whammy tails, or
  global tail width. Proof:
  `proofs/native_4x3_active_tight_join_patch_20260717_01`, with full-context
  screenshot, isolated join trace, corrected PCSX2 GS trace, and contact sheet
  `active_tight_join_patch_contact_sheet.png`. The isolated join trace measures
  a localized 26 px-tall component with near buckets `25.0`, `42.5`, `58.0`,
  and `25.5` px at 720p. The no-sustain-subtracted corrected PCSX2 comparison
  keeps the stable body/core in the accepted range: silhouette body median abs
  `0.862 px`, lane-body body median abs `1.936 px`, and core body median abs
  `1.0 px`. Focused tests pass:
  `ghogx_gameplay_venue_band_contract_test` and
  `ghogx_gameplay_session_test`. This is accepted only as a measured
  cyan/white near-cap join improvement; exact cap/effect silhouette and whammy
  ripple deformation remain open.
- Normal tight-join measured-profile routing fix:
  `proofs/native_4x3_tight_join_profile_route_20260717_01` fixes the renderer
  dispatcher so `TailWidthProfileKind::kNormalTightJoin` sections the short
  `held_tight_join` layer with `kPcsx2NormalTightJoinWidthProfile4x3` instead
  of falling through to the parked whammy body profile. The proof reuses the
  same strict 4:3 Shout at the Devil active sustain window (`tick=12120`,
  lanes red/blue, frame 60 at 960x720) and includes full-context gameplay,
  no-sustain baseline, clean cap/ring-off `held_tight_join` isolation, clean
  diff, lane-constrained trace overlay, and the stock PCSX2 GS source trace.
  The clean native join trace remains local (`bbox=343,622..396,643`) and
  measures bucket medians `29.0`, `52.0`, `48.0`, `21.5` px. This pass is a
  routing/profile correctness fix, not a final join-width signoff: the PCSX2
  GS source trace still contains a scaled near-cap peak around `93.75 px`, so
  exact cap/effect silhouette and active-tail near clipping remain open.
  Focused tests pass: `ghogx_gameplay_venue_band_contract_test` and
  `ghogx_gameplay_session_test`.
- Normal tight-join visible-width compensation:
  `proofs/native_4x3_tight_join_visible_comp_20260717_01` keeps the PCSX2 GS
  near-cap width curve as the visible target, then adds a separate normal-only
  geometry compensation curve for the `tail_tight.tex` source response
  (`1.000, 1.132, 1.960, 1.000, 1.000` at rel `0.00, 0.33, 0.66, 0.86,
  1.00`). The compensation values come from the previous clean native
  `held_tight_join` trace divided against the PCSX2 target curve, with
  endpoints left unexpanded. Fresh strict 4:3 proof uses the same Shout Expert
  active sustain frame (`tick=12120`, lanes red/blue, frame 60 at 960x720) and
  includes full gameplay, no-sustain baseline, before/after clean join traces,
  clean diff, and the stock PCSX2 GS trace. Clean native bbox width improves
  `54 -> 82 px`, max row width improves `53 -> 81 px`, mean absolute row error
  against the PCSX2 visible curve improves `14.703 -> 11.693 px`, and max row
  error improves `45.125 -> 29.482 px`. This is promoted as an incremental
  source-backed normal tight-join improvement, not a final silhouette signoff:
  the after trace still undershoots the PCSX2 peak (`81 px` native vs
  `93.75 px` target) and overshoots some shoulder rows. Exact cap/effect
  row-shape parity, active-tail near clipping, and whammy ripple remain open.
  Focused tests pass: `ghogx_gameplay_venue_band_contract_test` and
  `ghogx_gameplay_session_test`.
- Normal tight-join row-shape refinement:
  `proofs/native_4x3_tight_join_refined_20260717_01` keeps the same stock
  PCSX2 GS near-cap visible-width target and 4:3 Shout Expert active sustain
  frame, but draws `TailWidthProfileKind::kNormalTightJoin` with four
  subdivisions per measured profile interval so the short `tail_tight.tex`
  layer follows the PCSX2 row curve instead of spanning each section with one
  coarse quad. The normal-only geometry compensation curve is reshaped to
  `1.000`, `1.000`, `1.630`, `1.170`, `1.250` at rel `0.00`, `0.33`, `0.66`,
  `0.86`, `1.00`. Clean lane-constrained native join trace now measures
  bucket medians `28.5`, `47.0`, `85.5`, `31.5` px against target bucket means
  `28.577`, `52.019`, `78.230`, `31.812` px. Mean absolute row error improves
  from `11.693 -> 4.647 px` versus the previous visible-comp proof, median
  absolute row error improves `10.518 -> 3.557 px`, max row error improves
  `29.482 -> 14.798 px`, and signed mean error lands near neutral
  (`2.119 -> -0.336 px`). This is promoted as an incremental source-backed
  normal tight-join shape fix only; exact active cap/effect silhouette,
  broader color parity, and whammy ripple deformation remain open. Focused
  tests pass: `ghogx_gameplay_venue_band_contract_test` and
  `ghogx_gameplay_session_test`.
- Current normal-tail retrace after tight-join refinement:
  `proofs/native_4x3_normal_tail_current_retrace_20260717_02_tight_join`
  recaptures the same Shout Expert normal red+blue active sustain frame after
  the refined `held_tight_join` patch, using 4:3 960x720 full/no-tail frames
  and the same corrected PCSX2 red-lane trace. This is a measurement-only pass
  with no renderer patch promoted. The body remains close enough to avoid
  another body-width change: silhouette body median abs moves
  `0.644 -> 1.000 px`, lane-body body median abs `2.218 -> 3.000 px`, and core
  body median abs stays `1.000 -> 1.000 px`. The broader full-diff cap/effect
  classifier remains mixed rather than resolved: silhouette cap median abs
  worsens `8.366 -> 16.165 px`, lane-body cap median abs worsens
  `8.928 -> 9.981 px`, while core cap median abs slightly improves
  `2.316 -> 2.078 px`. The isolated tight-join proof is still accepted as the
  stronger local GS-backed measurement, but this composite retrace says the
  next cap/flare change needs stronger PCSX2/source evidence, not a guessed
  cyan flare or extra clipping rule. Contact sheet:
  `normal_tail_current_retrace_tight_join_contact_sheet.png`; summary:
  `normal_tail_current_retrace_tight_join_summary.json`. The compare helper
  now auto-pairs matching normal-tail layer labels on the PCSX2 side when all
  native inputs request the same layer and `--pcsx2-label` is omitted, which
  prevents accidental silhouette-vs-core/lane comparisons in future retraces.
- Native 4:3 cap/tail overlap audit:
  `proofs/native_4x3_cap_tail_overlap_audit_20260717_01` recaptures the same
  Shout Expert 4:3 frame with four renderer-isolation variants: full current,
  sustains disabled, active caps disabled, and `held_tight_join` only with caps
  disabled. The full-minus-no-tail diff isolates the tail stack; the
  full-minus-no-caps diff isolates the active smasher caps. Against the
  corrected PCSX2 red normal-tail trace, the tail-only body remains close
  enough that another body-width tweak is rejected: silhouette body median abs
  `1.0 px`, lane-body body median abs `2.0 px`, and core body median abs
  `1.0 px`. The unresolved error is near the cap/effect rows, especially
  silhouette and lane-body (`10.844 px` and `12.958 px` median abs), while core
  cap/effect stays close at `2.078 px`. The cap-only diff is a clean compact
  `107x28 px` bbox centered at `379.0,596.5`, so this pass also rejects moving
  the active smasher cap without stronger PCSX2/source evidence. Visual proof:
  `cap_tail_overlap_audit_contact_sheet.png`; summary:
  `cap_tail_overlap_audit_summary.json`; compact run trace:
  `cap_tail_overlap_audit_capture_trace.txt`.
- Native 4:3 active-cap source inventory:
  `proofs/native_4x3_active_cap_source_inventory_20260717_01` refreshes the
  same Shout Expert red+blue active sustain window with the current source mesh,
  smasher, active-cap, note-draw, and tail logs enabled. The run reaches live
  gameplay at `t=17.250` with one hit, zero misses, and a retained 960x720 4:3
  screenshot. The current normal active sustain cap path is confirmed as the
  measured smasher body-only branch: lanes 1 and 3 log
  `smasher=1 body=1 add=0 center_y=10.041 lower_y=7.760 scale=1.456`.
  The same trace confirms the normal fret smasher stack has source base/add/ring
  assets (`base=5 add=5 add_mesh=5 ring=5`) and the held red/blue smashers log
  the authored add masks and ring texture. This is an inventory guardrail, not a
  renderer patch: it proves the current cap is not accidentally using a missing
  add layer, but it does not prove the authored smasher add/ring stack owns the
  unresolved PCSX2 cyan/white cap/effect silhouette. The next cap/effect change
  still needs a multi-frame PCSX2 GS trace or source-object mapping that ties
  the wider near-cap shape to a specific authored layer. Visual proof:
  `active_cap_source_inventory_contact_sheet.png`; summary:
  `active_cap_source_inventory_summary.json`; compact trace:
  `active_cap_source_inventory_trace.txt`.
- PCSX2 normal active cap draw-group evidence:
  `proofs/pcsx2_normal_cap_drawgroups_20260717_01` reruns the GS GIF packet
  vertex tracer only for stock PCSX2 frame 17, distills the result to a compact
  summary, and deletes the temporary full trace. The embedded screenshot trace
  still selects the visible red/cyan sustain component at `x=212..296`,
  `y=245..370`, with the bottom rows blooming near the fret. The GS packet
  candidate list proves a nearby repeated cyan/white textured draw exists:
  eight 4-vertex draws use `tex0.tbp0=12384`, `tbw=2`, `psm=2`, `tw=7`,
  `th=5`, at `x=191..319`, `y=214..246`, paired with a dark additive-style
  pass. The overlay also shows that these candidate bboxes do not directly
  cover the full visible component, so this is accepted as source evidence for
  a missing/nearby active-sustain effect layer, not as a placement formula. No
  renderer patch was promoted from this pass; the next cyan-flare/cap change
  needs either a stronger multi-frame GS trace tying this draw to the visible
  component or a source asset/material route that maps `tex0.tbp0=12384` back
  to an authored highway object. Visual proof:
  `frame17_cap_drawgroups_native_pair_contact_sheet.png`; compact summary:
  `frame17_cap_drawgroups_summary.json`.
- PCSX2 normal cap texture-lead audit:
  `proofs/pcsx2_normal_cap_texture_probe_20260717_01` follows up on the
  draw-group evidence by reading frame 17's initial GS state with the official
  PCSX2 `GSLocalMemory` / `GSTables` swizzle rules for `PSMCT16`. The
  recovered `tex0.tbp0=12384` probe shows only a small nonzero mask-like corner
  and the candidate's 128x32 bbox is static across frames 15..17, so it is now
  rejected as a standalone basis for a cyan flare/cap renderer patch. The
  stronger unresolved lead is instead the only cyan-vertex textured group
  retained across all three frames: `tex0.tbp0=14208`, `psm=20`, `tw=8`,
  `th=7`, `cbp=13810`. Its projected bbox spans far outside the visible
  component, so vertex bbox alone cannot place it; the next PCSX2/source pass
  should recover that `PSMT4` texture with CLUT or map the `14208/13810` pair
  to an authored track material before changing renderer geometry or color.
  Visual proof: `normal_cap_texture_lead_audit_contact_sheet.png`; compact
  summaries: `frame15_17_cyan_texture_lead_summary.json` and
  `tbp0_12384_swizzle16_recovery_summary.json`.
- PCSX2 `14208/13810` PSMT4 recovery follow-up:
  `proofs/pcsx2_normal_cap_texture_probe_20260717_02_psmt4` ports the official
  PCSX2 `PSMT4` address math, row-table selector, `ReadPixel4` nibble order,
  and `clutTableT32I4` palette order to recover the unresolved
  `tex0.tbp0=14208`, `cbp=13810`, `tw=8`, `th=7` texture from frame 17. The
  recovered material is a low-alpha green/cyan indexed texture with noisy
  atlas-like content, not a clean normal sustain-tail body/cap material, and
  its closest extracted-source matches are weak star/smasher-style assets
  rather than a tail layer. A targeted GIF image-upload scan found the `TEX0`
  use at transfers 214 and 235 and no image payload overlapping block 14208 or
  13810 before the relevant draw range, so the recovery is accepted as the
  current best trace for this lead. No renderer patch was promoted from this:
  `14208/13810` is rejected as a standalone basis for normal tail geometry,
  color, or cap placement. Visual proof:
  `tbp0_14208_psmt4_recovery_contact_sheet.png`; summaries:
  `tbp0_14208_psmt4_recovery_summary.json` and
  `frame17_tbp0_14208_upload_scan_summary.json`.
- PCSX2 active cap/effect candidate correlation:
  `proofs/pcsx2_active_cap_candidate_correlation_20260717_01` reuses the
  existing rebuilt GS packet trace for frames 15-17 rather than launching PCSX2
  again. The ranked tail-like GS candidates in all three frames are the same
  repeated `tex0.tbp0=12384`, `psm=2`, `tw=7`, `th=5` strip at
  `x=191..319`, `y=214..246`, eight draws per frame. The visible unresolved
  normal active cap/effect mismatch is lower, in the near-cap ROI
  `x=175..330`, `y=300..420`; the `12384` candidates have zero overlap with
  that ROI in frames 15, 16, and 17. This keeps `12384` rejected as the active
  near-cap owner. Together with the recovered `14208/13810` PSMT4 rejection
  above, this means the next cap/effect patch still needs a different draw
  group, recovered texture, or source object whose bbox/material follows the
  actual near-cap visible component across frames. Visual proof:
  `pcsx2_active_cap_candidate_correlation_contact_sheet.png`; summary:
  `pcsx2_active_cap_candidate_correlation_summary.json`; compact trace:
  `pcsx2_active_cap_candidate_correlation_trace.txt`.
- PCSX2 lower near-cap ROI trace:
  `proofs/pcsx2_near_cap_roi_trace_20260717_01` reruns the GS GIF packet
  tracer over the same stock PCSX2 frames 15-17, this time ranking the lower
  active cap/effect ROI (`x=175..330`, `y=300..420`) instead of the earlier
  upper tail strip. A fresh native 4:3 frame 60 was captured for the same
  Shout Expert active sustain window and retained in the contact sheet. The
  PCSX2 embedded frame still shows the visible red/cyan sustain component in
  that lower region, but the packet-bbox trace reports zero tall/narrow
  tail-like candidates in all three frames: frame 15 `total=6724`,
  `visible=264`, `roi_intersecting=120`, `tall_roi_intersecting=0`;
  frame 16 `total=6769`, `visible=265`, `roi_intersecting=120`,
  `tall_roi_intersecting=0`; frame 17 `total=6737`, `visible=264`,
  `roi_intersecting=120`, `tall_roi_intersecting=0`. The previous
  `tex0.tbp0=12384` candidate remains an upper strip at `x=191..319`,
  `y=214..246`, above this lower ROI. This rejects a lower-cap renderer patch
  from the current GS bbox evidence: retained lower-ROI intersections are broad
  clipped groups, not a mapped cap/effect owner. The next cap/effect pass needs
  either a more precise GS raster/triangle-overlap trace or a source-object
  mapping that follows the visible lower component before changing cap
  geometry, cyan flare, or smasher add/ring behavior. Visual proof:
  `near_cap_roi_trace_contact_sheet.png`; compact trace:
  `near_cap_roi_trace.txt`; summary: `near_cap_roi_trace_summary.json`.
- PCSX2 lower near-cap triangle-overlap trace:
  `proofs/pcsx2_near_cap_triangle_trace_20260717_01` reruns the same stock
  PCSX2 frames 15-17 with enough retained GIF vertices to reconstruct the
  traced triangle strips and score individual triangle bboxes against the
  lower active cap/effect ROI. This makes the previous bbox-only rejection
  stricter: each frame has `roi_triangles=772`, `lower_bbox_hits=772`, and
  `visible_component_bbox_hits=764`, but all hits are huge clipped strips and
  `narrow_component_hits=0` in frames 15, 16, and 17. The top overlap in all
  three frames is a broad `tbp0=14176`, `psm=19`, `tw=7`, `th=6` triangle with
  bbox around `x=-747..1903`, `y=-1824..2272`, not a cap/effect-sized owner.
  No renderer patch was promoted: this proves geometry/triangle overlap alone
  still cannot identify the visible lower cap plate. The next cap/effect pass
  must recover texture-alpha/raster ownership or map the relevant GS texture
  key back to an authored source object before changing cap geometry, cyan
  flare, or smasher add/ring behavior. Visual proof:
  `near_cap_triangle_trace_contact_sheet.png`; compact trace:
  `near_cap_triangle_trace.txt`; summary:
  `near_cap_triangle_trace_summary.json`.
- PCSX2 `14176/13812` PSMT8 recovery follow-up:
  `proofs/pcsx2_tbp0_14176_psmt8_recovery_20260717_01` follows the broad
  lower-ROI triangle lead above through the official PCSX2 `PSMT8`
  local-memory address path and CLUT ordering. The target
  `tex0.tbp0=14176`, `cbp=13812`, `psm=19`, `tw=7`, `th=6` recovers a
  non-empty 128x64 index pattern with `unique_indices=23`, but the sampled
  palette/alpha state is fully transparent (`nonzero_alpha_pixels=0/8192`),
  so source RMSE comparisons against the candidate tail/smasher textures are
  unavailable. A targeted GIF upload scan finds four `TEX0` uses for this key
  across frame 17's transfer stream and zero image payloads touching the
  texture or palette blocks (`image_hits=0`, `transfer_count=4094`). No
  renderer patch was promoted: this rejects `14176/13812` as a standalone
  basis for cap geometry, cyan flare, smasher add/ring, or tail color. The
  next cap/effect pass still needs a texture-alpha/raster owner or authored
  source-object mapping that follows the visible lower component, not the
  broad clipped triangle alone. Visual proof:
  `tbp0_14176_psmt8_recovery_contact_sheet.png`; compact traces:
  `tbp0_14176_psmt8_recovery_trace.txt` and
  `frame17_tbp0_14176_upload_scan_trace.txt`; summaries:
  `tbp0_14176_psmt8_recovery_summary.json` and
  `frame17_tbp0_14176_upload_scan_summary.json`.
- PCSX2 lower near-cap raster/compact-owner probe:
  `proofs/pcsx2_near_cap_raster_probe_20260717_01` reuses the same stock GH2
  PCSX2 frames 15-17 and the accepted embedded-frame red/cyan/white component
  mask, then parses path-3 GIF draw groups with every vertex retained
  (`max vertex_count=48`) and scores exact triangle coverage of the selected
  component pixels for groups intersecting the lower active-cap ROI. This makes
  the previous bbox/triangle-overlap rejection stricter but still negative:
  frame 17 selects the visible sustain/cap component at `x=212..296`,
  `y=245..370`, finds 120 lower-ROI draw groups and 80 component-covering draw
  groups, but `0` cap-sized (`<=260x220`) and `0` medium (`<=420x520`) groups
  cover the component. The smallest covering draws are full-screen
  `512x448` passes or broad clipped strips, so geometry alone cannot identify a
  cap/effect owner. No renderer patch was promoted: this rejects moving,
  widening, recoloring, or adding smasher/cyan layers from screen-space
  triangle coverage alone. The next cap/effect pass needs either texture-alpha
  raster ownership, a VRAM/image-upload owner, or a source-object mapping that
  follows the visible component. Visual proof:
  `near_cap_raster_probe_contact_sheet.png`; compact traces:
  `near_cap_raster_probe_trace.txt` and
  `near_cap_raster_probe_compact_trace.txt`; summaries:
  `near_cap_raster_probe_summary.json` and
  `near_cap_raster_probe_compact_summary.json`.
- Rejected normal tight-edge width follow-up:
  `proofs/native_4x3_normal_tail_tight_edge_width_20260717_01` tried moving
  `kNormalHeldTightEdgeWidthScale4x3` from 0.6 to 0.8 while keeping the accepted
  tight-layer alpha values. The isolated full-minus-no-tail 4:3 trace showed
  the intended strict-core target got worse: core median absolute error moved
  from 1.5 px to 2.0 px, core-body median absolute error from 1.08 px to
  2.0 px, and near-core median absolute error from 1.79 px to 3.5 px. Although
  lane-body near error improved slightly, this was collateral from a brighter/
  broader edge and not a valid fix for the PCSX2 core. The renderer and contract
  were restored to the accepted 0.6 width; do not retry a simple width-only
  expansion without a new PCSX2/source trace explaining the core classifier.
- Source color-path recheck:
  `proofs/native_4x3_color_source_recheck_20260717_01` has been regenerated
  and distilled after the current color/source-material work. The strict 4:3
  run exits 0, captures frames 1/60/120 at 960x720, and retains only the PNG
  contact sheet, compact trace, summary JSON, and exit marker; raw BMPs and
  verbose logs were deleted after distillation. The compact trace confirms the
  source chain: `track_graphics.dtb` supplies slot names, material formats,
  tail widths, camera, and fade values; the runtime samples lane body colors
  from authored gem textures as `e12cc947/e1f32f3a/e1fbe200/e16eacfb/e1f86917`;
  red/blue held-tail detail uses `tail_glow_red.mat` and
  `tail_glow_blue.mat` with `line01.tex`; and held red/blue fret buttons draw
  source smasher body/ring meshes. The searched `track_graphics`, `player`,
  and `scoring` DTB dumps do not expose a separate per-lane RGB table, and the
  only explicit per-lane gem specular meshes decoded from source remain
  red/yellow/orange. Therefore this pass rejects inventing green/blue specular
  overlays, force-enabling source lighting, or applying a global lane-brightness
  multiplier as a color fix. Broader note/button color parity remains open, but
  future brightness changes need PCSX2/source texture-owner evidence instead of
  screenshot tuning.
- Current 4:3 projection/fade refresh:
  `proofs/native_4x3_projection_fade_refresh_20260717_01` reruns the current
  executable in strict 4:3 at 960x720 with the highway alignment and fade
  profile diagnostics enabled. The compact trace confirms the source-backed
  profile is live (`aspect=1.333333`, `root_x_scale=1.027691`,
  `cam_x=-0.041889`, `fade_top_y=164.480`, `alpha=71.675`). The projection
  overlay compares the current frame against the accepted PCSX2-derived rail
  trace and still measures width RMS `0.621769 px` and center RMS `0.080742 px`
  over nine samples. This is accepted only as a 4:3 root projection/far-fade
  regression proof; it does not sign off sustain-tail body shape, whammy
  deformation, note/button color parity, or audio behavior.
- Current 4:3 projection/fade refresh after tight-join refinement:
  `proofs/native_4x3_projection_fade_current_20260717_02_tight_join`
  recaptures the same Shout Expert 16.0s gameplay window after the normal
  tight-join row-shape work. The projection overlay still matches the accepted
  PCSX2 rail model at width RMS `0.621769 px`, width max abs `0.937 px`,
  center RMS `0.080742 px`, and center max abs `0.176 px` over nine samples.
  The far fade remains at target screen Y `326.666631 px`, world top
  `164.477297`, and alpha distance `71.67506`. No projection/fade renderer
  patch was promoted: this proves the latest tail/cap work did not reintroduce
  the wrong-angle/far-fade highway regression. Contact sheet:
  `projection_fade_current_contact_sheet.png`; summary:
  `projection_fade_current_summary.json`. The projection trace helper now also
  writes these RMS/max values directly under `projected_rail_fit.metrics` so
  future projection proofs do not need a sidecar metric script.
- Current gameplay/audio feedback refresh:
  `proofs/native_4x3_gameplay_audio_feedback_current_20260717_01` reruns the
  focused explicit-ARK 4:3 audio/gameplay proof after the active-cap and color
  source-state renderer passes. `bad_pick_miss_recovery` uses the raw diagnostic
  script `4.967:0x20,5.000:0x00,5.430:0x21,5.470:0x01`, exits 0, saves five
  960x720 frames, and logs the GH2 ingame bank with `miss_gtr=1`,
  `sp_gemhit=1`, and `sp_awarded=1`. It proves `bad_pick route=miss_gtr`, an
  overstrum, missed-note guitar-stem mute, recovery-hit unmute, and the final
  summary `hits=1 misses=3 overstrums=1 sp=0.000 failed=0`. The
  `star_phrase_clean` window uses the chart-derived
  `4.800:11.400:-0.0083` script, exits 0, saves five 960x720 frames, logs three
  `sp_gemhit` submissions, logs `star_phrase_complete route=sp_awarded`, awards
  `star phrase complete sp=0.25`, and finishes with `hits=8 misses=0
  overstrums=0 sp=0.250 failed=0`. Contact sheet:
  `gameplay_audio_feedback_current_20260717_01_contact_sheet.png`; compact
  summary: `gameplay_audio_feedback_current_20260717_01_summary.json`. This is
  a current gameplay/audio contract proof, not a waveform-level PCSX2 audio
  comparison or whammy-tail visual parity pass.
- Current missed-star-phrase/no-stinger guard:
  `proofs/native_4x3_star_phrase_miss_no_stinger_20260717_01` fills the
  missing negative half of the current gameplay/audio proof. It uses a strict
  4:3 960x720 Shout Expert window and the diagnostic guitar-script hook
  `5.760:0x21,5.805:0x00,5.920:0x2a,5.965:0x0a,6.500:0x2e,6.540:0x0a,9.610:0x30,9.660:0x00,10.249:0x21,10.300:0x00`
  to hit the early star notes, overstrum inside the phrase, hit the final star
  note, and then cross the boundary on the next normal note. The trace logs the
  GH2 ingame bank with `miss_gtr=1`, `sp_gemhit=1`, and `sp_awarded=1`, logs
  three `star_gem_hit route=sp_gemhit` submissions, one
  `bad_pick route=miss_gtr` submission, one `star_phrase_miss` event, and a
  final `sp=0.000` summary. It explicitly finds zero
  `star_phrase_complete`, zero `star_phrase_complete route=sp_awarded`, and
  zero `route=sp_awarded` lines, proving the stinger is not submitted after the
  phrase is broken. The same run also logs a later missed-note guitar-stem mute.
  Retained proof files are the PNG contact sheet, compact trace, full log,
  summary JSON, and exit marker; raw BMPs and duplicate stdout/stderr were
  deleted after distillation.
- Current 4:3 motion/root/fade trace:
  `proofs/native_4x3_motion_root_fade_trace_20260717_01` reruns the Shout
  Expert 16.0s window with the highway alignment, fade, surface-scroll, and
  tail diagnostics enabled. The strict 4:3 preset renders at 960x720 and logs
  15 lane-alignment records across frames 1/30/60; all lanes report
  `root_shared=1`, all aspects are `1.333333`, and the maximum smasher-to-
  strike X delta is `0.030 px`. The surface-scroll trace logs three samples
  with `same_pace=1`, `note_dy=-191.163`, and `surface_dy=-191.163`, proving
  the moving notes and authored track surface features are still traveling in
  the same direction and at the same world pace. The far fade remains on the
  source-backed projection constants (`root_x_scale=1.027691`,
  `cam_x=-0.041889`, `fade_top_y=164.480`, `fade_alpha_dist=71.675`), and the
  board fade-top sample is at screen Y `326.67 px` with `fade=0.000`. No
  renderer patch was promoted from this pass; it is accepted only as a
  regression proof for the previous broken-highway/wrong-motion concern.
  Contact sheet: `motion_root_fade_trace_contact_sheet.png`; compact trace:
  `motion_root_fade_trace.txt`; summary:
  `motion_root_fade_trace_summary.json`.
- Current normal sustain tail/cap status proof:
  `proofs/native_4x3_current_tail_cap_status_20260717_01` captures the same
  strict 4:3 Shout Expert 16.0s chart-script sustain window at frame 60, then
  splits the renderer into full, no-sustain, no-cap, held-body-only, and
  tight-join-only views. Raw BMPs/logs and rejected generic traces were deleted
  after PNG/contact-sheet/JSON distillation. The contact sheet confirms the
  highway/root is stable, the broken plank artifact is absent, and the current
  note/tail color path is visually closer than earlier passes. The accepted
  metrics use `trace_normal_tail_layers.py` on unboosted
  full-minus-no-sustain and full-minus-no-caps diffs against the corrected
  PCSX2 red normal-tail trace. The tail-only silhouette body remains close
  (median abs `1.0 px`, core-body median abs `1.078 px`), but the separated
  lane-body and core classifiers do not support another renderer promotion
  (`lane_body` body median abs `5.202 px`, `core` body median abs `14.0 px`),
  and the cap/effect rows remain unresolved (`silhouette` cap/effect median
  abs `10.844 px`, `core` cap/effect median abs `24.727 px`). No renderer patch
  was promoted from this pass: it is a current visual/status proof only, not a
  cap/effect owner trace, whammy deformation trace, or final tail signoff.
  Contact sheets: `current_tail_cap_status_contact_sheet.png` and
  `current_tail_cap_trace_contact_sheet.png`; summary:
  `current_tail_cap_status_summary.json`.
- Current normal sustain layer-decomposition guardrail:
  `proofs/native_4x3_tail_layer_decomposition_20260717_01` reuses the current
  strict 4:3 frame-60 split above, but traces positive layer-minus-no-sustain
  diffs instead of raw layer screenshots. The raw screenshot trace was rejected
  because fret buttons and note-head remnants contaminated the red lane body
  component. The clean held-body positive diff compares against the corrected
  PCSX2 red `lane_body` trace with stable body median abs `1.936 px`, core-body
  median abs `2.5 px`, and far-edge body median abs `1.276 px`, so another
  body-width or body-color renderer tweak is rejected for now. The tight-join
  positive diff only exposes two short lower rows and cannot derive a new
  PCSX2 core/cap formula; its apparent core error is not accepted as a width
  patch basis. This keeps the next real renderer target on lower cap/effect
  ownership or a source-mapped tight-join/cap layer, not the stable red body.
  Contact sheet: `tail_layer_decomposition_contact_sheet.png`; summary:
  `tail_layer_decomposition_summary.json`.
- Normal active sustain smasher-rim source experiment:
  `proofs/native_4x3_active_sustain_smasher_rim_experiment_20260717_01`
  extracts the relevant `track.milo_ps2` source object map and confirms
  `gem_smasher.view` owns `gem_smasher.mesh`, `smasher_rim.mesh`, and the
  smasher transforms, while `smash_normal.view` separately owns the hit
  particles plus `smash_flamelight` mesh/material/animations. A default-off
  diagnostic hook, `GHOGX_EXPERIMENT_HIGHWAY_ACTIVE_SUSTAIN_SMASHER_RIM=1`,
  draws the source-owned `smasher_rim.mesh` with `now_ring.tex` on the current
  normal active sustain cap without enabling the pressed-button smasher add
  flash. The strict 4:3 A/B trace logs default `rim=0` and experiment `rim=1`
  for lanes 1/3 at the same Shout Expert active sustain frame; the A/B diff is
  localized to bbox `303,578..646,612` with RGB RMS `9.198`. This experiment
  is not promoted as a default visual change: it proves source membership, but
  the existing PCSX2 GS raster probe still does not tie the unresolved
  cyan/white near-cap component to the now-ring/smasher-rim draw group, and
  visually the rim adds a broad red outline rather than solving the cap/tail
  join. Retained proof: `active_sustain_smasher_rim_experiment_contact_sheet.png`;
  compact trace: `active_sustain_smasher_rim_experiment_trace.txt`; source map:
  `active_sustain_smasher_rim_source_map.txt`; summary:
  `active_sustain_smasher_rim_experiment_summary.json`. Focused tests pass:
  `ghogx_gameplay_venue_band_contract_test` and
  `ghogx_gameplay_session_test`.
- Rejected normal tight-join refinement:
  `proofs/native_4x3_tight_join_refined2_20260717_01` tried one more measured
  adjustment to the existing normal-only `held_tight_join` geometry
  compensation curve (`0.33 -> 1.025`, `0.66 -> 1.540`) to reduce the current
  refined proof's shoulder/peak row errors against the PCSX2 GS visible-width
  profile. The strict 4:3 isolated join trace rejected it: median absolute
  row-width error worsened from the accepted refined proof's `3.557 px` to
  `9.205 px`, mean absolute error worsened from `4.647 px` to `8.862 px`, and
  the bucket medians moved to `24.0, 47.5, 82.5, 31.5` instead of improving on
  the accepted `28.5, 47.0, 85.5, 31.5`. The renderer was restored to the
  accepted `1.000/1.630/1.170/1.250` compensation profile. Retained proof:
  `tight_join_refined2_rejected_contact_sheet.png`; summary:
  `tight_join_refined2_rejected_summary.json`; trace:
  `trace_join_clean_red_lane/native_lane_tail_trace.json`. Focused tests pass:
  `ghogx_gameplay_venue_band_contract_test` and
  `ghogx_gameplay_session_test`.
- PCSX2 cap/tail local-owner audit:
  `proofs/pcsx2_cap_tail_owner_audit_20260717_01` rebuilds the stock GH2
  PCSX2 frame 15-17 GS draw-group trace with ST/UV/TEX0/ALPHA/TEST state and
  96 retained vertices per group, then ranks only local small/medium draw boxes
  against the known visible sustain/cap ROIs. This pass keeps the same
  repeatable local owner as the earlier cap probe: the cyan/white top strip
  `tbp0=12384`, `psm=2`, `tw=7`, `th=5`, bbox `x=191..319`, `y=214..246`,
  with a paired dark alpha pass and the neighboring `tbp0=12448` upper strip.
  It overlaps the lower visible sustain/cap column (`x=212..296`,
  `y=245..370`) by only `84 px^2`, and no small/medium draw group covers that
  lower component below `y=246`. The broad clipped alpha/texture strips still
  intersect the ROI, but remain rejected as renderer-patch sources without
  texture-alpha raster ownership or a source-object mapping. No renderer patch
  was promoted: keep the smasher-rim diagnostic default-off, do not add a cyan
  flare, and do not change normal tail/cap geometry from this pass. Fresh
  strict 4:3 native proof is included at `native_current_frame_00060.png`.
  Contact sheet: `pcsx2_cap_tail_owner_audit_contact_sheet.png`; compact
  trace: `pcsx2_cap_tail_owner_audit_compact_trace.txt`; summary:
  `pcsx2_cap_tail_owner_audit_summary.json`. The 89 MB temporary draw-group
  JSON and raw native BMP were deleted after distillation.
- Current 4:3 source material/color audit:
  `proofs/native_4x3_source_material_color_audit_20260717_01` reruns the
  strict 4:3 Shout Expert 16.0s sustain window with source mesh, tail, and note
  draw diagnostics enabled, then distills the UTF-16 runtime logs into a compact
  trace and deletes the raw logs/BMP. The proof confirms the current color path
  is still DTB/MILO driven: `track_graphics.dtb` supplies slot order
  `green/red/yellow/blue/orange`, source tail widths `tail=1.500` and
  `tight=0.700`, and the runtime samples lane ARGB colors from the decoded
  `gem_%s.tex` textures as
  `e12cc947/e1f32f3a/e1fbe200/e16eacfb/e1f86917`. Standard moving gems all use
  the source `gem.tex` atlas with lane-specific UVs, blend `3`, zmode `1`,
  and authored `intensify=1`; star notes use `stargem.tex`, `gem_glow.tex`,
  `gem_specular.tex`, and the black star top alias from source layers. The only
  explicit per-lane gem specular meshes decoded from `track.milo_ps2` are
  `red_gem_spec`, `yellow_gem_spec`, and `orange_gem_spec`; `green_gem_spec`
  and `blue_gem_spec` remain absent in source. Held-tail line materials come
  from `tail_glow_%s.mat` with source colors
  green `(0.141,0.753,0.388)`, red `(1.000,0.412,0.102)`,
  yellow `(0.960,0.880,0.090)`, blue `(0.235,0.310,0.761)`, orange
  `(1.000,0.651,0.310)`, and whammy `(0.000,0.400,0.500)`. No renderer patch
  was promoted: this rejects inventing green/blue specular overlays or applying
  a global lane-brightness multiplier from screenshot comparison alone. Future
  note/button color work needs a PCSX2/source contradiction sharper than
  "looks dimmer." Contact sheet: `source_material_color_audit_contact_sheet.png`;
  compact trace: `source_material_color_audit_trace.txt`; summary:
  `source_material_color_audit_summary.json`.
- Current 4:3 source object/tail map:
  `proofs/native_4x3_source_object_tail_map_20260717_01` extracts only
  `track/gen/track.milo_ps2` to temp, inventories the authored source objects
  with the existing `ghogx groups/mesh/mats` tools, then deletes the raw MILO,
  BMPs, and full logs after distilling the proof. The strict 4:3 screenshots
  cover default, `held_body`-only, and `incoming_body`-only tail layers at the
  Shout Expert 16.0s frame-60 sustain window. Source findings: `track.milo_ps2`
  decodes as 106 meshes, 20 particles, 130 materials, and 30 groups;
  `tail02.mesh` is the only sustain ribbon mesh and has 12 vertices/10 faces
  with center vertices, so the normal-tail "two outer rails" failure mode is
  not caused by a missing middle mesh in source; normal lane tails bind
  `tail_%s.mat` -> `gem_star.tex` with blend `3`, zmode `1`, texgen `5`,
  intensify, and lane material colors; active held `tail_glow_%s.mat` binds
  `line01.tex` with blend `4` and source lane colors. `gem_smasher.view`
  contains `gem_smasher.mesh`, `smasher_rim.mesh`, `gem_smasher.tnm`, and
  `gem_smasher_hit.tnm`, with no separate lower active-cap mesh found, while
  `smash_normal/star/bonus.view` own the authored hit particles and flamelight
  meshes. No renderer patch was promoted: this pass narrows remaining
  normal-tail/cap work to PCSX2-backed UV/texgen/blend/order/placement evidence,
  not missing source geometry or another screenshot-only shape/color guess.
  Contact sheet: `source_object_tail_map_contact_sheet.png`; filtered trace:
  `native_tail_cap_trace_filtered.txt`; source summaries:
  `track_milo_source_groups.txt`, `track_milo_source_meshes.txt`,
  `track_milo_source_materials.txt`; summary:
  `source_object_tail_map_summary.json`.
- PCSX2 normal-tail draw-state ROI filter:
  `proofs/pcsx2_normal_tail_drawstate_20260717_01` rebuilds the stock GH2
  frame-17 GS dump vertex/state trace in temp, filters it to the accepted red
  normal-tail ROI, then deletes the full parser JSON after distilling a compact
  trace, overlay, current native 4:3 screenshot, and contact sheet. The parse
  completed cleanly (`225536` vertices, `448` visible vertices, `0` errors).
  A broad ROI pass still intersects many clipped/full-screen texture strips,
  but the strict local draw-sized filter finds only 16 candidates and all are
  the repeated upper-strip passes already seen in cap probes: `tbp0=12384`,
  `psm=2`, `tw=7`, `th=5`, bbox `191,214..319,246`, plus the adjacent
  `tbp0=12448`, `tw=7`, `th=4`, bbox `191,198..319,214`. The focused lower
  body ROI has `small_focus_groups=0`, so the visible lower normal-tail body is
  not backed by a simple local strip draw group in this dump. No renderer patch
  was promoted: this rejects adding another guessed local strip, rim, flare, or
  cap/tail shape from the frame-17 draw-state trace. The useful follow-up is
  texture-alpha/source-object ownership for the broad clipped groups or a
  specific investigation of `tail_%s.mat`'s source `texgen=5` path; source
  materials prove that field exists, while `RuntimeMesh` currently does not
  carry it, but this pass does not yet prove the PS2 texture-coordinate
  behavior needed for a visual patch. Contact sheet:
  `pcsx2_tail_drawstate_contact_sheet.png`; compact trace:
  `pcsx2_frame17_tail_drawstate_compact_trace.txt`; findings:
  `pcsx2_normal_tail_drawstate_findings.json`.
- Current 4:3 tail texgen/source audit:
  `proofs/native_4x3_tail_texgen_source_audit_20260717_01` captures a fresh
  strict 4:3 Shout Expert 16.0s sustain proof after the color-pipeline pass and
  distills the run to a compact tail trace, PNG, contact sheet, and summary.
  The proof remains 960x720, shows no recurrence of the broken plank/highway
  regression, and keeps the current normal-tail middle fill visible. Source
  audit findings: ihatecompvir's `RndMat` source maps material `texgen=5` to
  `kEnviron`/`kTexGenEnviron`; GH2 `track.milo_ps2` normal tail materials
  `tail_%s.mat` bind `gem_star.tex` with blend `3`, zmode `1`, texgen `5`,
  intensify, UV scale `[0.5000,1.0000]`, and lane colors; held-detail materials
  `tail_glow_%s.mat` bind `line01.tex` with blend `4`, zmode `0`, texgen `0`,
  wrap `1`, and their own lane colors. `tail02.mesh` raw UVs still decode with
  constant `u=0` and cross-tail `v` values, while the current renderer
  reconstructs a cross-tail U for `tail02.mesh`; that workaround is therefore
  not a proven 1:1 implementation of source `texgen=5`. No renderer patch was
  promoted because the exact environment-coordinate math and PCSX2
  texture-owner mapping are still unproven. Future tail work should either
  recover the `Tail::UpdateVerts` UV rewrite/texgen math from source or tie the
  broad clipped PCSX2 draw groups back to `tail_%s.mat` texture ownership before
  changing tail UVs again. Contact sheet:
  `tail_texgen_source_audit_contact_sheet.png`; screenshot:
  `native_current_4x3_frame_00060.png`; trace:
  `tail_texgen_source_audit_trace.txt`; source texture stats:
  `tail_source_texture_stats.txt`; summary:
  `tail_texgen_source_audit_summary.json`.
- PCSX2 tail texture-owner trace:
  `proofs/pcsx2_tail_texture_owner_trace_20260717_01` adds
  `tools/trace_gsdump_texture_uploads.py` and runs it against the same stock GH2
  frame-17 GS dump plus the full temporary vertex trace. The tool tracks
  `BITBLTBUF`/`TRXPOS`/`TRXREG`/`TRXDIR`, hashes image payload GIF tags, and
  maps ROI draw `TEX0.tbp0` pages to the latest observed in-frame upload at the
  same DBP page. The frame parses cleanly (`4094` transfers, `0` errors) and
  contains only 5 path-3 image payload uploads, to DBP pages `11541`, `11104`,
  `11576`, `10662`, and `11575`. The 143 ROI draw references instead use
  persistent texture pages such as `14208`, `14176`, `12384`, `5632`, `15744`,
  `15616`, `12448`, `14944`, `14352`, `13824`, and `11424`; none maps to an
  in-frame upload owner. The known local upper-strip groups remain
  `tbp0=12384` / `tbp0=12448` (`psm=2`, `tw=7`, `th=5/4`,
  `128x32`/`128x16`) and also have no in-frame owner. No decoded source BMP
  payload hash matched any image payload, and a direct byte search of the GS
  state did not find raw decoded `gem_star`, `gem_bonus`, `line01`,
  `tail_tight`, `tail2`, or `gem` payload variants, so persistent owner work
  now needs GS-state/VRAM layout decoding rather than transfer-event matching
  alone. No renderer patch was promoted: normal-tail body ownership remains
  unproven, so UV/material changes would still be guesswork. Contact sheet:
  `pcsx2_texture_owner_contact_sheet.png`; compact trace:
  `pcsx2_frame17_texture_owner_compact_trace.txt`; full compact JSON:
  `pcsx2_frame17_texture_owner_trace.json`; findings:
  `pcsx2_texture_owner_findings.json`.
- PCSX2 frame-17 GS-state VRAM page trace:
  `proofs/pcsx2_vram_page_trace_20260717_01` decodes the persistent texture
  pages from the saved GS state instead of searching only in-frame GIF image
  uploads. The layout is derived from PCSX2 `GSState::Freeze`: saved GS
  registers, `m_mem.m_vm8`, four saved `GIFPath` tag/reg pairs, then `m_q`.
  For this dump the state is `4194813` bytes, so the VRAM block starts `425`
  bytes into the state (`1229279` absolute dump offset). The pass decodes 11
  ROI texture pages using PCSX2's block/pixel swizzle and 32-bit CSM1 CLUT
  ordering. Useful positive IDs from the contact sheet are venue/HUD pages,
  including `tbp0=14208` as a wood-like `PSMT4` page, `tbp0=15744/15616` as
  brick/wall pages, and `tbp0=11424` as the rock-meter texture; this proves the
  broad ROI page list is noisy and must not be treated as tail ownership. The
  local upper-strip pages from the draw-state pass, `tbp0=12384` and
  `tbp0=12448`, decode as near-empty/black `PSMCT16` pages under the captured
  `TEXA` state (`TA0=0`, `TA1=128`) and still do not map to a named tail source
  BMP. No renderer patch was promoted: this pass is a data-backed owner/noise
  filter, not a proven normal-tail material/UV fix. Follow-up tail work should
  isolate the actual lower sustain body from broad clipped geometry or recover
  the source `Tail::UpdateVerts`/`texgen=5` path before changing tail rendering
  again. Contact sheet: `pcsx2_frame17_vram_page_contact_sheet.png`; compact
  trace: `pcsx2_frame17_vram_page_trace.txt`; summary:
  `pcsx2_frame17_vram_page_trace.json`.
- Native 4:3 tail material traceability patch:
  `proofs/native_4x3_tail_material_traceability_20260717_01` carries the
  source `RndMat` state that matters for tails through runtime diagnostics:
  `RuntimeMesh` now preserves `texgen`, and `RuntimeLineMaterial` preserves
  `zmode`, `texgen`, `intensify`, and sampler wrap. The line-tail quad path
  now binds source intensify/wrap too; current GH2 line/tight materials still
  report `texgen=0`, `intensify=0`, `wrap=1`, so this pass is traceability,
  not a guessed visual change. Direct `track.milo_ps2` material output confirms
  the split: broad normal/star/bonus body tails (`tail_%s.mat`,
  `tail_star.mat`, `tail_bonus.mat`) use `gem_star`/`gem_bonus` with
  `texgen=5` and intensify, while held/tight/whammy glow layers
  (`tail_glow_%s.mat`, `tail_glow_tight.mat`, `tail_glow_whammy.mat`) use
  `texgen=0` additive line/tight textures. The new 960x720 4:3 proof frame
  shows no broken-highway regression. Screenshot:
  `native_current_4x3_frame_00060.png`; compact runtime trace:
  `tail_material_traceability_trace.txt`; direct material query:
  `track_tail_materials.txt`; summary:
  `tail_material_traceability_summary.json`.
- Tail source-gap audit:
  `proofs/native_4x3_tail_source_gap_audit_20260717_01` follows the material
  traceability patch back into ihatecompvir's public source instead of guessing
  at `texgen=5` or whammy deformation. The useful source-backed structure is
  limited to RB3 `GemRepTemplate`: it stores `mNumTailSections`,
  `mTailSectionLen`, `mTailVerts`, and `mCapVerts`; `CreateTail` reserves a
  dynamic sectioned mesh from `mTailVerts.size() * (1 + mNumTailSections)`;
  and `SetupTailVerts` starts from `tail02.mesh`, requires an even vertex
  count, sorts the verts, copies cap verts, then resizes both vectors to 420.
  The public source tree does not expose `Tail::UpdateVerts`, `Tail.cpp`, or
  the bodies for `Gem::UpdateTailPositions`/`Gem::ApplyDuration`, and `re-gh2`
  has no matching tail implementation. A fresh hidden strict-4:3 Shout Expert
  frame-60 capture reached gameplay at 960x720 and logs the current runtime
  split: broad body tails keep `texgen=5`/`intensify=1`, while held/tight/
  whammy line materials keep `texgen=0`, additive blend, wrap, and
  `intensify=0`. No renderer patch was promoted: dynamic tail section/cap
  structure is real, but exact GH2 tail UV, cap/effect ownership, and whammy
  ripple math remain unproven. Contact sheet:
  `tail_source_gap_audit_contact_sheet.png`; screenshot:
  `tail_source_gap_audit_frame_00060.png`; source trace:
  `tail_source_gap_audit_source_trace.txt`; runtime trace:
  `tail_source_gap_audit_runtime_trace.txt`; summary:
  `tail_source_gap_audit_summary.json`.
- Current 4:3 color/button source trace:
  `proofs/native_4x3_color_button_trace_20260717_01` reruns the strict 4:3
  Shout Expert red/blue sustain window at 960x720 with note mesh, note draw,
  tail, and smasher diagnostics. The fresh frame shows no broken-highway
  regression and keeps the current source-driven color path: lane colors are
  sampled from decoded `gem_%s.tex`, standard notes draw the native `gem.tex`
  body plus `top.mesh`, active red/blue tails use the held-tail source
  line/fill stack, and pressed red/blue buttons log native smasher body/ring
  meshes. The pass also checked the tempting `gem_%s_1.mat` specular lead:
  `red_gem_spec`, `yellow_gem_spec`, and `orange_gem_spec` decode from source,
  while `green_gem_spec` and `blue_gem_spec` remain absent; however
  `gem_template.view` still lists only `red_gem.mesh` and `top.mesh`, and the
  PCSX2 frame-10 texture-owner attempt could not map the visible draw pages to
  a named `gem_specular`/`gem_%s_1` source texture because the relevant pages
  are resident rather than uploaded in-frame. No renderer patch was promoted:
  do not add moving-note specular overlays or global brightness multipliers
  without a stronger PCSX2/source ownership trace. Contact sheet:
  `color_button_trace_contact_sheet.png`; screenshot:
  `color_button_trace_frame_00060.png`; runtime trace:
  `color_button_trace_runtime_trace.txt`; PCSX2 candidate trace:
  `pcsx2_note_specular_candidate_trace.txt`; summary:
  `color_button_trace_summary.json`.
- Tail/cap owner guardrail after color pass:
  `proofs/native_4x3_tail_owner_guard_20260717_01` adds a contract guard around
  the rejected active-sustain smasher-rim path. The renderer still contains the
  diagnostic `GHOGX_EXPERIMENT_HIGHWAY_ACTIVE_SUSTAIN_SMASHER_RIM` hook, but
  `gameplay_venue_band_contract_test` now pins the default active sustain cap
  to `cap_ring_mesh=nullptr` and requires the rim path to be opt-in only. A
  fresh strict 4:3 Shout Expert frame-60 capture at 960x720 logs normal held
  tail layers (`held_body_detail`, `held_body_fill`, `held_tight_join`, and
  `held_tight_edge`) plus red/blue active sustain caps with `smasher=1`,
  `add=0`, and `rim=0`. The pass also carries forward the PCSX2 owner rejection:
  the repeatable local upper-strip pages `tbp0=12384/12448` recover as
  near-empty/black pages with no named tail source BMP, and the previous
  frame-17 owner audit found no small/medium draw group for the lower cap
  column. No renderer visual patch was promoted: do not enable smasher rims,
  cyan flare, or lower-cap geometry by default until the lower owner or source
  `Tail::UpdateVerts`/texgen path is recovered. Contact sheet:
  `tail_owner_guard_contact_sheet.png`; screenshot:
  `tail_owner_guard_frame_00060.png`; runtime trace:
  `tail_owner_guard_runtime_trace.txt`; PCSX2 rejection trace:
  `pcsx2_tail_owner_rejection_trace.txt`; summary:
  `tail_owner_guard_summary.json`.
- Normal active sustain cap grounding fix:
  `proofs/native_4x3_active_cap_grounded_fix_20260717_01` addresses the
  red/blue "floating cap" regression visible in the tight-layer diff sheet and
  the current 4:3 visual guards. The root cause was the normal active-sustain
  cap overlay: after a successful hit, the grounded pressed smasher was already
  drawn on the fret row, then the unresolved smasher-derived active-cap branch
  drew a second plate using the previous `+6.26` world-y / `1.456` scale fit.
  That made the note-button art appear to shoot upward on button press. Default
  normal non-star sustains now skip that overlay unless
  `GHOGX_EXPERIMENT_HIGHWAY_NORMAL_ACTIVE_SUSTAIN_CAPS` is explicitly enabled;
  star active sustain caps remain on their authored star stack. The focused 4:3
  Shout Expert proof reaches live gameplay, hits tick `12120`, logs held red
  and blue smashers, and logs `0` default `highway-active-sustain-cap` rows.
  Visual proof:
  `active_cap_grounded_fix_contact_sheet.png`; crop:
  `frame_00060_strikeline_crop.png`; compact trace:
  `active_cap_grounded_fix_trace.txt`; summary:
  `active_cap_grounded_fix_summary.json`. This is a regression fix only: it
  does not solve lower cap/effect ownership, cyan flare attribution, or whammy
  ripple deformation.
- Active sustain cap source gate:
  `proofs/native_4x3_active_cap_source_gate_20260717_01` tightens the previous
  normal-only grounding fix. ihatecompvir's local `rb3` source shows
  `GemRepTemplate::CreateTail` building sustain tails from `mTailVerts` and
  `SetupTailVerts` copying `tail02.mesh` verts into both `mTailVerts` and
  `mCapVerts`, but the local source drop still does not include
  `Tail::UpdateVerts`; `re-gh2` has no matching `GemRepTemplate`/`mCapVerts`
  implementation. That means the separate active sustain note-head/smasher cap
  pass is not a source-proven gameplay layer. Default rendering now requires
  `GHOGX_EXPERIMENT_HIGHWAY_ACTIVE_SUSTAIN_CAPS` before drawing any active
  sustain note-head/cap overlay, while the older disable gate remains for A/B
  traces. Fresh strict 4:3 proof captures frames 1/30/60 at `960x720`,
  reaches `state=playing` at `t=17.500`, and logs `0`
  `highway-active-sustain-cap` rows even with
  `GHOGX_DEBUG_HIGHWAY_ACTIVE_SUSTAIN_CAPS=1`; the same trace still logs
  `26` tail rows and `15` smasher rows, proving the held sustain and pressed
  smasher path was exercised. Visual proof:
  `active_cap_source_gate_contact_sheet.png`; crop:
  `frame_00060_strikeline_crop.png`; compact trace:
  `active_cap_source_gate_trace.txt`; summary:
  `active_cap_source_gate_summary.json`. Lower cap/effect ownership remains
  open and should be solved by tail-cap source recovery or stronger PCSX2 GS
  attribution, not by re-enabling the separate cap overlay by default.
- Normal-tail color-pass retrace / empty-core guard:
  `proofs/native_4x3_normal_tail_colorpass_retrace_20260717_01` refreshes the
  strict 4:3 Shout Expert normal red+blue sustain window after the color pass,
  using full/no-sustain and layer-only captures against the corrected PCSX2 red
  normal-tail trace. The current native frame still shows solid red/blue tails
  and no broken-highway regression, but the strict positive full-minus-no-tail
  trace exposes only the colored body as a stable component: silhouette body
  median abs is `4.0 px`, lane-body body median abs is `2.0 px`, and the native
  core comparison now has `0` rows because the cyan/white core does not survive
  the current diff/mask as a component. Runtime rows still emit
  `held_tight_edge` and `held_tight_join`, but isolated positive diffs for
  those layers are empty under the same trace. No renderer patch is promoted:
  the existing backlog already rejects simple tight-edge width expansion and
  broader red source-line/profile changes, so the next visual change needs
  stronger PCSX2/source evidence for the hidden core blend/owner path. The
  compare helper now treats empty layer components as explicit empty profiles
  instead of crashing, so future retraces can carry this evidence directly.
  Contact sheet: `normal_tail_colorpass_retrace_contact_sheet.png`; compact
  trace: `normal_tail_colorpass_retrace_trace.txt`; summary:
  `normal_tail_colorpass_retrace_summary.json`.
- Tight-tail cross-U fix:
  `proofs/native_4x3_tight_tail_cross_u_20260717_01` promotes the
  PCSX2-backed cross-tail texture coordinate path only for
  `tail02.mesh` + `tail_glow_tight.mat`. The source mesh dump still shows
  constant raw `u=0` on `tail02.mesh`, but the PCSX2 frame-17 local cyan strip
  groups span `ST 0..1`; using the decoded raw U sampled the black edge of
  `tail_tight.tex` and made `held_tight_edge`/`held_tight_join` nearly
  invisible. The new strict 4:3 Shout Expert frame-60 pass keeps broad
  `tail02` texgen reconstruction behind `GHOGX_EXPERIMENT_HIGHWAY_TAIL_SYNTH_U`
  while defaulting only the tight material to reconstructed cross-tail U.
  Evidence: the previous empty-core retrace measured only `52` boosted
  edge-diff pixels at max `6`; this pass measures `966` raw edge pixels at max
  `202` and `620` raw join pixels at max `253`. PCSX2 core comparison now has
  rows again: edge-only core-body median abs is `1.5 px`, and full-composite
  core-body median abs is `1.0 px` against the corrected PCSX2 red normal-tail
  core. The held-body-only capture is retained as visual proof, but the current
  lane/warm masks classify it as `no_tail_rows`, so this pass must not be used
  as a new body-width proof. Contact sheet:
  `tight_tail_cross_u_contact_sheet.png`; screenshot:
  `full/frame_00060.png`; metrics: `tight_tail_cross_u_metrics.json`;
  source-backed comparisons: `compare_edge_core/tail_profile_compare.json` and
  `compare_full_core/tail_profile_compare.json`.
- Post-cross-U normal body/core trace:
  `proofs/native_4x3_post_cross_u_tail_body_trace_20260717_01` reuses the
  post-cross-U strict 4:3 frame-60 screenshots to remeasure the normal red
  sustain body with `trace_normal_tail_layers.py`, because the broader
  lane-tail helper had classified the held-body-only layer as `no_tail_rows`.
  The normal-tail layer trace confirms that was a mask/tool limitation, not a
  missing body: held-body lane-body rows are measurable across 180 rows. Against
  the corrected PCSX2 frame-17 red sustain trace, stable body rows remain close
  enough that another body-width/color renderer patch is rejected:
  held-body lane-body body median abs is `0.822 px`, core-body median abs is
  `0.961 px`, and far-edge body median abs is `0.722 px`. The restored tight
  core also holds: full-composite core body median abs is `1.0 px`, and
  edge-only core body median abs is `1.0 px`. The large remaining mismatch is
  still near the cap/effect region (`12.923 px` held-body cap/effect median abs
  and `16.575 px` full-silhouette cap/effect median abs), which stays assigned
  to the unresolved lower cap/effect owner path rather than tail-body geometry.
  No renderer patch was promoted. Contact sheet:
  `post_cross_u_tail_body_trace_contact_sheet.png`; summary:
  `post_cross_u_tail_body_trace_summary.json`; comparisons:
  `compare_held_body_lane_body/tail_profile_compare.json`,
  `compare_full_silhouette/tail_profile_compare.json`, and
  `compare_full_core/tail_profile_compare.json`.
- Smasher TransAnim source audit:
  `proofs/native_4x3_smasher_transanim_audit_20260717_01` extracts only
  `track/gen/track.milo_ps2` long enough to inspect `gem_smasher.view`,
  `smash_normal.view`, `gem_smasher.tnm`, and `gem_smasher_hit.tnm`, then keeps
  only compact trace/proof files. Source membership confirms
  `gem_smasher.view` owns `gem_smasher.tnm`, `gem_smasher_hit.tnm`,
  `gem_smasher.mesh`, and `smasher_rim.mesh`, while `smash_normal.view` owns
  the separate normal-hit particle/flamelight stack. Both smasher TransAnims
  decode as short translation-only Z pulses on `gem_smasher.mesh`: no scale
  curve, no rotation curve, and no material/color/cyan/rim/add ownership
  evidence. No renderer patch was promoted. Do not use these `.tnm` files as a
  basis for changing active sustain cap color, cyan flare, rim/add behavior,
  lower cap geometry, or tail width. Contact sheet:
  `smasher_transanim_audit_contact_sheet.png`; source map:
  `smasher_transanim_source_map.txt`; trace:
  `smasher_transanim_audit_trace.txt`; summary:
  `smasher_transanim_audit_summary.json`.
- PCSX2 lower-cap visibility trace:
  `proofs/pcsx2_lower_cap_visibility_trace_20260717_01` improves
  `trace_pcsx2_lower_cap_owner.py` so lower-cap owner probes rank recovered
  texture-page signal separately from raw triangle coverage. The tool now
  samples known recovered pages including `tbp0=12384/12448`, records
  point-sampled texture color/alpha over the selected component, estimates
  alpha-test pass fraction, and demotes black/no-signal clears. High-retention
  stock GH2 frame 15-17 GIF traces were rebuilt temporarily and deleted after
  compact proof creation. The stricter trace still does not find a stable local
  lower-cap owner: the top visible signals are broad clipped texture pages and
  vary by frame (`tbp0=15616` on frame 15, `tbp0=14944` on frame 16, and
  `tbp0=14208` on frame 17), with all three winners reporting
  `local_bbox=false` and broad-geometry penalties. No renderer patch was
  promoted. Treat this as stronger negative evidence against another
  screen-space lower-cap/cyan-flare patch; the next real cap/effect change
  still needs source-object ownership, full GS blend/depth attribution, or a
  recovered `Tail::UpdateVerts`/texgen formula. Contact sheet:
  `lower_cap_visibility_contact_sheet.png`; summary:
  `lower_cap_visibility_trace_summary.json`; retained per-frame traces:
  `frame15_visibility/lower_cap_owner_trace.txt`,
  `frame16_visibility/lower_cap_owner_trace.txt`, and
  `visibility_trace/lower_cap_owner_trace.txt`.
- PCSX2 lower-cap color-model trace:
  `proofs/pcsx2_lower_cap_color_model_trace_20260717_01` extends
  `trace_pcsx2_lower_cap_owner.py` with a diagnostic-only color-model rank
  over raw vertex RGB, raw texture RGB, and simple texture*vertex modulation
  candidates. This is still not a GS blend/depth replay. On the accepted stock
  GH2 frame-17 component (`[212,245,296,370]`, `1813` pixels), `60` draw groups
  cover the component. The prior visibility lead remains broad/page-level
  `tbp0=14208` with full coverage, but its best raw-texture RGB MAE is about
  `92 px`. The best color-model rows are also broad/page-level pages
  (`tbp0=14944` at about `77.8 px` MAE and `tbp0=15616` at about `78.1 px`
  MAE), with `local_bbox=false` and `broad_penalty=2.0`. No local or
  source-owned lower cap/effect draw is identified, so no renderer patch is
  promoted. Treat this as stronger negative evidence against using the
  `14208/13810` lead, another cyan flare, or another smasher/tail cap geometry
  guess as a default visual change. Contact sheet:
  `lower_cap_owner_contact_sheet.png`; trace:
  `lower_cap_owner_trace.txt`; full compact summary:
  `lower_cap_owner_summary.json`.
