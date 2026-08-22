# Guitar Hero II current state

### 2026-08-21 Midori retirement pause: r181-r182 arm findings

- User requested commit/push and retirement of the Midori effort until a future
  LLM model. The active goal remains incomplete.
- No ISO was mounted or used. r181/r182 used loose-DLC staging only.
- Builder priority was tightened: `tools/gh3_midori_build_fullclip_coupled_contact_candidate.py`
  now launches `milo_convert_tool.exe` subprocesses with
  `IDLE_PRIORITY_CLASS`.
- Added `--skip-contact-patches` to both fullclip builders so arm-only
  diagnostics can leave guitar/fret/strum contact channels unchanged.
- r181 non-live outfit-1 attack candidate was built and captured:
  `analysis/gh3_midori_gh2_milos/gh3_midori_1_main_staticface_attack_r181.milo_ps2`,
  SHA256 `9CD65B3D781331B3DAEC7D616B9C591E8A90A14A63612019071458474FE4F105`.
  It proved per-case `main_milo` review/staging works, but visual verdict is
  rejected. Screenshot:
  `analysis/gh3_midori_r181_outfit1_attack_proofs/midori_1_attack_left_r181_f030.bmp`.
  Decision:
  `analysis/gh3_midori_r181_outfit1_staticface_attack_visual_decision.json`.
- r182 non-live outfit-1 arm-only attack candidate was built:
  `analysis/gh3_midori_gh2_milos/gh3_midori_1_main_staticface_armonly_attack_r182.milo_ps2`,
  SHA256 `BB05A8641909AC3BF70B9F35BBB3FE1058BA89C8469E0915878DCD270977B403`.
  It is built-not-captured and not approved. Status:
  `analysis/gh3_midori_r182_outfit1_staticface_armonly_attack_status.json`.
- Rebuildable r181/r182 donor MILOs and ACP scratch directories were removed.
- Git blocker: workspace root `.git` is an empty directory with no `HEAD` or
  `config`; `git rev-parse --show-toplevel` fails. Nested repos exist but do
  not own the Midori state/tools/analysis files. Commit/push from this
  workspace is blocked until repository metadata or a remote is restored.

### 2026-08-21 Midori r180 static-face arm tooling pivot

- Face crop work is paused for the current phase. The user rejected the face
  animation result because changing calls distort the face in the wrong
  direction. Treat face as static/no-op and do not include `face_clip` in the
  next arm diagnostic capture.
- No r180 MILO was built or promoted. Live DLC remains unchanged.
- Patched `tools/gh3_midori_pose_review.py` so custom pose-review cases may
  provide `main_milo`, `strum_milo`, `fret_milo`, and `face_milo` paths. This
  removes the review-side assumption that both Midori outfits must use the same
  `gh3_midori_main.milo_ps2`.
- Patched `tools/gh3_midori_capture_with_loose_dlc_backup.py` with
  `--extra-candidate-file SOURCE_NAME=DLC_REL_PATH` for temporary loose-DLC
  staging of additional candidate banks. Existing destinations are restored;
  destinations created only for the capture are deleted afterward.
- Patched both fullclip builders with repeatable `--case-name` filters so
  outfit-/pose-specific arm banks can be built without hand-editing solve
  reports or weakening duplicate-clip conflict checks.
- Runtime/manifest finding: addon variants already parse independent
  `main_anim` fields and gameplay receives the selected variant
  `main_anim_path`, so outfit-specific main animation banks are expressible
  without Midori-specific runtime code.
- ihatecompvir position remains unchanged: use local MiloLib/GH2 notes as
  reference material, but public glTFMilo is not a drop-in GH2 PS2 final
  converter. GLB can be an automated intermediate if it feeds the GH2
  `CharClipSamples` writer/validator.
- Retained decision artifact:
  `analysis/gh3_midori_r180_staticface_arm_tooling_decision.json`.
- A non-live r181 outfit-1 arm-bank build was attempted with the new
  `--case-name` filter and stopped during the slow MILO replace stage for lag
  control. No r181 MILO output was retained; temporary ACP scratch was deleted.
  The next run should pass the pinned r176 `--source-bridge` path.
- Next branch is an arms-only capture with no face layer and, preferably,
  distinct outfit-specific main-bank paths for outfit 1 and outfit 2.

### 2026-08-21 Midori r179 shared-attack upper-chain diagnostics

- No r179 MILO was built or promoted. Face remains static/no-op, and live DLC
  remains unchanged.
- Exposed `--solve-visible-clavicles`, `--visible-clavicle-blend`, and
  existing hand-rotation toggles in
  `tools/gh3_midori_score_visible_forearm_planes.py` so render-feedback
  scoring can test upper-chain channel leverage instead of only left forearm
  positions.
- The first broad mode sweep was stopped because it was too slow for the
  value. Retained compact report:
  `analysis/gh3_midori_r179_shared_attack_upperchain_compact_report.json`.
  It tested the best shared-attack vectors across five modes:
  forearm only, clavicle blend 0.5, clavicle blend 1.0,
  clavicle+target-hand rotation, and clavicle+axis-hand rotation. Every mode
  still selected the same shared vector and the same bad max score `53.3016`.
- Retained duplicate-signature report:
  `analysis/gh3_midori_r179_shared_attack_upperchain_duplicate_signature_report.json`.
  With one shared vector, both outfit attack rows emit identical patch
  signatures even when clavicle/hand channels are enabled, so the issue is not
  duplicate ACP conflict; it is that the same emitted pose renders differently
  against the two outfit meshes.
- Metadata inspection of the current review/build path shows both selections
  use the same `gh3_midori_main.milo_ps2` clipset and both character MILOs
  expose the same visible arm skeleton names. Under current ordinary
  DLC/tooling, `gh3_guit_mido_a_attackl` is a shared animation.
- Decision artifact:
  `analysis/gh3_midori_r179_upperchain_shared_attack_decision.json`.
- Next branch should build a true shared-clip objective that evaluates actual
  rendered captures for both outfits, or investigate authoring separate
  outfit-specific main clipsets in MILO. Do not spend more cycles on simple
  forearm-only or existing clavicle-toggle sweeps for `attackl`.

### 2026-08-21 Midori r177-r178 render-aware arm scorer diagnostics

- No MILO was built or promoted in r177/r178. Face remains static/no-op, and
  live DLC remains unchanged.
- Patched `tools/gh3_midori_score_visible_forearm_planes.py` to accept
  r176 render feedback:
  `--render-feedback-candidate-report` and
  `--render-feedback-meshpart-summary`. It now predicts rendered part13 center
  from the captured part13-minus-elbow offset and scores candidate vectors by
  predicted part13 height/front/far penalties in addition to joint target
  distance.
- Added a sparse render-feedback grid around the observed r176 vector and the
  useful r123 seeds. Added shared-clip conflict resolution so duplicate main
  clips choose one vector by minimax score instead of writing incompatible
  per-case map entries.
- Patched both fullclip builders to default to `--duplicate-clip-policy error`
  when multiple solve rows would patch the same main clip with conflicting
  values. This prevents silent last-row-wins ACP output during diagnostics.
- Retained r177 report-only artifacts:
  `analysis/gh3_midori_r177_visible_forearm_renderfeedback_grid_shared_score_report.json`
  and
  `analysis/gh3_midori_r177_visible_forearm_renderfeedback_grid_shared_override_map.json`.
- r177 shared map is not build-worthy. The shared `gh3_guit_mido_a_attackl`
  clip must serve both `midori_1_attack_left_f030` and
  `midori_2_attack_left_f030`; the minimax shared vector still scores
  `53.3016`, too high to justify a visual build.
- Ran focused r178 shared-attack grid over the current forearm-vector family.
  Retained:
  `analysis/gh3_midori_r178_shared_attack_forearm_grid_report.json` and
  `analysis/gh3_midori_r178_shared_attack_forearm_grid_decision.json`.
  Best shared vector was `[-1.1737,-5.1629,-4.8704]`, still max score
  `53.3016`. No r177/r178 MILO was built.
- Next branch should stop treating the attack failure as independent
  per-outfit forearm placement. Solve `gh3_guit_mido_a_attackl` as one shared
  clip objective across both outfits, likely requiring upper/clavicle/hand
  rotation channels or confirming whether separate outfit-specific main
  animation assets can be authored in MILO.

### 2026-08-21 Midori r176 per-case forearm-plane capture rejected

- Captured r176 through `tools/gh3_midori_capture_with_loose_dlc_backup.py`
  using temporary loose-DLC substitution only; no ISO was mounted or used.
  The wrapper restored live DLC afterward and live main hashes still match
  static-face SHA256
  `2902C712F81ED32A8D7D14E147453F16196C7AE935E5F3D029C179648A36AC8B`.
- Capture cases were the five r176-patched diagnostics:
  `midori_1_attack_left_f030`, `midori_1_fast_jump_f040`,
  `midori_1_fast_solo_f090`, `midori_1_transition_out_f050`, and
  `midori_2_attack_left_f030`.
- Retained contact sheet:
  `analysis/gh3_midori_r176_staticface_arm_percase_forearm_sheet.jpg`.
  Sheet manifest:
  `analysis/gh3_midori_r176_staticface_arm_percase_forearm_sheet_manifest.json`.
  Pose review manifest copy:
  `analysis/gh3_midori_r176_staticface_arm_percase_forearm_pose_review_manifest.json`.
- r176 is rejected. Visually, attack still folds the left arm across/into the
  guitar-body area, jump/solo are upright but still not solved performance-arm
  poses, transition occludes/replaces the arm silhouette with the guitar, and
  second-outfit attack is visibly non-bipedal/tilted.
- Mesh summary:
  `analysis/gh3_midori_r176_staticface_arm_percase_forearm_meshpart_summary.json`.
  The rendered part13 signal confirms the visual reject:
  `midori_1_attack_left_f030` part13 center `[8.19383,2.48217,56.6356]` with
  `59` positive-y face-band vertices; `midori_1_fast_jump_f040` part13 center
  `[-10.9078,3.43151,55.4187]` with `44` positive-y face-band vertices.
  `midori_1_fast_solo_f090` and `transition_out` clear the face band better
  but still fail visually.
- Decision artifact:
  `analysis/gh3_midori_r176_staticface_arm_percase_forearm_visual_decision.json`.
- Next branch should make the forearm-plane scorer capture/render-aware:
  select candidate vectors by rendered part13/hand mesh bounds and occlusion,
  not only solved joint world targets, before building r177.

### 2026-08-21 Midori r175-r176 per-case forearm-plane arm candidate

- Face remains static/no-op. This branch only advances visible arms from the
  accepted torso/head chain outward; guitar attachment remains last.
- Added `--visible-arm-forearm-override-map` to both fullclip builders, keyed
  by diagnostic case name, so forearm-plane overrides no longer have to be one
  global vector across attack/jump/solo/transition.
- Added `tools/gh3_midori_score_visible_forearm_planes.py`. It runs a
  no-MILO solver screen over r123 reference vectors, treats rejected
  `r122_base` as reference-only, and generates per-case fitted left-forearm
  vectors targeting hand-relative safe height.
- New retained reports:
  `analysis/gh3_midori_r175_visible_forearm_plane_score_report.json` and
  `analysis/gh3_midori_r175_visible_forearm_override_map.json`.
  Selected left vectors:
  `attack`/`midori_2_attack` `[-1.1737,-5.1629,-10.8704]`,
  `fast_jump` `[4.8223,-7.7020,7.4480]`,
  `fast_solo` `[6.6046,-4.0225,11.7787]`,
  `transition_out` `[-5.1,-2.2,18.4]`.
- Built non-live r176 candidate with that map:
  `analysis/gh3_midori_gh2_milos/gh3_midori_main_staticface_arm_percase_forearm_r176.milo_ps2`,
  SHA256 `26917D91D9E4A1511622D2B4B5210F4E7ECABD50D3B168AF80EF36CEA78380C0`,
  size `20234418`. Candidate report:
  `analysis/gh3_midori_r176_staticface_arm_percase_forearm_candidate_report.json`.
- r176 was not copied into live DLC and has not been visually captured/reviewed
  yet. Next step is a loose-DLC capture sheet using this exact MILO, then
  reject/promote based on direct rendered bipedal arm silhouette. No ISO was
  mounted or used; Python and converter work were kept below-normal priority.

### 2026-08-21 Midori r174 static-face arm objective started

- User rejected continued face crop work. Face is now explicitly deferred as
  static/no-op for the current phase; GH2 has fewer face controls than
  Neversoft, so later work should build a small translation/control layer
  rather than forcing GH3 facial controls directly.
- Priority order for the active branch is arms from the already accepted torso
  chain outward: pelvis/root through head remains regression-only, then solve
  visible arms/wrists/hands/fingers, with guitar attachment last.
- Added opt-in fullclip/seed builder arguments:
  `--visible-arm-left-forearm-guitar-local` and
  `--visible-arm-right-forearm-guitar-local`. These allow the r123 low/back
  forearm plane to be reproduced in the automated fullclip path instead of only
  through the older scratch overlay tool.
- Compile check passed for
  `tools/gh3_midori_build_fullclip_coupled_contact_candidate.py`,
  `tools/gh3_midori_build_fullclip_candidate_from_seed_acp.py`, and
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`.
- A full r174 candidate build was attempted at low priority but produced no
  ACP/donor/MILO/report artifacts before being stopped; only zero-byte logs
  were left and then cleaned. No ISO was mounted or used.
- Created report-only objective artifact:
  `analysis/gh3_midori_r174_staticface_arm_lowback_objective_report.json`.
  It runs the current five-case arm solver with r172's fitted source-palm hand
  target mode, source-pose elbow hints, and r123 `lowback_old`
  `--visible-arm-left-forearm-guitar-local -5.1,-2.2,18.4`. Live DLC was not
  changed.
- Report-only result: the override is now reproducible in the fullclip solver,
  but the resulting per-case forearm world target is not obviously safe across
  all clips (`fast_jump`/`fast_solo` move the left forearm high/back again).
  Next useful branch should score/choose forearm plane per case from rendered
  part13/hand bounds before building a visual candidate, instead of applying
  one global lowback vector blindly.

### 2026-08-21 Midori r172-r173 fit-stock source-palm diagnostics rejected

- Active goal remains open. Commands ran at below-normal/low priority. No ISO
  was mounted or used; captures used temporary loose-DLC staging with only the
  tested main MILO substituted. Face remains intentionally static/no-op.
- Fixed source bridge compatibility in
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`: bridge readers now match
  either `bone` or `source_bone`, which is required for merged hand/guitar
  source bridge records.
- Added
  `--visible-arm-target-mode source-palm-fit-stock-hand-targets-per-case` and
  `--visible-arm-source-coordinate-map` to the full-clip and seed-ACP builders.
  The mode maps per-case source IK-helper hand spacing onto GH2 stock
  fret/strum hand target locals, then transforms source palm positions through
  that fitted frame.
- Preview report:
  `analysis/gh3_midori_r172_fitstock_sourcepalm_target_preview.json`. The
  fitted target offsets were sane/small, with scale `5.148` across the five
  cases.
- r172 used the merged hand/guitar source bridge root,
  `palm-fit-negx-y-negz`, source-pose elbow hints, and visible-axis hand
  calibration. Contract was green for the five patched diagnostic cases, but
  direct visual review rejected it: it is guitar-adjacent but still not a
  readable performance arm pose.
- r173 repeated r172 with `direct` coordinate map. Patch values and final MILO
  were byte-identical to r172, so no duplicate capture was kept.
- Latest retained sheet:
  `analysis/gh3_midori_r172_staticface_fixedliveguitar_fitstock_sourcepalm_sourceelbow_sheet.jpg`.
  Decision artifact:
  `analysis/gh3_midori_r172_r173_staticface_fitstock_sourcepalm_decision.json`.
- Next branch should stop relying solely on endpoint/contact fitting. Use
  rendered mesh part bounds and silhouette objectives for the arms, especially
  left forearm/hand parts, because formal source/stock target fitting is
  deterministic but visually insufficient.

### 2026-08-21 Midori r168-r171 source-derived arm diagnostics rejected

- Active goal remains open. Commands ran at below-normal/low priority. No ISO
  was mounted or used; captures used temporary loose-DLC staging with only the
  tested main MILO substituted. Face remains intentionally static/no-op.
- Added `--visible-arm-elbow-hint-mode source-pose-per-case` to the full-clip
  and seed-ACP builders. This maps source bicep-to-forearm pose offsets into
  the GH2 visible shoulder frame and uses the result as the two-bone IK
  bend-plane hint.
- r168 used the existing explicit guitar-local grip map plus source-pose elbow
  hints. Contract was green for the five patched diagnostic cases, but direct
  visual review stayed rejected.
- r169 used source-palm targets plus source-pose elbow hints. This produced
  visibly more animated/natural arm motion, but hands abandoned guitar contact.
  It is useful diagnostic evidence but not promotable.
- Derived
  `analysis/gh3_midori_explicit_guitar_grip_map_r170_hybrid_sourcepalm65contact.json`
  by blending r169 source-palm hand targets 35% with r168 explicit contact
  targets 65%, then converting the blended targets into per-case guitar-local
  offsets.
- r170 used the hybrid target map plus source-pose elbow hints. r171 added 50%
  clavicle solve. Both were contract-green for the five patched cases and both
  were visually rejected.
- Latest retained sheet:
  `analysis/gh3_midori_r171_staticface_fixedliveguitar_hybrid_sourcepalm65contact_sourceelbow_clav50_sheet.jpg`.
  Useful comparison sheet:
  `analysis/gh3_midori_r169_staticface_fixedliveguitar_sourcepalm_sourceelbow_sheet.jpg`.
  Decision artifact:
  `analysis/gh3_midori_r168_r171_staticface_sourcearm_decision.json`.
- Next branch should keep source-pose elbow mode and hybrid-map derivation, but
  stop blending against rejected explicit grip offsets as the main strategy.
  Fit source palm/IK-helper positions into GH2 guitar space using source
  guitar-neck/body landmarks, then solve hand targets and wrist axes from that
  fitted guitar frame.

### 2026-08-21 Midori r166-r167 elbow-hint diagnostics rejected

- Active goal remains open. Commands ran at below-normal/low priority. No ISO
  was mounted or used; captures used temporary loose-DLC staging with only the
  tested main MILO substituted. Face remains intentionally static/no-op.
- Fixed a real visible-arm IK bug in
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`:
  `solve_two_bone_position_chain` now computes bend direction from the provided
  elbow hint rather than the current elbow after projecting the hint onto the
  shoulder-target aim. With `elbow_hint_world=None`, current behavior is
  unchanged.
- r166 rebuilt the r165 explicit guitar-local grip target setup with the fixed
  elbow hint solver and mild down/out hints. Contract was green for the five
  patched diagnostic cases, but direct visual review was still rejected.
- r167 reused the r166 ACP seed with a deliberately stronger up/out elbow plane
  (`side=12`, `down=-8`). Contract was green for the five patched diagnostic
  cases, but direct visual review was still rejected.
- Latest retained sheet:
  `analysis/gh3_midori_r167_staticface_fixedliveguitar_elbowupout_axisgrip_sheet.jpg`.
  Decision artifact:
  `analysis/gh3_midori_r166_r167_staticface_elbowhint_decision.json`.
- Next branch should keep the elbow-hint fix, but stop tuning elbow plane alone.
  Derive hand targets and elbow planes together from source/reference geometry,
  or first run an exaggerated per-channel visual sensitivity probe to confirm
  the rendered visible arm channels are not being overridden/suppressed.

### 2026-08-21 Midori r164-r165 explicit guitar-local grip diagnostics rejected

- Active goal remains open. Commands ran at below-normal/low priority. No ISO
  was mounted or used; captures used a temporary loose-DLC add-ons tree with
  only the tested main MILO substituted. Face remains intentionally static.
- Added `--visible-arm-target-mode explicit-guitar-local-grip` and
  `--visible-arm-grip-map` to the full-clip builder and seed-ACP builder. Grip
  maps define fret/strum hand targets as guitar-local row-vector offsets plus a
  `current` or `swapped` visible-hand role per case.
- Added initial diagnostic grip maps:
  `analysis/gh3_midori_explicit_guitar_grip_map_r164.json` and
  `analysis/gh3_midori_explicit_guitar_grip_map_r165.json`. Both shorten the
  fretting target from the previous fixed proxy `z=22.41567` to `z=16.0` to
  reduce overextension.
- r164 used explicit shortened grip targets with attack current and
  jump/solo/transition swapped. Contract stayed green, but visual review still
  rejected. Jump/solo are a little less overextended, but the same stiff posture
  remains.
- r165 used the same explicit grip offsets with all reviewed cases swapped.
  Contract stayed green, but attack did not improve enough and the branch was
  visually rejected.
- Latest retained sheet:
  `analysis/gh3_midori_r165_staticface_fixedliveguitar_explicitgrip_all_swapped_axisgrip_sheet.jpg`.
  Decision artifact:
  `analysis/gh3_midori_r164_r165_staticface_explicitgrip_decision.json`.
- Next branch should keep the explicit grip-map infrastructure, but the first
  static offsets are not enough. Derive per-case grip offsets from source
  geometry or stock-performance reference frames, then solve guitar-local grip
  positions and wrist axes together instead of shortening the old proxy points
  by hand.

### 2026-08-21 Midori r162-r163 calibrated hand-axis diagnostics rejected

- Active goal remains open. Commands ran at below-normal/low priority. No ISO
  was mounted or used; captures used a temporary loose-DLC add-ons tree with
  only the tested main MILO substituted. Face remains intentionally static.
- Added `--visible-hand-rotation-mode visible-axis-calibration` to
  `tools/gh3_midori_build_fullclip_coupled_contact_candidate.py` and threaded it
  through the seed-ACP builder. It consumes
  `analysis/gh3_midori_visible_hand_axis_calibration_r68.json` and orients each
  visible hand by calibrated hand-local finger/palm axes against guitar-local
  finger/palm axes in the current guitar frame. Optional guitar-local grip axis
  bias flags are also available.
- r162 rebuilt the r161-style mapped jump/solo target roles with aim rotations,
  down/out elbow hints, and calibrated guitar-local hand-axis quats instead of
  proxy-derived hand quats. Contract stayed green, but visual review was
  essentially indistinguishable from r161 and rejected.
- r163 used the fast seed path from r162 and also swapped transition target
  roles. Contract stayed green, but transition/attack were still visibly wrong
  and jump/solo remained only partially guitar-like. Rejected.
- Latest retained sheet:
  `analysis/gh3_midori_r163_staticface_fixedliveguitar_mapped_jump_solo_transition_aim_elbow_axisgrip_sheet.jpg`.
  Decision artifact:
  `analysis/gh3_midori_r162_r163_staticface_axisgrip_decision.json`.
- Next branch should stop expecting hand orientation alone to correct arm
  posture. Use mapped role data, but define explicit per-case/per-clip
  guitar-local grip positions and wrist orientations, then solve visible hands
  and proxies from those grip definitions together.

### 2026-08-21 Midori r160-r161 mapped target + hand-quat diagnostics rejected

- Active goal remains open. Commands ran at below-normal/low priority. No ISO
  was mounted or used; captures used a temporary loose-DLC add-ons tree with
  only the tested main MILO substituted. Face remains intentionally static.
- Replaced `tools/gh3_midori_patch_acp_constant_channel.py` with an ACP
  parser/repacker that can add missing constant channels (`--allow-add`), not
  just overwrite existing channels. Threaded this through
  `tools/gh3_midori_build_fullclip_candidate_from_seed_acp.py` as
  `--allow-add-channels`.
- Added `--visible-arm-target-mode mapped-current-proxies` and repeated
  `--visible-arm-target-swap-case` support. This lets the arm solve swap
  visible hand target roles only for selected cases/clips instead of globally.
- r160 did a full rebuild with visible hand quaternion channels, current target
  roles for attack/transition, swapped target roles for
  `midori_1_fast_jump_f040` and `midori_1_fast_solo_f090`, 25% source-pose
  rotation blend, and down/out elbow hints. Contract stayed green, but visual
  review still rejected.
- r161 reused the r160 hand-quat ACP seed and removed the source-pose rotation
  blend (aim-only rotations) while keeping mapped jump/solo swaps and hand
  quats. Contract stayed green. Visual review is slightly cleaner than r160 but
  still rejected: jump/solo are the most guitar-like so far, while
  attack/transition remain visibly wrong and the arms still read stiff.
- Latest retained sheet:
  `analysis/gh3_midori_r161_staticface_fixedliveguitar_mapped_jumpsolo_aim_elbow_handrot_sheet.jpg`.
  Decision artifact:
  `analysis/gh3_midori_r160_r161_staticface_mapped_handquat_decision.json`.
- Next branch should stop looking for a global target-role toggle. Use the new
  mapped-role infrastructure, but add per-case/per-clip hand orientation and
  guitar-local grip definitions rather than deriving hand quats by copying the
  current target proxy world rotations. The jump/solo role swap is useful data,
  not a finished solve.

### 2026-08-21 Midori r154-r159 elbow/rotation/swap arm diagnostics rejected

- Active goal remains open. Commands ran at below-normal/low priority. No ISO
  was mounted or used; captures used a temporary loose-DLC add-ons tree with
  only the tested main MILO substituted. Face remains intentionally static.
- Extended `tools/gh3_midori_build_fullclip_coupled_contact_candidate.py` and
  the seed-ACP builder with `--visible-arm-elbow-hint-mode down-out`,
  `--visible-arm-elbow-side-offset`, `--visible-arm-elbow-down-offset`,
  `--visible-arm-source-rotation-blend-with-aim`, and
  `--visible-arm-target-mode swapped-current-proxies`.
- r154 tested r153's source-palm anim blend with an explicit down/out elbow
  hint. It stayed contract-green but was visually indistinguishable from r153.
  The elbow plane alone is not the unlock.
- r155/r157 tested current proxy targets with per-case source-pose rotations
  blended 25%/50% into aim rotations plus the elbow hint. Both stayed
  contract-green, but both remained visually stiff and incorrect.
- r156 attempted the same branch with visible hand rotations, but the fast seed
  ACP lacked `bone_L/R-hand.mesh.quat` channels. Do not spend a slow full
  rebuild on that alone unless adding hand-quat channels to the seed path or
  broader hand solve work.
- r158 swapped visible hand target roles while keeping aim rotations and elbow
  hint. r159 combined swapped targets with 25% source-pose rotation blend. Both
  stayed contract-green. r159 is the best diagnostic from this turn: jump/solo
  are more guitar-like, but attack and transition are still visibly wrong, so it
  is rejected.
- Latest retained sheet:
  `analysis/gh3_midori_r159_staticface_fixedliveguitar_swappedtargets_sourceposeblend025_elbow_sheet.jpg`.
  Decision artifact:
  `analysis/gh3_midori_r154_r159_staticface_arm_swap_rotation_decision.json`.
- Next branch should treat left/right target role mapping as a real suspect, not
  a solved toggle. Build a hand/arm solve that can use swapped guitar-side roles
  per case or per clip, add hand-rotation channels to the seed/repack path, and
  solve visible hand orientation plus reachable guitar-local grip together.

### 2026-08-21 Midori r148-r153 source-palm arm-target diagnostics rejected

- Active goal remains open. Commands ran at below-normal/low priority. No ISO
  was mounted or used; captures used a temporary loose-DLC add-ons tree with
  only the tested main MILO substituted. Face remains intentionally static.
- Added `--visible-arm-target-mode source-palm-locals-per-case` and
  `--visible-arm-target-blend-with-current` to
  `tools/gh3_midori_build_fullclip_coupled_contact_candidate.py`. The source
  palm target mode derives `Bone_Palm_L/R` offsets from the per-case bridge
  pose relative to `Bone_Chest`, maps them into GH2 basis/scale, and reapplies
  them from `bone_spine3.mesh`.
- Added `tools/gh3_midori_patch_acp_constant_channel.py` and
  `tools/gh3_midori_build_fullclip_candidate_from_seed_acp.py`. The second tool
  reuses an already-expanded full-clip ACP directory and overwrites only the
  constant patch channels, reducing arm-target sweeps from multi-minute
  resampling to about 8 seconds.
- Results: r148 full helper-basis source-palm targets kept the contract green
  but restored vertical guitar because it used the older r135 coupled guitar
  solve. r149 patched fixed-live guitar after r148 but broke proxy/guitar
  contract, so it is diagnostic only. r150 helper basis with `0.35` blend,
  r151 helper basis with `0.15` blend, r152 direct basis with `0.35` blend, and
  r153 anim basis with `0.35` blend all kept packed HMX contract gaps at
  `0.000` with fixed/non-vertical guitar, but all were visually rejected.
- r153 is the least-bad source-palm basis check and has zero average guitar
  offset, but arms remain stiff/incorrect instead of forming a natural grip.
  Latest retained sheet:
  `analysis/gh3_midori_r153_staticface_fixedliveguitar_sourcepalm_anim_blend035_sheet.jpg`.
- Next branch should stop treating source palm positions as direct target
  points. Use the fast seed-ACP builder for iteration, but derive a real
  elbow/hand comfort solve: e.g. constrain elbows in a guitar/body plane, solve
  visible hand positions to reachable guitar-local grip points, then align
  target proxies after the visible chain rather than pulling the visible chain
  toward raw source palm offsets.

### 2026-08-21 Midori r144-r147 per-case source-pose arm diagnostics rejected

- Active goal remains open. Commands ran at below-normal/low priority. No ISO
  was mounted or used.
- Found existing per-case source bridge evidence under
  `.codex/current-evidence/midori-review-source-bridges-fresh-targetlength-20260818-5case`.
  These JSON/GLB files cover the exact reviewed frames for attack, jump, solo,
  transition, and idle, including arm bones. This is better source data than
  the attack-only bridge used by r143.
- Extended `tools/gh3_midori_build_fullclip_coupled_contact_candidate.py` with
  `source-per-case` and `source-pose-per-case` modes. r144 showed that the old
  per-case `matrix_local` route is byte-identical to r143 because those local
  rotations are static. r145-r147 extract pose-parent local rotations from the
  per-case bridge `pose` matrices and test `direct`, `anim`, and `helper`
  bases.
- r145-r147 all kept packed HMX hand/contact gaps at `0.000` and avoided
  vertical guitar silhouettes, but all failed direct visual review. The arm
  rotations now vary by frame, but still do not form a coherent natural guitar
  grip. Decision artifact:
  `analysis/gh3_midori_r144_r147_staticface_percase_source_pose_arm_decision.json`.
  Latest retained sheet:
  `analysis/gh3_midori_r147_staticface_fullclip_fixedguitar_percase_posearmrot_helper_sheet.jpg`.
- Next branch should keep the source-pose-per-case extraction code, but stop
  simply replacing visible arm rotations. Derive hand/guitar comfort targets
  from source pose palms or solve guitar position/orientation toward source
  visible hands, then update proxies around that.

### 2026-08-21 Midori r141-r143 clavicle/source-arm diagnostics rejected visually

- Active goal remains open. Commands ran at below-normal/low priority. No ISO
  was mounted or used; captures staged temporary loose DLC copies with only the
  tested main MILO substituted.
- Extended `tools/gh3_midori_build_fullclip_coupled_contact_candidate.py` with
  optional visible clavicle solving, clavicle blend, and source-bridge
  upper/forearm rotation mode. The source bridge available for this branch only
  covers attack frames `0,15,30,45`, so source-arm mode is a diagnostic static
  prior, not real per-clip animation coverage.
- Built unpromoted r141, r142, and r143. All kept packed HMX hand/contact gaps
  at `0.000` and avoided vertical guitar silhouettes, but all were visually
  rejected. r141 full clavicles overdrive shoulders/arms; r142 half clavicle
  blend mostly regresses toward r138/r140; r143 source frame-30 arm rotations
  introduce wrong arm behavior and do not generalize across jump/solo/transition.
- Decision artifact:
  `analysis/gh3_midori_r141_r143_staticface_arm_branch_decision.json`.
  Latest retained sheet:
  `analysis/gh3_midori_r143_staticface_fullclip_fixedguitar_sourcearmrot_sheet.jpg`.
  Live main remains r133 static face everywhere:
  `2902C712F81ED32A8D7D14E147453F16196C7AE935E5F3D029C179648A36AC8B`.
- Next branch should not keep grinding static arm rotations. Use per-frame or
  per-clip source/GLB arm pose data, or invert the solve so guitar/targets move
  toward reachable natural arm poses instead of forcing visible arms to fixed
  guitar target points.

### 2026-08-21 Midori r136-r140 static-face arms branch rejected visually

- Active goal remains open. Commands ran at below-normal/low priority. No ISO
  was mounted or used; all captures staged temporary loose DLC copies from the
  local `gh2_ps2_hybrid_assets/DLC` package and substituted only the tested main
  MILO.
- Extended `tools/gh3_midori_build_fullclip_coupled_contact_candidate.py` so it
  can label revisions, preserve full current r133 main clip channels, optionally
  solve visible upper/forearm channels, emit visible forearm/hand positions, and
  optionally emit visible hand rotations. This is the first arm branch that
  moves beyond guitar/proxy-only contact math.
- Built and rejected unpromoted r136-r140 diagnostics. Decision artifact:
  `analysis/gh3_midori_r136_r140_staticface_arm_branch_decision.json`.
  Live current main remains r133 static face in all three live locations:
  `2902C712F81ED32A8D7D14E147453F16196C7AE935E5F3D029C179648A36AC8B`.
- Findings: r136 `+90` twist still produced vertical guitar failures in
  fast-jump/fast-solo. r137 fixed the guitar to the current live r133 local
  guitar rotation and removed vertical silhouettes while keeping zero packed HMX
  hand/contact gaps. r138 added visible arm rotations/positions and is real
  progress, but captures still show stiff/warped arms rather than a natural
  grip. r139 reach scale `1.0` and r140 visible hand quats were visually
  indistinguishable from r138.
- Latest retained sheet:
  `analysis/gh3_midori_r140_staticface_fullclip_fixedguitar_visiblearms_handrot_sheet.jpg`.
  Better comparison sheet:
  `analysis/gh3_midori_r138_staticface_fullclip_fixedguitar_visiblearms_sheet.jpg`.
  Next branch should keep fixed live guitar orientation and full-clip donors,
  then add elbow-hint/clavicle/source-arm orientation before position solving.
  Also cache sampled full frames; the current builder is correct but very slow.

### 2026-08-21 Midori r135 full-clip contact probe rejected visually

- Active goal remains open. Commands ran at below-normal/low priority. No ISO
  was mounted or used; visual capture used a temporary loose DLC copy with only
  the r135 main substituted, then that temp tree was cleaned.
- Found the r134 methodological flaw: `replace-clipset-clips` had replaced
  selected main clips with sparse guitar-only donors, dropping the full body
  channels. Added
  `tools/gh3_midori_build_fullclip_coupled_contact_candidate.py` so selected
  donor clips are reconstructed from the current live r133 main MILO samples,
  preserving all existing channels while adding/overwriting only coupled
  `bone_pos_guitar.mesh` and `bone_fret_hand.mesh` / `bone_strum_hand.mesh`
  contact target channels.
- Built unpromoted r135 full-clip contact candidate
  `analysis/gh3_midori_gh2_milos/gh3_midori_main_staticface_fullclip_contact_r135.milo_ps2`
  (SHA256 `9D35E0D2B36473AC962EA0BA8C62F9A324AAD7DB55E0FE8A0FD6417C62A354B4`,
  size `20236639`) and measured it from a flat temp candidate. The rejected
  candidate MILO/donor ACP scratch was cleaned after reports were retained. HMX
  contract report
  `analysis/gh3_midori_r135_staticface_fullclip_contact_contract_report.json`
  is perfectly green on the five main-only cases: both hand-to-target gaps and
  solved guitar-anchor delta are `0.000`.
- Captured the five r135 frames anyway because the contract passed. Direct
  visual/silhouette rejects. Contact sheet:
  `analysis/gh3_midori_r135_staticface_fullclip_contact_sheet.jpg`. Silhouette
  report marks fast-jump and fast-solo as vertical guitar failures
  (`72.004` and `86.92` degrees). Attack/transition are bipedal and more
  diagonal, but the set is not approval-worthy. Decision artifact:
  `analysis/gh3_midori_r135_staticface_fullclip_contact_decision.json`.
- Cheap report-only twist checks at `-90` and `+90` degrees preserve the same
  coupled residuals (`max_residual_error=4.958501`, mean `2.577198`). Next
  branch should keep the r135 full-clip donor path and add a stock-attach or
  diagonal guitar roll/twist prior so jump/solo do not become vertical while
  retaining the zero HMX contact contract.

### 2026-08-21 Midori r133 static face promotion + r134 arm/guitar probe

- Active goal remains open. Commands ran at below-normal/low priority. No ISO
  was mounted or used; all runtime checks used local `gh2_ps2_hybrid_assets/GEN`
  plus loose DLC.
- User rejected the r131/r132 animated face crop behavior: where the face
  visibly changed, it distorted in the wrong direction. Face animation is now
  frozen intentionally so work can move on to arms.
- Built r133
  `analysis/gh3_midori_gh2_milos/gh3_midori_main_face_static_r133.milo_ps2`
  with `tools/gh3_midori_static_face_call_candidate.py`. It materializes 14
  GH2 face-call clips, including `EyesClosed`, as zero-channel no-ops.
  Validator `tools/gh3_midori_validate_static_face_call_candidate.py` passed:
  `static_calls=14`, `errors=0`. Live `gh2_face_call_*` clips show
  `sample_bytes=0`; sampling `gh2_face_call_EyesClosed@120` emits no channels.
- Promoted r133 static face to analysis and both loose DLC mirrors. Current
  main SHA256 in all three locations is
  `2902C712F81ED32A8D7D14E147453F16196C7AE935E5F3D029C179648A36AC8B`, size
  `20235667`. Rebuilt source-tree DLC, deployed hybrid DLC, and verified byte
  identity. Updated `analysis/gh3_midori_gh2_face_control_matrix.json`; latest
  face matrix validator passes with zero errors/warnings.
- Moved on to arms/guitar. Ran current static-live five-case coupled
  hand-target/guitar-anchor solve:
  `analysis/gh3_midori_r133_staticface_mainonly_coupled_guitar_solve_report.json`.
  The report-only coupled solve is plausible (`max_residual_error=4.958501`,
  mean `2.577198`), so an r134 guitar-only packed probe was tested.
- r134 replaced only per-case `bone_pos_guitar.mesh` channels in the main clips
  and was rejected before capture. Contract report
  `analysis/gh3_midori_r134_staticface_coupled_guitar_contract_report.json`
  still has whole-limb gaps (`LH=26.830-33.460`, `RH=21.866-32.394`) and
  solve deltas `33.350-52.052`. Decision artifact:
  `analysis/gh3_midori_r134_staticface_coupled_guitar_decision.json`.
- Next arm branch: do not emit guitar-only coupled fits. Emit or solve guitar,
  visible arms, and contact target proxies together in the same per-main-frame
  ACP path, then require packed HMX replay to keep both hand gaps and solved
  guitar-anchor delta at visual-contact scale before capture.

### 2026-08-21 Midori r132 face-control routing diagnosis

- Active goal remains open. Commands ran at below-normal/low priority. No ISO
  was mounted or used; r132 capture used local `gh2_ps2_hybrid_assets/GEN` plus
  a temporary loose DLC copy with only `gh3_midori_main.milo_ps2` substituted,
  then the temp DLC tree was deleted.
- Built unpromoted r132 amplified jaw-delta candidate
  `analysis/gh3_midori_gh2_milos/gh3_midori_main_facealiases_jawdelta_gain2_r132.milo_ps2`
  from r131 with `--jaw-delta-gain 2.0`. Candidate SHA256
  `BE817245FD0600F8E832BCEAE04D3A9152A17875746D6A2744F731560552016D`, size
  `20378820`, generated max jaw delta `16.892662` degrees. Validator passed:
  two open calls, zero errors/warnings, sampled max jaw delta `14.360422`
  degrees.
- Runtime diagnosis found the earlier viewer face filter was dropping hashed
  face controls: pre-patch pose publisher showed `gh2_face_call_open` as
  `ch=14:out=7`. Patched known Midori hashed face controls into the app,
  gameplay, and CharClip face filters. Rebuilt `ghogx_app` and deployed the
  loose hybrid runtime; deployed app SHA256
  `42dcf513c230f7c57adbb927fd3b1c784238727d398d3786592638caf1639241`.
- Patched-filter r132 capture
  `analysis/gh3_midori_r132_facefilter_open_compare_report.json` passed with
  nine proofs, zero failures, min margin `23`. Its paired diff report records
  `runtime_face_channels=15` and `runtime_face_output_bones=10` for frames
  60/120/180/240, but the visible default/open deltas remain tiny
  (`0.00085178`, `0.00084744`, `0.00085612`, `0.00099935`).
- Added hash-aware face debug classification in `app_main.cpp`. Focused debug
  proof
  `analysis/gh3_midori_r132_facefilter_debug_after_classifier_report.json`
  intentionally fails only the single-case idle-difference harness check, but
  its log proves `faceRows=8`, `faceOutputBones=10`, and publisher layer
  `gh2_face_call_open@f120:w=1.000:ch=15:out=10`.
- Interpretation: GH2-style face-call routing to the limited Midori/GH3 hashed
  face controls is now proven. This is not visual approval; the mesh response is
  still too subtle, so the next face work should diagnose actual mesh influence
  or build a stronger explicit GH2-call-to-available-control translation matrix.

### 2026-08-21 Midori r131 native face-open visual probe

- Active goal remains open. Commands ran at below-normal/low priority. No ISO
  was mounted or used; capture used local `gh2_ps2_hybrid_assets/GEN` plus
  loose `gh2_ps2_hybrid_assets/DLC`.
- Temporarily deployed r131 into loose DLC with
  `tools/gh3_midori_capture_with_loose_dlc_backup.py`, captured native viewer
  face-open frames, and restored live r127 afterward. Verified live main hashes
  in analysis and both loose-DLC mirrors are still
  `077AE6973D3DE99B8CC3009C6825755B19F399DD92AFD52F303EA8F6E5B5697C`.
- The first r131 visual packet advanced the main idle frame and face frame
  together, so it was superseded and deleted during cleanup. Added
  same-body-frame paired comparison cases:
  `analysis/gh3_midori_r131_face_open_compare_cases.json`. Retained
  `analysis/gh3_midori_r131_face_open_compare_report.json`,
  `analysis/gh3_midori_r131_face_open_compare_contact_sheet.jpg`,
  `analysis/gh3_midori_r131_face_open_compare_face_crop_sheet.jpg`,
  `analysis/gh3_midori_r131_face_open_compare_face_crop_sheet.json`, and
  `analysis/gh3_midori_r131_face_open_compare_diff_report.json`. The paired
  capture passed with nine proofs, zero failures, min margin `23`.
- Paired visual read: r131 remains bipedal/coherent; large head orientation
  changes are shared main-body-frame motion rather than face-layer corruption.
  Default/open same-frame visual deltas are measurable but small
  (`0.00085178`, `0.00084744`, `0.00085612`, `0.00099935` for frames
  60/120/180/240). This is suitable for user review, not promotion yet.
- Added `tools/gh3_midori_face_crop_sheet.py`. Re-ran r131 jaw-delta validator
  and face-control matrix validator; both passed. Completion still requires
  direct visual approval.

### 2026-08-21 Midori r131 GH2 face open source-derived jaw-delta candidate

- Active goal remains open. Commands ran at below-normal priority. No ISO was
  mounted or used; this pass used retained local MILO/ACP artifacts only.
- Source audit found only `gh3_guitarist_midori_acc01`,
  `gh3_guitarist_midori_acc02`, and `gh3_guitarist_midori_default` carry the
  Midori jaw/eye hashes in staged ACP. `default` is reset-only; `acc02` has the
  stronger non-reset jaw quaternion delta.
- Built unpromoted r131 candidate
  `analysis/gh3_midori_gh2_milos/gh3_midori_main_facealiases_jawdelta_r131.milo_ps2`
  by generating an automated ACP donor that combines `yeah` mouth-deformer
  motion with time-stretched `acc02` jaw quaternion motion for
  `gh2_face_call_open` and `gh2_face_call_singer_face_open`. Candidate SHA256
  `DC93FC5593A246100BF44EBF80BE775E99BC07E9D94357ACB16C34A4E57D62F6`, size
  `20378639`.
- Added `tools/gh3_midori_face_open_jaw_delta_candidate.py` and
  `tools/gh3_midori_validate_face_open_jaw_delta_candidate.py`, plus retained
  manifest/report
  `analysis/gh3_midori_face_open_jaw_delta_candidate_r131.json` and
  `analysis/gh3_midori_face_open_jaw_delta_validation_report_r131.json`.
  Latest validation passed with two open calls, zero errors/warnings. Both open
  calls preserve the seven mouth deformers, expose `bone_gh3_b8ca856b`, and
  produce source-derived non-reset jaw motion. Validator sampled max jaw delta
  from reset: `7.179979` degrees; generated source sweep max: `8.446331`
  degrees.
- Updated the face matrix, matrix validator, build pipeline, and completion
  audit to track r131 separately from r129 aliasing and r130 reset-carrier
  plumbing. Latest targeted gates passed: r131 jaw-delta validator,
  face-control matrix validator, and pelvis/face approval validator. Completion
  audit still fails only at the known visual precheck/promoted triage/user
  approval layer.
- Next target: capture or otherwise visually inspect r131's GH2 open-call face
  behavior before any promotion claim. `EyesClosed` remains an explicit
  unsupported gap.

### 2026-08-21 Midori r130 GH2 face open jaw-carrier candidate

- Active goal remains open. Commands ran at below-normal priority. No ISO was
  mounted or used; this pass used retained local MILO/ACP artifacts only.
- User clarified that GH2 does not expose the larger Neversoft facial-control
  surface. The face goal is now treated as a GH2-call translation matrix with
  honest gaps, not Neversoft facial parity.
- Live/promoted DLC remains r127. The r129 alias candidate remains unpromoted
  and materializes 13 non-gap GH2 face calls. Its diagnosis still records
  `jaw_open_gap_confirmed`: `open` and `singer_face_open` reach the seven
  Midori mouth deformers but not the Midori jaw hash, while `EyesClosed` remains
  an explicit unsupported gap.
- Built unpromoted r130 candidate
  `analysis/gh3_midori_gh2_milos/gh3_midori_main_facealiases_jawcarrier_r130.milo_ps2`
  by generating an automated ACP donor and replacing only
  `gh2_face_call_open` and `gh2_face_call_singer_face_open` in r129. Candidate
  SHA256 `91F9EEFB9CD62950AA85A972E9C7A18443A0CC768AB586F7F34912C6DD653B56`,
  size `20369689`.
- Added `tools/gh3_midori_face_open_jaw_candidate.py` and
  `tools/gh3_midori_validate_face_open_jaw_candidate.py`, plus retained
  manifest/report `analysis/gh3_midori_face_open_jaw_candidate_r130.json` and
  `analysis/gh3_midori_face_open_jaw_validation_report_r130.json`. Latest r130
  validation passed with two open calls, zero errors/warnings. Both open calls
  preserve the seven mouth deformers and expose `bone_gh3_b8ca856b`, but the
  jaw motion status is `jaw_control_carrier_present_reset_only`; this is
  plumbing proof, not visible jaw-open proof.
- Updated the face matrix, matrix validator, build pipeline, and completion
  audit to track r130 separately from live r127 and r129. Latest targeted gates
  passed: r129 alias validator, r129 jaw/open diagnosis, r130 jaw-carrier
  validator, face-control matrix validator, and pelvis/face approval validator.
  Completion audit still fails only at the known visual precheck/promoted
  triage/user approval layer.
- Next target: isolate or synthesize a source-derived jaw-open delta for
  `bone_gh3_b8ca856b` before any promotion claim. Do not hide jaw-open or
  `EyesClosed` as no-ops.

### 2026-08-21 Midori r114 conservative forearm-position blend rejection

- Active goal remains open. Commands ran at Idle/low priority; compiler/MSBuild
  child processes were explicitly lowered to Idle during converter rebuild.
  Captures used loose/extracted assets only: `gh2_ps2_hybrid_assets/GEN` plus
  `gh2_ps2_hybrid_assets/DLC`; no ISO was mounted or used at game time.
- Lower body/pelvis/root remain solved and regression-only. r114 torso/upright
  contracts passed for both outfits.
- Found and fixed a second solver-order issue: partial emitted forearm/hand
  position blends were solved with full IK local positions, then the local
  positions were blended afterward. That made partial blends replay worse than
  their synth intent. `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py`
  now establishes emitted/blended local positions before solving rotations.
  `tools/gh3_midori_pipeline_test.py` now covers this path; the suite passed
  `126` tests.
- Swept conservative left forearm/hand position blends in memory. Best bounded
  candidate was left forearm `0.10`, left hand `0.00`; higher blends increased
  the hand-to-fret miss.
- Built and captured r114 `fore10/hand0` with the r113 reference-relative
  contact branch. Structural gates passed: synth/built replay equivalence
  matched, torso axes passed, and `LH-FRET` stayed within
  `0.198/0.230/0.114/0.625/0.198`.
- Direct visual rejects all five r114 proofs. The small forearm position blend
  does not visibly improve the pinned/hidden fretting-arm silhouette compared
  with r113, and it costs contact margin in transition.
- Direct visual decision artifact:
  `analysis/gh3_midori_r114_fore10hand0_visual_triage.json`.
- Current best partial remains r113: contact fixed, but fretting arm silhouette
  still needs a real natural upper-chain pose prior. Do not spend more captures
  on tiny visible forearm position blends unless a structural/mesh diagnostic
  shows a meaningful elbow silhouette change first.

### 2026-08-21 Midori r113 no-emit position-restore diagnostic

- Active goal remains open. Commands ran at Idle/low priority; compiler/MSBuild
  child processes were explicitly lowered to Idle during converter rebuild.
  Captures used loose/extracted assets only: `gh2_ps2_hybrid_assets/GEN` plus
  `gh2_ps2_hybrid_assets/DLC`; no ISO was mounted or used at game time.
- Lower body/pelvis/root remain solved and regression-only. r113 torso/upright
  contracts passed for both outfits.
- Added `tools/gh3_midori_synth_built_transform_equivalence_report.py`, a
  focused diagnostic comparing solver context, in-memory emitted-channel
  replay, and built MILO replay for the same cases.
- Root cause found for the r105-r112 synth/runtime gap:
  `solve_two_bone_position_chain` always moved the visible forearm local
  position while computing the IK elbow. Rotation-only/no-position branches
  then reported hand contact from that un-emitted forearm position, while the
  built clip replayed only quats and kept the original forearm local position.
- Fixed `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py` so
  `solve_side` restores the original forearm/hand local positions when
  `emit_positions` is false before producing quat channels. Added a regression
  to `tools/gh3_midori_pipeline_test.py`; the suite passed `125` tests.
- Rebuilt and captured the fixed reference-relative branch. Structural gates
  are now honest and strong: synth/built transform equivalence matches,
  `LH-FRET` is `0.005/0.008/0.005/0.006/0.005`, and torso axes pass.
- Direct visual inspection of all five r113 proofs: this is the new best
  partial. The old below/behind-neck fretting-hand detach is fixed; the hand is
  actually planted at the neck target in attack, jump, solo, transition, and
  outfit 2. Reject for promotion because the fretting arm/forearm silhouette
  still reads mechanically pinned or hidden behind the guitar/body rather than
  stock-quality natural fretting.
- Explicit left elbow hints were checked in-memory. Small and large hints moved
  the replayed elbow only slightly and larger hints began degrading contact, so
  no extra elbow-hint capture was kept.
- Direct visual decision artifact:
  `analysis/gh3_midori_r113_noemitfixed_visual_triage.json`.
- Next useful branch: preserve the r113 no-position replay equivalence and
  near-zero hand contact, but add a natural visible fretting upper-chain/arm
  pose prior that changes the rendered elbow/forearm silhouette. Do not reopen
  pelvis/root, guitar attachment, sample serialization, or raw target reach.

### 2026-08-21 Midori r112 reference-relative wrist diagnostic

- Active goal remains open. Commands ran at Idle/low priority; compiler/MSBuild
  child processes were explicitly lowered to Idle during converter rebuild.
  Captures used loose/extracted assets only: `gh2_ps2_hybrid_assets/GEN` plus
  `gh2_ps2_hybrid_assets/DLC`; no ISO was mounted or used at game time.
- Lower body/pelvis/root remain solved and regression-only. r112 torso/upright
  contracts passed for both outfits, so the active blocker is still fretting
  upper-chain/hand contact.
- Added default-preserving `--visible-hand-axis-sides` to
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py` so visible
  hand-axis calibration can be limited to `both`, `left`, or `right`. The
  pipeline regression passed after the edit.
- A planned full-hand fallback test was abandoned before capture because
  `finger_chord_bar` contains the 15 fretting finger quats but no
  `bone_L-hand.mesh.quat`; it can supply finger curl, not the wrist quat.
- r112 retested `--hand-orientation-mode reference-relative` with
  `--emit-hand-target-rotation` on the newer length-preserving arm solve and
  retained the working fretting finger quats. Synth-space left hand-to-target
  distances improved to roughly `0.592` to `0.785`, but the built HMX replay
  contract still reports large left misses: attack `9.187`, jump `7.691`,
  solo `9.299`, transition `15.154`, outfit-2 attack `9.187`.
- Merge reports confirm the intended overlay channels, including
  `bone_L-hand.mesh.quat`, were copied into all five target clips. Direct
  decode sanity is worse than HMX, so HMX remains the correct replay mode.
- A focused overlay-vs-MILO equivalence report confirms the generated ACP quats
  and built `gh3_midori_main.milo_ps2` samples match within `4.846e-7` across
  all 28 checked channels in all five clips. Serialization/merge/build are not
  the current culprit.
- Direct visual rejects all five r112 proofs. Body/guitar are coherent and
  fingers are visible, but the fretting hand still sits below/behind the neck
  and does not clearly wrap the fretboard, especially transition and outfit 2.
- Direct visual decision artifact:
  `analysis/gh3_midori_r112_refrelative_visual_triage.json`.
- Next useful branch: stop treating hand rotation alone as sufficient. Compare
  the synth-time transform stack against the built-candidate contract transform
  stack for one clip/frame, especially upper-body reference application, base
  candidate stage selection, and target/helper world positions.

### 2026-08-21 Midori r111 finger-pose contact diagnostic

- Active goal remains open. Commands ran at Idle/low priority; compiler/MSBuild
  child processes were explicitly lowered to Idle during converter rebuild.
  Captures used loose/extracted assets only: `gh2_ps2_hybrid_assets/GEN` plus
  `gh2_ps2_hybrid_assets/DLC`; no ISO was mounted or used at game time.
- Lower body/pelvis/root remain solved and regression-only. r111 torso/upright
  contracts passed, so the remaining failure is not a Control_Root or pelvis
  problem.
- Added default-preserving fallback existing hand-pose clip/frame support to
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py`, allowing
  main-only approval cases to borrow fret/strum hand pose clips.
- r111 merged real `finger_chord_bar` fretting-finger quats into the
  length-preserving main-arm branch with `--merge-existing-hand-pose` and
  `--existing-hand-pose-mode fingers-only`. All 15 fretting finger channels
  were injected and were visibly active/stable in the capture.
- Direct visual rejects all five r111 proofs. Body and arm length remain
  coherent and the finger curl channels work, but the fretting wrist/hand is
  still below/behind the neck and does not read as gripping. Hand shape is now
  separated from the real blocker: wrist/contact transform placement.
- Direct visual decision artifact:
  `analysis/gh3_midori_r111_finger_pose_triage.json`.
- Next useful branch: keep the length-preserving arm solve and working finger
  channels, but solve/calibrate the actual hand/wrist contact transform against
  the fretboard. Likely targets are hand target rotation/reference delta or a
  visible hand contact marker; avoid direct visible forearm/hand local-position
  emission in promoted visuals.

### 2026-08-21 Midori r110 target-offset diagnostics

- Active goal remains open. Commands ran at Idle/low priority; compiler/MSBuild
  child processes were explicitly lowered to Idle during converter rebuild.
  Captures used loose/extracted assets only: `gh2_ps2_hybrid_assets/GEN` plus
  `gh2_ps2_hybrid_assets/DLC`; no ISO was mounted or used at game time.
- Lower body/pelvis/root are still solved and regression-only. r110
  torso/upright contracts passed.
- r110 probed preserved-length fretting wrist target offsets after r109 showed
  a plausible-length but detached fretting limb. The best synth probe moved the
  target toward the observed r109 wrist/hand direction with
  `--left-target-world-offset=-4,-3,7`; opposite and simple Z-only probes were
  weaker or less balanced.
- The captured r110 offset branch visually rejects all five proofs. Body and
  arm length remain coherent, but the visible fretting hand still sits
  below/behind the neck and does not read as a grip. Target offset alone is
  mostly swallowed by the same replay/hand-orientation limits.
- Direct visual decision artifact:
  `analysis/gh3_midori_r110_target_offset_triage.json`.
- Next useful branch: keep length-preserving arm solve, but calibrate hand
  orientation/finger pose against the guitar neck. Reuse existing hand/finger
  pose channels or introduce a visible-hand contact marker; do not return to
  direct visible forearm/hand local-position emission.

### 2026-08-21 Midori r105/r109 length-preserving rotation diagnostics

- Active goal remains open. Commands ran at Idle/low priority; compiler/MSBuild
  child processes were explicitly lowered to Idle during converter rebuild.
  Captures used loose/extracted assets only: `gh2_ps2_hybrid_assets/GEN` plus
  `gh2_ps2_hybrid_assets/DLC`; no ISO was mounted or used at game time.
- Lower body/pelvis/root are still solved and regression-only. r107/r108/r109
  torso/upright contracts passed.
- Added default-off length-preserving rotation refinement to
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py`:
  `--length-preserving-rotation-refine-iterations` and
  `--length-preserving-rotation-refine-strength`. The refinement rotates
  upper/forearm joints toward the actual child hand target without emitting
  visible forearm/hand `.pos` channels.
- r105 used fitted source positions as a source forearm elbow hint with
  `--no-emit-visible-arm-positions`. It preserved local limb lengths but missed
  badly (`LH-FRET` about 4.4 to 11.9).
- r106/r107 showed the new length-preserving refinement can improve internal
  synth distances without the r103/r104 sleeve-collapse failure. A synth/build
  mismatch exposed that overlay generation and merge/build base must match, and
  that sample mapping alone was not the root cause of the remaining runtime
  miss.
- r109 merged the rebuilt-base length-preserving branch using repeat-last
  sample mapping and was captured. Direct visual rejects all five proofs:
  the body is bipedal and the fretting limb keeps plausible length, but the
  fretting hand remains detached below/behind the neck. This fixes the collapse
  mode, not the grip/contact mode.
- Direct visual decision artifact:
  `analysis/gh3_midori_r105_r109_lengthpreserve_triage.json`.
- Next useful branch: calibrate the actual rendered palm/finger contact point
  against the guitar neck/fretboard target space. Keep source rotations and
  direct visible local forearm/hand position emission out of promoted visuals;
  preserve limb lengths and solve toward a target offset that matches the
  visible hand mesh, not just the abstract `bone_fret_hand` target.

### 2026-08-21 Midori r102/r103/r104 visible-position diagnostics

- Active goal remains open. Commands ran at Idle/low priority. Captures used
  loose/extracted assets only: `gh2_ps2_hybrid_assets/GEN` plus
  `gh2_ps2_hybrid_assets/DLC`; no ISO was mounted or used at game time.
- Lower body/pelvis/root are solved and regression-only. r102/r103/r104 all
  passed torso/upright contracts, so the current failure is still upper-body
  fretting-arm/neck grip.
- Added default-preserving split visible position controls to
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py`:
  `--left/right-visible-forearm-position-blend` and
  `--left/right-visible-hand-position-blend`. Existing callers keep the old
  behavior because the split blends default to each side's arm blend.
- r102 used source forearm-position elbow hint with conservative left position
  blend. It stayed bipedal, but visually rejected because the fretting hand
  remained detached below/behind the neck (`LH-FRET` about 3.3 to 4.4).
- r103 raised left visible position solving to full strength. It achieved
  `LH-FRET=0.000` on all five cases and passed torso/upright, but visually
  hard rejected because the fretting limb collapsed into an abbreviated
  sleeve/hand shape near the neck.
- r104 kept forearm position conservative while moving the visible hand fully.
  It also achieved `LH-FRET=0.000` on all five cases and passed torso/upright,
  but still visually rejected with a snapped/collapsed fretting wrist/forearm.
- Direct visual decision artifact:
  `analysis/gh3_midori_r102_r103_r104_visible_position_triage.json`.
- Next useful branch: keep source rotations and direct visible local
  forearm/hand position emission out of promoted visuals. Use GLB/source bridge
  positions as IK constraints while preserving bind/local limb lengths, likely
  through rotation-only retargeting plus a constrained neck target offset or a
  MILO-side retarget that respects the child mesh offsets.

### 2026-08-21 Midori r99/r100 calibrated source diagnostics

- Active goal remains open. Commands ran at Idle/low priority except short
  literal `cmd` cleanup deletes where PowerShell recursive deletion was blocked
  by the shell safety layer. Captures used loose/extracted assets only:
  `gh2_ps2_hybrid_assets/GEN` plus `gh2_ps2_hybrid_assets/DLC`; no ISO was
  mounted or used at game time.
- Lower body/pelvis/root remain solved and regression-only. r99/r100 did not
  reopen matrix-local pelvis/`Control_Root`.
- Added default-off reference-case source orientation calibration to
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py`:
  `--source-orientation-calibration-mode reference-case`,
  `--source-orientation-calibration-case-name`, and
  `--source-orientation-calibration-frame`. The calibration maps GLB/source
  rotations through a visible medium-idle reference before applying animated
  source deltas.
- r99 used calibrated left-only GLB target-basis source rotations applied to
  upper/forearm/hand. It achieves perfect numeric left-hand contact
  (`LH-FRET=0.000` for all five cases) and torso/upright contracts pass, but
  direct visual hard rejects because the fretting arm rises beside/across the
  face/head.
- r100 kept the same calibration but applied source rotations only to
  forearm/hand, leaving upperArm IK-controlled. It also achieves perfect numeric
  left-hand contact and passes torso/upright contracts. Direct visual still
  rejects: attack/transition avoid the worst face crossing, but jump/solo raise
  the arm and the forearm reads as a stiff horizontal bar behind/over the neck.
- Direct visual decision artifact:
  `analysis/gh3_midori_r99_r100_calibrated_source_visual_triage.json`.
- Next useful branch: keep lower body as regression-only. Do not emit source
  rotations directly as visible-arm quats. Use retained source bridge positions
  to drive a fretting elbow/forearm/hand chain-plane prior, or project source
  rotations into IK constraints instead of direct quats.

### 2026-08-21 Midori r97/r98 neck-offset diagnostics

- Active goal remains open. Commands ran at Idle/low priority except short
  literal `cmd` cleanup deletes where PowerShell recursive deletion was blocked
  by the shell safety layer. Captures used loose/extracted assets only:
  `gh2_ps2_hybrid_assets/GEN` plus `gh2_ps2_hybrid_assets/DLC`; no ISO was
  mounted or used at game time.
- Lower body/pelvis/root remain solved and regression-only. r97/r98 did not
  reopen matrix-local pelvis/`Control_Root`.
- r97/r98 tested explicit visible fretting hand target offsets around the
  calibrated left palm-normal, using the stable stockattach base, medium-idle
  upper-body reference, left blend `0.45`, right blend `0.75`, and r95 elbow
  hint `0,4,4`.
- Default left fret target in guitar-local space is
  `[-3.102, -0.159, 22.416]`. r97 tested +4 palm-normal:
  `[0.144, -0.447, 24.734]`. r98 tested -4 palm-normal:
  `[-6.349, 0.129, 20.097]`.
- Direct visual rejects both. r97 remains bipedal but the fretting hand still
  reads under/behind the neck. r98 is worse: the fretting upper/forearm pulls
  back behind the neck and transition contact regresses.
- Direct visual decision artifact:
  `analysis/gh3_midori_r97_r98_neckoffset_visual_triage.json`.
- Next useful branch: keep lower body as regression-only and calibrate the
  GLB-to-visible upper/forearm/hand basis from retained source bridges before
  applying source arm rotations/positions. Simple terminal hand-target offsets
  and palm roll are not enough; the visible forearm and hand need a coupled,
  source-like arm prior.

### 2026-08-21 Midori r96 grip-roll diagnostics

- Active goal remains open. Commands ran at Idle/low priority except short
  literal `cmd` cleanup deletes where PowerShell recursive deletion was blocked
  by the shell safety layer. Captures used loose/extracted assets only:
  `gh2_ps2_hybrid_assets/GEN` plus `gh2_ps2_hybrid_assets/DLC`; no ISO was
  mounted or used at game time.
- Lower body/pelvis/root remain solved and regression-only. r96 did not reopen
  matrix-local pelvis/`Control_Root`.
- Added default-off visible hand grip-roll controls to
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py`:
  `--left-hand-grip-guitar-local-axis`,
  `--left-hand-grip-guitar-local-degrees`,
  `--right-hand-grip-guitar-local-axis`, and
  `--right-hand-grip-guitar-local-degrees`. These bias the canonical visible
  hand finger/palm axes in guitar-local space and emit hand quat channels when
  used.
- r96 tested two signs around the calibrated left finger/neck axis
  `[-0.373615, 0.698834, 0.609953]`: `+60` and `-60`, keeping the r95 elbow
  hint `0,4,4`, medium-idle upper-body reference, left blend `0.20`, right blend
  `0.75`, and canonical visible hand axis. Both variants pass torso/upright
  contracts and remain bipedal in direct visual inspection.
- Direct visual rejects both r96 variants. The hand-roll knob is wired and emits
  `bone_L-hand.mesh.quat`, but the fretting hand still sits under/behind the
  neck. The defect is dominated by visible hand/chain position, not palm roll
  alone.
- Direct visual decision artifact:
  `analysis/gh3_midori_r96_griproll_visual_triage.json`.
- Next useful branch: keep lower body as regression-only and move the visible
  fretting hand/palm around the fretboard through explicit neck-grip position
  offsets, or calibrate the GLB-to-visible arm basis before applying source
  upper/forearm/hand rotations.

### 2026-08-21 Midori r95 explicit elbow-hint diagnostic

- Active goal remains open. Commands ran at Idle/low priority. Capture used
  loose/extracted assets only: `gh2_ps2_hybrid_assets/GEN` plus
  `gh2_ps2_hybrid_assets/DLC`; no ISO was mounted or used at game time.
- Lower body/pelvis/root are still solved and regression-only. The current
  failure remains visible upper-body/fretting-arm neck-grip quality.
- Added default-off explicit elbow hint controls to
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py`:
  `--left-elbow-hint-shoulder-guitar-local` and
  `--right-elbow-hint-shoulder-guitar-local`. These express a guitar-local
  shoulder-to-elbow hint direction and do not affect pelvis/root handling.
- r95 tested medium-idle upper-body reference, left blend `0.20`, right blend
  `0.75`, canonical visible hand axis, and left elbow hint `0,4,4`. Torso-axis
  contract passed; loose-DLC capture produced five proofs. Direct visual rejects
  r95: bipedal stance stays coherent, but the fretting chain still forms a stiff
  triangular bend behind the neck and the hand remains under/behind the
  fretboard instead of gripping it.
- Direct visual decision artifact:
  `analysis/gh3_midori_r95_upperbodyref_leftelbow_downneck_visual_triage.json`.
- Next useful branch: keep lower body as regression-only and either calibrate
  the GLB-to-visible arm basis before applying source arm rotations, or add an
  explicit neck-grip pose prior that rotates/places the visible fretting hand
  around the fretboard instead of only changing the elbow bend plane.

### 2026-08-21 Midori r93/r94 source-prior diagnostics

- Active goal remains open. Commands ran at Idle/low priority. Capture used
  loose/extracted assets only: `gh2_ps2_hybrid_assets/GEN` plus
  `gh2_ps2_hybrid_assets/DLC`; no ISO was mounted or used at game time.
- Lower body/pelvis/root remain solved and are regression-only. The current
  failure is visible upper-body/fretting-arm quality.
- Added a default-off source-orientation side selector to
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py`:
  `--source-orientation-sides {both,left,right}`. Default behavior is
  unchanged. Also made missing source position rows tolerated for rotation-only
  GLB prior use.
- r93 tested the stronger source prior requested by the r90-r92 diagnosis:
  medium-idle upper-body reference, left blend `0.20`, right blend `0.75`, and
  left-only GLB target-basis source rotations applied to upper/forearm/hand.
  Structurally it is excellent (`LH=0.000` for all five cases, right
  `2.246/2.134/0.940/0.937/2.246`; torso axis green), but direct visual hard
  rejects because the left arm spikes upward beside/over the head.
- r94 constrained the GLB source prior to left hand-only rotation. It avoids the
  r93 arm spike, but looks essentially like r90 and does not improve the visible
  palm/neck grip. Reject r94.
- Direct visual decision artifact:
  `analysis/gh3_midori_r93_r94_sourceprior_visual_triage.json`.
- Next useful branch: keep upper-body reference stabilization, do not apply raw
  GLB source upper/forearm rotations directly in this basis, and either
  calibrate the GLB-to-visible-rig arm basis before source rotations or add an
  explicit side-specific elbow/chain-plane prior that moves the fretting elbow
  without spiking it upward.

### 2026-08-21 Midori r90-r92 upper-body reference grip experiments

- Active goal remains open. Commands ran at Idle/low priority. Capture used
  loose/extracted assets only: `gh2_ps2_hybrid_assets/GEN` plus
  `gh2_ps2_hybrid_assets/DLC`; no ISO was mounted or used at game time.
- Lower body/pelvis/root remain solved and are regression-only. The current
  failure is visible upper-body/fretting-arm quality.
- r90 tested the midpoint between r88/r89: upper-body reference,
  left/fret blend `0.20`, right/strum blend `0.75`, canonical visible hand
  axis. It stays bipedal but does not visibly improve the fretting neck grip.
  Contact: `attack 5.216/2.246`, `jump 6.152/2.134`,
  `solo 5.605/0.940`, `transition 5.762/0.937`, outfit-2 attack matching.
- r91 kept r90 and enabled reference-relative hand-target rotation. It changed
  the rotation path but did not visibly improve the fretting palm/neck
  relationship; reject it.
- r92 kept r90 and moved the left target in guitar-local space from default
  `[-3.102,-0.159,22.416]` to `[-3.102,-2.0,22.416]`. Jump contact improved,
  but transition regressed (`LH=8.236`) and the visual grip did not improve;
  reject target nudging.
- Direct visual decision artifact:
  `analysis/gh3_midori_r90_r92_upperbodyref_grip_visual_triage.json`.
- Next useful branch: keep upper-body reference stabilization, stop blend-only
  and small target-offset sweeps, and use a stronger fretting visible-arm/hand
  pose source or explicit side-specific chain prior from real grip evidence.

### 2026-08-21 Midori r88/r89 upper-body reference branch

- Active goal remains open. Commands ran at Idle/low priority. Capture used
  loose/extracted assets only: `gh2_ps2_hybrid_assets/GEN` plus
  `gh2_ps2_hybrid_assets/DLC`; no ISO was mounted or used at game time.
- Lower body/pelvis/root remain solved and are regression-only. The current
  failure is visible upper body/fretting-arm quality.
- Added optional upper-body reference stabilization to
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py`:
  `--upper-body-reference-mode`, `--upper-body-reference-case-name`,
  `--upper-body-reference-frame`, and `--upper-body-reference-channels`. Default
  behavior is unchanged (`none`).
- r88 kept the stockattach guitar frame and r86 side blends (`left=0.35`,
  `right=0.75`), then applied medium-idle upper-body reference channels before
  the visible-arm solve. This fixed the non-bipedal jump/solo/transition read:
  head/torso no longer collapse backward. Reject r88 for approval because the
  fretting elbow/hand still do not make a natural neck grip. Contact:
  `attack 4.238/2.246`, `jump 4.998/2.134`, `solo 4.554/0.940`,
  `transition 4.682/0.937`, outfit-2 attack matching.
- r89 kept the same upper-body reference but dropped left/fret position blend to
  `0.0`. It preserves the bipedal read and slightly improves left upper-arm
  silhouette, but fret contact becomes too detached (`attack LH=6.520`,
  `jump LH=7.690`, `solo LH=7.007`, `transition LH=7.202`). Reject r89 as too
  loose.
- Direct visual decision artifact:
  `analysis/gh3_midori_r88_r89_upperbodyref_visual_triage.json`.
- Next useful branch: keep upper-body reference stabilization, sweep left/fret
  blend between `0.0` and `0.35` (likely `0.15-0.25`), and add a fret-hand
  rotation/grip prior. Do not reopen lower body.

### 2026-08-21 Midori r86/r87 side-specific blend branch

- Active goal remains open. Commands ran at Idle/low priority. Capture used
  loose/extracted assets only: `gh2_ps2_hybrid_assets/GEN` plus
  `gh2_ps2_hybrid_assets/DLC`; no ISO was used at game time.
- Added side-specific arm position blend overrides to
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py`:
  `--left-visible-arm-position-blend` and
  `--right-visible-arm-position-blend`. Unset sides inherit
  `--visible-arm-position-blend`.
- r86 tested left/fret `0.35`, right/strum `0.75`. It is the new best partial
  branch: diagonal guitar remains stable, attack is close to r85 but with better
  strum contact, and the overall tradeoff beats r87. Contact:
  `attack 1.410/2.559`, `jump 4.997/2.572`, `solo 4.174/4.265`,
  `transition 4.724/0.835`, outfit-2 attack matching.
- r87 tested left `0.25`, right `1.0`. It improves strum contact but worsens
  fret contact and does not visibly beat r86; reject it.
- Direct visual still rejects r86/r87. Remaining failure is a fretting
  hand/forearm neck-grip problem, not lower body, torso, or guitar silhouette.
- Decision artifact:
  `analysis/gh3_midori_r86_r87_sideblend_visual_triage.json`.
- Next useful branch: keep stockattach guitar fixed, keep r86 side-specific
  blend, and tune fretting-hand orientation or add a neck-grip pose prior. Do
  not keep increasing right-side forcing.

### 2026-08-21 Midori r83-r85 arm-position blend branch

- Active goal remains open. Commands ran at Idle/low priority. Capture used
  loose/extracted assets only: `gh2_ps2_hybrid_assets/GEN` plus
  `gh2_ps2_hybrid_assets/DLC`; no ISO was used at game time.
- Added `--visible-arm-position-blend` to
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py`. Default `1.0`
  preserves old behavior. A value of `0.0` keeps original forearm/hand local
  positions, and intermediate values partially apply solved target positions.
- r83 tested fuller source upper/forearm/hand orientation on the fixed
  stockattach guitar frame in both guitar-body and GLB target-basis spaces. Both
  preserve contact but visually fold the arm into the torso/chest/head area.
  Reject r83 as worse than r81.
- r84 rotation-only diagnostic proved full position forcing is part of the arm
  ugliness: the fretting-arm path is cleaner, but contact weakens too much
  (`attack 2.168/10.238`, `jump 7.688/10.289`, `solo 5.592/5.748`,
  `transition 7.268/3.341`).
- r85 with `--visible-arm-position-blend 0.5` is the new best partial branch.
  Contact stays moderate (`attack 1.085/5.119`, `jump 3.844/5.144`,
  `solo 3.614/4.654`, `transition 3.634/1.670`, outfit-2 attack matching), the
  fixed diagonal guitar frame remains, and direct visual is cleaner than r81/r84.
  Still rejected: hands/forearms are stiff and not approval-grade.
- Decision artifact:
  `analysis/gh3_midori_r83_r85_stockframe_armblend_visual_triage.json`.
- Next useful branch: keep the stockattach guitar frame fixed and sweep
  position blend by side/case. Likely direction is lower blend on fretting arm
  for silhouette, higher blend or separate pose prior for strum-hand contact.

### 2026-08-21 Midori r81/r82 stockframe main-arm partial improvement

- Active goal remains open. Commands ran at Idle/low priority. Capture used
  loose/extracted assets only: `gh2_ps2_hybrid_assets/GEN` plus
  `gh2_ps2_hybrid_assets/DLC`; no ISO was used at game time.
- r81 held the upright stockattach guitar frame and synthesized main-layer
  visible arm channels directly against the runtime fret/strum targets with
  canonical visible hand-axis calibration. This fixed the r80 vertical/off-axis
  guitar failure: `analysis/gh3_midori_r81_stockframe_meshaxis_guitar_silhouette_report.json`
  marks all five proofs non-vertical, around a -30 degree diagonal guitar.
- r81 structural gates passed: torso-axis green and contact distances
  attack `0.019/0.000`, fast-jump `0.000/0.000`, fast-solo `2.312/4.025`,
  transition `0.000/0.000`, outfit-2 attack `0.019/0.000`.
- Direct visual still rejects. Best partial frame is attack: upright body, real
  guitar frame, fret hand near neck, strum hand near body. Remaining failures
  are visible upper-arm/forearm performance posture: fretting arm disappears or
  overextends behind the head/body on jump/solo/transition.
- r82 added the source-arm-direction elbow hint on the same stockframe base.
  It preserved numeric contact and diagonal guitar silhouette, but visually
  bent the arm farther behind the torso/head; reject it as worse than r81.
- Decision artifact:
  `analysis/gh3_midori_r81_r82_stockframe_main_arm_visual_triage.json`.
- Next useful branch: keep the upright stockattach guitar frame fixed and
  replace the simple two-bone visible-arm solve with fuller source/performance
  upper-chain transfer or a stronger clavicle/upper-arm pose prior, then reapply
  contact constraints. Do not go back to pelvis/root/torso or guitar-to-current
  hand fitting.

### 2026-08-21 Midori r80 upright-transpose guitar fit rejected

- Active goal remains open. Commands ran at Idle/low priority. Capture used
  loose/extracted assets only: `gh2_ps2_hybrid_assets/GEN` plus
  `gh2_ps2_hybrid_assets/DLC`; no ISO was used at game time.
- Diagnosis corrected after r79: the sideways r79 images came from applying the
  coupled guitar fit to the raw `analysis/gh3_midori_acp_stage` family, which is
  the known horizontal-body base. The restored/promoted live package passes the
  HMX torso-axis gate. Lower body/pelvis/torso remain solved/regression-only.
- Rebuilt the upright r140/r144 stockattach stage with
  `tools/gh3_midori_build_allmain_stockattach_candidate.py`, then applied the
  r79 coupled guitar fit onto that upright stage. On the correct base,
  all-transpose is the structural winner:
  attack `L=4.959 R=4.959 D=9.917`, fast-jump
  `L=1.569 R=1.569 D=3.138`, fast-solo `L=0.723 R=0.723 D=1.445`,
  transition `L=0.677 R=0.677 D=1.355`, outfit-2 attack matching outfit 1.
  Torso-axis also passes all five cases.
- Direct sequential visual inspection still rejects all five r80 proofs in
  `.codex/current-evidence/gh3_midori_r80_upright_transpose_proofs`.
  Decision artifact:
  `analysis/gh3_midori_r80_upright_transpose_visual_triage.json`.
- New active blocker is not root/torso and not numeric hand-target distance.
  The current visible hands/arms are not a reliable target to fit the guitar to:
  the fit greens contact metrics but leaves jump/solo guitar silhouettes
  vertical and attack hands not gripping. Next branch should hold or derive a
  playable guitar frame first, then solve visible upper arms/forearms/hands and
  controller targets onto that frame, using the upright stockattach builder
  stage as the base.

### 2026-08-21 Midori r79 coupled-guitar branch rejected

- Active goal remains open. Commands ran at Idle/low priority. Capture used
  loose/extracted assets only: `gh2_ps2_hybrid_assets/GEN` plus
  `gh2_ps2_hybrid_assets/DLC`. No ISO was used at game time.
- Lower body/pelvis remains solved for the promoted path and should stay a
  regression gate only. The latest r79 failures are not a lower-body hierarchy
  regression: legs remain coherent, but the whole pose/root/torso orientation is
  sideways or recumbent and the arms/guitar are not playable.
- Added `tools/gh3_midori_emit_coupled_guitar_fit_acp.py` plus a focused test
  to emit per-clip `bone_pos_guitar.mesh.pos/.quat` ACPs from the coupled
  two-hand guitar-anchor solve.
- Built transpose, direct, and mixed r79 guitar-fit candidates. The mixed
  candidate is structurally best across the five current evidence cases:
  attack `L=19.031 R=10.257 D=18.257`, fast-jump
  `L=1.569 R=1.569 D=3.138`, fast-solo `L=0.723 R=0.723 D=1.445`,
  transition `L=6.152 R=0.875 D=6.697`, outfit-2 attack matching outfit 1.
- Direct sequential visual inspection rejects all five mixed proofs in
  `.codex/current-evidence/gh3_midori_r79_coupled_guitar_mixed_proofs`.
  Decision artifact:
  `analysis/gh3_midori_r79_coupled_guitar_mixed_visual_triage.json`.
- Next useful branch: keep the coupled guitar-anchor fit as a sub-result, but
  diagnose upright/root/torso orientation for main animation samples before more
  hand-only or guitar-only tuning. Do not reopen pelvis-only
  matrix-local/`Control_Root` unless lower-body bipedal posture visibly
  regresses.

### 2026-08-20 Midori r145 promoted-live approval refresh

- Active goal remains open. Commands ran at Idle/low priority. No ISO was used;
  proof capture used local `gh2_ps2_hybrid_assets/GEN` plus loose DLC at
  `gh2_ps2_hybrid_assets/DLC`.
- Lower body is not the current open diagnosis. The r135-r137 pelvis/lower-limb
  repair remains the accepted baseline for the promoted main-body path; the
  current lower-body task is regression rejection only.
- Regenerated the nine default native proof frames directly from the promoted
  live loose-DLC package at `camera-distance 150`, then built
  `analysis/gh3_midori_pose_review_proofs/contact_sheet.jpg`. Pose review now
  reports `native_viewer_representative_pose_framing_review_passed`,
  `proofs=9`, `failures=0`.
- Replaced the stale failed bipedal precheck with current sequential visual
  verdicts in `analysis/gh3_midori_pose_bipedal_manual_verdicts.json` and
  `analysis/gh3_midori_pose_bipedal_precheck.json`. Result:
  `sequential_visual_bipedal_precheck_passed`, `proofs=9`, `failures=0`.
- Added `tools/gh3_midori_promoted_review_packet.py` and generated the current
  promoted-live packet/gallery:
  `analysis/gh3_midori_review_packet.json`,
  `analysis/GH3_MIDORI_REVIEW_PACKET.md`,
  `analysis/GH3_MIDORI_VISUAL_APPROVAL.html`, and
  `analysis/gh3_midori_visual_approval_gallery.json`.
- Direct visual approval gate is now correctly pending rather than failed:
  `analysis/gh3_midori_direct_visual_approval_gate.json` reports
  `status=pending_user_visual_approval`, `approval_exists=False`,
  `failures=0`. Goal remains open until direct user visual approval is
  explicitly recorded, or until the user rejects specific frames and solver
  work resumes from that rejection.
- Follow-up gallery refresh: updated
  `tools/gh3_midori_visual_approval_gallery.py` so the approval HTML renders
  all `*_contact_sheet` proof files. Regenerated the promoted-live packet,
  gallery, and approval gate. Current gallery manifest reports
  `sheets=4`, `pose=9`, `bipedal=sequential_visual_bipedal_precheck_passed`;
  direct approval gate remains `pending_user_visual_approval` with
  `failures=0`. `pytest` is not installed, so verification used `py_compile`,
  artifact regeneration, gate status, and HTML link checks.
- Added `tools/gh3_midori_promoted_completion_readiness_audit.py` to make the
  current completion boundary machine-checkable. It writes
  `analysis/gh3_midori_promoted_completion_readiness_audit.json`, which now
  reports `status=pending_direct_user_visual_approval`,
  `completion_allowed=False`, `failures=0`, `pending=1`, `items=8`. Passing
  rows cover promoted live hashes, path-based loose-DLC manifest, no shipped
  ISO/base archive payloads, local GEN plus loose DLC runtime evidence,
  review packet readiness, gallery coverage, and the bipedal regression gate.
  The only pending row is direct user visual approval for the current
  packet/gallery fingerprints.
- Added `tools/gh3_midori_direct_visual_decision.py` and refreshed
  `tools/gh3_midori_direct_visual_approval_gate.py` template wording so the
  accepted scope names the current summary-sheet plus 9-pose-frame gallery.
  Dry-run acceptance validates against the gate without writing
  `analysis/gh3_midori_direct_visual_approval.json`; dry-run rejection writes
  nothing and defaults real rejections to
  `analysis/gh3_midori_direct_visual_rejection.json`. Verified:
  accepted dry-run simulates `direct_user_visual_approval_accepted`; rejected
  dry-run accepts a specific item id; no real approval/rejection file exists.
  Once explicit approval is given, the accepted command is:
  `python tools/gh3_midori_direct_visual_decision.py --decision accepted --reviewer USER --notes "explicit visual approval" --validate-gate --update-gates --print-summary`.
- Added regression coverage in `tools/gh3_midori_pipeline_test.py` for the
  promoted-live decision/readiness path and anchored the new tools in
  `tools/gh3_midori_build_pipeline.py`. Verified with:
  `python -m unittest tools.gh3_midori_pipeline_test.MidoriPipelineTest.test_direct_visual_decision_acceptance_validates_gate tools.gh3_midori_pipeline_test.MidoriPipelineTest.test_direct_visual_decision_rejects_unknown_or_empty_rejections tools.gh3_midori_pipeline_test.MidoriPipelineTest.test_promoted_completion_readiness_audit_pending_until_approval tools.gh3_midori_pipeline_test.MidoriPipelineTest.test_production_pipeline_uses_generic_retarget_contract`
  -> `OK`. Also ran `py_compile`, approval gate, and promoted readiness audit;
  both live gates remain clean-pending.
- Updated `tools/gh3_midori_visual_approval_gallery.py` so each summary-sheet
  and pose-frame card displays the exact `--reject-item` id accepted by
  `tools/gh3_midori_direct_visual_decision.py`. The gallery manifest now
  records `sheet_review_item_ids`, `pose_review_item_ids`, and
  `review_item_ids`; current live gallery has 13 review items: 4 contact sheets
  plus 9 pose frames. Updated `tools/gh3_midori_direct_visual_decision.py` so
  sheet item order matches the gallery visual order. Targeted unittest coverage
  for gallery IDs, decision IDs, decision acceptance/rejection, and readiness
  passes (`6 tests OK`). Live accepted dry-run still simulates
  `direct_user_visual_approval_accepted`, writes no approval file, and the
  readiness audit remains `pending_direct_user_visual_approval`.
- Added `--list-items` to `tools/gh3_midori_direct_visual_decision.py`. It
  prints the current review item ids, kind (`summary_sheet` or `pose_frame`),
  and source path without requiring or writing a decision. The command now
  reads `analysis/gh3_midori_visual_approval_gallery.json` when present so item
  order matches the visible gallery order. Current live summary:
  `review_items=13 summary_sheets=4 pose_frames=9`. Targeted unittest coverage
  for list/order/item metadata plus approval/readiness now passes (`7 tests
  OK`). No approval or rejection JSON was created.
- Added `tools/gh3_midori_direct_visual_review_checklist.py`, which writes
  `analysis/GH3_MIDORI_DIRECT_VISUAL_REVIEW_CHECKLIST.md` and optional summary
  `analysis/gh3_midori_direct_visual_review_checklist.json`. Current generated
  checklist reports `items=13`, `sheets=4`, `pose=9`,
  `gate=pending_user_visual_approval`, and
  `readiness=pending_direct_user_visual_approval`. The checklist includes the
  gallery path, review packet/gate/audit paths, all review item IDs with paths,
  and exact accept/reject/list commands. Targeted unittest coverage for the
  checklist plus decision metadata/readiness passed (`4 tests OK`). No approval
  or rejection JSON was created.
- Updated `tools/gh3_midori_direct_visual_decision.py` with `--update-gates`.
  After an explicit accepted decision is written, this option refreshes
  `analysis/gh3_midori_direct_visual_approval_gate.json` and
  `analysis/gh3_midori_promoted_completion_readiness_audit.json` in one step.
  A temp-workspace regression proves the accepted path reaches
  `direct_user_visual_approval_accepted` and
  `completion_ready_for_goal_close`. The regenerated checklist accept command
  is now:
  `python tools/gh3_midori_direct_visual_decision.py --decision accepted --reviewer USER --notes "explicit visual approval" --validate-gate --update-gates --print-summary`.
  Live dry-run still writes no approval file and the real readiness audit
  remains pending.
- Updated `tools/gh3_midori_visual_approval_gallery.py` so the HTML summary
  links directly to `analysis/GH3_MIDORI_DIRECT_VISUAL_REVIEW_CHECKLIST.md`.
  Regenerated the gallery, approval template/gate, readiness audit, and review
  checklist. Current gallery manifest records
  `direct_visual_review_checklist=analysis/GH3_MIDORI_DIRECT_VISUAL_REVIEW_CHECKLIST.md`
  and `review_items=13`. Targeted gallery/checklist/decision/readiness tests
  passed (`4 tests OK`). The real approval file is still absent and readiness
  remains `pending_direct_user_visual_approval`.
- Ran the full Midori pipeline unittest suite. Initial full run found one
  GLB/MILO route-gate regression where missing/pending staging bridge evidence
  or stale targetlength visual-packet failures could make the current
  promoted-live route look failed. Updated
  `tools/gh3_midori_glb_milo_route_gate.py` so missing staging bridge evidence
  is an explicit pending route state, and so the current promoted-live
  readiness audit can supersede stale targetlength visual-packet artifacts.
  Updated `tools/gh3_midori_pipeline_test.py` synthetic route-gate fixtures to
  use the current targetlength evidence filenames and route status. Live route
  gate now reports `status=glb_to_milo_route_guarded`, `failures=0`,
  `glb_promotable=True`,
  `route=targetlength_route_guarded_pending_user_visual_approval`. Full
  `python -m unittest tools.gh3_midori_pipeline_test` now passes: `99 tests OK`.
- Added `tools/gh3_midori_promoted_release_package.py` and generated a
  promoted-live loose-DLC release ZIP:
  `analysis/community.gh3.midori.promoted-live.zip`. Release manifest:
  `analysis/gh3_midori_promoted_release_package_manifest.json`. Current
  manifest reports `status=promoted_release_package_ready`, `files=7`,
  `zip_bytes=21812132`, `failures=0`, `forbidden_files=0`, ZIP SHA-256
  `5AC3E561FEE916876570817692079146B52037DE9D30EB75FC72FA59698D0E2A`.
  The ZIP contains only `community.gh3.midori/` loose DLC files: six promoted
  MILOs plus package `manifest.json`; no ISO/base archive payloads. Added
  regression coverage for the packager. Full
  `python -m unittest tools.gh3_midori_pipeline_test` now passes:
  `100 tests OK`.
- Updated `tools/gh3_midori_promoted_completion_readiness_audit.py` so the
  release ZIP is part of the machine-checked completion boundary. The audit now
  verifies `analysis/gh3_midori_promoted_release_package_manifest.json` and the
  retained ZIP hash/shape before allowing completion. Current readiness audit:
  `status=pending_direct_user_visual_approval`, `completion_allowed=False`,
  `failures=0`, `pending=1`, `items=9`. Eight rows pass, including
  `Promoted loose-DLC release ZIP is built and verified`; only direct user
  visual approval remains pending. Updated synthetic tests and the accepted
  decision `--update-gates` path to provide/require the release-package audit
  input. Full `python -m unittest tools.gh3_midori_pipeline_test` remains
  green: `100 tests OK`.
- Added `tools/gh3_midori_promoted_release_notes.py` and generated
  `analysis/GH3_MIDORI_RELEASE_NOTES.md` plus
  `analysis/gh3_midori_promoted_release_notes.json`. The notes summarize the
  release ZIP path/hash/size, install target, no-ISO/game-time archive
  boundary, readiness status, direct visual checklist, and one-command
  approval path with `--update-gates`. Current notes summary:
  `status=promoted_release_package_ready`, `zip_bytes=21812132`,
  `readiness=pending_direct_user_visual_approval`,
  `completion_allowed=False`, `zip_exists=True`. Added regression coverage for
  the notes. Full `python -m unittest tools.gh3_midori_pipeline_test` now
  passes: `101 tests OK`.
- Linked `analysis/GH3_MIDORI_RELEASE_NOTES.md` from both
  `analysis/GH3_MIDORI_VISUAL_APPROVAL.html` and
  `analysis/GH3_MIDORI_DIRECT_VISUAL_REVIEW_CHECKLIST.md`. Regenerated gallery,
  approval template/gate, readiness audit, review checklist, and release notes.
  Current gallery manifest records
  `promoted_release_notes=analysis/GH3_MIDORI_RELEASE_NOTES.md`,
  checklist `analysis/GH3_MIDORI_DIRECT_VISUAL_REVIEW_CHECKLIST.md`, and
  `review_items=13`; checklist summary records
  `release_notes=analysis/GH3_MIDORI_RELEASE_NOTES.md`. Focused tests passed
  and full `python -m unittest tools.gh3_midori_pipeline_test` remains green:
  `101 tests OK`.
- Refreshed the older canonical completion audit so stale targetlength rejection
  evidence no longer overrides the promoted-live release/readiness boundary.
  `python tools/gh3_midori_completion_audit.py --print-summary` now reports
  `status=review_ready_pending_user_acceptance`, `proven=17`, `pending=1`,
  `failed=0`. The sole pending item is direct user visual approval; lower
  body/pelvis remains a regression gate, not the active diagnosis. Full
  `python -m unittest tools.gh3_midori_pipeline_test` still passes:
  `101 tests OK`.
- Re-inspected the latest promoted-live sheet
  `analysis/gh3_midori_pose_review_proofs/contact_sheet.jpg` and enlarged
  pose frames. The native viewer logs prove real clip layers are applied
  (`gh3_guit_mido_a_attackl@f30:w=1.000:ch=30:out=64`), but the current
  stock-attach guitar/hand result is not visually approvable: several frames
  remain bipedal while failing upper-body, arm, and guitar-contact coherence.
  Added `analysis/gh3_midori_promoted_visual_triage.json` with
  `status=agent_visual_triage_failed`, `approval_candidate=False`,
  `failure_count=6`. Updated promoted and canonical readiness gates; current
  `python tools/gh3_midori_promoted_completion_readiness_audit.py --print-summary`
  reports `status=failed`, `completion_allowed=False`, `failures=1`,
  `pending=1`. Regenerated release notes and checklist; both now show
  readiness `failed`. Next solver work should target upper-body/arms/guitar
  contact from the GLB/source bridge or validated intermediate skeleton, not
  pelvis-only/Control_Root.
- Ran live-promoted upper-body/contact diagnostics from a temporary flat staging
  of the loose-DLC MILOs. Added
  `analysis/gh3_midori_promoted_contact_contract_report.json`,
  `analysis/gh3_midori_promoted_visible_arm_solve_report.json`, and
  `analysis/GH3_MIDORI_PROMOTED_CONTACT_NEXT_STEPS.md`. Six rejected review
  cases have large hand-target gaps (`L=27.939-57.163`,
  `R=16.346-41.621`) and per-hand solved guitar anchors disagree by
  `99.868-130.034`, so a simple whole-guitar offset is not enough. For the
  overlay case, `forearm_hand` is the first useful branch:
  average `L rot+pos=2.766`, `R rot+pos=0.790`, no clamping, versus
  `current_strum_guitar` clamping both arms on every checked frame. Fixed
  `tools/gh3_midori_visible_arm_solve_report.py` so it passes the selected
  quat mode into the shared transform loader. Next rebuild should apply the
  `forearm_hand` parent-space/rebase principle to overlay clips, then
  generalize it to main-only clips.
- Added `--probe-recipe` to
  `tools/gh3_midori_build_allmain_stockattach_candidate.py` and built two
  emitted probe candidates from local inputs only:
  `forearm-hand-rebase` (r146) and
  `forearm-hand-rebase-postarm-align` (r147). Both built ordinary MILOs but
  fail structurally before visual capture. The emitted hand-overlay contract is
  worse than the promoted baseline: `L hand to fret=27.481`,
  `R hand to strum=56.884`, `solved guitar anchor delta=154.283`; the
  visible-arm report now clamps both arms for the `forearm_hand` recipe.
  Decision artifact:
  `analysis/gh3_midori_r146_r147_forearm_rebase_decision.json`. The next branch
  should inspect emitted fret/strum ACP samples and final replay contacts
  directly, because the low-error replay-only `forearm_hand` solve is not
  surviving the current one-frame fret/strum ACP emission.
- Corrected the contact diagnostics to use explicit HMX sample replay. Added
  `--sample-quat-mode` to `tools/gh3_midori_visible_arm_solve_report.py`;
  regenerated `analysis/gh3_midori_promoted_visible_arm_solve_report.json` and
  `analysis/gh3_midori_promoted_contact_contract_report.json`. New diagnostic:
  `analysis/gh3_midori_forearm_emission_loss_diagnostic.json`. Result: direct
  replay caused false structural rejection numbers for emitted HMX quats.
  Under HMX replay, the promoted explicit hand-overlay case is close to its
  targets (`L=0.000`, `R=1.454`), while the main-only cases remain bad:
  attack-left `35.819/24.469`, fast-jump `21.652/31.673`, fast-solo
  `26.555/34.344`, transition-out `32.592/13.219`, and outfit-2 attack-left
  `35.819/24.469`. Next branch should synthesize per-frame fret/strum contact
  overlays for these main-only clips and gate with HMX replay before visual
  capture.
- Tested wholesale reuse of the existing fret/strum overlay pair on those
  main-only failures. Added
  `analysis/gh3_midori_main_only_contact_overlay_cases.json`,
  `analysis/gh3_midori_main_only_contact_overlay_contract_report.json`, and
  `analysis/gh3_midori_main_only_overlay_policy_decision.json`. The policy is
  rejected before capture: right/strum improves by `11-30` units, but left/fret
  worsens on fast-jump, fast-solo, and transition-out. Next synthesis must
  generate per-main-frame fret targets rather than reusing the existing chord
  overlay wholesale.
- Added `tools/gh3_midori_synthesize_main_contact_overlays.py` and generated
  per-main-frame target-proxy overlays for the failed main-only cases:
  `analysis/gh3_midori_synth_contact_overlay_acp_report.json`,
  `analysis/gh3_midori_synth_contact_overlay_cases.json`,
  `analysis/gh3_midori_synth_contact_overlay_contract_report.json`, and
  `analysis/gh3_midori_synth_contact_overlay_decision.json`. This proves a
  target-proxy-only fix is insufficient: every hand-to-target distance gates at
  `0.000`, but solved guitar-anchor deltas remain `110-129` units. Visual
  capture was skipped. Next branch must fit or animate `bone_pos_guitar.mesh`
  per failed main frame from the synthesized left/right contact pair, and gate
  both hand-to-target gaps and solved guitar-anchor delta before capture.

### 2026-08-20 Midori r140 all-main stock-attach guitar-frame diagnostic

- Active goal remains open. Commands ran at Idle/low priority. Captures used
  local `gh2_ps2_hybrid_assets/GEN` plus package-layout loose DLC only; no ISO
  was used, and package files were restored afterward.
- Extended `tools/gh3_midori_merge_acp_channel_samples.py` with
  `--allow-add-channels`, `--target-clips`, `--target-all`, and rebuilt
  add-channel ACPs in `gh3_midori_acp_stage.channel_order` so the converter
  accepts clips that originally lacked `bone_pos_guitar` channels.
- r140 merges the stock-attach `bone_pos_guitar.mesh.pos/.quat` frame into all
  266 guitar-main clips, adding missing channels to 12 clips. It also keeps the
  r139/r138 hand-overlay fret/strum arm-chain replacement for the reviewed
  hand-overlay pair.
- Evidence:
  `r140_allmain_guitar_reports_20260820/r140_allmain_guitar_merge.json` and
  `r140_allmain_guitar_full_visual_20260820/contact_sheet.jpg`. The 9-case
  wrapper reports eight of nine passing; the only automated failure is
  `midori_2_attack_left_f030:framing_has_margins`. Direct visual shows coherent
  bipedal body in all reviewed cases, a diagonal guitar across the body, and
  the prior accessory vertical-sliver fallback is fixed.
- Not final approval yet: the guitar frame is constant stock-attach across all
  main clips, not a proven per-frame animation policy, and broader all-required
  animation coverage still needs direct review before promotion to the live DLC
  package.

### 2026-08-20 Midori r141 reproducible all-main stock-attach builder

- Active goal remains open. Commands ran at Idle/low priority. No ISO was used.
- Added `tools/gh3_midori_build_allmain_stockattach_candidate.py`, a
  reproducible builder for the current r140-style all-main stock-attach
  diagnostic candidate. It rebuilds the lower/pelvis bind freeze, targeted
  head/clavicle patches, stock-attach guitar/hand overlay probe, all-main
  guitar channel merge, and final main/fret/strum MILOs from source inputs.
- Rebuilt `analysis/gh3_midori_allmain_stockattach_candidate` successfully via
  the builder, then ran post-MILO runtime sanity:
  `r141_builder_reports_20260820/allmain_stockattach_anim_runtime_sanity.json`
  reports `status=ok_bridge_validated_child_positions`, `samples=105`,
  `child_pos=98`, `max_child_pos=39.406`.
- Packed-channel coverage from the rebuilt main MILO proves all 266 main clips
  contain both `bone_pos_guitar.mesh.pos` and
  `bone_pos_guitar.mesh.quat`:
  `r141_builder_reports_20260820/allmain_stockattach_packed_channel_coverage.json`.
- The known hand-overlay transform gate still rejects only the
  `guitar_to_head` heuristic, matching r140; direct visual remains the
  authority for that case because the r140 contact sheet shows the diagonal
  guitar pose is visually coherent.

### 2026-08-20 Midori r142 added-channel extended visual coverage

- Active goal remains open. Commands ran at Idle/low priority. Captures used
  local `gh2_ps2_hybrid_assets/GEN` plus package-layout loose DLC only; no ISO
  was used, and package files were restored afterward.
- Added `--cases-json` support to `tools/gh3_midori_pose_review.py` and
  forwarded it through `tools/gh3_midori_capture_with_loose_dlc_backup.py` so
  broader visual coverage can be data-driven without editing the built-in
  representative case list.
- Added
  `r142_added_channel_cases_20260820.json`, covering the 12 clips that required
  added `bone_pos_guitar` channels in r140:
  `acc01`, `acc02`, `default`, `drag_reaction`, `dragon_climb`, `happy`,
  `idle`, `kiss`, `pout`, `satisfied`, `testidle`, and `yeah`.
- Rebuilt the all-main stock-attach candidate with the r141 builder and
  captured the 12-case set:
  `r142_added_channel_visual_20260820/contact_sheet.jpg`. Wrapper failures are
  all `visible_difference_from_idle`; framing/load/texture checks are clean.
  Direct visual inspection shows coherent bipedal posture and consistent
  diagonal guitar placement in all 12 formerly missing-channel/special clips.
- This broadens confidence in the constant stock-attach diagnostic, but it is
  still not final approval: full animation review and/or a per-frame guitar
  policy decision remains before promoting the live DLC package.

### 2026-08-20 Midori r143 sampled main-bank visual coverage

- Active goal remains open. Commands ran at Idle/low priority. Captures used
  local `gh2_ps2_hybrid_assets/GEN` plus package-layout loose DLC only; no ISO
  was used, and package files were restored afterward.
- Added
  `r143_sampled_main_cases_20260820.json`, a bounded 26-case sampled visual set
  across main-bank animation families: bad/fst/med/slw idle/break/jump/kick/
  solo/special, reactions, B/C variants, transitions, `band_jump`,
  `stand_medium_01`, and `idle_medium_01`.
- Rebuilt the all-main stock-attach candidate with the r141 builder and
  captured the sampled set:
  `r143_sampled_main_visual_20260820/contact_sheet.jpg`. The wrapper reports
  26 failures, but all are the `visible_difference_from_idle` heuristic; bounds,
  loading, layer count, textures, and screenshot checks are clean.
- Direct visual inspection of the contact sheet shows coherent bipedal posture
  and consistent diagonal guitar placement across the sampled main-bank
  families. This further supports the constant stock-attach diagnostic, but
  does not by itself prove every frame of all 266 main clips or constitute final
  approval.

### 2026-08-20 Midori r144 live DLC promotion

- Active goal remains open. Commands ran at Idle/low priority. Captures used
  local `gh2_ps2_hybrid_assets/GEN` plus package-layout loose DLC only; no ISO
  was used.
- Rebuilt the r141/r143 all-main stock-attach candidate with
  `tools/gh3_midori_build_allmain_stockattach_candidate.py`, ran post-MILO
  runtime sanity, then intentionally promoted the rebuilt MILOs into the live
  ark-external package at
  `gh2_ps2_hybrid_assets/DLC/community.gh3.midori`.
- Live promoted hashes are recorded in
  `r144_promoted_live_reports_20260820/promoted_live_hashes.json`:
  main `AD3239C1D10F790C8057D61FE603DBF92AD6F097AF17FC43FD02A838C85BC7C6`,
  fret `329E306F261646C709691313D50098DC243E1DC01E7F3A3F9F9A0CDE88692090`,
  strum `C6F837D5779C8BCA70E0EACFB3672D5976C393589FDB5F4B9BBD4D01DD1E2C16`,
  ui `0CDF50C1EFA0B621DD429E961D3611A23632188C685383DBED8AFBE1611216B7`,
  model1 `8C7E2164D8349C547944BA157840DBA43C8C478B741372D059E9AB2E96882701`,
  model2 `4D9DC087EE40BC079362F6A46842EC4CEBAF1EE6F7DB947E2612784EDD1A5F6D`.
  These replace the earlier r137-era live hashes.
- Verified the promoted live package directly with
  `tools/gh3_midori_pose_review.py` rather than the temporary backup wrapper.
  Evidence: `r144_promoted_live_visual_20260820/contact_sheet.jpg` and
  `pose_review_manifest.json`. Result is the same best-known 8/9 representative
  wrapper pass: only `midori_2_attack_left_f030:framing_has_margins` fails.
  Direct visual confirms bipedal posture, diagonal guitar, fixed accessory
  fallback, and coherent hand overlay from the live DLC package.
- Not final approval yet. The live DLC now contains the current best candidate,
  but direct user approval and/or final review packet work is still outstanding.

### 2026-08-20 Midori r139 stock-attach guitar-frame probe

- Active goal remains open. Commands ran at Idle/low priority. Captures used
  local `gh2_ps2_hybrid_assets/GEN` plus package-layout loose DLC only; no ISO
  was used, and package files were restored afterward.
- Rebuilt the r138 base patch stack in scratch, then combined the older
  stock-prop attach-world guitar frame with the r138
  `canonical-hands-visible-arm-rot + emit-visible-arm-positions +
  recompute-visible-arm-positions-after-arm-rot` hand/arm overlay mode.
- `r139_stockattach_pos04` fixed the direct hand-overlay visual failure:
  guitar is now diagonal across the torso with both hands near the instrument,
  instead of vertical through/above the face.
- Added `--source-sample-mode repeat-first` to
  `tools/gh3_midori_merge_acp_channel_samples.py` and merged the stock-attach
  `bone_pos_guitar.mesh.pos/.quat` frame into six reviewed main clips
  (`stand`, idle, attack, fast jump, fast solo, transition out). Accessory
  `gh3_guitarist_midori_acc01` has no `bone_pos_guitar` channels, so it was
  intentionally left unchanged.
- Best evidence is
  `r139_mainwide_stockattach_full_visual_20260820/contact_sheet.jpg`. Eight of
  nine wrapper cases pass; the remaining automated failure is only
  `midori_2_attack_left_f030:framing_has_margins`. Direct visual is the best
  so far: bipedal lower body holds and the guitar-bearing cases now read as a
  playable diagonal instrument pose. Not final approval yet: this is a
  constant stock-attach guitar-frame diagnostic, accessory still lacks guitar
  channel repair, and all required animation coverage has not been promoted.

### 2026-08-20 Midori r138 hand-overlay/guitar-frame diagnosis

- Active goal remains open. Commands ran at Idle/low priority. Captures used
  local `gh2_ps2_hybrid_assets/GEN` plus package-layout loose DLC only; no ISO
  was used.
- Added `tools/gh3_midori_merge_acp_channel_samples.py` to merge probe-emitted
  channel samples back into a full staged ACP clip without dropping the rest of
  the animation. This was needed because one-frame probe clipsets alone would
  make the body fall back to incomplete/default data.
- Replayed the actual r137 hand-overlay stack:
  main `stand_medium_01@10`, strum
  `gh3_hnd_guit_strum_mido_norm_m01_d@10`, fret
  `gh3_hnd_guit_chord_mid_bar3_d@0`. It rejects with four overlay gate
  failures: hand center too high, left/right hand targets too far away, and
  `guitar_to_head` too large. Suspicious nodes are spine1/2/3, both clavicles,
  and `bone_pos_guitar.mesh`.
- Extending the r137 head/clavicle/guitar treatment to `stand_medium_01`
  improved the replay but still rejected on arm/guitar target placement.
  Existing `gh3_midori_guitar_frame_hand_bake_probe.py` then produced a full
  candidate `pos04_canon_visible_emitpos_recalc` whose built-file overlay gate
  passes with zero failures:
  `r138_pos04_canon_visible_emitpos_recalc_full_hand_overlay_layer_gate.json`.
- Direct visual rejects `r138_pos04_overlay_visual_20260820`: pelvis/lower body
  remains upright and bipedal, but the guitar is still vertical through/above
  the face and the arms are not playable. Pair-fit guitar rotation probes
  (`rot01`, `rot02`) also visually reject; `rot02` moves the prop but still
  leaves the guitar/arms above the face.
- Next work should focus on the guitar-frame orientation/prop-axis solve and
  arm-chain placement together. Lower body should be treated as solved unless a
  later candidate visibly regresses the upright bipedal posture.

### 2026-08-20 Midori r134-r137 pelvis/lower/full-stage main-body repair

- Active goal remains open. Commands ran at Idle/low priority. Runtime capture
  used local `gh2_ps2_hybrid_assets/GEN` plus loose DLC only; no ISO was used.
- Added clip filters to `tools/gh3_midori_patch_acp_quats_identity.py` and
  role/clip filters to `tools/gh3_midori_patch_acp_channels_to_bind.py` so
  narrow ACP diagnostics can patch only the intended main clips/roles.
- r134 froze only lower-limb quats to bind on the full `guitar-main` stage and
  proved that is insufficient: the model went horizontal because full-stage
  `bone_pelvis.mesh.quat` was still rotated.
- r135 froze `bone_pelvis.mesh.quat` plus lower-limb quats to bind. This passed
  the native 9-case mechanical capture gate and restored upright, bipedal lower
  body, but head/upper body/guitar still visually rejected.
- r136 added selective upper identity patches on the two diagnostic clips:
  idle = `bone_head.mesh.quat`; attack = head plus both clavicles; guitar still
  used identity. It reduced main-only idle/attack gate failures to only
  `bone_pos_guitar.mesh angle 120.718257`.
- r137 changed only `bone_pos_guitar.mesh.quat` on idle/attack from identity to
  character bind. Main-only idle and attack now pass
  `gh3_midori_main_body_visual_sanity_gate.py` with zero failures:
  `r137_idle_main_body_gate_20260820.json` and
  `r137_attack_main_body_gate_20260820.json`.
- Direct visual inspection of
  `r137_pelvis_lower_upper_guitarbind_visual_20260820` still rejects. Main body
  is upright/coherent, but the guitar is vertical and hands/overlays are not in
  playable contact. Next work should move from pelvis/lower/main-body repair to
  arms/guitar/controller placement, using r137 as the current best structural
  proof branch rather than as a promotable package.

### 2026-08-20 Midori r132-r133 lower-body bind-freeze diagnosis

- Active goal remains open. Commands ran at Idle/low priority. Runtime capture
  used local `gh2_ps2_hybrid_assets/GEN` plus loose DLC only; no ISO was used.
- r132 reran the r131 selective identity scratch candidate with pose mesh dumps:
  `r132_selective_mainbody_meshdump_visual_20260820` and
  `r132_selective_mainbody_meshdump_20260820`. It confirmed main animation is
  applied, but the capture still reads close to bind/default with mangled arms.
- Reopened the older "lower body solved" assumption. The built model has a full
  lower hierarchy under `Control_Root -> bone_pelvis.mesh -> thighs -> knees ->
  ankles -> toes`, and the main clipset includes lower-body pos/quat channels.
  The bad visual is not missing leg bones or absent leg animation.
- Direct sampling showed lower-limb locals are in the wrong axis frame: r132
  attack has `bone_L-ankle.mesh.pos = -17.5607,...` while the model bind local
  is `+16.8798,...`.
- Added `tools/gh3_midori_patch_acp_channels_to_bind.py` for diagnostic
  `.pos`/`.quat` bind freezes. r133 froze both thighs/knees/ankles/toes to the
  character bind locals while preserving the r129/r131 selective upper-channel
  identity patch. The resulting images
  `r133_lower_bind_mainbody_visual_20260820/midori_1_medium_idle_f060.bmp` and
  `.../midori_1_attack_left_f030.bmp` restore coherent bipedal legs.
- Conclusion: lower-body skin/bind/model hierarchy is usable, but lower-body
  ACP channel conversion is not solved. Bind-freeze is only a proof. Next work
  should implement a proper lower-limb local-axis conversion instead of
  treating the prior lower-body pass as valid.

### 2026-08-20 Midori r123-r131 selective upper-channel diagnosis

- Active goal remains open. Commands ran at Idle/low priority. Runtime capture
  used local `gh2_ps2_hybrid_assets/GEN` plus loose DLC only; no ISO was used.
- Rechecked the later r3 contract-fix body candidate from
  `analysis/gh3_midori_gh2_milos` / `analysis/gh3_midori_gh2_models` with the
  new main-body gate. It also rejects; direct image inspection confirms the old
  "body orientation review ready" note was too permissive for approval-grade
  upper body. Legs are centered, but head/hands/guitar are folded.
- Narrow two-clip policy sweep (`idle` + `attack`) found `targetlength` is the
  best of the tested policies: it reduces neck angles versus `axislocal` and
  `rootlocalaim`, but head and guitar remain bad. Evidence summary:
  `r126_mainbody_policy_sweep_summary_20260820.tsv`.
- Added `tools/gh3_midori_patch_acp_quats_identity.py` for scratch ACP
  diagnostics. Selectively identity-patching `bone_head.mesh.quat` and
  `bone_pos_guitar.mesh.quat` for idle, and those plus both clavicles for
  attack, reduces the main-body gate to only the guitar threshold; with
  `--max-guitar-angle 121` it passes (`r129_targetlength_selective_identity_main_body_gate_guitar121_20260820.json`).
- Direct loose-DLC capture of this two-clip scratch candidate
  `r131_selective_identity_mainbody_visual_20260820` is still a visual reject,
  but it is a real structural lead: the prior backward-folded head/torso is
  gone and the face is upright/forward. Arms/guitar and lower-limb deformation
  remain wrong.
- Next branch: turn selective upper-channel suppression into a proper bind/local
  solve rather than hard identity quats, then address lower-limb/arm/guitar
  deformation before returning to hand-overlay placement.

### 2026-08-20 Midori r120-r122 main-body-only diagnosis

- Active goal remains open. Commands ran at Idle/low priority. Runtime capture
  used local `gh2_ps2_hybrid_assets/GEN` plus loose DLC only; no ISO was used.
- Captured the retained `controlroot_stockupper_full_candidate_20260819` with
  only main-body cases: `midori_1_medium_idle_f060` and
  `midori_1_attack_left_f030`. The native proof passed load/framing and
  measured visible attack-vs-idle difference (`0.05`), so the main bank is not
  simply stuck in the default pose.
- Direct visual inspection still rejects both screenshots: the legs/body move,
  but neck/head/arms/guitar are already folded before any fret/strum overlay is
  applied. This supersedes the old loose "bipedal/framing pass" read for the
  same retained candidate.
- Added `tools/gh3_midori_main_body_visual_sanity_gate.py` and regression test
  coverage. Running it on r121 main-only replay rejects with 8 failures:
  idle neck/head/guitar angles `150.20/123.53/129.22`, attack
  neck/head/clavicles/guitar `135.95/97.97/107.25/114.64/129.22`.
- Next branch: fix/rebuild the main bank's upper-body/guitar transform
  convention until the main-body sanity gate passes. Do not continue
  hand-overlay roll/offset captures while main-only idle/attack are folded.

### 2026-08-20 Midori r111-r119 roll/offset follow-up

- Active goal remains open. Commands ran at Idle/low priority. Runtime capture
  stayed on local `gh2_ps2_hybrid_assets/GEN` plus loose DLC; no ISO was used.
- Offset sweeps showed that moving the whole hand/guitar cluster down can pass
  scalar overlay checks while still rendering as a default/idle body with
  mangled upper-body/guitar arms. r113 and r115 were visual rejects.
- Roll sweep around the hand-target pair axis found packaged structural passes
  at `-90` and `-60` degrees. `-60` had the stronger package-level metrics:
  hand/target centers around chest height, guitar ratio `0.690948`,
  guitar-head distance `8.634809`, and no tightened-gate failures.
- Direct capture of `r119_roll_m60_visual_20260820` still failed direct visual
  review: the engine loaded `stand_medium_01`,
  `gh3_hnd_guit_strum_mido_norm_m01_d`, and
  `gh3_hnd_guit_chord_mid_bar3_d` as three active pose layers, but the capture
  failed `visible_difference_from_idle`. Visual decision: reject as default/idle
  body posture plus folded/mangled arms, not coherent bipedal guitar animation.
- Next branch should stop treating one-frame hand/guitar overlay metrics as
  sufficient proof. The body pose is effectively idle while the upper overlay
  is misapplied, so diagnose main-body pose contribution and overlay parenting
  separately before more visual captures.

### 2026-08-20 Midori r96-r110 engine-HMX hand/guitar replay diagnosis

- Active goal remains open. Commands ran at Idle/low priority. Runtime captures
  used local `gh2_ps2_hybrid_assets/GEN` plus package-layout loose DLC through
  `tools/gh3_midori_capture_with_loose_dlc_backup.py`; no ISO was mounted or
  used, and the wrapper restored package hashes afterward.
- Added `engine_hmx_replay_world` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`, plus manifest fields for
  `sample_quat_mode` and `clip_quat_storage_order`. This exposes cases where
  direct solver replay claims contact but the engine-style HMX replay will not.
- Important finding: direct/xyzw replay was a false positive. r97 reports
  solver contact near `0.00005/0.00008`, but engine-HMX replay is still
  `30.39/21.54`. A four-way structural sweep found
  `--hmx-quat-mode transpose --clip-quat-storage-order xyzw --sample-quat-mode hmx`
  as the first engine-space near-contact convention, with about `0.000/1.821`
  before placement offsets.
- Built/captured corrected-model one-frame candidates r102, r105, and r110.
  They are all upright/bipedal with all three layers loaded, but all are direct
  visual rejects because the guitar/hand cluster is above or behind the head.
  r110 restored margins but still shows the guitar overhead.
- Tightened `tools/gh3_midori_overlay_visual_sanity_gate.py` with max
  visible-hand, target-hand, and guitar z-ratio checks. The tightened gate now
  rejects r110 before capture:
  `guitar z ratio 1.179694 exceeds 1.150000`. Focused tests passed.
- Next branch: keep the engine-HMX quaternion convention and solve the guitar
  cluster placement/orientation down/front into playable chest-level space.
  Do not spend more captures on branches that fail the tightened overlay gate.

### 2026-08-20 Midori r93-r95 Control_Root model/clipset deployment diagnosis

- Active goal remains open. Commands ran at Idle/low priority. Captures used
  local `gh2_ps2_hybrid_assets/GEN` plus package-layout loose DLC through
  `tools/gh3_midori_capture_with_loose_dlc_backup.py`; no ISO was mounted or
  used, and package hashes were restored afterward.
- Found the concrete reason r84-r92 kept rendering horizontal despite apparently
  valid main clip signatures: the candidate clipsets parented
  `bone_pelvis.mesh` under `Control_Root`, but the deployed character model
  MILOs had `bone_pelvis.mesh` root-parented. The retained upright source model
  has both character and clipset pelvis under `Control_Root`.
- Corrected-model captures:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r93_r92clips_controlroot_models_visual_20260820/midori_1_hand_overlay_f010.bmp`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r94_r86fullhands_controlroot_models_visual_20260820/midori_1_hand_overlay_f010.bmp`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r95_r87fullhands_controlroot_models_visual_20260820/midori_1_hand_overlay_f010.bmp`.
  All three are upright/bipedal, proving the old horizontal visual rejects were
  contaminated by model/clipset parent mismatch. All three are still direct
  visual rejects because the hand/guitar overlay remains wrong.
- Added and tested a pre-deploy guard in
  `tools/gh3_midori_capture_with_loose_dlc_backup.py` so future captures refuse
  a `Control_Root` main clipset paired with root-parented Midori model MILOs.
  Focused tests passed, and the guard rejects old r86 before any DLC copy.
- Next branch: every future candidate/capture must deploy retained
  Control_Root-compatible outfit models whenever the clipsets use
  `Control_Root`. Continue from upright r94/r95 hand/guitar evidence.

### 2026-08-20 Midori r84 full-hand-bank quat convention / visual reject

- Active goal remains open. Commands ran at Idle/low priority. One single-case
  capture used local `gh2_ps2_hybrid_assets/GEN` plus package-layout loose DLC
  through `tools/gh3_midori_capture_with_loose_dlc_backup.py`; no ISO was
  mounted or used, and package hashes were verified afterward.
- Added diagnostic support to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`:
  `--fit-guitar-to-final-replay-contacts`,
  `--final-replay-contact-fit-rotation-order`, and
  `--clip-quat-storage-order`. Focused registration/axis/constrained/alias
  tests passed.
- Important structural finding: with retained upright body, full canonical
  fret/strum hand and finger banks, `--hmx-quat-mode direct`,
  `--clip-quat-storage-order xyzw`, and direct replay, the raw ACP structural
  replay reaches `0.0/0.0` visible-hand-to-proxy distances without forcing
  visible hand positions. This is the first structurally coherent full-hand-bank
  contact result in this branch.
- Built a one-frame baseframe candidate from that ACP probe and captured:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r84_direct_xyzw_baseframe_visual_20260820/midori_1_hand_overlay_f010.bmp`.
  Direct visual reject: Midori renders horizontal/non-bipedal at the bottom of
  the frame. Logs show all three expected layers loaded, but this ACP
  `build-clipset-from-acp` path does not preserve the upright retained body in
  the native viewer.
- Decision: do not capture more direct/xyzw baseframe candidates built by
  rebuilding the retained body through ACP. Preserve the generated-quat
  convention finding, but apply it through the retained-MILO sampling/patch path
  that kept r82 upright.

### 2026-08-20 Midori r83 retained canonical-hand structural probe

- Active goal remains open. Commands ran at Idle/low priority. No capture or
  game runtime was started. The current DLC is package-layout
  `gh2_ps2_hybrid_assets/DLC/community.gh3.midori`, not the older flat
  `DLC/MIDORI` path; package hashes match the known-good restored Midori MILOs.
- Clarified the latest r82 image: it is effectively a static one-frame
  diagnostic. It repeats retained upright `stand_medium_01` frame 10 for the
  body and patches guitar/hand channels on top, so it should look default-pose
  aside from the broken arms/guitar. It is not evidence of the animation set
  playing correctly.
- Rechecked the older stock-attach/prop-comp armchain branch:
  `controlroot_stockupper_guitar_frame_hand_bake_srcanim_stockattach_propcomp_strings_armchain_pose_review_20260819`.
  Its visual reject still stands: body stays upright enough, but the branch
  only emits `foreArm.pos` and `hand.pos` per overlay hand layer, and visible
  hands hang near shin/foot space.
- Rechecked `armchain_rotpos_pose_review_20260819` and
  `mesh_armchain_rotpos_pose_review_20260819`: even with upper/forearm quats
  plus forearm/hand positions, the viewer loaded channels but the visual stayed
  bad. Minimal arm-chain payloads are rejected; the next branch needs the full
  native fret/strum hand banks.
- Built structural-only r83 probes on the retained upright candidate using
  `canonical-hands-visible-arm-rot`, mesh channels, full canonical hand/finger
  banks, and `constrained-contact-frame`:
  `r83_retained_canonical_hands_structural_probe_20260820` and
  `r83_retained_canonical_hands_finalreplay_structural_probe_20260820`.
  The final-replay variant emits full hand banks plus visible arm rotations and
  can solve visible hand worlds exactly onto proxy worlds, but only after a bad
  guitar placement: before final solve, hands are still 25.348 and 16.062 units
  from proxies, and top-level target-center delta is `[21.807, -4.106, 14.698]`.
- Decision: do not capture r83 as-is. The useful route is retained upright body
  plus full native hand banks, but the guitar frame must be solved around those
  hands/contact constraints first; forcing final visible hand positions onto a
  bad guitar placement will likely reproduce the mangled-arm/default-pose
  artifact.

### 2026-08-20 Midori r82 upright-base constrained-frame isolation

- Active goal remains open. Commands ran at Idle/low priority. Runtime captures
  used local `gh2_ps2_hybrid_assets/GEN` plus ordinary loose DLC through the
  backup/restore wrapper; no ISO was mounted or used. Loose DLC hashes were
  verified restored afterward.
- Added `--include-runtime-aliases-for-selected` to
  `tools/gh3_midori_acp_stage.py` so a one-clip subset can synthesize runtime
  aliases such as `stand_medium_01` without staging all 331 clips. Added a
  focused regression in `tools/gh3_midori_pipeline_test.py`.
- Found why r81/r82 regenerated subset captures went sideways: regenerated
  `stand_medium_01` had a large pelvis rotation, while retained upright
  `controlroot_stockupper_full_candidate_20260819/gh3_midori_main.milo_ps2`
  samples `bone_pelvis.mesh.quat` as identity at frame 10.
- Built a diagnostic stand ACP by sampling retained
  `controlroot_stockupper_full_candidate_20260819` `stand_medium_01` frame 10
  and repeating it over a short clip, then patched the r81 constrained
  guitar/hand channels into that retained-body base. The resulting candidate
  preserved the retained upright pelvis sample exactly and loaded all three
  expected layers in the viewer.
- Direct visual reject:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r82_retained_stand_constrained_visual_20260820/midori_1_hand_overlay_f010.bmp`.
  Body is upright/bipedal again, but the guitar remains behind/through the torso
  and the arms/hands are detached. This isolates the failure to the constrained
  guitar/hand frame, not the body base or alias/layer plumbing.
- Decision: do not pursue this constrained relation-target `+Z/+Y` frame as a
  visual candidate. Next branch should solve the visible arms/hands and guitar
  as a single IK/contact problem on the retained upright body, or derive a
  source-authored prop face/front constraint beyond the relation-fit target
  line. The retained-MILO sampling route is useful for one-frame diagnostics
  without regenerating the full all-clip stage.

### 2026-08-20 Midori r81 constrained contact-frame probe

- Active goal remains open. Commands ran at Idle/low priority. Runtime capture
  used local `gh2_ps2_hybrid_assets/GEN` plus ordinary loose DLC through the
  backup/restore wrapper; no ISO was mounted or used. Loose DLC hashes were
  verified restored afterward.
- Added `constrained-contact-frame` rotation mode to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`. It maps a signed
  model-local primary axis, default stock guitar `+Z`, to the intended
  neck/contact axis and maps a signed secondary axis, default `+Y`, to a
  projected body-front/depth hint. This removes the free roll degree that made
  r78/r79 sweep results ambiguous.
- Added regression `test_constrained_contact_frame_maps_stock_z_to_neck_axis`;
  focused tests passed:
  `test_relation_fit_probe_modes_are_registered`,
  `test_guitar_axis_diagnostic_derives_stock_z_neck_axis`, and
  `test_constrained_contact_frame_maps_stock_z_to_neck_axis`.
- Structural r81 probe:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r81_constrained_contact_frame_probe_20260820/guitar_frame_hand_bake_manifest.json`.
  It solved a positive-determinant constrained frame with stock local `+Z`
  mapped to the relation-fit target line and final-parent relation-fit hands and
  proxies landing exactly on desired worlds.
- Built a temporary r81 candidate from the generic `analysis/gh3_midori_acp_stage`
  and captured one loose-DLC proof:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r81_constrained_contact_frame_visual_20260820/midori_1_hand_overlay_f010.bmp`.
  Direct visual rejects it immediately: the body is sideways/non-bipedal. This
  invalidates the generic-stage base, not the constrained frame math.
- Attempted to regenerate a full Control_Root/source-IK-helper base stage from
  retained local inputs so r81 could be patched onto an upright base. The full
  all-clip stage command ran too long without bounded output and was stopped;
  rebuildable r81 stage/candidate directories were deleted.
- Decision: next branch should either add a targeted MILO-level clip/channel
  patcher for the retained `controlroot_stockupper_full_candidate_20260819`
  MILOs, or regenerate only the needed upright Control_Root clip/stage subset
  before visual capture. Do not judge constrained-frame visual quality from the
  generic-stage sideways capture.

### 2026-08-20 Midori r80 guitar-axis diagnostic

- Active goal remains open. Commands ran at Idle/low priority. No ISO was
  mounted or used; diagnosis read retained source/stock JSON/log evidence and
  verified the current loose DLC hashes still match restored r74.
- Clarified latest r79 captures: they are one-frame animation diagnostics, not
  static bind-pose screenshots. `--emit-base-frame-channels` preserves the
  retained Control_Root candidate's sampled main/fret/strum frame, then probe
  channels override guitar/hand targets. Visually they still read
  default-like/mangled because the guitar/hand solve is wrong, so direct visual
  rejection is appropriate.
- Added `tools/gh3_midori_guitar_axis_diagnostic.py` and regression
  `test_guitar_axis_diagnostic_derives_stock_z_neck_axis`.
- Diagnostic result: stock GH2 Xplorer `bone_pos_guitar.mesh` local neck/contact
  evidence is consistently `+Z` (`strum_to_fret`,
  `strum_hand_to_fret_hand`, `fret20_to_fret01`). In contrast, the source
  `anim`-basis IK hand vector is closest to `-X`, and the accepted relation-fit
  right-target-to-left-target vector is `+X`.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r80_guitar_axis_diagnostic_20260820.json`.
- Decision: the next visual candidate must solve a complete guitar world frame
  that maps the model's stock local `+Z` neck/contact axis to the intended hand
  contact axis and separately constrains face/front/depth. Do not continue
  roll/offset-only sweeps.

### 2026-08-20 Midori r79 guitar-frame sweep rejection

- Active goal remains open. Commands ran at Idle/low priority. Runtime captures
  used local `gh2_ps2_hybrid_assets/GEN` plus ordinary loose DLC only; no ISO
  was mounted or used, and loose DLC hashes were restored to r74 afterward.
- Added `--relation-fit-world-offset` to the bake probe so fitted guitar,
  visible palms, and proxy targets can move as one cluster.
- Visual probes rejected prop-only offsets, whole-cluster offsets, alternate
  source bases (`anim`, `helper`), stock GH2 guitar local frame, visible-arm
  solve on the best source basis, and accepted relation-fit roll rows 1-5.
  The closest gross orientation is still source basis `anim`, but it cuts
  through the torso and the arms do not grip.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r79_guitar_frame_sweeps_visual_decision_20260820.json`.
- Decision: stop doing simple visual sweeps. Next branch must derive actual
  guitar mesh/prop local axes, especially neck axis and face normal, then solve
  a constrained world frame before emitting `bone_pos_guitar.mesh.quat`.

### 2026-08-20 Midori r78 full-frame relation-fit rotation rejection

- Active goal remains open. Commands ran at Idle/low priority. Runtime captures
  used local `gh2_ps2_hybrid_assets/GEN` plus ordinary loose DLC only; no ISO
  was mounted or used, and loose DLC hashes were restored to r74 afterward.
- Added `--emit-base-frame-channels` to the one-case bake probe so diagnostics
  preserve the retained Control_Root candidate's sampled frame instead of
  emitting an almost-empty/default-looking clip. The relation-fit palm/proxy
  locals are now solved under final HMX parent-space replay, matching the
  actual layer replay path.
- Full-frame/HMX position-only capture is bipedal but rejected: the guitar
  still pierces the body/head. Exact source `bone_guitar_body` rotation moves
  the guitar horizontal, but remains edge-on/through the torso. A six-shot
  90/270 post-roll sweep on X/Y/Z also rejects.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r78_sourceguitarrot_sweep_contact_sheet_20260820.jpg`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r78_sourceguitarrot_sweep_visual_decision_20260820.json`.
- Decision: next branch must derive a complete guitar world frame/offset from
  contact axes and body facing, then solve arms/hands to it. Raw source
  rotation roll is not enough.

### 2026-08-20 Midori r77 relation-fit probe rejection

- Active goal remains open. Commands ran at Idle/low priority. Runtime capture
  used local `gh2_ps2_hybrid_assets/GEN` plus ordinary loose DLC only; no ISO
  was mounted or used, and loose DLC hashes were restored to the r74 values.
- Fixed the relation-fit feasibility diagnostic so every requested roll is
  evaluated and added `spine3` / `visible-hand-center` anchor modes. The r77
  upper-anchor report found 18 structural passes; the best row was `spine3`,
  scale `0.18`, roll `0`.
- Current ACP-stage promotion and a same-knob Control_Root rebuild both failed
  structural replay, so the retained bipedal Control_Root MILOs remain the only
  known-good body basis for this branch.
- Added a `relation-fit-world` sparse one-case bake mode and captured it. It
  replayed with `suspicious=none`, but direct visual inspection rejects it: the
  body is sparse/default-like and the guitar is a vertical strip through the
  performer, not a coherent playable attachment.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r77_relation_fit_world_onecase_visual_decision_20260820.json`.

### 2026-08-20 Midori r76 Control_Root pair-fit one-case rejection

- Active goal remains open. Commands were run at Idle/low priority. Runtime
  capture used local `gh2_ps2_hybrid_assets/GEN` plus ordinary loose DLC only;
  no ISO was mounted or used. Loose DLC was restored to the r74 hashes after
  the temporary probe.
- Diagnosed the r75 Control_Root+r74-proxy branch with
  `tools/gh3_midori_guitar_ik_contract_report.py`. The active guitar/hand
  contract is not a whole-guitar-offset problem: for
  `midori_1_hand_overlay_f010`, direct sampling reports
  `LH-FRET=25.552`, `RH-STRUM=25.889`, and the guitar anchor solved from fret
  vs strum targets differs by `147.164` units. Forcing stock hand-target locals
  still leaves `solve_delta=136.547`; HMX quat materialization is worse
  (`152.704`). That means the two visible hands and the GH2 proxy pair do not
  define one consistent guitar frame in this branch.
- Built a one-case diagnostic from `gh3_midori_guitar_frame_hand_bake_probe.py`
  using `hand-target-pair-midpoint`, `hand-target-pair-fit`,
  `current-proxies`, `canonical-hands-visible-arm-rot`, mesh channel names, and
  direct quat mode. It moved the solved guitar world to approximately
  `[7.619, -13.402, 56.205]`, but direct visual capture rejects it: body is
  upright, guitar remains behind/over the left shoulder, and arms do not grip.
- Decision: do not continue plain current-proxy two-point pair-fit. The next
  branch needs a source-authored guitar orientation/contact frame or relation
  fit that constrains prop front/neck direction, not just target midpoint.
  Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r76_pairfit_onecase_visual_decision_20260820.json`.

### 2026-08-20 Midori r75 post-r74 visual/Control_Root probe

- Active goal remains open. Commands were run at Idle/low priority. Runtime
  captures used local `gh2_ps2_hybrid_assets/GEN` plus ordinary loose DLC only;
  no ISO was mounted or used.
- Captured the currently deployed r74 loose DLC through
  `tools/gh3_midori_pose_review.py`. The automated manifest is green
  (`9` proofs, `0` failures), but direct visual inspection rejects it:
  most full-body captures are sideways/non-bipedal, and the hand-overlay case
  is upright but still visibly wrong at arm/guitar alignment. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r75_post_r74_pose_review_20260820/contact_sheet.jpg`.
- Diagnosed the current deployed model/clipset with
  `tools/gh3_midori_actual_milo_parent_replay_report.py`: current r74 loose
  DLC still has `bone_pelvis.mesh` parented to package root in both character
  and clipset (`pelvis_parented=False`), while the source pelvis is authored
  under `Control_Root`. Multiclip root/pelvis diagnostics confirm the active
  guitar clips exercise large pelvis/lower-body motion on that mismatched
  target graph.
- Built a reversible probe from retained
  `controlroot_stockupper_full_candidate_20260819`: copied its animation set,
  patched only the two model MILOs with the r74 guitarist proxy fix, deployed
  temporarily, captured the same 9-case pose review, then restored the r74
  loose DLC hashes. This probe has the expected actual-MILO replay result
  (`pelvis_parented=True`, `pelvis_static=True`, `spine_dynamic=True`) and
  restores upright/bipedal body captures, but direct visual inspection still
  rejects final approval because guitar/hand placement remains visibly offset
  or behind the performer. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r75_controlroot_r74proxy_pose_review_20260820/contact_sheet.jpg`.
- Decision: the Control_Root-parented branch remains the correct body-frame
  base; r74's proxy fix helps the model graph but does not solve the final
  guitar/hand attachment. Do not continue from the current flattened-parent
  deployed visual result as if it were approval. Continue from the bipedal
  Control_Root branch and solve the guitar/hand attachment alignment.

### 2026-08-20 Midori r74 strum-hand proxy model fix

- Active goal remains open. Commands were run at Idle/low priority. No ISO was
  mounted or used. This pass rebuilt `milo_convert_tool`, patched model MILOs
  from the loose DLC tree in scratch, then deployed only the two model MILOs
  after graph verification.
- Ran `tools/gh3_midori_guitar_anchor_graph_report.py` against the currently
  deployed loose `gh3_midori_1.milo_ps2`. Pre-fix result isolated a real model
  graph bug: `bone_pelvis.mesh`, `bone_pos_guitar.mesh`, `bone_fret.mesh`,
  `bone_strum.mesh`, and `bone_fret_hand.mesh` matched stock GH2, but
  `bone_strum_hand.mesh` differed by `14.965029589335648` local units.
  Midori local was `[-1.45011, -0.169293, 10.8653]`; stock GH2 local is
  `[-6.73834944, -1.31678379, -3.08711553]`.
- Found the source of the stale value in
  `GuitarHeroOGX-main-ui-engine/tools/milo_convert/milo_convert_tool.cpp`:
  `patch_guitarist_proxy_transforms()` and
  `generated_guitar_controller_local()` both hardcoded the bad strum-hand
  local. Patched both to `[-6.73835, -1.31678, -3.08712]` and rebuilt
  `milo_convert_tool`.
- Applied the patched converter's `patch-guitarist-proxies` command to the
  existing loose model MILOs in scratch, then deployed the patched outputs to:
  `gh2_ps2_hybrid_assets/DLC/community.gh3.midori/content/char/gh3_midori_1/og/gen/gh3_midori_1.milo_ps2`
  and
  `gh2_ps2_hybrid_assets/DLC/community.gh3.midori/content/char/gh3_midori_2/og/gen/gh3_midori_2.milo_ps2`.
- Deployed model hashes now:
  `gh3_midori_1.milo_ps2 = B7014B8CD499EC27434D0E428716FA00DDED311E762034B08C3C1AA58EA6E759`;
  `gh3_midori_2.milo_ps2 = D7A45AB0D004ABDE1C71E414766F79BF7DE8C568F18ADC18CA5FA3A59E0230FE`.
  Animation MILO hashes remained unchanged from r73.
- Post-deploy graph reports for both outfits pass: every measured anchor
  (`bone_pelvis.mesh`, `bone_pos_guitar.mesh`, `bone_fret.mesh`,
  `bone_strum.mesh`, `bone_fret_hand.mesh`, `bone_strum_hand.mesh`) reports
  `local=0.000 world=0.000` and diagnosis
  `Midori guitar-hand parent graph matches stock GH2 within the diagnostic threshold.`
- Added focused regression
  `test_milo_convert_uses_stock_strum_hand_proxy_local` so the stale converter
  constant cannot return silently. Also fixed the anchor graph diagnostic text
  so a passing graph no longer reports a stale strum-side mismatch sentence.
- Decision: r74 is a real deployed model fix, not visual approval. Next branch
  can resume candidate/runtime review with the known guitar controller parent
  graph fixed in the loose DLC models.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r74_current_loose_guitar_anchor_graph_report_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r74_deployed_guitar_anchor_graph_report_20260820.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r74_deployed_guitar_anchor_graph_report_midori2_20260820.json`.

### 2026-08-20 Midori r73 hand-root correspondence audit

- Active goal remains open. Commands were run at Idle/low priority. No ISO was
  mounted or used; this pass consumed retained r72 JSON evidence plus existing
  candidate MILOs through local `milo_convert_tool.exe` only.
- Fixed drift in `tools/gh3_midori_source_local_frame_bridge_report.py`: its
  internal bake-probe `SimpleNamespace` now supplies
  `sample_quat_mode="direct"`, matching the current
  `gh3_midori_guitar_frame_hand_bake_probe.py` API. Added a focused regression
  in `tools/gh3_midori_pipeline_test.py`.
- Ran `tools/gh3_midori_source_local_frame_bridge_report.py` on r72
  `midori_1_attack_left_f030` frames `0,30` against the existing
  `controlroot_stockupper_full_candidate_20260819` candidate. Result rejects
  source-local hand pairs as a direct runtime-visible-hand fit:
  `ik_helper rms=13.793709 max=18.355553`; `palm rms=11.03664 max=14.35687`.
- Ran `tools/gh3_midori_source_guitar_contract_report.py` on the r72 pose
  bridge. Direct source IK-helper locals are tiny versus GH2 stock controller
  locals: left source mean `[0.077938, 0.257191, 0.019462]` vs stock
  `[-6.27464, -0.453556, -4.32057]` (`delta_len=7.726337`); right source mean
  `[-0.087862, 0.000002, 0.019791]` vs stock
  `[-6.73835, -1.31678, -3.08712]` (`delta_len=7.4576`).
- Added `tools/gh3_midori_hand_root_mode_report.py` to compare writer
  `--hand-root-position-source` modes against the GH2 hand-reference rows for
  the current pose-review case. For `midori_1_attack_left_f030`, direct
  source-helper modes are worse than the existing reference/prop-local modes:
  left distances from reference: `prop-local=5.666294`,
  `source-ik-helper=5.760625`, `source-ik-helper-gh2scale=9.693967`; right
  distances: `prop-local=2.884601`, `source-ik-helper=4.74687`,
  `source-ik-helper-gh2scale=8.280853`.
- Decision: do not promote a candidate using source IK helper hand-root
  emission. For the next candidate branch, keep GH2 hand-reference/proxy locals
  for `bone_fret_hand`/`bone_strum_hand` and use GLB source helpers only to
  reason about guitar frame/placement unless a richer rebase is added. The
  next useful implementation branch is a guitar placement/rotation rebase that
  preserves stock/reference hand-root locals, not direct source-helper hand
  proxy locals.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r73_source_local_frame_bridge_attack_report_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r73_source_guitar_contract_report_20260820.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r73_hand_root_mode_report_20260820.json`.

### 2026-08-20 Midori r72 guitar-neck anchor/frame-fit diagnostic

- Active goal remains open. Commands were run at Idle/low priority. The GH3 ISO
  was used only as an extraction source for the focused Blender/NXTools bridge;
  no game/runtime capture used an ISO, and extracted source inputs were not
  retained.
- Added `bone_guitar_neck`, `BONE_GUITAR_FRET_POS`, and
  `BONE_GUITAR_STRUM_POS` to
  `tools/gh3_midori_review_source_bridge_batch.py` default exported bones.
  The focused `midori_1_attack_left_f030` bridge now has `58` pose records
  across frames `0,30`.
- Extended `tools/gh3_midori_guitar_helper_contract_report.py` with anchor
  parent/source-only rows. r72 proves `bone_guitar_body` is coincident with
  `BONE_GUITAR_STRUM_POS`, while `bone_guitar_neck` is coincident with
  `BONE_GUITAR_FRET_POS`; the useful source guitar frame is therefore
  `bone_guitar_body -> bone_guitar_neck`, not the previous assumed
  body-to-helper-parent frame.
- Added `tools/gh3_midori_guitar_frame_fit_report.py` to fit source GLB
  guitar-local points onto GH2 target helper bind points. Best r72 review-frame
  fit uses `source_hint=fret_hand`, `target_hint=strum_hand`,
  `target_normal_sign=-1`, `scale=91.183463`, `rms_error=4.884814`,
  `max_error=6.431115`. Point errors at frame 30: neck/fret `3.146083`,
  fret hand `6.431115`, strum hand `4.508564` GH2 units.
- Decision: GLB/NXTools is now part of the automated diagnostic bridge and has
  exposed the source guitar endpoints, but the helper fit is not good enough to
  promote to a visual candidate. Next branch should either solve a
  non-uniform/role-specific guitar helper mapping or audit the target
  `bone_fret`/`bone_strum`/hand-helper correspondence before writing another
  MILO candidate.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r72_guitar_neck_anchor_contract_report_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r72_guitar_frame_fit_report_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r72_guitar_neck_anchor_bridge_20260820/review_source_bridge_batch_manifest.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r72_guitar_neck_anchor_bridge_20260820/midori_1_attack_left_f030/gh3_guit_mido_a_attackl.ska.pose_bridge.json`.

### 2026-08-20 Midori r70 guitar-helper bridge/contract diagnostic

- Active goal remains open. Commands were run at Idle/low priority. The GH3 ISO
  was used only as an extraction source for Blender/NXTools bridge export, not
  for game/runtime capture; extracted inputs were not retained.
- Added `bone_guitar_body`, `bone_ik_hand_guitar_l`, and
  `bone_ik_hand_guitar_r` to
  `tools/gh3_midori_review_source_bridge_batch.py` default exported bones.
  The parsed source skeleton already contains all three helper bones; earlier
  five-case bridge manifests omitted them because the export whitelist did not
  request them.
- Regenerated a focused `midori_1_attack_left_f030` source bridge at frames
  `0,30` using pinned NXTools at
  `C:\Users\smmel\AppData\Local\Temp\nxtools_ref`. The pose bridge contains
  `52` records and includes `Control_Root`, `Bone_Head`, `bone_guitar_body`,
  `bone_ik_hand_guitar_l`, and `bone_ik_hand_guitar_r`. Generated GLB/logs were
  deleted as rebuildable; the small pose JSON and manifest were retained.
- Added `tools/gh3_midori_guitar_helper_contract_report.py`, which maps source
  helper positions into the GH2 target frame and compares source helper vectors
  against GH2 target bind vectors for `bone_pos_guitar.mesh`,
  `bone_fret_hand.mesh`, and `bone_strum_hand.mesh`.
- r70 contract result for attack frame 30:
  `status=guitar_helper_contract_measured`, worst segment
  `fret_to_strum_hand`, `direction_dot=-0.866264`,
  `angle=150.027336`, `length_ratio=0.002058`. Other helper vectors are also
  badly scaled/opposed (`guitar_to_fret_hand dot=-0.846395`,
  `pelvis_to_guitar dot=-0.778258`). This proves the exported source helper
  records are not directly usable as GH2 guitar/hand target offsets.
- Focused tests passed:
  `python -m unittest tools.gh3_midori_pipeline_test.MidoriPipelineTest.test_review_source_bridge_defaults_include_guitar_helpers tools.gh3_midori_pipeline_test.MidoriPipelineTest.test_target_bind_pose_preserves_structural_head_source`.
- Decision: the source guitar helpers now exist in the automated bridge, but
  the contract is a guitar-local basis/scale problem. Next branch should include
  or reconstruct the source helper parent frame (`BONE_GUITAR_FRET_POS` /
  `BONE_GUITAR_STRUM_POS`) or derive the prop-anchor basis before solving
  GH2 `bone_pos_guitar`/fret/strum target-space. Do not capture another visual
  candidate until this helper frame is explained.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r70_guitar_helper_contract_report_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r70_guitar_helper_bridge_20260820/review_source_bridge_batch_manifest.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r70_guitar_helper_bridge_20260820/midori_1_attack_left_f030/gh3_guit_mido_a_attackl.ska.pose_bridge.json`.

### 2026-08-20 Midori r69 structural head bind/target-graph diagnostic

- Active goal remains open. Commands were run at Idle/low priority. No ISO was
  mounted or used.
- Found a real target-bind blind spot: `target_bind_pose_by_source_name()` did
  not expose structural `Bone_Head` because multiple GH3 face/detail bones map
  to GH2 `bone_head`, leaving the generated `bone_head(.mesh)` transform tagged
  as a facial alias (`Bone_Lip_Upper_Mid`) instead of `Bone_Head`.
- Added a narrow fallback in `tools/gh3_midori_acp_stage.py` so `Bone_Head`
  resolves from `bone_head.mesh`/`bone_head` under `Bone_Neck`. Added a focused
  regression in `tools/gh3_midori_pipeline_test.py`.
- Extended `tools/gh3_midori_target_graph_solve_validator.py` with
  `--graph-scope target_graph_head`, adding the `Bone_Neck -> Bone_Head`
  segment to the existing pelvis/torso/leg target graph.
- Ran the five-case GLB/source bridge validator:
  `status=target_graph_solve_valid`, `passing=40`; best convention:
  `compose=local_desired_t`, `pelvis=bind`, `aimfix=transpose_post_t`,
  `max_position_error=0.0`, `min_segment_dot=1.0`,
  `min_selected_child_aim_dot=1.0`, rotation roundtrip max `0.021455`.
- Focused tests passed:
  `python -m unittest tools.gh3_midori_pipeline_test.MidoriPipelineTest.test_target_bind_pose_preserves_structural_head_source tools.gh3_midori_pipeline_test.MidoriPipelineTest.test_torso_bridge_leg_ik_policy_is_registered`.
- Decision: the head/neck target graph can now be validated structurally, but
  r69 is diagnostic only and not a visual candidate. The five-case source
  bridge contains `Control_Root`/`Bone_Head` but not `bone_guitar_body` or
  hand-guitar helpers, so the next branch should export/consume those source
  helper records and extend the contract diagnostic to
  `bone_guitar_body -> bone_pos_guitar.mesh` plus fret/strum target rows before
  another visual capture.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r69_head_target_graph_contract_report_20260820.json`.

### 2026-08-20 Midori r68 runtime pose/mesh diagnostic boundary

- Active goal remains open. Heavy commands were run at Idle/low priority. No
  ISO was mounted or used; the focused capture used local
  `gh2_ps2_hybrid_assets/gen` plus `GHOGX_ADDONS_DIR=gh2_ps2_hybrid_assets/DLC`.
- Added a diagnostic-only native viewer flag,
  `--char-pose-mesh-dump <jsonl>`, in
  `GuitarHeroOGX-main-ui-engine/engine/src/app/app_main.cpp`. At the screenshot
  frame it dumps the actually drawn skinned mesh rows with posed bounds,
  material, palette, dominant bones, and face-height-band bone attribution.
  `tools/gh3_midori_pose_review.py` now passes this through with
  `--pose-mesh-dump-dir`.
- Built `ghogx_app` successfully through the VS developer environment. Then ran
  one focused loose-DLC diagnostic capture for `midori_1_hand_overlay_f010`.
  The pose-review manifest still fails the expected visual-difference gate
  (`failure=visible_difference_from_idle`, min margin `38`), so r68 is not a
  candidate promotion.
- Runtime attribution from
  `r68_runtime_pose_mesh_focus_report_20260820.json` shows the face-height band
  is dominated by `bone_head.mesh` (`1372.844` aggregate weight), then
  `bone_spine3.mesh` (`400.8`), `bone_neck.mesh` (`83.1`),
  `bone_L-clavicle.mesh` (`63.1`), and `bone_L-upperArm.mesh` (`51.9`).
  `bone_L-foreArm.mesh` contributes only `2.2` in the face band. The lone
  positive-y face-band cluster is `midori_1_mesh0_part13.mesh`, mostly
  `bone_L-upperArm.mesh`; the large `midori_1_mesh0_part16.mesh` bound is not a
  forearm/palm face-band driver because its face-band vertices are head/detail
  weighted.
- Decision: r67's "left sleeve/palm across the face" interpretation is not
  supported by runtime skinned vertices. Resume by diagnosing the shared
  Control_Root/pelvis/neck/head/guitar target-space contract, preferably via
  GLB/source-authoritative pose comparison or a target-graph local transform
  validator. Do not spend the next branch on palm-only/simple arm-aim edits.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r68_runtime_pose_mesh_focus_report_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r68_pose_mesh_dump_jsonl_20260820/midori_1_hand_overlay_f010.jsonl`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r68_pose_mesh_dump_visual_20260820/midori_1_hand_overlay_f010.bmp`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r68_pose_mesh_dump_review_20260820.json`.

### 2026-08-20 Midori r67 arm-aim and mesh-influence boundary

- Active goal remains open. Heavy commands were run at Idle/low priority. No
  ISO was mounted or used. Capture used local `gh2_ps2_hybrid_assets/GEN` with
  loose DLC backup/restore only.
- Added optional arm aim rotation controls to
  `tools/gh3_midori_patch_relation_fit_worlds_into_stage.py`:
  `--aim-arm-rotations` and `--aim-arm-rotation-side`. The aim solve computes
  upper/forearm world rotations from solved segment vectors and then emits ACP
  `.quat` samples through the existing patch path.
- r67 reused r66's reference-biased left hand position and two-bone forearm
  solve, then aimed the visible left upperArm/foreArm rotations. Offline replay
  passed the segment-aware gate and confirmed left upperArm/foreArm rotations
  changed to roughly `39.013154` / `44.727859` degrees while preserving segment
  lengths:
  left upper-forearm `9.301086`, forearm-hand `8.975371`,
  upper-hand `10.000006`.
- Direct visual still rejects r67. Runtime signatures changed
  (`main sig=320.037`, fret sig=`96.185`), so the clips loaded, but r66/r67 BMPs
  differ by only `247` bytes. The rendered face-crossing/occlusion silhouette
  remains effectively unchanged.
- Added `r67_mesh_influence_focus_report_20260820.json`. It shows many
  palm/forearm-weighted parts are low in bind space, while face-height
  purple/occluding regions include clavicle/upper-arm and GH3/head-weighted
  parts. This means offline palm/forearm replay and even simple arm-aim quats
  are no longer reliable visual proxies for the occluder.
- Decision: do not continue palm-only or simple arm-aim candidate captures.
  Next branch should add a runtime-space pose/mesh debug dump, or target the
  exact face-height occluder mesh/bone influence set before another capture.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r67_reference_left_aim_small_right_ik_layer_replay_f010_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r67_reference_left_aim_small_right_ik_visual_20260820/midori_1_hand_overlay_f010.bmp`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r67_mesh_influence_focus_report_20260820.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r67_reference_left_aim_small_right_ik_visual_decision_20260820.json`.

### 2026-08-20 Midori r65-r66 constrained/reference hand diagnostics

- Active goal remains open. Heavy commands were run at Idle/low priority. No
  ISO was mounted or used. Captures used local `gh2_ps2_hybrid_assets/GEN` with
  loose DLC backup/restore only.
- Added optional hand-position controls to
  `tools/gh3_midori_patch_relation_fit_worlds_into_stage.py`:
  `--min-upper-hand-distance`/`--constrain-upper-hand-side` pushes an accepted
  palm away from the upper arm, and
  `--reference-hand-distance`/`--reference-hand-direction-side` places a palm
  along the retained reference upper-arm-to-hand direction.
- r65 used the smaller accepted cluster with left `upper_to_hand >= 7` and
  right-arm IK. Replay passed the segment-aware gate
  (`left_upper_to_hand=6.999989`), but direct visual rejects it: the left
  sleeve/hand still crosses the face. Pushing along the accepted-fit direction
  preserves the bad rendered direction.
- r66 placed the left palm along the retained reference fret-arm direction at
  distance `10.0` and solved both forearms. Replay passed:
  left hand `(-1.232919, -5.329112, 46.440652)`,
  left `upper_to_hand=10.000002`, left `forearm_to_hand=8.975374`.
  Runtime fret-layer signature changed versus r64, so the candidate clip was
  loaded. However, r64 and r66 BMPs differ by only `389` bytes out of
  `2,764,854`, and direct visual inspection still rejects r66.
- Decision: do not keep grinding palm position only. The next branch should
  patch visible left upperArm/foreArm aim rotations from the solved segment
  vectors, or add a runtime-space pose dump to identify which visible mesh/bone
  drives the face-crossing sleeve. Offline replay palm positions alone are no
  longer strong evidence of rendered improvement.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r65_relation_fit_small_left_constrained_right_ik_visual_decision_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r66_reference_left_small_right_ik_layer_replay_f010_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r66_reference_left_small_right_ik_visual_20260820/midori_1_hand_overlay_f010.bmp`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r66_reference_left_small_right_ik_visual_decision_20260820.json`.

### 2026-08-20 Midori r62-r64 arm IK and segment-aware rejection

- Active goal remains open. Heavy commands were run at Idle/low priority. No
  ISO was mounted or used. Runtime captures used `gh2_ps2_hybrid_assets/GEN`
  with loose DLC backup/restore only.
- Tightened `tools/gh3_midori_overlay_visual_sanity_gate.py` so actual MILO
  layer replays can reject arm segment failures before capture:
  `forearm_to_hand > 14.0` catches r61-style stretched hand geometry, and
  `upper_to_hand < 6.0` catches r62/r64-style face-folded arms. Added focused
  regression coverage in `tools/gh3_midori_pipeline_test.py`.
- Extended `tools/gh3_midori_patch_relation_fit_worlds_into_stage.py` with
  `--solve-arm-forearms` and `--solve-arm-forearm-side` for a two-bone forearm
  point solve that preserves reference upper-arm and forearm lengths while
  retaining accepted hand positions.
- r62 solved both forearms. Replay passed the old overlay metrics and restored
  right forearm-to-hand distance to `8.975395`, but direct visual inspection
  rejects it because the left arm folds across the face. The new gate now
  rejects it pre-capture:
  `left_upper_to_hand distance 4.341531 is below 6.000000`.
- r63 solved only the right forearm while retaining r61's left arm. Replay
  passed and fixed the right stretch, but direct visual inspection still rejects
  it: the pose remains folded/occluded and the guitar/hand cluster is not
  stock-quality.
- r64 used the smaller accepted relation-fit cluster (`prop_scale_ratio=0.10`)
  with right-forearm IK. Replay passed old overlay metrics, but direct visual
  still rejects it. The new gate rejects it pre-capture:
  `left_upper_to_hand distance 4.511182 is below 6.000000`.
- Decision: the accepted left hand positions from both 0.15 and 0.10 clusters
  are too close to the upper arm for a coherent guitar pose. Next branch should
  keep the r61 body/arm rotations and segment-aware gate, but stop preserving
  the fitted left palm blindly. Try a guitar/hand cluster solve that constrains
  both forearm-hand and upper-hand distances before capture, or bias the left
  visible hand toward the retained/reference fret-side arm while separately
  fitting the guitar/right hand.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r62_relation_fit_body_arm_ik_segment_gate_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r62_r63_arm_ik_visual_decision_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r64_relation_fit_small_right_ik_layer_replay_f010_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r64_relation_fit_small_right_ik_segment_gate_20260820.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r64_relation_fit_small_right_ik_visual_20260820/midori_1_hand_overlay_f010.bmp`.

### 2026-08-20 Midori r59-r61 body/arm rotation patch and visual rejection

- Active goal remains open. Heavy commands were run at Idle/low priority. No
  ISO was mounted or used. Runtime captures used local `gh2_ps2_hybrid_assets/GEN`
  plus loose DLC backup/restore only.
- Extended `tools/gh3_midori_patch_relation_fit_worlds_into_stage.py` so the
  accepted relation-fit world-position patch can also solve local ACP `.quat`
  samples from a retained replay frame.
- r59 patched r41b body rotations plus guitar rotation on top of r58's
  body-position/accepted hand-target cluster. MILO replay passed the overlay
  gate with `head_spine3=11.674565` and `guitar_head=11.955781`; body rotations
  matched r41b-like angles. Direct capture rejects it: upright/bipedal body
  returned, but forearm/hand/guitar geometry was still visibly broken.
- r60 added arm-frame rotations/forearm positions. It fixed the left arm, but
  failed before capture because right hand position was solved before the right
  forearm rotation was applied in the strum layer. Gate rejected it with
  `hand_ratio=1.204236`.
- r61 fixed the strum-layer solve ordering. MILO replay passed with no
  suspicious nodes and preserved the accepted relation-fit positions:
  left hand `(-3.865619, 2.787378, 53.463442)`,
  right hand `(-8.603565, 5.437462, 57.980519)`,
  fret target `(-2.967461, 2.137803, 49.686966)`,
  strum target `(-4.224256, 2.304937, 50.372323)`.
- One no-ISO local-GEN visual capture was run for r61. Direct visual inspection
  still rejects it: body/left arm are improved, but the guitar occludes too
  much upper body and the right hand/finger geometry remains spiky/incoherent.
- Decision: do not return to pelvis-only/body-position diagnosis. The next
  branch should keep the body/arm-frame solve and diagnose guitar local
  rotation/scale plus hand-detail/finger rotations before further captures.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r59_relation_fit_body_rot_visual_decision_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r60_relation_fit_body_arm_rot_visual_sanity_gate_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r61_relation_fit_body_arm_rot_ordered_layer_replay_f010_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r61_relation_fit_body_arm_rot_ordered_visual_20260820/midori_1_hand_overlay_f010.bmp`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r61_relation_fit_body_arm_rot_ordered_visual_decision_20260820.json`.

### 2026-08-20 Midori r57/r58 relation-fit ACP replay and visual rejection

- Active goal remains open. Heavy commands were run at Idle/low priority. No
  ISO was mounted or used. Temp candidates/stages were cleaned. Loose DLC was
  restored and hashes match baseline.
- Added `tools/gh3_midori_patch_relation_fit_worlds_into_stage.py` to turn the
  accepted r56 relation-fit worlds into editable ACP `.pos` rows.
- r57 patched only the accepted r56 guitar/hand/target cluster into the current
  editable matrix-local stage. It applied 5 channels with no missing rows and
  rebuilt a temp candidate, but rejected before capture:
  `head_spine3=6.512686`, `guitar_head=17.339225`. The accepted cluster worlds
  survived local solving, but they were inserted into the current collapsed
  body frame, so torso ratios were far above the head.
- r58 also patched r41b body world-position rows
  (`pelvis/spine1/spine2/spine3/neck/head`) plus the accepted r56 cluster. It
  applied 11 channels with no missing rows and passed the stricter post-MILO
  overlay gate:
  `head_spine3=11.674564`, `guitar_head=11.955744`,
  `LH_to_fret=3.935799`, `RH_to_strum=9.320675`,
  `failure_count=0`.
- A single no-ISO local-GEN visual capture was run for
  `midori_1_hand_overlay_f010`. Automated proof wrote the BMP but failed
  `visible_difference_from_idle`; direct inspection rejects the image. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r58_relation_fit_body_visual_20260820/midori_1_hand_overlay_f010.bmp`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r58_relation_fit_body_visual_decision_20260820.json`.
- Visual decision: r58 is not bipedal/coherent. The body is folded/side-on,
  the guitar occludes the upper body, and the leg/body silhouette has severe
  elongated artifacts. Position-only body-frame patching is insufficient.
- Decision: do not continue from r58 and do not trust the overlay gate alone
  for body coherence. Next branch should either solve the corresponding r41b
  body rotations/orientation with the accepted relation-fit cluster, or add a
  body-orientation/silhouette gate before any further capture.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r57_relation_fit_world_patch_report_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r57_relation_fit_layer_replay_f010_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r57_relation_fit_visual_sanity_gate_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r58_relation_fit_body_world_patch_report_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r58_relation_fit_body_layer_replay_f010_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r58_relation_fit_body_visual_sanity_gate_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r58_relation_fit_body_pose_review_20260820.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r58_relation_fit_body_source_final_relation_report_20260820.json`.

### 2026-08-20 Midori r56 same-frame relation fit feasibility

- Active goal remains open. Heavy commands were run at Idle/low priority. No
  ISO was mounted or used. No candidate was deployed over loose DLC.
- Added `tools/gh3_midori_relation_fit_feasibility.py`. It takes the r47 exact
  source bridge, fits source palms/IK helpers/`bone_guitar_body` as one
  same-frame cluster into a retained final body frame, and evaluates the
  stricter overlay gate without building MILOs.
- Pelvis-anchored source preservation is infeasible. The source bridge
  pelvis-to-head scale is only `0.520488` versus r41b final
  pelvis-to-head `21.591641`, so body-scale fitting explodes the prop/hand
  cluster. Even reduced pelvis-anchored fits fail `guitar_to_head <= 14`.
- Base-guitar-anchored cluster fitting on r41b is feasible. Accepted rows:
  - `prop_scale_ratio=0.15`, `roll_degrees=330`,
    `guitar_head=11.955784`,
    `left_visible_to_fret_target=3.935828`,
    `right_visible_to_strum_target=9.320717`.
  - `prop_scale_ratio=0.10`, `roll_degrees=330`,
    `guitar_head=11.955784`,
    `left_visible_to_fret_target=2.623885`,
    `right_visible_to_strum_target=6.213812`.
- The accepted `0.15` frame positions are:
  `guitar=(-7.601438, 3.992937, 51.964199)`,
  `left_palm=(-3.865620, 2.787372, 53.463444)`,
  `right_palm=(-8.603568, 5.437445, 57.980525)`,
  `left_target=(-2.967433, 2.137773, 49.686935)`,
  `right_target=(-4.224255, 2.304934, 50.372321)`.
- r36c base-guitar fitting remains rejected because its retained guitar anchor
  is already too far from the head (`best guitar_head=15.575487`).
- Tooling check: `milo_convert_tool` exposes build/inspect/sample commands but
  no ACP extraction command, so retained candidate MILOs cannot be converted
  back into editable stages directly.
- Decision: next branch should synthesize an editable one-frame ACP/probe
  manifest from the accepted r41b-anchored cluster positions and feed it through
  the existing ACP writer, rather than trying to decompile retained MILOs or
  preserve pelvis-anchored source scale.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r56_r41b_relation_fit_anchor_feasibility_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r56_r36c_relation_fit_anchor_feasibility_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r56_r41b_relation_fit_feasibility_20260820.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r56_r41b_relation_fit_lowscale_feasibility_20260820.json`.

### 2026-08-20 Midori r55 retained-candidate audit and stricter no-capture gate

- Active goal remains open. Heavy commands were run at Idle/low priority. No
  ISO was mounted or used. No candidate was deployed over loose DLC.
- Searched retained r47/r48/exact-overlay evidence. The useful r48/r41b ACP
  stages are not retained; only compact reports, source bridge JSON/GLB files,
  visuals, and a few older candidate MILO directories remain.
- Replayed retained candidate directories directly. All retained candidates
  have good torso scale but are worse than r41b on guitar/hand contract:
  `controlroot_stockupper_full_candidate_20260819` replays with
  `head=11.592237`, `guitar=23.142596`, `LH=47.388798`,
  `RH=34.429409`; `controlroot_nostock_full_candidate_20260819` is
  `head=11.562576`, `guitar=23.235512`, `LH=38.465553`,
  `RH=21.875854`; the exactbase/repro pairfit candidates have
  `guitar=26.510836` and `LH=52.016120`.
- Visually inspected retained r34/r36c/r40/r41b overlays. r34/r36c are
  bipedal but put the guitar behind/above the head. r40/r41b bring the guitar
  forward but still have the waist/forearm cluster collapse. None are capture
  candidates.
- Tightened `tools/gh3_midori_overlay_visual_sanity_gate.py` so excessive
  visible-hand-to-target distance and excessive `guitar_to_head` distance are
  hard failures, not warnings or untracked defects. Defaults:
  `max_target_hand_distance=12.0`, `max_guitar_head_distance=14.0`.
- Added a regression test in `tools/gh3_midori_pipeline_test.py` proving the
  overlay gate rejects large hand-target and guitar-head offsets.
- Representative gate checks now reject r34, r36c, r41b, r48, r54, and the
  retained `controlroot_stockupper_full_candidate_20260819` before capture.
  This is intentional: these are visually/structurally unacceptable even when
  bipedal.
- Verification:
  `python -m py_compile tools/gh3_midori_overlay_visual_sanity_gate.py tools/gh3_midori_source_final_relation_report.py tools/gh3_midori_pipeline_test.py`
  and
  `python -m unittest tools.gh3_midori_pipeline_test.MidoriPipelineTest.test_overlay_gate_rejects_large_hand_or_guitar_offsets`
  pass.
- Decision: do not continue from retained candidate directories. Next work must
  generate a new same-frame relation solve from source bridge data, with this
  stricter gate preventing another non-coherent capture.

### 2026-08-20 Midori r54 source/final relation and source-IK bake rejection

- Active goal remains open. Heavy commands were run at Idle/low priority. No
  ISO was mounted or used. All r54 candidate build outputs were temporary
  scratch and were not deployed over loose DLC.
- Refreshed the local ihatecompvir source audit:
  `r54_ihatecompvir_bridge_audit_20260820.json` reports
  `status=reference_ready_direct_writer_needed`, with public `glTFMilo` still
  not a GH2 PS2 `CharClipSamples` converter and MiloLib/GH2 notes still
  reference-ready.
- Added `tools/gh3_midori_source_final_relation_report.py`. It compares the
  r47 exact source bridge against final MILO layer replay in
  pelvis-to-head-normalized units. Results:
  - r48 exact overlay: source/final relation mismatch (`mismatches=3`,
    `drift=2`).
  - r52 visible-hand patch: source/final relation mismatch (`mismatches=6`).
    This proves the direct hand-target patch masks hand distance but destroys
    the source IK-helper-to-palm offset instead of preserving the authored
    guitar/hand relation.
  - r53b: source/final relation mismatch (`mismatches=5`, `drift=2`).
- Ran a bounded r54 source-IK visible-arm-chain bake probe using the r47 exact
  main source bridge, current matrix-local candidate, `source-ik-helper-locals`
  pair fit/target mode, `visible-arm-chain-rotpos`, and emitted fret/strum
  target proxies. The stage patch applied 12 channels with no missing channels.
- r54 is rejected before capture. Actual-layer replay:
  `head_spine3=6.512686`, `guitar_head=15.852803`,
  `LH_to_fret=20.460141`, `RH_to_strum=33.400034`. The overlay gate rejects
  (`head_span=2.764657` below minimum), and the source/final relation report
  is worse (`mismatches=7`).
- Decision: do not capture r54 and do not repeat source-IK-helper locals as a
  direct frame bake. The current failure is confirmed as a final-frame relation
  preservation problem: source palms, IK helpers, and `bone_guitar_body` must be
  solved together in the same emitted character frame, not patched as separate
  local rows after the fact.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r54_ihatecompvir_bridge_audit_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r54_r48_source_final_relation_report_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r54_r52_source_final_relation_report_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r54_r53b_source_final_relation_report_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r54_sourceik_visible_arm_stage_patch_report_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r54_sourceik_visible_arm_layer_replay_f010_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r54_sourceik_visible_arm_visual_sanity_gate_20260820.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r54_sourceik_visible_arm_source_final_relation_report_20260820.json`.

### 2026-08-20 Midori r53 r41b replay-on-current-stage rejection

- Active goal remains open. Heavy commands were run at Idle/low priority. No
  ISO was mounted or used. All ACP/MILO outputs were temporary scratch under
  `%TEMP%` and were not deployed over loose DLC.
- Replayed the retained r41b front-placement patch report onto the current
  matrix-local / Control_Root-pelvis ACP stage as a bounded r53 diagnostic:
  froze `upper-head-clavicles` into a temp stage, applied
  `r41b_r40_full_contract_offset_patch_report_20260820.json`, built temporary
  main/ui/fret/strum MILOs, and replayed frame 10 on the current Midori graph.
- r53 is rejected before capture. It is not a portable reconstruction of the
  coherent r41b visual lead on the current stage:
  `head_spine3=6.515129`, `guitar_head=11.710696`,
  `LH_to_fret=31.788181`, and `RH_to_strum=23.812096`.
- r53b then applied the runtime visible-hand target patch to the same stage.
  That proved the direct visible hand local solver still works
  (`LH_to_fret=0.000011`, `RH_to_strum=0.000050`), but the candidate remains
  a structural reject because the upper body stays collapsed and the guitar
  anchor is still far above the corrected hand/target cluster:
  `r53b_visiblehands_visual_sanity_gate_20260820.json` reports
  `status=reject`, `head_span=3.547160`, and guitar/hand delta ratio
  `3.329834`.
- Decision: do not capture or retry r53/r53b. The old r41b channel report is
  not reusable on the current Control_Root/model-parent stage. Next work should
  solve visible hands and `bone_pos_guitar.mesh` together in one final
  character-space frame, or install/test a real automated GLB/MiloLib bridge
  before more manual offset grinding.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r53_r40like_freeze_report_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r53_r41like_from_r41b_report_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r53_r41like_layer_replay_f010_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r53_r41like_visual_sanity_gate_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r53b_visiblehands_to_targets_patch_report_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r53b_visiblehands_layer_replay_f010_20260820.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r53b_visiblehands_visual_sanity_gate_20260820.json`.

### 2026-08-20 Midori r48 exact overlay ACP/MILO no-capture rejection

- Active goal remains open. Heavy commands were run at Idle/low priority. No
  ISO was mounted or used. All ACP/MILO outputs were temporary scratch under
  `%TEMP%` and were not deployed over loose DLC.
- Fed the r47 exact overlay bridge manifest into `gh3_midori_acp_stage.py` for
  a tiny three-clip probe: main `gh3_guit_mido_c_med_idle`, fret
  `gh3_hnd_guit_chord_mid_bar3_d`, and strum
  `gh3_hnd_guit_strum_mido_norm_m01_d`. Built temporary clipset MILOs with
  `milo_convert_tool build-clipset-from-acp` and replayed frame 10 on the
  current Midori character graph.
- Baseline r47-manifest ACP/MILO probe is rejected before capture:
  `r48_r47_exact_overlay_visual_sanity_gate_20260820.json` reports
  `status=reject`, visible hand center ratio `0.074428`, target center ratio
  `-0.376701`, guitar ratio `0.141367`, and
  guitar-minus-hand delta `0.066939`. Replay positions confirm visible hands
  are unsampled by the new source bridge (`sample_sources={}`) and remain low,
  while the fret/strum targets are far from those hands
  (`LH->fret=25.795910`, `RH->strum=31.962692`).
- Regenerated exact source guitar contract reports from r47 fret/strum pose
  JSONs, then tested `--hand-root-position-source source-ik-helper-gh2scale`
  and `source-ik-helper`. Both are rejected before capture:
  - gh2scale: `target_ratio=-0.472886`, `hand_ratio=0.074428`,
    `guitar_ratio=0.141367`
  - unscaled: `target_ratio=-0.364381`, `hand_ratio=0.074428`,
    `guitar_ratio=0.141367`
- Decision: r47 fixed the source-bridge/GLB side, but ACP/MILO promotion still
  needs bridge-driven visible palm/arm position baking. Changing only
  fret/strum hand target locals does not move the visible hands out of the
  waist frame and must not be captured.
- Evidence retained:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r48_r47_exact_overlay_layer_replay_f010_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r48_r47_exact_overlay_visual_sanity_gate_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r48_r47_fret_source_guitar_contract_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r48_r47_strum_source_guitar_contract_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r48_r47_exact_overlay_sourceik_layer_replay_f010_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r48_r47_exact_overlay_sourceik_visual_sanity_gate_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r48_r47_exact_overlay_sourceikraw_layer_replay_f010_20260820.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r48_r47_exact_overlay_sourceikraw_visual_sanity_gate_20260820.json`.
- Next branch: add or reuse a bridge-palm-driven visible arm/hand position bake
  so `Bone_Palm_L/R` from the coherent r47 source bridge affects
  `bone_L-hand.mesh` / `bone_R-hand.mesh` final replay, then run the r46
  overlay sanity gate before any emulator capture.

### 2026-08-20 Midori r47 exact overlay source bridge pass

- Active goal remains open. Heavy commands were run at Idle/low priority. No
  ISO was mounted or used. Raw GH3 Midori files were read from the pre-existing
  `%TEMP%/gh3_midori_source_visual_20260816_211144/source` extraction, not from
  an ISO or runtime mount.
- Re-exported fresh explicit Blender/NXTools source bridges with the current
  exporter for the exact frame-10 overlay clips:
  `gh3_guit_mido_c_med_idle` -> `stand_medium_01`,
  `gh3_hnd_guit_chord_mid_bar3_d`, and
  `gh3_hnd_guit_strum_mido_norm_m01_d`. Each bridge requested the same body,
  upper-arm/forearm, visible palm, IK-helper, and `bone_guitar_body` set and
  produced 36 records for frames `0,10`.
- This fixes the r46 source-side problem for the exact overlay sample. The
  stale pinned force-partial merged bridge was mismatched-space evidence, but
  the current explicit exporter path produces coherent same-frame source data.
  Source sanity results:
  - main `gh3_guit_mido_c_med_idle` frame 10:
    `source_handguitar_bridge_sanity_pass`, center ratio `0.184920`
  - fret `gh3_hnd_guit_chord_mid_bar3_d` frame 10:
    `source_handguitar_bridge_sanity_pass`, center ratio `0.779794`
  - strum `gh3_hnd_guit_strum_mido_norm_m01_d` frame 10:
    `source_handguitar_bridge_sanity_pass`, center ratio `0.657953`
  - combined exact-overlay manifest:
    `source_handguitar_bridge_sanity_pass`, `cases=3`, `failures=0`,
    worst center ratio `0.779794`.
- Extended `tools/gh3_midori_source_handguitar_bridge_sanity_gate.py` so it can
  validate a single `--pose-json` directly and so custom manifest-relative
  paths resolve against the supplied manifest directory.
- Evidence retained:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r47_exact_c_med_idle_f10_handguitar_bridge_20260820/`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r47_exact_fret_bar3_f10_handguitar_bridge_20260820/`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r47_exact_strum_m01_f10_handguitar_bridge_20260820/`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r47_exact_overlay_handguitar_bridge_manifest_20260820.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r47_exact_overlay_handguitar_bridge_manifest_sanity_20260820.json`.
- Next branch: feed the r47 exact overlay manifest into ACP/MILO generation and
  require the r46 overlay visual sanity gate before any emulator capture. This
  is now a real GLB/pose intermediate path, not a stale mismatched-space GLB.

### 2026-08-20 Midori r46 overlay/source bridge sanity gates

- Active goal remains open. Heavy commands were run at Idle/low priority. No
  ISO was mounted or used; this checkpoint only inspected retained local
  evidence and wrote small JSON reports/tools.
- Added `tools/gh3_midori_overlay_visual_sanity_gate.py`. It consumes an
  actual-layer replay JSON and computes torso-relative hand/target/guitar
  ratios so visually impossible waist-cluster candidates are rejected before
  build/capture. Retained r41b now fails automatically:
  `status=reject`, visible hand center ratio `0.107244`, target center ratio
  `0.041469`, guitar ratio `0.589405`, delta `0.482161`. This makes the
  "internally coherent but visually waist-collapsed" failure a promotion-gate
  fact instead of relying only on screenshot memory.
- Added `tools/gh3_midori_source_handguitar_bridge_sanity_gate.py`. It audits
  source pose bridges that claim to merge body, visible palms, IK helpers, and
  guitar helper in one frame. The retained pinned force-partial merged
  hand/guitar 5-case bridge is rejected: worst hand/guitar center is
  `268.578881x` the pelvis/head body span, and medium idle is `249.727645x`.
  Those GLBs/pose bridges are therefore mismatched-space evidence, not a valid
  same-frame route into ACP/MILO.
- Evidence retained:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r46_overlay_visual_sanity_gate_r41b_20260820.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r46_source_handguitar_bridge_sanity_gate_20260820.json`.
- Next branch: a usable GLB/intermediate bridge still needs body + visible
  palms/hands + IK helpers + `bone_guitar_body` in one coherent body-scale
  frame. The retained pinned merged bridge cannot be promoted. A future
  candidate should pass both r46 gates before emulator capture.

### 2026-08-20 Midori r45 shared prop-frame pair-fit rejection

- Active goal remains open. The earlier instruction to block after step 4 is
  stale; do not block unless the user explicitly instructs that again. Heavy
  commands must stay Idle/low priority, and runtime/capture proof must use
  `gh2_ps2_hybrid_assets/GEN` plus loose DLC, not a mounted ISO.
- Rebuilt r41b again from local evidence only: r40 clav/bind freeze into
  `%TEMP%/midori_r45_r40_stage_20260820_a`, r41b patch replay into
  `%TEMP%/midori_r45_r41b_stage_20260820_a`, then temporary MILOs in
  `%TEMP%/midori_r45_r41b_milos_20260820_a`.
- Rejected the shared prop-frame pair-fit probe before build/capture. The
  stock prop-string target pair is too short for the visible-hand pair:
  `current_pair_length=9.611476` versus
  `desired_pair_length=18.423591`. The midpoint solve moves
  `bone_pos_guitar.mesh` from `[-7.601,3.993,51.964]` to
  `[-25.200,-12.949,61.791]` and sends the targets/hands to a clearly invalid
  shared frame. The no-proxy-emission variant produces the same solve, so it is
  also rejected.
- Source IR check: the retained JSONL/IR has exact main/fret/strum animation
  channel records, but the exact main clip does not animate the guitar-body/IK
  helper nodes needed for a same-frame bridge. Retained GLB bridges are useful
  evidence, but they are attack or med-idle bridges, not the exact
  stand/fret/strum overlay. Raw `.ska.ps2` / `.ske.ps2` / skin sources were not
  present under `analysis/gh3_midori_source_ir`, and the ISO must not be used
  at game time.
- GLB-to-MILO route check: local ihatecompvir/MiloLib sources remain useful
  reference material (`analysis/gh3_midori_ihatecompvir_bridge_audit.json`
  reports `reference_ready_direct_writer_needed`), but public `glTFMilo` is not
  a drop-in GH2 PS2 character converter. The guarded final route is
  source-authoritative GLB/pose samples into ACP samples, then the repo's GH2
  PS2 `milo_convert_tool` writer.
- Evidence retained:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r45_rebuilt_r40_clavbind_freeze_report_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r45_rebuilt_r41b_from_report_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r45_r41b_shared_prop_pairfit_strings_probe_20260820/guitar_frame_hand_bake_manifest.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r45_r41b_shared_prop_pairfit_strings_noemit_probe_20260820/guitar_frame_hand_bake_manifest.json`.
- Next branch should derive the shared final frame from actual runtime or a
  true same-frame source bridge, not from target-only offsets, prop-string
  blending, or pair-fitting a mismatched hand/prop distance.

### 2026-08-20 Midori r44 target-only correction rejection

- Active goal remains open. Do not block unless the user explicitly instructs
  it again. All heavy commands were run at Idle priority. No ISO was mounted or
  used; the only runtime proof used local `gh2_ps2_hybrid_assets/GEN` plus
  loose DLC.
- Added reproducibility tooling:
  `tools/gh3_midori_patch_stage_from_report.py` can replay retained
  patch-report channel values onto a fresh ACP stage. This recreated r41b from
  local `analysis/gh3_midori_acp_stage`: r40 freeze then r41b patch report.
  Rebuilt r41b replay matched the retained gate (`suspicious=none`,
  `head_spine3=11.674564`, `guitar_head=11.955784`) and the runtime-aware
  contract matched r41b (`LH-FRET=3.832`, `RH-STRUM=0.000`).
- Extended `tools/gh3_midori_guitar_frame_hand_bake_probe.py` with
  `--hand-target-blend-with-current` and `--hand-target-world-offset`, and
  extended generic `--emit-hand-target-proxies` support for visible-arm-chain
  probes. Extended
  `tools/gh3_midori_patch_bake_probe_positions_into_stage.py` so it can consume
  the new generic emitted-proxy manifest field.
- r44 rejected two specific hypotheses:
  - Fixed-r41b-guitar prop-string targets are not useful. A full prop-string
    snap is over-aggressive; a 35% blend still moves the target frame in the
    wrong direction for the waist collapse.
  - Fixed-r41b-guitar current-proxy target-only `+8` world-Z lift is not the
    fix. Bare replay is internally coherent, but runtime-aware contract worsens
    to `LH-FRET=11.593`, `RH-STRUM=5.551`, and direct visual proof rejects:
    the tan hand/forearm mass moves up into the chest/neck while the guitar
    remains too high/right.
- Evidence retained:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r44_rebuilt_r40_clavbind_freeze_report_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r44_rebuilt_r41b_from_report_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r44_rebuilt_r41b_layer_replay_f010_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r44_rebuilt_r41b_contract_hmx_check_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r44_r41b_fixedguitar_propstrings_blend035_probe_20260820/guitar_frame_hand_bake_manifest.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r44_r41b_fixedguitar_targets_zplus8_emitproxy_probe_20260820/guitar_frame_hand_bake_manifest.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r44_targets_zplus8_direct_contract_hmx_20260820.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r44_targets_zplus8_direct_visual_20260820/midori_1_hand_overlay_f010.bmp`.
- Next branch should not do target-only vertical offsets or prop-string blends.
  The useful unresolved problem is the shared final frame that orients the
  guitar prop and hands together; target-only movement leaves the prop frame
  wrong.

### 2026-08-20 Midori r43 pelvis / Control_Root diagnosis

- Active goal remains open. Do not block unless the user explicitly instructs
  it again. Heavy commands should stay Idle/low priority, and runtime/capture
  proof must use `gh2_ps2_hybrid_assets/GEN` plus loose DLC, not a mounted ISO.
- No new build/capture was run for r43. This checkpoint uses retained r41b/r42
  evidence and OGX converter source only:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r43_pelvis_control_root_diagnosis_20260820.md`.
- r41b actual-layer replay says the IK target graph is internally coherent:
  left visible hand to fret target is `3.548390`, and right visible hand to
  strum target is `0.000023`. The rejected visual is therefore not primarily a
  missing `CharIKHand`, missing hand target, or orphaned guitar proxy.
- Key frame-10 placements show the problem: pelvis is at
  `[-0.000,-0.000,39.406]`, `bone_pos_guitar.mesh` is at
  `[-7.601,3.993,51.964]`, left hand/target are around z `40/38`, and right
  hand/target are around z `43`. The main/fret/strum layers have been made
  coherent in a pelvis/root-local frame that still leaves the hand/target
  cluster visually around waist space.
- OGX `milo_convert_tool` already regenerates `bone_fret`, `bone_strum`,
  `bone_fret_hand`, and `bone_strum_hand` under `bone_pos_guitar.mesh`, and
  points `left_hand.ik` / `right_hand.ik` to those targets. That stock-like
  graph is expected; the next useful branch is an automated same-frame bridge
  that emits visible arms/hands and `bone_pos_guitar.mesh` from one shared final
  character-space solve before ACP layer separation.

### 2026-08-20 Midori r42 front-anchor mesh-filter rejection

- Active goal remains open. Do not block unless the user explicitly instructs it
  again. Heavy commands should stay Idle/low priority, and runtime/capture proof
  must use `gh2_ps2_hybrid_assets/GEN` plus loose DLC, not a mounted ISO.
- r41b remains the current visual lead: bipedal/coherent head and body, guitar
  in front rather than over the shoulder, but still rejected because the guitar
  is too high/right and the tan hand/forearm mass collapses around the waist.
- r42 reproduced r41b structurally (`LH-FRET=3.832`, `RH-STRUM=0.000`) and
  tested the narrow hypothesis that three suspicious mixed-bone mesh chunks
  (`midori_1_mesh0_part16/26/32.mesh`) were causing the tan tangle. Isolation
  BMPs show those chunks as debris-like slivers, but deleting them from outfit 1
  did not fix the full pose.
- Evidence retained:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r42_mesh_iso_probe_20260820/mesh_isolation/`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r42_filtered_parts16_26_32_pose_review_20260820_b/midori_1_hand_overlay_f010.bmp`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r42_filtered_parts16_26_32_pose_review_20260820_b.json`.
- Tooling note: ihatecompvir's `MiloEditor`/`MiloLib`/`MiloUtil` is not present
  in the workspace. Web review confirms it exists as a cross-platform Milo scene
  editor/library, but GH2 PS2 character-MILO emission has not been validated.
  Current local builds use OGX `milo_convert_tool`.
- Next branch: solve visible arm/hand positions in the final r41b packed local
  frame, or test a real GLB/MiloLib bridge end-to-end before more manual offset
  grinding.

### 2026-08-20 Midori r3 roll_m10 frame-stability rejection

- Active Midori status remains **not complete**, and the goal should remain
  open unless the user explicitly says otherwise. The latest hand/guitar lead
  (`roll_m10`, final offset `-0.808,0.085,-10.376`, roll `-10`) is rejected as
  an animation-set fix after a frame sweep.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_rollm10_frame_sweep_probe_20260820`
  and contact sheet
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_rollm10_frame_sweep_visual_batch_20260820/contact_sheet.png`.
  frame10 is the only coherent capture (`LH=3.644/RH=2.015`); frame20,
  frame30, and frame40 structurally/visually detach the guitar-hand
  relationship.
- Current decision: preserve `roll_m10` as one-frame diagnostic evidence only.
  Next work should move to a frame-stable source-authored
  guitar-body-to-visible-hand local-frame bridge, or an automated GLB/ACP to
  MILO constraint that feeds the existing direct MILO writer. Do not repeat
  broad one-frame roll, post-translation, or post-rotation sweeps.
- Runtime/capture proof must continue to use local `gh2_ps2_hybrid_assets/GEN`
  plus ordinary loose DLC, not a mounted GH2 ISO. Run heavy build/capture tools
  at Idle/low CPU priority.

### 2026-08-20 Midori r3 source-local frame bridge rejection

- Added `tools/gh3_midori_source_local_frame_bridge_report.py` and exported a
  bounded five-frame source bridge with guitar body, IK helpers, palms, and
  visible arm records:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/source_bridge_attack_arm_guitar_framesweep_20260820/`.
- Raw source-local probes are structural rejects. Summary:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_source_local_frame_probe_20260820/selected_contract_summary.tsv`.
  `ik_direct` left-hand errors remain `61.661`-`76.233`; `palm_direct`
  left-hand errors remain `41.388`-`68.574`; source-IK plus the old
  roll/offset remains `52.447`-`64.410`.
- The local-frame fit report
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_source_local_frame_bridge_report_20260820.json`
  rejects simple source guitar-local calibration: best both-hand IK helper fit
  is `rms=9.160/max=18.098`, best both-hand palm fit is
  `rms=12.415/max=16.857`, and even per-side palm fits are too loose. IK helper
  offsets are nearly static across frames, so they cannot drive frame-stable
  hand motion.
- Current decision: do not repeat raw source-local IK/palm pair sweeps. Next
  work should inspect canonical fret/strum target-space sampling or build a
  richer per-frame constraint using visible-hand deltas directly through the
  existing ACP/MILO writer.

### 2026-08-20 Midori r3 canonical target-space rejection

- Added `tools/gh3_midori_canonical_hand_target_space_report.py` and measured
  canonical fret/strum `.pos` samples over frames `0/10/20/30/40`. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_canonical_hand_target_space_report_20260820.json`.
- Plausible parent interpretations are not close enough: best left plausible
  parent is `bone_pos_guitar.mesh` (`mean=25.055/max=31.552`) and best right
  plausible parent is `bone_pelvis.mesh` (`mean=22.328/max=26.817`).
  Visible-hand-local interpretation is close in the static report, but fails
  after real emission/arm solving.
- Added diagnostic flags `--rebase-fret-target-to-visible-hand` and
  `--rebase-strum-target-to-visible-hand` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`. Structural evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_visible_hand_target_rebase_probe_20260820/contract_summary.tsv`.
  `vh_emitpair` left errors remain `48.626`-`52.402`; `vh_currentpair` left
  errors remain `72.136`-`75.386`; old roll/offset makes the right side worse.
- Current decision: reject parent-only canonical target rebases. Next work
  should inspect solve ordering/feedback between visible arm pose and proxy
  emission, or apply visible-hand deltas directly after the arm pose is fixed.

### 2026-08-20 Midori r3 corrected-candidate post-arm rejection

- The same-turn canonical target/post-arm diagnostics were corrected to use
  `analysis/gh3_midori_gh2_milos` consistently for probe simulation and
  structural contracts. The earlier mixed default-candidate batch is
  superseded.
- Corrected target-space evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_canonical_hand_target_space_report_candidate_20260820.json`.
  It ranks canonical fret `.pos` closest as `bone_L-foreArm.mesh` local
  (`mean=4.149/max=4.149`) and canonical strum `.pos` closest as
  `bone_R-hand.mesh` local (`mean=4.321/max=4.321`).
- Added `--align-target-proxies-to-post-arm-hands` and
  `--rebase-fret-target-to-visible-forearm` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`.
  Corrected structural evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_postarm_proxy_align_candidate_probe_20260820/contract_summary.tsv`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_forearm_hand_target_rebase_probe_20260820/contract_summary.tsv`.
- Decision: reject both routes. Correct candidate usage improves early frames
  but frame20+ still collapses. `postarm_current` left error grows to
  `34.516/35.203/38.050` at frames `20/30/40`; `forearmhand_currentpair`
  similarly jumps to `35.067/36.114/34.800`.
- Next work should inspect the visible-arm two-bone solve itself across
  frames: target reachability, elbow plane choice, aim vectors, and emitted
  local rotation continuity. Do not keep sweeping target parent rebases,
  post-arm proxy alignment, or roll/offset combinations.

### 2026-08-20 Midori r3 visible-arm rot+pos rejection

- Added `tools/gh3_midori_visible_arm_solve_report.py`. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_visible_arm_solve_report_20260820.json`.
  The current strum-guitar target recipe is mostly clamped/out of practical arm
  reach. The corrected forearm/right-hand recipe is reachable in isolated solve
  math, but does not survive the final emitted ACP/MILO contract.
- Added diagnostic `--emit-visible-arm-positions` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`. Structural evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_emit_visible_arm_positions_probe_20260820/contract_summary.tsv`.
  `rotpos_current` improves frames 0/10 but still collapses at frames 20/30/40
  (`LH=31.453/32.867/39.820`). `rotpos_current_rollm10` and
  `rotpos_forearmhand_currentpair` are not better.
- Sanity check: the failing frame20 candidate contains the expected
  `bone_L-foreArm.mesh.pos` and `bone_L-hand.mesh.pos` channels, but final
  contract reconstruction still places `bone_L-hand.mesh` 31.453 from
  `bone_fret_hand.mesh`.
- Current decision: reject canonical visible-arm rot+pos emission. Next work
  should compare bake-side world reconstruction against final contract-side
  reconstruction for the same emitted samples, focusing on sample ordering and
  viewer prop override application. Do not continue arm-position, parent
  rebase, or roll/offset sweeps until that mismatch is closed.

### 2026-08-19 Midori stock hand-target override rejection

- Added diagnostic-only post-sample stock hand target overrides to
  `tools/gh3_midori_guitar_ik_contract_report.py`. This tests whether the
  stock-vs-Midori `bone_strum_hand.mesh` mismatch should be corrected in the
  hand target channel after canonical hand-bank samples are applied.
- New evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/stock_hand_target_override_decision_20260819.json`.
  Baseline generated-main/fullhands rerun matches the prior useful branch:
  left hand to fret target `40.978`, right hand to strum target `13.162`,
  anchor disagreement `112.450`.
- Forcing `bone_strum_hand.mesh` to the stock GH2 controller local after clip
  samples makes the right-hand distance worse (`15.621`). Forcing
  `bone_fret_hand.mesh` helps the left side only modestly (`35.971`) and
  worsens anchor disagreement (`120.564`). Forcing both keeps the right side
  worse and does not resolve the anchor split.
- Decision: do not rebuild/capture stock hand-target local overrides. The
  canonical full hand-bank target positions are being interpreted in the wrong
  parent frame or space; substituting stock controller locals is not enough.

### 2026-08-19 Midori guitar anchor graph diagnosis

- Added `tools/gh3_midori_guitar_anchor_graph_report.py` and evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/guitar_anchor_graph_decision_20260819.json`.
- The stock-vs-Midori graph report compares only the guitar anchor subtree
  (`bone_pos_guitar.mesh`, `bone_fret.mesh`, `bone_strum.mesh`,
  `bone_fret_hand.mesh`, `bone_strum_hand.mesh`) from stock glam1/xplorer and
  the current Midori model. After the xplorer fret override, Midori's fret
  subtree matches stock, but `bone_strum_hand.mesh` local differs from stock by
  `14.965` units in the deployed model bind graph.
- Also built a structural stock attach-world plus canonical full hand-bank
  candidate. It was not captured because the contract report worsened:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/stockattach_fullhands_anchor_contract_report_20260819.json`
  reports left hand to fret target `43.106` and right hand to strum target
  `24.875`, compared with `40.978` and `13.162` for the prior
  generated-main/fullhands branch.
- Next branch should not capture stockattach/fullhands. Correct or compensate
  the stock-vs-Midori `bone_strum_hand.mesh` target graph mismatch first, then
  remeasure the full-hand contract before another visual run.

### 2026-08-19 Midori canonical-fret visible-left-arm rejection

- Extended `tools/gh3_midori_guitar_frame_hand_bake_probe.py` with canonical
  fret merge modes. The tested proof branch preserves canonical
  `bone_fret_hand.mesh` and left finger channels, then appends visible left
  upper/forearm quats. The best structural variant was
  `canonical-fret-visible-left-arm-rot` with `--arm-chain-reach-scale 1.5`:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/canfret_leftarm_rot_r150_anchor_contract_report_20260819.json`
  reports left hand to fret target `10.782`, versus `40.978` for the previous
  generated-main/full-hand branch.
- Captured the branch through local `gh2_ps2_hybrid_assets/GEN` plus loose DLC
  only, at low priority, then restored canonical loose hashes. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/canfret_leftarm_rot_r150_pose_review_20260819/visual_decision.json`.
- Visual result: rejected. The layer is genuinely loaded (`fret ch=19`), and
  the pose remains upright/bipedal, but the guitar still runs down through the
  legs and visible hands remain near shin/foot space. The missing-left-arm
  channel diagnosis is therefore only partial.
- Next branch should stop simple left-arm merge variants and diagnose the
  guitar prop/main anchor frame against the native stock hand-bank parent graph:
  `bone_fret.mesh`, `bone_strum.mesh`, and their relation to
  `bone_pos_guitar.mesh` after the external xplorer prop attach.

### 2026-08-19 Midori full hand-bank z-offset rejection

- Active Midori status remains **not complete**. The useful hand-overlay family
  is still generated narrow mesh-channel guitar-main plus canonical full
  fret/strum hand banks, but direct visual inspection rejects both new z-offset
  probes:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/fullhands_zminus8_pose_review_20260819/visual_decision.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/fullhands_zplus8_pose_review_20260819/visual_decision.json`.
- `zminus8` stays upright and bipedal, but the guitar still runs down through
  the legs and the picking hand collapses near knee/foot space. `zplus8` is
  worse: the guitar/arm stack is lower and partly clipped. Simple world-Z frame
  offsets are therefore rejected as the fix.
- Next branch should keep the canonical full hand banks and diagnose the
  generated narrow guitar-main placement/anchor contract. Do not revisit
  pelvis/Control_Root, endpoint-only probes, minimal arm-chain probes, or
  canonical-main/full-body-main overlays.

### 2026-08-19 Midori hand-bank anchor contract diagnosis

- Added optional `--case-name` filtering to
  `tools/gh3_midori_guitar_ik_contract_report.py` so single-clip diagnostic
  MILOs can be measured without requiring a full review candidate.
- New evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/hand_bank_anchor_contract_decision_20260819.json`.
  The generated-main/full-hand branch reports left hand to fret target
  `40.978`, right hand to strum target `13.162`, and solved guitar anchor
  disagreement `112.450`. Canonical-main/full-hands is also inconsistent
  (`45.267`, `25.889`, `66.774`), and disabling viewer prop overrides does not
  materially change the diagnosis.
- Channel contract diagnosis: canonical fret clips drive `bone_fret_hand.mesh`
  and left finger quats, but not `bone_L-upperArm.mesh`, `bone_L-foreArm.mesh`,
  or `bone_L-hand.mesh`. Canonical strum clips do include the right clavicle,
  upper-arm, forearm, hand, and finger quats. The z-offset/guitar-placement
  probes were therefore trying to satisfy a left proxy/finger target without a
  matching visible left arm solve.
- Next branch should preserve canonical full fret/strum hand and finger
  channels, then merge a visible left-arm chain bake toward
  `bone_fret_hand.mesh`. Do not replace the canonical full hand banks with the
  earlier minimal arm-chain-only clips.

### 2026-08-18 Midori targetlength visual rejection

- Active Midori status remains **not complete**. The current targetlength
  candidate is now formally rejected before user approval:
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-targetlength-bakepos-bind`.
- Added
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_packedparent_codex_bipedal_verdicts.json`
  and wired it into `tools/gh3_midori_targetlength_packet_precheck.py`.
  The stricter precheck rejects the two native medium-idle captures
  (`midori_1_medium_idle_f060`, `midori_2_medium_idle_f060`) because they are
  horizontal/sideways instead of readable upright idle poses. The packet
  precheck now reports `failed_reject_before_user_approval`, `failed_item_count
  = 2`; the direct visual approval gate now reports `failed`.
- Refreshed rollout, review packet, and completion audit after this rejection.
  Canonical completion audit now reports `status=failed`, `proven=13`,
  `pending=0`, `failed=5`. A dry-run accept with `--validate-gate` now fails
  cleanly because the visual packet precheck is rejected, so stale approval
  commands cannot override the bipedal gate.
- Fresh med-idle source bridge remains upright/coherent, so the failure is in
  the retarget/output mapping. New med-idle targetlength pose report at frame
  60 shows ordinary target graph max pose error `0.540991`, shin dots `1.0`,
  and min child aim `0.996907`, proving child-aim metrics can miss the global
  idle orientation failure. A diagnostic rerun with `--control-root-pelvis-parent`
  exposed through `tools/gh3_midori_pose_report.py` makes the skeleton much
  worse (`max_pose_error=49.412737`, min child aim `-0.048227`), so the next
  route is **not** a simple pelvis reparent. Continue with a Control_Root
  basis-compensated root/pelvis solve that keeps med-idle upright while
  preserving target-length child segments.
- Follow-up rootbasis sweep and a new diagnostic
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-targetlength-stablepelvis30-bakepos-bind`
  are recorded in
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/stablepelvis30_diagnostic_decision.json`.
  Existing `rootlocal`, `rootbasis`, `rootyawfold`, `upalign`, and axisblend
  branches do not dominate targetlength across med-idle and attack-left.
  The stablepelvis30 branch is rejected before visual staging: med-idle frame
  60 max pose error worsens from `0.540991` to `35.533740`, and attack-left
  frames 15/30/45 worsen from near-zero max pose error to `35.367826`-
  `39.466837`. It preserves child aim, which further proves child-aim metrics
  alone are insufficient for bipedal visual approval.
- Added `tools/gh3_midori_visual_orientation_diagnostic.py` and wired its
  output into `tools/gh3_midori_targetlength_packet_precheck.py`. Current
  evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_idle_orientation_diagnostic.json`.
  Source med-idle front render is near-vertical (`upright_score=1.022727`,
  PCA delta from vertical `12.900003` degrees), but the two target medium-idle
  native captures fail upright-idle orientation (`upright_score=0.767699` /
  `0.812095`, PCA delta from vertical `64.730045` / `57.226530` degrees).
  The same diagnostic records the parent-space mismatch: source bridge
  `bone_pelvis` is under `Control_Root`, while the current target runtime
  skeleton frame has `Bone_Pelvis.parent_source=""`. The visual packet
  precheck still rejects exactly `midori_1_medium_idle_f060` and
  `midori_2_medium_idle_f060`, now with both manual bipedal and numeric
  orientation evidence.
- Added `tools/gh3_midori_parentage_basis_diagnostic.py` and wired its output
  into `tools/gh3_midori_targetlength_packet_precheck.py`. Current evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_parentage_basis_diagnostic.json`.
  This corrects the parentage diagnosis: the current Midori model rig has
  `bone_pelvis.parent="Control_Root"` and the source bridge has
  `Bone_Pelvis.pose_parent="Control_Root"`, but the targetlength animation
  skeleton report has `Bone_Pelvis.parent_source=""`. Status is
  `target_model_animation_parentage_mismatch_confirmed`. Next fix should make
  the animation bind graph match the deployed model hierarchy, or compensate
  for the mismatch before packing; do not pursue the already-rejected flat
  targetlength/stable-pelvis branches.

### 2026-08-18 Midori expectation/status checkpoint

- Active Midori status remains **experimental and not complete**. The current
  uncertainty is narrowed to the animation retarget against the GH2 target
  hierarchy, not basic packaging or whether the static Midori assets can be
  staged as ordinary loose DLC.
- The current candidate remains
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-targetlength-bakepos-bind`.
  Automated structural/gameplay gates pass, but direct user visual approval is
  still required because non-bipedal captures are immediate rejects.
- `tools/gh3_midori_targetlength_visual_decision.py` now supports
  `--validate-gate`. Dry-run accept/reject decisions are validated against the
  real targetlength visual approval gate using a temporary approval JSON, so no
  real approval file is created during simulation. Dry-run accept produced
  `direct_user_visual_approval_accepted` with zero failures; dry-run reject of
  `midori_1_fast_jump_f040` produced
  `direct_user_visual_approval_rejected` with zero failures. The live gate
  remains `pending_user_visual_approval` with `approval_exists=False`.

### 2026-08-18 Midori visual rejection and route correction

- Active Midori status remains **not complete**. The
  `matrix-local-axis-align-bind` approval packet under
  `.codex/current-evidence/midori-review-source-bridges-20260818/` is now
  treated as visually rejected by the user, not pending approval. User
  observations: only the middle-right pose looked coherent, and non-bipedal
  captures must be immediate rejects.
- The problem is the animation set retargeted against the GH2 target rig, not
  only the static model/rig. Continue from the pelvis-only
  `Control_Root.matrix_local` diagnosis and the later source-authoritative
  bridge/target-skeleton solve work.
- GLB remains acceptable as an automated intermediate, but the public
  `glTFMilo` route is not a drop-in GH2 PS2 final converter. Use the
  ihatecompvir/MiloLib material as format reference and keep final output on
  the local GuitarHeroOGX GH2 `CharClipSet`/`CharClipSamples` writer path.
- Runtime proof must use local `gh2_ps2_hybrid_assets/GEN` plus loose DLC, not
  a mounted GH2 ISO. Build/capture processes should be run at Idle/low CPU
  priority.
- Fresh low-priority source bridge export for
  `gh3_guit_mido_a_med_idle01` frames 0 and 60 now produces coherent upright
  source GLB renders. Evidence:
  `.codex/current-evidence/midori-review-source-bridges-20260818/checksum_bonelist_source_bridge_medidle_20260818_decision.json`.
  The old non-biped rotation-only GLB evidence is superseded for this clip; the
  next step is to re-export the rejected representative source clips and feed
  those fresh bridge poses into ACP/MILO retargeting.
- Fresh low-priority source bridge export for attack
  `gh3_guit_mido_a_attackl` frames 0/15/30/45 also succeeds and renders a
  coherent source motion contact sheet. Evidence:
  `.codex/current-evidence/midori-fresh-attack-pelvisbase-20260818/decision.json`.
  The fresh source bridge was not enough by itself: `pelvisbase` matched the
  in-memory pose report, but the staged ACP disk gate rejected because
  source-relative tiny child positions were persisted as `.mesh.pos` channels,
  collapsing leg lengths after the pelvis.
- New target-length-preserving policy
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-targetlength-bakepos-bind`
  now uses fresh source-bridge directions while preserving GH2 target bind
  segment lengths. One-clip attack-left proof passes in-memory pose report
  and raw disk ACP gate when reconstructed with matching
  `--mesh-target-scope all-common --stock-hand-detail-rig --stock-bind-scope upper-limbs-guitar`.
  Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/decision.json`.
  Gate summary: max pose error `0.000012`, right/left shin dots `1.0`,
  min child aim `0.894114`. This is **not** visual-approved yet; next step is
  targetlength visual review/contact-sheet or game-facing render, then carry
  the passing policy through automated GLB/ACP-to-MILO DLC output if coherent.

### 2026-08-16 Midori pelvis-only hierarchy diagnosis

- Active Midori status remains **not complete**. The current blocking gate is
  pelvis-only animation-space conversion for `gh3_guit_mido_a_attackl` at
  frames 0/15/30/45, before thighs or upper hierarchy are added back.
- New matrix-local probe found `Control_Root.matrix_local` is a non-identity
  rest basis (`[[1,0,0],[0,0,-1],[0,1,0]]` approximately), and
  `bone_pelvis` evaluates under that parent. Direct matrix-local emission fails
  in GHC, and the best Control_Root parent-conjugate diagnostic only improves
  frames 0/15; it still fails frames 30/45.
- Rejected in this pass: matrix-local pose, matrix-local plus bind,
  parent-conjugate transpose/source, parent-conjugate direct Hmx storage,
  parent-conjugate delta-bind, and parent-conjugate target-basis rebase.
  Next diagnostic should compare source evaluated `pose.matrix` rows against
  the exact GHC-reconstructed pelvis local matrix. An automated Blender/.glb
  evaluated-pose bridge is acceptable if the direct SKA quaternion path remains
  ambiguous.
- Follow-up matrix diagnostic now proves emitted matrices round-trip through
  GHC/Hmx quaternion decode with near-zero error, so sample packing is not the
  blocker. Blender `--pose-json` bridge export is automated and shows small
  IR-vs-evaluated pose deltas, worst at frame 15 (`0.01607`). The missing piece
  is deriving GH2 pelvis parent-local space from Blender evaluated pose plus
  the stock Glam pelvis bind basis, not another Hmx storage variant.
- Tested `matrix-local-rest-delta-bind`
  (`target_local = (source_rest^-1 * source_pose) * target_bind`) visually in
  GHC. It is rejected: frame 30 leans but as screen-plane roll, and frame 45 is
  nearly horizontal instead of the source leaned-back profile. Continue with a
  solved constant axis alignment between Blender evaluated pose and GH2 pelvis
  parent-local space.
- New signed-axis bind-alignment diagnostic `matrix-local-axis-align-bind`
  selects `C = [[0,1,0],[0,0,-1],[-1,0,0]]` for Midori pelvis to Glam pelvis,
  then bind-corrects rest exactly to Glam. This is the first viable pelvis-only
  visual branch: frames 15/30/45 read upright/profile and leaned-back/profile
  in GHC instead of rolling flat or staying upright. Frame 0 remains slightly
  too neutral/front-facing, so treat this as provisional and continue from this
  branch rather than declaring the pelvis gate fully approved.
- First child-tier probe with
  `Bone_Pelvis,Bone_Thigh_L,Bone_Thigh_R` under
  `matrix-local-axis-align-bind` is provisionally coherent: frames 15/30/45
  keep the profile/back-lean family and visible legs do not explode or mirror.
  Frame 0 and lower-leg visibility are weak because the current GHC viewer crop
  hides much of the lower body. Next: improve camera parity or add knees under
  the same signed-axis policy.
- Latest bridge work has moved past local blend sweeps. Source bridge export
  via Blender/NXTools is automated and can also emit GLB. The best visually
  stable lower-body branch before the new probe remains
  `matrix-leg-ik-pelvisgate-flipblend42-bendout45-bind` / bridge42, but it is
  still not source-approved. Added
  `matrix-leg-constraint-bridge-pelvisgate-bind`, which keeps pelvis in the
  known-good axis-aligned family and solves target-length thigh/knee local
  rotations from bridge hip/knee/ankle positions. It stages and builds as an
  ordinary one-clip guitar-main CharClipSamples MILO, and f30 sampled quats show
  material thigh/knee changes while pelvis remains identical to bridge42.
  Direct visual capture is pending because `gh2_ps2_hybrid_assets/gen/main.hdr`
  and `main_0.ark` are absent in the current workspace state. Evidence:
  `.codex/current-evidence/midori-constraint-bridge-20260817/`.
- Follow-up direct visual check mounted `Guitar Hero II PS2 (USA).iso` as
  `D:\GEN`, re-staged bridge42 and
  `matrix-leg-constraint-bridge-pelvisgate-bind` with the correct animated
  pelvis translation, built one-clip main MILOs, temporarily swapped each into
  the loose Midori DLC, captured attack frames `15/30/45` at yaws
  `0/90/180/270`, and restored the deployed main MILO. All captures loaded the
  intended clip and pose-publisher layer. Result: rejected. The constraint
  branch tracks bridge42 and does not approach the source f30/f45 lifted-leg /
  back-lean silhouette. Evidence:
  `.codex/current-evidence/midori-constraint-bridge-visual-20260817/`.
- New torso-aware source bridge export is available at
  `.codex/current-evidence/midori-source-bridge-torso-20260817/`. Use the
  `gh3_guit_mido_a_attackl.torso.ignorepartial.pose_bridge.json` variant. The
  honor-partial NXTools bridge retained only neck and one ankle curve, while
  decoded local IR shows the clip has broad pelvis/spine/leg animation. The
  `--ignore-partial-anims` bridge validates as 60 records plus a skinned
  animated GLB and retains pelvis, stomach, chest, neck/head, and leg curves.
  Next route is a real target-skeleton bake/solve from these evaluated matrices,
  solving pelvis/spine/legs together and emitting ordinary GH2 local rotations.
- Added diagnostic
  `matrix-torso-bridge-leg-ik-pelvisgate-flipblend42-bendout45-bind`.
  It uses the ignore-partial torso bridge for pelvis/spine/neck/head and runs
  the stable 42/bendout45 leg IK under that moving bridge pelvis parent.
  One-clip staging for `gh3_guit_mido_a_attackl` succeeds with
  `source_pose_bridge_active=true`; evidence is
  `.codex/current-evidence/midori-torso-bridge-legik-20260817/`. Tests now pass
  10/10. Valid grouped GHC capture rejects it: frame 15 keeps vertical leg
  failure and frames 30/45 floor-fold or roll sideways.
- Follow-up target-graph diagnosis found the root/pelvis bug in the rejected
  torso branch: `Control_Root` is a source parent, but GHC guitar-main does not
  stage Control_Root channels and target `bone_pelvis` is root-parented. Added
  `matrix-torso-targetgraph-bridge-leg-ik-pelvisgate-flipblend42-bendout45-bind`
  to localize through the GH2 target graph. It captures validly but still
  rejects visually. Added
  `matrix-torso-targetgraph-stablepelvis-bridge-leg-ik-pelvisgate-flipblend42-bendout45-bind`,
  preserving the known-good matrix-local-axis pelvis while applying bridge/IK
  below it; this also captures validly and is still rejected. Evidence:
  `.codex/current-evidence/midori-torso-targetgraph-bridge-20260817/` and
  `.codex/current-evidence/midori-torso-targetgraph-stablepelvis-20260817/`.
  Follow-up bridge endpoint variants under the stable pelvis also capture
  validly and reject:
  `matrix-torso-targetgraph-stablepelvis-bridgeendpoints-leg-ik-pelvisgate-flipblend42-bendout45-bind`
  and
  `matrix-torso-targetgraph-stablepelvis-bridgeendpointsaxis-leg-ik-pelvisgate-flipblend42-bendout45-bind`.
  They stabilize f30/f45 compared with the floor-fold branches, but neither
  reaches the source arched lifted-leg silhouette and f15 remains vertical-leg.
  Evidence:
  `.codex/current-evidence/midori-torso-targetgraph-stablepelvis-bridgeendpoints-20260817/`
  and
  `.codex/current-evidence/midori-torso-targetgraph-stablepelvis-bridgeendpointsaxis-20260817/`.
  Next branch should solve the root/control frame, pelvis, and leg endpoints
  together from the evaluated bridge before writing ordinary GH2 target-graph
  locals.
- Root-placement continuation added `matrix-torso-targetgraph-rootworld-facingpos-bridgepos-bind`,
  `matrix-torso-targetgraph-rootworld-absposepos-bind`, and
  `matrix-torso-targetgraph-rootworld-glbposepos-bind`. `facingpos` publishes
  mapped `Control_Root` absolute placement as `bone_facing.pos` but is
  pixel-identical to bridgepos at frames 15/30/45. `absposepos` writes mapped
  absolute `Bone_Pelvis` placement to `bone_pelvis.pos`; only frame 30 changes
  and the silhouette remains rejected. `glbposepos` emits GLB/evaluated local
  position tracks for pelvis, spine/neck, and both leg chains; it changes all
  three frames through the visible pose path, but direct inspection rejects the
  over-extended non-source-like lower body. Evidence:
  `.codex/current-evidence/midori-facingpos-rawstage-20260817/`,
  `.codex/current-evidence/midori-absposepos-rawstage-20260817/`, and
  `.codex/current-evidence/midori-glbposepos-rawstage-20260817/`. GLB remains
  acceptable as an automated intermediate, but the next useful branch is a
  target-length-preserving skeleton solve from evaluated positions, not raw
  per-joint local `.pos` baking. Focused pipeline tests now pass 23/23.
- Added `matrix-torso-targetgraph-rootworld-glbik-bind`, which feeds evaluated
  GLB/source hip-knee-ankle positions into the target-length-preserving two-bone
  IK path while emitting ordinary target-graph local rotations instead of raw
  local `.pos` tracks. One-clip staging emits `13` channels (`bone_pelvis.pos`
  plus local quats) and focused tests pass 24/24. GHC accepts the
  `gh3_guit_mido_a_attackl` override at frames 15/30/45, but direct visual
  inspection rejects it: frame 15 lies sideways and frames 30/45 collapse near
  the floor/vertical leg line. Evidence:
  `.codex/current-evidence/midori-glbik-rawstage-20260817/`. Next branch should
  solve root/pelvis orientation and leg plane together before IK. GLB remains
  acceptable as an automated bridge input, but future diagnostic captures should
  not mount the GH2 ISO and should run app/build processes at Idle priority.
- Added staged `matrix-torso-targetgraph-rootworld-glbframeik-bind` candidate.
  It fixes the `glbik` ordering mistake by combining bridge/bind-height pelvis
  placement (`z ~= 39.43` at frames 15/30/45) with a GLB-derived pelvis/torso
  frame before target-length leg IK. The staged one-clip `guitar-main` output
  remains ordinary clip data: `13` channels, `bone_pelvis.pos` plus local
  quats, with no raw per-joint GLB `.pos` tracks. Focused tests pass 25/25.
  Evidence:
  `.codex/current-evidence/midori-glbframeik-rawstage-20260817/`. Status is
  staged pending visual; next step is one-clip MILO build and GHC frames
  15/30/45 without mounting the GH2 ISO, with build/app processes at Idle
  priority.
- No-ISO local-GEN visual gate for `glbframeik` completed. `gh2_ps2_hybrid_assets/GEN`
  now contains extracted `main.hdr` + `main_0.ark`, and GHC ran from that local
  archive path rather than a mounted ISO (`D:\GEN=False` afterward). GHC
  accepted `gh3_guit_mido_a_attackl` at frames 15/30/45 and restored the
  deployed main MILO to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
  Direct visual inspection rejects the branch: pelvis height is corrected, but
  the body remains sideways/upside-down. Evidence:
  `.codex/current-evidence/midori-glbframeik-rawstage-20260817/`. Next branch:
  solve GLB pelvis/root frame handedness/orientation before torso/leg IK; do not
  retune limb IK until pelvis frame alone reads upright/profile in GHC.

### 2026-08-15 GH3 PS2 Midori external character pipeline

- Active goal: build a GH3 PS2 to GH2 PS2 character pipeline, starting with
  Midori as an ark-external custom character. Scope remains both base Midori
  outfits and all Midori animations, with alternate color skins excluded.
- Current rollout status: **not complete**. The latest Guitar Hero Classic
  visual captures show Midori loading externally as a `BandCharacter`, with
  corrected texture decode, stage placement, and the stock guitar prop attached,
  but the playable pose/IK presentation is still wrong: the guitar occludes the
  body and the arms/hands do not read as a stock-quality guitarist hold. Do not
  treat structural manifests or automated checks as completion until visual
  captures for both outfits look release-ready.
- Community reuse remains the rule: GH3 mesh/texture/SKA behavior is anchored
  to Fretworks NXTools/GHTools and Addy Mills' GH Toolkit references; local
  code owns only the GH3 PS2 DATAP shim, GH3-to-GH2 basis/scale bridge, and
  GH2 MILO/DLC packaging around the existing repo serializers.
- Current live package:
  `GuitarHeroOGX-main-ui-engine/DLC/community.gh3.midori/`, six assets,
  two outfits, four role-split animation banks, 280 source GH3 clips plus 36
  GH2 runtime aliases, and no alternate skins.
- Automated manifests are useful structural gates only. Any completion audit or
  aggregate proof that reports complete is stale until the gameplay screenshots
  satisfy the visual contract.
- Important fix: Midori skeleton/animation naming now resolves GH3 names by
  QB-style checksum (`~crc32(lowercase_name)`) instead of positional
  `BoneList_GHWTRocker.ghbones` order. This corrected hierarchy mistakes such
  as `Bone_Thigh_R` being parented under `Bone_Ankle_L`; it now maps to
  `bone_R-thigh` under `bone_pelvis`. Unknown extras use parser-compatible
  checksum-stable `bone_gh3_*` names so generated models and animation banks
  agree without invalid `CharBonesSamples` channel prefixes.
- Fresh native catalog validation:
  `ghogx_character_variant_catalog_test` passes against the loose DLC package
  after rebuilding both packaged model MILOs from the checksum-compatible
  model source directory.
- New retained pose evidence:
  `analysis/gh3_midori_pose_quality_proofs/pose_quality_proof_manifest.json`
  verifies both outfits in bind pose and at `stand_medium_01` frame 60. The
  native viewer captures are nonblank, centered, upright, textured, and within
  GH2 character scale; logged Z spans are about 72.6 and 74.1 units. This is a
  basis/skinning smoke, not broad visual approval for all clips.
- New retained motion evidence:
  `analysis/gh3_midori_animation_pose_proofs/animation_motion_proof_manifest.json`
  verifies native viewer motion away from bind pose for checksum-resolved main
  animations on both outfits, plus a representative main/strum/fret overlay
  stack with three pose-publisher layers. Hand banks omit unsafe position and
  checksum-unknown fallback channels so strum/fret clips parse in the native
  loader.
- New retained pose/framing review:
  `analysis/gh3_midori_pose_review_proofs/pose_review_proof_manifest.json`
  captures nine 1280x720 native viewer cases at a wider camera distance:
  main idle, attack, jump, solo, transition, accessory fallback, main+strum+fret
  overlay, and the second outfit's idle/attack checks. It verifies clip loads,
  pose-publisher layer counts, framing margins, texture-rich nonblank pixels,
  and sampled pixel differences from each outfit's idle reference.
- Hybrid deployment proof:
  `tools/gh3_midori_deploy_hybrid_dlc.py` mirrors the package into
  `gh2_ps2_hybrid_assets/DLC/community.gh3.midori/` and verifies 7 files, 6
  content assets, and 24,883,338 content bytes match the source-tree DLC.
  `tools/gh3_midori_deploy_hybrid_runtime.py` deploys the current release app
  into `gh2_ps2_hybrid_assets/ghogx_app.exe` and verifies it byte-matches the
  source build at SHA-256
  `1caef1294fa9a914ac4d4eae59fd8523990c8e53c3fedb182436725977e3c190`.
  `analysis/gh3_midori_hybrid_dlc_pose_review_proofs/pose_review_proof_manifest.json`
  reuses the native viewer review through the deployed hybrid executable
  `gh2_ps2_hybrid_assets/ghogx_app.exe` against
  `gh2_ps2_hybrid_assets/GEN` with
  `GHOGX_ADDONS_DIR=gh2_ps2_hybrid_assets/DLC`; 9 captures pass.
- Guitar Hero Classic gameplay proof:
  `analysis/gh3_midori_ghc_gameplay_proofs/gameplay_proof_manifest.json`
  verifies both `gh3_midori_1` and `gh3_midori_2` in song as `guitarist0`
  through `gh2_ps2_hybrid_assets/ghogx_app.exe` against
  `gh2_ps2_hybrid_assets/GEN` and `GHOGX_ADDONS_DIR=gh2_ps2_hybrid_assets/DLC`.
  The manifest records runtime name `Guitar Hero Classic`, the deployed app
  SHA-256
  `1caef1294fa9a914ac4d4eae59fd8523990c8e53c3fedb182436725977e3c190`,
  active `stand_medium_01` clips sourced from
  `char/gh3_midori/anims/gen/gh3_midori_main.milo_ps2` at song time 30.000,
  final gameplay state `playing`, and nonblank 1280x720 captures for both
  outfits.
- PCSX2 handoff:
  `tools/gh3_midori_pcsx2_handoff.py` writes
  `analysis/gh3_midori_pcsx2_handoff_manifest.json`, verifies the hybrid DLC
  package prerequisites, the deployed hybrid runtime app manifest, and the
  live deployed app hash before recording the no-desktop-automation capture
  boundary. It binds optional user-run evidence hashes when supplied, validates
  a strict runtime evidence JSON, and can generate the reusable template at
  `analysis/gh3_midori_pcsx2_runtime_evidence.template.json`. Acceptance
  requires PCSX2 executable/version details, game image or runtime path, true
  no-input/no-window/no-memory-write attestations, PCSX2-native capture
  attestation, the deployed hybrid app path and SHA-256, and existing nonempty
  capture files for both `gh3_midori_1` and `gh3_midori_2`. Capture artifacts
  must be typed as `screenshot` or `video` with matching image/video
  extensions; BMP screenshots are additionally inspected for minimum
  dimensions and sampled nonblank color diversity. The aggregate verifier now
  checks the runtime evidence template against the deployed app/DLC hash set.
  Current summary:
  status `pcsx2_handoff_ready_pending_user_capture`, 6 assets, 24,883,338
  bytes, deployed app SHA-256
  `1caef1294fa9a914ac4d4eae59fd8523990c8e53c3fedb182436725977e3c190`,
  0 PCSX2 executable candidates from safe/common discovery, and 0 attached
  evidence files; runtime evidence is missing/incomplete by design. This is
  optional emulator evidence, not the completion gate for Guitar Hero Classic.
- Still open: only any later human/art-direction pass desired beyond the
  representative automated review, plus optional PCSX2-native corroboration if
  wanted.

### 2026-08-11 singer selectability / addon catalog continuation

- Latest cross-thread context is from `Resume stuck work thread`, not the older
  release-audit handoff. The active direction is to make both singer characters
  selectable without replacing stock characters.
- Accepted portrait masters are retained in
  `GuitarHeroOGX-main-ui-engine/proofs/singer-icon-final-inputs/`:
  `male-singer-icon-final-master.png` (219x292) and
  `female-singer-icon-final-master.png` (660x880). Both are 3:4 bust crops
  matching the measured `sel_character.milo_ps2` portrait quad ratio.
- The selector direction is a provider-driven one-column film reel per player:
  fixed center selected portrait, previous/next above and below, wrapping
  roster order, independent P1/P2 positions, and no fixed 11-slot rack.
- Catalog load now supports the needed data order: built-in
  `config/gen/character_variants.dtb` first, then per-addon
  `manifest.json` files from `addons/*` or `GHOGX_ADDONS_DIR`.
  Each addon is self-contained; duplicate selection IDs are rejected.
- `CharacterVariant` now carries `portrait_path`, and `character_provider`
  exposes `get_portrait(index)` so the upcoming film reel can consume the
  merged catalog without knowing whether a row came from DTB or JSON.
- `ghogx_character_variant_catalog_test` creates a temporary addon manifest,
  merges it after the packed catalog, verifies provider label/portrait access,
  and still validates the existing packed model/animation routes.
- Verified: `ghogx_character_variant_catalog_test` passes via CTest against
  the merged archive. No commit or push was made.

### 2026-08-11 release audit continuation

- Continued from the dirty `GuitarHeroOGX-main-ui-engine` release-readiness
  worktree without reverting existing changes.
- Registered `ghogx_character_variant_catalog_test` as a normal CTest gate.
  The test now defaults to the merged
  `gh2_ps2_hybrid_assets/gen` archive, skips cleanly when absent, and still
  accepts explicit `<main.hdr> <main_0.ark>` arguments.
- Fixed campaign difficulty canonicalization so UI/display spellings such as
  `easy` persist progress under the production
  `kDifficultyEasy` keys. This restores saved/reloaded per-song beat progress
  for the release progression audit and keeps the converted female singer's
  `won_campaign` unlock proof meaningful.
- `ghogx_release_progression_audit` now canonicalizes its difficulty argument
  before checking saved `beat.<difficulty>.<song>` rows.
- Verified:
  - Targeted build of `ghogx_release_progression_audit`,
    `ghogx_ui_test`, and `ghogx_character_variant_catalog_test`.
  - Six-mode release progression audit against isolated temp profiles:
    write/read, purchase-write/purchase-read, and
    profiles-write/profiles-read all pass. Temp profiles were deleted.
  - Focused CTest pass:
    `ghogx_gameplay_venue_band_contract_test`,
    `ghogx_gameplay_rules_test`, `ghogx_ui_test`, and
    `ghogx_character_variant_catalog_test` all pass.
- No commit or push was made. The broader dirty worktree remains preserved.

### 2026-08-02 crowd and crowd-floor source parity

- The crowd/floor subcase is complete across all seven converted GH1 venues
  and all eight native GH2 venues. This does not close the broader matched
  venue-presentation item.
- GH1 now decodes and renders all 41 `MultiMesh0` crowd objects from exact
  template references and instance transforms. The converter emits the
  source-derived `__gh1_runtime_multimeshes.grp`; after venue-section merge,
  runtime binds decoded `crowd.env` to it, matching the retail
  `Arena + 0x9C` ownership recovered at
  `SLUS_212.24:0x001685DC..0x0016862C`.
- The seven-venue runtime submits all 2,265 authored GH1 instances with zero
  missing templates. The separate native GH2 matrix retains 49
  `WorldCrowd6` actors and 4,080 placements.
- The fake crowd-floor renderer was removed. Retail
  `WorldCrowd::CleanUpCrowdFloor` establishes that the GH2 placement Mesh is
  construction data, not a visible floor.
- Seven GH1 crowd packages preserve their authored 25,220-transparent /
  7,548-opaque binary mask. Theatre's cached source CLUT is all opaque even
  though its decoded RGB pixels match those packages. Retail trace proves
  indexed textures consume CLUT alpha and Theatre does not take the `_tb`
  branch. Runtime now reconstructs the matched sibling mask only for a fully
  opaque image used by an alpha-blended `MultiMesh0` template; there is no
  venue/texture/material/mesh-name rule and unrelated indexed textures retain
  authored CLUT alpha.
- All 15 input-free runs reach `playing`, exit zero, and report zero decode
  errors or manufactured floor rows. Focused converter and scene tests pass;
  the broad gameplay contract has zero crowd-source failures and 64 unrelated
  stale baseline failures.
- Active deployment executable SHA-256:
  `5B3326A2BAE2F69EC3158FAC172DF2CD1A2384046B45287C74319D5D7A827567`.
- Fresh Theatre proof submits 500/500 cards, applies 25,220 transparent
  silhouette pixels, and shows no rectangular card backgrounds. The Arena
  authored-alpha control does not activate reconstruction.
- Baseline source/proof commit: `9d11a2d8` (`Restore source-authored venue
  crowds and floors`); Theatre follow-up commit: `e108c314`
  (`Restore GH1 Theatre crowd silhouettes`).
- Documentation: `GuitarHeroOGX-main-ui-engine/docs/CROWD_AND_FLOOR_SOURCE_PARITY.md`.
  Proof: `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/crowd-floor-source-contract/`.

### 2026-08-01 RB2 instruments: v10 face-forward batch and live store/gameplay proof

- Final build: `rb2_wii/batch_build/rb2_retail_native_v10` (59 guitars,
  33 basses). The native audit passes 92/92 packages, 196 required meshes,
  129,289 body triangles, and zero errors.
- The converter now resolves each RB2 primary mesh through its complete
  authored parent/root chain, subtracts translation only, and converts body
  parts, strings, and hand/fret targets in a canonical face-forward frame.
  This corrects the 90-degree edge-on roll on the Neon, Prefish, and Skeletar
  guitar/bass packages without changing their accepted anchor or scale.
- `audit_rb2_native_batch.py` now rejects a primary body when its X face width
  is not greater than its Y thickness. All 92 v10 packages pass.
- Multipart prop children use `scene_object_world` in
  `engine/src/character/char_renderer.cpp`; this retains the valid partial
  transform chain when it terminates at a non-`RndTransformable` Character
  root. Neon detail meshes therefore remain assembled in gameplay.
- Catalog audit passes with 92 imported models, 24 GH2 models, 27 GH2 skins,
  116 store models, 92 friendly model names, 92 `Standard Finish` names, and
  92 descriptions. Imported prices remain `floor(RB2 cost / 2)`; GH2 model
  and skin prices are also halved. There is no uniform price.
- Live retail proof purchased the **Fender American Vintage 1962 Jaguar** for
  `$499`, persisted it across processes, equipped it on Player 1, and loaded
  it in gameplay. The active profile also owns the **Fender American Vintage
  1962 Precision Bass** (`$999`) and **Neon Dream Bass** (`$27,000`).
- Branded bass baseline:
  `proofs/rb2-store-gameplay/precision-bass-v10-face-forward-gameplay.bmp`.
  Multipart/root-rotation regression:
  `proofs/rb2-store-gameplay/neon-bass-v10-face-forward-multipart-gameplay.bmp`.
  Both use the Player 2 bassist role, bassist-anchored camera, proof lighting,
  and autoplay.
- The complete 95-file v10 overlay is deployed. `ark_tool verify` reports an
  exact v3 archive, and
  `rb2_wii/batch_build/rb2_retail_native_v10/active_readback_audit.tsv`
  records 95/95 byte-identical active readbacks. The active UI archive boot
  test passes.
- Shareable procedure and evidence index:
  `rb2_wii/RB2_INSTRUMENT_BATCH_CONVERSION.md`.
- The unrelated stock singer intermittent freeze remains an open TODO in
  `GuitarHeroOGX-main-ui-engine/engine/src/game/PERFORMER_ANIMATION_LIFECYCLE_TODO.md`.

### 2026-07-31 approved Telecaster conversion baseline

- The first `rb2_guitar_telecaster01` gameplay run was rejected. Placement was
  accepted, but the RB2 string plane stopped mid-neck and caused the body
  behind it to disappear; the white pickguard was also missing.
- Root causes were concrete: strings were fitted to the stock SG string
  envelope instead of the converted body envelope; the prop renderer ignored
  the MILO `outfit0_lod*` body-before-strings group order and used raw archive
  order; the qualified string material had lost RB2 alpha-cut; the converter
  flattened the diffuse alpha before restoring fixed-color detail.
- `telecaster_fix_v3` now shares the exact body fit for strings, preserves
  source cull/alpha-cut, restores fixed detail (the Telecaster pickguard), and
  the renderer consumes group-authored prop draw order. The approved placement
  is unchanged.
- Active package/readback SHA-256:
  `6EECBB1E0D9C8302CC776CD7F243D0463B6A9E39015C0EBFCCC94841A8648EA7`.
  Active executable SHA-256:
  `F20115231C8767E221EAA3093BB4614A9BF2792E4BD9B2DC64459ECC5D1BAAAF`.
- The native window class now has a black background brush so startup/stall
  repaint is black instead of the system white window color.
- A visible autoplay test ran with converted Clive, Telecaster, GH1 Festival,
  `--diagnostic-front-camera guitarist0`, and proof lighting. The runtime
  reached 135 hits with zero misses and the user approved the Telecaster.
- Full human-readable defect/fix/command documentation is in
  `rb2_wii/RB2_INSTRUMENT_BATCH_CONVERSION.md`. These rules are now the
  generated-instrument baseline and should be applied to the full batch.

### 2026-07-31 RB2 instrument batch correction

- Live inspection rejected the batch Fender because the converter had treated
  an early raw-body extraction directory as the qualified reference. The
  actual accepted Fender is the 147,010-byte package at
  `rb2_wii/output/drop_in/char/og/guitars/gen/guitar_sg.milo_ps2`, SHA-256
  `95E5B2C4FB68937E9E1B4402DB57FE4B419B0473A7F385BE477F86EEC4C808B6`.
- `convert_rb2_instruments.py` now follows the documented fitted-body path,
  restores the rendered-assembly `-0.45` toward-character offset while leaving
  hand targets fixed, and uses the exact qualified Fender string material plus
  both qualified string textures for every instrument. The Fender catalog
  entry itself receives the exact qualified package bytes.
- Corrected full build `rb2_wii/batch_build/rb2_retail_native_v8` contains 92
  items (59 guitars, 33 basses). Offline audit reports zero failures for 92
  package hashes/prices, 91 generated placements, 100 opaque encoded body
  images, and 276 qualified string records. The full 94-entry overlay is now
  deployed; `ark_tool verify` is exact and all 94 active-ARK readbacks match
  their source SHA-256 values.
- Human-readable process and acceptance gate:
  `rb2_wii/RB2_INSTRUMENT_BATCH_CONVERSION.md`.
- The CLI proof-light switch is now applied before `--auto-start` constructs
  performers, uses neutral per-material lighting, omits venue-only guitar
  shadow/fire helpers in proof mode, and the front lock uses a wider view.
  Automated GUI relaunch became unavailable when the Codex execution-credit
  limit was reached; final live reinspection remains required.

### 2026-07-31 converted Clive/Fender/Arena full-song qualification

- The converted GH1 Clive (`classic`) + converted RB2 Fender (`guitar_sg`) +
  converted GH1 Arena (`gh1_arena`) stack completed a visible, normal-camera
  Expert autoplay of `Shout at the Devil`. The user confirmed Clive was
  working correctly.
- Final score was 178,082 with a 518-note streak, zero misses, and zero
  overstrums. The trace contains 220 Clive samples, 14 distinct performance
  clips, 10 distinct walking clips, seven completed walks, zero expired walk
  requests, zero Clive duration overruns, 28 camera sweeps, and 25 distinct
  camera targets.
- The full-body freeze was corrected by continuing authored guitarist
  performance group nodes rather than a single selected clip. The sliding
  correction gives active CharWalk temporary main-pose ownership, and the
  converted Arena now supplies a connected three-node guitarist route.
- Deployed executable SHA-256:
  `3F740BC3C1E707939DBF04209096B22B53D1EE00ED712247EF3401EED50C6C0E`.
  Rebuilt/read-back Arena chars MILO SHA-256:
  `CA838299F0E2AD48A33F71C71A434BEF92763F59406BB0C67A8A9BE7C6F357A8`.
- The stock `metal_singer` visibly clamped until a later band event. Per user
  direction, this is separate open work in
  `GuitarHeroOGX-main-ui-engine/engine/src/game/PERFORMER_ANIMATION_LIFECYCLE_TODO.md`.
- Shareable procedure and evidence:
  `GuitarHeroOGX-main-ui-engine/docs/CONVERTED_CLIVE_FENDER_ARENA_QUALIFICATION.md`
  and
  `GuitarHeroOGX-main-ui-engine/proofs/full-conversion-stack-visible-final/`.

### 2026-07-30 Career character/outfit menu fidelity

- The two user-supplied retail screenshots are successive states of the same
  packed GH2 panel: hero select, then the 0.4-second `sel_skin.tnm` sweep into
  outfit select.
- The right outfit wall is `cs_wall2.mesh` using the same `cs_wallhalf.tex` as
  the left. Stock authors both a negative-X transform and reversed source
  triangles. Native culling previously used only transform handedness and
  removed that wall; it now combines decoded face parity and transform parity.
  The rule is generic and contains no object or panel exception.
- A renderer bug mixed LocalXfm-built glyph vertices with a serialized
  WorldXfm animation bind for labels whose transition bind began just off the
  normal text plane. `sg_selectyouroutfit.lbl` was the visible casualty.
- Labels now resolve their real MILO ancestor chain and packed TransAnim
  targets. Animated chains use the authored world bind; static off-plane
  labels retain the existing local fallback. There are no panel, label, or
  character name exceptions.
- Build and the focused label/cull tests pass. Hidden forward and reverse menu
  runs prove the hero, transition, outfit, and restored hero states. The
  corrected settled wall is in `fixed-source-winding/outfit-right-wall.bmp`.
- Packed `outfit1.btn` and `outfit2.btn` are `helveticablack` BandButtons
  directly under `text_skin.grp`. Retail black is now scoped to that exact
  source identity; `helveticablack.mat` remains its authored white, so other
  menus using the typeface are unaffected.
- Outfit descriptions are variant-source-driven. Native GH2 selections use
  stock `<character>_outfit_blurb` copy when present; added GH1/GH80s variants
  are blank. Missing stock outfit tokens remain blank instead of falling back
  to the character biography.
- Focused font, label, catalog, and localization checks pass. Native and
  imported proofs are in the `outfit-corrections` evidence folder.
- `gh2_ps2_hybrid_assets/ghogx_app.exe` is deployed and byte-matches the
  verified build at SHA-256
  `E555728866DEBDEF9C5435CABD6E639AB0E4D14B82C5FF867B1881E9E9C99770`.
- Contract and evidence:
  `GuitarHeroOGX-main-ui-engine/docs/CHARACTER_SELECT_MENU_FIDELITY.md` and
  `GuitarHeroOGX-main-ui-engine/proofs/campaign-character-select-right-background/`.

### 2026-07-30 project-authored character outfit names

- The 17 user-supplied GH1/GH2/GH80s outfit names are versioned in
  `GuitarHeroOGX-main-ui-engine/config/character_variant_labels.tsv`.
- `build_character_variant_overlay.py` applies that data only after deriving
  the 33 exact source variants. Every naming row must match the derived
  canonical character, selection symbol, and source game; unknown, duplicate,
  or stale identities fail generation. There are no runtime character-name
  branches.
- The overlay was regenerated and applied directly to
  `gh2_ps2_hybrid_assets/gen`. The naming deployment reused 64 entries and
  replaced only `config/gen/character_variants.dtb`; a repeat application was
  fully idempotent (`reused=65 replaced=0 added=0 appended=0`).
- Deployed ARK readback matches all 17 labels, the complete 33-variant
  model/animation/door audit passes, and an input-free UI capture shows Punk's
  fourth outfit as `80'S PUNK` with wrapped `ATOMIC`.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/campaign-character-select-label-catalog/`.

### 2026-07-30 campaign character-select door correction

- Stock `career.dtb` binds `sel_character.milo_ps2::cs_door.mesh` through
  `CharsysPanel::set_door`; stock character UI clips drive it through
  `bone_door.rotz`. The panel MILO has no independent door TransAnim.
- The first implementation/proof was rejected because it applied the typed Z
  rotation directly to the panel mesh, leaving the door edge-on as a gray slab.
- Retail `SLUS_214.47:0x00142420..0x00142464` proves the actual Poll bridge:
  read character `bone_door` Euler Z, force X=`pi/2` and Y=`0`, rebuild the
  external panel door Matrix3, and retain the panel mesh's authored
  translation. `set_door` itself only retains the indexed object pointer.
- The runtime now preserves the indexed external door binding and reproduces
  that exact `MakeRotMatrix(pi/2,0,Z)` bridge. Variants without a door channel
  use a catalog-authored `ui_loop` open Z pose through the same bridge; there
  are no character-name branches, mesh-hiding rules, offsets, or guessed
  angles.
- The deployed catalog audit passes all 33 variants and classifies 25 direct
  door drivers plus 8 authored-open fallbacks. Its numeric contract also
  checks the forced quarter-turn rows.
- Numeric and visual evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/campaign-character-select-door-corrected/`
  and `campaign-character-select-door-gh1-fallback/`.
- Deployed executable SHA-256:
  `3085B433D59B9BD764626F1BE8EA2AADC38E59C36E459DD4119FBEA3F8AC61C7`.

### 2026-07-29 source-exact GH2 fret-target contract

- Visual inspection is not the fret/arm-target acceptance oracle.
  `GHOGX_DEBUG_ARM_CONTRACT=1` records owner/controller/target/parent/spot,
  parser and task beats, transition fraction/rate/easing, and full-float
  start/spot/desired/applied transforms.
- Converted `CharIKMidi fret.ik` targets the GH2 instrument-owned
  `bone_fret.mesh`. The shared pose-target resolver now handles skeleton bones,
  character meshes, and resident attached-prop transforms; there is no
  character, instrument, bone, pose, or venue rule.
- GH2's inverted `player*_fret_pos` parser retains dense events and applies
  authored `min_gap 0.22` in beats. Runtime follows the recovered PS2
  `NewSpot`/`Poll` remaining-beat rate, clamp, half-cosine easing, and
  quaternion-sign-corrected normalized transform interpolation.
- A hidden, input-free raw/native matrix covers all eight GH1 guitarists.
  Its independent verifier accepts 80/80 events with zero failures, maximum
  component delta `6.7e-6`, and at most seven binary32 ULPs in the separately
  gated world/local/world application round trip. A six-ULP negative control
  correctly rejects the two copies of the seven-ULP row.
- Six regenerated all-GH1-band main-executable videos pass their owner-tagged
  runtime matrix for Classic, both singers, bassist, drummer, and keyboardist.
  Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/`
  `matched-arm-chain-differential/` and
  `instrument-hand-parity/arm-twist-isolation/strict-native-gameplay-current/`.
  Deployed executable SHA-256:
  `E0EA25C03710A3638C2F634A47E49B3B1EE62EA2C2E9F5511FBFFB384802A047`.
- Current progress: format/conversion 100%; GH1 venue parity 96%; character
  parity 92%. Fret target lookup/timing/interpolation is machine-closed;
  matched retail/user visual acceptance and broader character presentation
  remain open.

### 2026-07-29 matched retail/native arm-twist numeric closure

- Visual inspection is no longer the twist-controller acceptance oracle.
  `GHOGX_DEBUG_TWIST_CONTRACT=1` emits exact-float source, authored-basis,
  parameter, and output rows without changing behavior.
- Fresh bounded GH1 disassembly corrects a stale interpretation:
  `AnimServoUpperTwist::Poll` loads exact `-0.5` and publishes the same
  source-based half-twist row to both packed siblings. GH1 and GH2 fore polls
  both use exact `0x3EAAAA9F` for the one-third correction.
- The raw revision-10 helper, focused source test, source-truth contract, and
  documentation now use those source facts. No identity-specific rule was
  added.
- Eight hidden, input-free, strict-native runs on the exact deployed
  executable cover all 13 GH1 performers and reach `state=playing`. An
  independent log-only verifier recomputes 6,000 events: 4,800 upper sibling,
  zero upper serial substitutions, 1,200 fore, and zero failures at `5e-6`.
  Maximum output-transform delta is `1.28159703e-7`.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/`
  `matched-retail-twist-differential/`.
  Deployed executable SHA-256:
  `1CB61282BC035F097A712AEA98F9DCA492B32E513BD5EC7087995145F47F0EA7`.
- Current progress: format/conversion 100%; GH1 venue parity 96%; character
  parity 91%. Twist formula/branch selection is closed; broader arm/character
  presentation remains open.

### 2026-07-29 source-to-live GH1 performer formation closure

- Retail GH1 static analysis and read-only runtime anchors prove Arena and
  CharSys consume the same `0x40`-byte walk/stage transform records from the
  Arena owner at global `0x00363748`. The four converted role mappings are
  therefore source-derived, not visual placement guesses.
- `milo_convert_audit` now emits a 28-row venue-placement ledger containing all
  raw source and emitted target transform values. `milo_convert_test` verifies
  the four mappings, flags, basis normalization, translation retention, and
  serialized native Waypoints.
- The main executable's opt-in `[world-start-ledger]` row records selection
  source, Waypoint, three flag masks, and all 12 live transform values at
  round-trip-safe precision.
- A hidden, input-free, all-GH1 strict-native sweep reaches `playing` in all
  seven venues. All 28 live transforms have zero float-bit mismatches against
  their packed source-derived Waypoints, and all 42 role pairs reject exact
  translation overlap. Minimum venue separation is 73.707 source units.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/formation-transform-contract/`.
  Deployed executable SHA-256:
  `99770559E306C80E4E950E3036AC7B0237BE8F153C4D3231F5EDF2C75E4603DC`.
- Current progress: format/conversion 100%; GH1 venue parity 96%; character
  parity 91%. Start formation is closed; matched retail pose/animation and
  remaining venue presentation stay open.

### 2026-07-29 GH1 VenueCam target and screen-coordinate recovery

- Read-only GH1 savestate and `SLUS-21224:0x0016E080` tracing proves normal
  single-player VenueCam target index zero resolves ArenaSinger slot zero, the
  player guitarist. The historical `singer_in/out` field names do not select
  the vocalist.
- The selected ArenaSinger vtable entry adjusts to the owning object and calls
  `SLUS-21224:0x0018D3C0`, which looks up `bone_head.mesh` and returns its
  world transform. The earlier spine interpretation was wrong; the exact head
  anchor explains the previously unexplained retail/native subject-height
  delta without a camera or character offset.
- `SLUS-21224:0x0016E548..0x0016E5BC` proves endpoint progress is the selected
  source path frame normalized between the lower and upper `start/end`
  frames. Forward traversal is `in` to `out`, reverse is `out` to `in`, and an
  equal range resolves immediately to `out` even with nonzero task duration.
- The offline converter and both compatibility adapters now retain
  `guitarist0:bone_head.mesh`, direct centered screen coordinates, and the
  native path-frame endpoint progression. No venue, shot, performer, mesh, or
  field-value exception was added.
- The full clean sweep passes 105/105 MILOs, 926/926 ACPs, 201/201 VenueCam
  records, 191,320 generated camera keyframes, 762 semantic field rows, and
  zero blockers. Consecutive clean conversions match all 124 payloads,
  39,275,873 bytes, at ledger SHA-256
  `819FCE3A740A4E7619C493653984A507D0F08EB320D40FE630A9979F06FB287B`.
- All seven regenerated venue MILOs and the rebuilt app are in the primary
  deployed archive. Archive extraction matches every loose payload, the
  immediate second overlay appends zero bytes, and the header verifies exact.
- The earlier matched Basement trace exposed a height delta only because the
  provisional native target was the spine. The exact retail head target
  removes that false character-placement inference without an offset.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/venuecam-target-screen-contract/`.
  Deployed executable SHA-256:
  `A01E6586977197A5656242CD96857802F487B6441149A0FF79386717F9E72DB9`.
- Current progress: format/parity 100%; venue visual parity 96%.

### 2026-07-28 GH1 View environment scopes and Arena-helper regression

- A GH1 revision-7 View is an ordered drawable/state stream. Retail Small Club
  `lighting_transparent.view` changes from `Environ2` to `smoke.env`, and its
  nested lighting rig changes through orange, pink, and red Environs. The old
  converter flattened the entire View under its first resolved Environ.
- `milo_object` now resolves ordered environment segments. `milo_convert`
  emits deterministic `<view>.__environment_<index>.grp` children and a
  `<view>.__draw_only.grp` traversal container; the flat helper rejects
  multi-scope input. Tests cover segment order, child environments, traversal
  order, and rejection.
- The clean packed audit remains 105/105 MILO assets, 926/926 ACPs, 13,115
  converted objects, 793 synthesized objects, zero blockers, 33 venue assets,
  14,696 venue references, 201 cameras/181,864 keyframes, and 172/172 exact
  DTB/SEQ round trips. The semantic ledger is now 762 rows: 603 source and 159
  synthesized.
- Source and converted `smoke.tex` decode to identical RGBA bytes
  (`4b65c5d20601268485446b1699d622d79827495e81a76a357ba9e7a0534cae90`).
  A deterministic A/B proves `smoke_plane01.mesh` supplies the textured golden
  haze; it was not the solid red block.
- The red block is source-proven `target_parent.mesh`, a temporary GH1 Arena
  helper. Its previously recovered generic helper classifier had been removed
  from both venue and lighting base-hidden paths by current uncommitted work.
  The classifier is restored for target-parent, numbered stage/walk/fire/name
  light spots, and zero-based crowd-limit families. No Small Club or visual
  exception was added.
- The deployed fixed camera retains the amp and smoke while removing the
  helper. A fresh strict-native sweep reaches gameplay in all seven GH1 venues
  at 57.269--57.730 steady FPS. Deployed executable SHA-256:
  `351ABFFA760EBD17A31A8EEB263789027C0130B9ABCE84003D306A65209CCC57`.
  Evidence:
  `proofs/gh1-native-conversion-parity/view-environment-scope-contract/` and
  `proofs/gh1-native-conversion-parity/venue-parity-matrix/final-view-scope-helper-lifecycle/`.
- Current progress: venue parity 94.5%; overall format/parity goal 99.0%.
  Matched retail GH1 camera/time comparison, Small Club panel acceptance, and
  front/rear hanging-record retail proof remain open.

### 2026-07-28 native GH2 guitarist group handlers

- Static GH2 PS2 recovery proves guitarist chart `play` is handled by native
  `BandCharacter::Handle`, not by a missing DTA row. The dispatch range
  `0x0010C9E0..0x0010CB7C` maps play/idle/wail-on/solo-on to
  normal/idle/extreme/solo through `0x0010B7F8`; wail-off and solo-off restore
  normal through `0x0010C5B8`.
- The decoded character type owner now exposes those native class messages to
  the same generic `main.drv` group bridge. Focused synthetic and real Glam1
  type-program tests exit zero.
- A 600-frame hidden, input-free, strict-native Small Club run resolves guitar
  `[play]` to source-authored `stand_fast_02`, retains the corrected GH1 drum
  no-snare selection, reaches `state=playing`, and reports zero unresolved
  driver requests, handler failures, or unhandled Character messages at
  59.393 steady FPS.
- A deployed-archive type-owner sweep covers all 13 converted GH1 model
  packages. Every decoded class/type enter and native BandCharacter group
  check exits zero with no unhandled messages; optional peak WorldFx checks
  are gated by actual package ownership.

### 2026-07-28 target-only GH2 band animation domains

- Packed GH1 `macros.dtb` and `band_anims.dtb` prove the drummer domain has
  normal/all-beat/double/half but no no-snare member. Stock GH2
  `char_objects.dtb` maps `[nobeat]` to `kBandNosnare`, and its native drummer
  clip set proves that bit is `0x200`.
- Conversion now identifies a drummer clip set structurally by the presence
  of all three source all-beat/double/half flag families. Its normal active
  clips gain target `kBandNosnare`; source samples and all existing flags are
  retained. No asset or clip name participates.
- The regenerated full bundle remains complete: 105/105 assets, 926/926 ACP
  clips, 13/13 character models, 39 animation packages, and zero blocked
  conversions. Deployment replaced only the changed drummer package.
- A 600-frame hidden, input-free, strict-native GH1 Small Club run with GH2
  `shoutatthedevil` resolves `[nobeat]` to source-authored
  `drummer_active_fast_normal` at target flags `0x206`, reports no unresolved
  driver request, reaches `state=playing`, and sustains 60.010 steady FPS.
  Evidence is under
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/`
  `charclipdriver-authored-owner/`.

### 2026-07-28 exact GH2 PS2 CharDriver outer Poll

- Retail `CharDriver::Poll` is recovered at
  `0x00171830..0x00171C64`, including the PS2 stack-starved predicate,
  integer-beat realignment, optional starved-event dispatch, graph-loop
  replay, saved-`DataNode` node-loop replay, Evaluate, and ScaleAdd order.
- The verified owner fields are head `+0x38`, starved-event symbol `+0x3C`,
  saved node `+0x40`, old beat `+0x48`, `realign` `+0x4C`, and beat scale
  `+0x50`. `CharClipDriver+0x2C` is the clip-reference event owner.
- Native exposes synchronous enter/exit/beat-event owner callbacks, keeps
  lossless crossed-event rows, and does not invent a current-clip fallback
  when a saved node cannot be resolved.
- The final serialized `CharDriver3` byte is corrected from `enabled` to
  `realign`. All 13 converted GH1 models/30 drivers audit cleanly; only
  `metal_drummer/main.drv` and `metal_keyboard/main.drv` set it.
- Driver-flags, source-truth, and left-hand tests pass. Fresh hidden,
  input-free strict-native Small Club/Arena runs reach `playing` at
  60.017/59.935 steady FPS. Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/`
  `charclipdriver-outer-poll/`.
- Deployed `gh2_ps2_hybrid_assets/ghogx_app.exe` SHA-256:
  `8116381CB275177B0D6DA858BE3EDD7CE6A143BE10B8B4FF68FBB8D2DA0F91BB`.
- Remaining driver boundary: live saved-`DataNode` and script-owner binding
  at authored call sites, followed by the full matched character matrix.

### 2026-07-28 exact GH2 PS2 CharClipDriver node core

- `SLUS_214.47` static recovery now covers the complete
  `CharClipDriver` constructor/copy/destructor/Evaluate/ScaleAdd/alignment/
  event-cursor range at `0x00198660..0x00199084`, including its 0x3C-byte
  layout. The copied `+0x38` word remains explicitly opaque because none of
  those routines assigns or consumes it.
- Native construction now matches sentinel versus explicit starts,
  zero-fraction inherited-stack stripping, mode-2 deletion, authored
  transition/fallback selection, explicit ramp state, mode-8 epsilon, and
  zero-width completion in Evaluate. Blend overrides are no longer clamped.
- The PS2 range-randomization dependency is recovered through
  `0x002D9B10..0x002D9DC8`: fixed seed 666, 256 seeded words, cursors 0/103,
  PS2 modulus 249, low-16 `1/65536` float conversion, and half-clip phase
  wrap. It remains separate from the already-recovered XEX modulus-250 state.
- Crossed beat events are surfaced losslessly with clip/index/beat/symbol, but
  are not dispatched through a guessed script owner.
- Driver-flags, source-truth, and left-hand tests pass. Fresh hidden,
  input-free strict-native GH1 Small Club and GH2 Arena runs both reach
  gameplay with four GH2-layout performers at steady 59.961/60.043 FPS.
  Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/`
  `charclipdriver-exact-core/`.
- The release app is deployed to `gh2_ps2_hybrid_assets/ghogx_app.exe`;
  SHA-256
  `97F07E4FFA452E483FEB0E5C94A5318EB1B58DC4CF582E1338BFB92FD3E03439`.
- Remaining driver boundary: exact outer `CharDriver::Poll` ownership for
  default/graph/node loops, starvation, clip groups, and beat-event execution,
  followed by the full matched character matrix.

### 2026-07-28 full 15-venue frame-pacing closure

- The hidden, input-free, strict-native performance matrix now passes all
  seven converted GH1 venues and all eight native GH2 venues with one GH2 song
  and four GH2-layout performers.
- All 15 runs exit zero, reach `playing`, produce four gameplay profile
  windows, and report `max_init_ms=0.000`. Steady cadence is
  58.785--60.201 FPS (16.611--17.011 ms), sampled render time is
  6.734--15.136 ms, and sampled D3D9 present time is 0.241--0.452 ms.
- The original 33.241--58.242 FPS spread came from stacked D3D9/application
  waits. D3D9 is now immediate and one accumulated 60 Hz application deadline
  owns cadence. Shared static-buffer, authored-hierarchy/world-matrix, and
  consecutive environment-state caches exclude all dynamic/skinned paths.
  Disconnected XInput slots are probed every 15 frames while connected devices
  remain polled every frame.
- No venue, mesh, material, object, or character-name exception was added.
  Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/`
  `frame-pacing-full-matrix/`.

### 2026-07-28 CharWalk regulator closure and deployed visual proof

- GH2 `RegulateWalk` is now recovered through its PlanPoint distance
  correction: `mCurPoint=+0x6C`, `mLastPoint=+0x70`, PlanPoints `=+0x74`,
  `mOffsetSpeed=+0xC74`, and beat remainder `=+0xC78`. Passing a PlanPoint
  compares real remaining waypoint distance with authored remaining plan
  distance, divides the error over at most two points, and moves toward
  ForwardPredict by the normalized frame-delta-scaled offset speed.
- The stop chooser's `0x00186934` store proves the selected stop point replaces
  the provisional overshoot count. Native now mirrors that write.
- Focused clip-plan, source-truth, and gameplay-rule tests pass.
- A hidden, input-free, native-only Arena run completes
  `walk_right.way -> walk_center.way -> walk_left.way` with one authored turn,
  four repeated walk phases, one stop, seventeen PlanPoint advances, and final
  destination handoff. The fixed source-authored camera video and compact log
  are in
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/`
  `charwalk-motion-plan-complete/`.
- The release app is deployed to `gh2_ps2_hybrid_assets/ghogx_app.exe`;
  SHA-256
  `5037C2DCB7F070E7634ED0C13362534AC67892890867D561B117F62FBE464346`.
- Remaining CharWalk boundary: complete live `CharClipDriver` ownership across
  the full character matrix and matched retail comparison.

### 2026-07-28 exact Waypoint graph and CharWalk motion plan

- GH2 Waypoint revision 3 is decoded through ObjectFields0, Drawable3,
  Transformable9, flags, source-order connection ObjPtrs, radius, Y radius,
  and angle radius with zero residual bytes. Live CharWalk uses the decoded
  graph: exact registry-order nearest selection, source-order DFS with blocked
  intermediates, and successful-destination registry rotation.
- The start type scripts and CharWalk now share the same mutable Waypoint
  registry. Arena's exact graph is
  `left -> center -> right/solo -> stage`, including neutral traversal nodes.
- The retail motion plan is implemented generically: authored walk
  self-transitions, one-beat cumulative-distance PlanPoints, forward/back clip
  prediction, reverse stop-start prediction, integer-beat parity, two-point
  search, strict source-order stop scoring, decoded waypoint-radius endpoint
  adjustment, and turn/walk-repeat/stop playback.
- The signed corridor regulator now consumes those predictions live. Static
  analysis corrects `Character+0x23C` to the active TaskMgr frame delta (also
  consumed by the clip driver), not `CharServoBone::mMoveSelf`.
- Focused Waypoint, route, type-script, clip-plan, stop, ForwardPredict,
  BackPredict, turn-score, and mirrored corridor tests pass. The rebuilt app
  and source-truth contract pass. Still open in this slice: later
  plan-distance positional smoothing, complete live CharClipDriver ownership,
  deployed multi-segment proof, and documentation/proof packaging.

### 2026-07-28 native CharWalk target configuration and converted groups

- The target type compiler now retains data-only members and reads stock
  `BandCharacter/guitarist` walk delays, waypoint mask, maximum wait, and
  `CharWalk/guitarist.path_radius`: `{off,off,35..55,20..40,off}`, `0xC0`,
  `6`, and `12`.
- GH1 guitarist family membership is represented by GH2
  `CharClipGroup`, so the converter now removes the derived GH1 membership
  mask `0x063FC0E0` from clip flags after group construction. This fixes
  collisions such as GH1 `kGuitarBad=0x4000` versus GH2
  `kStarPowerFar=0x4000`.
- All eight guitarist-main packages pass a 623-clip sweep with zero masked
  bits and one `walk_turn`, `walk_walk`, and `walk_stop` group each. The eight
  corrected packages are deployed. Extracted deployed Metal flags are
  turn/stop `0x00C00802` and walk `0x00C02802`.
- Hidden, input-free native-only runs pass in converted GH1 Small Club and
  stock GH2 Arena. Both load 20 turn, 16 walk, and 16 stop clips. Arena exposes
  three matching waypoints; Small Club exposes one.
- Focused converter, type-script, and source-truth tests pass. The animation
  loader now keeps one decompressed MILO cached while loading a group's clips.
- Still open: completed walk-phase proof, exact transition/path-regulation
  math, and live CharClipDriver ownership. Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/`
  `native-charwalk-controller/`.

### 2026-07-28 GH2 live CharWalk controller recovery

- Retail `SLUS_214.47` static analysis now recovers the outer
  `CharWalk::Poll` state machine at `0x00184B10..0x00184CC8`, start/setup,
  prediction/regulation, authored `walk_walk`/`walk_turn`/`walk_stop`
  selection, plan-point construction, and waypoint direction update through
  `0x001872F0`.
- The accesses corroborate ihatecompvir's `0xCB0` layout and exact
  `None=0`, `Going=1`, `Stopping=2` enum. Native now keeps that state separate
  from turn/walk/stop clip phase. GH2 `actually_walking` dispatch at
  `0x0010CD88` calls `CharWalk` `0x00184FD0`; camera gating now requires both
  active state and a concrete current walk clip.
- Focused source-truth test, release build, and hidden input-free Small Club
  runtime pass. The larger venue/band omnibus still reports unrelated
  pre-existing contract failures.
- Fully typed path-regulation equations and live CharClipDriver ownership
  remain open. Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/`
  `charwalk-runtime-contract/`.
- Small Club hanging records remain separately open for matched retail GH1
  confirmation of front/rear visibility, source alpha masking, and the paired
  `record*.1.mesh` surface role. No speculative fix is authorized.

### 2026-07-28 GH2 character start/waypoint script contract

- Retail/source evidence now distinguishes serialized `CharWalk1` from the
  global Waypoint start-selection path. `waypoint_find` returns the first
  construction-order flag match; GH2 retail `waypoint_last` at `0x00191160`
  moves the selected registry node to the end.
- The character type-script host now executes `waypoint_find`, native
  `Character::teleport`, and `waypoint_last`, preserves the decoded source
  waypoint index, and identifies the live character object by its world role
  rather than its asset root. Stock Glam1 therefore requests flag `1` as
  `guitarist0`, not multiplayer guitarist flag `2`.
- A hidden current-build Small Club run routes guitarist, singer, bassist, and
  drummer through `source=type-script`. Focused synthetic and stock Glam1
  audits pass. `waypoint_nearest` remains fenced; the later CharWalk section
  records the separately recovered live-controller boundary. Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/`
  `waypoint-type-script-contract/`.

### 2026-07-28 stock GH2 character opaque-row closure

- The runtime character graph now promotes the exact target readers already
  used by the offline round-trip layer for `OutfitLoader1` and `CharWalk1`.
  OutfitLoader is backed by retail `SLUS_214.47` save/load traces at
  `0x0018AEC8` and `0x0018B170`; CharWalk1 is revision 1 plus its complete
  ObjectFields0 row with no remaining payload.
- This is serialized format closure only. The missing live
  `CharWalk::Poll` movement algorithm and OutfitLoader switching behavior are
  still fenced rather than fabricated.
- Current stock sweeps pass with zero decode failures and zero opaque rows:
  24 base performer packages contain 20 OutfitLoader, 19 CharWalk, and 87
  WorldFx rows; 13 explicit crowd packages add 13 WorldFx rows.
- Decoder, synthetic/real type-script, source-truth, bind-audit, and release app
  targets pass. Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/`
  `stock-character-opaque-row-closure/`.

### 2026-07-28 GH2 WorldFx format and performer runtime contract

- All observed GH2 WorldFx rows are revision 1 followed by a complete RndDir8
  body. Native now decodes object-directory, transform, drawable, animatable,
  proxy, subdirectory, environment, parent, and draw-order state through the
  shared `milo_object` reader with exact residual accounting.
- Retail `SLUS_214.47` static analysis proves a separate live gate:
  `WorldFx::Poll` at `0x002725D0` checks object word `+0x98`, `Start` at
  `0x002726B8` sets it, `Stop` at `0x002728E0` clears it, and the handler at
  `0x00272BC8` dispatches exact `start`/`stop` symbols. Serialized drawable
  visibility does not initialize running state.
- One common crowd/performer runtime resolves proxy MILOs relative to the owner
  archive, merges authored visual subdirs, attaches to the decoded parent bone,
  updates the proxy scene, and respects decoded draw order. No asset-name rule
  participates.
- Hidden, input-free stock Glam1 proof resolves all five WorldFx proxies and
  visibly activates both hand flames through the authored guitarist
  `solo_on` + `peak_on` script path. Decoder, synthetic/real type-script, full
  source-truth, and app builds/tests pass. Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/`
  `worldfx-performer-contract/`.
- This closes the observed WorldFx subcase, not the full active
  format/conversion parity goal. Unrelated Character type-script handlers and
  the complete matched character/venue matrix remain open.

### 2026-07-28 authored WorldCrowd script/event contract

- GH2 `CharClipSamples10` enter/exit strings and counted beat-event rows are
  retained for MILO and ACP decoding instead of discarded.
- A clean packed crowd sweep finds 55/55 clips with authored enter events:
  23 clap, 14 fist, 11 devil, and 7 lighter. Clip spelling is not reliable;
  `male02_1armpump_02` explicitly requests `devil`.
- Runtime now preprocesses packed `char/gen/char_objects.dtb`, resolves the
  serialized `BandCharacter -> Character` superclass chain, and executes the
  exact `crowd` type enter handler and clip enter/exit event. The former
  clip-name hand selection and monkey-head rule are removed.
- Synthetic and deployed-retail-ARK contract tests plus the application build
  pass. The later shared WorldFx runtime now renders those start/stop states;
  see the newer section above. Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/`
  `worldcrowd-script-contract/`.

### 2026-07-28 decoded material visibility contract

- Venue rendering no longer suppresses `invisible.mat` or `ray_blocker.mat`
  by name.
- Packed Arena/Festival clip masks use decoded zero material alpha. Theatre's
  same-named material has alpha one and no mesh users; its 16x16
  `invisible.tex` instead has zero alpha in all 256 pixels and is referenced
  by a differently named material. No `ray_blocker.mat` appears in the
  seven-GH1/eight-GH2 main-venue sweep.
- Decoded material/texture alpha, blend, alpha test, culling, depth, and color
  are authoritative. Hidden input-free Arena/Festival/Theatre guards each
  reached playing state and emitted frames 2/120 with no exposed clip-mask or
  floor-gap regression.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/`
  `render-material-identity-contract/`.
- This closes one identity branch only. Matched GH1 venue parity and the Small
  Club hanging-record front/rear/alpha comparison remain open.

### 2026-07-27 GH1 View draw graph and Small Club record alpha contract

- GH1 View7 has separate animation/message and Drawable1 child lists. The
  converter now lowers animation-only Drawables through a GH2 Group13
  `draw_only` companion, recursively expands legacy Drawable child closures,
  preserves nested View boundaries, and holds authored-root-unreachable
  helpers in a hidden target Group. No venue/object name determines the rule.
- Full conversion audit passes 105/105 directories and 926/926 ACPs with zero
  blocked objects. Small Club raw/native submission matches exactly:
  main 181 meshes / 211 grouped and lighting 145 meshes / 241 grouped.
- Small Club `record*.mat` is `kBlendSrc`, `alphaCut=true`,
  `alphaWrite=false`, and `cull=false` (authored two-sided). Each 64x64
  indexed PS2 record bitmap has 814 zero-alpha, 130 partial-alpha, and 3,152
  opaque pixels. Raw and converted decoded textures are SHA-256 identical.
- Venue rendering now consumes decoded `RndMat.alphaCut`, greater-than alpha
  function, and decoded/defaulted threshold on every mesh. The black/square
  cards are gone without a black-key, record/material/mesh name, or venue
  exception.
- Native proof:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/`
  `venue-prop-layering-contract/native-after-alpha-cut-frame2.bmp`.
  Retail GH1 PCSX2 visual confirmation and user acceptance remain open.
- Shared hidden runtime regression passes all seven converted GH1 and all
  eight native GH2 venues at fixed-time frames 2/120: 15/15 zero exits, 30/30
  frames, no failed load, unsupported operation, or unresolved target. Contact
  sheets are under
  `proofs/gh1-native-conversion-parity/shared-alpha-cut-regression/`.
- The generated venue bundle is overlaid into
  `gh2_ps2_hybrid_assets/gen`; ARK verification is exact with 1,690 entries.
  A second clean conversion matches all 116 payload files and both manifests;
  both sorted path/file-hash ledgers hash to
  `49E0C3D6B14D0D0FA61C2189AF5F32F47E843AD0335764E8516473BD781281B3`.
  Continue the shared seven-GH1/eight-GH2 venue parity goal. Do not mark the
  overall format/parity goal complete.

### 2026-07-27 PS2 skinned-weight correction and final all-13 twist proof

- The previous attempt to apply the later `Hmx::Color32` in-memory rule to
  every pre-separate-color Mesh slot was wrong for GH1/GH2 PS2 skinned
  geometry. Original packed GH1 female-singer arm meshes contain signed and
  over-one slot values (`-0.78294009..1.80243146` across `fsing.18..21`).
  Those are raw extrapolative weights; low-byte wrapping destroys their
  cancellation and caused the rejected stretched polygons.
- The recovered PS2 branch now keeps the four serialized floats raw until the
  bone table resolves. Skinned meshes preserve them as weights and clear color
  to white. Only unskinned meshes apply Color32 and retain vertex color. There
  is no venue, character, mesh, material, texture, bone, or value exception.
- The source GH1 Mesh25 and converted GH2 Mesh28 female-singer arm ranges are
  identical. The Small Club unskinned vertex-color correction remains intact.
  The exact sibling/serial upper-twist branch remains topology-driven.
- Focused MILO-scene, gameplay, and character tests pass 55/55. Fresh hidden,
  input-free captures cover all eight GH1 guitarists and all five GH1 band
  roles. The labeled motion proof is 640x480, 10 fps, 260 frames, 26 seconds,
  decodes fully, and has SHA-256
  `99FCA280225E1132D4D34786DC6E9B790525B22117FCB6D6CAC5586C0C035713`.
- Current release/deployed executable SHA-256:
  `3EF2B5874189D825001D96B5D85ED7BA8514871C624DB91974BCB977FED6FAB2`.
  Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/`
  `instrument-hand-parity/arm-twist-isolation/`
  `gh2-ps2-dual-branch-final/`.
- Native Conversion Parity TODO 8 remains open for matched gameplay/user
  acceptance. Continue the overall format/parity goal; do not mark it
  complete.

### 2026-07-27 GH1 Small Club panels: pre-separate-color Mesh contract

- The light stone-like Small Club panels/trim are
  `main_room_stage.1.mesh` with `plaster_wall.mat`/`plaster.tex`.
  Raw GH1 Mesh25 and converted GH2 Mesh28 both contain 105 vertices, 94 faces,
  no bones, and identical serialized four-float channel ranges. Conversion
  did not lose the data.
- Mesh25/Mesh28 predate `MESH_REV_SEP_COLOR`. The PS2 runtime keeps the slot
  as four raw floats until the bone table resolves. It converts the floats
  through `Hmx::Color32` and retains them as vertex color only for an empty
  bone table; populated bone tables preserve the raw floats as weights and
  clear color to white.
- Native incorrectly treated every slot only as weights and forced all vertex
  colors white. The systemic decoder now makes the exact post-bone-table
  decision with no venue/object/mesh/material/texture/value rule.
- The same Small Club shot now renders the authored dark/baked panel result.
  Focused source-layout and combined MILO-scene/character/gameplay tests pass
  55/55. Fresh Metal and Hip Hop captures verify the shared skinned path did
  not regress the dual-topology twist fix.
- Release and deployed executable SHA-256:
  `3EF2B5874189D825001D96B5D85ED7BA8514871C624DB91974BCB977FED6FAB2`.
  Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/`
  `small-club-vertex-color-contract/`.
- Open GH1 Venue TODO 2 remains open until user visual acceptance. Continue
  the overall format/parity goal; do not mark it complete.

### 2026-07-27 second twist rejection: exact GH2 PS2 topology branches

- The user correctly rejected the native-Trans twist proof. This supersedes
  the section immediately below for twist acceptance and invalidates its old
  video/montage signoff.
- Root cause: conversion reparented GH1 `upperTwist2` beneath
  `upperTwist1`, and runtime applied serial-chain row factors to every
  `CharUpperTwist`. Object-level inspection proves GH1 and stock GH2 band-role
  packages use sibling twist1/twist2/upperArm rows, while stock GH2 selectable
  `metal1` uses the serial chain.
- Retail GH2 PS2 `SLUS_214.47` `0x001823C8..0x00182958` implements both.
  The parent-pointer comparison at `0x00182454..0x00182460` selects serial
  translated factors `(0.666,-0.333)` or the sibling fallback beginning at
  `0x00182670` with `(0.500,0.500)` from the live source basis.
- Conversion now preserves and validates the source sibling graph for 26/26
  upper-twist controllers across 13/13 packages. Runtime selects the formula
  from the resolved parent relation without character, role, side, mesh, bone,
  pose, or offset rules.
- All 13 packages and the release executable are redeployed. `ark_tool verify`
  reports revision 3, 1,690 entries, zero trailing bytes, exact. Source and
  deployed executable SHA-256:
  `070E45E430A19FCA8F24B2F539725082E64D8A9E473AB592440C8DED26A81782`.
- Converter tests and all 54 character/gameplay regressions pass. Fresh hidden,
  input-free package views cover all eight GH1 guitarists and all five GH1
  band roles. A labeled 26-second motion sweep covers all 13 current packages.
  Stock GH2 Metal1 separately logs the unaffected serial branch.
  Proof:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/`
  `instrument-hand-parity/arm-twist-isolation/`
  `gh2-ps2-dual-branch-final/`.
- Open Native Conversion Parity TODO 8 remains open for user review and
  matched gameplay/video acceptance. Continue the overall format/parity goal;
  do not mark it complete.

### 2026-07-27 user-confirmed twist rejection and native-Trans correction

- The user correctly rejected the prior twist checkpoint. The upper arms still
  formed wedges because `transform_local_chain_world_depth` preferred the
  pre-controller `runtime_pose_output_worlds` snapshot over the live resident
  transform after `CharIKHand`; `CharUpperTwist` therefore consumed a stale
  upper-arm row. Revision-24 converted characters also drew every authored LOD
  group because renderer group selection was incorrectly gated on revision 10.
- Runtime lookup now resolves explicit overrides, then the resident bone/mesh
  object, then a nonresident pose-output fallback. A focused regression pins
  that ordering. Authored top/LOD selection is now independent of directory
  revision. Matched raw/native post-controller upper-arm and twist rows agree
  to displayed precision.
- Recovered `Character::SyncObjects` source calls
  `ConvertBonesToTranses(this, false)` when exact `bone_pelvis.mesh` exists.
  Recovered `ShouldStrip` selects case-insensitive `bone_`/`exo_` and
  case-sensitive `spot_`. The systemic converter now rewrites each selected
  Mesh28 as Trans9 while preserving metadata, transforms, target, constraint,
  parent, names, and references.
- All 13 regenerated packages are deployed into the primary patch ARK.
  `ark_tool verify` reports an exact revision-3 header. The mounted GH1 source
  archive was restored to its original raw character packages after
  verification, so future conversion audits still start from GH1 data.
- Fresh input-free views load 63-78 native transforms for every selectable
  guitarist and 28-34 for male singer, female singer, bassist, drummer, and
  keyboardist. The all-eight guitarist and five-role captures have continuous
  twist chains. A labeled 39.65-second motion sweep covers all 13 packages.
  Focused twist and converter tests pass.
- Bass attachment is not universally closed. Raw and converted
  `bassist_idle@f60` both leave the hands down;
  `bassist_active_medium@f60` places both hands on the authored bass. An
  input-free native Small Club run at song time 6 consumes the authored
  `[play]` event, selects `bassist_active_medium`, and visibly puts both hands
  on the bass. Matched raw/native frame-60 captures now cover all five packed
  bassist families (Idle, Active Medium, Active Fast, Win, Lose); released
  contacts in the non-playing clips match GH1. The bassist subcase is closed.
- Source and deployed executable SHA-256:
  `D9D208234322FA4F7E3123FAA6318A60045502CB67763763C7E969768303F46D`.
  Proof:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/`
  `instrument-hand-parity/arm-twist-isolation/native-trans-overlay-fixed/`.
  Active items are Open Native Conversion Parity TODO 7 and 8; do not mark the
  overall goal complete.

### 2026-07-27 historical CharDriver checkpoint (superseded above)

- The converted GH1 bassist's local bass attachment data and ACP samples were
  already exact. The visible release came from gameplay appending the
  role-generic GH2 `bass_main.milo_ps2` to the decoded converted
  `metal_bass_main` driver domain, which selected GH2 `bassist_intro` during an
  interval where raw GH1 uses `bassist_idle`.
- Decoded `CharDriver` clip paths are now authoritative. The shared role
  package is used only if the driver publishes no usable path. This is a
  role-generic provenance rule, not a bassist, character, clip, mesh, bone,
  pose, or venue exception.
- At frame 120, converted-native and raw GH1 both use
  `bassist_idle@f183`. Both upper arms, `bone_pos_gutbass.mesh`, and the bass
  mesh world transform agree to displayed precision. The current screenshot
  shows both hands on the bass.
- Untouched GH2 still loads `alterna1_main` for the guitarist and `bass_main`
  for the bassist. Focused gameplay-rule tests pass. Source and deployed
  release executables share SHA-256
  `C93F9F824C69D0D562E932AD99CC23653B4A63A78743117F64F498232C5000EF`.
- The reported detached guitar is corrected for the matched Alterna sample.
  Raw/converted `attach-world`, `prop-anchor-world`, `prop-to-attach`, and
  `target-local` matrices are identical to six decimals, while the recovered
  mesh-backed controller publication makes both arms agree to the same
  precision. Fresh no-input raw/native gameplay frame 180 shows the same
  two-handed guitar pose; the close native viewer proves the complete prop and
  both hands without a camera crop.
- All 53 character tests plus `ghogx_gameplay_rules_test` pass (54/54).
- Proof:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/instrument-hand-parity/INSTRUMENT_HAND_PARITY_PROOF.md`.
  Keep the overall instrument item open until the all-eight-guitarist and
  all-bassist sweep is proved.

### 2026-07-27 historical mesh-backed checkpoint (superseded above)

- Converted GH1 and stock GH2 controller targets can be zero-geometry
  `RndMesh` transform records. Native `CharIKHand`, `CharForeTwist`, and
  `CharUpperTwist` now resolve mutable transforms across both the bone and mesh
  tables; there is no GH1 compatibility controller path.
- Static GH2 PS2 analysis identifies `CharIKHand::Poll` at `0x0017A080`.
  Instructions at `0x0017A1AC..0x0017A1EC` load
  `0xBF7C28F6`/`0x3F7C28F6`, proving a target elbow-cosine clamp of
  `[-0.985, 0.985]` rather than later RB3's `[-1, 1]`.
- At one matched Alterna intro sample, converted-native left and right
  post-controller upper-arm matrices now match the raw GH1 path to six decimal
  places. All focused IK/twist/clip-driver tests pass.
- Release executable is deployed; source and destination SHA-256:
  `25136D1925EE82804A6E91E56B93A7B0E613AF0FC21FCAA363E66C58EB414ADF`.
  Focused proof:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/placement-small-club/postcontroller-differential/CONTROLLER_PARITY_PROOF.md`.
- New open visual decode item: the GH1 Small Club's pale stone-like stage
  panels and horizontal trim are not decoded/rendered correctly. Recover their
  source material, texture, and render-state contract without object/material
  or venue-specific overrides.

### 2026-07-27 GH1 native conversion parity goal reopened

- User rejected the 2026-07-27 completion: the proof only established that
  converted assets load and run, not retail parity.
- Concrete visible failure: every performer overlaps in the smoke-test video.
  Converted GH1 venue worlds omitted the GH2-native character-placement
  assembly, so revision-24 character packages bypassed the old
  character-layout-gated stage-spot fallback and retained identity placement.
- Active systemic correction translates the four authored GH1 Arena
  stage/walk spot mesh transforms into native revision-3 GH2 Waypoints in a
  generated `<venue>_chars.milo_ps2`, linked from the converted world. This
  preserves mix-and-match placement independently of character origin.
- G7, G8, and G12 are open. Do not mark complete from zero-error logs, load
  matrices, or short video; matched PCSX2/runtime character and venue parity
  remains required.

### 2026-07-27 complete GH1-to-GH2 native format conversion

- Complete packed sweep: 105/105 GH1 directories, 926/926 ACPs, 13,115
  converted plus 625 source-derived target objects, zero blocked, 13/13
  character packages, 31 animation sets/926 clips, and 25 ACG assets/29,808
  nodes.
- All seven venues emit native revision-24 worlds with 12,979 typed
  references, seven compiled scripts (886,557 bytes), and 201 native
  CamShots/181,864 keyframes. Native flat TypeProps is authoritative and GH1
  `camera.dtb` is only a no-native-key compatibility fallback.
- Venue-script `foreach` now lowers finite dynamic `switch` collections into
  their concrete branches, eliminating bogus syntax/variable targets without
  a venue or object rule.
- Independent clean conversions produce identical 163-row ledgers with
  SHA-256
  `0722D07FBA11E1BFCF9813FD5ABC065FA0F87FC69708B140892FAD7689D63269`.
  The deployable set is 111 files/38,872,235 bytes.
- Final hidden-window runtime proof uses GH2 `shoutatthedevil`, GH1 Small
  Club, and an all-GH1 band. Native `Intro01` transitions to regular
  `flr_near_lft01`; the run exits 0 with no unsupported script operation,
  unresolved animation target, or performer-load failure. No synthetic input
  or focus change was used.
- Proof:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-final/FINAL_PROOF_MANIFEST.md`.
- Deployed executable SHA-256:
  `A56316B35CE0B0E690636FAEE7C8986D4452051085108684C41FCCAF067B6869`.

### 2026-07-26 complete GH1 character support, reopened arm-twist correction

- GH1 selectable characters are decoded as anonymous revision-10 `RndDir`
  graphs whose skeleton transforms are zero-geometry `RndMesh` records.
  Standalone ACP scalar channels use absolute sampled axes only behind that
  three-part packed-format gate; GH2 `BandCharacter` graphs retain their
  independent path.
- Every mesh with a serialized bone palette now uses decoded skinning. Empty
  palettes alone use authored vertices through the mesh transform. This
  source-shaped split restores the singer's weighted face/eyes and, together
  with the ACP correction, fixes the disconnected guitarist arms and floating
  bassist legs without character, mesh, material, bone, pose, or offset rules.
- GH1 Mat21 selectors 0/1 supply sampled 2-D textures; selector 5 retains the
  environment-map contract. Compact use-environ, prelit, and Z-mode state is
  preserved.
- Packed `charsys/gen/charbase.dtb` also creates two
  `AnimServoForeTwist` and two `AnimServoUpperTwist` controllers. GH1 ACPs
  animate the surrounding arm chain but not the four twist transforms used
  by weighted arm palettes. The recovered shared graph now distributes the
  packed fore/upper twist only on GH1 revision-10 mesh-transform characters;
  GH2 keeps its separate `CharForeTwist`/`CharUpperTwist` bone graph.
- Close animated proof covers Alterna as the different guitarist with two
  explicitly all-GH1 bands, one using the female singer and one using the
  male singer. Separate current-build proofs cover bassist, drummer,
  keyboardist, female singer, and all eight playable guitarist models.
- YYZ's packed `BAND KEYS` route owns the keyboard slot; it is not presented as
  an invented fifth band member. The female singer is proved independently.
- GH2-only old/current SSIM rounds to `1.000000`; repeat current captures vary
  slightly more (`0.999999`) from GPU rasterization. All 53 character tests
  pass. All 78 tests outside the unrelated stale venue/band source-string
  scanner pass, including the independently rerun import test.
- Final manifest:
  `.codex/current-evidence/gh1-arm-twist-reopen/FINAL_PROOF_MANIFEST.md`.
- Release executable is deployed to
  `gh2_ps2_hybrid_assets/ghogx_app.exe`; source/deployed SHA-256 is
  `B847A28F037CE0284DAFD9AA0C9E12C38434C7DF280BF1B7099D355FB1F92824`.

### 2026-07-24 complete GH2 lighting presentation

- GH2 LightPreset animation now respects serialized RndAnimatable units:
  normal rate-1 venue presets use 480 frames per beat instead of 30 fps.
  Automatic keyframes honor duration/fade, looping, and final-key hold.
- The separately traced GH2 manual cue path remains timer-plus-four seconds.
  First/next/previous now wraps only for looping presets and clamps otherwise.
- Category/adjective fallback uses the packed script order and source-shaped
  random selection among eligible presets, retaining the selected preset
  across frame polls.
- Decoded SpotlightEntry, EnvironmentEntry, and EnvLightEntry state reaches
  venue/prop/performer/drum/crowd consumers, including ambient/fog,
  light type/range/transform, flare state, and authored overbright colors.
- GH2 failure and completion select packed LOSE/WIN states; encore
  INTRO_ENCORE/WIN_ENCORE routing is gated to GH2 state. GH1 remains on its
  independent `set_lights_*` venue-script route.
- Final build passes. The focused broad contract runner still reports its
  pre-existing unrelated stale string assertions, but no lighting assertion.
- Proof and format contract:
  `.codex/analysis/gh2-lighting-gh1-regression-audit.md` and
  `.codex/current-evidence/gh2-lighting-complete/`.
- Release executable is deployed to
  `gh2_ps2_hybrid_assets/ghogx_app.exe`; source/deployed SHA-256 is
  `E5D2603CA688832DACAD7DF0EF7ED378BF72D1E6130D06D60529BB15D8C6267E`.
  The deployed Small2 smoke run exits 0 and applies decoded preset state.

### 2026-07-24 GH2 lighting / GH1 shared-path audit

- GH1 venue work did alter one shared GH2 path: Environ-owned meshes with no
  decoded approximate directional light no longer receive two synthetic fill
  lights. GH1 performer-environment routing itself remains legacy-gated.
- Deterministic A/B captures show a real but modest effect. Theatre venue-only
  is visually unchanged (3,134 changed pixels; mean absolute RGB difference
  about 0.1). Small2 with performers averages roughly three RGB units darker
  after removing the invented fill; the singer region is identical.
- The larger unresolved GH2 fidelity gap is dynamic LightPreset application.
  Small2 selects packed `blackout.pset` and then `color1.pset`/`blue`, while
  performer modulation remains identity white pending complete
  EnvironmentEntry/EnvLightEntry consumption. This is documented as open work,
  not repaired with a fill-light or preset-name band-aid.
- Format/evidence:
  `.codex/analysis/gh2-lighting-gh1-regression-audit.md` and
  `.codex/current-evidence/gh2-lighting-audit-current/`.
- The separate Quickplay venue leak is fixed and deployed. Retail `game` mode
  now synchronizes `song_provider`; Quickplay leaves the selected song's
  `songs.dtb` venue untouched.

### 2026-07-24 career song / tier venue / camera correction

- Career `song_index` is provider-relative. The runtime now resolves it through
  `song_provider get_symbol` and uses that one symbol for chart/audio loading
  and title/artist presentation. A second-row proof selects and displays
  `Mother` / `Danzig`, independently of the first-row proof.
- Packed `campaign.dtb::order` owns tier venue assignment. All eight mappings
  are regression-covered; tier one resolves to `battle` (the High School) and
  replaces the selected song's unrelated quickplay venue for career gameplay.
- Packed `world_objects_worldbase.dta::intro_start_msg` selects an exact
  `INTRO`, `INTRO_FAST`, or `INTRO_ENCORE` category. Normal tier-one entry now
  selects `battle::Intro01`, not the first serialized `intro_encore1`.
- Direct multi-pose CamShots retain one authored shot and their per-frame
  duration/blend fields. The intro path now uses the source frame evaluator
  instead of flattening every pose to frame zero; `Intro01` visibly sweeps
  across its authored 300-frame blend.
- Video proofs:
  `.codex/current-evidence/career-three-fixes/`
  `career-first-tier-song-venue-camera-proof.mp4` and
  `career-second-song-identity-proof.mp4`.
- Format record:
  `.codex/analysis/gh2-career-song-venue-camera-format.md`.
- Final release build and the focused intro-overlay format test pass. The
  executable is deployed to `gh2_ps2_hybrid_assets/ghogx_app.exe`; the
  source/deployed SHA-256 is
  `B6CCDF6F6159AC7C9ABA8EB7AA7F5546E570A1FDE705A906A22F683423825198`.

### 2026-07-24 combined retail song-entry / GH1 Arena pass

- User review accepts the corrected GH2 title-card text but rejects the venue
  frame as a fidelity proof. The mixed GH1 Small Club capture visibly has
  displaced drum pieces in the foreground, a microphone/stand intersecting a
  floor monitor, ceiling-hung records/strings displaced into the foreground,
  and anomalously bright rear surfaces. These are logged as open GH1 venue
  prop/hierarchy/material/lighting work; the screenshot does not prove their
  causes, and no object-specific fixes are authorized.
- Packed retail GH2 `ui/gen/game.dtb` proves the MTV overlay is hidden at
  `intro_start_msg`, shown by a +1-second task, and hidden by a +6-second task.
  `extend_track` starts the existing TrackPanel assembly, schedules the
  already-built HUD meter slide at +1.8 seconds, and ends the intro at +2.0.
- Normal song entry now keeps the highway/HUD out during the authored six-bar
  venue-camera window, renders the GH2 white title/caption/artist through
  `ui/gen/mtv_overlay.milo_ps2` and `impactor_mtv.milo_ps2`, then starts the
  existing highway/HUD sequence at the intro boundary for every entry route.
  Proof: `.codex/current-evidence/combined-intro-arena/gh2-intro-after/`.
- Mixed-content ownership is explicit: the primary GH2 archive supplies the
  overlay scene, camera, and upright block font, while the independent content
  archive supplies song/artist metadata. A normal menu run with GH1 `ironman`
  now renders the GH2 card in front of the GH1 venue instead of GH1's
  tilted/italic overlay. The packed `_shadow` labels render before the white
  face labels, preventing coplanar black glyphs from cutting through the
  foreground. Proof:
  `.codex/current-evidence/combined-intro-arena/mixed-gh2-overlay-white-front.bmp`.
- GH1 Arena `back_splash.tex` and `fx_beam.tex` have fully opaque PS2 alpha;
  `back_splash.mat` also authors `Src` in both retained Mat21 blend fields.
  No alpha/chroma key or venue/object exception was added. The hard red/blue
  card edges came from the renderer's invented blue-gray world clear
  contrasting with the textures' black fades. Restoring the default world
  clear to black removes both rectangle boundaries while preserving the
  gradients. Proof:
  `.codex/current-evidence/combined-intro-arena/arena-black-clear.bmp`.
- Both items are recorded completed in `TO_DO.MD`. The focused intro-overlay
  format regression passes. Release build is deployed to
  `gh2_ps2_hybrid_assets/ghogx_app.exe`; source/deployed SHA-256 is
  `BC32300C598A9203671AEC2F8FD6ABA894D245EC60EC92B62CA746181D0A7D72`.

### 2026-07-24 retail setlist/back/pause pass

- Fresh PCSX2 retail captures prove the full setlist and paper move together
  while the selected song stays fixed after scrolling. Accepted trace:
  `.codex/current-evidence/ui-three-pass-20260724/stock-setlist-scroll3.*`.
- The local setlist now uses the authored `ss_song.lst` transform/40-unit row
  height, renders every campaign and store-bonus row, and removes the duplicate
  hand-built title. Top and final-song proofs are
  `setlist-traced-top-final.bmp` and `setlist-traced-last-final.bmp`.
- The temporary always-on review cheat now unlocks all 64 retail songs,
  including the 24 `store.dtb` bonus songs; it remains explicitly temporary.
- Authored backward `goto_screen` routes now pop history instead of re-pushing
  the exiting screen. Enter/Start now pauses/resumes through stock
  `pause_screen` over the retained gameplay render.
- Release build succeeded and was deployed to
  `gh2_ps2_hybrid_assets/ghogx_app.exe`; deployed SHA-256 is
  `A284091FD581DAA4150532E82F0748DF4FFB14F9E55A5B943AF6B78A2C261F5A`.
- These three runtime items are documented completed in `TO_DO.MD`. Arena
  opaque rectangles remain the next requested task.

### 2026-07-23 GH1 Theatre environment/Flare correction

- Theatre's all-white stage was not `RndFlare` output. Category isolation
  proved the mass came from additive `light_spot_*.mesh` floor pools.
- Those pools are owned by revision-1 Environs with zero authored Light refs.
  The renderer incorrectly installed two diagnostic fill lights when the
  approximate-light list was empty. Authored zero-light Environs now disable
  fallback fill slots and remain ambient-only; defaults remain for meshes with
  no Environ ownership.
- Normal automatic Theatre lighting now renders distinct purple pools with the
  stage surface visible:
  `.codex/current-evidence/gh1-zero-light-environ/theatre-automatic-lighting.bmp`.
- Native Flare point tests initially queried `(0,0,0)` because Theatre Flare
  roots serialize identity local matrices/self parents and distinct cached
  WorldXfms. Root Flare rendering now uses serialized WorldXfm, while runtime
  parents still compose local transforms.
- D3D9 point primitives were replaced by sub-pixel camera-facing query quads.
  All eight Theatre `ok_chorus*.flare` objects now report 1--4 visible samples
  and enter the draw path:
  `.codex/current-evidence/gh1-flare-quad-query/theatre-stored-world.log`.
- Seven venue-only automatic-lighting runs exit zero, remain playing, and
  report zero unsupported operations:
  `.codex/current-evidence/gh1-environ-flare-seven-venue/`.
- Format notes:
  `.codex/analysis/gh1-rndenviron-zero-light-state.md` and
  `.codex/analysis/gh1-rndflare-format-and-set-steps.md`.
- Goal remains active. By user direction, later PCSX2 lighting/exposure review
  and generic final visual acceptance are outside the current GH1 venue-open
  list. The Big Club central wedge was closed by user direction on 2026-07-24
  after a fresh matched-timing capture did not reproduce it; pause/fail
  presentation is an overall runtime item rather than a GH1 venue item.

### Deferred retail setlist-screen fidelity observation

- User live review shows the quickplay setlist screen is not DTB-accurate:
  two overlapping “Setlist” graphics, a song list placed much too high, and
  incorrect overall composition.
- Marked only; do not fix yet. Later work must recover panel/DTB ownership and
  authored transforms rather than deleting one graphic or tuning coordinates.
- Screenshot:
  `C:/Users/smmel/AppData/Local/Temp/codex-clipboard-0c568b73-93fe-4d62-b5da-f6a4433e40f1.png`.

### Additional deferred live-play observations

- Pause and fail menus should occupy only a centered overlay region while the
  venue and characters remain rendered behind them. Current presentation does
  not preserve that retail composition.
- Song entry currently lacks both the retail
  `"[song] AS MADE FAMOUS BY [band]"` introduction and the opening venue
  camera.
- Lighting needs a source-backed review: the observed venue was blue while
  performers appeared notably more orange. This is not yet diagnosed; compare
  the selected venue/band/drummer/character Environs and LightPreset timing
  with retail evidence before changing color or intensity.
- Marked only; no fixes were made. Do not introduce full-screen menu
  replacements, song-specific title/camera paths, or hand-tuned performer
  color corrections.

### Additional deferred retail interaction defects

- Manage Band does not allow Up/Down focus movement into character entries.
  Pressing Green/`A` reached “Please enter a band name” and soft-locked.
- `Esc` must not terminate normal play; quitting needs an intentional UI path.
- Audio Settings adjustments are reversed: expected Up -> right/increase and
  Down -> left/decrease.
- Red/Back must follow real previous-screen history. Audio Settings currently
  soft-locks navigation in an OPTIONS <-> AUDIO SETTINGS loop.
- `Enter`/Start does not pause during a song.
- Default calibration appears substantially wrong: live Easy play on a
  first-tier song registered only about 5% of attempted notes.
- Marked only; no fixes made. Navigation must come from retail focus,
  dialog, parent/history, and transition facts. Calibration must be based on
  measured audio/render/input clocks rather than a wider ad hoc hit window.

### 2026-07-22 GH1 venue-first checkpoint

- Venue fidelity is now the immediate milestone. Highway/HUD/scoring and
  animated character fidelity are deferred while the original GH1 venue files
  are verified.
- Venue intro animation is part of that fidelity target, not optional polish.
  Retail GH2 venues animate during their intro sequences, so GH1 intro
  handlers and animation filters must run alongside each native intro camera;
  the basement camera still intentionally walks down the stairs.
- The only accepted shipping choices are native reads of the original GH1
  Milos or an offline tool that emits complete persistent GH2-format Milos.
  Runtime conversion and generated caches are not part of the design.
- `docs/GH1_VENUE_COMPATIBILITY.md` records the evidence hierarchy, verified
  legacy revisions, basement relationships, and unresolved fidelity gates.
- `--venue-only` is a presentation-only diagnostic mode. It suppresses HUD,
  highway, performer models, and performer props without modifying assets.
- Release `ghogx_app` builds successfully with the new mode.
- Clean proof `.codex/current-evidence/gh1-basement-venue-only-frame300.bmp`
  shows the camera embedded in riser/stair geometry even with every gameplay
  and character layer removed. The obstruction is therefore venue/camera
  transform behavior, not the highway or Metal.
- Native `Cam_basement_intro.tnm` source samples range from
  `(178.779,134.478,151.359)` to `(111.400,28.286,84.359)`, contain 88
  translation keys and no rotation/scale keys, and have no serialized Trans
  target. Do not invent a parent transform; recover the GH1 `switch_cam`
  coordinate/framing behavior before changing these values.

### 2026-07-22 GH1 VenueCam native recovery

- GH1 `switch_cam` dispatches to native `VenueCam` handler `0x0016F3B8`.
  Its property path stores `singer_in/out` as two-float screen vectors at
  `VenueCam+0x1C/+0x24` and `offset_in/out` as separate vector values at
  `VenueCam+0x50/+0x60`.
- The per-frame update at `0x0016E390` interpolates singer coordinates and
  maps them using `(x+1)/2,(1-y)/2`; it separately interpolates the spatial
  offset. Bounded disassemblies are in `.codex/analysis/` and the behavior is
  documented in `docs/GH1_VENUE_COMPATIBILITY.md`.
- Basement Intro has an authored parent `arena::venue.view` and deliberately
  no explicit target. The earlier guitarist-spine target was invented and has
  been removed. Native no-target framing begins from the selected parent.
- Venue camera targets now distinguish an authored Group/View transform from
  its member-mesh centroid. `venue.view`'s embedded transform is verified exact
  identity; using the 90-mesh centroid as its transform was incorrect.
- Three clean path samples remain embedded in overlapping architecture even
  after the exact identity parent/no-target route is restored. Evidence is in
  `.codex/current-evidence/gh1-basement-view-transform-sequence/`.
- This proves the next blocker is GH1 venue mesh/View transform assembly, not
  an intro endpoint, highway, character, or missing root transform. Audit
  legacy child reconstruction versus stored-world composition before further
  camera tuning.

## Active cross-game content goal (2026-07-22)

- Keep GH2's UI, HUD, highway, scoring, and gameplay permanently authoritative.
- GH2 is always the primary/base game and owns every visual gameplay and UI
  element unless the user explicitly changes that rule. Foreign archives must
  never become the primary asset mount merely to load their content.
- Songs, venues, and characters are independent cross-game selections. Any
  combination must work, for example a GH80s character in a GH1 venue playing
  a GH2 song, while GH2 continues to own the surrounding gameplay/UI.
- A foreign character brings its character/guitar assets and its unique
  character highway texture. That highway texture is the sole stated
  exception to GH2's ownership of visual gameplay elements.
- Make GH1 and GH80s songs, native characters/guitars, venues, and the
  character-linked highway-texture exception selectable content inside that
  runtime.
- This clarification is an architectural note, not authorization to change or
  test the runtime now. Existing proofs that mounted GH1 as the primary archive
  do not demonstrate the required hybrid boundary.
- Career: YELLOW cycles GH1 / GH2 / GH80s, with separate progress per game.
- Quickplay: YELLOW cycles GH1 / GH2 / GH80s / DLC setlists.
- The gameplay loader now has a separate base-asset archive mount; menu mode
  pins it to GH2 so a foreign content archive cannot replace highway textures.
- Build proof: `.codex/current-evidence/gh1-layered-base-build.log` (exit 0).
- GH1 native performer loading now reaches the gameplay character renderer:
  `char/<model>/og/...` falls back to `charsys/<model>/gen/<model>.rnd_ps2`.
- The character decoder now supports GH1 Mesh rev25 / directory rev10. Metal
  improved from 0/185 to 184/185 decoded meshes; Iron Man enters gameplay and
  loads that native asset. Evidence: `gh1-ironman-metal-gameplay.log` and BMP.
- GH1 legacy `View` compatibility is now decoded authoritatively. The MILO
  parser preserves an intentional zero-body Mesh before packed View/Group rev7
  bodies; Metal yields 64 LOD0 members, 52 LOD1 members, and 6 top details.
  The character renderer uses those exact lists to exclude helpers/inactive LOD.
- Remaining character blocker is Mesh geometry ownership/runtime Sync behavior.
  Visible GH1 View entries reference geometry-owner Meshes extensively. A naive
  eager buffer copy produced empty output and was removed; port the actual
  owner resolution semantics before changing geometry again.
- Source inspection established `RndMesh::Verts/Faces` forwards exactly one
  level to `mGeomOwner` while retaining the alias Mesh's transform/material.
  The value renderer now materializes that direct view (not recursive owner
  chaining), and Character transform lookup includes decoded View/Group rows.
  This exposes substantially more intended geometry, but the rigid model is
  still spatially incorrect; do not claim GH1 character visuals complete.

Updated: 2026-07-22 (resumed active-panel fidelity pass)

## Active goal

Make every out-of-gameplay Guitar Hero II menu and flow match shipped behavior from stock MILO, DTB, and audio data in both 4:3 and 16:9, including animated characters/guitars and gameplay entry/exit.

## Authoritative worktree

- Worktree: `GuitarHeroOGX-main-ui-engine`
- Branch: `main-ui-engine`
- Base commit: `1e6247c7a2aa`
- Current dirty scope: 30 tracked paths; preserve all of them.
- Preserve the existing dirty worktree; it contains the active menu-fidelity implementation.
- Do not commit or push unless the user asks.

## Latest verified state

- Stock `ui_enter` and `ui_loop` clips decode for every character UI bank.
- Menu clips now retain their authored real-time mode and 1-second blend instead of being forced to `NoBlend`.
- Character preview/driver objects persist across ordinary visual redraws instead of restarting at frame zero.
- Two-player character models render and animate from the stock `char_multi` placers.
- The menu renderer now excludes panels whose stock `showing` or
  `MultiSelectPanel::active` gate is false. Direct entry to character select no
  longer draws both inactive outfit MILOs over the character-select scene.
- The stock `multi_char_selected` handler is verified to activate only the
  selected player's outfit panel and write the character-specific
  `punk1_outfit` / `punk2_outfit` tokens into its live buttons.
- `MultiSelectScreen` now receives each controller button event exactly once;
  a character-select X press no longer also marks the newly opened outfit
  panel ready.
- Active outfit panels now render as a single foreground unit after live
  characters (backing meshes, then labels). Legacy BandButton text retains the
  stock `impact` font and fits its serialized component width, so `LIBERTY
  SPIKES` no longer bleeds outside the panel.
- `PlayerConfig::set_outfit_index` now returns and stores the resolved outfit,
  and `CharsysPanel::show_char` returns the stock truthy swap-accepted result,
  allowing the authored focus handler to complete.
- Live outfit swaps preserve the old `main.drv` clip stack and blend the new
  outfit's `ui_loop` over it. Alternate outfit geometry uses the base outfit's
  shared UI animation bank when its own is absent (`punk2 -> punk1_ui`), while
  `store` remains the stock reset/no-blend loop path.
- `ghogx_ui_test` passes: 40 DTBs, 2634 widgets, 238 objects, 272 routes, 0 unresolved.

## Retained current evidence

- `.codex/current-evidence/2p-character-select-current.bmp`
- `.codex/current-evidence/2p-character-select-current.log`
- `.codex/current-evidence/menu-ui-clip-audit.log`
- `.codex/current-evidence/2p-character-select-active-gate.bmp`
- `.codex/current-evidence/2p-character-select-active-gate.log`
- `.codex/current-evidence/2p-p1-outfit-layered.bmp`
- `.codex/current-evidence/2p-p1-outfit-layered.log`
- `.codex/current-evidence/2p-p1-outfit-layered-16x9.bmp`
- `.codex/current-evidence/2p-p1-outfit-layered-16x9.log`
- `.codex/current-evidence/2p-p1-outfit-driver-transfer.bmp`
- `.codex/current-evidence/2p-p1-outfit-driver-transfer.log`
- Older `%TEMP%/gh2-*` and `%TEMP%/punk-ui-*` artifacts were moved to the Windows Recycle Bin on 2026-07-22; no source or repo proof archive was removed.

## Immediate defects

- No currently evidenced blocking visual defect. Continue targeted audits from
  shipped data and real stateful flows; do not infer defects from invalid
  direct-start state.

## Next action

Continue the requirement-by-requirement completion audit using the 130-screen
structural pass, targeted visual captures only for suspicious screens, and
stateful flow tests. Keep every command output bounded by `AGENTS.md`.

### 2026-07-22 guitar-screen correction in progress

- `sel_guitar_new_screen` had frozen `guitar_single.filt` at its serialized
  frame. The live guitar renderer now advances that stock looped TransAnim.
- A populated `CharsysPanel` is retained state, not a second preview layer while
  a live `GuitarDisplayPanel` is visible. The erroneous character is no longer
  drawn on the guitar/case screen.
- Proof: `.codex/current-evidence/single-guitar-exclusive-4x3.bmp` and
  `single-guitar-exclusive-16x9.bmp`. Build passed; `ghogx_ui_test` passed with
  40 DTBs, 2634 widgets, 238 objects, 272 routes, and 0 unresolved routes.
- Next: pin runtime quaternion composition/phase to the stock front-facing pose,
  then verify multiplayer and store guitar screens in both aspects.

### 2026-07-22 canonical guitar/store pass completed

- Live UIProxy guitars now advance the stock looped AnimFilter instead of
  freezing at the serialized frame. Runtime quaternion deltas preserve each
  guitar mesh's authored base-local rotation; single selection moves from its
  edge-on initial phase to the stock front-facing pose.
- Single and multiplayer guitar screens render animated, independently
  namespaced guitars in 4:3 and 16:9. Guitar screens no longer draw retained
  character state.
- Store guitar detail now uses the owning panel camera with UIProxy clipping
  range, so the stock `guitar.pxy` world transform is visible rather than being
  rejected by the store camera's near=400 plane.
- Store category BandButtons are gated by the stock `st_screen1.view` page state;
  they no longer bleed over `st_screen2.view` item detail.
- Canonical store draw order was audited from `store.milo`: `us_gate.mesh`,
  `us_gate_side.mesh`, `us_gate_frame.mesh`, then outer right/left/bottom/top
  meshes. No guessed frame/door reversal was added.
- Proof:
  - `.codex/current-evidence/single-guitar-compose-frame0.bmp`
  - `.codex/current-evidence/single-guitar-compose-frame30.bmp`
  - `.codex/current-evidence/single-guitar-animated-16x9.bmp`
  - `.codex/current-evidence/multi-guitar-animated-4x3.bmp`
  - `.codex/current-evidence/multi-guitar-animated-16x9.bmp`
  - `.codex/current-evidence/store-guitar-category-exact-4x3.bmp`
  - `.codex/current-evidence/store-guitar-category-exact-16x9.bmp`
  - `.codex/current-evidence/store-stock-draw-order.log`
- Final build and `ghogx_ui_test` pass: 40 DTBs, 2634 widgets, 238 objects,
  272 routes, 0 unresolved.

### 2026-07-22 unlock-guitar reward composition

- Auditing the real stateful reward path exposed a live-UIProxy camera bug on
  `unlock_guitar_screen`: `unlock_guitar_panel` correctly supplied the decoded
  `fish_reward`, `ug_guitar.pxy`, and `ug_guitar.filt`, but the shared lighting
  rig's `guitar_setup.cam` overrode the owning reward-screen camera and hid the
  built guitar.
- Live proxy scenes now explicitly clear an automatically selected shared-rig
  camera and draw with the owning screen camera plus the broad UIProxy clipping
  range. This preserves selection/store behavior and renders the unlocked guitar.
- Added opt-in `GHOGX_MENU_SEED_UNLOCK_GUITAR=1` capture setup. It queues the
  first decoded tour reward and re-enters the shipped `unlock_guitar_panel`
  handler; bare direct-start behavior remains unchanged and is still not proof.
- Proof:
  - `.codex/current-evidence/unlock-guitar-canonical-4x3.bmp`
  - `.codex/current-evidence/unlock-guitar-canonical-16x9.bmp`
  - `.codex/current-evidence/unlock-guitar-canonical-4x3.log`
  - `.codex/current-evidence/unlock-guitar-canonical-16x9.log`
- `ghogx_ui_test` remains green: 40 DTBs, 2634 widgets, 238 objects, 272 routes,
  0 unresolved.

### 2026-07-22 store garage-frame composition

- Canonical `store.milo` draw order is the shutter, handle, side trim, top
  door frame, then the outer right/left/bottom/top shop frame. The menu overlay
  split had deferred only `us_gate.mesh`, causing the shutter to composite over
  the later frame pieces despite their authored order.
- The complete contiguous store foreground stack is now deferred together, so
  the shutter still covers store labels while the door and outer frame remain
  canonically in front of it.
- Proof:
  - `.codex/current-evidence/store-stock-draw-order.log`
  - `.codex/current-evidence/store-canonical-frame-order-4x3.bmp`
  - `.codex/current-evidence/store-canonical-frame-order-16x9.bmp`
  - `.codex/current-evidence/store-frame-ui-test.log`
- Build and `ghogx_ui_test` pass: 40 DTBs, 2634 widgets, 238 objects,
  272 routes, 0 unresolved.

### 2026-07-22 completion audit

- The shipped out-of-gameplay fidelity goal is complete at the current evidence
  boundary. The final structural audit loads all 130 stock screens with
  `ok=130`, `failed=0`, and the route audit reports 272 routes with 0
  unresolved.
- The full UI regression passes (40 DTBs, 2634 widgets, 238 objects), including
  the stateful unlock-venue, guitar, character, store, profile, helpbar, and
  transition contracts added during this goal.
- Targeted visual checks cover representative menu families and the repaired
  stateful flows in both 4:3 and 16:9. Automated entry/exit loops pass from the
  stock menu route through loading and gameplay, then through results,
  post-show, endgame, stats, high score, completion, and back to song select in
  both aspect ratios.
- No currently evidenced blocking visual, routing, animation, audio, or
  gameplay-boundary defect remains. Do not replace this checkpoint with a bulk
  contact sheet; use only targeted on-disc screenshots for later regressions.

### 2026-07-22 gameplay meter clearance adjustment

- Shifted the complete left HUD parent (`score_panel`) outward from normalized
  X `0.162` to `0.111857`, and the complete right HUD parent (`right_panel`)
  from `0.841` to `0.875857`. A measured `+0.001400` screen-width correction
  applies at 16:9 because its projected rail slope differs from 4:3. The
  unequal anchor deltas compensate for the unequal
  rendered silhouettes, producing visually uniform clearance from the highway
  in both 4:3 and 16:9.
- Only parent X anchors changed. Sizes, child placement, depth, left/right
  rotations (`35` / `-23` degrees), and the decoded `score_slide_in.tnm` /
  `meter_slide_in.tnm` translation and rotation deltas are unchanged.
- Updated both the baked defaults and `engine/out/hud_tuning/hud_layout.txt`,
  then rebuilt and checked targeted mid-intro (frame 120) and settled (frame
  150) captures in both aspect ratios.

### 2026-07-22 GH1/GH80s bonus-content goal (active)

- Product model clarified: GH2 remains the permanent UI/gameplay/base-asset
  layer. GH1 and GH80s are optional content profiles supplying songs,
  characters, guitars, and venues. Career YELLOW cycles GH1/GH2/GH80s with
  separate progression; Quickplay YELLOW cycles GH1/GH2/GH80s/DLC setlists.
- Pristine GH1 archive at `C:/Programming/GitHub/Guitar Hero II/GH1/GEN`
  opens as ARK v3 (3167 entries), and `ironman` enters the GH2 gameplay state.
- Central ARK fallback resolves missing `.milo_ps2` requests to GH1
  `.rnd_ps2`, plus GH2 `world/` to GH1 `venues/` roots. Exact GH2/GH80s paths
  still win.
- GH1 version-10 object-directory support advanced:
  - Mat revision 21 source layout decoded.
  - Mesh revision 25 directory slicing and object decoding added.
  - Tex revision 8 decoded without GH2's per-entry Object metadata.
  - GH1 basement now decodes 123/124 geometry meshes, 92/93 lighting meshes,
    35/36 geometry textures, and 18/19 lighting textures.
  - GH1 `charsys/metal/gen/metal.rnd_ps2` decodes 184/185 meshes and 6/7
    textures. Raw drawing still shows hidden alternatives/bone pieces because
    GH1 character visibility/assembly and animation routing are not yet wired.
- Current proof images:
  - `.codex/current-evidence/gh1-basement-textured-v1.bmp`
  - `.codex/current-evidence/gh1-metal-scene-first.bmp`
- Remaining goal work is substantial: layered GH2-base/content mounts, GH1
  camera/light/particle/animation decoding, character visibility and clips,
  catalog/profile selection UI, per-game career state, and both-aspect runtime
  verification. Goal remains active.

### 2026-07-22 attract-mode picker and authored outgoing transitions

- Production `campaign pick_attract_song` now chooses uniformly from the
  decoded shipped campaign order and avoids immediately repeating the previous
  song. `attract_song_index` remains an explicit deterministic test/capture
  override only.
- Screen replacement now retains and renders the exiting screen, live guitar,
  character state, text, and panel animation sources for the exact lifetime of
  `ScreenManager`'s authored transition. The entering screen draws over that
  retained layer, so stock `ui_exit` and `ui_enter` transforms/materials—not a
  guessed generic fade—control what is visible until `exit_complete`/`unload`.
- Targeted Main-to-Options transitions render cleanly in 4:3 and 16:9; the
  opaque entering poster canonically covers the retained outgoing layer.
- Proof:
  - `.codex/current-evidence/main-options-transition-4x3-frame20.bmp`
  - `.codex/current-evidence/main-options-transition-4x3-frame60.bmp`
  - `.codex/current-evidence/main-options-transition-16x9-frame20.bmp`
  - `.codex/current-evidence/main-options-transition-16x9-frame60.bmp`
- Build and full `ghogx_ui_test` pass: 40 DTBs, 2634 widgets, 238 objects,
  272 routes, 0 unresolved.

### 2026-07-22 stateful unlock-venue animation

- Direct-start reward proof previously seeded campaign status only after panel
  MILO widget ingestion. That attempted `unlockvenue0.milo_ps2`, left the
  authored `unlock_anim.grp` absent, and caused the first panel poll to jump
  immediately to `complete_screen`.
- Menu boot now installs meta singletons before state-dependent panel widget
  ingestion. The opt-in `GHOGX_MENU_SEED_UNLOCK_VENUE=1` state is applied for
  ingestion and restored after `init.dtb`, without changing ordinary unseeded
  boot behavior.
- At deterministic 60 Hz, stock `unlockvenue1.milo_ps2` remains active through
  frames 15 and 120 while the Midwest-to-Boston map route advances, then reaches
  `complete_screen` after the authored 0..200 / 3-second task by frame 200.
  The sequence is verified in both 4:3 and 16:9.
- Added a regression that requires stateful panel ingestion to materialize the
  panel-scoped `unlock_anim.grp` and verifies the transition handler starts its
  named animation task.
- Proof:
  - `.codex/current-evidence/unlock-venue-canonical-4x3-frame15.bmp`
  - `.codex/current-evidence/unlock-venue-canonical-4x3-frame120.bmp`
  - `.codex/current-evidence/unlock-venue-canonical-4x3-frame200.bmp`
  - `.codex/current-evidence/unlock-venue-canonical-16x9-frame15.bmp`
  - `.codex/current-evidence/unlock-venue-canonical-16x9-frame120.bmp`
  - `.codex/current-evidence/unlock-venue-canonical-16x9-frame200.bmp`
  - `.codex/current-evidence/unlock-venue-regression-ui-test.log`
- Build and full `ghogx_ui_test` pass: 40 DTBs, 2634 ordinary widgets,
  238 objects, 272 routes, 0 unresolved.

### 2026-07-22 canonical UILabel FitJust wrapping

- `win_game_screen` collapsed the shipped multiline contract into an
  unreadable hairline because the Text/BandLabel renderer treated serialized
  `kFitJust` (2) as explicit-newlines-only. Each long paragraph was measured as
  one enormous line, forcing the entire block to a tiny scale.
- Harmonix `UILabel.h` is authoritative: `kFitWrap=0`, `kFitStretch=1`, and
  `kFitJust=2`. FitJust still lays text out through the retained RndText wrap
  width, then fits the wrapped block to the UILabel rectangle. Only FitStretch
  preserves authored lines as a single scalable block.
- The Text/BandLabel renderer now follows those semantics. The separate legacy
  BandButton fit path is unchanged.
- Proof:
  - `.codex/current-evidence/win-game-fitjust-canonical-4x3.bmp`
  - `.codex/current-evidence/win-game-fitjust-canonical-16x9.bmp`
  - `.codex/current-evidence/win-game-fitjust-ui-test.log`
- Build and full `ghogx_ui_test` pass: 40 DTBs, 2634 widgets, 238 objects,
  272 routes, 0 unresolved.

### 2026-07-22 PC title and gameplay entry/exit proof

- The PC menu/gameplay window title is now `Guitar Hero Classic`. This changes
  only the native window chrome; stock in-game branding and Xbox behavior are
  untouched. A live Win32 `MainWindowTitle` query against the rebuilt app
  returned the exact requested title.
- Added opt-in `GHOGX_MENU_DIAGNOSTIC_SONG_START_SEC` support to the existing
  automated menu loop. It enters through the real stock menu/loading route and
  uses Gameplay's existing diagnostic seek/replay path only after the selected
  chart and world have loaded, allowing repeatable post-song verification
  without a real-time full-song wait. Ordinary gameplay is unchanged.
- Deterministic automated loops pass in 4:3 and 16:9 through:
  `main_screen -> qp_selsong_screen -> qp_diff_screen -> loading_screen ->`
  gameplay/autoplay -> result commit -> YOU ROCK -> `post_show_screen ->`
  `endgame_screen -> endgame_stats_screen -> highscore_screen ->`
  `complete_screen -> qp_selsong_screen`.
- Proof:
  - `.codex/current-evidence/full-loop-seek-4x3.log`
  - `.codex/current-evidence/full-loop-seek-16x9.log`
  - `.codex/current-evidence/pc-title-full-loop-ui-test.log`
  - `.codex/current-evidence/pc-title-menu-loop-build.log`
- Build and full `ghogx_ui_test` pass: 40 DTBs, 2634 widgets, 238 objects,
  272 routes, 0 unresolved.

### 2026-07-22 focus-keyed helpbar display

- A broad menu capture audit found Data Settings drawing `CONTINUE` and
  `ON/OFF` in the same helpbar slot, visually resembling a stray `TRUE` suffix.
- Extracted shipped `ui/gen/options.dtb` proves `mem_card_screen.helpbar` is a
  focus-keyed table: `default` owns `help_continue`, while `autosave.btn` owns
  `help_onoff`; both share `help_back` and `help_updown`. There is no `TRUE`
  token in the authored display.
- Helpbar composition now selects the live focused-component branch with a
  `default` fallback. Direct lists installed by `helpbar set_display` retain
  their existing semantics. UI regression checks fence the two stock branches.
- Proof:
  - `.codex/current-evidence/stock-options.dtb`
  - `.codex/current-evidence/stock-options.dta`
  - `.codex/current-evidence/mem-card-helpbar-canonical-4x3.bmp`
  - `.codex/current-evidence/mem-card-helpbar-canonical-16x9.bmp`
  - `.codex/current-evidence/mem-card-helpbar-ui-test.log`
- Build and `ghogx_ui_test` pass: 40 DTBs, 2634 widgets, 238 objects,
  272 routes, 0 unresolved.

### 2026-07-22 GH1 native-character checkpoint

- Active product boundary: GH2 always owns UI/gameplay/HUD/highway. Career
  YELLOW cycles GH1/GH2/GH80s with separate progress; Quickplay also includes
  DLC. Content packs contribute songs, characters, guitars, and venues.
- GH1 standalone `AnimClipSamples` rev18 `.acp` is decoded. Its two legacy
  channel sets are names + sample-count + compression followed by their sample
  blocks. Metal loads idle (152), stand (195), and intro (217), 57 channels.
- GH1 Trans8 parent pointers require ParentWorld behavior in character world
  resolution. Applying it removes the hundreds-unit accumulated transform
  chains and assembles Metal's central torso/legs/head correctly. Remaining
  detached geometry/LOD membership and materials still need correction.
- Current targeted proof:
  `.codex/current-evidence/gh1-metal-parentworld-char.bmp`.
- Gameplay smoke still loads Iron Man, basement, Metal, and all three ACPs.
  Basement camera/lighting/geometry remains badly framed and blocks useful
  gameplay proof.

### 2026-07-22 GH1 character body-order/hierarchy breakthrough

- GH1 rev10 directories containing legacy `View` objects omit the first Mesh
  body. The prior slicer assigned bodies to entries 0..N-1 and treated the last
  Mesh as empty, shifting every transform/material/geometry association by one.
  It now leaves the first Mesh identity bodyless and maps the remaining bodies
  to entries 1..N.
- Trans8 trailing self pointers are no longer treated as parents. The authored
  rev<9 child lists are retained and rebuild the actual mesh/bone hierarchy,
  matching the old loader's `SetTransParent` behavior.
- Result: GH1 Metal now renders as one coherent, textured native character and
  accepts the decoded `metal_idle_medium.acp` pose. Targeted proof:
  `.codex/current-evidence/gh1-metal-authored-hierarchy-acp.bmp`.
- The isolated character viewer accepts a standalone `.acp` path via `--clip`
  for deterministic GH1 pose verification.
- The same legacy hierarchy and direct geometry-owner rules now run in the
  scene loader. GH1 `View` entries are routed through the legacy Group decoder;
  7/13 basement Views decode, but `venue.view` still fails and all meshes are
  consequently drawn, including hidden/editor geometry. This is the current
  basement obstruction/camera blocker.

### 2026-07-22 GH1 basement View and camera-path checkpoint

- GH1 View7 layout is now fully decoded: two reserved words, a preliminary
  nested-object vector, embedded Trans8, the legacy drawable block, then the
  authored member vector. All 13 basement Views decode, including the 65-item
  `venue.view` root.
- Rev10 venue rendering now traverses only `venue.view` and nested Views (90
  meshes) instead of appending 34 unreferenced editor/reference meshes.
- GH1 all-TransAnim `campaths.rnd_ps2` directories use the same first-entry
  body omission. The slicer now aligns their remaining bodies correctly.
- Legacy RndTransAnim rev4 + embedded Drawable1 decoding is implemented.
  `Cam_basement_intro.tnm` loads 88 native translation keys and is preferred
  over the editor `6 foot camera.cnm` fallback.
- At this historical checkpoint the path was provisionally aimed at
  `guitarist0:bone_spine1.mesh` using the GH1 INTRO record (50-degree FOV,
  near 10, far 10000), making animated Metal visible in the live Iron Man
  gameplay camera. The 2026-07-29 ArenaSinger virtual trace supersedes that
  provisional subject with the exact `guitarist0:bone_head.mesh` target.
- Remaining camera issue: GH1 `camera.dtb` describes `switch_cam` framing with
  `singer_in/out`, `offset_in/out`, and per-shot visibility. The path currently
  follows and aims correctly but does not yet apply those procedural framing
  offsets, so it passes through the active `rafters.view` geometry.

### 2026-07-22 GH1 camera/venue isolation

- Product selector semantics confirmed: Career YELLOW cycles GH1/GH2/GH80s
  with independent progress; Quickplay YELLOW cycles GH1/GH2/GH80s/DLC.
- A trial conversion of INTRO `singer_in/out` to GH2 CamShot screen offsets was
  rejected and removed: GH2's solver translated the eye by hundreds of units,
  which is not evidence of GH1 `switch_cam` semantics.
- Explicit fixed free-camera support now works for GH1 venues even though they
  have no GH2 debug marker objects. Three viewpoints in
  `.codex/current-evidence/gh1-freecam-views/` show coherent animated Metal and
  coherent portions of the room, separating venue decode from the malformed
  gameplay camera/lighting presentation.
- Camera mesh projection proves the current GH1 path eye sits near the room
  boundary and projects ceiling/front-wall/rafters over most of the screen.
  The immediate blocker is correct GH1 `switch_cam` path/framing semantics plus
  legacy Environ/Light decoding; duplicate View traversal was tested and did
  not change the image, so that renderer experiment was reverted.
- `singer_in/out` and `offset_in/out` remain unapplied until their exact GH1
  solver semantics are established.

### 2026-07-22 GH1 legacy lighting checkpoint

- Native GH1 Light revision 3 is decoded from its exact source layout:
  embedded Trans8, RGBA, range, and type (no later preset booleans). Basement
  now loads all 3 base lights and all 15 lighting-archive lights.
- Native GH1 Environ revision 1 is decoded. Its variable legacy header is
  resolved by the invariant fixed 41-byte payload boundary and validated `.lit`
  vector. All 2 base Environs and all 23 lighting-archive Environs now load,
  including the variable-header `lamp.env`.
- GH1 View7 post-member vectors carry environment refs such as `venue.env`.
  These now populate `GroupObj::environment_ref`, and renderer environment
  traversal recognizes nested `.view` groups by object lookup instead of only
  `.grp` suffixes. Basement maps 90/90 authored meshes to decoded Environs.
- Fixed free-camera support explicitly works for GH1 scenes without GH2 debug
  marker objects. Current targeted image:
  `.codex/current-evidence/gh1-basement-view-environ2.bmp`.
- The remaining primary visual blocker is still correct GH1 `switch_cam`
  procedural framing; the lighting reader itself is no longer failing.

### 2026-07-22 GH1 revision-10 directory alignment and VenueCam correction

- The revision-10 directory slicer was globally off by one object body. GH1
  has an external-resource vector and no serialized root-directory object, but
  the parser discarded child body 0 as if it were that root. In basement this
  produced `band_shadow.tex -> bike_tire.bmp`, then shifted every later texture,
  material, and mesh association.
- The slicer now reads the external-resource vector and starts child body 0 at
  the exact following byte. The compensating first-Mesh/first-TransAnim body
  omission was removed. A synthetic revision-10 two-entry regression test
  passes.
- Verified repaired examples: `band_shadow.tex -> band_shadow.png`,
  `ply_wood.mat -> ramp_ply_mip.tex`, `poster_wall.mat -> poster_wall.tex`, and
  `rug.mat -> rug.tex`. Basement now decodes 124/124 meshes and loads 36/36
  requested textures.
- Native `RndMat` defaults `use_environ` on; revision 21 does not serialize the
  field. The compatibility object now retains that default, routing GH1 mats
  through their decoded environments instead of the bright fallback fill.
- The native VenueCam update maps interpolated `singer_in/out` values from
  centered screen coordinates with `(x+1)/2, (1-y)/2`. The GH1 intro adapter
  previously passed raw singer values into GH2's normalized screen-offset
  path. Basement INTRO now maps `(0,.5)->(.5,.5)` to
  `(.5,.25)->(.75,.25)` exactly.
- Build and a 610-frame venue-only run pass. Individual corrected-asset
  captures are in `.codex/current-evidence/gh1-basement-dir10-fixed/`.
- Textures and lighting are now recognizably coherent. The camera still
  runs close to the stair structure because basement Intro is authored as a
  walk down the stairs; that proximity must not be removed as a workaround.

### 2026-07-22 basement stair-descent camera checkpoint

- Corrected the VenueCam/GH2 camera-coordinate API boundary. GH1 maps centered
  singer coordinates to viewport `(u,v)`, while the local rotation helper
  accepts centered projection coordinates. The adapter now converts
  centered -> viewport -> centered explicitly; normalized viewport values are
  no longer misused as an additional screen displacement.
- The corrected revision-10 `Cam_basement_intro.tnm` has 88 translation keys,
  no rotation keys, and is rendered as the intentional stair-descent sequence.
- Controlled venue-only captures are in
  `.codex/current-evidence/gh1-basement-camera-coordinate-fixed/`.
- The standalone `6 foot camera.cam` is rolled approximately 90 degrees in its
  authored transform; it is an editor/reference camera and is not a valid
  upright-orientation baseline for the VenueCam intro.

### 2026-07-22 GH1 theatre camera discovery

- GH2 intro handling remains its existing CamShot/`Intro.tnm` pipeline. GH1
  legacy venue intros are a separate `camera.dtb` -> external campath route.
- Theatre's `$camedit.INTRO` selects `Cam_t_np_zoom`, record name `Intro01`,
  path percentages 60 -> 0, duration 10000 ms, plus authored
  singer/offset/FOV values. `Intro01` is not a target object.
- The DTB reader now recognizes GH1 `.seq` files' four-byte zero plaintext
  wrapper. Theatre's `.seq` is an audio clap sequence, not camera selection.
- GH1 camera lookup now accepts `venues/<venue>/gen/campaths.rnd_ps2`.
  Theatre's path decodes to 68 keys and correctly retains 40 for its first 60%
  in reverse; basement retains all 88 for 0 -> 100 and still walks downstairs.
- Fixed camera solver ordering that discarded `offset_in/out` by refreshing
  the unoffset frame-target cache after applying them. Offsets now apply after
  cache refresh, before BuildTransform.
- Animated GH1 View7 objects now locate their embedded Trans8 after the legacy
  animatable payload. `main_curtains.view` no longer fails decode.

### 2026-07-22 GH1 regular-camera materialization

- GH1 regular cameras no longer remain empty after the six-bar INTRO window.
  `camera.dtb` `$camedit.<category>` switch records are decoded into the
  existing CameraManager categories. Theatre materializes 31 records and
  selects `flr_near_lft01x2w` at song time 22.217 seconds.
- The existing source category rotation already matches GH1:
  `flr_near_lft`, `flr_near_rt`, `flr_far_lft`, `flr_far_rt`, `band_POV`,
  `balcony_lft`, `balcony_rt`, `SOLO_NEAR`, `SOLO_FAR`.
- Camera ingestion smoke passes for all GH1 venue directories: arena 32,
  basement 24, big_club 37, fest 44, small_club 23,
  small_club_multi 3, theatre 31 regular records. Every venue except the
  intentionally shared small-club multiplayer variant also has a decoded
  INTRO path.
- Preserved exact GH1 source key `big_club`; it must not be normalized to
  GH2's RedOctane `big` alias. Geometry, 30/30 textures, INTRO (33 keys), and
  37 regular records now load for GH1 big_club.
- Regular GH1 `VenueCam` records use the venue-space `Cam_*.tnm` eye path but
  frame the singer. `flr_near_lft01x2w` is a short singer close-up; with the
  character layer absent, the drum kit behind the intended subject fills the
  image and falsely resembles an embedded camera.
- Debugging proves the current GH1-only run cannot resolve
  `singer::bone_spine1.mesh` for either the camera target or parent because
  `world/big_club/gen/big_club_chars.milo_ps2` does not exist in the GH1 ARK.
  The next camera-fidelity gate is therefore a source-backed GH1 performer
  placement/A-pose target (or the eventual GH2-character hybrid asset graph),
  not arbitrary camera-path scaling.

### 2026-07-23 GH1 native A-pose band and camera subjects

- GH1 songs leave quickplay character and band fields empty. The runtime now
  reads the first source character from `config/gen/characters.dtb` and GH1's
  source `band` tuple from `charsys/gen/charsys.dtb`.
- Logical band definitions are resolved through the authored
  `charsys/gen/band_chars.dtb` directory fields. Big club now loads native
  `metal`, `metal_singer`, `metal_bass`, and `metal_drummer` RndDir models.
- GH1 has no GH2 `<venue>_chars.milo_ps2`. Temporary A-pose placements are
  derived from each venue's native `characterLimits.mesh` quarter-grid:
  singer left-front, guitarist center-front, bassist right-front, drummer
  center-rear. The grid world transform supplies both exact placement and
  audience-facing basis.
- Big-club regular singer close-up is now correctly composed with its intended
  foreground subject; the previous apparent drum-kit collision was the view
  through an absent singer. Clean proof:
  `.codex/current-evidence/gh1-big-club-apose-band-facing/frame_00180.bmp`.
- Gameplay UI/highway can be suppressed while retaining performers by setting
  `GHOGX_DEBUG_VENUE_ONLY_CAPTURE` and `GHOGX_HIDE_GAMEPLAY_HUD` without the
  venue-only performer filter.
- Big-club INTRO eye-offset semantics are now source-backed: like the regular
  GH1 VenueCam records, interpolated `offset_in/out` corrects the moving camera
  eye and must not translate the singer target. Retail RedOctane Club footage
  confirms the baseline is a wide animated stage reveal. Local frames 60/300
  now produce the same coherent reveal with the venue animation active:
  `.codex/current-evidence/gh1-big-club-intro-eye-offset/`. The late path still
  reaches a red hall surface near frame 600, so intro completion/transition
  timing is not yet accepted.
- Visual fidelity is not complete: theatre and big_club reveal incorrect
  visibility/material/lighting results. These are current decoder/rendering
  targets, not accepted proofs.
- Regression requirement: GH2 venue intros include authored venue-object
  animation as well as CamShot/`Intro.tnm` camera motion. GH1 adapters must be
  source-format gated and preserve the complete native GH2 intro route.
- GH1 revision-10 subdirectories now draw from their authored root View:
  `venue.view` when present, otherwise `<directory-name>.view`. This removes
  big_club's unreferenced `crowd_limits*.mesh` editor/helper geometry while
  retaining the authored `lighting.view -> beam.view` beam path. Venue-only
  frame 30 confirms the opaque purple crowd-limit polygons are gone.
- Legacy View7 preliminary animation members are retained separately as
  `GroupObj::anim_children`. Big-club `lighting.view` contains eight such
  members, including event `.anim` Views and `spotlight01.tnm`; applying View
  frame propagation is now connected to the existing venue animation sampler.
- GH1 venue DTBs now load from `venues/<venue>/gen/<venue>.dtb`; function and
  `arena add_handlers` tables execute `switch_anim` loop/range/scale/blend and
  direct `set_showing` commands. Big-club loads 20 handlers. Its intro starts
  `fan.view`, music starts both cymbal TransAnims, and script visibility resolves
  after the GH1-gated post-lighting replay. GH2 script/intro behavior remains
  on its existing path.
- Preserved GH1 Drawable1 child lists on Mesh25 aggregates and recursively
  emit them during authored View traversal. Big-club now draws 153/159 meshes
  instead of 107/159; `main_hall.*` and other material-split sections visibly
  return while unreferenced editor/helper geometry remains excluded. Theatre
  smoke remains clean and now draws 203/262 authored meshes.
- GH1 Drawable1 aggregate roots now rebuild world transforms from local plus
  parent like native `WorldXfm_Force`; their children retain serialized world
  caches. This reconnects big-club's hall/floor sections. A broad all-local
  rule was tested and rejected because it moved theatre into its camera path.
  Permanent big_club and theatre frame-30 venue-only smokes both exit 0.

### 2026-07-23 resumed big-club intro endpoint audit

- Recovered the interrupted `Resume work` task from its final live turn without
  replaying the oversized thread. The authored GH1 big-club intro reaches camera
  frame 300, holds for 30 camera frames, then cuts to the first regular shot at
  frame 330; app capture frames include roughly one second of loading and must
  not be treated as camera frames.
- The triangular wedge visible at the frame-300 endpoint is not
  `main_hall.1.mesh`. Its center-ray intersection is a culled back face, and
  controlled frame-600 captures retain the wedge when either
  `main_hall.1.mesh` or the complete `main_hall*` family is omitted.
- The wedge also survives diagnostic removal of the `stage01` /
  `stage_flat_color` / `stage_floorm` family and `fan_cone*`. Continue
  identifying it from the authored draw list before changing camera or mesh
  semantics.
- A/B proof is in
  `.codex/current-evidence/gh1-big-club-endpoint-ab/`. Release `ghogx_app`
  builds successfully. No commit was made.

### 2026-07-23 theatre intro regression correction

- The big-club endpoint wedge is explicitly deferred at the user's request.
- Current theatre validation must retain the A-pose band while suppressing only
  the highway and meters. `--venue-only` removes the singer transform required
  by GH1 VenueCam framing and is not valid camera proof for this venue.
- A recent experiment had regressed GH1 `VenueCam` `start`/`end` handling by
  treating the values as raw TransAnim frames. Restored the documented native
  percentage semantics. Theatre's `60 -> 0` record now retains 41 keys
  (40 serialized samples plus the exact 60% boundary) instead of only 4 and
  traverses the segment in reverse.
- Release `ghogx_app` builds and the corrected theatre capture exits 0.
  Evidence:
  `.codex/current-evidence/gh1-theatre-percent-restored/`.
- Theatre is still not visually correct: venue/lighting geometry and the
  fallback A-pose band remain spatially disjoint. Continue with theatre
  hierarchy/placement transforms; do not tune the restored camera percentages.

### 2026-07-23 GH1 source-backed placement boundary

- User requirement: document GH1 formats thoroughly and do not ship one-off
  venue/character fixes. Per-venue coordinates, mesh-name placement tables,
  and camera compensation are prohibited. Use retail assets, recovered native
  behavior, and later PCSX2 traces as authority.
- Theatre's revision-10 venue RndDir has 262 Meshes, 30 Mats, one Cam, 33
  Views/Groups, and zero standalone Trans, Waypoint, or BandPlacer objects.
  Its `stage*` and `drum_riser*` meshes are drawable geometry, not serialized
  role assignments.
- Shared `charsys/gen/charsys.dtb` contains authored logical `band_spots`:
  bass 1, drummer 2, keyboard 0, singer 0. Theatre's script calls
  `char_sys get_spot guitarist0`, proving that a shared CharSys runtime lookup
  owns spot selection. The native mapping from logical indices to final world
  transforms is not yet recovered.
- `characterLimits.mesh` quarter-grid role assignment is a provisional
  diagnostic approximation, not verified native behavior. A tested
  theatre `stage.mesh` bounds fallback made the stage/camera composition
  coherent but was removed because no asset or native routine proves that
  relationship.
- Detailed evidence and acceptance boundaries are recorded in
  `docs/GH1_VENUE_COMPATIBILITY.md`. Extracted source artifacts:
  `.codex/analysis/gh1-theatre.dtb`,
  `.codex/analysis/gh1-theatre.dtb.dta`, and
  `.codex/analysis/gh1-system-charsys.dtb`.
- Release `ghogx_app` builds after removing the hypothesis. The broad
  `ghogx_gameplay_venue_band_contract_test` remains red on nine pre-existing
  source-text contracts spanning unrelated gameplay, animation, camera, and
  material work; do not cite it as validation for this change.
- Next authoritative step for placement is static recovery of
  `CharSys::get_spot` where possible, followed by bounded PCSX2 traces of the
  four final performer world transforms after usage resets.

### 2026-07-23 GH1 native stage-spot contract recovered

- Retail GH1 does not derive performer coordinates from visible stage bounds.
  Arena initialization probes numbered Mesh helpers using the embedded format
  `arena::stage_spot_%02d.mesh`, then copies/composes each helper's authored
  transform into a contiguous vector of `0x40`-byte records.
- Shared `charsys/band_spots` values are logical indices into these records.
  Native placement bounds-checks the selected index and applies the record.
  `CharSys::get_spot` is the complementary nearest-position query over
  `record+0x30`; it reports an already-established spot and does not generate
  placement.
- Exact static evidence and confidence boundaries are in
  `.codex/analysis/gh1-charsys-band-spots-static.md`; the user-facing format
  contract is in
  `GuitarHeroOGX-main-ui-engine/docs/GH1_VENUE_COMPATIBILITY.md`.
- No placement code was changed. Next inspect the decoded theatre object
  inventory for `stage_spot_%02d.mesh` and recover its parent/world transform
  chain. Do not fall back to `stage.mesh`, `characterLimits.mesh`, or
  venue-specific coordinates.
- Fresh theatre `milo_tool list` proof is
  `.codex/analysis/gh1-theatre-list.log`: all 396 revision-10 objects decode,
  but none is named `stage_spot` or `walk_spot`. The optimized retail
  representation/handoff is therefore still missing; do not synthesize it.

### 2026-07-23 GH1 native Arena placement integrated

- The missing theatre helpers are source objects in the loaded lighting
  section, not the main venue RndDir. Theatre lighting contains
  `stage_spot_01..03.mesh` and `walk_spot_01..03.mesh`.
- Retail native mapping is recovered end-to-end for a normal band:
  guitarist0 uses zero-initialized walk spot 0; singer/bass/drummer use shared
  `band_spots` indices 0/1/2. Runtime enumerates numbered helpers generically
  and merges lighting-section transforms before character load.
- Removed the provisional `characterLimits.mesh` quarter-grid and the
  basement-specific `riser.mesh` fallback. No venue-specific coordinates,
  mesh-bound sampling, or venue-name branches remain in GH1 placement.
- Helper visualization scale is removed while retaining authored axis
  directions and translation. Theatre positions are
  guitarist `(141.0,460.4,-15.7)`, singer `(18.1,437.3,-16.1)`,
  bassist `(-183.3,475.3,-16.1)`, drummer `(-3.8,598.3,3.5)`.
- Release app builds. Default-pose theatre proof with highway and HUD/meters
  disabled exits 0:
  `.codex/current-evidence/gh1-theatre-native-arena-spots-normalized/`.
- Next verify the same native helper contract across every GH1 venue lighting
  section, rebuild after parser cleanup, then capture another venue proof.
- Cross-venue inventory complete: basement, big_club, fest, small_club,
  small_club_multi, and theatre store stage spots 01-03 in lighting; arena
  stores stage spots 01-03 in its main RndDir. Every venue provides walk spot
  01 and therefore satisfies normal guitarist initialization.
- Second default-pose proof exits 0 with highway and HUD/meters disabled:
  `.codex/current-evidence/gh1-small-club-native-arena-spots/`. Its positions
  come from walk/stage helper transforms, but the capture also confirms
  unresolved small_club material/lighting fidelity (overbright fixtures and
  missing/incorrect surfaces). Placement is no longer the cause.
- Final release build after parser cleanup exits 0:
  `.codex/current-evidence/gh1-native-arena-spots-final-build.log`.
- Revision-10 renderer traversal now recursively expands the selected authored
  root View/Group and no longer appends ungrouped helper meshes. Arena's
  numbered stage/walk/fire/name-light spot families are non-draw in both main
  and section RndDirs. Build proof:
  `.codex/current-evidence/gh1-rev10-recursive-root-view-build.log`.
- Theatre remains visible and correctly placed after the root traversal
  correction:
  `.codex/current-evidence/gh1-theatre-recursive-root-view/`.
- Small-club still shows overbright lighting and large flat red/purple
  surfaces after helper/root correction:
  `.codex/current-evidence/gh1-small-club-recursive-root-view/`.
  Those defects are therefore not Arena placement helpers; continue with
  exact mesh/material identification and revision-10 material semantics.

### 2026-07-23 GH1 Mat21 selector-1 texture recovery

- Revision-21 GH1 Mats carry a texture-entry array. Retail small_club proves
  that selector 1, not only selector 0, is sampled 2-D material data:
  `smokemat`, `color_plane.mat`, and `spot_beam_mat` otherwise lose their only
  texture. Selector 5 is the sphere/environment family.
- The generic Mat21 reader now takes the first non-empty selector-0/1 texture
  and preserves selector 5 as environment data. Added focused decode coverage
  for selectors 1 and 5.
- Small-club lighting now loads 13/13 requested textures instead of 9/9.
  The large flat red/purple defects resolve into authored neon/translucent
  imagery:
  `.codex/current-evidence/gh1-small-club-mat21-selector1/`.
- `ghogx_app` links successfully. `ghogx_milo_scene_test` compilation succeeds
  but its executable still fails to link on the pre-existing unresolved
  public `decode_cam` symbol; log:
  `.codex/current-evidence/gh1-mat21-selector1-build.log`.
- The remaining red block is not the same material issue. A bounded native ELF
  xref proves Arena explicitly looks up `arena::target_parent.mesh`
  (`0x00311188`, xref `0x0016DDB0`) and consumes it through `0x00167E28`.
  Do not hide it by name; recover that native visibility/use path.
- Disabling approximate environment lights does not remove the central
  overbright fixture, so continue with additive material/texture-combiner
  semantics rather than tuning GH1 light values.

### 2026-07-23 GH1 native Arena helper lifecycle recovered

- Retail ELF `arena::target_parent.mesh` (`0x00311188`, xref `0x0016DDB0`)
  is looked up and passed to `0x00167E28`. That routine removes references via
  `0x00178FD8`, then invokes the object's virtual destructor with delete flags
  `3`. It is Arena-consumed runtime/transform data, not drawable geometry.
- The ELF also names zero-based `arena::crowd_limits%02d.mesh`. Runtime now
  classifies that complete family, plus the native stage/walk/fire/name-light
  spot families, as non-draw Arena helpers in main and section RndDirs.
- This is a generic retail naming/lifecycle contract, not a venue-specific
  red-block suppression.
- Release app builds successfully:
  `.codex/current-evidence/gh1-arena-helper-lifecycle-build.log`.
- HUD/highway-free small_club proof removes the solid red
  `target_parent.mesh` block without changing authored geometry or performer
  placement:
  `.codex/current-evidence/gh1-small-club-native-helper-lifecycle/`.
- Static record:
  `.codex/analysis/gh1-arena-helper-lifecycle-static.md`.
- Remaining small-club defect: central additive fixtures are overbright.
  Continue recovering GH1 additive texture-combiner semantics from source or
  retail static evidence; do not tune light values or substitute
  `SRCALPHA/ONE` without proof.

### 2026-07-23 GH1 native lighting-message contract recovered

- The small-club hotspot is `light_solo_opt.mesh`; removing
  `light_glow_solo.mesh`, the general cone/glow/bright families, spotlight
  instances, or approximate environment lights does not remove it.
- It uses `spot.mat`, `spot_beam.tex`, `kAdd`, and is submitted once.
  Extracted `spot_beam.tex` is fully opaque (8,192/8,192 alpha-255 pixels).
  This disproves the preliminary texture-alpha/additive-combiner hypothesis.
- Retail small_club DTB defines `OFF` as
  `((loop 99999 99999) (scale 1) (blend 0))` and sends
  `solo.anim OFF` from `set_lights_bad`.
- GH1 function and Arena-handler names intentionally overlap. They are now
  stored separately so a same-named message wrapper can call its function
  instead of overwriting it and recursing.
- `switch_anim` now accepts macro-bundled option arrays as well as inline
  rows. Degenerate ranges propagate static frames through revision-7 View
  Animatable members.
- Small-club `solo.anim` propagates to `solo.envanim`; frame 99999 drives
  `solo.env` to authored off state and removes the hotspot without hiding the
  beam mesh or changing blend factors.
- GH2 excitement/chart-section state now selects the native GH1 message
  family: bad, or okay/great verse/chorus/solo.
- Release build:
  `.codex/current-evidence/gh1-view-group-anim-build.log`.
- HUD/highway-free small_club and theatre proofs:
  `.codex/current-evidence/gh1-native-lighting-message-proofs-v2/`.
- Nondegenerate okay/verse validation proves View propagation reaches the
  authored environment and character/crowd LightAnim loops:
  `.codex/current-evidence/gh1-small-club-view-group-okay-verse/`.
- Format/runtime record:
  `.codex/analysis/gh1-venue-lighting-message-contract.md`.
- The broad venue/band source-contract executable still reports its existing
  unrelated string-contract failures; build succeeds and the failure log is
  `.codex/current-evidence/gh1-lighting-message-contract-test.log`.

### 2026-07-23 GH1 ParticleSys revision 22 recovered

- Added generic Animatable0, Trans8, Drawable1, and revision-22 payload/tail
  parsing. Arena stage flames prove the Animatable lists are variable-length.
- Two retail pre-material shapes are selected structurally by the following
  material reference, never by venue/object name.
- All 32 audited particles decode: basement 11, festival 3, theatre 12,
  arena 6. All bounded runs exit `0` with zero decode failures.
- Build: `.codex/current-evidence/gh1-particle-rev22-build.log`.
- Logs: `.codex/current-evidence/gh1-particle-rev22-runtime/`.
- Frames: `.codex/current-evidence/gh1-particle-rev22-proofs/`.
- Format record: `.codex/analysis/gh1-particlesys-revision22.md`.
- Big-club wedge remains deferred. Continue with the next generic GH1 gap.

### 2026-07-23 integrated GH1 playable/character proof

- Captured native GH1 guitarist, singer, bassist, and drummer in all seven GH1
  venue routes with only highway/HUD suppressed:
  `.codex/current-evidence/gh1-all-venues-character-playable-audit/`.
- All seven runs exit `0`; every role mesh and requested texture loads.
- Added a stronger deterministic seek/autoplay proof because sparse capture
  advances scene time faster than the streaming audio clock:
  `.codex/current-evidence/gh1-playable-integrated-proof/small_club/`.
- One process proves GH1 `hey.mid`, four-channel VGS, native small-club and
  lighting, native GH1 band, and GH2 scoring (`t=30.033`, score 50, streak 1,
  hits 1, misses 0). Highway and meters are absent.
- Non-guitarist GH1 role RndDirs lack later GH2 controller graphs. Default pose
  remains fact-backed; no controller/IK/viseme graph is fabricated.
- Record: `.codex/analysis/gh1-integrated-playable-proof.md` and
  `GuitarHeroOGX-main-ui-engine/docs/GH1_VENUE_COMPATIBILITY.md`.
- Camera/foreground framing still needs later native tracing. Big-club wedge
  remains deferred; do not hide geometry by name.

### 2026-07-23 regular GH1 VenueCam endpoints recovered

- Regular GH1 camera records no longer collapse their selected TransAnim
  segment to one nearest start key. The runtime retains the segment and
  evaluates offset, singer coordinate, and FOV endpoints over duration.
- Retail zero-duration records switch immediately to their `out` endpoint.
  Arena `flr_near_lft01x12w` proves this with offset out
  `(-410,-110,-320)`.
- The old in-state put the Arena camera inside `drum_riser.1.mesh` (renderer
  ray distance 7.3). The out-state restores the floor-left view without hiding
  or special-casing that mesh.
- Regular singer coordinates now use the same centered-coordinate conversion
  as INTRO: `(-0.5*x, 0.5*y)`. This fixes theatre's cropped close framing.
- Release build:
  `.codex/current-evidence/gh1-regular-venuecam-path-build.log`.
- Seven song-time, character-visible, HUD/highway-free proofs:
  `.codex/current-evidence/gh1-regular-venuecam-centered-singer-proof/`.
- Static/runtime record:
  `.codex/analysis/gh1-regular-venuecam-endpoints.md`.
- This removes one generic camera/foreground defect. PCSX2 traces remain
  useful for the unresolved placement/remaining camera audit; the big-club
  wedge remains deferred.

### 2026-07-23 regular GH1 VenueCam behavior fields recovered

- Extracted all seven retail camera DTBs and inventoried 201 regular records:
  `.codex/current-evidence/gh1-camera-dtb-inventory/`.
- Propagated fields that have exact GH2 CamShot counterparts:
  `hide_crowd`, `walk_ok`, `enable_dof`, `low_excitement_ok`, and
  `force_char_lod`.
- Arena `flr_near_lft01x12w` now hides 15 decoded crowd meshes on StartAnim
  and restores them on teardown, matching its retail `hide_crowd 1`.
- Seven HUD/highway-free, character-visible regression runs exit `0` and
  retain GH2 scoring:
  `.codex/current-evidence/gh1-regular-venuecam-fields-proof/`.
- Release build:
  `.codex/current-evidence/gh1-regular-venuecam-fields-build.log`.
- `shaky`, `force_cam_facing`, `crowd_region`, `eyes`, and `guard` remain
  uninterpreted until native semantics are proven. No guessed shake or facing
  behavior was added.

### 2026-07-23 GH1 VenueCam `real_time` recovered

- `real_time 1` camera durations are milliseconds and now map to
  `kTaskSeconds` at 30 camera frames/second.
- False/absent `real_time` durations retain their serialized tick count and
  map to `kTaskBeats` at 480 frames/beat, preserving tempo-dependent motion.
- Arena `balcony_lft01` proves the beat domain (7,680 frames, anim rate 1);
  `win01` proves the real-time domain (10,000 ms -> 300 frames, anim rate 0).
- Both forced-shot runs move along their native TransAnim paths while GH1
  characters remain visible and highway/meters remain hidden:
  `.codex/current-evidence/gh1-regular-venuecam-realtime-proof/`.
- Release build:
  `.codex/current-evidence/gh1-regular-venuecam-realtime-build.log`.
- Remaining uninterpreted regular fields are `shaky`, `force_cam_facing`,
  `crowd_region`, `eyes`, and `guard`.

### 2026-07-23 GH1 INTRO VenueCam schema unified

- All seven retail `$camedit.INTRO` records explicitly use `duration 10000`
  with `real_time 1`; the runtime now treats them as 10-second,
  300-camera-frame tasks through the established seconds domain.
- INTRO and regular `switch_cam` records now share one decoder. Common framing,
  clipping, time-domain, DOF, crowd, walking, excitement, and character-LOD
  fields can no longer drift between duplicate parsers.
- Venue-specific TransAnim paths and percentages remain retail data, including
  basement's complete 0 -> 100 stair path and theatre's reversed 60 -> 0 path.
- Release build:
  `.codex/current-evidence/gh1-intro-venuecam-build.log`.
- Seven character-visible, HUD/highway-free intro proofs:
  `.codex/current-evidence/gh1-intro-venuecam-proof/`.
- Format record:
  `.codex/analysis/gh1-intro-venuecam-format.md`.
- No behavior was invented for `shaky`, `force_cam_facing`, `crowd_region`,
  `eyes`, or `guard`. Big-club wedge and PCSX2 traces remain deferred.

### 2026-07-23 GH1 VenueCam `ease` recovered

- Retail `ease` is a float: 95 regular records use 0, 11 use 0.5, and 34 use
  1; omission defaults to zero.
- Static GH1 code proves zero constructs a 0x1C `LinearInterpolator`, while
  nonzero constructs a 0x3C `ATanInterpolator` and passes `ease` as severity.
- INTRO and regular records now feed that value to the existing source-backed
  CamShot arctangent evaluator in mode 0.
- Forced Arena `balcony_lft01` reports `t=0.925 -> eased_t=0.948` with
  `blend_ease=1`, while retaining the native band and hidden highway/meters.
- All seven regular-camera venue routes exit 0 after the change.
- Static record:
  `.codex/analysis/gh1-venuecam-ease-static.md`.
- Runtime proof:
  `.codex/current-evidence/gh1-venuecam-ease-proof/`.
- Release build:
  `.codex/current-evidence/gh1-venuecam-ease-build.log`.
- Remaining uninterpreted fields are `shaky`, `force_cam_facing`,
  `crowd_region`, `eyes`, and `guard`. The wedge and PCSX2 work remain
  deferred.

### 2026-07-23 GH1 VenueCam `shaky` static boundary

- `shaky 1` always looks up the shared `shaky_cam1.tnm`; false clears the
  active shake-animation pointer.
- Native code records the animation's task-time interval, advances it with
  blend 1 at the start of every VenueCam update, and clears it at end time.
- The retail object is in
  `../../system/run/arena/gen/fx.rnd_ps2`: RndTransAnim rev4, 12 spline
  translation keys, frames 0–3200, zero endpoints, X about -0.20..0.23,
  effectively-zero Y, and Z about -0.16..0.20.
- The object serializes no transform target. The recovered handler does not
  assign one, so camera binding/composition is still unproven.
- No guessed local/world additive shake was authored.
- Static/format record:
  `.codex/analysis/gh1-venuecam-shaky-static.md`.
- Extracted proof:
  `.codex/current-evidence/gh1-venuecam-shaky-static/`.
- Continue other static GH1 fidelity work now; resolve this binding during the
  later bounded PCSX2 trace pass if static evidence remains insufficient.

### 2026-07-23 GH1 VenueCam `crowd_region` static contract

- `crowd_region` is read as an integer with default `-1` and passed to native
  Crowd `switch_region`.
- Nonnegative values select the zero-based authored region. Negative values
  invoke camera/projection-based automatic selection.
- Native selection clears and repopulates every crowd-archetype instance list
  from the selected region; it is not a visibility index.
- Retail crowd sections pair numbered `crowd_limits%02d.mesh` placement
  helpers with five `Crowd*.mm` archetypes. Theatre and festival each expose
  12 limit regions; basement exposes 15.
- Current compatibility decodes/draws serialized MultiMesh instances and
  correctly keeps the limit helpers non-draw, but does not yet rebuild native
  region-specific instance transforms.
- No per-venue region table or MultiMesh hide/show workaround was added.
- Static/asset record:
  `.codex/analysis/gh1-venuecam-crowd-region-static.md`.
- Bounded inventories:
  `.codex/current-evidence/gh1-crowd-region-static/`.
### 2026-07-23 GH1 revision-10 material-boundary audit

- The big-club wedge remains deferred.
- Theatre’s bright floor rectangle is retail `band_shadow1.mesh` using its
  serialized `19 - Default.mat` reference, not a leaked stage/walk helper.
- All seven venue main ObjectDirs plus GH1 Metal show the same current static
  symptom: the last Mat is sized like the first Mesh and that Mesh is size 0.
- A generic first-Mat-body omission model made those sizes align but severely
  corrupted venue and character materials in the integrated runtime. It was
  removed; no one-off remap or object-name suppression was retained.
- Release `milo_test`, `milo_tool`, and `ghogx_app` rebuild; `milo_test` passes.
  Restored theatre gameplay remains playable at song time 30 with highway and
  meters hidden, score 150/streak 3 after 63 frames.
- Full format record:
  `.codex/analysis/gh1-revision10-material-body-association.md`.
- Evidence:
  `.codex/current-evidence/gh1-rev10-mat-body-omission/` and
  `.codex/current-evidence/gh1-rev10-mat-body-omission-rejected-build.log`.
- Next: recover a class-aware Mat21 extent/transition rule or defer the exact
  native association until PCSX2 tracing is available. Do not shift all Mat
  identities, hide `band_shadow1.mesh`, or add venue-specific material logic.
### User-observed GH1 proof defects

- Singer microphone stand was observed upside down and is now resolved by the
  source-backed role-owned singer ACP path documented below; no mic-specific
  transform correction was required.
- The top half of the singer's face is missing or has inverted polygons.
- Metal's wrist is incorrectly placed.
- The face and wrist observations are recorded but deferred under the current
  allowance for GH1 characters to remain in their default pose. Do not convert
  any of these observations into object-name suppression or pose-specific
  offsets; recover their native mesh/transform facts when they enter scope.
### 2026-07-23 GH1 Mat21 provenance correction and compact state

- The earlier apparent last-Mat/first-Mesh shift was produced by a stale
  `milo_tool` binary. Rebuilt current source aligns all seven venues and Metal;
  theatre `19 - Default.mat` natively contains `band_shadow.tex`.
- The rejected first-Mat omission rule remains removed.
- `tex_tool` now generically accepts extracted revision-8/10 Milo `.tex`
  entries as well as bare PS2 bitmaps.
- Mat21 now decodes compact `use_environ`, `prelit`, and `z_mode` bytes instead
  of forcing constructor lighting defaults. Small-club and theatre show a
  material/lighting improvement.
- Additive venue rendering now weights straight-alpha PS2 texture RGB with
  `SRCALPHA`. This removes theatre's bright `band_shadow1.mesh` rectangle
  without a mesh/material/venue exception.
- All seven HUD/highway-free playable runs exit 0 at song time 31.033 with
  score 150/streak 3:
  `.codex/current-evidence/gh1-mat21-compact-state/cross-venue/`.
- Release app and texture tool build; `ps2_texture_test` passes. The broad
  Milo-scene test retains its pre-existing obsolete `decode_cam` link failure.
- Format record:
  `.codex/analysis/gh1-revision10-material-body-association.md`.
- Remaining venue-side review includes the source-authored small-club pale
  plaster panels. The singer mic stand is resolved by the later role-owned ACP
  correction. Big-club wedge remains deferred; face/wrist character defects
  remain recorded and deferred.
### 2026-07-23 GH1 mic-stand placement audit

- `mic_stand.mesh` is an embedded, palette-free singer mesh parented directly
  to `bone_pos_mic.mesh`; it is not venue geometry or an external prop.
- GH1 and GH2 Metal singer archives serialize identical mic geometry, parent,
  local transform, and stored-world transform.
- The GH2 archive renders the stand upright in the isolated character viewer.
  Integrated gameplay produces a negative final mic Z basis and places the
  circular base at the singer's waist.
- Theatre's normalized singer Arena spot has basis rows
  `(-1,0,0)`, `(0,-1,0)`, `(0,0,1)` at the authored stage-spot translation.
  The defect is localized to the generic character-space/Arena-spot handoff,
  not GH1 mic decoding.
- No mic-name rotation, vertex edit, or venue coordinate was added. Record:
  `.codex/analysis/gh1-mic-stand-transform.md`.
### 2026-07-23 small-club white-panel identification

- The left pale panel picks as retail `main_room_stage.1.mesh` with
  `plaster_wall.mat`.
- Current rebuilt extraction correctly resolves `plaster_wall.mat` to embedded
  `plaster.tex`; the decoded 128x128 indexed source is predominantly pale,
  chipped plaster.
- Its Mat21 state is coherent: use-environ 1, prelit 1, z-mode normal, blend
  source. No missing-texture fallback is involved.
- No hide/recolor/lighting exception was added. Native prominence remains a
  later comparison question. Record:
  `.codex/analysis/gh1-small-club-white-panels.md`.
### 2026-07-23 GH1 native role-owned ACP resolution

- The earlier mic-stand inversion did not require a character/Arena basis
  correction. The isolated viewer had a clip pose; gameplay had failed to
  resolve singer animation and was composing the stored bind chain.
- The retail GH1 ARK catalog proves guitarist ACP stems are character-owned,
  while singer, bassist, and drummer stems are role-owned: `singer_*`,
  `bassist_*`, and `drummer_*`. Female singer retains `female_singer_*`.
- Gameplay now resolves those retail paths generically by performer role. It
  does not inspect mic, mesh, venue, or individual character names, and does
  not fabricate absent intro clips.
- Native singer animation drives `bone_pos_mic.mesh`; the mic's final Z basis
  is positive and the stand is upright with its base on the floor. No
  mic-specific rotation, vertex edit, venue offset, or Arena-basis override
  was added.
- All seven GH1 venue proofs exit 0 at `hey` song time 31.033, score 150,
  streak 3, with highway/meters hidden:
  `.codex/current-evidence/gh1-native-role-acp-cross-venue/`.
- Format records:
  `.codex/analysis/gh1-role-acp-resolution.md` and
  `.codex/analysis/gh1-mic-stand-transform.md`.
- The singer upper-face and Metal wrist observations remain logged and
  deferred character issues. Big-club wedge remains deferred. PCSX2 traces
  can wait for usage reset.
### 2026-07-23 GH1 VenueCam translation and multiplayer categories

- The `small_club_multi` 90-degree roll was the raw `6 foot camera.cam`
  fallback, not a venue mesh transform. The selector searched GH2 normal
  categories, found zero records, and never started a VenueCam shot.
- Retail `small_club_multi/camera.dtb` contains only `MULTIPLAYER`,
  `MULTIPLAYER_0`, and `MULTIPLAYER_1` regular buckets. These three source
  symbols now participate in the legacy regular-camera category order; there
  is no venue-name check or fixed roll.
- Native `VenueCam::Update` projects the singer and converts viewport error
  into camera-local positional translation before adding `offset_in/out`.
  INTRO and regular GH1 records now use the translation route instead of
  rotating the camera toward the screen coordinate.
- A raw TransAnim-basis preservation experiment looked away from the venue
  because these paths rely on the established camera builder; it was removed.
- Final `small_club_multi` unforced proof selects `multi01` and is upright.
  All seven venues pass at both intro time 1.033 and regular time 31.033:
  `.codex/current-evidence/gh1-venuecam-translation-category-final/`.
- Release app and focused contract-test targets build. The broad contract test
  still reports unrelated pre-existing stale source-string expectations; the
  new multiplayer-category assertion is not among its failures.
- Format record:
  `.codex/analysis/gh1-venuecam-screen-translation-and-multiplayer-categories.md`.
### 2026-07-23 GH1 venue animation completion gate

- User explicitly requires every GH1 venue to retain movement and life like a
  GH2 venue. Static geometry/material/camera correctness is insufficient.
- Completion now requires intro and post-intro object animation, View
  propagation, environment/light animation, particles, visibility, and
  excitement/section transitions.
- The seven venue DTBs expose 115 handlers and 80 functions in total. Runtime
  inventory finds 360 TransAnims, 2 MeshAnims, and 29 ParticleSysAnims across
  the seven current assemblies.
- Fixed-camera, HUD/highway/character-free proofs under native
  `excitement_great` show `set_lights_great_verse`, active retail animation
  loops, and rendered scene changes in all seven venues.
- Time-separated settled-frame comparisons change between 19% and 91% of BMP
  pixel bytes depending on venue/state. This establishes ongoing rendered life,
  not exact timing fidelity.
- Evidence:
  `.codex/current-evidence/gh1-venue-animation-route-inventory/`,
  `.codex/current-evidence/gh1-venue-animation-time-separated-audit/`, and
  `.codex/current-evidence/gh1-venue-animation-great-state-audit/`.
- Remaining animation work is exact intro/post-intro timing, all retail
  excitement/section transitions, continuous object frame domains, visibility,
  and particle transitions. Do not author per-venue motion or force lively
  frames.
- Full gate:
  `.codex/analysis/gh1-venue-animation-completion-gate.md`.

### 2026-07-23 GH1 retail `switch_anim_rt` animation route

- Exact retail-DTB/runtime inventory found a generic parser gap:
  `arena switch_anim_rt` occurs 80 times across arena, basement, festival,
  and theatre.
- Runtime now recognizes both retail `switch_anim` spellings, preserves the
  `_rt` fact on `VenueScriptStep`, and sends both through the common decoded
  animation-reference route. There are no venue/object-name exceptions.
- This restores basement's entirely `_rt` `anim_great` function. Basement
  inventory rises from 15 handlers/9 functions to 18/10; a deterministic
  great-state run executes 26 `_rt` statements.
- Release `ghogx_app` builds. Fixed-camera, venue-only basement proof exits 0;
  settled frames 121/181 differ by 0.898472 of pixel bytes.
- Evidence:
  `.codex/current-evidence/gh1-venue-animation-route-inventory/*/switch-anim-rt.log`
  and
  `.codex/current-evidence/gh1-venue-animation-switch-anim-rt-basement/`.
- Format record:
  `.codex/analysis/gh1-venue-script-switch-anim-rt.md`.
- Remaining exact script gap: theatre `curtain_open`/`curtain_close` use the
  generic global `animate_to` task. Do not replace it with hard-coded curtain
  movement.

### 2026-07-23 GH1 retail `animate_to` View route

- Retail inventory corrected the scope: `animate_to` occurs five times across
  Theatre and Festival, targeting `main_curtains.view` and
  `reactor_set.view`; it is not a curtain-specific command.
- The parser now preserves the target View, destination frame, and retail
  millisecond period in a generic `AnimateTo` step.
- GH1 View7 decoding now preserves the animation-reference vector inside its
  actual revision-0 `RndAnimatable` payload. The extracted retail
  `main_curtains.view` authors four curtain TransAnim members; Festival's
  `reactor_set.view` independently authors `reactor.tnm`, `reactor2.tnm`, and
  `cloud.tnm`.
- Runtime View propagation resolves decoded member relations, with a generic
  authored drawable-membership/TransAnim-target route for merged scenes. No
  venue, curtain, reactor, mesh, or animation names are inspected.
- Interrupted transitions start at their sampled current frame. Duration is
  derived from the retail period, not a hand-tuned speed.
- Single-job release app build exits 0. Theatre `excitement_great` resolves
  four filters and starts frame 0 -> 2500 over 5.000 seconds; at 2.983 seconds
  all four sample frame 1500. Proof:
  `.codex/current-evidence/gh1-animate-to-retail/theatre-member-run.log`.
- Festival `excitement_great` exits 0 and resolves the View directly to three
  filters, starting frame 0 -> 50 over the retail 3.840 seconds. Proof:
  `.codex/current-evidence/gh1-animate-to-retail/festival-final.log`.
- Format record:
  `.codex/analysis/gh1-venue-script-animate-to.md`.
- This closes the named `animate_to` script gap, but the broader all-venues
  exact timing/state/visibility/particle completion gate remains open.

### 2026-07-23 GH1 bad/okay/great verse-state transition audit

- All seven venues were run sequentially in low-resource mode for
  `set_lights_bad`, `set_lights_okay_verse`, and
  `set_lights_great_verse`.
- Every one of the 21 fixed-time, venue-only runs exited 0.
- Each venue produced three distinct deterministic settled-frame hashes,
  proving visible authored state differentiation rather than handler
  presence alone.
- Arena and Basement debug runs reported zero unresolved references across
  all three states.
- This completes the verse-state portion of the transition matrix. Chorus,
  solo, exact timing, visibility, and particle parity remain open.
- Evidence record:
  `.codex/analysis/gh1-venue-verse-state-transition-audit.md`.

### 2026-07-23 GH1 chorus/solo transition audit

- Corrected diagnostic proof semantics: a forced venue event is no longer
  immediately replaced by normal chart-category lighting selection.
  Non-diagnostic retail gameplay is unchanged.
- Corrected chorus/solo runs exit 0 and render distinct states in Arena,
  Basement, Big Club, Festival, and Small Club.
- Small Club Multi has distinct chorus states but convergent okay/great solo
  frame-121 output; source confirmation remains open.
- Theatre executes every requested function with zero unresolved references,
  samples the authored environment-animation domains and differing colors,
  but all four chorus/solo captures are pixel-identical.
- The next fact boundary is GH1 revision-1 Environ ownership. Theatre Environ
  bodies contain a variable auxiliary mesh-reference region; current decoding
  reaches the ambient/fog tail but does not preserve that ownership, leaving
  the animated environments with zero decoded lights.
- Do not assign Theatre lights or meshes by name. Decode the generic retail
  Environ ownership semantics.
- Record:
  `.codex/analysis/gh1-venue-chorus-solo-transition-audit.md`.

### 2026-07-23 GH1 revision-1 Environ drawable ownership

- Preserved Harmonix source and raw Theatre objects establish that GH1
  Environ revision 1 loads an inherited RndDrawable revision 1 payload before
  its old light vector: showing byte, drawable-reference vector, and sphere.
- The decoder now follows that exact order and preserves the authored
  drawable traversal. The renderer queues those referenced meshes through
  the showing Environ, selects that Environ for their materials, and does not
  require each traversed mesh to be independently showing. There are no venue
  or mesh-name exceptions.
- A single-job release app build succeeds.
- Correct explicit-HDR/ARK Theatre frame-121 hashes are now distinct for okay
  chorus, great chorus, and solo. Okay/great solo converge on the shared
  authored solo presentation. This closes Theatre's missing visible
  chorus/solo route.
- Small Club Multi's converged solo capture is source-confirmed: okay/great
  address the same three EnvAnims with scale 1 versus 2; logs show distinct
  frames but constant sampled colors at the capture.
- Format record:
  `.codex/analysis/gh1-environ-revision1-drawable-format.md`.
- Proof hygiene correction: `--ark-dir` unexpectedly failed to find the GH1
  MIDI in this run, while explicit `--hdr`/`--ark` entered `state=playing`.
  Discard the resulting splash hashes; the Theatre result above uses the
  explicit files.

### 2026-07-23 GH1 intro/post-intro venue-animation audit

- All seven retail venue DTBs define `intro_start`; none defines a separate
  `intro_end`. The post-intro presentation is established by the normal
  section/excitement transition.
- Fourteen sequential fixed-time, venue-only runs cover song time ~1.0 and
  ~31.0 for all seven venues. Every run exits 0 and every venue produces
  distinct intro/post-intro output.
- Debug reruns show the decoded `intro_start` followed by `set_lights_bad`
  after the intro window, with zero unresolved-reference lines once the full
  venue plus lighting assembly is available.
- The pre-lighting and post-lighting script applications occur at the same
  song time within one load operation; the second resolves lighting-owned
  objects before any rendered frame.
- This closes the time-separated intro/post-intro visible-state gate. Exact
  continuous frame domains, visibility/particle transitions, and later PCSX2
  comparison remain.
- Record:
  `.codex/analysis/gh1-venue-intro-post-intro-audit.md`.

### 2026-07-23 GH1 MIDI sections, visibility, and particle switches

- GH1 EVENTS tracks use bare `verse`, `chorus`, and `solo`; the runtime
  previously recognized only GH2 bracketed forms. The common decoder now
  accepts both, restoring normal-play GH1 section handlers and lighting
  categories.
- The shared retail chorus condition
  `{> $arena.excitement kExcitementBad}` is now parsed and evaluated as a
  typed condition. It remains off at bad excitement.
- Direct `.partanim` switches were decoded but never activated by the script
  dispatcher. Generic direct particle maps now cover both venue and lighting
  archives and preserve range/loop, scale, blend, target, and decoded tracks.
- Five particle-authoring venues bind their startup switches with zero
  unresolved references. Big Club and Theatre author none.
- Arena autoplay reaches bare `chorus` at tick 49920 / 47.9 seconds, evaluates
  excitement 3 > 1, hides `flare_obstruct.view`, starts four fire animations,
  and samples changing emission intensity through frame 363.
- Small Club independently starts both dry-ice systems on retail loop
  1..15360 and advances their samples.
- Seven-venue intro visibility runs resolve every authored `set_showing`
  target after the full venue/lighting assembly is loaded.
- Single-job release build succeeds. Format record:
  `.codex/analysis/gh1-midi-sections-and-particle-switch-format.md`.

### 2026-07-23 GH1 View7 mixed animation-member propagation

- Retail `_rt` inventory is 80 statements, of which 32 have moving
  loop/range endpoints. Arena and Theatre executed targets decode at source
  rate 0 (`kTaskSeconds`, 30 fpu); no opcode-name frame-rate override was
  invented.
- Theatre `stage.anim` contains a serialized RndAnimatable member vector with
  EnvAnim, MatAnim, and ParticleSysAnim children. Only the EnvAnim child had
  been dispatched.
- The generic View-member route now starts direct particle and material
  children while preserving the parent switch range/loop, scale, blend, and
  source rate. No venue/object names are special-cased.
- GH1 MatAnim revision 5 per-stage symbol keys are decoded as authored
  texture keys for material stage zero. `VenueMaterialAnim` retains inherited
  RndAnimatable rate.
- Theatre great chorus now starts three flame MatAnims and ten fire particle
  systems. Fixed frames 10 and 60 differ; single-job release build succeeds.
- Record:
  `.codex/analysis/gh1-view-animatable-member-format.md`.

### 2026-07-23 GH1 persistent event/task lifetime

- Persistent excitement was polled every frame, but duplicate suppression
  occurred after Arena script dispatch. Basement `excitement_great` ran 531
  times in one 60-second proof and continuously reset its loops.
- Old persistent animation state was also cleared after the replacement
  script ran, deleting the new script-created tasks in the same call.
- Persistent duplicate suppression and old-state cleanup now both precede
  replacement script dispatch. Explicit camera `resend_excitement` still uses
  the authored force-replay path.
- Basement's rate-0 furnace loop now advances through frames 18.0..49.5 with
  changing rotation. Theatre's flame MatAnim advances through frames 3..108,
  and its forced great-chorus event runs once.
- Single-job release build succeeds. New ordering contract assertions pass;
  the broader contract executable retains unrelated pre-existing failures.
- Record:
  `.codex/analysis/gh1-venue-persistent-event-task-lifetime.md`.

### 2026-07-23 GH1 delayed tasks and source-null animation routes

- Festival `delay_task` arguments are Arena k480_fpb beat-frame units.
  `1200` becomes 2.5 chart beats and resolves through the tempo map.
- GH1 local script state such as `$reactorState` is serialized as DTB
  Variable tag `0x02`, not Property tag `0x13`. Both now use the retained
  common state map.
- Festival great-state proof schedules the delayed task, passes the authored
  equality guard, starts `nuke_toxic.partanim`, and advances
  `blast.matanim` through the decoded `_rt` route.
- Basement `plaster.partanim` is a source-null ParticleSysAnim. Arena
  `full.anim` and Theatre `great_verse.anim` are View7 objects with empty
  animation-member vectors. All resolve as authored SetFrame no-ops; no
  target is guessed from the object name.
- Focused Arena, Basement, Festival, and Theatre runs exit 0. Post-assembly
  moving routes now have zero unresolved targets.
- Builds succeed. New focused contract assertions pass; the broad contract
  executable still reports unrelated pre-existing failures.
- Record:
  `.codex/analysis/gh1-venue-delay-task-and-null-animation-format.md`.

### 2026-07-23 GH1 MTV song-opening overlay

- `ghui/gen/game.dtb::game::intro_start` hides `mtv_overlay`, then schedules
  `show_overlay TRUE` with the authored 1000-millisecond delay.
- Legacy `LabelEx` is now decoded through its retail `BandLabel` mapping.
  GH1 Trans begins at body offset `0x04`; its revision-6 text tail supplies
  bounds, leading, alignment, and type-resource identity.
- Label resources and colors come from `ghui/gen/config.dtb`. Named Font,
  Mat, and Tex selection comes from `ghui/gen/resources.rnd_ps2`; GH1 Font
  revision 7 correctly omits later Hmx Object metadata.
- The implementation is now the shared `SongIntroOverlay` used by both direct
  gameplay and the normal menu-to-gameplay render path.
- Normal GH2 menu boot can mount a separate content HDR/ARK. GH2 retains
  front-end configuration while ConfigDb replaces only its song table; chart,
  audio, venue, band, and GH1 overlay resolve from the content archive.
- The focused retail-archive format test passes. A highway/HUD/performer-free
  `hey` proof shows no overlay before the delay and the title/caption/artist
  after it; the log records first submission at `t=1.000`.
- A normal-menu layered proof mounts the GH1 57-song catalog, loads `hey`,
  enters gameplay through `loading_screen`, and shows the overlay at
  `t=1.000`; its accepted frame suppresses highway and meters.
- Record:
  `.codex/analysis/gh1-mtv-song-overlay-format.md`.

### 2026-07-23 GH1 opening presentation retail-path closure

- The stale opening-camera to-do predated the completed generic GH1 VenueCam
  adapter. Current source still resolves all seven `$camedit.INTRO`
  `switch_cam` records through native `campaths.rnd_ps2` TransAnim paths.
- The layered normal-menu `hey` proof logs Theatre
  `Cam_t_np_zoom.tnm`, 41 reverse-path keys, source duration 10000 ms, and the
  six-bar intro window.
- Highway and meters are suppressed. Retail-menu frames 50 and 100 visibly
  advance from a wide authored composition to the closer reverse-path
  composition; they have distinct SHA-256 hashes.
- Together with the native MTV overlay at `t=1.000`, this closes the song
  opening presentation item. It does not close broader lighting or character
  fidelity review.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/.codex/current-evidence/gh1-mtv-song-overlay-retail-menu/`
  and
  `GuitarHeroOGX-main-ui-engine/.codex/current-evidence/gh1-opening-camera-retail-menu/`.

### 2026-07-23 GH1 performer lighting environments

- All GH1 performers previously logged `environment=<none>` because the
  runtime searched only the GH2 venue-character proxy scene. GH1's performer
  Environs actually live in each venue's lighting RndDir.
- Every GH1 venue lighting directory owns `stagechar.env` plus singer
  environments. Retail venue functions select those dynamically through
  `arena::set_singer_env`.
- The generic legacy script path now retains that operation. Singer uses the
  currently selected source Environ; the other roles use `stagechar.env`.
  Small Club Multi has no singer selector and source-falls back to its shared
  `stagechar.env`.
- Environs are applied through the lighting renderer which owns them. No RGB,
  exposure, venue, song, or character-name correction was added.
- The release build succeeds. All seven venue runs exit zero and explicitly
  apply a decoded source environment to all four roles. New focused contract
  assertions pass; the broad executable retains unrelated pre-existing
  failures.
- Keep the broader lighting review open until later PCSX2 comparison validates
  device-light interpretation and exposure.
- Record:
  `.codex/analysis/gh1-performer-lighting-environment-format.md`.
### 2026-07-23 GH1 venue function/list-control recovery

- The GH1 venue interpreter now retains `func` DataVariable parameters,
  call arguments, scoped variable substitution, `foreach`, switch expressions,
  switch statements, and authored `random_int` animation-range endpoints.
- GH2 `hit_p0_fret1..5` presentation events dispatch GH1's authored
  `hit_gem(slot)` message with zero-based slot `0..4` only when a GH1 venue
  script is active. This uses the shared note-event route rather than a
  song/venue/object branch.
- Festival proof
  `.codex/current-evidence/gh1-function-foreach-fest/fest-fret1-final.log`
  executes `hit_guitar_amp`, both speaker helpers, and native cone1-4 TransAnim
  routes with authored random ranges.
- Seven-venue inventory
  `.codex/current-evidence/gh1-function-foreach-seven-venue/` exits zero for
  every venue. Unsupported source families are down to 2-4 per venue:
  `add_trans`, `game anim_task`, `flare set_steps`, and Basement-only
  `if_else`/`with_namespace`.
- Format record:
  `.codex/analysis/gh1-venue-script-functions-foreach-switch.md`.
- The broad venue/band contract executable still exits 1 on its established
  unrelated source-string failures; none of the new function, iteration,
  switch, random-range, or `hit_gem` contracts are reported missing.
### 2026-07-23 GH1 `game anim_task` and legacy MatAnim recovery

- GH1's extracted `system_script.dtb` proves `game anim_task` argument order
  as `(animatable, period, from, to)`. Venue periods are milliseconds.
- The interpreter now retains explicit start/end frames and computes native
  AnimTask scale as `span / (period_seconds * frames_per_unit)` using the
  target's decoded RndAnimatable rate.
- Theatre revealed `grey_cone_anm.mnm` is a revision-5 MatAnim. The legacy
  stage block's translation, scale, rotation, and texture tracks are now
  retained for authored stage zero instead of consuming the first three
  without rendering them.
- Proof
  `.codex/current-evidence/gh1-anim-task-runtime/theatre-matanim-final.log`
  executes both the `kickdrum01.tnm` TransAnim and `grey_cone_anm.mnm` MatAnim
  from `kick_drum` and exits zero.
- Seven-venue inventory
  `.codex/current-evidence/gh1-anim-task-seven-venue/` exits zero throughout
  and no longer lists `game anim_task` as unsupported. Remaining script
  families are `add_trans`, `flare set_steps`, and
  `if_else`/`with_namespace`.
- Format record:
  `.codex/analysis/gh1-game-anim-task-and-legacy-matanim-format.md`.

### 2026-07-23 GH1 `RndTransformable::add_trans` recovery

- Static analysis of retail `GH1/SLUS_212.24` recovered the `add_trans`
  handler at `0x001DC1A0` and its graph mutation at `0x001DA100`.
- Native behavior appends the child to the parent's transform-child list,
  marks the child dirty, and preserves the child's local matrix. It is not a
  preserve-world reparent operation.
- The generic venue interpreter now retains each authored parent/child pair.
  The renderer recursively resolves the resulting graph across Mesh, Group,
  Trans, Light, and Cam objects, including the selected live venue camera.
- Festival proof
  `.codex/current-evidence/gh1-add-trans/fest.log` exits zero, reports zero
  unsupported venue-script operations, and executes both authored camera
  children with `local_preserved=1`.
- Seven-venue inventory
  `.codex/current-evidence/gh1-add-trans/seven-venue/` exits zero throughout.
  Big Club and Festival have no unsupported operations; Arena, Small Club
  Multi, and Theatre retain only `flare::set_steps`; Basement retains
  `if_else`/`with_namespace`; Small Club retains `flare::set_steps` and
  `with_namespace`.
- No venue, song, light, or mesh name branch was added.
- Remaining unsupported venue-script families are `flare set_steps` and
  Basement-only `if_else`/`with_namespace`.
- Format record:
  `.codex/analysis/gh1-rndtransformable-add-trans-format.md`.

### 2026-07-23 GH1 RndFlare and `set_steps` recovery

- Harmonix source proves `set_steps` is the
  `RndFlare::Handle -> SetSteps(_msg->Int(2))` action, not an Animatable frame
  command.
- Native GH1 revision-3 Flare entries now decode their embedded revision-8
  Transform, revision-1 Drawable, material, size pair, distance range, and
  authored step count in source order.
- Rendering submits those authored objects as depth-tested camera-facing
  flares with source distance-size interpolation. The generic venue script
  path resolves foreach targets and publishes exact `0`, `180`, or `360`
  step values from each venue DTB.
- Theatre proof
  `.codex/current-evidence/gh1-flare-set-steps/theatre-chorus.log` renders all
  eight flares, executes authored up/down step values, exits zero, and reports
  no unsupported operation.
- Seven-venue proof
  `.codex/current-evidence/gh1-flare-set-steps/seven-venue/` exits zero
  throughout. Arena, Big Club, Festival, Small Club Multi, and Theatre now
  have zero unsupported script operations. Remaining operations are
  Basement `if_else`/`with_namespace` and Small Club `with_namespace`.
- Format record:
  `.codex/analysis/gh1-rndflare-format-and-set-steps.md`.

### 2026-07-23 GH1 DataArray control and namespace completion

- Harmonix `DataIfElse` source proves one condition and two lazily evaluated
  alternatives. Basement's tutorial branch now retains that exact shape.
- DataArray `exists` now checks the script-object/live-performer registry
  instead of the unrelated task scheduler.
- `with_namespace {performer geom_space}` now scopes nested lookup to that
  decoded character ObjectDir and restores the previous namespace afterward.
  Character View/Group visibility resolves recursively to its authored member
  meshes.
- Regular-camera proofs keep highway/meters suppressed but performers loaded:
  `.codex/current-evidence/gh1-data-control/basement-regular-camera.log`
  resolves singer `top.view`, and
  `.codex/current-evidence/gh1-data-control/small-club-regular-camera.log`
  resolves drummer `top.view`. Both exit zero with `failed=0`.
- Seven-venue inventory
  `.codex/current-evidence/gh1-data-control/seven-venue/` exits zero throughout
  and every venue reports zero unsupported script operations.
- This completes the currently inventoried GH1 venue-script operation surface.
  It does not close later PCSX2 lighting review or broader visual acceptance.
- Format record:
  `.codex/analysis/gh1-dataarray-if-else-with-namespace.md`.

### 2026-07-25 GH1 character instrument scope

- User direction: instruments receive only a cursory compatibility pass during
  GH1 character work.
- GH2 instruments remain authoritative. If a GH1 instrument is difficult to
  decode or reproduce, use the GH2 counterpart rather than delaying character
  support.
- Do not spend the current goal on GH1 instrument format, visual, animation,
  attachment, or fidelity work unless a minimal check is required to establish
  compatibility.

### 2026-07-25 GH1-to-GH2 asset conversion evaluation

- Preserve an explicit future evaluation of converting GH1 venues, characters,
  and animations into GH2-native formats instead of treating runtime
  cross-revision loading as the assumed final architecture.
- Compare fidelity, mixed-content behavior, maintenance, archive integration,
  and packed-fact traceability.
- Any converter must be deterministic and systemic, not a collection of
  hand-authored or object-specific asset fixes.

### 2026-07-25 GH1 face and animation runtime

- GH1 guitarist RndMorph faces now use packed excitement pools, authored
  0.5-second blend/1-second hold values, the recovered source Rand generator,
  and the retail no-repeat selection rule.
- A live retail PCSX2/PINE trace recovered the singer producer. The
  `I Love Rock & Roll` face list contains 392 sixteen-byte rows whose beat
  starts/ends exactly match all 392 `T1 GEMS` pitch-108 note spans. The runtime
  now retains those spans, adds packed `event_offset=257` ticks to the query,
  blends to packed pose `open` while a row is active, and returns to `ref`.
  This path is gated by the GH1 packed `event_list=singer` contract; GH2
  singers do not enter it.
- Character `*_anims.dtb` files are matched to ACP inventories structurally.
  Bad/Normal/Extreme and band Active/Idle/Win/Lose selection now comes from
  authored flags and tempo eligibility instead of filename fragments.
- All eight GH1 guitarists match 76-81 flagged ACP definitions and enter their
  authored `kGuitarBad` clips in focused runs. Metal transitions to authored
  `kGuitarExtreme`; singer/bassist/drummer enter their `kBandActive` clips.
- Format/recovery record:
  `.codex/analysis/gh1-character-face-animation-contract.md`.
- PCSX2 remains an approved authoritative trace target, but tracing is
  observation-only: no synthetic input, window/focus control, or navigation
  through memory writes. The procedure is recorded in
  `.codex/analysis/pcsx2-read-only-memory-trace-procedure.md`.

### 2026-07-25 PCSX2 interaction mandate

- Do not control PCSX2 or any other desktop window on the user's behalf.
- No UI Automation, accessibility invocation, window messages, SendKeys,
  simulated keyboard/controller input, focus changes, or window manipulation.
- PCSX2 capture must be PCSX2-native, but any interactive navigation or capture
  action must be performed directly by the user unless a genuinely unattended
  command-line interface can do it without desktop control.
- A native PCSX2 H.264/AAC capture path was validated, but the diagnostic take
  reached the retail fail screen and is not accepted as character-motion
  proof.

### 2026-07-25 GH1 character runtime completion

- The active GH1-character goal is implemented. All eight playable guitarist
  models and all five packed stage-role models load from the independently
  mounted GH1 archive while GH2 remains the primary gameplay/UI owner.
- Revision-10 active LOD View discovery follows the authored `lod0*`/`lod1*`
  child View and resident group. This restored Grim and Punk without a model
  name table.
- Direct GH2 XEX/ReXGlue recovery supersedes the old bounded output bridges.
  The runtime now uses exact `.trans` then `.mesh` acquisition plus
  `ScaleDown`, typed `ScaleAdd`, and final pose commit for all decoded output
  rows. Serialized `OutputBone.local` never seeds a live pose.
- ACP parsing retains all recovered position/scale/quaternion/scalar/delta
  types, exact packed byte widths, source weights, and output inventory.
  Animation flags, play flags, venue exclusions, GH1 ANIM messages,
  walk/turn/stop selection, and `bone_facing` root prediction are live.
- Static retail face analysis closes the old blink question: `blink` exists
  only in the pose inventory, not in any configured excitement/event list, and
  retail update `0x2879D8` has no independent blink scheduler. No synthetic
  blink timer was added.
- Performer references accept `source:model` for every authored role. The
  proof-only `--diagnostic-performer role=source:model` substitutes an
  existing song role but cannot invent a fifth stage slot.
- Release verification passes all 53 `ghogx_character_*` tests. Final-binary
  smokes prove: untouched four-role GH2, four-role GH1, and GH1 keyboard in
  `Jessica`'s packed keyboard lineup.
- Main motion proof:
  `.codex/current-evidence/gh1-character-complete/video-sweep-20260725/`
  `GH1-all-characters-small2-proof.mp4`.
- Full-band motion proof:
  `.codex/current-evidence/gh1-character-complete/role-proof-20260725/`
  `gh1-full-band-small2-video/GH1-full-band-small2-proof.mp4`.
- Format record:
  `.codex/analysis/gh1-character-runtime-format-contract.md`.
- Deployed `gh2_ps2_hybrid_assets/ghogx_app.exe` SHA-256:
  `9F4A82C8E8D5E3F639FFA2D667850F4B53B1C20496B5795431BE093D54422942`.
  Post-copy four-role GH1 smoke:
  `.codex/current-evidence/gh1-character-complete/deployed-smoke-20260725/`.
- Exact-deployed mixed motion proof:
  `.codex/current-evidence/gh1-character-complete/`
  `deployed-mixed-video-20260725/GH1-metal-GH2-arena-deployed-proof.mp4`.

### 2026-07-25 GH1 character visual rejection and goal reopen

- The preceding completion claim is rejected. Its captures remain failure/load
  evidence only and must not be cited as accepted visual proof.
- The full-band deployed frame visibly has a disconnected/deformed guitarist
  arm chain, missing singer facial geometry, and a bassist floating with
  invalid knee/lower-leg posing.
- The eight-character composite is too distant to verify those systems.
- The exact original full GH1-character objective is active again. Completion
  now requires systemic source-derived corrections plus readable close
  front/side video evidence for the affected roles.

### 2026-07-28 GH2 CharWalk scheduler and transition checkpoint

- GH2 retail recovery now covers `BandCharacter::Poll`, the CharWalk request
  delay/reset path, task expiry, direction/style flags, source-order
  destination selection, and `CharClipGroup::GetClip(int)` cyclic flagged
  selection/promotion. Raw GH1 compatibility behavior remains isolated.
- Native `CharClip` loading now retains source timing and every ordered
  transition target/node pair instead of discarding the transition graph.
  Focused first/last transition-node lookups mirror the recovered source rows.
- Focused release tests pass:
  `ghogx_character_clip_driver_flags_test`,
  `ghogx_character_source_truth_contract_test`, and
  `ghogx_gameplay_rules_test`.
- GH2 retail `CharClip::FindNode` at `0x00196888..0x00196A70` is typed:
  modes 3/4 search authored first/last nodes, mode 2 returns null, and the
  generic fallback uses source timing plus the target beat-align nibble.
- GH2 retail turn-candidate scoring is now implemented exactly. The runtime
  uses source `FirstPlaying`, current-to-turn mode 3, turn-to-walk last-node
  lookup, `bone_facing` root/heading prediction, farther-distance rejection,
  strict wrapped-angle minimization, and authored turn/walk beat handoff.
  Focused facing interpolation, driver-stack, distance, angle, and timing
  tests pass.
- Deployed `gh2_ps2_hybrid_assets/ghogx_app.exe` SHA-256:
  `F1BE930C78FD1F0E899E5BA1502D2EEF4AC022F19D228027409C8EED2E0A6F95`.
  Its native-only Arena guard exits 0 with 3 waypoints, request mask `0x40`,
  20/15/32 turn/walk/stop clips, and final state `playing`.
- Remaining CharWalk boundary is fully typed path regulation, final stop
  selection, complete CharClipDriver task ownership, and an input-free
  completed live walk-phase proof. Do not claim parity before those close.

### 2026-07-28 GH1 guitarist play and highway bundle checkpoint

- All eight converted GH1 guitarists pass hidden, input-free, strict-native
  600-frame GH2-song runs. The recovered native `BandCharacter` `play`
  handler selects each source-authored normal clip, all type owners report
  zero unhandled messages, no driver request fails, every run reaches
  `state=playing`, and steady FPS spans 59.248-59.788.
- Proof:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/native-guitarist-play-matrix/`.
- GH1 `config/gen/characters.dtb` is now parsed as the authoritative
  per-character `track_surface` table. The offline bundle maps each compiled
  source bitmap to the GH2-native `<package>_keep.bmp_ps2` path using the
  character manifest join; this preserves `hair_metal -> hair.bmp` without a
  name-specific rule.
- Full regeneration remains deterministic and complete: 105/105 MILO assets,
  926/926 ACP clips, 13/13 character models, 39 animation packages, and zero
  blockers. The character bundle now includes eight highway-surface rows.
- The deployed primary patch ARK overlay reports `reused=54`, `replaced=2`,
  `added=6`, `appended=270592`, `entries=1696`; exact ARK-v3 verification
  passes. A live exact-deployed Alterna run resolves and selects
  `track/surfaces/gen/alterna_keep.bmp_ps2` with no missing-asset row.
- Matched retail visual/behavioral character parity remains open.

## 2026-07-28 GH1/GH2 hand-strum bridge checkpoint

- Packed GH1 `charsys.dtb` defines the three authored strum semantics as
  `pluck_short`, `pluck_down`, and `down_long`; packed GH2
  `midi_parsers.dtb` addresses four short, two pick, and four long names.
- The converter now emits those ten target-family names as aliases sharing
  the exact converted GH1 sample bodies. Every native guitarist strum package
  contains four retained source entries plus ten aliases; the focused
  round-trip test passes.
- Full regeneration remains clean at 105/105 MILO assets, 926/926 ACP clips,
  13/13 models, and zero blockers. The deployed ARK overlay replaced all
  eight strum packages, added no paths, and passes exact verification.
- Hidden, input-free, strict-native 1,200-frame runs cover all eight GH1
  guitarists. Every row resolves all ten target names, reaches real GH2 note
  events, publishes guitar/string transforms and post-controller hand
  telemetry, reports zero missing clips or invalid handlers, remains playing,
  and sustains 58.911-59.840 FPS.
- Matched retail visual hand-placement acceptance remains open.

## 2026-07-28 singer rear-head draw-closure checkpoint

- The white geometry behind both GH1 singers was reproduced with fixed rear
  viewer cameras and identified as unmaterialed `Bip01 Ponytail*`
  skeleton/editor helper meshes.
- The decoded singer `top.view` graphs do not contain those helpers. They
  contain the active LOD branch plus legitimate direct accessories such as
  the mic stand and earrings. The fault was the renderer appending every
  ungrouped resident Mesh after the authored graph.
- Character drawing now consumes the decoded `Character9` active LOD and
  ancestor-group closure, skips sibling LODs, retains authored accessories,
  and excludes unrelated ungrouped helpers. There is no singer, ponytail,
  mesh, material, or empty-material exception.
- The mesh-decode and source-truth tests pass. Fresh unfiltered rear captures
  remove the white geometry from both singers. A complete Small Club scene
  remains clean at 59.893 steady FPS, and a strict-native runtime sweep loads
  all 13 converted GH1 character packages with zero engine failures.
  Executable
  `715341FF7283856DBAAF799BD8EA88F883FD8DC5E163FE4B2FD0C1C253FCCB57`
  is deployed.

## 2026-07-28 native material light-channel checkpoint

- GH2 PS2 `Ps2Mat::Select` at `0x0019CFE0` reads `mUseEnviron` at `+0x40`
  and `mPreLit` at `+0x9C`; `0x0019D12C..0x0019D154` enables one material
  light channel exactly when `use_environ || !prelit`.
- Runtime now bypasses fixed lighting only for `prelit && !use_environ`.
  The former all-prelit bypass plus manual ambient fold crushed common
  prelit/environment-enabled Basement surfaces nearly to black.
- The four-case contract test and focused renderer set pass 4/4. Live strict-
  native converted GH1 Basement and native GH2 Arena runs reach gameplay with
  authored environments active. The deployed Basement proof reports 59.238
  steady FPS.
- Executable
  `59FB90C95CD6DFDBFC05B249DC58906D5230B955860DEE69B1541064E2DF158C`
  is deployed. Evidence:
  `proofs/gh1-native-conversion-parity/material-light-channel-contract/`.

## 2026-07-29 GH1 Animatable0 ownership/scheduling closure

- A generic `milo_object_inspect matanim5` mode now prints every legacy
  Animatable0 operation/object and every MatAnim key channel.
- Packed Small Club `smoke.matanim` is proven to contain scale
  `0.00120000006`, offset `0`, loop range `0..100`, and two
  texture-translation keys from `(0,0,0)` to `(1,0,0)`.
- No Small Club lighting View, PollAnim, EventTrigger, or compiled WorldDir
  script owns or references that MatAnim. It is a standalone Animatable.
- Offline conversion already follows the recovered compatibility loader:
  `smoke.matanim` plus a root `smoke.filt` preserve the exact source program.
- The full packed audit passes 105/105 MILOs and 926/926 ACPs. It records
  1,866 Animatable0 bodies, 330 with operations, 229 with child references,
  and 1,147 references. Shipped content uses only operation types 0 and 1.
- Retail static execution evidence closes the former timing boundary:
  `SLUS_212.24:0x001AC0C0` loads the filter/child lists,
  `0x001AC640` creates operation types 0..4, and `0x001ABAB8` filters only
  an externally supplied frame before recursing to children. Those paths do
  not create a task or read TaskMgr time.
- Unowned Small Club `smoke.matanim`, `record.matanim`,
  `record_g.matanim`, and Theatre `tram.mnm` therefore have no implicit
  playback owner. Their native MatAnim/filter pairs are correct and no
  automatic song-time rule is added.
- The deployed GH2 audit independently records 1,698 native world
  Animatables, 401 AnimFilters, 588 directory-root candidates, and zero
  PollAnim objects, with Group/filter/EventTrigger ownership emitted to
  `.anim-ownership.tsv`.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/.codex/analysis/gh1-native-matanim-scheduling/`.

## 2026-07-29 current singer rear-head motion proof

- The deployed native packages were recaptured through the hidden,
  input-free character viewer with a fixed rear camera.
- Female singer runs source-authored `female_singer_active_medium`; male
  singer runs source-authored `singer_active_medium`.
- The ordinary unfiltered draw path keeps both rear silhouettes clean for the
  61-frame motion capture. No mesh/material suppression diagnostic is active.
- The combined 960x360, 10-fps, 6.1-second MP4 has SHA-256
  `D27B266EDCD87E186F5834EB348B2D8D8724AB3603E4441470D4C9851D58F75B`.
- The proof was refreshed against deployed executable SHA-256
  `4F8494CAD257BA09DA8E1159FFDD8F13AE19C7309916DB89EFC5A924144F94C4`
  after the serialized FaceFx path closure.
- The temporary 122-frame BMP set was deleted after video and midpoint-preview
  validation. Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/singer-rear-head-contract/current-motion-proof/`.

## 2026-07-29 serialized FaceFx viseme-package path closure

- Stock GH2 female/male singer packages serialize
  `../../anims/female_viseme.milo` and
  `../../anims/metal_viseme.milo` in `FaceFxLipSyncServo5`.
- Gameplay and the character viewer now share one resolver for those decoded
  paths. The former model-stem-derived `*_viseme.milo_ps2` fallback is gone.
- Converted GH1 singers have source Morph/EventTrigger face programs and no
  FaceFx servo. An empty servo list therefore performs no guessed FaceFx
  package lookup.
- Focused FaceFx, character mesh, and source-truth tests pass. Hidden,
  input-free strict-native loads pass for both converted GH1 singers and both
  stock GH2 singer horse outfits; stock `neutral`/`visemes` clips resolve from
  the serialized references.
- Built and deployed executable SHA-256:
  `4F8494CAD257BA09DA8E1159FFDD8F13AE19C7309916DB89EFC5A924144F94C4`.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/facefx-serialized-viseme-path/`.

## 2026-07-29 intentional-discard instance ledger

- `milo_convert_audit` now emits
  `.discarded-value-instances.tsv` with archive path, object name, source
  class/revision, field, value class, and proof status for every discarded
  source-field instance.
- The full GH1 sweep emits 17,314 rows across 1,030 archive assets and 11,471
  objects. All 28 aggregate buckets reconcile exactly with zero mismatches.
- Status totals: 15,415 default/absent, 867 structural revision, 861 target
  class not drawable, 171 retail-traced Mat21 target discards, and zero
  `nondefault_requires_retail_proof`.
- The six `Cam9.drawable.showing=false` instances are all
  `6 foot camera.cam` in Basement, Small Club, Small Club Multi, Big Club,
  Theatre, and Arena.
- The audit target builds, the packed sweep completes, and the copied instance
  ledger SHA-256 is
  `322BB2A8BA96619A09FA162C9384B527C37DE5D5A47EFC65301BBA246CC33DE6`.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/semantic-field-contract/`.

## 2026-07-29 shared ObjectDir hand-target closure

- Actual strict-native gameplay, not the diagnostic character viewer, exposed
  converted GH1 hand-driver targets collapsing near world origin. GH1 hand
  clip output graphs name instrument-only `bone_fret_hand` and
  `bone_strum_hand` transforms, but typed target acquisition only searched
  character bones/meshes. World lookup also allowed a stale output snapshot to
  supersede the live attached-prop transform.
- `resolve_gh2_pose_target` now includes the imported attached-prop
  `.trans`/`.mesh` namespace. Transform-chain lookup selects the live resident
  prop proxy before the nonresident output snapshot fallback. No asset,
  character, instrument, bone, pose, venue, or offset-specific rule was added.
- Focused tests prove typed acquisition/blending/publication into a parented
  attached-prop transform and prove that `CharIKHand` selects a live proxy over
  a conflicting origin-valued snapshot.
- Hidden, input-free, strict-native gameplay covers all eight GH1 guitarists
  with a full GH1 backing band in converted Small Club, a GH2 song/runtime, and
  the GH2 Xplorer. All 224 logged targets are finite and non-origin; all 176
  full-weight solves pass with maximum residual 1.7035 world units.
- Hair Metal's transition-only residual is source-accounted. Packed
  `hair_intro_03.acp` itself starts `bone_pos_guitar` at
  `(-311.78289795, 174.30110168, -152.40872192)` before the active clip settles;
  all 22 full-weight Hair Metal solves are exact.
- Current built/deployed executable SHA-256:
  `0338E2173BEE189CE8199788E7EBF26089B3B55138956B75812F18294B114C30`.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/instrument-hand-parity/shared-objectdir-hand-targets/`.

## 2026-07-29 all-guitarist attachment-transform matrix

- A hidden, input-free raw/native matrix now covers all eight selectable GH1
  guitarists at exact stand-clip frame 60.
- The resolved character-owned `bone_pos_guitar.mesh` world matrix matches
  across all twelve components: seven pairs are identical at six decimals and
  Grim's maximum absolute delta is `0.000001`.
- Raw runs use the packed GH1 Xplorer; native runs use the authoritative GH2
  Xplorer. Instrument-internal anchor matrices are deliberately not compared.
- This closes the all-guitarist attachment-transform conversion subcase. The
  broader item remains open only for matched retail visual hand-placement
  acceptance.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/instrument-hand-parity/all-guitarist-attachment-matrix/`.

## 2026-07-29 owner-tagged arm-twist gameplay proof

- Current proof uses the main strict-native `ghogx_app.exe` gameplay path, an
  all-GH1 converted band and Small Club, and GH2 song/gameplay ownership.
- Six close videos cover Classic, female singer, male singer, bassist,
  drummer, and keyboardist. The camera set keeps the relevant arms visible;
  the guitarist's GH2 instrument remains attached and both bassist hands hold
  the bass during the active clip.
- Runtime twist headers and output transform rows now identify the owning
  decoded Character. Each focused role records 150 upper-twist solves and
  1,200 finite transform rows, the source-authored sibling branch with
  `(0.500,0.500)` factors, a nonzero 0.59318--2.35621-radian roll span, and
  zero nonfinite rows. Classic additionally records 150 forearm solves.
- Read-only retail PCSX2/PINE captures for the live `Guitar Hero` title and all
  four serialized twist-controller names are copied beside the proof.
- The obsolete separate `ghogx_viewer` target and `engine/src/viewer.cpp` are
  removed. Main-executable diagnostics remain.
- A no-capture 960x720 phase profile measures 6.268 ms gameplay draw and
  6.501 ms total render. The following clean 600-frame run holds 60.014 steady
  FPS; the earlier 55.438 sample remains recorded as below target.
- Six focused tests pass. Current built/deployed executable SHA-256:
  `D938201B9A33622990FF94710E79AA05D257205CB6F6B4FBBEB607D03B7288C8`.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/instrument-hand-parity/arm-twist-isolation/strict-native-gameplay-current/`.

## 2026-07-29 GH1 retail placement-record closure

- A fresh hidden PCSX2 2.7.93 run loaded the preserved GH1 Small Club
  savestate. Read-only PINE reports title `Guitar Hero` and repeats the
  accepted live owner/world anchors three times. No input, focus/window
  control, or emulated-memory write was used.
- Static recovery removes the supposed Arena-to-CharSys transfer gap:
  `0x00363748` is the shared Arena owner. Arena fills `stage_spot` at `+0xE0`
  and `walk_spot` at `+0xF0`; CharSys consumes those exact vectors.
- Walk and stage consumers each copy four 16-byte quadwords at record offsets
  `0x00/0x10/0x20/0x30`. The `0x40` record is one complete transform matrix
  with no unknown tail.
- The full packed corpus contains 40 numbered helpers across all seven
  venues—21 stage and 19 walk. Every source Mesh25 and converted Mesh28
  resolved-transform digest matches. No packed helper is externally parented.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/matched-retail-small-club-anchor/`.

## 2026-07-29 Font7 retained/derived value ledger

- `milo_convert_audit` now emits `.font-values.tsv`.
- All 20 packed fonts are compared across 5,540 field rows after resolving
  their proven Mat/Tex contracts and decoding the source bitmaps.
- An independent alpha-column scan regenerates all 5,120 character-info rows,
  covering 1,896 rasterized placements, blank-glyph defaults, and tab/space
  advance behavior. The report also covers 128 kerning rows and all native
  fields/defaults.
- All rows are exact. No packed font exercises the leading-NBSP normalization
  branch.
- Report SHA-256:
  `54364E4748F479C0A8801A4A8CBC7DECF5A499E569A3E941FF5505A39E5ED84F`.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/semantic-field-contract/`.

## 2026-07-29 simple-object retained/translated value ledger

- `milo_convert_audit` now emits `.object-field-values.tsv`.
- The full packed sweep records 6,588 field rows across all 456 Cam9,
  Environ1, Flare3, Light3, MultiMesh0, and Text15 instances.
- Each emitted GH2 object is reparsed. Independent comparisons cover retained
  payloads, Drawable conversion, target-native ObjectFields0/new-field
  defaults, MultiMesh transform arrays, and Cam9's 4:3 horizontal-to-vertical
  FOV formula.
- All rows are exact. Removed Drawable object-list and legacy-target fields
  are empty in every affected source instance.
- Report SHA-256:
  `443CD2E341975FD3AC919E8F22068D9D794043DE4C6029464C29FA97BF9D8B9F`.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/semantic-field-contract/`.

## 2026-07-29 MatAnim5 stage-value ledger

- `milo_convert_audit` now emits `.mat-anim-values.tsv`.
- The packed sweep covers all 156 MatAnim5 objects, 183 stage outcomes, 994
  keys, and 89 filters.
- Results: 156 exact roots, 13 exact generated split passes, 12 retail-consumed
  frame-zero early stages, and two source-authored `amp_inside_star_1.mnm`
  split-name overrides in the two HUD packages.
- Every stage is instance-accounted by the generic loader rule; there is no
  asset-name conversion branch.
- Report SHA-256:
  `A7955D0B0D2867E99665C92ABC168ED0A70E32BE0281AFA1136155C4781EF200`.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/semantic-field-contract/`.

## 2026-07-29 direct animation-payload value ledger

- `milo_convert_audit` now emits `.animation-payload-values.tsv`.
- The packed sweep covers all 270 Morph, LightAnim, CamAnim, EnvAnim,
  ParticleSysAnim, MeshAnim, and Movie objects with 4,502 authored keys and 19
  filters.
- Independent comparisons cover every retained reference/key/flag, CamAnim FOV
  conversion, Morph poses and intensity, Movie flags, target base defaults,
  and filter values. There are zero mismatches and no further nested
  Animatable0 memberships.
- Report SHA-256:
  `43FD1834573F5352EBFE84FF34764DC4D6D386911D1DDAD06E8887D507606EB5`.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/semantic-field-contract/`.

## 2026-07-29 TransAnim4 retained/filter value ledger

- `milo_convert_audit` now emits `.trans-anim-values.tsv`.
- The packed sweep pairs all 686 TransAnim4 objects with reparsed TransAnim6
  targets and verifies 30,263 ordered transform keys plus all owner and
  interpolation flags with zero mismatches.
- An independent reducer verifies all 312 legacy Animatable0 operations and
  all 163 synthesized AnimFilter1 objects. The source contains only three
  animation-membership references and no TransAnim drawable memberships; the
  memberships remain scoped to the View graph.
- Report SHA-256:
  `F1E3BFDC3531311D2BDA6F9C8524C0725E3EC80BFE6405FB27A42AA2EA7CDDF1`.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/semantic-field-contract/`.

## 2026-07-29 View7 membership-graph closure

- The converter now expands non-View Animatable0 membership recursively inside
  View animation graphs, using the same generic cycle-checking model as
  drawable closure.
- The full packed ledger covers 678 Views, 1,132 animation references, 5,433
  drawable references, 14 MatAnim expansions, 72 environment-scope Groups, 51
  draw-only Groups, and 10 filters with zero mismatches.
- This closes the three previously omitted nested memberships:
  `arena_fire_flash.tnm -> arena_fire_flash.mnm` and
  `gem_bonus_spark1.tnm -> gem_bonus_spark2.mnm, gem_bonus_spark1.mnm`.
- Focused conversion tests and the full 105-MILO/926-ACP sweep pass.
- Report SHA-256:
  `CABD6DEDCE22130A0A9CF54C74B48953D41B7203D8110360685CA8D50C00BEE7`.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/semantic-field-contract/`.

## 2026-07-29 ACP18 retained/translated value ledger

- `milo_convert_audit` now emits `.acp-values.tsv`.
- The full packed sweep pairs all 926 ACP18/sample-set-5 clips with reparsed
  CharClipSamples10 targets: 42,386 channel entries and 18,810,192 compressed
  sample bytes.
- An independent check reports zero mismatches across timing, flag
  translations, ordered channels, cumulative native category counts,
  compression/sample counts, frame-size accounting, and sample byte digests.
  Target wrapper/event defaults and the data-free retail duplicate header are
  also verified.
- Report SHA-256:
  `513DE477EE51889780B170F2A0D60C8B36657D9152636AD2BCD821F388E1C8AE`.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/semantic-field-contract/`.

## 2026-07-29 Tex8 retained-value ledger

- `milo_convert_audit` now emits `.tex-values.tsv`.
- The full packed sweep pairs all 1,067 Tex8 objects with reparsed Tex10
  targets. All contain embedded bitmaps, totaling 16,535,680 payload bytes.
- An independent field/digest check reports zero mismatches across dimensions,
  external state, mip bias/type, every HMX bitmap header field, reserved
  bytes, and complete payload bytes. Native target ObjectFields0 defaults are
  also verified.
- Report SHA-256:
  `0BD6C2545AB08D3AEB413D283AB0CE9EED388F2CECA3867ED836208BDF349ED3`.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/semantic-field-contract/`.

## 2026-07-29 revision-8 directory transform-graph closure

- `milo_convert_audit` now emits `.transform-values.tsv`.
- The full packed sweep covers all 8,095 GH1 transformables across Mesh, View,
  Light, ParticleSys, Text, Flare, and Cam, plus all 4,545 authored child
  links.
- An audit implementation independent from the converter replays source-order
  child ownership, explicit-parent/self-parent behavior, and the revision-8
  constraint mapping. Reparsed target local/world matrices, parents,
  constraints, targets, and preserve-scale flags have zero mismatches and
  there are zero unresolved child links.
- Outcomes: 4,507 self-parent objects with child owners, 1,928 self-parent
  objects without owners, 1,653 explicit-parent wins, seven later-child wins,
  and 21 multiple-child-owner cases.
- Report SHA-256:
  `B7A27C38449D6F10AF2BD90CAE47D4D606B70C75B7CB5F8CF1943D287905C0B3`.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/semantic-field-contract/`.

## 2026-07-29 Mesh25 retained/translated value ledger

- `milo_convert_audit` now emits `.mesh-values.tsv`.
- The full packed sweep records all 7,087 Mesh25 sources beside reparsed
  Mesh28 targets: 356,288 vertices, 411,158 faces, and 1,609 skinned meshes.
- An independent report check finds zero mismatches across all non-transform
  fields: drawable state/sphere, material, geometry owner, mutable flags,
  volume, empty BSP conversion, vertices, faces, patch/group sizes, bone
  slots/matrices, and cached strip sections.
- All 1,139 nonempty legacy child vectors are classified as inputs to the
  directory transform graph. That graph remains a separately verified family.
- Report SHA-256:
  `74A3A891A83954ACE91CB1559F89159577382A81E516E1389BE4F35AABD693B6`.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/semantic-field-contract/`.

## 2026-07-29 Mat21 retained/translated value ledger

- `milo_convert_audit` now emits `.mat-root-values.tsv` and
  `.mat-stage-values.tsv`.
- The full packed sweep passes 105/105 MILOs and 926/926 ACPs with zero
  blockers and records all 1,693 Mat21 roots plus all 1,181 authored texture
  stages.
- The root report covers global blend, color, environment/prelit, cull, Z,
  alpha, texture/pass counts, root stage mode, and intensify. The stage report
  covers stage blend, TexGen, wrap, transform, texture, target pass state,
  synthesized links, defaults, and reset rule.
- An independent PowerShell formula check joins the two ledgers and reports
  zero root mismatches, zero stage mismatches, and an exact 1,181 stage-count
  sum.
- Report SHA-256:
  - roots:
    `1F79B200EF0B8C2B22BA2D16BD0B239E99B9C36C45AF73914982E2619791D557`
  - stages:
    `2676D247852D6E1368B99EF2E658D0F9706FD03ECFD4D6495B0969936C5B23BD`
- Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/semantic-field-contract/`.

## 2026-07-29 ParticleSys22 retained/translated value ledger

- `milo_convert_audit` now emits `.particle-values.tsv`.
- The full packed sweep records all 76 systems across 3,737 field rows,
  including 896 preserved particle rows and two legacy-operation filters.
- Thirteen nonzero bounce planes independently reproduce their synthesized
  Trans9 matrices and references. Three enabled zero-normal planes correctly
  remain unbound. All Animatable0 membership lists are empty.
- All rows are exact.
- Report SHA-256:
  `232A7278ADB4D29E2B35EC55AE65BDFA9494085D6707DDF967AE4138636A36BB`.
- Evidence:
  `GuitarHeroOGX-main-ui-engine/proofs/gh1-native-conversion-parity/semantic-field-contract/`.

## 2026-08-10 Dynamic character selector and JSON DLC system

- Character select is now provider-driven: one centered five-card film reel in
  single-player and independent centered three-card reels for P1/P2.
- The male and female singers are distinct additive guitarists, not replacements.
  Each orders its GH2 outfit first and GH1 outfit second, uses its approved
  portrait, retargeted guitarist animation source, attached guitar, and hides the
  authored microphone subtree only while performing as a guitarist.
- `DLC/core.singers/manifest.json` proves the schema and loose ARK-relative
  content mounting. Per-addon manifests can combine characters, outfits,
  guitars/finishes, venues, songs, and setlists. DTB data loads first, JSON DLC
  loads deterministically afterward, and invalid packages roll back atomically.
- Sequential focused tests pass 4/4. The deployed executable SHA-256 matches the
  build at `1A0BD60AFDF11A818119FA2B08513FF63B360EA273FEF617410B1995720AD535`.
- Deployment: `gh2_ps2_hybrid_assets/`; proof:
  `GuitarHeroOGX-main-ui-engine/proofs/dlc-character-selector/README.md`.

## 2026-08-11 Two-player selector and portrait refinement

- The dynamic two-player film reels suppress the four source-authored neon
  selector descendants at their obsolete rack positions, then clone the pink
  P1 and blue P2 pairs outside their transform animations and fix them over the
  two selected center cards.
- Both singer portraits now occupy the canonical stock 64x128 matte rectangle
  `(10,17)..(54,111)` rather than touching the texture edges.
- Deployed trace reports `suppressed_source_highlights=4` and
  `static_source_highlights=4`; UI/catalog tests pass 2/2. Fresh proof:
  `GuitarHeroOGX-main-ui-engine/proofs/dlc-character-selector/multiplayer-static-selectors-normalized-icons.bmp`.
- Deployed executable SHA-256:
  `BAF88B22D016D0455AAED2466F176A539CFFBB14595B823DF71E36C8B0099F31`.

## 2026-08-15 GH3 PS2 Midori external character pipeline

- Goal state reached for Guitar Hero Classic runtime verification: Midori is
  packaged as external DLC files only; no Midori-specific runtime conversion
  path is required.
- Both main outfits are present (`gh3_midori_1`, `gh3_midori_2`), alternate
  skins are excluded, and the deployed DLC package contains 6 assets.
- Animation package contains 331 runtime clips/aliases across four external
  banks: guitar-main 266, guitar-ui 6, guitar-strum 23, guitar-fret 36.
- The generated main bank now carries stock-style guitar `CharClipGroup`
  objects plus hand-driver clip flags, so GHC resolves intro/idle/normal
  groups and evaluates left/right hand weights from external clip data.
- The external model package now emits GH2 helper transforms for
  `bone_fret_hand` and `bone_strum_hand`, plus a stock-basis
  `bone_pos_guitar.mesh` attach alias so the shared Flying V prop attaches in
  a playable pose.
- Verification:
  `python tools/gh3_midori_gameplay_proof.py --print-summary` ->
  `status=in_song_midori_variant_animation_verified failures=0`.
- Full gate:
  `python tools/gh3_midori_pipeline_verify.py --print-summary` ->
  `status=guitar_hero_classic_midori_runtime_verified checks=103 failures=0`.
- Current visual proof:
  `analysis/gh3_midori_ghc_gameplay_proofs/gh3_midori_1_variant_gameplay_guitarist0_f120.bmp`
  and
  `analysis/gh3_midori_ghc_gameplay_proofs/gh3_midori_2_variant_gameplay_guitarist0_f120.bmp`.

## 2026-08-17 Midori matrix-local lower-body diagnosis

- `matrix-local-axis-align-bind` remains the first viable pelvis branch. Pelvis
  and thighs are visually coherent but still provisional.
- Adding knees under the same signed-axis policy is stable but not approved:
  knees round-trip through GHC/Hmx, yet both knee rest residuals are large
  (`1.98605`) and the GHC silhouette stays too straight versus Blender's crouch.
- Additional world-chain diagnostics now bracket the failure: full
  `matrix-world-axis-align-bind` and knee-only
  `matrix-knee-world-axis-align-bind` recover bend energy but over-curl the
  lower legs; the 50/50 `matrix-knee-world-axis-blend-bind` midpoint splays the
  legs rather than matching the source crouch.
- New recursive source-pose evaluator matches Blender/NXTools child rotations:
  `parent_pose * parent_rest^-1 * child_rest * child_basis`, about `1e-5` max
  error on thighs/knees at frame 30. This proves the previous child matrix
  diagnostics were using the wrong non-recursive source pose.
- `matrix-eval-world-axis-align-bind` is rejected because it rolls the full
  character horizontal. `matrix-eval-knee-world-axis-align-bind` preserves
  pelvis/thigh posture but still crosses/splays lower legs.
- `matrix-vector-aim-bind` confirms the GH2 child transform convention and uses
  verified source hip/knee/ankle vectors to aim thighs/knees. It is stable and
  moves toward source bend, but still over-splays the legs because each segment
  is aimed independently.
- `matrix-leg-plane-aim-bind` is rejected after direct GHC `--char --clip`
  captures at frames 30/45. It stays stable but pushes the legs into a hard
  horizontal extension, while Blender source reference frames show bent
  back/down legs.
- `matrix-knee-vector-aim-bind` is rejected for broad use. It improves the
  single attack diagnostic, but full-bank direct GHC captures for idle, attack,
  jump, solo, and transition show neutral source legs becoming a seated /
  horizontal-shin pose.
- The first `matrix-knee-vector-aim-gated-bind` review was invalid as a final
  judgment because the helper skipped to the old distal world-axis base, not to
  the broad `matrix-local-axis-align-bind` baseline.
- Added a true local-axis world-chain helper and switched knee-vector diagnostics
  onto it. With the corrected base, `matrix-knee-vector-aim-gated-bind` at
  threshold `0.94` preserves neutral idle/jump/transition shapes and adds some
  correction to attack/solo. It remains diagnostic only; attack is still not
  source-matched enough for final signoff.
- True-base correction-order retest: `pre` preserves idle but barely improves
  attack; `post` gives stronger attack bend but over-curls; gated-post preserves
  neutral frames but still overdoes attack/solo. Added
  `matrix-knee-vector-aim-gated-postblend-bind`, a 50/50 blend between default
  and post correction under the same `0.94` gate.
- Strength retest across 50/65/75 and post shows
  `matrix-knee-vector-aim-gated-postblend65-bind` is currently best: it
  preserves neutral frames and gives more attack/solo bend than 50% without the
  obvious post-order overcurl. Still diagnostic only.
- Full-bank all-clips staging of
  `matrix-knee-vector-aim-gated-postblend65-bind` succeeded (`331` staged clips;
  guitar-main bank `266` clips). Direct GHC captures for idle, attack, jump,
  solo, and transition at yaws 0/180 are stable and neutral-safe, but attack and
  solo still do not source-match the deeper knee tuck. Evidence:
  `.codex/current-evidence/midori-postblend65-fullbank-20260817/full65_contact.png`.
- Added and rejected pelvis diagnostic `matrix-local-rest-bind-delta`
  (`target_bind * (source_rest^-1 * source_pose)`). Hmx round-trip remains clean
  around `1e-8`, but pelvis-only attack frames 30/45 become almost horizontal in
  GHC. Evidence:
  `.codex/current-evidence/midori-pelvis-restbinddelta-20260817/restbinddelta_pelvis_contact.png`.
- Added and execution-tested bridge automation from the NXTools-evaluated source
  scene. `tools/gh3_midori_source_visual.py` now writes `source_bone` aliases in
  `--pose-json` records, so Blender pose names such as `bone_pelvis` can be
  consumed as GH3 source names such as `Bone_Pelvis`. Added
  `tools/gh3_midori_source_bridge_export.py`, a one-command Blender wrapper that
  exports both pose JSON and an animated skinned GLB, then validates expected
  frames/source bones plus GLB skins/animations/meshes. Blender 4.5 at
  `C:\Program Files\Blender Foundation\Blender 4.5\blender.exe` successfully
  exported `gh3_guit_mido_a_attackl` frames `0,15,30,45` for the lower body:
  `36` pose records, `9` source bones, GLB v2 with `1` skin, `1` animation,
  `2` meshes, and `76` nodes. Evidence:
  `.codex/current-evidence/midori-source-bridge-export-20260817/`.
- Confirmed the target graph root differs from the GH3 source graph:
  `Bone_Pelvis -> bone_pelvis` has no target parent, while source
  `Bone_Pelvis` is under `Control_Root`. Target bind records now carry
  `target_parent_source` for diagnostics.
- Rejected `matrix-local-axis-target-parent-bind`, which tried to emit the
  Control_Root-composed pelvis world directly as target-local. GHC pelvis-only
  frames roll upside-down/horizontal. Evidence:
  `.codex/current-evidence/midori-pelvis-targetparent-20260817/targetparent_pelvis_contact.png`.
- A 24-way signed-axis sweep for pelvis-only `matrix-local-axis-align-bind`
  confirms axes 12-15 are the only visually useful family. Axis 14 is the
  automatic pelvis choice and remains the best current pelvis constant:
  `[[0,1,0],[0,0,-1],[-1,0,0]]`. Evidence:
  `.codex/current-evidence/midori-pelvis-axis-sweep-20260817/`.
- Rejected target-graph knee-vector postblend65:
  `matrix-knee-vector-aim-targetgraph-gated-postblend65-bind` is stable after
  the helper fix, but attack frames 30/45 regress to the too-straight
  local-axis silhouette. Evidence:
  `.codex/current-evidence/midori-targetgraph-knee-20260817/targetgraph65_fixed_attack_contact.png`.
- Added analytic two-bone IK diagnostics that solve target-length
  hip-knee-ankle chains from mapped source positions:
  `matrix-leg-ik-bind`, `matrix-leg-ik-flip-bind`,
  `matrix-leg-ik-flipblend35-bind`, `matrix-leg-ik-flipblend50-bind`,
  `matrix-leg-ik-pelvisgate-flipblend35-bind`, and
  `matrix-leg-ik-pelvisgate-flipblend50-bind`.
- Raw and flipped IK add real attack tuck but either cross/fold too hard or
  break neutral clips into a crouched/seated pose. Ungated 35/50 blends still
  damage idle.
- A source pelvis-motion gate cleanly separates neutral from deep lean frames:
  idle/jump/transition are about `4/15/6` degrees, while attack and solo are
  about `79-108` and `44` degrees. Gated IK therefore preserves neutral clips.
- `matrix-leg-ik-pelvisgate-flipblend35-bind` is the better current IK
  candidate: representative GHC captures preserve idle/jump/transition and add
  visible attack/solo bend without the full 50% overpush. It is not final and
  has not passed full-bank visual approval. Evidence:
  `.codex/current-evidence/midori-leg-ik-20260817/`.
- Full all-clips staging/build of
  `matrix-leg-ik-pelvisgate-flipblend35-bind` succeeded: `331` staged clips and
  a `266`-clip guitar-main bank with `5` lower-body bones. Direct GHC captures
  from that full bank for idle, attack frame 30, attack frame 45, jump, solo,
  and transition at yaws 0/180 match the per-clip diagnostic. This is the best
  current lower-body candidate, but it is still not source-approved: it moves
  toward the GH3 crouch while not yet reproducing the full source
  hip-knee-ankle silhouette. Evidence:
  `.codex/current-evidence/midori-leg-ik-fullbank-20260817/full_legikgate35_contact.png`.
- Added and rejected scaled-distance IK diagnostics:
  `matrix-leg-ik-scaled-pelvisgate-flip-bind`,
  `matrix-leg-ik-scaled-pelvisgate-flipblend35-bind`, and
  `matrix-leg-ik-scaled-pelvisgate-flipblend50-bind`. These correct the
  source-to-target hip-ankle distance scale (`~42.5x` from source rest to GH2
  rest), but visually raw scaled becomes seated/horizontal and scaled 35/50
  regress toward the too-straight silhouette. Evidence:
  `.codex/current-evidence/midori-scaled-ik-20260817/scaled_ik_all_contact.png`.
- Expanded the best unscaled gated IK candidate down the hierarchy. Per-clip
  broad probes with ankles only, then ankles plus toes, remained stable with no
  idle/transition regression. Full all-clips staging/build with pelvis, thighs,
  knees, ankles, and toes succeeded (`331` staged clips; `266`-clip main bank;
  rebuilt main MILO `9,500,500` bytes). Direct GHC captures from that full bank
  match the previous five-bone result: ankles/toes are a safe hierarchy
  expansion, but they do not solve the remaining source hip-knee-ankle shape
  mismatch. Evidence:
  `.codex/current-evidence/midori-leg-ik-ankletoe-20260817/`.
- Added pelvis-gated unscaled IK bend-plane/strength diagnostics:
  `matrix-leg-ik-pelvisgate-flipblend42-bind`,
  `matrix-leg-ik-pelvisgate-flipblend35-bendin15-bind`,
  `matrix-leg-ik-pelvisgate-flipblend35-bendout15-bind`,
  `matrix-leg-ik-pelvisgate-flipblend42-bendin15-bind`,
  `matrix-leg-ik-pelvisgate-flipblend42-bendout15-bind`,
  `matrix-leg-ik-pelvisgate-flipblend35-bendin45-bind`,
  `matrix-leg-ik-pelvisgate-flipblend35-bendout45-bind`,
  `matrix-leg-ik-pelvisgate-flipblend42-bendin45-bind`, and
  `matrix-leg-ik-pelvisgate-flipblend42-bendout45-bind`. The 15-degree plane
  sweep was stable but too subtle. `matrix-leg-ik-pelvisgate-flipblend42-bendout45-bind`
  is the best current branch: it keeps idle/jump/transition neutral-safe and
  makes attack/solo slightly more compact than the 35% candidate. Full all-clips
  staging/build succeeded (`331` staged clips; `266`-clip main bank; `9`
  lower-body bones; `283` entries; main MILO `9,498,516` bytes). This improves
  the lower-body diagnostic but is still not source-approved. Evidence:
  `.codex/current-evidence/midori-leg-ik-bendplane-20260817/`.
- Broad `matrix-local-axis-align-bind` remains the safest fallback lower-body
  baseline for neutral clips, though attack is still too straight.
- Retained compact evidence:
  `.codex/current-evidence/midori-knee-vector-aim-20260817/` and
  `.codex/current-evidence/midori-broad-knee-diagnostics-20260817/` and
  `.codex/current-evidence/midori-truebase-knee-gate-20260817/` and
  `.codex/current-evidence/midori-truebase-knee-variant-20260817/` and
  `.codex/current-evidence/midori-truebase-knee-strength-20260817/`.
- Best live lower-body diagnostic is now full-bank-stable
  `matrix-leg-ik-pelvisgate-flipblend42-bendout45-bind`, with pelvis/thigh/knee/
  ankle/toe staged. Next branch should move to a source-authoritative automated
  bridge or a deeper source-fit solve; a `.glb` intermediate is acceptable if
  the final process remains automated and writes ordinary ark-external GHC DLC
  assets. Do not retest source distance scaling, and do not regress to
  postblend65 unless a later branch breaks neutral safety.
- Added bridge-pose consumption to ACP staging:
  `--source-pose-bridge-json PATH` loads Blender/NXTools evaluated pose records,
  validates the bridge format, keys positions by `source_bone`, and applies them
  only when the staged source clip matches the bridge animation. Added explicit
  diagnostic policy
  `matrix-leg-ik-bridge-pelvisgate-flipblend42-bendout45-bind`, equivalent to
  the current best local IK branch but driven by bridge pose positions at
  matching frames. Focused attack staging confirmed
  `source_pose_bridge_active=True` for frames `0,15,30,45`; the single-clip MILO
  built and GHC captures at yaws `0/180` were stable. This proves bridge-fed
  retarget plumbing into ordinary ACP/MILO/GHC assets, but visual result remains
  in the `out42_45` family and is still not final source approval. Evidence:
  `.codex/current-evidence/midori-bridgefed-retarget-20260817/`.
- Extended the bridge loader to consume evaluated rotation matrices as well as
  positions, and added bridge-eval diagnostics:
  `matrix-bridge-eval-world-axis-align-bind` and
  `matrix-bridge-eval-knee-world-axis-align-bind`. Both were staged from the
  source bridge for `gh3_guit_mido_a_attackl` frames `0,15,30,45`, built as
  single-clip MILOs, and captured in GHC. Result: full bridge-eval world
  alignment is rejected because it rolls the character horizontal again; knee
  bridge-eval is stable but does not improve over `out42_45`. Evidence:
  `.codex/current-evidence/midori-bridge-evalrot-20260817/`.
- Added target-local bridge-rotation diagnostics:
  `matrix-bridge-eval-local-axis-align-bind` and
  `matrix-bridge-eval-knee-local-axis-align-bind`. These derive source
  parent-space local rotations from bridge evaluated parent/child pose matrices,
  then align those local rotations into target Glam local bind space. Focused
  attack GHC captures reject the full local bridge branch because late frames
  roll/horizontal again. The knee-only local branch is stable, but it regresses
  toward the too-straight silhouette and is worse than `out42_45`. Evidence:
  `.codex/current-evidence/midori-bridge-localrot-20260817/`.
- Added stronger bridge-constrained IK blend diagnostics:
  `matrix-leg-ik-bridge-pelvisgate-flipblend46-bendout45-bind` and
  `matrix-leg-ik-bridge-pelvisgate-flipblend50-bendout45-bind`, compared
  against existing
  `matrix-leg-ik-bridge-pelvisgate-flipblend42-bendout45-bind`. All use bridge
  positions for matching attack frames, Glam thigh/shin lengths, the
  pelvis-motion gate, and the current bendout45 plane. Focused GHC captures for
  idle, attack frames `15/30/45`, and transition at yaws `0/180` are stable,
  but 46/50 add little or no material improvement over 42 / current `out42_45`.
  Blend strength alone is not the remaining source-fit gap. Evidence:
  `.codex/current-evidence/midori-bridge-ik-blend-20260817/`.
- Added coupled bridge lower-body frame diagnostic
  `matrix-leg-ik-bridgeframe50-pelvisgate-flipblend42-bendout45-bind`. It
  derives a dynamic lower-body frame from bridge thigh/knee positions, maps that
  frame into target lower-body space, applies the Glam pelvis bind correction,
  blends it 50% with the stable baseline pelvis, then uses the current
  bridge-fed `out42_45` leg IK. Focused GHC captures for idle, attack frames
  `15/30/45`, and transition at yaws `0/180` are stable but almost identical to
  bridge42; a simple pelvis/lower-body frame blend does not close the source-fit
  gap. Evidence:
  `.codex/current-evidence/midori-bridge-frame-20260817/`.
- Added torso-aware bridge consumer
  `matrix-torso-bridge-leg-ik-pelvisgate-flipblend42-bendout45-bind`, driven by
  the ignore-partial NXTools/Blender pose bridge for pelvis, stomach, chest,
  neck/head, and leg matrices. Numeric staging works and preserves ordinary GH2
  ACP/MILO locals, but valid GHC visual capture rejects the branch: frame `15`
  keeps vertical leg failure, while frames `30/45` floor-fold or roll sideways
  instead of matching the source arched pose. The visual proof is
  `.codex/current-evidence/midori-torso-bridge-legik-visual-20260817/`.
- Fixed the diagnostic one-clip runtime harness in
  `GuitarHeroOGX-main-ui-engine/tools/milo_convert/milo_convert_tool.cpp`:
  generated guitar-main clipsets now emit a fallback `normal` `CharClipGroup`
  when ACP group flags are absent. Without this, GHC reports the diagnostic
  performer clip override as missing because the generated main bank has
  `active=<none>`. Keep this converter fix; it makes the visual reject valid
  rather than a clip-selection false negative.
- Added target-graph torso diagnostics after identifying the `Control_Root`
  mismatch: source `Bone_Pelvis` is parented by `Control_Root`, but target
  `bone_pelvis` is root-parented and guitar-main suppresses `Control_Root`
  channels. New policy
  `matrix-torso-targetgraph-bridge-leg-ik-pelvisgate-flipblend42-bendout45-bind`
  composes/localizes through `target_parent_source`; valid GHC capture still
  rejects it. New policy
  `matrix-torso-targetgraph-stablepelvis-bridge-leg-ik-pelvisgate-flipblend42-bendout45-bind`
  keeps pelvis on the known-good matrix-local-axis family and applies
  bridge/IK below it; valid capture also rejects it, though the f45 failure
  shape changes. Evidence:
  `.codex/current-evidence/midori-torso-targetgraph-bridge-20260817/` and
  `.codex/current-evidence/midori-torso-targetgraph-stablepelvis-20260817/`.
- Added stable-pelvis bridge endpoint diagnostics using both transpose and
  direct signed-axis vector maps:
  `matrix-torso-targetgraph-stablepelvis-bridgeendpoints-leg-ik-pelvisgate-flipblend42-bendout45-bind`
  and
  `matrix-torso-targetgraph-stablepelvis-bridgeendpointsaxis-leg-ik-pelvisgate-flipblend42-bendout45-bind`.
  Both valid GHC captures reject. They remove the worst floor-fold behavior at
  frames `30/45`, but the source silhouette is still wrong and frame `15`
  remains vertical-leg. Evidence:
  `.codex/current-evidence/midori-torso-targetgraph-stablepelvis-bridgeendpoints-20260817/`
  and
  `.codex/current-evidence/midori-torso-targetgraph-stablepelvis-bridgeendpointsaxis-20260817/`.
- Rendered the ignore-partial evaluated source bridge GLB directly with
  `tools/gh3_midori_glb_source_render.py`. The source attack pose is horizontal
  at frames `15/30/45`, including `15`; older pelvis-only “f15 upright” notes
  were a partial-source blind spot. Evidence:
  `.codex/current-evidence/midori-source-glb-reference-20260817/`.
- Added
  `matrix-torso-targetgraph-rootworld-bridgeendpointsaxis-leg-ik-pelvisgate-flipblend42-bendout45-bind`.
  It maps evaluated source world/root rotations into the GH2 target graph and
  makes GHC frame `15` horizontal, so the Control_Root/root frame diagnosis is
  real. Valid capture rejects because the diagnostic one-clip emitted
  `bone_pelvis.pos` near zero, collapsing the performer into the stage.
  Evidence:
  `.codex/current-evidence/midori-torso-targetgraph-rootworld-bridgeendpointsaxis-20260817/`.
- Added
  `matrix-torso-targetgraph-rootworld-posbind-bridgeendpointsaxis-leg-ik-pelvisgate-flipblend42-bendout45-bind`,
  which keeps the same root-world rotations but composes pelvis translation
  with the GH2 target bind height. Emitted pelvis `z` returns to about `39.4`
  and all three GHC captures succeed/restore, but the visual still rejects:
  frames `15/30/45` remain offset/intersecting and do not match the GLB source
  silhouette. Evidence:
  `.codex/current-evidence/midori-torso-targetgraph-rootworld-posbind-bridgeendpointsaxis-20260817/`.
  Next branch: bake target-skeleton locals from evaluated bridge/GLB positions
  for pelvis placement, torso, neck/head, and leg endpoints together; do not
  continue with rotation-only variants.
- Added evaluated-position bake diagnostics:
  `matrix-torso-targetgraph-rootworld-posbind-positionbake-bind` and
  `matrix-torso-targetgraph-rootworld-posbind-positionframe-bind`. The first
  corrects root-world/posbind rotations toward mapped evaluated child vectors;
  the second derives aimed world frames directly from mapped bridge
  parent/child position frames. Result: `positionbake` captures validly but is
  pixel-identical to the previous rootworld-posbind reject, and `positionframe`
  builds to the exact same one-clip MILO SHA256 as `positionbake`, so capture
  was skipped as a duplicate. Evidence:
  `.codex/current-evidence/midori-torso-targetgraph-rootworld-posbind-positionbake-20260817/`
  and
  `.codex/current-evidence/midori-torso-targetgraph-rootworld-posbind-positionframe-20260817/`.
  Next branch should derive the source bridge absolute/evaluated
  `Control_Root` + `Bone_Pelvis` translation contract into GH2
  `bone_pelvis.pos`; local orientation bake is now proven redundant.
- Added pelvis-position bridge diagnostics:
  `matrix-torso-targetgraph-rootworld-bridgepos-bind` and
  `matrix-torso-targetgraph-rootworld-bridgepos-unscaled-bind`. Both keep
  root-world torso rotations and replace `bone_pelvis.pos` with evaluated
  bridge `Bone_Pelvis` delta mapped into target lower-body basis; the first
  uses the legacy `72/467.25` translation scale and the second omits it. Both
  valid captures reject. The scaled branch changes only frame `30` pixels
  materially versus posbind; the unscaled branch moves the pelvis channel much
  more but still only frame `30` changes materially versus scaled bridgepos,
  with frames `15/45` visually identical and still wrong. Evidence:
  `.codex/current-evidence/midori-torso-targetgraph-rootworld-bridgepos-20260817/`
  and
  `.codex/current-evidence/midori-torso-targetgraph-rootworld-bridgepos-unscaled-20260817/`.
  Next branch: `bone_pelvis.pos` alone is not the missing placement contract;
  inspect GHC guitar-main root/recenter/move_self performer placement and
  whether source `Control_Root` absolute translation must be represented
  outside pelvis local position or via clip/character root metadata.
- Completed the pelvis-only `move_self` branch. Added
  `milo_convert_tool build-clipset-from-acp ... --move-self 0|1` as a
  diagnostic override; default generated `guitar-main` behavior remains
  `move_self=1`. Rebuilt the same rootworld `bridgepos` one-clip diagnostic
  with `move_self=0`, verified identical `bone_pelvis.pos` samples at frames
  `15/30/45`, temporarily swapped it into the loose Midori DLC, mounted the
  GH2 ISO only for capture, and restored the deployed main MILO to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
  GHC accepted the diagnostic clip override and pose publisher, but the
  resulting screenshots are pixel-identical to the prior `move_self=1`
  `bridgepos` rejection: zero changed pixels at frames `15/30/45`. Evidence:
  `.codex/current-evidence/midori-moveself0-rawstage-20260817/`.
  Next branch: `move_self` is not the missing contract; represent evaluated
  source `Control_Root`/character-root translation outside pelvis-local
  `.pos`, potentially through an automated GLB/Control_Root placement bridge
  before converting back to ordinary ark-external GHC assets.
- Added root-placement channel diagnostics:
  `matrix-torso-targetgraph-rootworld-facingpos-bridgepos-bind` and
  `matrix-torso-targetgraph-rootworld-absposepos-bind`. `facingpos` emits
  mapped absolute evaluated `Control_Root` placement as synthetic
  `bone_facing.pos` while keeping bridgepos pelvis placement. The generated
  MILO samples `bone_facing.pos=(-11.1802,8.66232,3.31421)` and GHC accepts
  the diagnostic clip, but frames `15/30/45` are pixel-identical to the prior
  bridgepos rejection, proving `bone_facing.pos` is not consumed by the
  visible guitar-main pose path. `absposepos` composes mapped absolute
  evaluated `Bone_Pelvis` placement into `bone_pelvis.pos`, emitting around
  `(-11.3,8.76,42.7)`; GHC captures validly and frame `30` changes
  materially, but frames `15/45` remain pixel-identical to bridgepos and the
  silhouette still rejects. Evidence:
  `.codex/current-evidence/midori-facingpos-rawstage-20260817/` and
  `.codex/current-evidence/midori-absposepos-rawstage-20260817/`.
  Next branch: stop adding raw root-position channels; test generated
  CharBone output hierarchy/root parenting (`bone_facing -> bone_pelvis`) or
  an automated GLB-derived target skeleton solve before ordinary MILO emission.
- 2026-08-17 latest pelvis-only matrix-local/Control_Root update: fixed a
  diagnostic dispatch bug where `ROOTWORLD_TORSO_POLICIES` shadowed the later
  GLB-specific branches, and added a static regression test. Focused tests now
  pass `31/31`. Added `glbevalik`, `glblocalik`, `glblocalraw`, and three
  `glbframeik` composition-order probes. Corrected staging proves the
  evaluated/position-frame variants genuinely collapse to the old
  `glbframeik` pelvis quaternion family, while raw Control_Root-local bridge
  pose finally emits a distinct pelvis-only family. Local-GEN captures only:
  app launched with `--ark-dir gh2_ps2_hybrid_assets/GEN`; no GH2 ISO mounted
  at game time, `D:\GEN=False`, and deployed main restored to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
  Visual result: default raw-local rejects because frame `45` pitches nearly
  vertical/upside-down; direct-storage raw-local is best so far because frame
  `45` stays horizontal, but it still appears mirrored relative to the source
  GLB reference and is not promoted as a passed pelvis gate. Compact evidence:
  `.codex/current-evidence/midori-glblocalraw-source-pelvisonly-fixed-visual-20260817/glblocalraw_pelvisonly_contact.png`
  and
  `.codex/current-evidence/midori-glblocalraw-direct-pelvisonly-fixed-visual-20260817/glblocalraw_direct_pelvisonly_contact.png`.
  Next branch: sweep fixed `180` degree post-corrections around the raw
  Control_Root-local pelvis pose, starting with direct storage plus local/world
  X/Y/Z half-turns, still pelvis-only.
- 2026-08-17 latest: added explicit direct-storage half-turn diagnostics
  `glblocalraw-localx/y/z180`, `glblocalraw-worldx/y/z180`, and
  `glblocalraw-alllocalz180`; focused tests pass `32/32`. Frame-45 local-GEN
  triage rejected X/Y half-turns and narrowed to `localz180` / `worldz180`.
  Full frames `15/30/45` selected `localz180` as the pelvis-only pass
  candidate: it keeps the source horizontal side-fall and head/feet
  relationship most consistently. First thigh rung
  (`Control_Root,Bone_Pelvis,Bone_Thigh_L,Bone_Thigh_R`) still rejects:
  pelvis-only `localz180` plus raw thigh locals sends legs into the wrong
  plane, and `alllocalz180` improves but still mismatches thigh direction at
  frames `30/45`. All captures used local
  `gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark`; no ISO at game time,
  `D:\GEN=False`, deployed main restored to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
  Evidence:
  `.codex/current-evidence/midori-glblocalraw-halfturn-finalists-20260817/halfturn_finalists_contact.png`,
  `.codex/current-evidence/midori-glblocalraw-localz180-thighs-visual-20260817/localz180_thighs_contact.png`,
  and
  `.codex/current-evidence/midori-glblocalraw-alllocalz180-thighs-visual-20260817/alllocalz180_thighs_contact.png`.
  Next branch: keep `localz180` for pelvis and derive thigh locals from
  evaluated hip-knee vectors under the corrected pelvis; do not add knees yet.
- 2026-08-17 latest thigh diagnosis: added
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighvec-bind`
  and four `localz180-thighframe` plane-frame composition diagnostics.
  Focused tests now pass `34/34`. `thighvec` was directly captured and
  rejected because frames `15/45` throw the legs nearly vertical even though
  pelvis remains in the accepted family. The four `thighframe` variants staged
  to identical thigh quaternion samples, so only representative `thighframe`
  was captured; it also rejects because the source-like sideways leg extension
  is lost at frame `15` and frames `30/45` collapse into an upright/dangling-leg
  pose. All captures used local `gh2_ps2_hybrid_assets/GEN/main.hdr` +
  `main_0.ark`; no ISO at game time, `D:\GEN=False`, GHC/build processes at
  Idle priority, and deployed main restored to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
  Evidence:
  `.codex/current-evidence/midori-glblocalraw-localz180-thighvec-visual-20260817/localz180_thighvec_contact.png`
  and
  `.codex/current-evidence/midori-glblocalraw-localz180-thighframe-visual-20260817/thighframe_contact.png`.
  Next branch: keep pelvis-only `localz180`, but treat thigh solving as a
  pelvis-corrected local-basis conversion problem rather than direct hip-knee
  aiming or world leg-plane replacement.
- 2026-08-17 latest thigh local-basis continuation: added
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-bind`
  and
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-bind`.
  Focused tests now pass `35/35`. Plain `thighbasis` converts evaluated source
  thigh local rotation through the signed-axis target-bind helper under the
  accepted pelvis-only `localz180`; it rejects because frame `45` still has a
  vertical/wrong-plane leg failure. `thighbasis-parentcomp` compensates that
  thigh basis against the pelvis half-turn; it is a real partial improvement
  because the legs stay in the sideways fall plane better than `thighvec`,
  `thighframe`, or plain `thighbasis`, but frames `30/45` are still too
  extended and non-source-like to pass the thigh gate. Evidence:
  `.codex/current-evidence/midori-glblocalraw-localz180-thighbasis-visual-20260817/thighbasis_contact.png`
  and
  `.codex/current-evidence/midori-glblocalraw-localz180-thighbasis-visual-20260817/visual_decision.json`.
  All captures used local `gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark`;
  no ISO at game time, `D:\GEN=False`, app/build at Idle priority, and deployed
  main restored to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
  Next branch: preserve the parent-compensation insight but damp it via a
  plain-basis/parentcomp blend sweep, or add knees only if thigh direction can
  be judged independently from bind lower-leg extension.
- 2026-08-17 latest damped thighbasis sweep: added
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp35-bind`,
  `...parentcomp50-bind`, and `...parentcomp65-bind`. These slerp between
  plain `thighbasis` and full `thighbasis-parentcomp` in world space for thigh
  bones only. Focused tests still pass `35/35`; the test now checks the blend
  amount map. Frame-45 local-GEN triage rejects all three: they do not move
  toward the useful full-parentcomp visual, and instead reintroduce a
  vertical-leg failure. Evidence:
  `.codex/current-evidence/midori-glblocalraw-localz180-thighbasis-blend-visual-20260817/thighbasis_blend_f45_contact.png`
  and
  `.codex/current-evidence/midori-glblocalraw-localz180-thighbasis-blend-visual-20260817/visual_decision.json`.
  All captures used local `gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark`;
  no ISO at game time, `D:\GEN=False`, GHC/build at Idle priority, and deployed
  main restored to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
  Next branch: stop damping this slerp route; keep full
  `thighbasis-parentcomp` as the useful local-basis clue and run a knee-aware
  minimal diagnostic to see whether bind lower legs are exaggerating the
  apparent thigh over-extension.
- 2026-08-17 latest knee-aware diagnostic: added
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-kneebasis-bind`.
  It keeps pelvis-only `localz180` plus full `thighbasis-parentcomp` for
  thighs, then converts `Bone_Knee_L/R` local rotations through the signed-axis
  target-bind helper instead of raw bridge-local knees. Added a regression test
  proving localz180 thighbasis policies remain in the pelvis half-turn
  correction map; focused tests pass `36/36`. Minimal thighs+knees visual gate
  rejects with partial improvement: knee basis fixes some raw-knee lower-leg
  extension and improves frame `15`, but frames `30/45` still throw one leg
  vertically instead of matching the source bent side-fall silhouette. Evidence:
  `.codex/current-evidence/midori-glblocalraw-localz180-kneebasis-visual-20260817/kneebasis_f45_contact.png`,
  `.codex/current-evidence/midori-glblocalraw-localz180-kneebasis-visual-20260817/kneebasis_full_contact.png`,
  and
  `.codex/current-evidence/midori-glblocalraw-localz180-kneebasis-visual-20260817/visual_decision.json`.
  All captures used local `gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark`;
  no ISO at game time, `D:\GEN=False`, GHC/build at Idle priority, and deployed
  main restored to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
  Next branch: stay under pelvis-only `localz180` plus full
  `thighbasis-parentcomp`, but diagnose lower-leg handedness/chain composition
  by adding ankles or testing side-specific knee/ankle axis flips.
- 2026-08-17 latest ankle-chain basis diagnostic: added
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-legbasis-bind`.
  It keeps pelvis-only `localz180`, full `thighbasis-parentcomp` thighs,
  signed-axis target-bind knees, and extends the same basis conversion to
  `Bone_Ankle_L/R`. Focused tests still pass `36/36`. Frame-45 local-GEN
  comparison against `...kneebasis-bind` rejects it: the vertical-leg silhouette
  remains, while ankle basis mostly shifts foot/ankle twist. Evidence:
  `.codex/current-evidence/midori-glblocalraw-localz180-legbasis-visual-20260817/legbasis_f45_contact.png`
  and
  `.codex/current-evidence/midori-glblocalraw-localz180-legbasis-visual-20260817/visual_decision.json`.
  All captures used local `gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark`;
  no ISO at game time, `D:\GEN=False`, GHC/build at Idle priority, and deployed
  main restored to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
  Next branch: stop adding chain depth on this route; test side-specific
  lower-leg handedness/axis flips under the same pelvis-only `localz180` plus
  full `thighbasis-parentcomp` root.
- 2026-08-17 latest side-specific knee handedness sweep: added six local
  half-turn probes under pelvis-only `localz180` plus full
  `thighbasis-parentcomp`:
  `...kneeflip-lx180`, `...ly180`, `...lz180`, `...rx180`, `...ry180`, and
  `...rz180`. Each uses knee basis for both knees and flips one selected knee.
  Focused tests still pass `36/36` and now cover the six-entry flip spec map.
  Frame-45 local-GEN triage rejects left-knee X/Y/Z and right-knee X because
  they keep the vertical-leg failure. Right-knee Y/Z are useful partial
  improvements and were captured at frames `15/30/45`, but still reject:
  frames `30/45` fold/cross the problem leg into a non-source side extension
  instead of the source bent side-fall silhouette. Evidence:
  `.codex/current-evidence/midori-glblocalraw-localz180-kneeflip-visual-20260817/kneeflip_f45_contact.png`,
  `.codex/current-evidence/midori-glblocalraw-localz180-kneeflip-visual-20260817/kneeflip_finalists_contact.png`,
  and
  `.codex/current-evidence/midori-glblocalraw-localz180-kneeflip-visual-20260817/visual_decision.json`.
  All captures used local `gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark`;
  no ISO at game time, `D:\GEN=False`, GHC/build at Idle priority, and deployed
  main restored to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
  Next branch: keep right-knee Y/Z as the handedness clue and test a right-leg
  plane/bend-axis solve or combine that handedness with source hip-knee-ankle
  plane constraints.
- 2026-08-17 latest right-knee bend-axis solve: added
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-kneebend-r-bind`,
  `...kneebend-ry180-bind`, and `...kneebend-rz180-bind`. These keep
  pelvis-only `localz180` plus full `thighbasis-parentcomp`, convert knees
  through signed-axis target-bind, optionally seed right knee with the useful
  Y/Z half-turn, then vector-aim the right shin toward the mapped evaluated
  source knee-to-ankle vector. Focused tests still pass `36/36`. Frame-45
  local-GEN triage rejects all three: the pure vertical pole is gone, but the
  right leg still folds/crosses into a non-source side extension instead of
  the source bent side-fall silhouette. Evidence:
  `.codex/current-evidence/midori-glblocalraw-localz180-kneebend-visual-20260817/kneebend_f45_contact.png`
  and
  `.codex/current-evidence/midori-glblocalraw-localz180-kneebend-visual-20260817/visual_decision.json`.
  All captures used local `gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark`;
  no ISO at game time, `D:\GEN=False`, GHC/build at Idle priority, and deployed
  main restored to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
  Next branch: stop single-knee corrections and solve a coupled right-leg
  hip-knee-ankle plane constraint under pelvis-only `localz180` plus full
  `thighbasis-parentcomp`.
- 2026-08-17 latest right-leg plane solve: added
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-rightlegplane-bind`,
  `...rightlegplane-ry180-bind`, and `...rightlegplane-rz180-bind`. These keep
  pelvis-only `localz180` plus full `thighbasis-parentcomp`, then replace only
  `Bone_Knee_R` with a source hip-knee-ankle plane-frame solve; Y/Z variants
  seed the local plane frame using the prior right-knee handedness clues.
  Focused tests still pass `36/36`. Frame-45 local-GEN triage rejects all
  three: the base policy still crosses/extends the lower body, and the Y/Z
  seeded variants worsen the fold into a high kicked leg. Evidence:
  `.codex/current-evidence/midori-glblocalraw-localz180-rightlegplane-rawstage-20260817/rightlegplane_f45_contact.png`
  and
  `.codex/current-evidence/midori-glblocalraw-localz180-rightlegplane-rawstage-20260817/visual_decision.json`.
  All captures used local `gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark`;
  no ISO at game time, `D:\GEN=False`, GHC/build at Idle priority, and deployed
  main restored to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
  Next branch: stop replacing only the right knee world frame. Bridge the gap
  higher by comparing emitted target world joint positions after conversion
  against the source bridge, or use an automated intermediate skeleton/GLB solve
  that still emits ordinary ACP/MILO/DLC output.
- 2026-08-17 latest emitted pose-position report and localz180 posepos:
  added `tools/gh3_midori_pose_report.py`, which stages policies in memory,
  decodes emitted ACP local samples, reconstructs target world joint positions,
  and compares lower-body joints against the mapped source pose bridge. The
  corrected report proves pelvis under
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-bind` is exact after
  using the same base-frame bridge delta and `72/467.25` scale as staging; the
  remaining divergence starts below pelvis. Evidence:
  `.codex/current-evidence/midori-pose-report-20260817/pose_report.json`,
  `.codex/current-evidence/midori-pose-report-20260817/pose_report_position_policies.json`,
  and
  `.codex/current-evidence/midori-pose-report-20260817/pose_report_localz180_posepos.json`.
  Added policies
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-posepos-bind` and
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-posepos-bind`.
  Both emit ordinary ACP/MILO and add synthetic GLB/evaluated local `.pos`
  channels while preserving the pelvis-only localz180 rotation family. Numeric
  triage is promising: right knee/ankle errors drop near zero at frames
  `15/30/45`, with max visible-pose error about `17` units once the known
  `Control_Root` offset is excluded. Direct local-GEN visual capture improves
  over the vertical/kicked-leg failures, especially frame `45`, but still
  rejects because the body is heavily crouched/compressed and frame `30` shows
  non-source-like guitar/body collision. Evidence:
  `.codex/current-evidence/midori-localz180-posepos-rawstage-20260817/localz180_posepos_f45_contact.png`,
  `.codex/current-evidence/midori-localz180-posepos-rawstage-20260817/localz180_posepos_full_contact.png`,
  and
  `.codex/current-evidence/midori-localz180-posepos-rawstage-20260817/visual_decision.json`.
  Focused tests pass `36/36`; captures used local
  `gh2_ps2_hybrid_assets/GEN`, no ISO at game time, `D:\GEN=False`, Idle
  priority, and deployed main restored to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
  Next branch: keep localz180 posepos as the first promising position bridge,
  but do not raw-bake every GLB joint position. Test restricted lower-body-only
  posepos/IK or an intermediate skeleton bake that preserves target bone lengths
  while using mapped source joint positions as constraints.
- 2026-08-17 latest localz180 target-length IK rejection: added
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-ik-bind` and
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-ik-bind`.
  These keep the exact pelvis-localz180 base and use mapped GLB/source leg
  positions only as target-length two-bone IK constraints, without raw per-joint
  `.pos` tracks. Numeric report:
  `.codex/current-evidence/midori-localz180-ik-20260817/pose_report_localz180_ik.json`.
  Both policies are worse than `localz180-posepos` on lower-body target-space
  position error; thighbasis-parentcomp IK improves right ankle error at frame
  `45` but not enough. Frame-45 local-GEN visual triage rejects both because
  they reintroduce non-source leg crossing/verticality, with the green leg
  visibly upright/crossed in the thighbasis-parentcomp variant. Evidence:
  `.codex/current-evidence/midori-localz180-ik-20260817/localz180_ik_f45_contact.png`
  and
  `.codex/current-evidence/midori-localz180-ik-20260817/visual_decision.json`.
  Focused tests pass `36/36`; captures used local
  `gh2_ps2_hybrid_assets/GEN`, no ISO at game time, `D:\GEN=False`, Idle
  priority, and deployed main restored to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
  Next branch: keep `localz180-posepos` as the best partial. Do not continue
  pure target-length IK under this base; instead restrict posepos to the subset
  that fixed leg placement while preserving torso/upper-body bind lengths, or
  use an intermediate target-skeleton bake with length constraints before
  emitting selected local translations.
- 2026-08-17 latest restricted posepos and sampler correction: fixed an
  important diagnostic bug in `tools/gh3_midori_acp_stage.py`. Synthetic
  GLB pose-position channels were created for all
  `ROOTWORLD_GLBPOSEPOS_POLICIES`, but the sample loop only applied
  `target_glb_pose_local_translation` for
  `matrix-torso-targetgraph-rootworld-glbposepos-bind`. The newer
  `localz180-posepos` policies therefore emitted default/non-GLB translations.
  Treat the prior promising `localz180-posepos` visual evidence as a
  default/zero-position artifact, not true GLB posepos. Added restricted
  policies:
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-lowerposepos-bind`,
  `...distalposepos-bind`,
  `...thighbasis-parentcomp-lowerposepos-bind`, and
  `...thighbasis-parentcomp-distalposepos-bind`. `lowerposepos` emits
  GLB/evaluated local `.pos` only for pelvis and thigh/knee/ankle chains;
  `distalposepos` emits them only for knees/ankles. Numeric report:
  `.codex/current-evidence/midori-restricted-posepos-20260817/pose_report_restricted_posepos.json`.
  After the sampler fix, true full/lower GLB posepos is worse than the earlier
  artifact. Distal-only posepos reduces numeric knee/ankle error versus
  rotation-only branches, but frame-45 local-GEN visual triage rejects all four:
  lowerposepos pulls the green leg downward/vertical, and distalposepos still
  leaves a non-source hanging/crossed leg silhouette. Evidence:
  `.codex/current-evidence/midori-restricted-posepos-20260817/restricted_posepos_f45_contact.png`
  and
  `.codex/current-evidence/midori-restricted-posepos-20260817/visual_decision.json`.
  Focused tests pass `36/36`; captures used local
  `gh2_ps2_hybrid_assets/GEN`, no ISO at game time, `D:\GEN=False`, Idle
  priority, and deployed main restored to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
  Next branch: if pursuing the accidentally promising prior partial, formalize
  it as an explicit controlled/default/blended position policy instead of using
  the fixed `posepos` name. Otherwise move to an intermediate target-skeleton
  bake that blends source constraints with GH2 bind/local translation limits.
- 2026-08-17 latest controlled zero/blended posepos rejection: added explicit
  `zeroposepos` and `posepos25` policies for the localz180 and
  thighbasis-parentcomp families, so the earlier accidental default-position
  artifact is now reproducible by name. `zeroposepos` preserves the solved
  pelvis bridge translation and writes zero local positions for non-pelvis
  synthetic GLB `.pos` channels; `posepos25` blends those non-pelvis channels
  25% toward true GLB/evaluated local positions. Focused tests pass `36/36`.
  Numeric report:
  `.codex/current-evidence/midori-controlled-posepos-20260817/pose_report_controlled_posepos.json`.
  Frame-45 local-GEN visual capture rejects the branch: zero reproduces the
  compressed/crouched artifact, and 25% blending does not rescue the silhouette.
  Evidence:
  `.codex/current-evidence/midori-controlled-posepos-20260817/controlled_posepos_f45_contact.png`
  and
  `.codex/current-evidence/midori-controlled-posepos-20260817/visual_decision.json`.
  Captures used local `gh2_ps2_hybrid_assets/GEN`, no ISO at game time,
  `D:\GEN=False`, GHC/build at Idle priority, and deployed main restored to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
  Next branch: move to an automated intermediate target-skeleton bake. A `.glb`
  bridge is acceptable if automated and ultimately emitted as ordinary
  ACP/MILO/DLC output.
- 2026-08-17 latest first target-bakepos numeric rejection: added
  `targetbakepos` and `targetbakepos-altlocal` diagnostics for the localz180
  and thighbasis-parentcomp families. They emit synthetic lower-body `.pos`
  channels for pelvis/thigh/knee/ankle: pelvis uses the solved bridge
  translation, thighs use mapped source bridge hip positions, and knees/ankles
  use the GH2-length two-bone solve baked back into local translation channels.
  The altlocal variants test the opposite parent-rotation convention. Focused
  tests pass `37/37`. Numeric report:
  `.codex/current-evidence/midori-target-bakepos-20260817/pose_report_target_bakepos.json`;
  decision:
  `.codex/current-evidence/midori-target-bakepos-20260817/numeric_decision.json`.
  Result is numeric rejection without visual capture: the best row improves
  only frame `15`, while frames `30/45` remain worse than the controlled
  zero/default artifact with large knee/ankle errors and unstable shin
  direction. Next branch: generate a true intermediate constrained skeleton
  outside the existing target hierarchy, with `.glb` acceptable as an automated
  bridge before ordinary ACP/MILO/DLC emission.
- 2026-08-17 latest target-skelrot numeric rejection: added constrained
  target-skeleton rotation policies `targetskelrot` and
  `targetskelrot-bakepos` for the localz180 and thighbasis-parentcomp families.
  These aim thigh/knee rotations at the GH2-length solved target joints; the
  `bakepos` variants also emit the solved pelvis/thigh/knee/ankle local `.pos`
  channels so rotations and lower-body translations use the same constrained
  target points. Focused tests pass `38/38`. Numeric report:
  `.codex/current-evidence/midori-target-skelrot-20260817/pose_report_target_skelrot.json`;
  decision:
  `.codex/current-evidence/midori-target-skelrot-20260817/numeric_decision.json`.
  Result is numeric rejection without visual capture: isolated right-leg rows
  improve, but the full frame set remains worse than the already rejected
  controlled zero/default artifact, especially frame `45` and shin-direction
  stability. Next branch: stop patching the existing GH2 target hierarchy with
  selected local rotations/translations. Generate an explicit intermediate
  constrained skeleton/GLB whose solved joint transforms are the source-of-truth
  animation, then emit ordinary ACP/MILO from that skeleton.
- 2026-08-17 latest direct solved-skeleton visual rejection: added
  `targetsolveskel` and `targetsolveskel-bakepos` policies for localz180 and
  thighbasis-parentcomp. These derive thigh/knee world frames directly from the
  solved hip/knee/ankle target skeleton, then localize those frames against the
  solved skeleton rather than applying a correction to an inherited base
  rotation. Also fixed target-skeleton bake thigh targets to use pelvis-relative
  source bridge positions, matching the verifier and solved pelvis, instead of
  absolute GLB positions. Focused tests pass `39/39`. Evidence:
  `.codex/current-evidence/midori-target-solveskel-20260817/pose_report_target_solveskel.json`,
  `.codex/current-evidence/midori-target-solveskel-20260817/target_solveskel_f45_contact.png`,
  and
  `.codex/current-evidence/midori-target-solveskel-20260817/visual_decision.json`.
  Result is visual rejection: the best corrected candidate,
  `...thighbasis-parentcomp-targetsolveskel-bakepos-bind`, loads from local
  `GEN` but frame `45` curls the lower body into large non-source arcs across
  yaw views. Deployed main was restored to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`;
  `D:\GEN=False`. Next branch: build the intermediate skeleton as a separate
  generated source asset (JSON and, if useful, automated `.glb`) so local joint
  axes/rest-frame basis can be inspected before ACP/MILO emission.
- 2026-08-17 target-skeleton diagnostic continuation: added automated
  target-solved skeleton JSON export flags to `tools/gh3_midori_acp_stage.py`
  and a focused parser/helper regression; tests pass `40/40`. Evidence:
  `.codex/current-evidence/midori-target-skeleton-diagnostic-20260817/target_skeleton_diagnostic.json`
  and
  `.codex/current-evidence/midori-target-skeleton-diagnostic-20260817/pose_report_target_solveskel_parentfix.json`.
  The diagnostic found the f45 curl was not from bad solved thigh/knee aim:
  solved child-vector angles are effectively zero. The bug was synthetic
  lower-body `.pos` channels localized against the base parent rotation while
  `.quat` channels emitted solved parent rotations. Patched
  `target_skeleton_bake_local_translation()` so targetsolveskel policies use
  the solved parent world rotation for baked `.pos` localization. Regenerated
  diagnostics prove knees/ankles reconstruct to generated world positions with
  max error about `0.000005`; frame-45 right ankle numeric error improves from
  roughly `28.28` to `5.21`, though frames `15/30` remain mixed. Direct visual
  capture from local `GEN` is still pending; keep goal open.
- 2026-08-17 parent-frame fix visual capture: built a temporary one-clip main
  MILO for the corrected
  `...thighbasis-parentcomp-targetsolveskel-bakepos-bind` candidate, swapped it
  into the loose DLC, and captured frame `45` at yaws `0/90/180/270` from local
  `gh2_ps2_hybrid_assets/GEN` only, with app/build processes at Idle priority.
  First capture attempt was invalid because it loaded stock `glam1`; the valid
  rerun used `--diagnostic-character-variant gh3_midori_1` and logs prove
  `gh3_guit_mido_a_attackl` loaded from
  `char/gh3_midori/anims/gen/gh3_midori_main.milo_ps2`. Evidence:
  `.codex/current-evidence/midori-target-skeleton-diagnostic-20260817/parentfix_f45_midori_contact.png`
  and
  `.codex/current-evidence/midori-target-skeleton-diagnostic-20260817/parentfix_visual_decision.json`.
  Result: not approved, but materially improved. The previous catastrophic
  lower-body curl is gone and the pose is in the horizontal side-fall family;
  remaining lower body is too straight/overextended versus source f45, and
  frames `15/30` still need direct inspection. Deployed main restored to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`;
  `D:\GEN=False`.
- 2026-08-17 bendvariant numeric fork: added three corrected targetsolveskel
  variants (`scaled`, `bendout45`, and `scaled-bendout45`) under the
  thighbasis-parentcomp parent-frame fix and registered them in the focused
  tests. Tests pass `40/40`. Numeric report:
  `.codex/current-evidence/midori-target-skeleton-diagnostic-20260817/pose_report_target_solveskel_bendvariants.json`;
  decision:
  `.codex/current-evidence/midori-target-skeleton-diagnostic-20260817/bendvariants_numeric_decision.json`.
  Result is numeric rejection without visual capture: `bendout45` improves
  frames `15/30` but regresses frame `45`; the parent-fixed baseline remains
  best at f45, and scaled variants do not dominate. Continue with a
  frame/source-driven bend plane or an automated intermediate constrained
  skeleton/GLB bridge before ordinary ACP/MILO/DLC emission. The root/pelvis and
  GHC reconstruction problem is no longer the active blocker.
- 2026-08-17 sourcebendgate numeric promotion: added
  `...targetsolveskel-bridgescale-bakepos-bind` and
  `...targetsolveskel-sourcebendgate-bakepos-bind` under the corrected
  thighbasis-parentcomp targetsolveskel path. Tests pass `40/40`. Numeric
  report:
  `.codex/current-evidence/midori-target-skeleton-diagnostic-20260817/pose_report_target_solveskel_sourcebendgate.json`;
  decision:
  `.codex/current-evidence/midori-target-skeleton-diagnostic-20260817/sourcebendgate_numeric_decision.json`.
  `bridgescale` is identical to the parent-frame baseline because the endpoint
  is already GH2-length clamped. `sourcebendgate` is the next direct visual
  candidate: it preserves f45 (`max_pose 17.33`, right ankle `5.21`) and
  improves f15/f30 to `max_pose 13.92/13.31` versus baseline `29.02/25.42`.
  Next step is a local-GEN one-clip GHC capture for frames `15/30/45`; do not
  mount the GH2 ISO, and keep converter/GHC processes at Idle priority.
- 2026-08-17 local-GEN smoke continuation: built the converter in `%TEMP%` at
  Idle priority, staged one-clip sourcebendgate/ankletoe/leftlate MILOs, swapped
  each into the loose Midori DLC only for capture, and restored deployed main to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
  `D:\GEN=False` throughout. `sourcebendgate` f45/yaw0 is visually the same
  overextended family as the rejected parent-frame baseline. `ankletoe`
  successfully aims ankle-to-toe vectors in diagnostics, but renders
  pixel-identical to sourcebendgate, ruling out ankle twist as the visible f45
  blocker. Added `...sourcebendgate-leftlate-bakepos-bind`; numeric f45 improves
  to `max_pose 13.31` while preserving f15/f30 sourcebendgate gains, but direct
  visual is still unapproved because the smoke command's `--diagnostic-front-camera`
  path ignores `GHOGX_DEBUG_GAMEPLAY_CAMERA_YAW`, making the attempted yaw90
  capture identical to yaw0. Evidence:
  `.codex/current-evidence/midori-leftlate-visual-20260817/visual_decision.json`.
  Next action is to restore the earlier proven yaw-sweep capture harness or use
  a direct character/clip viewer that can force clip frame and yaw, then inspect
  `leftlate` f45 from side/back views before promotion or rejection.
- 2026-08-17 leftlate yaw-debug rejection: validated the no-front-camera
  gameplay debug camera harness using local `gh2_ps2_hybrid_assets/GEN` only:
  `GHOGX_DEBUG_GAMEPLAY_CAMERA=1`,
  `GHOGX_DEBUG_GAMEPLAY_CAMERA_TARGET=guitarist0:bone_spine1.mesh`, and yaw
  values `0` / `1.57079632679`. The screenshots differ, proving the harness
  avoids the prior `--diagnostic-front-camera` yaw override. The loose-DLC
  one-clip `leftlate` MILO loaded and the attack clip override was accepted,
  but direct yaw90 visual rejects it: torso/root collapses under the guitar
  while the lower body towers above. Treat the blocker as
  Control_Root/pelvis/root-motion placement/orientation, not ankle/toe twist
  or late left-leg bending. Evidence:
  `.codex/current-evidence/midori-leftlate-yawdebug-20260817/visual_decision.json`.
  The deployed main MILO was restored to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`;
  `D:\GEN=False`.
- 2026-08-17 leftlate HMX direct correction: the yaw-debug rejection above was
  the default `gh3_midori_acp_stage.py` HMX storage path, while the promising
  numeric `leftlate` report used `--hmx-quat-mode direct`. Captured a compact
  `GHOGX_DEBUG_CHARBONE_OUTPUT_MAP` rootdiag proving the default-storage
  candidate loaded from loose DLC and published live target rows; the failure
  is generated pose data, not post-publication runtime loss. Rebuilt the same
  one-clip candidate with `--hmx-quat-mode direct` and captured f45 yaw0/yaw90
  from local `gh2_ps2_hybrid_assets/GEN` only. Direct storage is materially
  better in yaw90 and returns to the source horizontal attack family, but yaw0
  remains tangled/occluded, so it is `promising_not_approved`. Evidence:
  `.codex/current-evidence/midori-leftlate-rootdiag-20260817/rootdiag_summary.json`
  and
  `.codex/current-evidence/midori-leftlate-direct-visual-20260817/visual_decision.json`.
  Next action is a direct-storage f15/f30/f45 yaw contact sheet with the
  validated no-front-camera debug-yaw harness, still no ISO at game time.
- 2026-08-17 direct-storage leftlate contact sheet rejection: captured
  `leftlate` with `--hmx-quat-mode direct` at frames `15/30/45`, yaws `0/90`,
  from local `gh2_ps2_hybrid_assets/GEN` only. All six logs prove loose-DLC
  load, Midori selection, `gh3_guit_mido_a_attackl` override, and screenshots;
  deployed main was restored to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
  Direct storage remains materially better than default storage, but the
  contact sheet rejects promotion: front views are tangled/occluded and side
  views still miss the source sideways airborne silhouette, especially f30/f45.
  Evidence:
  `.codex/current-evidence/midori-leftlate-direct-sheet-20260817/visual_decision.json`
  and
  `.codex/current-evidence/midori-leftlate-direct-sheet-20260817/leftlate_direct_contact_sheet.png`.
  Next action: keep `--hmx-quat-mode direct`, resume Control_Root/pelvis frame
  diagnosis, and solve a pelvis/root frame that preserves the source sideways
  airborne silhouette across f15/f30/f45 before more thigh/knee bend tuning.
- 2026-08-17 direct root-frame numeric diagnosis: kept runtime untouched
  and used only in-memory ACP pose reports at low/Idle process priority.
  Extended `tools/gh3_midori_pose_report.py` with `--source-bones` so the
  direct `leftlate` branch can be checked against torso markers, not only
  lower-body rows. The root sweep shows `Control_Root` remains an invariant
  diagnostic miss (`39.417502`), and prior `facingpos` evidence already proved
  synthetic `bone_facing.pos` is not consumed by the visible guitar-main pose.
  The new actionable failure is torso placement: plain `leftlate` has
  neck/chest/stomach drift around `7-18` target units. Added diagnostic policy
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcebendgate-leftlate-torsopos-bakepos-bind`,
  which preserves the direct `leftlate` rotation/lower-body path but emits
  torso bake-position channels from the GLB bridge. Numeric result: torso rows
  collapse near zero error; remaining non-root error is the known knee/ankle
  profile (`max_pose` about `13.3`). Focused tests pass `40/40`. Evidence:
  `.codex/current-evidence/midori-direct-rootframe-diagnostic-20260817/numeric_decision.json`
  and
  `.codex/current-evidence/midori-direct-rootframe-diagnostic-20260817/pose_report_direct_leftlate_torsopos.json`.
  Next action: build one-clip direct-storage `torsopos` and run the validated
  no-front-camera f15/f30/f45 yaw visual gate from local
  `gh2_ps2_hybrid_assets/GEN` only, still no ISO at game time.
- 2026-08-17 direct-storage torsopos visual gate: built a temporary one-clip
  main MILO for
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcebendgate-leftlate-torsopos-bakepos-bind`
  with `--hmx-quat-mode direct` (SHA
  `462A84D34DB01AEAF2A65CE7A457F418FBC1E3AEA5FCD99F75E068858C311432`), swapped
  it into loose DLC only for capture, and restored the deployed main MILO to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
  Captured f15/f30/f45 at yaws `0/90` using local
  `gh2_ps2_hybrid_assets/GEN`, `GHOGX_ADDONS_DIR=gh2_ps2_hybrid_assets/DLC`,
  the validated no-front-camera debug-yaw harness, and
  `GHOGX_DIAGNOSTIC_PERFORMER_CLIP=guitarist0=gh3_guit_mido_a_attackl`; no ISO
  was mounted or used at game time. All six captures exited zero and proved
  loose-DLC load, Midori variant selection, clip override, and screenshot save.
  Visual result is rejected: torso bake-position helps numerically, but the
  rendered contact sheet still has tangled/occluded front views and side views
  that do not preserve the clean source sideways airborne silhouette across
  f15/f30/f45, especially f30/f45. Evidence:
  `.codex/current-evidence/midori-torsopos-direct-visual-20260817/visual_decision.json`
  and
  `.codex/current-evidence/midori-torsopos-direct-visual-20260817/torsopos_direct_contact_sheet.png`.
  Next action: keep `torsopos` as a partial/negative result; continue on the
  remaining root-space/hierarchy mismatch plus knee/ankle errors, and do not
  retry synthetic `bone_facing.pos`.
- 2026-08-17 sourcepos exact-position diagnostic and visual gate: added
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-bakepos-bind`.
  This keeps the direct/localz180 target-skeleton rotation family but bypasses
  the GH2-length IK knee/ankle clamp, baking mapped source bridge positions for
  pelvis, torso, knees, and ankles. Numeric result is the strongest so far:
  expanded direct pose-report non-root `max_pose` falls to
  `0.054419/0.087491/0.107007` at frames `15/30/45`; the only large remaining
  row is the known unconsumed `Control_Root` diagnostic row at `39.417502`.
  Focused tests pass `40/40`. Then built a temporary one-clip direct-storage
  sourcepos main MILO (SHA
  `E405C0D19371285A339960EEAF7AA98C9FFAD1C2ED34E571B51E017C918C6839`), swapped
  it into loose DLC only for capture, and restored deployed main to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
  Captured f15/f30/f45 at yaws `0/90` from local
  `gh2_ps2_hybrid_assets/GEN` only, no ISO runtime access, with the validated
  no-front-camera debug-yaw harness and
  `GHOGX_DIAGNOSTIC_PERFORMER_CLIP=guitarist0=gh3_guit_mido_a_attackl`; all six
  captures exited zero and proved loose-DLC load, Midori variant, clip override,
  and screenshot save. Visual result is still rejected: exact position baking
  improves endpoint data but rendered front views remain tangled/occluded and
  side views still miss the source sideways airborne silhouette. Evidence:
  `.codex/current-evidence/midori-sourcepos-direct-diagnostic-20260817/numeric_decision.json`,
  `.codex/current-evidence/midori-sourcepos-direct-visual-20260817/visual_decision.json`,
  and
  `.codex/current-evidence/midori-sourcepos-direct-visual-20260817/sourcepos_direct_contact_sheet.png`.
  Next action: run a focused runtime/offline output-map comparison for the
  sourcepos candidate to determine whether GHC reconstructs the baked `.pos`
  rows as the offline report predicts. If yes, continue with root/rotation
  frame composition rather than more position baking.
- 2026-08-17 sourcepos rootdiag sampling finding: resumed at the
  pelvis-only/output-map diagnosis with low-priority processes and local
  `gh2_ps2_hybrid_assets/GEN` only. A first focused run used an older temp
  converter and reproduced the old harness issue: the candidate contained
  `gh3_guit_mido_a_attackl`, but no fallback `normal` `CharClipGroup`, so GHC
  reported the diagnostic clip override missing. Rebuilt current
  `GuitarHeroOGX-main-ui-engine/tools/milo_convert` into `%TEMP%` at Idle
  priority, rebuilt the one-clip candidate, and recovered the prior accepted
  candidate SHA
  `E405C0D19371285A339960EEAF7AA98C9FFAD1C2ED34E571B51E017C918C6839` with
  `group normal clips=gh3_guit_mido_a_attackl`. The valid rootdiag accepted
  `GHOGX_DIAGNOSTIC_PERFORMER_CLIP=guitarist0=gh3_guit_mido_a_attackl`, saved
  a screenshot, and restored the deployed main MILO to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`; `D:\GEN`
  stayed false and no ISO was used at game time. New finding: the sourcepos
  rows are present in the MILO at sample `45`, but runtime output-map for
  Midori stayed on startup sample rows despite
  `GHOGX_DIAGNOSTIC_PERFORMER_CLIP_TIME=guitarist0=0.75`
  (`bone_pelvis` runtime `outLocal/meshLocal=(0,0,39.406)`,
  `bone_L-knee≈-13.264`, `bone_L-ankle≈-17.561`). Evidence:
  `.codex/current-evidence/midori-sourcepos-rootdiag-20260817/rootdiag_summary.json`.
  Next action: validate/fix forced clip-time sampling, or build a frozen
  one-sample f45 diagnostic alias, then rerun output-map before drawing a
  conclusion about runtime consumption of sourcepos `.pos` rows.
- 2026-08-17 frozen f45 sourcepos bridge: added diagnostic
  `--freeze-sample` to `tools/gh3_midori_acp_stage.py` so a one-sample ACP can
  be emitted from any source sample index without changing normal staging.
  Built a one-sample f45 sourcepos `gh3_midori_main.milo_ps2` (SHA
  `E6BD8D5B0D44EAD603A0606F8B72CB4635EA1D4DA7D51E4845E6D817986E2390`,
  1609 bytes), verified sample `0` contains the prior f45 sourcepos rows, and
  captured a focused output-map run from local `gh2_ps2_hybrid_assets/GEN`
  only. GHC accepted the `gh3_guit_mido_a_attackl` override and published the
  f45 rows into Midori's mesh locals: pelvis `(0.038,0.047,39.432)`, left knee
  `(-0.052,0,0)`, left ankle `(-0.069,0,0)`, left thigh
  `(-2.192,-14.673,-8.792)`. Deployed main restored to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`;
  `D:\GEN=False`; no ISO runtime access. The side-view screenshot is still
  visually rejected: Midori is curled/folded near the highway/floor instead of
  matching the source airborne sideways silhouette. Evidence:
  `.codex/current-evidence/midori-sourcepos-timefix-20260817/freeze45_decision.json`
  and
  `.codex/current-evidence/midori-sourcepos-timefix-20260817/freeze45_yaw90_rootdiag.png`.
  Decision: sourcepos `.pos` rows are consumed; stop adding position baking and
  continue with pelvis/Control_Root/root rotation-frame composition using the
  frozen f45 harness.
- 2026-08-17 sourcepos root-frame matrix-order sweep: added three diagnostic
  variants that keep frozen f45 sourcepos `.pos` rows but solve
  `Bone_Pelvis`/torso frames from mapped source child positions and vary the
  solved-frame composition:
  `...sourcepos-desired-localt-bakepos-bind`,
  `...sourcepos-desiredt-local-bakepos-bind`, and
  `...sourcepos-localt-desired-bakepos-bind`. The unchanged default still
  rebuilds to SHA
  `E6BD8D5B0D44EAD603A0606F8B72CB4635EA1D4DA7D51E4845E6D817986E2390`;
  focused tests pass `40/40`. Direct local-GEN captures for all three new
  variants ran at idle priority, used no ISO, left `D:\GEN=False`, and restored
  deployed main SHA
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
  All three reject visually; `desiredt-local` moves out of the old curled family
  but still folds into the highway/guitar. Evidence:
  `.codex/current-evidence/midori-sourcepos-rootrot-sweep-20260817/rootrot_decision.json`
  and
  `.codex/current-evidence/midori-sourcepos-rootrot-sweep-20260817/sourcepos_rootrot_f45_contact.png`.
  Next branch: matrix order alone is not the missing contract; derive a
  different pelvis basis/root frame from evaluated `Control_Root` +
  `Bone_Pelvis` axes, or use an automated GLB bridge to compare source and
  target root frames before ordinary MILO emission.
- 2026-08-17 sourcepos Control_Root fold diagnostics: added
  `...sourcepos-foldroot-bakepos-bind`, `...sourcepos-rootfold-bakepos-bind`,
  `...sourcepos-invfoldroot-bakepos-bind`, and
  `...sourcepos-rootinvfold-bakepos-bind`. These keep frozen f45 sourcepos
  position baking but fold either the axis-aligned evaluated `Control_Root`
  world frame, or its inverse, into rootless GH2 `bone_pelvis`. Focused tests
  pass `40/40`. Numeric triage selected `foldroot` over `rootfold`, but direct
  local-GEN f45 capture rejected `foldroot`: the pose collapses low across the
  highway with limbs/guitar spread. Inverse variants were not captured because
  pose-report triage was worse than `foldroot`. Capture used local
  `gh2_ps2_hybrid_assets/GEN`, idle priority, no ISO, `D:\GEN=False`, and
  restored deployed main SHA
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
  Evidence:
  `.codex/current-evidence/midori-sourcepos-foldroot-20260817/foldroot_decision.json`
  and
  `.codex/current-evidence/midori-sourcepos-foldroot-20260817/sourcepos-foldroot_yaw90_f45.png`.
  Next branch: compare source GLB/evaluated and GH2 target root axes directly
  and derive a new pelvis basis rather than composing the full `Control_Root`
  frame into `bone_pelvis`.
- 2026-08-17 sourcepos Control_Root yaw-only diagnostics: added
  `...sourcepos-yawfold-bakepos-bind` and
  `...sourcepos-rootyawfold-bakepos-bind`. These keep the sourcepos/target solve
  path but extract only the axis-aligned `Control_Root` twist around target
  root local Z, then fold that yaw delta into rootless GH2 `bone_pelvis`.
  Focused tests pass `40/40`. In-memory pose-report triage at frames 15/30/45
  rejects `rootyawfold` numerically (`max_pose` up to `0.128624`, right shin
  dot as low as `0.038974`, left shin dot as low as `0.276683`). `yawfold` is
  mixed: it improves frame 15 `max_pose` versus sourcepos baseline (`0.050450`
  versus `0.054419`) and keeps strong right-shin agreement at frame 45
  (`0.907925`), but does not beat the already visually rejected full `foldroot`
  on frame 45 `max_pose`. No game runtime/capture was launched, no ISO was
  used, and `D:\GEN=False`. Evidence:
  `.codex/current-evidence/midori-sourcepos-yawroot-20260817/yawroot_decision.json`.
  Next branch: if spending a visual pass, capture only `yawfold` frozen f45
  from local `gh2_ps2_hybrid_assets/GEN` at idle priority; otherwise continue
  to a GLB/evaluated bridge comparison that solves the pelvis basis without
  folding any root pitch/roll into `bone_pelvis`.
- 2026-08-17 sourcepos pelvis-basis diagnostics: added
  `...sourcepos-axislocal-bakepos-bind` plus `axisblend35/50/65` variants.
  These keep the sourcepos target-skeleton solve and source-position bake, but
  replace rootless GH2 `bone_pelvis` rotation with the bridge-evaluated
  axis-aligned local pelvis basis, or blend from the prior `yawfold` pelvis
  toward that basis. Focused tests pass `40/40`. In-memory pose reports reject
  the branch numerically: pure `axislocal` improves frame 45 `max_pose`
  (`0.052164`) but catastrophically flips frame 15 left shin (`-0.993090`);
  `axisblend35` still worsens frame 15 (`max_pose=0.097295`, right shin dot
  `0.061172`), and stronger blends degrade further. No game runtime/capture
  was launched, no ISO was used, and `D:\GEN=False`. Evidence:
  `.codex/current-evidence/midori-pelvisbasis-20260817/pelvisbasis_decision.json`.
  Next branch: do not visually capture `axislocal`/`axisblend`; either capture
  prior `yawfold` frozen f45 from local `gh2_ps2_hybrid_assets/GEN` at idle
  priority, or move to a real automated GLB/constraint bake that solves pelvis
  and legs together instead of changing pelvis alone.
- 2026-08-17 pelvis correction probe: reused the existing ACP
  staging/reconstruction path to compare `yawfold` pelvis world rotation
  against a position-derived pelvis frame from the evaluated GLB/source bridge
  at frames 15/30/45. A single constant correction is numerically rejected: the
  yawfold-to-position-frame correction changes by `61.985417` degrees from
  f15/f30, `117.258086` degrees from f15/f45, and `61.890271` degrees from
  f30/f45. The older `matrix-leg-constraint-bridge-pelvisgate-bind` policy was
  also rechecked and is not sourcepos-aligned (`max_pose` around `55-57`), so it
  is not useful for this branch. No game runtime/capture was launched, no ISO
  was used, and `D:\GEN=False`. Evidence:
  `.codex/current-evidence/midori-constraint-bridge-20260817/pelvis_correction_decision.json`.
  Next branch: stop spending time on constant pelvis-only matrix variants. Use
  either a direct local-GEN frozen f45 capture of prior `yawfold`, or implement
  a true frame-wise automated GLB/constraint bake that solves pelvis and legs as
  one chain.
- 2026-08-17 yawfold frozen f45 capture attempt: built the current
  `milo_convert_tool` in `%TEMP%` at Idle priority, staged a one-sample f45
  `...sourcepos-yawfold-bakepos-bind` guitar-main ACP, and converted it to
  `gh3_midori_main.milo_ps2` (SHA
  `AC9CA4EC4D4201C792084972D6240C5381025FC22113393DAF19FEA3E516E299`, 1,610
  bytes). Temporarily swapped only the loose-DLC main MILO, launched GHC from
  local `gh2_ps2_hybrid_assets/GEN` with the validated no-front-camera debug
  yaw90 harness and `ghogx_app` at Idle priority, and restored deployed main to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
  No ISO was used and `D:\GEN=False`. The visual attempt is incomplete, not a
  pass or reject: under the 180-second Idle cap no screenshot was saved, though
  logs proved local DLC catalog load, Midori variant selection, and local GEN
  runtime. Evidence:
  `.codex/current-evidence/midori-yawfold-visual-20260817/yawfold_capture_attempt.json`.
  Cleanup removed the temp converter build and staged ACP trees; retained only
  the 1,610-byte candidate MILO and compact JSON. Next branch: either run the
  same yawfold capture with a longer timeout or reduce load cost/warm the
  harness; otherwise proceed to the frame-wise automated GLB/constraint bake.
- 2026-08-17 yawfold frozen f45 visual reject: reran the same yawfold candidate
  from loose DLC against local `gh2_ps2_hybrid_assets/GEN` only, with
  `ghogx_app` forced to Idle priority and `D:\GEN=False`. The delayed capture
  produced `yawfold_f45_yaw90.png`; direct inspection rejects the policy because
  Midori is still visibly folded/contorted around pelvis, legs, torso, and
  guitar. The deployed main MILO was restored to
  `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`, and no
  GHOGX/PCSX2 process remains running. Evidence:
  `.codex/current-evidence/midori-yawfold-visual-20260817/yawfold_visual_decision.json`.
  Next branch: stop pelvis-only/yaw-only variants and move to an automated
  frame-wise GLB/constraint bake or equivalent matrix-local hierarchy solve that
  treats pelvis and legs as one chain.
- 2026-08-17 explicit intermediate skeleton export: extended
  `tools/gh3_midori_pose_report.py` with `--skeleton-output`, which materializes
  staged ACP output as an intermediate target-skeleton JSON containing local and
  world rotations/translations per frame. Added
  `tools/gh3_midori_intermediate_skeleton_glb.py`, which converts that JSON into
  an automated visible GLB line-skeleton diagnostic. Ran the existing
  `glbframeik` compose family plus `glbaxisik` against frames 15/30/45 via an
  Idle child process. The branch is numerically rejected: best row is plain
  `glbframeik` at frame 30 with `max_pose_error=18.017467`, right-shin dot
  `0.607422`, and left-shin dot `0.810763`; worst row is
  `glbframeik-localt-desired` at frame 15 with `max_pose_error=27.951382`.
  Exported
  `.codex/current-evidence/midori-glbframeik-20260817/intermediate_skeleton_glbframeik.glb`
  (1,640 bytes, 24 vertices, 18 line edges, GLB v2 header validated). Focused
  tests pass `41/41`. No game runtime was launched, no ISO was used, and
  `D:\GEN=False`. Evidence:
  `.codex/current-evidence/midori-glbframeik-20260817/glbframeik_decision.json`,
  `.codex/current-evidence/midori-glbframeik-20260817/pose_report_glbframeik.json`,
  and
  `.codex/current-evidence/midori-glbframeik-20260817/intermediate_skeleton_glbframeik.json`.
  Next branch: inspect/consume the explicit intermediate-skeleton GLB/JSON to
  identify the wrong matrix-local basis, then implement a corrected hierarchy
  solve instead of adding more hidden pelvis-only variants.
- 2026-08-17 frame-IK bake-position diagnosis: added
  `...glbframeik-bakepos-bind`,
  `...glbframeik-sourcepos-bakepos-bind`, and
  `...glbframeik-sourcepos-bakepos-altlocal-bind`. The plain bakepos branch is
  rejected (`best max_pose_error=13.315613`) because it still emits bind-length
  knee/ankle offsets; segment audit keeps a `13.21` unit thigh-to-knee length
  delta. The sourcepos-bakepos branch is the useful narrowed branch:
  `max_pose_error` is `0.098296/0.130078/0.101254` at frames 15/30/45, and the
  segment audit shows `0.0` length delta on the worst segments, proving the
  source-position bake fixes chain length. Remaining failure is direction/basis:
  worst direction dots are `-0.799569` at f15 `Rthigh_to_Rknee`, `0.161688` at
  f30 `Rthigh_to_Rknee`, and `0.378556` at f45 `Lthigh_to_Lknee`. The altlocal
  sourcepos branch is rejected (`max_pose_error=23.875582/33.346062/33.674302`).
  Focused tests pass `43/43`. No game runtime was launched, no ISO was used, and
  `D:\GEN=False`. Evidence:
  `.codex/current-evidence/midori-glbframeik-bakepos-20260817/glbframeik_bakepos_decision.json`.
  Next branch: keep sourcepos-bakepos as the narrowed diagnostic and fix the
  remaining thigh/knee direction basis in the frame-IK rotation solve before
  spending game capture time.
- 2026-08-17 sourceposrot frame-IK correction-order sweep: added
  `...glbframeik-sourceposrot-bakepos-bind`,
  `...glbframeik-sourceposrot-pre-bakepos-bind`,
  `...glbframeik-sourceposrot-post-bakepos-bind`, and
  `...glbframeik-sourceposrot-pret-bakepos-bind`. These aim frame-IK rotations
  at source-position segment vectors and sweep correction composition order.
  All sourceposrot variants are rejected: they share the same numeric envelope
  (`max_pose_max=0.126262`, `max_pose_avg=0.112203`) and worsen shin direction
  (`min_shin_dot=-0.344378`) versus plain `glbframeik-sourcepos-bakepos`.
  A comparison against existing sourcepos frame-solve policies reconfirms the
  best numeric branch is still
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-bakepos-bind`
  (`max_pose_max=0.107007`, `max_pose_avg=0.082972`, `min_shin_dot=0.541926`).
  Desired/local transpose variants are rejected by negative shin direction dots
  down to `-0.902602`. Focused tests pass `43/43`. No game runtime was launched,
  no ISO was used, and `D:\GEN=False`. Evidence:
  `.codex/current-evidence/midori-glbframeik-sourceposrot-20260817/sourceposrot_decision.json`.
  Next branch: stop pursuing frame-IK sourceposrot order variants. Return to the
  best sourcepos targetsolveskel branch and diagnose its remaining
  `Control_Root`/pelvis basis before visual capture.
- 2026-08-17 sourcepos targetsolveskel pelvis/torso basis diagnosis: added
  `...sourcepos-pelvisframe-bakepos-bind`,
  `...sourcepos-torsoframe-bakepos-bind`, and
  `...sourcepos-torsoframe-kneeaim-bakepos-bind`, plus
  `tools/gh3_midori_rotation_basis_audit.py`. The rotation audit proved the
  baseline best numeric branch has a torso basis bug: pelvis-to-stomach
  row-direction dots are `-0.942928/-0.945080/-0.802354` at frames 15/30/45,
  even though its lower-body summary remains best balanced
  (`max_pose_max=0.107007`, `max_pose_avg=0.082972`, `min_shin_dot=0.541926`).
  `pelvisframe`/`torsoframe` fix the torso aim and improve f30/f45 lower-body
  pose (`max_pose_max=0.097062`, `max_pose_avg=0.077272`), but regress frame 15
  right shin badly (`min_shin_dot=0.094157`). `torsoframe-kneeaim` is rejected:
  it worsens to `max_pose_max=0.153258`, `max_pose_avg=0.122818`, and
  `min_shin_dot=-0.941772`. Focused tests pass `44/44`. No game runtime was
  launched, no ISO was used, and `D:\GEN=False`. Evidence:
  `.codex/current-evidence/midori-sourcepos-targetsolveskel-basis-20260817/sourcepos_targetsolveskel_basis_decision.json`.
  Next branch: do not use kneeaim. Either keep baseline sourcepos for balanced
  legs despite bad torso aim, or add a gated torsoframe that avoids the frame 15
  right-shin regression before spending visual capture time.
- 2026-08-17 sourcepos torsoframe right-fold gate: added
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-torsoframe-rightfoldgate-bakepos-bind`.
  The gate uses source right thigh/shin fold direction (`dot > 0`) to skip
  torsoframe at f15 while enabling it at f30/f45. Pose report confirms the
  intended blend: f15 matches baseline (`max_pose_error=0.054419`,
  right-shin dot `0.834330`), f30/f45 match torsoframe
  (`0.069256/0.065499`, right-shin dots `0.800051/0.863442`). Aggregate is the
  best diagnostic so far: `max_pose_max=0.069256`, `max_pose_avg=0.063058`,
  `min_shin_dot=0.716983`. However, rotation-basis audit still shows the f15
  baseline pelvis-to-stomach inversion (`row_direction_dot=-0.942928`), so this
  is promising but not visual-ready. Focused tests pass `44/44`. No game
  runtime was launched, no ISO was used, and diagnostics ran at low/Idle
  priority. Evidence:
  `.codex/current-evidence/midori-sourcepos-torsoframe-rightfoldgate-20260817/rightfoldgate_decision.json`.
  Next branch: fix the f15 torso/pelvis basis without reintroducing the
  right-shin collapse; do not spend local-GEN visual capture time until that
  remaining basis issue is resolved or explicitly accepted for a visual check.
- 2026-08-18 f15 torsoframe right-leg aim diagnosis: added targeted policies
  for the remaining f15 torso/leg conflict:
  `...sourcepos-torsoframe-rkneebakeaim-bakepos-bind`,
  `...sourcepos-torsoframe-rkneeaim-bakepos-bind`,
  `...sourcepos-torsoframe-rlegaim-bakepos-bind`, and
  `...sourcepos-torsoframe-pelvisblend50-bakepos-bind`. `rkneebakeaim` is a
  no-op against the failing metric. `rkneeaim` improves f15
  `max_pose_error` to `0.086690` but leaves right-shin dot at `0.183920`.
  `pelvisblend50` is rejected by negative f15 shin dots
  (`-0.545129/-0.159121`). The best new branch is `rlegaim`: it keeps
  torsoframe active at f15, preserves positive torso rows, raises f15
  right-shin dot to `0.999384`, and gives aggregate
  `max_pose_max=0.070177`, `max_pose_avg=0.068311`,
  `min_shin_dot=0.716983`. Remaining caveat: intermediate segment audit still
  flags the short f15 `Rthigh_to_Rknee` segment direction (`0.082772`), so this
  is a plausible visual candidate but not approved. Focused tests pass
  `44/44`. No game runtime was launched, no ISO was used, and diagnostics ran
  at low/Idle priority. Evidence:
  `.codex/current-evidence/midori-sourcepos-torsoframe-rkneebakeaim-20260818/rlegaim_decision.json`.
  Next branch: either use `rlegaim` for a local-GEN visual capture at low
  priority, or refine the f15 right-thigh segment direction first. Do not use
  `pelvisblend50` or `rkneebakeaim`.
- 2026-08-18 rlegaim local-GEN build/capture: updated
  `tools/gh3_midori_build_pipeline.py` with `--rotation-policy` so candidate
  ACP/MILO/DLC builds are automated instead of hand-edited. Narrowed
  `tools/gh3_midori_acp_stage.py` so sourcepos targetsolveskel bakepos policies
  can use built-in source position evaluation without diagnostic
  `--source-pose-bridge-json`. Added `--low-priority` to
  `tools/gh3_midori_gameplay_proof.py`; GHC captures launch with Windows
  below-normal priority. Rebuilt all rlegaim ACP clips (`331`, split
  `266/6/23/36`), rebuilt animation/model MILOs, packaged engine DLC, and
  deployed byte-verified loose DLC to
  `gh2_ps2_hybrid_assets/DLC/community.gh3.midori`. Captured both gameplay
  variants from local `gh2_ps2_hybrid_assets/GEN` plus loose DLC:
  `analysis/gh3_midori_ghc_gameplay_proofs/gh3_midori_1_variant_gameplay_guitarist0_f60.bmp`
  and
  `analysis/gh3_midori_ghc_gameplay_proofs/gh3_midori_2_variant_gameplay_guitarist0_f60.bmp`.
  Gameplay proof status is `in_song_midori_variant_animation_verified`,
  `failure_count=0`, `proof_count=2`. No ISO was used or mounted at game time.
  Aggregate pipeline verifier has one known stale catalog-test hash failure
  (`catalog_test_matches_current_package_hashes`) because
  `analysis/gh3_midori_catalog_test_summary.json` was not rerun for the new
  animation hashes; do not treat aggregate status as final until that test is
  refreshed. Evidence:
  `.codex/current-evidence/midori-rlegaim-build-candidate-20260818/rlegaim_build_capture_decision.json`.
  Next gate: user direct visual approval of the two screenshots. If rejected,
  continue from rlegaim visual evidence and the f15 `Rthigh_to_Rknee` segment
  direction warning.
- 2026-08-18 catalog refresh for rlegaim: configured/built
  `ghogx_character_variant_catalog_test` with Visual Studio 2022 at low/Idle
  priority after the Ninja preset was unavailable in this shell. Ran
  `ghogx_character_variant_catalog_test --midori-assets-only` against local
  `gh2_ps2_hybrid_assets/GEN/MAIN.HDR` and
  `gh2_ps2_hybrid_assets/GEN/MAIN_0.ARK`; it passed
  (`Midori external assets: 2 models, 2 textures, 331 clips`). Refreshed
  `analysis/gh3_midori_catalog_test_summary.json` with current rlegaim package
  asset hashes and local-GEN evidence. Reran
  `tools/gh3_midori_pipeline_verify.py`: status
  `guitar_hero_classic_midori_runtime_verified`, `checks=79`, `failures=0`,
  `clips=280`, `assets=6`. Focused unit tests still pass `44/44`. No ISO was
  used or mounted at game time. Evidence:
  `.codex/current-evidence/midori-catalog-refresh-20260818/catalog_refresh_decision.json`.
  Next gate remains user direct visual approval of the two rlegaim gameplay
  screenshots.
- 2026-08-18 rlegaim representative pose review: added `--low-priority` to
  `tools/gh3_midori_pose_review.py` so native viewer captures launch with
  Windows below-normal priority. Ran the pose review from local
  `gh2_ps2_hybrid_assets/GEN` plus deployed loose DLC under
  `gh2_ps2_hybrid_assets/DLC`; status
  `native_viewer_representative_pose_framing_review_passed`, `proofs=9`,
  `failures=0`, `min_margin=10`. Generated contact sheet:
  `analysis/gh3_midori_pose_review_proofs/rlegaim_pose_review_contact_sheet.jpg`.
  Reran aggregate verifier: `guitar_hero_classic_midori_runtime_verified`,
  `checks=79`, `failures=0`. Unit tests still pass `44/44`. No ISO was used or
  mounted at game time. Evidence:
  `.codex/current-evidence/midori-rlegaim-pose-review-20260818/pose_review_decision.json`.
  Next gate remains user direct visual approval of the gameplay screenshots and
  representative pose contact sheet.
- 2026-08-18 rlegaim visual rejection: user reviewed
  `analysis/gh3_midori_pose_review_proofs/rlegaim_pose_review_contact_sheet.jpg`
  and reported that only the middle-right pose is coherent. That grid position
  is `midori_1_accessory_acc01_f030` (`gh3_midori_1`,
  `gh3_guit_mido_acc01`, frame 30). Treat rlegaim as visually rejected despite
  the green gameplay, pose-framing, catalog, and aggregate audits. The package
  loads as loose DLC from local `gh2_ps2_hybrid_assets/GEN`, but normal guitar
  animation poses remain incoherent. Evidence:
  `.codex/current-evidence/midori-rlegaim-visual-rejection-20260818/visual_rejection_decision.json`.
  Next branch: compare the coherent accessory/static case against failing
  guitar-animation cases, then broaden the matrix-local/Control_Root diagnosis
  beyond the sampled `attackl` frames. Do not call Midori complete until direct
  user visual approval passes.
- 2026-08-18 post-rejection source activity comparison: direct visual
  inspection confirms the rejected contact sheet has multiple non-bipedal,
  collapsed guitar poses; the previous pose-review script only proved framing,
  not semantic pose quality. Source IR comparison shows the only coherent
  contact-sheet case, `gh3_guitarist_midori_acc01`, is sparse
  (`animated_bone_count=3`, `trans_changes=6`) and has no sampled
  Control_Root/pelvis/torso/leg activity. The failed guitar cases exercise the
  full body (`animated_bone_count=16..50`), with pelvis translation keyed
  `10..48` times and heavy torso/leg rotation. Evidence:
  `.codex/current-evidence/midori-rlegaim-visual-rejection-20260818/source_clip_activity_comparison.json`.
  Conclusion: rlegaim did not solve the full-body animation space; stop
  treating accessory/static coherence as a candidate success signal.
- 2026-08-18 multi-clip pose score refresh: repaired stale diagnostic script
  `tools/gh3_midori_pose_score.py` after `compose_rotation_with_bind` gained
  `hmx_quat_mode`; diagnostic calls now pass `direct`. Ran the scorer at Idle
  priority on the rejected contact-sheet body clips. The failed clips still
  show high mean body-bone rotation distances even under the best simple
  variant (`edit-local`): medium idle `74.42`, attackl `77.31`, fast jump
  `69.65`, fast solo `74.50`, transition out `62.11` degrees. The coherent
  accessory clip has `body_bone_count=0`, confirming it does not exercise this
  space. Evidence:
  `.codex/current-evidence/midori-rlegaim-visual-rejection-20260818/pose_score_multiclip_summary.json`.
  Next branch must reopen full-body matrix-local/Control_Root/pelvis transform
  evaluation across these clips, or automate GLB/source-bridge comparison after
  source file regeneration.
- 2026-08-18 GLB/source bridge reopening: added automated low-priority bridge
  generation support. `tools/gh3_midori_source_bridge_export.py` now accepts
  `--low-priority`; new
  `tools/gh3_midori_review_source_bridge_batch.py` extracts only the required
  GH3 Midori source files, emits Blender/NXTools GLB + pose JSON for review
  clips, and deletes extracted source scratch. Recreated the pinned NXTools
  checkout in `%TEMP%` at commit `6cea808a27d6773bde55e947c9f0ffd72081e164`
  and generated source bridges for the five visually failed body cases:
  medium idle, attack left, fast jump, fast solo, and transition out. The
  accessory case was intentionally excluded from the clean manifest after
  Blender/NXTools retained no body curves for the requested source bones,
  proving it is not a body-retarget success case. Evidence:
  `.codex/current-evidence/midori-review-source-bridges-20260818/review_source_bridge_batch_manifest.json`.
  Added `tools/gh3_midori_bridge_pose_compare.py`; bridge-vs-IR comparison
  shows the raw IR evaluator is not source-authoritative for these full-body
  poses (`ir_evaluated_vs_blender` mean mismatch: idle `140.220`, attack
  `136.800`, jump `134.033`, solo `150.824`, transition `125.839` degrees).
  Evidence:
  `.codex/current-evidence/midori-review-source-bridges-20260818/bridge_pose_compare_summary.json`
  and
  `.codex/current-evidence/midori-review-source-bridges-20260818/bridge_diagnosis_decision.json`.
  Next branch: stop deriving final candidate rotations from raw source IR
  pose reconstruction alone. Use the Blender/NXTools GLB pose bridge as the
  authoritative source pose stream for the target-local/Control_Root/pelvis
  derivation, then automate bridge generation for production scope if the
  bridge-fed candidate becomes visually viable.
- 2026-08-18 refreshed two-frame bridge/policy sweep: updated
  `tools/gh3_midori_review_source_bridge_batch.py` so review cases export frame
  `0` plus the reviewed frame; regenerated the five failed body bridges with
  extracted GH3 source scratch removed afterward. This makes bridge-fed pelvis
  translation policies meaningful instead of using the review frame as its own
  base. Updated `tools/gh3_midori_pose_report.py` and
  `tools/gh3_midori_acp_stage.py` so duplicate target clips are disambiguated
  by the source-pose bridge animation path (`frontend/gh3_guit_midori_tran_atoout`
  vs non-frontend). Added
  `tools/gh3_midori_bridge_policy_multiclip_report.py` and swept bridge-fed
  policies across all five body cases. Result: no current policy is promotable.
  `glbframeik-sourcepos` has low position error but flips right shin on
  `fast_jump` (`-0.524948`) and left shin on `fast_solo` (`-0.400349`).
  `glbframeik-sourceposrot` flips attack shins (`right=-0.344378`,
  `left=-0.080896`). Current `rlegaim`, already visually rejected, now also
  shows a bridge-fed right-shin flip on `fast_jump` (`-0.471900`), despite its
  tempting average pose error. Evidence:
  `.codex/current-evidence/midori-review-source-bridges-20260818/bridge_policy_decision.json`.
  Next branch: develop a bridge-position two-bone leg solve or bend-plane sign
  gate driven from GLB pose bridge data, and require all five failed body cases
  to have positive left/right shin dots before spending capture time.
- 2026-08-18 legbakeaim registration refresh: user visual gate remains strict:
  all current rlegaim body captures are rejected as non-bipedal except the
  middle-right accessory pose, and that accessory pose has no body curves, so it
  is not a valid body-retarget success. Added a diagnostic full-leg bake-aim
  policy, registered it through the torso/pelvis path, added bridge-report
  coverage, and reran tests at Idle priority (`python -m unittest
  tools.gh3_midori_pipeline_test`: 44 tests OK). After full registration, the
  candidate is rejected: average pose error `0.114641`, max `0.155886`, min
  right shin dot `-0.510482` on `fast_solo`, min left shin dot `0.422834`.
  Evidence:
  `.codex/current-evidence/midori-review-source-bridges-20260818/bridge_legbakeaim_registration_decision.json`.
  The earlier no-negative-shin result was a false positive from an
  incompletely registered policy path. Next branch returns to the pelvis-only
  matrix-local/Control_Root diagnosis with Blender/NXTools GLB pose bridge data
  as source-authoritative. Do not run from the GH2 ISO at game time; use local
  `gh2_ps2_hybrid_assets/GEN` and loose DLC only.
- 2026-08-18 root/pelvis GLB bridge diagnostic and ihatecompvir routing:
  added `tools/gh3_midori_root_pelvis_bridge_diagnostic.py` and ran it at Idle
  over the five failed body clips using the existing Blender/NXTools pose
  bridges. Control_Root rotation delta is negligible (`max=0.016083` deg);
  Control_Root mapped translation is zero for the four main body cases and
  large only on transition-out (`16.224950` GH2 units). The repeated signal is
  pelvis world-vs-Control_Root-local mismatch (`122.666104` deg for the four
  main cases; `89.996` for transition-out), with tiny pelvis-minus-root
  translation deltas. Evidence:
  `.codex/current-evidence/midori-review-source-bridges-20260818/root_pelvis_bridge_diagnostic.json`.
  User also pointed out ihatecompvir tooling for GLB to MILO. Confirmed local
  sources are available in `ihatecompvir-public-milo-sources`: `glTFMilo`,
  `MiloEditor/MiloLib`, `milo_blender`, and GH2 CharClipSamples notes. Current
  usage was only the NXTools/Blender source-bridge half. `glTFMilo` itself is
  not a drop-in Midori animation path: its CLI targets RB2/RB3/TBRB xbox/ps3 and
  writes GLB animation as generic `TransAnim`, not GH2 PS2 guitarist
  `CharClipSet`/`CharClipSamples`. MiloLib is still directly useful because it
  models GH2 PS2 `CharClipSet` revision 14 and `CharClip` raw sample containers.
  Evidence:
  `.codex/current-evidence/midori-review-source-bridges-20260818/ihatecompvir_glb_milo_assessment.json`.
  Next branch should use MiloLib/ihatecompvir semantics to validate or replace
  the existing C++ GH2 CharClipSamples writer fed by GLB pose samples, while
  fixing the pelvis parent/local interpretation before spending capture time.
- 2026-08-18 pelvis/root-fold numeric candidate: target graph inspection
  confirms GH3 source `Bone_Pelvis` is parented to `Control_Root`, while GH2
  target `bone_pelvis` is root-level in the current mapping. Swept the existing
  sourcepos root-fold policies across the five failed body cases with GLB pose
  bridges. `rootyawfold` is the first branch in this pelvis/root pass to clear
  positive shin dots across all five cases: avg pose error `0.146616`, max
  `0.176421`, min right shin `0.276150`, min left shin `0.315446`. It is not
  visually approved and is only a candidate for the next local-GEN capture.
  Evidence:
  `.codex/current-evidence/midori-review-source-bridges-20260818/bridge_pelvisroot_decision.json`.
  Added per-clip GLB pose bridge manifest support: `tools/gh3_midori_acp_stage.py`
  now accepts `--source-pose-bridge-manifest`, and
  `tools/gh3_midori_build_pipeline.py` forwards it. Focused tests pass
  (`python -m unittest tools.gh3_midori_pipeline_test`: 45 tests OK). A one-clip
  `fast_solo` staging probe with `rootyawfold` confirmed
  `source_pose_bridge_active=True` and frames `0,90`; the raw ACP probe was
  deleted as rebuildable. Next branch: build/capture rootyawfold from local
  loose DLC only, with the bridge manifest passed through, or expand automated
  GLB bridge generation to broader production scope first if capture coverage
  requires more clips.
- 2026-08-18 rootyawfold structural build and pelvis matrix-local diagnosis:
  built/deployed `rootyawfold` structurally from local staged/source assets at
  Idle priority, with no game launch and no GH2 ISO game-time use. Structural
  output passed (`331` staged clips; anim MILOs `26,285,178` bytes; loose DLC
  `26,844,881` bytes), but retained yawfold visual
  `.codex/current-evidence/midori-yawfold-visual-20260817/yawfold_f45_yaw90.png`
  is rejected as non-bipedal. Added
  `tools/gh3_midori_pelvis_matrix_local_diagnostic.py`; report shows static
  `Control_Root` absolute basis is the main mismatch (`122.666098` deg from
  identity), while root delta is negligible (`0.016083` deg). `Bone_Pelvis`
  world-vs-parent-local mismatch is `122.666104` deg, and
  `Bone_Pelvis.matrix_local` is static rest data, not animated pose. Added
  diagnostic policy `matrix-bridge-eval-world-targetgraph-axis-align-bind`, but
  five-case bridge triage rejects it before capture (avg pose `57.226479`, max
  `71.423520`, min right shin `-0.580641`, min left shin `-0.320851`). Focused
  tests pass (`python -m unittest tools.gh3_midori_pipeline_test`: `46` tests
  OK). Evidence:
  `.codex/current-evidence/midori-review-source-bridges-20260818/pelvis_matrix_local_diagnostic.json`,
  `.codex/current-evidence/midori-review-source-bridges-20260818/targetgraph_evalworld_policy_report.json`,
  `.codex/current-evidence/midori-review-source-bridges-20260818/rootyawfold_structural_and_pelvis_matrix_decision.json`.
  Next branch: do not spend more capture slots on rootyawfold; use local
  ihatecompvir MiloLib/CharClipSamples references to validate or replace the
  existing C++ ACP-to-GH2 `CharClipSamples10` writer fed directly from GLB pose
  samples, with consistent normalization/baking of the absent `Control_Root`
  basis across pelvis, descendants, and positions.
- 2026-08-18 bridge-base/sourcepos probe: fixed an incomplete diagnostic
  registration where new `matrix-bridge-eval-world-*targetgraph*` policies were
  in `ROTATION_POLICIES` but missing from the large matrix dispatch allowlist.
  Added evaluated-rest, bridge-frame-0-base, and bridge-base-sourcepos variants.
  The sourcepos variant uses existing target skeleton bake-position `.pos`
  channels so the lower body is not stuck with raw near-zero pelvis translation.
  Focused tests pass (`python -m unittest tools.gh3_midori_pipeline_test`: `49`
  tests OK). Five-case bridge triage rejects the new policies before capture:
  bridgebase-only avg pose `55.131754`, max `78.913822`, min right shin
  `-0.000555`, min left shin `-0.558653`; bridgebase-sourcepos avg pose
  `35.364700`, max `46.566020`, min right shin `-0.822559`, min left shin
  `-0.361987`. Evidence:
  `.codex/current-evidence/midori-review-source-bridges-20260818/bridgebase_sourcepos_policy_report.json`,
  `.codex/current-evidence/midori-review-source-bridges-20260818/bridgebase_sourcepos_decision.json`.
  Next branch remains direct GLB-pose-to-GH2 CharClipSamples writer/validator
  with target-graph local transforms and a single normalized absent
  `Control_Root` basis solve; do not spend local-GEN capture time until the
  automated bridge gate and biped plausibility gate both pass.
- 2026-08-18 bridge-base solved-skeleton compose sweep: added
  `matrix-bridge-eval-bridgebase-targetgraph-axis-align-sourcepos-solve-altlocal-bind`
  and three compose-order variants (`desired-localT`, `desiredT-local`,
  `localT-desired`). They are registered through rotation, bake-position,
  altlocal translation, and solved-skeleton policy sets. Focused tests pass
  (`python -m unittest tools.gh3_midori_pipeline_test`: `52` tests OK). All
  variants are rejected before capture. Best new branch is default
  solve-altlocal with avg pose `30.229316`, max `46.566020`, min right shin
  `-0.584740`, min left shin `-0.245391`; localT-desired improves min right
  shin to `-0.460145` but worsens min left shin to `-0.420432`. Segment audits
  show correct lengths but inverted directions: `fast_jump` worst
  `Rknee_to_Rankle` dot `-0.584736`, length delta `0.000001`; `fast_solo`
  worst `Lthigh_to_Lknee` dot `-0.906775`, length delta `0.0`. Evidence:
  `.codex/current-evidence/midori-review-source-bridges-20260818/bridgebase_solve_order_policy_report.json`,
  `.codex/current-evidence/midori-review-source-bridges-20260818/fast_jump_solve_altlocal_segment_audit.json`,
  `.codex/current-evidence/midori-review-source-bridges-20260818/fast_solo_solve_altlocal_segment_audit.json`,
  `.codex/current-evidence/midori-review-source-bridges-20260818/bridgebase_solve_order_decision.json`.
  Next branch: stop adding single compose-order variants; implement a direct
  GLB-position target-graph skeleton validator/writer that asserts each emitted
  local translation/rotation reconstructs the desired parent-child vector before
  packing GH2 `CharClipSamples`, then validate the packed layout against
  ihatecompvir MiloLib/CharClipSamples semantics.
- 2026-08-18 pack-safe targetgraph branch: added
  `matrix-bridge-eval-bridgebase-targetgraph-axis-align-sourcepos-solve-packsafe-altlocal-bind`,
  which computes bake-position child translations against the parent basis after
  HMX quaternion pack/unpack. Focused tests pass at Idle priority
  (`python -m unittest tools.gh3_midori_pipeline_test`: `53` tests OK), but the
  five-case gate rejects it before capture: avg pose `27.766754`, max
  `46.566020`, min right shin `-0.863932`, min left shin `-0.967666`.
  The isolated validator showed pack-safe translations can reconstruct desired
  positions, but position reconstruction alone does not preserve bipedal limb
  direction. No GHC run was made and no GH2 ISO was used at runtime. Evidence:
  `.codex/current-evidence/midori-review-source-bridges-20260818/target_graph_solve_validator_report.json`,
  `.codex/current-evidence/midori-review-source-bridges-20260818/packsafe_policy_report.json`,
  `.codex/current-evidence/midori-review-source-bridges-20260818/packsafe_policy_decision.json`.
  Local ihatecompvir sources are available and should be used as source/format
  references for the next branch: `glTFMilo` is useful GLB/MILO code but not a
  direct GH2 PS2 guitarist converter because it writes generic `TransAnim`, not
  `CharClipSet`/`CharClipSamples`. Continue with an automated
  GLB-pose-to-GH2-CharClipSamples writer/validator using MiloLib/CharClipSamples
  semantics and keep the positive segment-direction gate before every capture.
- 2026-08-18 fail-closed bridge gate: `tools/gh3_midori_bridge_policy_multiclip_report.py`
  now supports `--fail-on-reject`, `--gate-min-shin-dot`, and
  `--gate-max-pose-error`. With `--fail-on-reject`, omitted
  `--gate-min-shin-dot` defaults to `0.0`, so folded/non-bipedal shins are an
  immediate nonzero exit instead of a human-noticed summary row. Focused tests
  pass at Idle priority (`python -m unittest tools.gh3_midori_pipeline_test`:
  `55` tests OK). Verification against the current solve-altlocal and
  pack-safe policies exited `2` as expected and named both negative shin dots
  and high pose error. Evidence:
  `.codex/current-evidence/midori-review-source-bridges-20260818/bridge_gate_reject_report.json`,
  `.codex/current-evidence/midori-review-source-bridges-20260818/bridge_gate_decision.json`.
  This is a pre-capture/writer guardrail, not visual success; next branch should
  wire it into build/capture automation and continue the MiloLib-backed
  GLB-pose-to-GH2 `CharClipSamples` writer/validator route.
- 2026-08-18 pipeline bridge gate wiring: `tools/gh3_midori_build_pipeline.py`
  now runs `bridge_biped_gate` when `--source-pose-bridge-manifest` is supplied
  unless `--skip-bridge-gate` is set. The gate runs immediately after source IR
  availability and before converter build, model staging, anim MILO packaging,
  DLC deploy, or GHC capture. Pipeline subprocesses now use
  `subprocess.IDLE_PRIORITY_CLASS` on Windows. The gate forwards the production
  staging settings (`animated`, `source`, `bind-delta`, `edit-inv-frame`,
  `transpose`) and defaults to min shin dot `0.0`, max pose error `1.0`.
  Focused tests pass at Idle priority (`python -m unittest
  tools.gh3_midori_pipeline_test`: `55` tests OK). A pipeline preflight with
  the current pack-safe policy stopped at `bridge_biped_gate` and exited `2`;
  it did not run model staging, anim MILO packaging, deploy, or capture.
  Production-flag gate result: avg pose `30.321585`, max pose `46.566020`, min
  right shin `-0.960752`, min left shin `-0.933769`. Evidence:
  `.codex/current-evidence/midori-review-source-bridges-20260818/pipeline_bridge_gate_report.json`,
  `.codex/current-evidence/midori-review-source-bridges-20260818/pipeline_bridge_gate_decision.json`.
- 2026-08-18 disk ACP bridge gate: added
  `tools/gh3_midori_acp_disk_bridge_gate.py` and wired
  `acp_disk_bridge_gate` into `tools/gh3_midori_build_pipeline.py` after ACP
  staging/reuse and before `build-clipset-from-acp` / GH2 `CharClipSamples`
  packaging. It reads the actual staged `.acp` files from
  `analysis/gh3_midori_acp_stage/stage_manifest.json`, reconstructs body world
  positions and child-aim directions from on-disk sample bytes, compares them
  with GLB bridge cases, and applies the biped/visual-reject gate. Focused
  tests pass at Idle priority (`python -m unittest
  tools.gh3_midori_pipeline_test`: `57` tests OK). Current staged rootyawfold
  bytes exit `2`: avg pose `6.061952`, max pose `6.068832`, min right/left shin
  `1.0/1.0`, min child-aim dot `-0.774455`. Worst child aim is
  `midori_1_fast_solo_f090` `Bone_Chest->Bone_Neck`, dot `-0.774455`, angle
  `140.756` degrees; next worst are attack-left ankle-to-toe segments. Diagnosis:
  ACP sample bytes and the GH2 `CharClipSamples` byte-preservation path are not
  the source of the rejected visual; the failure is upstream in
  pose-space/character-space interpretation of rotations and child bases.
  Evidence:
  `.codex/current-evidence/midori-review-source-bridges-20260818/acp_disk_bridge_gate_report.json`,
  `.codex/current-evidence/midori-review-source-bridges-20260818/acp_disk_bridge_gate_decision.json`.

- 2026-08-18 current approval-ready Midori candidate: the deployed route is now
  the target-side `matrix-local-axis-align-bind` loose-DLC package
  `gh2_ps2_hybrid_assets/DLC/community.gh3.midori`, running against local
  `gh2_ps2_hybrid_assets/GEN` plus loose `DLC` only. Do not resume by pushing
  the rejected GLB source bridge into MILO: the rotation-only GLB fixed the
  bogus left-leg translation injection, but remained non-bipedal after a global
  axis sweep. The current approval packet is
  `.codex/current-evidence/midori-review-source-bridges-20260818/VISUAL_APPROVAL_PACKET.md`.
  Visual sheets:
  `analysis/gh3_midori_pose_review_axisalign_redeploy_proofs/axisalign_pose_contact_9.png`
  and
  `analysis/gh3_midori_ghc_gameplay_axisalign_redeploy_proofs/axisalign_gameplay_contact_2.png`.
  Requirement audit:
  `.codex/current-evidence/midori-review-source-bridges-20260818/final_candidate_completion_audit_pending_visual_approval.json`.
  Deployed package/hash audit:
  `.codex/current-evidence/midori-review-source-bridges-20260818/current_approval_ready_candidate_audit.json`.
  Animation MILO audit:
  `.codex/current-evidence/midori-review-source-bridges-20260818/current_deployed_milo_payload_audit.json`.
  Model MILO audit:
  `.codex/current-evidence/midori-review-source-bridges-20260818/current_deployed_model_payload_audit.json`.
  Proven: ordinary ark-external loose DLC, manifest references resolve for both
  outfits, model and animation MILOs parse with local `milo_convert_tool`,
  representative native pose proof has 9 zero-failure captures, gameplay proof
  has 2 zero-failure loose-DLC captures with variant selected, Midori performer
  loaded, texture uploaded, lifecycle aliases loaded, and guitar attached.
  Missing final gate: direct user visual approval of the two sheet images above.
  Keep the goal open until that approval is explicit; if rejected, require the
  specific failing cell/screenshot name before changing the candidate.
- 2026-08-18 approval manifest refinement: visual approval now has a single
  machine-readable manifest with all 11 handles:
  `.codex/current-evidence/midori-review-source-bridges-20260818/visual_approval_manifest_pending.json`.
  It contains 9 `native_pose` cells and 2 `gameplay` cells, all with status
  `pending_user_approval`. The central packet is
  `.codex/current-evidence/midori-review-source-bridges-20260818/VISUAL_APPROVAL_PACKET.md`.
  Native cell index:
  `analysis/gh3_midori_pose_review_axisalign_redeploy_proofs/approval_cells/approval_cells.md`.
  Gameplay cell index:
  `analysis/gh3_midori_ghc_gameplay_axisalign_redeploy_proofs/approval_cells/approval_cells.md`.
  Next user-facing action should be either explicit approval of the 11-cell
  packet or a rejection naming one or more labels from the manifest. Do not mark
  the goal complete until approval is explicit.

- 2026-08-18 user rejected the axis-align approval packet: only the middle-right
  pose was considered coherent, and non-bipedal captures are immediate rejects.
  Resumed at pelvis-only matrix-local / `Control_Root` diagnosis without using
  the GH2 ISO at game time. Heavy subprocesses must stay low/Idle priority.
  Fresh GH3/NXTools source bridges were rebuilt for the five representative
  animation cases, with temporary extracted GH3 inputs deleted after export:
  `.codex/current-evidence/midori-review-source-bridges-fresh-targetlength-20260818-5case/review_source_bridge_batch_manifest.json`.
  NXTools background export now uses minimal scene/property shims and renames
  checksum armature bones to resolved Midori names before animation import.
  The generated GLBs are explicitly source-pose bridge artifacts only
  (`glb_summary.status = unskinned_pose_bridge_only`), not final GLB-to-MILO
  proof.

- 2026-08-18 current candidate after animation-set diagnosis:
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-targetlength-bakepos-bind`.
  Two fixes were required: target-skeleton bake `.pos` channels now apply to
  `guitar-ui` as well as `guitar-main`, and targetlength bake translations are
  encoded against the packed solved parent rotation so transition-out reconstructs
  with the same parent rotations as the emitted ACP bytes. Fresh five-case gate:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/fresh_5case_targetlength_packedparent_gate_20260818.json`
  (`avg_pose` `0.000003`, `max_pose` `0.000003`, right/left shin dots `1.0/1.0`).
  Skeleton visual precheck sheets for all five representative clips are in
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_packedparent_skeleton_visuals/`;
  all five were inspected sequentially and are bipedal/coherent at skeleton level.
  `valid-rowaim` was rejected despite numeric pass because its contact sheets
  collapsed/tiny limb segments. Remaining gates: build/stage actual MILO/DLC with
  this targetlength packed-parent path, run disk ACP and direct game-facing visual
  capture, then request explicit user approval. Keep the goal open.

- 2026-08-18 targetlength packed-parent structural rebuild and direct gameplay
  proof completed. Canonical pipeline ran with capture/runtime-proof skipped at
  first, using the fresh five-case bridge manifest, local `GEN`/loose `DLC`, and
  low/Idle-priority subprocesses. Fresh outputs:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pipeline_targetlength_bridge_gate.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pipeline_targetlength_acp_disk_gate.json`.
  Disk ACP bridge gate status was pass: `case_count=5`, max pose error
  `0.000003`, right/left shin dots `1.0/1.0`, max child position magnitude
  `16.879751`. MILOs were built and deployed as ordinary loose DLC under
  `gh2_ps2_hybrid_assets/DLC/community.gh3.midori`; deploy manifest status
  `hybrid_dlc_deploy_verified`, assets `6`, bytes `29267930`.

- 2026-08-18 targetlength packed-parent direct gameplay capture completed with
  `tools/gh3_midori_gameplay_proof.py --capture --low-priority`, proof directory
  `analysis/gh3_midori_ghc_gameplay_targetlength_packedparent_proofs`, output
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_packedparent_gameplay_proof_manifest.json`.
  Status `in_song_midori_variant_animation_verified`, `failure_count=0`,
  `proof_count=2`, app `gh2_ps2_hybrid_assets/ghogx_app.exe`, ark dir
  `gh2_ps2_hybrid_assets/GEN`, addons dir `gh2_ps2_hybrid_assets/DLC`. Fresh
  screenshots:
  `analysis/gh3_midori_ghc_gameplay_targetlength_packedparent_proofs/gh3_midori_1_variant_gameplay_guitarist0_f60.bmp`
  and
  `analysis/gh3_midori_ghc_gameplay_targetlength_packedparent_proofs/gh3_midori_2_variant_gameplay_guitarist0_f60.bmp`.
  Combined approval sheet:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_packedparent_gameplay_contact.png`.
  Codex visually inspected both screenshots and found coherent biped Midori
  gameplay captures with guitar present. Approval packet:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/TARGETLENGTH_PACKEDPARENT_APPROVAL_PACKET.md`.
  Remaining final gate: explicit user visual approval. Keep the goal open.

- 2026-08-18 expanded targetlength packed-parent visual packet: reran the native
  nine-pose review against the current deployed loose DLC using
  `tools/gh3_midori_pose_review.py --capture --low-priority --app
  gh2_ps2_hybrid_assets/ghogx_app.exe --ark-dir gh2_ps2_hybrid_assets/GEN
  --addons-dir gh2_ps2_hybrid_assets/DLC`. Output:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_packedparent_pose_review_manifest.json`.
  Status `native_viewer_representative_pose_framing_review_passed`,
  `failure_count=0`, `proof_count=9`. Combined sheet:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_packedparent_pose_review_contact_9.png`.
  Codex inspected all nine cells sequentially; each has a readable Midori body
  with visible head/torso/limbs. Some are sideways/flying animation poses, but
  none are the prior pelvis-only/non-bipedal collapse. Created current 11-item
  pending approval manifest:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_packedparent_visual_approval_manifest_pending.json`
  with 9 native pose cells plus 2 gameplay cells, all
  `pending_user_approval`. Final gate remains explicit user visual approval.

- 2026-08-18 cleanup note: rebuildable current-turn dirs remained because the
  local command safety policy rejected recursive `Remove-Item` cleanup even with
  fixed literal paths. Retained rebuildable dirs measured before cleanup attempt:
  `analysis/gh3_midori_acp_stage` `38.38 MB`,
  `GuitarHeroOGX-main-ui-engine/tools/milo_convert/out/build/win-amd64-release`
  `55.48 MB`, `GuitarHeroOGX-main-ui-engine/DLC/community.gh3.midori`
  `27.91 MB`, plus smaller model staging/bundle dirs. These are not part of the
  approval claim; the kept deliverable is the deployed loose DLC under
  `gh2_ps2_hybrid_assets/DLC/community.gh3.midori`.

- 2026-08-18 current targetlength visual approval gate formalized. Added
  `tools/gh3_midori_targetlength_visual_approval.py`, which fingerprints only
  the fresh targetlength packed-parent approval artifacts and writes a local
  gallery, decision template, and gate. Generated:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_packedparent_visual_approval_gallery.html`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_packedparent_visual_approval.template.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_packedparent_visual_approval_gate.json`.
  Gate status is `pending_user_visual_approval`, `approval_exists=false`,
  `item_count=11`, `failure_count=0`. Final goal remains open pending explicit
  user acceptance or rejection of this current visual packet.

- 2026-08-18 tightened current-packet rejection before user approval. Added
  `tools/gh3_midori_targetlength_packet_precheck.py`; it reads the current
  11-item approval manifest, re-analyzes each BMP proof, rejects missing,
  blank, tiny, clipped, or grossly collapsed captures before user review, and
  records the GLB/MILO route decision. Output:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_packedparent_visual_packet_precheck.json`.
  Status is `passed_pending_user_visual_approval`, `item_count=11`,
  `failed_item_count=0`, `failure_count=0`. The precheck explicitly states it
  does not replace manual visual approval. It also records that GLB is allowed
  as an automated intermediate, but public `glTFMilo` is not a drop-in GH2 PS2
  final converter; the current final route remains evaluated source pose/GLB
  bridge -> ACP samples -> local repo `milo_convert_tool` GH2 PS2
  `CharClipSet`/`CharClipSamples` writer. Regenerated
  `targetlength_packedparent_visual_approval_gate.json` and
  `targetlength_packedparent_visual_approval.template.json` so any explicit
  acceptance must match the precheck hash too. Gate remains
  `pending_user_visual_approval`, `failure_count=0`.

- 2026-08-18 refreshed GLB/MILO route gating for the current targetlength
  packet. `tools/gh3_midori_glb_milo_route_gate.py` now separates historical
  pelvis/`Control_Root` diagnostic evidence from the current candidate evidence
  and checks the fresh targetlength bridge gate, disk ACP bridge gate, visual
  packet precheck, and targetlength visual approval gate. Generated current
  evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_glb_milo_route_gate.json`
  and refreshed canonical
  `analysis/gh3_midori_glb_milo_route_gate.json`. Both report
  `status=glb_to_milo_route_guarded`, `failure_count=0`,
  `glb_solver_promotable=true`, and
  `current_route_status=targetlength_route_guarded_pending_user_visual_approval`.
  Narrowly updated `tools/gh3_midori_pipeline_verify.py` and
  `tools/gh3_midori_completion_audit.py` to recognize that current route status
  without loosening the final direct visual approval requirement. Goal remains
  open pending explicit user visual acceptance or rejection.

- 2026-08-18 refreshed runtime input guard for current targetlength evidence.
  `tools/gh3_midori_runtime_input_guard.py` now accepts explicit gameplay,
  pose-review, rollout, and hybrid-runtime manifest paths. Ran it against the
  fresh targetlength gameplay and native pose manifests and wrote
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_runtime_input_guard.json`,
  then refreshed canonical `analysis/gh3_midori_runtime_input_guard.json` with
  those same current manifests. Both report
  `status=local_gen_loose_dlc_runtime_inputs_verified`, `failure_count=0`,
  app `gh2_ps2_hybrid_assets\ghogx_app.exe`, ark dir
  `gh2_ps2_hybrid_assets\GEN`, addons dir `gh2_ps2_hybrid_assets\DLC`, and no
  forbidden `.iso`/archive files in the loose Midori DLC package.

- 2026-08-18 updated shared current-proof resolver. `tools/gh3_midori_proof_paths.py`
  now includes the fresh targetlength gameplay and native pose manifests ahead
  of the rejected axis-align manifests. Verified default resolution returns
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_packedparent_gameplay_proof_manifest.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_packedparent_pose_review_manifest.json`.
  Re-ran `tools/gh3_midori_runtime_input_guard.py` with no manifest overrides;
  it wrote
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_runtime_input_guard_default_resolver.json`
  and refreshed canonical `analysis/gh3_midori_runtime_input_guard.json`.
  Status remains `local_gen_loose_dlc_runtime_inputs_verified`,
  `failure_count=0`.

- 2026-08-18 refreshed the canonical human review packet for the fresh
  targetlength candidate. `tools/gh3_midori_review_packet.py` now uses current
  targetlength proof resolver results, the targetlength pose contact sheet,
  visual packet precheck, current GLB/MILO route gate, targetlength bridge and
  disk ACP gates, and the targetlength visual approval gallery/gate/template.
  It no longer requires the rejected axis-align bipedal precheck or contact
  sheet. Generated
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_review_packet.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/TARGETLENGTH_REVIEW_PACKET.md`,
  and refreshed canonical `analysis/gh3_midori_review_packet.json` plus
  `analysis/GH3_MIDORI_REVIEW_PACKET.md`. Both packet runs report
  `status=review_packet_ready`, `failure_count=0`, `proofs=12`. Validation
  commands now point at the current runtime-input guard, GLB/MILO route gate,
  targetlength packet precheck, and targetlength visual approval gate.

- 2026-08-18 refreshed completion audit for the current targetlength packet.
  `tools/gh3_midori_completion_audit.py` now uses the targetlength bridge gate,
  disk ACP gate, visual packet precheck, current targetlength visual approval
  gate/template, and refreshed review packet instead of stale axis-align
  pipeline/precheck/approval artifacts. The targetlength approval JSON is
  optional evidence because it is expected not to exist until explicit user
  acceptance/rejection. Generated
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_completion_audit.json`
  and refreshed canonical `analysis/gh3_midori_completion_audit.json`. Current
  status is `review_ready_pending_user_acceptance`, `proven_count=17`,
  `pending_count=1`, `failed_count=0`; the only pending item is direct user
  visual approval. Re-ran the current validation commands:
  runtime source audit (`no_midori_specific_runtime_hooks`, disallowed `0`),
  runtime input guard (`local_gen_loose_dlc_runtime_inputs_verified`,
  failures `0`), GLB/MILO route gate (`glb_to_milo_route_guarded`, failures
  `0`), rollout manifest (`midori_external_dlc_rollout_ready`, failures `0`,
  files `7`, bytes `29269538`), targetlength packet precheck
  (`passed_pending_user_visual_approval`, failed items `0`), review packet
  (`review_packet_ready`, failures `0`), and completion audit
  (`review_ready_pending_user_acceptance`, failed `0`). After rerunning review
  and completion serially, canonical rollout/review/audit all agree on
  shipping bytes `29269538`.

- 2026-08-18 refreshed rollout manifest proof references for the current
  targetlength packet. `tools/gh3_midori_rollout_manifest.py` now records the
  current targetlength pose-review manifest/contact sheet, visual packet
  precheck, and targetlength visual approval gallery/gate in addition to the
  gameplay resolver output. Generated
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_rollout_manifest.json`
  and refreshed canonical `analysis/gh3_midori_rollout_manifest.json`. Both
  report `status=midori_external_dlc_rollout_ready`, `failure_count=0`,
  `files=7`, `bytes=29269538`, with rollout checks
  `targetlength_pose_review_verified=true`,
  `targetlength_visual_packet_prechecked=true`, and
  `targetlength_visual_approval_gate_pending=true`. Refreshed review packet and
  completion audit after rollout; review remains `review_packet_ready`,
  failures `0`, and completion audit remains
  `review_ready_pending_user_acceptance`, proven `17`, pending `1`, failed `0`.

- 2026-08-18 refreshed aggregate pipeline verification for the current
  targetlength packet. `tools/gh3_midori_pipeline_verify.py` now requires the
  targetlength bridge gate, disk ACP gate, visual packet precheck, and
  targetlength visual approval gate instead of the rejected axis-align
  bipedal-precheck manifest. Generated
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_pipeline_verification.json`
  and refreshed canonical `analysis/gh3_midori_pipeline_verification.json`.
  Current status is `guitar_hero_classic_midori_runtime_verified`,
  `failure_count=0`, `check_count=90`. There is one explicit warning:
  the old `ghogx_character_variant_catalog_test` summary still records
  pre-targetlength package hashes. Attempted to rebuild the single catalog test
  target at low priority, but the existing engine build tree lacked
  `build.ninja` and CMake could not find Ninja, so the executable could not be
  rebuilt in this environment. The verifier therefore treats that catalog hash
  comparison as an optional warning while current package hashes remain enforced
  by the DLC package, hybrid deploy, rollout, runtime input, and gameplay proof
  gates. Refreshed review packet and completion audit after the pipeline
  verifier; review remains `review_packet_ready`, failures `0`, and completion
  remains `review_ready_pending_user_acceptance`, proven `17`, pending `1`,
  failed `0`.

- 2026-08-18 added explicit visual decision writer. New tool
  `tools/gh3_midori_targetlength_visual_decision.py` consumes the current
  targetlength approval template and writes the exact approval/rejection JSON
  expected by `tools/gh3_midori_targetlength_visual_approval.py`. It supports
  `--decision accepted` with all items accepted, or `--decision rejected` with
  one or more `--reject-item ITEM_ID` labels. Dry-ran both paths without
  writing the real approval JSON:
  accepted dry run `items=11`, `rejected=0`; rejected dry run using
  `midori_1_fast_jump_f040`, `items=11`, `rejected=1`. Verified
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_packedparent_visual_approval.json`
  still does not exist. Added accept/reject command examples to the review
  packet validation commands and regenerated canonical/current review and
  completion audit. Review remains `review_packet_ready`, failures `0`;
  completion remains `review_ready_pending_user_acceptance`, proven `17`,
  pending `1`, failed `0`; targetlength approval gate remains
  `pending_user_visual_approval`, failures `0`.

- 2026-08-18 improved the targetlength visual approval gallery handoff.
  `tools/gh3_midori_targetlength_visual_approval.py` now writes a
  `Decision Commands` section into
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_packedparent_visual_approval_gallery.html`,
  with exact accept/reject command templates using
  `tools/gh3_midori_targetlength_visual_decision.py`. Regenerated gallery,
  approval template, and approval gate together so hashes remain consistent.
  Gate remains `pending_user_visual_approval`, `failure_count=0`,
  `approval_exists=false`, and the real
  `targetlength_packedparent_visual_approval.json` still does not exist.
  Regenerated canonical/current review packets and completion audits after the
  gallery hash change; review remains `review_packet_ready`, failures `0`, and
  completion remains `review_ready_pending_user_acceptance`, proven `17`,
  pending `1`, failed `0`.

- 2026-08-19 resumed the pelvis-only matrix-local / `Control_Root` diagnosis
  from the rejected targetlength packet. Added
  `tools/gh3_midori_model_parent_replay_diagnostic.py`, which replays saved ACP
  locals under the deployed Midori model parent map from
  `analysis/gh3_midori_current_midori1_rig.json`. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_model_parent_replay_diagnostic.json`
  proves the current targetlength medium-idle frame 60 locals reproduce the flat
  animation skeleton but diverge when evaluated as the game model will evaluate
  them: `Bone_Pelvis` is flat in the animation report, `bone_pelvis` is parented
  under `Control_Root` in the model, and model-parent replay shifts by
  `120.000034` degrees / `60.073724` units. The same diagnostic proves an
  idealized compensated pelvis local can collapse back to the targetlength world
  reference (`0.061445` degrees / `0.000015` units), so the mismatch is now
  actionable rather than a broad policy sweep.

- 2026-08-19 added a first diagnostic exporter policy,
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-targetlength-modelrootcomp-bakepos-bind`,
  in `tools/gh3_midori_acp_stage.py`. This branch is not promotable yet:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_modelrootcomp_reference_replay_diagnostic.json`
  and the transpose-mode variant both fail. The hook changes the emitted pelvis
  sample but does not yet reproduce the reference targetlength world pose under
  model parentage (`90`-`180` degree replay error depending on `hmx` mode). Do
  not stage this branch visually. Next branch should derive the final
  `bone_pelvis.pos` / `bone_pelvis.quat` channel values directly in the packed
  convention shown by the replay diagnostic, then gate with model-parent replay
  before any MILO/DLC or emulator pass.

- 2026-08-19 advanced the model-parent compensation proof one step closer to
  automation. `tools/gh3_midori_model_parent_replay_diagnostic.py` now supports
  `--compensated-skeleton-output`, producing a skeleton-frame artifact with
  `Bone_Pelvis` local rotation/translation rewritten for the deployed model
  hierarchy while keeping the targetlength world pose as the reference. New
  evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_current_medidle_modelparentcomp_skeleton_frame.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_current_modelparentcomp_validation_diagnostic.json`.
  The validation status is `model_parent_replay_matches_reference`, with max
  model-parent replay error `0.088075` degrees and `0.000019` units. The exact
  frame-60 pelvis local changed from flat
  `[0.086219, -0.57805, 38.193199]` to model-parent local
  `[-38.193199, -0.086219, -0.57805]`; the local rotation becomes
  `[[1, -0.000001, -0.000003], [0.000003, -0.076374, 0.997079], [-0.000002, -0.997079, -0.076374]]`.
  This is the strongest current proof for the next automated step: apply the
  same transform to actual `bone_pelvis.pos` / `bone_pelvis.quat` ACP samples
  before MILO packing, then rerun the model-parent replay gate.

- 2026-08-19 diagnostic hygiene: `tools/gh3_midori_pose_report.py` now treats
  every policy in
  `ROOTWORLD_GLBLOCALRAW_LOCALZ180_THIGHBASIS_PARENTCOMP_TARGETSOLVESKEL_SOURCEPOS_TARGETLENGTH_BAKEPOS_POLICIES`
  as target-length-preserving for mapped-source comparison. This avoids
  misleading pose-report rows for derived targetlength policies.

- 2026-08-19 moved the passing model-parent compensation proof onto a real ACP
  sample stream. Added `tools/gh3_midori_acp_model_parent_compensate.py`, which
  copies a staged ACP directory and rewrites an explicit pelvis channel base
  (`bone_pelvis.mesh` for the retained targetlength attack-left stage) so the
  original flat pelvis world samples reconstruct under an explicit parent
  (`Control_Root`). Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_attack_acp_modelparentcomp_report.json`.
  It patched `145` `bone_pelvis.mesh.pos` samples and `145`
  `bone_pelvis.mesh.quat` samples in
  `analysis/gh3_midori_fresh_attack_targetlength_modelparentcomp_acp_20260819_run1`,
  with built-in replay errors `0` position / `0` rotation. Spot check at frame
  30: `bone_pelvis.mesh.pos` changed from `[-0.000006, -0.000004, 39.405975]`
  to `[-39.405975, 0.000006, -0.000004]`, and `bone_pelvis.mesh.quat` changed
  from `[-0.5, 0.5, 0.5, 0.5]` to `[0, 0, 0, 1]`.
- 2026-08-19 removed the failed direct `targetlength-modelrootcomp` exporter
  policy from `tools/gh3_midori_acp_stage.py` after replacing it with the
  validated post-stage ACP compensator. Do not use the old direct policy path.
  The next useful gate is to feed the compensated ACP stage through the existing
  ACP-to-MILO path in a small diagnostic package, then run the no-ISO visual
  bipedal precheck before any broader animation rollout.

- 2026-08-19 fed the compensated one-clip ACP stream through ihatecompvir's
  existing MILO converter as the requested automated middle bridge. Command
  route:
  `milo_convert_tool build-clipset-from-acp <flat-acp-dir> --name gh3_midori_main --role guitar-main --out <milo> --move-self 0 --control-root-pelvis-parent`.
  Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/modelparentcomp_milo_probe_20260819/modelparentcomp_milo_probe_report.json`.
  The diagnostic MILO is
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/modelparentcomp_milo_probe_20260819/gh3_midori_main_modelparentcomp_probe.milo_ps2`
  (`36,299` bytes, SHA256
  `72F304DC396C5A10B09517F11CD9663064FEA418400EA6CEB8230DB679E91ECB`).
  Inspection proves `clips=1`, fallback group `normal` contains
  `gh3_guit_mido_a_attackl`, `move_self=0`, `Control_Root` exists, and
  `bone_pelvis.mesh parent=Control_Root` with `bone_pelvis.mesh.pos/quat`
  channels preserved. Frame-30 MILO sample matches the compensated ACP values:
  pelvis pos `[-39.406, 0.00000642005, -0.00000352317]`, pelvis quat
  `[-0.000000089407, -0.0000000298023, -0.0000000298023, 1.0]`. Visual status
  remains `not_run`; next gate is a local-GEN/no-ISO bipedal precheck, with
  immediate rejection if the silhouette is non-bipedal.

- 2026-08-19 multi-clip follow-up status: the known-good one-clip
  model-parent path does not automatically generalize yet. A five-clip
  `guitar-main` MILO built from the existing fresh targetlength bridge was
  structurally valid but visually rejected before broad rollout: the captures
  were bipedal-ish rather than old catastrophic non-humanoids, but medium idle,
  attack, jump, and transition were curled/folded rather than playable. A
  hybrid isolation MILO that replaced only attack with the known-good one-clip
  ACP restored the coherent attack silhouette, proving the MILO converter and
  global `CharBone` union are not the cause.
- The current blocker is the source-bridge/staging input, not the pelvis
  `Control_Root` compensator. The passing attack bridge came from pinned
  NXTools commit `6cea808a27d6773bde55e947c9f0ffd72081e164`; current
  `C:\Users\smmel\nxtools` is `eee42e4`. The pinned checkout still exists at
  `C:\Users\smmel\AppData\Local\Temp\nxtools_ref`, and the old extracted GH3
  Midori source tree still exists at
  `C:\Users\smmel\AppData\Local\Temp\gh3_midori_source_visual_20260816_211144\source`.
  Current exporter attempts now suppress texture import for bridge generation
  and only repair checksum-named NXTools bones, but regenerated bridge `pose`
  matrices still do not match the old passing attack artifact. Numeric reject
  sample after staging/compensation remains the bad path:
  `bone_pelvis.mesh.pos [-38.1932, -0.086219, -0.57805]`,
  `bone_pelvis.mesh.quat [0.0270215, 0.706591, 0.706589, -0.0270225]`, instead
  of the known-good one-clip path near `[-39.406, 0, 0]` / identity.
- Practical next step: stop visual testing candidates that fail the pelvis
  numeric gate. Reproduce the old source bridge's evaluated `pose` fields for
  the representative clips, or stage directly from retained known-good bridge
  artifacts where available. Do not mount/use a GH2 ISO at game time; use local
  `gh2_ps2_hybrid_assets/GEN`, keep heavy processes low priority, and reject
  any non-bipedal contact-sheet frame immediately.

- 2026-08-19 follow-up: restored the old passing source bridge behavior
  automatically. Added `--force-partial-anims` to
  `tools/gh3_midori_source_visual.py`,
  `tools/gh3_midori_source_bridge_export.py`, and
  `tools/gh3_midori_review_source_bridge_batch.py`. This reads the GH3 partial
  animation block for stream alignment, then marks all bones allowed before
  NXTools builds Blender curves. With pinned NXTools
  `C:\Users\smmel\AppData\Local\Temp\nxtools_ref` at
  `6cea808a27d6773bde55e947c9f0ffd72081e164`, the regenerated attack bridge
  exactly matches the old passing artifact for `pose`, `basis`, and
  `matrix_local` (`max_abs_delta=0`).
- The current ACP staging drift was then traced to the default stock GH2 body
  bind override, not the source bridge. Staging the exact old bridge through
  current `tools/gh3_midori_acp_stage.py` with the default
  `analysis/gh3_midori_stock_glam1_rig.json` produces the bad pelvis sample
  `[-38.1932, -0.086219, -0.57805]`. Passing a nonexistent `--gh2-stock-rig`
  disables that stock bind override and restores the known-good compensated
  attack sample `[-39.406, 0.00000642005, -0.00000352317]` with identity quat.
- Built a matched five-clip diagnostic pair: main animation MILO
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/modelparentcomp_pinned_forcepartial_nostock_5case_milo_probe_20260819/probe.milo_ps2`
  (`380,333` bytes, SHA256
  `2D0B4C96478F923A228AACB6A2097A2F72FA8BCB3C657A08F83808FA33AA75F5`) and a
  no-stock `--control-root-pelvis-parent` model probe
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/model_controlroot_nostock_probe_20260819/gh3_midori_1_controlroot_nostock_probe.milo_ps2`
  (`283,646` bytes, SHA256
  `F08B6A02D5C172517FA5BD737C481A5DBA9EE5221FD7193CBC78727089A34F50`).
  `inspect-character --transforms` confirms the probe model has
  `bone_pelvis.mesh parent=Control_Root`.
- Direct native viewer precheck with the matched model+main pair used local
  `gh2_ps2_hybrid_assets/GEN`, `GHOGX_ADDONS_DIR=gh2_ps2_hybrid_assets/DLC`,
  no ISO, and `ghogx_app` at `Idle` priority. The deployed model and main were
  restored afterward to SHA256
  `D0927316AB57C1CCD3DC0A564C03FCD52F244F4C042394B2BDA03B34EB2C8A7A` and
  `F1A06A0E9507023D7F631598693D9F43C47C00A2982068737EDE67CF7452F598`.
  Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/modelparentcomp_pinned_forcepartial_nostock_5case_visual_20260819_fast/visual_decision.json`;
  contact sheet `matched_controlroot_contact_sheet.jpg`. Codex inspected all
  five captures sequentially. Status:
  `pass_matched_controlroot_model_and_main_representative_bipedal_gate`. This
  is **not** final DLC approval: it covers one outfit and five representative
  main clips, and arms/guitar/controller tiers still need follow-up before full
  rollout.

- 2026-08-19 completed the first no-ISO direct-viewer bipedal visual precheck
  for the model-parent-compensated one-clip MILO. Temporarily swapped only
  `gh3_midori_main.milo_ps2` in the loose DLC, launched
  `gh2_ps2_hybrid_assets/ghogx_app.exe` against local
  `gh2_ps2_hybrid_assets/GEN`, kept `ghogx_app` at `Idle` priority, captured
  `gh3_guit_mido_a_attackl` frames `15/30/45`, then restored the deployed main
  MILO to canonical SHA256
  `F1A06A0E9507023D7F631598693D9F43C47C00A2982068737EDE67CF7452F598`.
  Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/modelparentcomp_visual_probe_20260819_fast/`.
  Direct visual inspection and contact sheet
  `modelparentcomp_attack_contact_sheet.jpg` pass the immediate bipedal reject
  gate: all three frames are coherent bipedal side-fall poses, not the prior
  non-humanoid/vertical-leg failures. This is **not** final approval: it covers
  one attack-left clip in direct viewer only. Next step is to apply the same ACP
  post-stage compensation to a representative multi-clip main set, then run the
  sequential bipedal precheck before in-song/local-GEN validation.

- Latest pause checkpoint: resume from the matched Control_Root/no-stock
  representative pass, not the older one-clip-only point. This is now targeted
  rollout/iteration work. The GLB/source bridge is reproducible automatically
  via pinned NXTools plus `--force-partial-anims`; the folding/non-bipedal
  failure was traced to the stock body bind override colliding with the
  Control_Root/pelvis-local path. The matched diagnostic model+main pair proves
  coherent bipedal native-viewer captures for five representative guitar-main
  clips using local extracted assets only, no ISO, with `ghogx_app` at `Idle`
  priority. Remaining work is full-pipeline integration plus arms/guitar and
  controller/full-set visual coverage before any final DLC approval.

- 2026-08-19 continuation: promoted the proven diagnostic assumptions into
  first-class pipeline switches instead of relying on a fake missing stock-rig
  path. `tools/gh3_midori_build_pipeline.py` now supports
  `--no-gh2-animation-rig`, `--control-root-pelvis-parent`, and
  `--model-parent-compensate-acp`; it stages raw ACP to
  `analysis/gh3_midori_acp_stage_raw`, compensates into
  `analysis/gh3_midori_acp_stage`, forwards no-stock/Control_Root to the bridge
  gates, passes `--control-root-pelvis-parent --move-self 0` to
  `milo_convert_tool build-clipset-from-acp`, and passes the Control_Root model
  parent into model bundling. The staging/report gates now have explicit
  `--no-gh2-stock-rig` options. Syntax checks passed, and targeted pipeline
  tests for the production retarget contract plus pelvis-correction policy map
  pass. No full rebuild or visual approval was run in this continuation.

- 2026-08-19 structural full-set pipeline run: corrected the automation order
  so raw ACP staging remains no-stock but not Control_Root-parented when
  `--model-parent-compensate-acp` is enabled; the Control_Root parent is applied
  at model bundle / MILO clipset output after compensation, matching the
  previous passing probe. Ran the canonical pipeline at `Idle` process priority
  with `--skip-bridge-gate --no-gh2-animation-rig --control-root-pelvis-parent
  --model-parent-compensate-acp --skip-runtime-proof --skip-runtime-deploy
  --skip-dlc-package --skip-hybrid-dlc-deploy --skip-anim-runtime-sanity`.
  It built analysis-only candidate MILOs: 331 clips total across 4 animation
  packages, including 266 guitar-main clips, plus 2 outfit models.
  `analysis/gh3_midori_gh2_milos/gh3_midori_main.milo_ps2` SHA256
  `003B0C61512388F9F6DBFB02099EECF938360A67DE3EE51B3A7A572680637981`
  (`28,310,744` bytes); `analysis/gh3_midori_gh2_models/gh3_midori_1.milo_ps2`
  SHA256 `F08B6A02D5C172517FA5BD737C481A5DBA9EE5221FD7193CBC78727089A34F50`
  (`283,646` bytes). The generated full-set attack frame-30
  `bone_pelvis.mesh` sample exactly matches the retained passing five-case
  probe: pos `[-39.406, 0.00000642005, -0.00000352317]`, quat
  `[-0.000000089407, -0.0000000298023, -0.0000000298023, 1]`. Compact proof:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pipeline_controlroot_nostock_structural_orderfix_20260819.json`.
  The run did not launch GHC/native viewer, did not package/deploy DLC, and
  skipped GH3 ISO SHA hashing. Loose deployed DLC hashes remained unchanged.
  Scratch ACP/model-bundle/log outputs from this turn were cleaned; candidate
  analysis MILOs and compact manifests remain for the next visual candidate.

- 2026-08-19 full-set visual follow-up: the no-stock full-set candidate passed
  the bipedal gate but failed final visual due upper-body/guitar/hand defects,
  especially hand/finger issues. A scoped stock compromise using
  `--stock-bind-scope upper-limbs-guitar` preserved the known-good
  Control_Root pelvis sample while improving hand stability. Native-viewer
  proof:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_full_pose_review_20260819/pose_review_proof_manifest.json`
  status `native_viewer_representative_pose_framing_review_passed`, 9 proofs,
  0 automated failures. Sequential inspection passed biped/framing but still
  rejected final approval because hands/guitar are offset from performer
  contact. This is the current best branch, not a shippable final.

- 2026-08-19 suppress-guitar diagnostic: exposed and tested
  `--suppress-stock-guitar-main-anchor` on the scoped stock branch. Structural
  sample kept the exact passing pelvis (`bone_pelvis.mesh` attack-left frame
  30 pos `[-39.406, 0.00000642005, -0.00000352317]`, identity quat) and changed
  `bone_pos_guitar.mesh` away from the previous constant stock channel. Native
  viewer proof:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_suppressguitar_full_pose_review_20260819/pose_review_proof_manifest.json`
  passed automated framing (9 proofs, 0 failures, min margin 27), but sequential
  visual inspection of all 9 frames rejected the branch: the body is coherent
  bipedal, while the guitar crosses through/behind the torso and hands remain
  detached from fret/strum contact. Decision:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_suppressguitar_full_pose_review_20260819/visual_decision.json`.
  Resume from guitar/hand binding diagnosis or GLB-to-MILO automation, not from
  pelvis/Control_Root diagnosis.

- 2026-08-19 preserve-local guitar attach diagnostic: added explicit
  `--preserve-guitar-attach-local` passthrough to the model bundle/pipeline so
  scoped stock upper-limb/guitar binds can keep Midori's generated
  `bone_pos_guitar(.mesh)` local attach instead of replacing it with the stock
  guitar world. The analysis-only branch built structurally and preserved the
  known-good pelvis sample, but native-viewer visual capture rejected it:
  accessory frame clipped the guitar above the head, and all inspected frames
  kept hands detached from fret/strum contact. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_preservelocal_full_pose_review_20260819/visual_decision.json`.
  Current best remains the scoped stock upper-limbs/guitar baseline; next work
  should solve the combined main-guitar animation anchor versus independent
  fret/strum IK-target contract, not flip solely between stock-world and
  stock-local model attach.

- 2026-08-19 helper-compensation diagnostic: added explicit
  `--compensate-guitar-helper-for-main-anchor` plus matching
  `--main-guitar-pos-offset` passthrough to the model bundle/pipeline. Built a
  model-only candidate reusing the current-best stockupper animation MILOs.
  Candidate evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_helpercomp_full_candidate_20260819/`.
  Visual proof from local loose DLC/no ISO/low priority passed automated
  framing (9 proofs, 0 failures) but failed sequential final inspection:
  guitar remains behind/right of the body and hands remain detached from
  fret/strum contact across both outfits. Decision:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_helpercomp_full_pose_review_20260819/visual_decision.json`.
  Resume at the shared contract between main `bone_pos_guitar.mesh` animation
  and independent `bone_fret_hand.mesh` / `bone_strum_hand.mesh` IK target
  clipsets; do not spend the next branch only on GH3 helper local changes.

- 2026-08-19 guitar/hand IK contract diagnosis: added
  `tools/gh3_midori_guitar_ik_contract_report.py` and measured the current-best
  stockupper candidate with viewer xplorer prop overrides. The visible hand
  positions and target spaces are far apart: representative distances include
  LH-to-fret `7.826..37.630`, RH-to-strum `25.889..34.868`, and single-anchor
  solve deltas `77.645..147.164`, so one static guitar anchor cannot satisfy
  the current visible hands. Engine inspection shows both gameplay and the
  standalone viewer call `apply_character_pose_controller_frame`, but the
  rejected proof logs did not show deep IK/controller audit lines unless
  `GHOGX_AUDIT_CHARACTER_GRAPH` / `GHOGX_DEBUG_IK` are used.

- 2026-08-19 explicit MIDI fret-target sweep: rebuilt only `ghogx_app` at Idle
  priority using local Ninja plus VS `vcvars64`, temporarily swapped in the
  retained current-best candidate, captured hand-overlay frame 10 with no ISO,
  then restored loose DLC hashes to
  `D0927316AB57C1CCD3DC0A564C03FCD52F244F4C042394B2BDA03B34EB2C8A7A`,
  `ED36FA5AE11399EC4BDA7EE242CCDF7E10EBDF84CF5B8F956F7281087B928F3C`, and
  `F1A06A0E9507023D7F631598693D9F43C47C00A2982068737EDE67CF7452F598`.
  Captures for `none`, `spot_neck_fret03.mesh`, `spot_neck_fret07.mesh`,
  `spot_neck_fret11.mesh`, and `spot_neck_fret15.mesh` were visually
  equivalent and rejected: bipedal body, but guitar remains beside/behind the
  performer and visible hands remain detached. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_miditarget_sweep_20260819/visual_decision.json`.
  Next branch should instrument/apply the real IK target publication or bake a
  diagnostic visible-hand solve; do not keep sweeping fret target names alone.

- 2026-08-19 deep IK trace and left-reach offset branch: ran a single
  hand-overlay capture with `GHOGX_AUDIT_CHARACTER_GRAPH`, `GHOGX_DEBUG_IK`,
  and `GHOGX_DEBUG_ARM_CONTRACT`. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_deep_ik_trace_20260819/`.
  This proves `CharIKHand` is active in the viewer: `right_hand.ik` solves to
  `bone_strum_hand.mesh` with pre-final error `0.0362`, while `left_hand.ik`
  targets `bone_fret_hand.mesh` at about `33.75..39.06` units from the left
  shoulder against about `19.15` units of reach before stretch/final write.
  Updated `tools/gh3_midori_guitar_ik_contract_report.py` to report shoulder
  reach, target overreach, and suggested whole-guitar offsets. The new report:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_ik_contract_report_reach_20260819.json`.
  Its left-fret reach average suggested `main_guitar_pos_offset=6.0,-2.84,2.91`.
  Built that real file branch without ISO/runtime deploy:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_leftreach_offset_candidate_20260819/`.
  Visual proof failed automated framing on `midori_1_accessory_acc01_f030` and
  direct visual inspection rejected the branch: guitar/hand coherence is worse
  overall, with the guitar still beside/behind the performer and hand-overlay
  tangled. Decision:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_leftreach_offset_pose_review_20260819/visual_decision.json`.
  Next branch should alter target publication/rotation or bake a diagnostic
  visible-hand/guitar solve, not only translate `bone_pos_guitar.mesh`.

- 2026-08-19 target-publication probes: tested two narrower hypotheses on the
  current-best stockupper candidate, no ISO, low priority, with loose DLC
  restored after each run. First, captured hand-overlay frames 2, 8, and 14 with
  explicit `--midi-fret-target spot_neck_fret11.mesh`. `CharIKMidi` reached
  full fraction/weight by frame 8, but the visual stayed effectively unchanged
  and rejected. Decision:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_midi_timing_probe_20260819/visual_decision.json`.
  Second, ran the same hand-overlay with `GHOGX_DISABLE_HAND_OUTPUT_LAYER=1`.
  This proves the fret clip output layer was overwriting the prop-local
  `bone_fret_hand.mesh` target (`[-5.32,1.65,1.05]` versus prop local
  `[-6.27,-0.45,-4.32]`), but the resulting prop-local target was still
  visually rejected and numerically unreachable. Decision:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_disable_hand_output_probe_20260819/visual_decision.json`.
  Continue with the broader guitar/target orientation-publication contract or a
  diagnostic visible-hand/guitar solve; timing and simple hand-output disable
  are ruled out.

- 2026-08-19 hand-target-root world publication probe: added a temporary
  diagnostic branch in the build-target viewer that published only
  `bone_fret_hand` / `bone_strum_hand` runtime output worlds through
  `current_chain * inverse(bind_chain) * stored_world`, leaving finger rows
  hand-local. Captured the current-best hand-overlay through local
  `gh2_ps2_hybrid_assets/GEN` plus loose DLC, no ISO, low priority, and restored
  the loose DLC hashes afterward. The no-bridge control and bridge capture are
  byte-identical SHA256
  `0413779D974CB4B0C2545FA198F4D34DC1B46FC0BDBD87F7CDA0D8AF63A29FB0`;
  visually they remain the same rejected pose with the guitar/hand mass beside
  the performer. Decision:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_handtarget_root_world_probe_20260819/visual_decision.json`.
  The temporary code was removed and `ghogx_app` rebuilt back to source parity.
  Next branch should change the authored guitar/target relationship or bake a
  diagnostic visible-hand/guitar solve, not runtime root-publication-only
  variants.

- 2026-08-19 authored hand-root prop-local branch: added automated staging
  switch `--hand-root-position-source {reference,prop-local}` to
  `tools/gh3_midori_acp_stage.py` and pipeline pass-through in
  `tools/gh3_midori_build_pipeline.py`. The diagnostic keeps the current-best
  stockupper body/main/model files and rebuilds only fret/strum hand packages
  with `bone_fret_hand` / `bone_strum_hand` position rows sourced from the
  xplorer prop-local targets instead of the GH2 hand-reference proxy. No ISO;
  local `GEN` + loose DLC; viewer low priority; loose DLC restored afterward.
  Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_handroot_proplocal_pose_review_20260819/visual_decision.json`.
  Result is rejected: screenshot changed versus stockupper, proving ACP-level
  hand root rows affect the visible solve, but the guitar remains beside/behind
  the performer and the strum/fret contact is still incoherent. Do not promote
  raw prop-local hand roots. Next branch should solve the coupled guitar-body
  plus fret/strum target relationship or bake a diagnostic visible-hand/guitar
  pose, not move hand-root rows alone.

- 2026-08-19 main-guitar rotation correction probe: added automated staging
  switch `--main-guitar-rotation-correction
  {none,x180-pre,y180-pre,z180-pre,x180-post,y180-post,z180-post}` to
  `tools/gh3_midori_acp_stage.py` and pipeline pass-through in
  `tools/gh3_midori_build_pipeline.py`. Built a single `z180-post` main-MILO
  branch from the current-best stockupper candidate. Candidate main SHA256 was
  `F3A804C40576D7DF88D64C4FA4555ED96BEA9985736C2C9F2CE84C3233BEC2F5`. Captured
  one no-ISO, low-priority hand-overlay pose through local `GEN` and temporary
  loose DLC swap, then restored the canonical loose hashes. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_mainrot_z180post_pose_review_20260819/visual_decision.json`.
  Result is rejected: direct visual inspection shows the branch is visibly
  worse, with unusable body/prop framing and no improvement in guitar/hand
  contact. Do not promote `z180-post`. The correction switch can still be used
  for bounded future orientation probes if needed.

- 2026-08-19 source GLB guitar/hand helper diagnostic: refreshed the local
  ihatecompvir bridge audit and confirmed `glTFMilo`/MiloLib are present, with
  MiloLib useful as GH2 PS2 reference but public `glTFMilo` not a drop-in final
  PS2 converter. Regenerated a richer Blender/NXTools source bridge for
  `gh3_guit_mido_a_attackl` with torso, arms, palms, `bone_guitar_body`, and
  both GH3 guitar IK hand helpers:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/source_bridge_attack_arm_guitar_20260819/`.
  Added `tools/gh3_midori_source_guitar_contract_report.py`; it reports the
  stock GH2 fret/strum target locals are about `7.73` and `7.46` units away
  from the GH3 source IK helper locals in that source guitar-body frame. Added
  automated staging mode `--hand-root-position-source source-ik-helper` plus
  `--source-guitar-contract-report`, including build-pipeline pass-through.
  Built a full source-helper fret/strum branch and captured one no-ISO,
  low-priority hand-overlay pose through local `GEN`, then restored loose DLC.
  Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_sourceik_helper_pose_review_20260819/visual_decision.json`.
  Result is rejected: the pose remains bipedal, but guitar is still behind/beside
  the performer and hands are detached. Do not promote raw unscaled GH3 source
  IK helper locals as static hand target roots. Next branch should use the GLB
  source report as proof of target-space mismatch and solve guitar body/target
  orientation plus scale together.

- 2026-08-19 source IK helper GH2-scale overlay probe: extended the automated
  source-helper staging mode with `--hand-root-position-source
  source-ik-helper-gh2scale`. This maps source helper locals through the
  established Midori basis and `GH3_PS2_SKELETON_TO_GH2_SCALE` before writing
  fret/strum hand target position rows. Built only the two hand-overlay clips
  (`gh3_hnd_guit_strum_mido_norm_m01_d` and
  `gh3_hnd_guit_chord_mid_bar3_d`) into minimal diagnostic hand MILOs, captured
  one no-ISO, low-priority hand-overlay pose through local `GEN`, then restored
  loose DLC. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_sourceik_gh2scale_overlay_pose_review_20260819/visual_decision.json`.
  Result is rejected: the body remains bipedal, but the mapped targets only
  shift the detached hand/guitar mass; the guitar is still behind/right of the
  performer and fret/strum contact remains incoherent. Do not promote raw or
  GH2-scale-mapped GH3 source IK helper locals as static hand target roots.
  The next branch must move the visible guitar body and hand targets together
  in one shared target frame, or bake a diagnostic visible hand/guitar pose.

- 2026-08-19 coupled-center main-anchor overlay probe: used
  `tools/gh3_midori_guitar_ik_contract_report.py` on the current-best
  stockupper candidate and derived the hand-overlay
  target-center-to-hand-center delta `[15.012511, -12.346049, 15.937957]`.
  Built a minimal one-clip main MILO with this value as
  `--main-guitar-pos-offset`, reusing current-best full fret/strum/model/UI
  files, then captured one no-ISO, low-priority hand-overlay pose through local
  `GEN` and restored loose DLC. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_coupled_center_overlay_pose_review_20260819/visual_decision.json`.
  Result is a hard rejection: the large coupled translation moves the
  guitar/target cluster together but destroys the performer into a sideways
  non-bipedal silhouette. Do not generalize overlay center-delta main-anchor
  offsets. A future branch must solve orientation/target frame without
  corrupting the body, or explicitly bake a visible hand/guitar pose while
  preserving the accepted body solve.

- 2026-08-19 visible-hand bake overlay probe: added
  `tools/gh3_midori_visible_hand_bake_probe.py`. The probe reconstructs the
  current-best stockupper overlay world pose, solves only
  `bone_L-hand.mesh` / `bone_R-hand.mesh` local positions so their visible hand
  worlds land on the existing `bone_fret_hand.mesh` / `bone_strum_hand.mesh`
  targets, and emits minimal diagnostic ACPs for the two hand-overlay clips.
  Converted those ACPs to minimal fret/strum MILOs, reused the current-best
  body/main/model/UI files, and captured one no-ISO, low-priority hand-overlay
  pose through local `GEN`; loose DLC was restored to canonical hashes
  afterward. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_visible_hand_bake_overlay_pose_review_20260819/visual_decision.json`.
  Result is rejected but useful: the body remains upright/bipedal and visible
  hand rows can be baked safely, but the guitar still sits behind/through the
  torso and the hands/arms are detached/stacked. Do not promote hand-only
  visible bake overlays. Next branch should move or reorient the visible guitar
  frame together with the hand solve while preserving the accepted body solve.

- 2026-08-19 minimal visible-guitar frame plus hand-bake probes: added
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`. It emits minimal ACPs for
  `bone_pos_guitar.pos`/optional `.quat` in the main overlay plus visible
  `bone_L-hand.pos` and `bone_R-hand.pos` in fret/strum overlays, then solves
  all three through local `GEN` + loose DLC only with low-priority captures.
  Three one-pose branches were captured and all were visually rejected:
  translate-only moves the guitar beside the body but leaves it edge-on;
  `y-90-post` reveals the Xplorer face but fails because it translated before
  rotation; corrected `y-90-post` rotation-first preserves the bipedal body and
  visible guitar face but leaves the frame too low/right with hands piled near
  the legs. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_overlay_pose_review_20260819/visual_decision.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_y90post_pose_review_20260819/visual_decision.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_y90post_rotfirst_pose_review_20260819/visual_decision.json`.
  Next branch should keep this isolated prop/hand overlay path, but derive the
  guitar placement from source/body-relative GLB bridge rows or a stock GH2
  guitarist pose rather than from the current broken overlay hand center.

- 2026-08-19 source/body-relative visible-guitar placement probes: extended
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py` with
  `--placement-mode source-bridge-pelvis-delta`, `--source-frame`,
  `--source-basis {direct,anim,helper}`, and comma-separated composite rotation
  corrections. Captured three local-GEN, low-priority, no-ISO one-pose branches:
  `srcdirect_y90post`, `srcanim_y90post`, and `srcanim_y90x180`. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcdirect_y90post_pose_review_20260819/visual_decision.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_y90post_pose_review_20260819/visual_decision.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_y90x180_pose_review_20260819/visual_decision.json`.
  Results are all rejected. `direct` is better centered but still behind/low;
  `anim` is the better placement clue because it lifts the guitar into the
  torso/waist band, but orientation/hand contact remain wrong; `y90+x180` is
  worse and becomes a sideways slab. Next branch should stop blind axis flips
  and derive the actual visible guitar-frame rotation basis from source GLB
  matrices or a stock GH2 guitarist pose, while keeping the minimal prop/hand
  overlay path and not revisiting pelvis/Control_Root.

- 2026-08-19 source-relative visible-guitar rotation probes: extended
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py` with
  `--rotation-mode source-bridge-pelvis-relative` and
  `--rotation-source-basis {direct,anim,helper}`. Captured local-GEN,
  low-priority, no-ISO one-pose branches for source-anim placement plus
  source-relative rotation basis `anim` and `direct`. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_rotanim_pose_review_20260819/visual_decision.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_rotdirect_pose_review_20260819/visual_decision.json`.
  Both are rejected: the accepted bipedal body and source-anim placement remain,
  but the guitar becomes vertical/edge-on beside the torso with incoherent hand
  contact. Next branch should compare against a stock/runtime GH2
  `bone_pos_guitar` local basis and the Xplorer prop authored frame rather than
  continuing source-relative matrix convention guesses.

- 2026-08-19 stock runtime attach-world guitar-frame probe: found and fixed a
  diagnostic bug in `tools/gh3_midori_guitar_frame_hand_bake_probe.py`: non-
  default `--rotation-mode` values computed a rotation but did not emit
  `bone_pos_guitar.quat` unless `--rotation-correction` was also non-`none`.
  Earlier source-relative/stock-local rotation-mode captures should therefore
  be treated primarily as placement evidence, not true emitted-rotation
  evidence. Captured a stock glam1 + Xplorer runtime prop debug run through
  local packed `gh2_ps2_hybrid_assets/GEN` with `GHOGX_DEBUG_PROP=1`; evidence
  is retained at
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/stock_glam1_xplorer_prop_debug_20260819/stock_glam1_prop_debug.log`.
  The important stock runtime attach-world row starts near
  `r0=(0.420651 0.862351 0.281787)`, `r1=(-0.445973 0.467040 -0.763533)`,
  `r2=(-0.790039 0.195511 0.581046)`, `pos=(7.492639 7.913158 34.870792)`.
  Added `--rotation-mode stock-prop-debug-attach-world` and
  `--placement-mode stock-prop-debug-attach-world`. Captured two no-ISO,
  low-priority branches:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_stockattach_emitrot_pose_review_20260819/visual_decision.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_stockattach_frame_pose_review_20260819/visual_decision.json`.
  Both are rejected but useful: real stock attach-world rotation gives the
  Xplorer a normal diagonal orientation and preserves the bipedal body, but the
  frame is still behind/right/low and hand contact is incoherent. Next branch
  should keep this stock runtime attach-world rotation and solve
  Midori/body-relative placement plus hand targets inside that frame.

- 2026-08-19 stock attach-world rotation with hand-center placement probes:
  after the rotation-emission fix, captured two no-ISO, low-priority branches
  that keep the stock runtime attach-world rotation and translate the guitar
  frame toward the current visible hand center: full hand-center and 50% blend.
  Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_handcenter_stockattach_pose_review_20260819/visual_decision.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_handcenter50_stockattach_pose_review_20260819/visual_decision.json`.
  Both are rejected. Full hand-center over-corrects the instrument and hands
  downward into the leg/foot area; 50% blend is less extreme but still
  incoherent. This exhausts current visible-hand center as a placement target.
  Next branch should keep stock attach-world rotation but derive placement from
  source/stock fret/strum contact geometry or shoulder/arm reach, not from the
  already-broken hand center.

- 2026-08-19 reach/source-local hand target probes: extended
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py` with
  `--placement-mode arm-reach-offset`, `--reach-scale`, and
  `--hand-target-mode {current-proxies,source-palm-locals,source-ik-helper-locals}`.
  Captured four no-ISO, low-priority single-pose branches through local `GEN`
  plus loose DLC:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_reach_stockattach_pose_review_20260819/visual_decision.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_reach50_stockattach_pose_review_20260819/visual_decision.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_stockattach_ikhands_pose_review_20260819/visual_decision.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_stockattach_palmhands_pose_review_20260819/visual_decision.json`.
  All four are direct visual rejects. Arm-reach whole-frame offsets do not fix
  the contract; source palm/IK-helper locals mapped into the source-anim
  placement plus stock attach-world rotation still put the visible hands in
  shin/foot space. A parallel capture attempt briefly left loose anim files on
  diagnostic hashes; the loose DLC was restored from the canonical source
  package under `GuitarHeroOGX-main-ui-engine/DLC/community.gh3.midori` and
  verified. Future loose-MILO swap captures must be sequential. Next branch
  should solve the visible prop anchor/mesh compensation explicitly from the
  stock runtime prop debug rows (`attach-world`, `prop-anchor-world`,
  `prop-to-attach`, `prop-rel`/`prop-mesh-world`) rather than continuing
  whole-frame translation or direct source-local hand target guesses.

- 2026-08-19 prop-to-char compensation and overlay channel probes: extended
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py` with runtime prop
  compensation helpers. Verified that
  `prop_to_char = inverse(prop-anchor-world) * attach-world`, and that applying
  logged prop-local `comp` vectors through that matrix reproduces the stock
  `char_comp` rows. Added stock prop-compensated target modes plus
  `--overlay-channel-mode {visible-hands,visible-hands-main-delta,proxy-targets}`.
  Captured no-ISO, low-priority single-pose proofs:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_stockattach_propcomp_hands_pose_review_20260819/visual_decision.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_stockattach_propcomp_strings_pose_review_20260819/visual_decision.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_stockattach_propcomp_strings_proxy_pose_review_20260819/visual_decision.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_stockattach_propcomp_strings_handdelta_pose_review_20260819/visual_decision.json`.
  All are visual rejects, but they isolate the failure: the prop-compensated
  contact targets are now in the torso/guitar band and the diagnostic MILOs are
  loaded with different layer signatures, yet visible hands still render near
  shin/foot space. Proxy target channels alone also do not move the visible
  hands in the native viewer. Next branch should keep the prop-to-char contact
  math but bake or solve the full arm chain (`upperArm`, `foreArm`, `hand`) for
  the hand-overlay case, approximating the missing runtime IK instead of
  animating endpoint/proxy positions alone.

- 2026-08-19 minimal arm-chain bake rejection: extended
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py` with
  `--overlay-channel-mode visible-arm-chain` and `visible-arm-chain-rotpos`.
  These modes solve a two-bone reachable elbow/wrist chain from the clean main
  pose toward the prop-to-char guitar-strings contact target. The rot+pos mode
  emits `upperArm.quat`, `foreArm.quat`, `foreArm.pos`, and `hand.pos` for each
  side. Captured no-ISO, low-priority proofs:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_stockattach_propcomp_strings_armchain_pose_review_20260819/visual_decision.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/armchain_rotpos_pose_review_20260819/visual_decision.json`.
  Both are visual rejects. The native viewer logs the extra arm-chain channels
  as loaded (`ch=2` for pos-only, `ch=4` for rot+pos), but the rendered hands
  still hang near shin/foot space and do not contact the guitar. This rejects a
  minimal IK payload as sufficient. Next branch should keep the prop-to-char
  target math, but derive a fuller fret/strum hand-layer bake from the stock
  hand reference or source hand clips, including the expected native hand,
  finger, and arm channels. Operational note: the first rot+pos proof path was
  too long for Windows `CopyFile2`, so retained proof uses the shorter
  `armchain_rotpos_pose_review_20260819` directory.

- 2026-08-19 native full hand-bank comparison: discovered that canonical
  staged hand clips use `.mesh` channel names (`bone_fret_hand.mesh.pos`,
  finger `.mesh.quat`, etc.), while earlier tiny diagnostics emitted bare names.
  Added `--channel-name-mode {bare,mesh}` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py` and captured
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/mesh_armchain_rotpos_pose_review_20260819/visual_decision.json`.
  Mesh-channel minimal arm-chain still rejected, so namespace alone is not the
  fix. Then captured three full-native-hand comparisons:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/meshmain_fullhands_pose_review_20260819/visual_decision.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/canonical_fullhands_baseline_pose_review_20260819/visual_decision.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/bodymain_fullhands_pose_review_20260819/visual_decision.json`.
  Results: canonical full hands are necessary and visibly engage. Canonical main
  plus full hands collapses sideways; accepted full body main plus full hands
  also collapses/off-frames in this loose-model overlay path. The useful family
  is generated narrow mesh-channel guitar-main plus canonical full fret/strum
  banks: it keeps Midori upright and gives the first visibly plausible hand
  engagement, but the guitar is still too low/through the legs and the right
  hand remains near the foot. Next branch should keep canonical full hand banks
  and refine only the generated narrow guitar-main frame/placement around that
  hand layer.

- 2026-08-19 hand target parent-space diagnosis: added
  `tools/gh3_midori_hand_target_space_report.py` and extended
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py` with
  `--overlay-channel-mode canonical-fret-target-pelvis-rebase`. The report
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/meshmain_fullhands_target_space_report_20260819.json`
  shows canonical `bone_fret_hand.mesh.pos` fits `bone_pelvis.mesh` space
  better than its declared `bone_fret.mesh` parent (`40.978` down to `15.074`),
  while strum remains best in declared `bone_strum.mesh` space (`13.162`).
  The compiled pelvis-rebase branch
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/fret_pelvis_rebase_contract_report_20260819.json`
  is a structural reject: `LH-FRET=24.589`, `RH-STRUM=13.162`,
  `SOLVE-DELTA=137.812`. No visual capture was taken because this is weaker
  than the already captured/rejected left-arm rotation branch (`LH-FRET=10.782`).
  Decision record:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/hand_target_space_decision_20260819.json`.
  Next branch should keep the generated narrow mesh-channel guitar-main plus
  canonical full fret/strum hand banks, and refine the generated guitar-main
  frame/placement around that full native hand layer. Reject non-bipedal or
  guitar-through-legs captures immediately.

- 2026-08-19 guitar-frame pair-fit checkpoint: added
  `--case-name` to `tools/gh3_midori_pose_review.py` for one-case diagnostic
  captures, and added `hand-target-pair-fit` rotation plus
  `hand-target-pair-midpoint` placement to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/guitar_frame_pairfit_decision_20260819.json`,
  `fullhands_midfit_contract_report_20260819.json`,
  `fullhands_midfit_pose_review_20260819/visual_decision.json`,
  `fullhands_sourcerot_contract_report_20260819.json`,
  `fullhands_pairfit_contract_report_20260819.json`,
  `fullhands_pairfit_directquat_contract_report_20260819.json`, and
  `fullhands_pairfit_directquat_pose_review_20260819/visual_decision.json`.
  Results: least-squares whole translation equalizes the contract at
  `LH=26.405/RH=26.405` but visually rejects. Source-only guitar rotation is
  a structural reject (`LH=38.837/RH=21.273`). Pair-fit emitted with the
  default transpose quaternion convention is a structural reject
  (`LH=49.136/RH=9.955`). Pair-fit emitted with direct quaternion convention is
  the best current hand-overlay branch: `LH=18.376/RH=9.481`, zero suggested
  follow-up whole-guitar offset, and the visual proof is upright/bipedal with
  the guitar in a plausible across-body band. It is not accepted because direct
  user visual approval is still required and hand/arm contact remains rough.
  Next branch should refine hand/arm contact on top of the direct-quat pair-fit
  guitar-main frame, not return to whole-translation midpoint, source-only
  rotation, or transpose-quat pair-fit variants.

- 2026-08-19 pair-fit left-arm merge checkpoint: reproduced the current
  direct-quat pair-fit guitar frame and merged canonical fret full-hand banks
  with visible left-arm channels. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pairfit_leftarm_merge_decision_20260819.json`,
  `pairfit_leftarm_r094_contract_report_20260819.json`,
  `pairfit_leftarm_pos_contract_report_20260819.json`,
  `pairfit_leftarm_rotpos_contract_report_20260819.json`,
  `pairfit_leftarm_r094_pose_review_20260819/visual_decision.json`, and
  `pairfit_leftarm_rotpos_pose_review_20260819/visual_decision.json`.
  Results: rot-only visible-left-arm merge (`canonical-fret-visible-left-arm-rot`)
  improves structural contact to `LH=7.507/RH=9.481` while preserving the
  direct-quat pair-fit main hash. Reach scales `0.94`, `1.20`, and `1.50`
  converge to the same one-frame result. Position-only is weaker
  (`LH=15.038/RH=9.481`). Rot+pos has lower anchor split (`111.721`) but
  visually rejects because it introduces a displaced sleeve/arm artifact near
  the torso. Current best branch is `pairfit_leftarm_r094`, but it is not
  accepted; direct user visual approval is still required and the visible pose
  remains rough. Next branch should refine hand/arm silhouette/contact on top
  of `pairfit_leftarm_r094`, not use rot+pos.

- 2026-08-19 two-sided/source arm diagnostic rejection: extended
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py` with
  `canonical-hands-visible-arm-rot`, `canonical-hands-source-arm-rot`, and a
  source local-rotation mapping helper. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pairfit_arm_source_vs_aim_decision_20260819.json`,
  `pairfit_twoarm_rot_contract_report_20260819.json`,
  `pairfit_sourcearm_direct_contract_report_20260819.json`,
  `pairfit_sourcearm_anim_contract_report_20260819.json`, and
  `pairfit_sourcearm_helper_contract_report_20260819.json`. Results: solving
  both visible arms worsens the strum side (`LH=7.507/RH=23.300`). Raw source
  arm rotations also do not beat `pairfit_leftarm_r094`: direct basis is
  `17.682/10.284`, anim is `10.260/19.892`, helper is `19.267/20.931`.
  None were captured. Current best remains `pairfit_leftarm_r094`. Next work
  should not continue two-sided aim or raw source-arm rotation mapping; look
  for a localized silhouette/contact correction that preserves the strum-side
  canonical arm.

- 2026-08-19 visible left-hand quat copy rejection: added
  `canonical-fret-visible-left-arm-hand-rot` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`, which keeps the r094
  upper/forearm rotation merge and also copies canonical
  `bone_fret_hand.mesh.quat` onto visible `bone_L-hand.mesh.quat`. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pairfit_leftarm_handrot_decision_20260819.json`,
  `pairfit_leftarm_handrot_contract_report_20260819.json`, and
  `pairfit_leftarm_handrot_pose_review_20260819/visual_decision.json`.
  Result: structural left contact worsens to `LH=12.562/RH=9.481`, and the
  visual proof does not materially improve the rough hand/arm contact over
  r094. Reject this variant. Current best remains `pairfit_leftarm_r094`.

- 2026-08-19 left clavicle aim rejection: added
  `canonical-fret-visible-left-clavicle-arm-rot` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`, solving left clavicle
  toward the fret target before the r094 upper/forearm rotation solve. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pairfit_leftclav_decision_20260819.json`
  and `pairfit_leftclav_contract_report_20260819.json`. Result:
  `LH=17.670/RH=9.481`, `SOLVE-DELTA=136.157`, so this is a structural reject
  with no capture. Current best remains `pairfit_leftarm_r094`; do not continue
  clavicle aim solves.

- 2026-08-20 visible left-hand-position-only rejection: added
  `canonical-fret-visible-left-handpos-arm-rot` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`. This keeps the r094
  upper/forearm rotation solve but adds only `bone_L-hand.mesh.pos`, avoiding
  the rejected rot+pos branch's forearm position channel. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pairfit_left_handpos_armrot_decision_20260820.json`,
  `pairfit_left_handpos_armrot_bake_manifest_20260820.json`, and
  `pairfit_left_handpos_armrot_contract_report_20260820.json`. Result:
  structural reject before capture: `LH=19.755/RH=22.861`,
  `SOLVE-DELTA=80.144`. The smaller anchor split is not useful because both
  hand contacts are much worse than r094. Current best remains
  `pairfit_leftarm_r094`; do not continue visible left-hand position alone.

- 2026-08-20 source-anim emitted-rotation recheck: re-ran the earlier
  source/body-relative guitar rotation idea after the probe's rotation-emission
  bug had been fixed, using source-anim pelvis-delta placement plus
  source-anim pelvis-relative rotation and verifying that
  `bone_pos_guitar.mesh.quat` was emitted. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/sourceanim_rotanim_emitrot_reprobe_decision_20260820.json`,
  `sourceanim_rotanim_emitrot_reprobe_bake_manifest_20260820.json`, and
  `sourceanim_rotanim_emitrot_reprobe_contract_report_20260820.json`. Result:
  structural reject before capture: `LH=37.955/RH=28.296`,
  `SOLVE-DELTA=96.912`. Current best remains `pairfit_leftarm_r094`; do not
  continue source-bridge pelvis-relative guitar rotation for this pose.

- 2026-08-20 r094 average shared-offset rejection: tested a small whole
  guitar/target-frame nudge on top of `pairfit_leftarm_r094`, using the average
  visible-hand-minus-target residual as `--guitar-world-offset
  -2.127,-0.808,4.955`. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pairfit_leftarm_r094_avgoffset_decision_20260820.json`,
  `pairfit_leftarm_r094_avgoffset_bake_manifest_20260820.json`, and
  `pairfit_leftarm_r094_avgoffset_contract_report_20260820.json`. Result:
  structural reject before capture: `LH=30.404/RH=26.040`,
  `SOLVE-DELTA=70.610`. The lower internal guitar-anchor split is not useful
  because both visible hand contacts are much worse than r094. Current best
  remains `pairfit_leftarm_r094`; do not continue whole shared-offset nudges
  around the same pair-fit frame.

- 2026-08-20 r094 reproduction base audit: identified that recent r094
  follow-up reprobes used `controlroot_stockupper_full_candidate` as the base,
  while the retained r094 manifest was built from an older
  `midori_pairfit_leftarm_mesh_candidate` scratch base that has since been
  cleaned. Added diagnostic-only
  `--suppress-guitar-rotation-output` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py` to reproduce pre-fix
  no-guitar-quat ACP shape, but current-source rebuilds still do not hash-match
  the retained old mesh-armchain seed. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r094_repro_base_audit_20260820.json`.
  Current best remains the retained `pairfit_leftarm_r094` visual/contract
  evidence; do not claim further controlroot-stockupper-base reprobes are
  apples-to-apples r094 refinements unless an explicit correct-base r094
  candidate is retained or hash-exactly reconstructed.

- 2026-08-20 r094 reproduction follow-up: tested packaging without
  `--control-root-pelvis-parent`. This exactly reproduces the old
  mesh-armchain seed fret/strum hashes when combined with no-guitar-quat ACP
  shape and `--move-self 0`, but the old mesh seed main hash and old r094 main
  hash still are not present or reproduced. A bounded PowerShell search found
  15 `gh3_midori_main.milo_ps2` files and none matched old mesh seed
  `255F01FB...` or old r094 main `23DA06BE...`. The missing old main remains
  the blocker for hash-exact r094 reconstruction; retain/reconstruct it before
  claiming exact r094 follow-up variants.

- 2026-08-20 r094 exact reproduction resolved: the old mesh-armchain seed was
  reproduced with emitted guitar quat, `hmx-quat-mode transpose`, no
  `--control-root-pelvis-parent`, and `--move-self 0`. Then exact r094 was
  reproduced by composing that seed main with canonical full fret/strum hand
  banks and running `canonical-fret-visible-left-arm-rot` with
  `hand-target-pair-midpoint`, `hand-target-pair-fit`,
  `rotation-source-basis anim`, `arm-chain-reach-scale 0.94`,
  `hmx-quat-mode direct`, no `--control-root-pelvis-parent`, `--move-self 0`,
  and canonical strum/ui retained. Retained exact candidate:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/repro_exact_pairfit_leftarm_r094_candidate_20260820`.
  Hashes match retained r094 evidence for main/fret/strum/ui:
  main `23DA06BE...`, fret `240AAFF7...`, strum `06340333...`, ui
  `4A1B16DA...`; contract matches `LH=7.507/RH=9.481`,
  `SOLVE-DELTA=129.330`. Use this retained candidate for exact r094 follow-up
  variants.

- 2026-08-20 exact-base visible left-hand-position-only retest: reran
  `canonical-fret-visible-left-handpos-arm-rot` on the corrected exact
  meshmain/full-hands base instead of the wrong stockupper base. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/exactbase_left_handpos_armrot_decision_20260820.json`,
  `exactbase_left_handpos_armrot_bake_manifest_20260820.json`, and
  `exactbase_left_handpos_armrot_contract_report_20260820.json`. Result:
  structural reject before capture: `LH=11.253/RH=9.481`,
  `SOLVE-DELTA=120.815`. This is far less broken than the earlier wrong-base
  reprobe, but it still worsens r094's left contact (`7.507`), so do not
  continue visible left-hand position alone.

- 2026-08-20 exact-base visible left-hand-position blend sweep: added
  `--visible-hand-pos-blend` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py` and swept blends
  `0,0.25,0.5,0.75,1.0` for
  `canonical-fret-visible-left-handpos-arm-rot` on the exact r094 base.
  Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/exactbase_handpos_blend_sweep_decision_20260820.json`
  plus per-blend contract reports. Result is monotonic reject before capture:
  blend `0` reproduces r094 (`LH=7.507/RH=9.481`), then positive blends worsen
  left contact: `0.25 -> LH=7.706`, `0.5 -> LH=8.491`,
  `0.75 -> LH=9.722`, `1.0 -> LH=11.253`; RH stays `9.481`.
  Do not continue visible left-hand position blending.

- 2026-08-20 exact-base source target pair-fit rejection: added
  `--pair-fit-target-mode` and `--emit-hand-target-proxies` diagnostics to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`, then tested post-fit
  stock/source target modes plus source palm/source IK pair-fit with emitted
  proxy targets across direct/anim/helper bases. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/exactbase_source_target_pairfit_decision_20260820.json`.
  Result: structural reject before capture. The best rejected variant was
  source IK direct at `LH=24.206/RH=11.930`, `SOLVE-DELTA=151.290`;
  r094 remains best. Do not continue source-palm/source-IK pair-fit target
  modes for this pose.

- 2026-08-20 exact-base local rotation-correction rejection: swept small
  post-local `--rotation-correction` variants on exact r094 (`x/y/z` at
  `15`, `345`, `30`, `330` degrees). Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/exactbase_rotation_correction_sweep_decision_20260820.json`.
  Result: structural reject before capture. Every local-axis correction
  catastrophically regressed left-hand contact; best was `y-30-post` at
  `LH=33.941/RH=13.096`, `SOLVE-DELTA=121.136`. Do not continue simple
  local-axis post rotation-correction sweeps around r094.

- 2026-08-20 exact-base pair-axis roll visual rejection: added
  `--pair-axis-roll-degrees` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`, rolling the fitted guitar
  around the final `bone_fret_hand.mesh` / `bone_strum_hand.mesh` proxy axis.
  Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/exactbase_pair_axis_roll_decision_20260820.json`.
  Best metric variant was `+90` at `LH=7.007/RH=11.038`,
  `SOLVE-DELTA=122.381`, so it got a no-ISO local viewer capture:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pairaxisroll_p90_pose_review_20260820/midori_1_hand_overlay_f010.bmp`.
  Visual decision: reject; upright/bipedal, but the guitar is behind/through
  the torso and the strum-hand relationship is less coherent than r094. Do not
  continue pair-axis roll sweeps around r094.

- 2026-08-20 exact-base authored twist roll rejection: derived exact
  `--pair-axis-roll-degrees` angles by projecting source/stock authored guitar
  frame rows around r094's final fitted fret/strum axis. Sources tested:
  source relative guitar frame in direct/anim/helper bases, source world guitar
  frame in direct/anim/helper bases, stock local guitar frame, and stock
  attach-world frame. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/exactbase_authored_twist_roll_decision_20260820.json`.
  Result: structural reject before capture. Best summed contact was stock local
  row1 at `LH=18.753/RH=10.530`; best right-hand-only result was source world
  helper row1 at `RH=8.928` but with `LH=39.439`. Do not continue authored
  source/stock twist projection rolls around r094.

- 2026-08-20 exact-base stock prop pair-fit rejection: extended
  `--pair-fit-target-mode` with `stock-prop-comp-strings` and
  `stock-prop-comp-hand-locals`, then tested those modes with emitted fret/strum
  proxy targets on the exact r094 base. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/exactbase_stock_prop_pairfit_decision_20260820.json`.
  Result: structural reject before capture. Stock strings measured
  `LH=50.678/RH=27.729`; stock hand-locals measured `LH=51.241/RH=89.460`.
  Do not continue stock prop-compensated pair-fit target modes around r094.

- 2026-08-20 exact-base strum target guitar-rebase review candidate: added
  `--rebase-strum-target-to-guitar`, which interprets canonical
  `bone_strum_hand.mesh.pos` as `bone_pos_guitar.mesh` local and solves it back
  under `bone_strum.mesh`. Important correction: do not feed the retained final
  r094 candidate back into the probe; that double-applies r094 and produced an
  invalid r1 `LH=45.953` result. Reconstructed the pre-r094 exact
  meshmain/fullhands base from the seed recipe, verified exact r094 hashes and
  `LH=7.507/RH=9.481`, then reran this branch. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/exactbase_strum_guitar_rebase_decision_20260820.json`.
  Retained candidate:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/exactbase_strum_guitar_rebase_candidate_20260820`.
  Metrics: `LH=7.507/RH=8.135`, `SOLVE-DELTA=130.699`; main/fret/UI hashes
  match r094 and only strum changes (`733FEA4E...`). No-ISO visual capture:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/strum_guitar_rebase_pose_review_20260820/midori_1_hand_overlay_f010.bmp`.
  Treat this as current review candidate pending direct user visual approval.

- 2026-08-20 Control_Root/rootlocal body-animation rejection: captured the
  rootlocal output candidate through local `gh2_ps2_hybrid_assets/GEN` plus
  loose DLC only, at Idle/low priority, then restored canonical loose DLC
  anim/model hashes. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_rootlocal_visual_and_report_rejection_20260820.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/rootlocal_output_bipedal_pose_review_20260820/visual_decision.json`.
  Visual result: reject before user approval because most main body captures
  are sideways/horizontal rather than readable bipedal poses. Corrected
  report-only reruns agree: rootlocal medium-idle frame 60 has
  `max_pose_error=32.328456`, `right_shin_dot=-0.501932`,
  `left_shin_dot=-0.724448`, `min_child_aim=-0.607498`; attack-left frame 30
  has `max_pose_error=38.529800`, `min_child_aim=-0.497850`. Neighbor
  policies `rootlocal-rightshingate`, `rootbasis`, `pelvismeshpre`, and
  `pelvismeshpreinv` are not promotable. GLB/pose JSON remains acceptable as
  an automated bridge, but final output must still use the GH2 PS2
  `CharClipSet`/`CharClipSamples` writer path. Keep the goal open and continue
  at the pelvis/Control_Root orientation-basis solve; do not block unless the
  user explicitly asks.

- 2026-08-20 Control_Root bridge-convention direction: added
  `tools/gh3_midori_source_bridge_convention_audit.py` and fixed
  `tools/gh3_midori_visual_orientation_diagnostic.py` so explicit
  `--target-image` arguments do not append to stale defaults. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_bridge_convention_direction_20260820.json`.
  The older visually passing `modelparentcomp_pinned_forcepartial_nostock`
  proof used forcepartial/no-stock bridges and model-parent compensation; the
  rejected rootlocal proof used a fresh/world-root bridge convention with large
  `Control_Root` drift before staging. The audit reports rejected root drift of
  `129.685715` on medium-idle and `94.270244` on attack-left, with common-bone
  deltas up to `217.555184` units and `158.302445` degrees. Orientation
  reruns separate the branches cleanly: pinned forcepartial/no-stock
  modelparentcomp medium-idle passes (`upright_score=2.328571`), rootlocal
  medium-idle fails (`upright_score=0.915567`). Next body candidate should use
  pinned forcepartial/no-stock source bridges plus model-parent compensation,
  then move from the diagnostic matched-model/main proof toward ordinary loose
  DLC only after the orientation gate stays green.

- 2026-08-20 next build recipe recorded:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pinned_forcepartial_modelparentcomp_next_build_recipe_20260820.json`.
  The original intended no-deploy structural build used
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-targetlength-bakepos-bind`,
  `.codex/current-evidence/midori-review-source-bridges-pinned-forcepartial-5case-20260819/review_source_bridge_batch_manifest.json`,
  `--no-gh2-animation-rig`, `--no-stock-hand-detail-rig`,
  `--stock-bind-scope none`, and `--model-parent-compensate-acp`, but
  `none` is not accepted by `tools/gh3_midori_model_bundle.py`.

- 2026-08-20 pinned forcepartial/model-parent r2 ordinary-DLC rejection:
  reran the recipe with the valid `--stock-bind-scope upper-limbs-guitar`.
  The no-deploy build exited 0 and model-parent ACP compensation patched all
  331 clips (`Control_Root` parent, `bone_pelvis.mesh` channel). Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pinned_forcepartial_modelparentcomp_acp_report_next_r2.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pinned_forcepartial_modelparentcomp_candidate_decision_20260820.json`.
  A no-ISO loose-DLC capture through `gh2_ps2_hybrid_assets/GEN` plus
  `gh2_ps2_hybrid_assets/DLC` failed all 9 representative captures; direct
  visual inspection of the contact sheet shows the main animation poses are
  mostly horizontal/off-floor, not coherent bipedal poses. The live loose DLC
  was restored after capture and the temporary backup was removed. Continue
  at the source-to-target orientation basis; do not block the active goal
  unless the user explicitly asks.

- 2026-08-20 pinned forcepartial/model-parent r3 contract fix candidate:
  found that r2 was an invalid model/animation contract test. The pipeline
  applied `--model-parent-compensate-acp` but did not force the model bundle
  or packed clipsets onto the matching `Control_Root -> bone_pelvis.mesh`
  graph unless `--control-root-pelvis-parent` was separately specified.
  Patched `tools/gh3_midori_build_pipeline.py` so model-parent compensation
  implies the effective Control_Root pelvis-parent contract for gates, model
  bundle, post-comp disk gate, and final clipset packing; raw ACP staging
  stays pre-compensation.
  Reran no-deploy r3 at Idle priority with no ISO/package/deploy/runtime proof.
  Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pinned_forcepartial_modelparentcomp_contractfix_r3_decision_20260820.json`.
  The model bundle now records `control_root_pelvis_parent=true`. Ordinary
  loose-DLC capture through local `GEN` + `DLC` produced upright coherent
  bipedal main animation captures. The initial camera-distance-150 proof only
  failed accessory framing because the raised guitar touched the top edge; the
  full camera-distance-165 rerun passed all 9 representative checks with no
  failures:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pinned_forcepartial_modelparentcomp_contractfix_r3_pose_review_cam165_20260820/pose_review_manifest.json`.
  Camera-165 orientation diagnostic also passed. Treat r3 as the current
  body-orientation review candidate pending direct user visual approval;
  hands/guitar remain separate unfinished work.

- 2026-08-20 r3 hand/guitar probe pass: measured the r3 hand-overlay contract
  against a flat temporary candidate folder. Baseline r3 is poor:
  `LH-FRET=41.731`, `RH-STRUM=16.112`, `SOLVE-DELTA=103.315`, matching the
  visual detached-hand problem in the r3 hand-overlay proof. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_hand_overlay_contract_report_20260820.json`.
  Reapplying the old exactbase strum-guitar-rebase recipe directly to r3
  improves metrics to `LH=10.926/RH=9.275` but still looks visually bad: guitar
  crosses the torso/face area and hands remain awkward.
  A compact r3 sweep found the best structural lead with
  `canonical-hands-visible-arm-rot`, current-proxy pair-fit, strum target
  rebase, and downward guitar offset `1.192,0.085,-6.376`, giving
  `LH=4.876/RH=4.730`. Visual probe:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_twoarm_down6_pose_review_20260820/midori_1_hand_overlay_f010.bmp`.
  This is still rejected as final because the guitar/right-arm silhouette is
  not clean. Decision evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_hand_guitar_probe_decision_20260820.json`.
  Live loose DLC was restored after every visual probe; scratch candidate dirs
  were removed. Next work: improve the two-arm/down6 visual silhouette before
  generalizing it into the full r3 animation pipeline.

- 2026-08-20 r3 pair-axis roll sweep: tested roll values from -120 through
  +120 degrees from the `twoarm_down6` recipe. The no-roll structural metrics
  remain best (`LH=4.876/RH=4.730`); `roll_m15` was the only visually coherent
  capture in the four-image batch but is still not promotable because the
  guitar/right-hand silhouette is wrong. Evidence contact sheet:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_roll_visual_batch_20260820/contact_sheet.png`.
  Next work should move to guitar/prop local basis or full-pipeline hand-target
  application rather than grinding more pair-axis roll.

- 2026-08-20 r3 rotation/model-basis probe: tested whether the `roll_m15`
  visual lead was being hurt by emitted guitar rotation, Hmx quaternion storage,
  or model-bundle guitar attach switches. Default transpose storage reproduced
  the lead (`LH=8.821/RH=10.798`); direct Hmx storage and suppressing
  `bone_pos_guitar` rotation are structural rejects. Model-basis variants using
  `--preserve-guitar-attach-local`, `--compensate-guitar-helper-for-main-anchor`,
  and both together all measured the same bad result (`LH=9.409/RH=34.878`) and
  did not visually improve the high/across-torso guitar silhouette. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_hand_guitar_probe_decision_20260820.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_model_basis_visual_batch_20260820/contact_sheet.png`.
  Next work: source-authored guitar/hand local-frame bridge or hand-target
  application path in the direct MILO writer.

- 2026-08-20 r3 hand-target application probes: added
  `--rebase-fret-target-to-pelvis` and
  `pair-fit-target-mode canonical-emitted-proxies` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`. The r3 target-space
  report confirms the fret proxy sample fits pelvis-space better than its
  declared parent, but emitted fret-pelvis rebase variants are structural
  rejects. Pair-fitting to the final emitted canonical proxy worlds is also a
  structural reject, as are `canonical-hands-source-arm-rot` probes across
  direct/anim/helper source bases. Evidence is summarized in
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_hand_guitar_probe_decision_20260820.json`.
  Next useful work should preserve the r3 visible-arm lead and solve only the
  final guitar/hand local frame or constrained prop offset.

- 2026-08-20 r3 m15 final-offset refinement: a bounded visual/metric sweep
  around the coherent `m15` roll lead found that lowering the final guitar
  offset improves the silhouette. Current best lead is `lessx_down10`, offset
  `-0.808,0.085,-10.376`, roll `-15`, with `LH=4.156/RH=2.882`. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_m15_down10_refine_visual_batch_20260820/contact_sheet.png`.
  Retain it as the current hand/guitar refinement lead, but not a final
  candidate: the guitar/hand relationship is still awkward and too low around
  the hands/skirt line. Next work should keep this lowered silhouette and
  solve the remaining right-hand/guitar-body local-frame alignment.

- 2026-08-20 r3 lessx/down10 micro refinement: current best hand/guitar lead
  is now `roll_m10`, offset `-0.808,0.085,-10.376`, roll `-10`, with
  `LH=3.644/RH=2.015`. Visual evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_lessx_down10_micro_visual_batch_20260820/contact_sheet.png`.
  Retain as a refinement lead only; it is still not direct-approval quality.
  Next work should keep this silhouette and solve remaining prop/body overlap
  through a constrained guitar-body/right-hand local-frame correction.

- 2026-08-20 r3 roll_m10 post-prop translation: added
  `--post-guitar-world-offset` to the hand-bake probe and tested simple
  post-emission guitar translation from the `roll_m10` lead. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_rollm10_postprop_visual_batch_20260820/contact_sheet.png`.
  Result: reject as a fix. `post_none` remains at least as good visually as
  the tested offsets. Current lead is still `roll_m10`; next work should test a
  constrained post-guitar local rotation or right-hand/guitar-body frame
  correction while preserving that lead.

- 2026-08-20 r3 roll_m10 post-rotation: added
  `--post-guitar-rotation-correction` to the hand-bake probe and tested small
  emitted-only guitar rotations after hand/target solving. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_rollm10_postrot_visual_batch_20260820/contact_sheet.png`.
  Result: reject. `y_p10` is tolerable but not better than baseline; `x_p10`
  and `z_p10` are worse. Current lead remains `roll_m10` with no post
  correction. Next useful work should inspect source-authored
  guitar-body-to-visible-hand frame relationships across frames or begin a
  limited multi-frame generalization of the current lead.

- 2026-08-20 r3 reconstruction-mismatch closure: added
  `tools/gh3_midori_reconstruction_compare_report.py` plus bake-manifest replay
  diagnostics in `tools/gh3_midori_guitar_frame_hand_bake_probe.py`. For the
  failing `rotpos_current` frame20 case, probe-side replay already matches
  rebuilt-MILO contract failure (`LH=31.453/RH=15.784`), so ACP/MILO packaging,
  sample order, viewer override order, and GLB bridging are not the cause.
  Recomputing visible arm positions after arm rotation and direct quaternion
  mode both reject. A final-replay visible-hand endpoint solve passes metrics
  after rebuild (`LH/RH ~= 0`) but stretches the visible forearm-hand segments
  badly across frames (`LForeHand=27.46/38.44/38.69` at frames 20/30/40), so it
  is a diagnostic only, not a promotable bipedal fix. Evidence root:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_reconstruction_compare_rotpos_current_f20_20260820`.
  Next work should return to the current visual lead (`roll_m10` with
  lessx/down10) and solve a constrained guitar-body/right-hand local-frame
  correction or source-authored guitar/hand frame bridge, not continue
  unconstrained visible-hand endpoint solving.

- 2026-08-20 r3 GLB/MILO bridge and constrained-reach resume: refreshed the
  ihatecompvir source audit at
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_ihatecompvir_bridge_audit_20260820.json`
  and `analysis/gh3_midori_ihatecompvir_bridge_audit.json`. Result:
  `glTFMilo` is not a drop-in GH2 PS2 final converter, but MiloLib/GH2 notes are
  reference-ready for a direct CharClipSamples writer/validator. Route gate now
  only fails the expected visual approval/precheck for the rejected candidate.
  Added `tools/gh3_midori_guitar_reach_constraint_report.py`. Its abstract
  reach report showed `forearm_hand` low offset conflict, but the actual
  bake-probe replay with those offsets rejects across frames:
  `LH=18.66/21.16/39.73/37.95/34.60`, `RH=19.65/24.00/11.68/15.29/15.73`.
  Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_guitar_reach_constraint_report_20260820.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_forearm_hand_avg_offset_probe_20260820/actual_bake_replay_summary.json`.
  Decision: reject simple average guitar/proxy translation even when abstract
  reach offsets look compatible. Next useful work should implement/validate a
  real per-frame GLB/pose-to-GH2 CharClipSamples path or richer source-authored
  hand/guitar transform constraint inside the direct writer.

- 2026-08-20 r3 writer-contract, torso-axis, and actual-stage source gap:
  added `tools/gh3_midori_pose_bridge_writer_contract_report.py` and
  `tools/gh3_midori_torso_axis_contract_report.py`. The active 331-clip staged
  ACP manifest is GH2-writer-native (`.pos/.quat`, type-ordered, zero
  unsupported channels), and six clips are pose-bridge-active:
  `gh3_guit_midori_tran_atoout` (ui/main), `gh3_guit_mido_a_attackl`,
  `gh3_guit_mido_a_fst_jump01`, `gh3_guit_mido_a_fst_solo01`, and
  `gh3_guit_mido_a_med_idle01`.
  Important correction: the actual r3 stage manifest uses
  `.codex/current-evidence/midori-review-source-bridges-pinned-forcepartial-5case-20260819/review_source_bridge_batch_manifest.json`,
  which is writer-native but missing 11 critical source records for the
  emitted hand/guitar targets: left/right clavicle, upper arm, forearm, hand,
  `bone_pos_guitar`, `bone_fret_hand`, and `bone_strum_hand`. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_pose_bridge_writer_contract_report_actual_stage_20260820.json`.
  A contrast run against the richer attack arm/guitar bridge is clean
  (`writer_native_pose_bridge_ready`, zero critical gaps), proving the
  required GLB/pose source data exists but is not wired into the actual r3
  staging. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_pose_bridge_writer_contract_report_rich_attack_bridge_20260820.json`.
  The next engineering step should rebuild/export the pinned forcepartial
  source-bridge manifest with the same body convention plus the missing
  arm/hand/guitar source records, then restage r3 from that manifest.
  Torso-axis evidence still proves the deployed loose DLC versus analysis-r3
  distinction: live deployed old fails medium idle (`absZ=0.207/absX=0.937`),
  while analysis `pinned_forcepartial_modelparentcomp_contractfix_r3` passes
  all five sampled torso axes, including medium idle
  (`absZ=0.914/absX=0.321`). Evidence:
  `r3_torso_axis_live_deployed_20260820.json` and
  `r3_torso_axis_analysis_contractfix_20260820.json`. Decision: body
  orientation should resume from analysis r3/model-parent-compensated
  Control_Root-pelvis-parent output, not the restored deployed old hashes.
  Remaining blocker is actual-stage hand/guitar source coverage and silhouette
  quality, not ISO runtime, GLB as an intermediate, or GH2 writer capability.

- 2026-08-20 r4 source-coverage bisect: re-exported the pinned
  forcepartial source bridges through Blender 4.5 and pinned NXTools
  `C:\Users\smmel\AppData\Local\Temp\nxtools_ref` at idle priority, adding
  the missing arm/hand/guitar source bones. The wrapper deleted extracted GH3
  scratch inputs (`extracted_inputs_retained=false`); no game-time ISO path was
  used. Evidence manifest:
  `.codex/current-evidence/midori-review-source-bridges-pinned-forcepartial-handguitar-5case-20260820/review_source_bridge_batch_manifest.json`.
  The full fresh re-export fixed writer coverage but regressed torso axes
  (`failures=3/5`), so it is rejected as a candidate. Bisect manifests showed
  old body plus arms-only passes torso, and old body plus guitar-only also
  passes torso; the guitar-only build reproduces the original r3 main hash.
  The safe route is the merged manifest that preserves old pinned body records
  and grafts in the 11 donor hand/guitar records:
  `.codex/current-evidence/midori-review-source-bridges-pinned-forcepartial-merged-handguitar-5case-20260820/review_source_bridge_batch_manifest.json`.
  That merged candidate is writer-native with zero critical gaps and passes the
  five-case torso-axis gate:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r4_merged_actual_stage_pose_bridge_writer_contract_20260820.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r4_merged_torso_axis_20260820.json`.
  However, the hand-overlay contract is unchanged from r3:
  `LH-FRET=41.731`, `RH-STRUM=16.112`, `SOLVE-DELTA=103.315`; evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r4_merged_hand_overlay_contract_20260820.json`.
  Decision: keep the merged manifest/stage as the structurally cleaner base,
  but do not visually promote it. The next work is not more source-coverage
  plumbing; it is applying the now-available arm/hand/guitar source records in
  the hand/guitar local-frame solve path so the emitted hand target channels
  actually change.

- 2026-08-20 r5 hand-target local-frame diagnostics: added
  `tools/gh3_midori_final_hand_target_local_solve_report.py`. It reconstructs
  the final GH2 graph with the same math as
  `gh3_midori_guitar_ik_contract_report.py` and solves the local
  `bone_fret_hand.mesh` / `bone_strum_hand.mesh` `.pos` rows needed to put
  each target on the current visible hand. Re-tested the existing
  source-helper hand-root modes on the merged body-safe/source-complete base:
  raw `source-ik-helper` remains a structural reject (`LH=40.180`,
  `RH=15.751`), and `source-ik-helper-gh2scale` is worse (`LH=50.463`,
  `RH=17.121`). Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r5_sourceik_hand_overlay_contract_20260820.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r5_sourceik_gh2scale_hand_overlay_contract_20260820.json`.
  The final target-local solve for `midori_1_hand_overlay_f010` reports the
  actual GH2 locals needed at that frame: left
  `[0.840926,-1.174859,-40.127533]`, right
  `[-3.82114,-2.992369,15.026728]`; evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r5_restored_merged_final_hand_target_local_solve_20260820.json`.
  A diagnostic build using those solved locals proves the target channel path
  can move in the right final frame (`LH/RH=0.000`) while keeping torso axes
  green, but `SOLVE-DELTA=131.677` and direct local-GEN capture reject it:
  the body is upright, yet the guitar remains a black vertical mass behind/right
  of the performer and the visible pose does not materially change. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r5_final_local_solve_hand_overlay_contract_20260820.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r5_final_local_solve_hand_overlay_visual_20260820/midori_1_hand_overlay_f010.bmp`.
  The live loose DLC was restored after capture, and `analysis` was restored to
  the merged r4 baseline (`gh3_midori_main.milo_ps2` SHA256
  `BD92EDCB3CE5998C3ED75694724ABAF671E002E04F8538600E95E8491405E529`).
  Decision: do not promote static source-helper locals or single-frame final
  target locals. Next work should solve/move the visible guitar body and the
  fret/strum targets together in one final GH2 frame, then validate multiple
  frames before another visual capture.

- 2026-08-20 r6 coupled guitar-anchor diagnostic: extended
  `tools/gh3_midori_final_hand_target_local_solve_report.py` to solve a rigid
  `bone_pos_guitar.mesh` transform from the current final GH2 fret/strum target
  points to the visible hands, then added diagnostic ACP staging overrides:
  `--main-guitar-local-position-override` and
  `--main-guitar-local-rotation-override`. The coupled solve for
  `midori_1_hand_overlay_f010` predicts a rigid-fit residual of `3.507` per
  side, with local guitar position `[-14.957951,15.083294,9.594125]` and local
  rotation rows
  `[-0.013927,-0.668297,0.743764]`,
  `[-0.786855,-0.451652,-0.420558]`,
  `[0.616981,-0.591092,-0.519563]`; evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r6_merged_coupled_guitar_anchor_solve_20260820.json`.
  The no-deploy build keeps all five torso-axis cases green and improves the
  hand-overlay contract from r4's `LH=41.731/RH=16.112` to
  `LH=7.520/RH=3.569`; evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r6_coupled_guitar_anchor_hand_overlay_contract_20260820.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r6_coupled_guitar_anchor_torso_axis_20260820.json`.
  Direct local-GEN capture is visibly improved but still rejected: the guitar
  is no longer a featureless vertical slab behind her, but it is too high/right
  and the arm/hand silhouette is not coherent. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r6_coupled_guitar_anchor_hand_overlay_visual_20260820/midori_1_hand_overlay_f010.bmp`.
  Tested twist `+90` and `-90` around the fitted target axis; both preserve
  torso but are structural rejects (`+90 LH=50.463/RH=13.077`, `-90
  LH=47.383/RH=7.414`), likely because the current HMX quaternion path does not
  replay the twist report exactly. Analysis was restored to the 0-degree
  coupled lead (`gh3_midori_main.milo_ps2` SHA256
  `9127F77DE5292408529E055C8E1F37491E2BA4058D87D6FDB5757BD5FBC71DC4`). Next
  work should refine the coupled guitar-anchor orientation in the emitted GH2
  quaternion frame or derive the twist in post-replay/HMX space, not return to
  target-only solves.

- 2026-08-20 r7 HMX replay diagnosis: made sampled quaternion replay explicit
  in `tools/gh3_midori_guitar_ik_contract_report.py`,
  `tools/gh3_midori_final_hand_target_local_solve_report.py`, and
  `tools/gh3_midori_torso_axis_contract_report.py` via
  `--sample-quat-mode direct|hmx`. The r6 emitted guitar override matches the
  intended local rotation only under HMX replay, but the old direct hand metric
  was over-optimistic: r6 measures `LH=7.520/RH=3.569` in direct replay and
  `LH=7.277/RH=21.586` in HMX replay. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r7_current_analysis_hand_overlay_contract_direct_20260820.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r7_current_analysis_hand_overlay_contract_hmx_20260820.json`.
  Re-solving the coupled guitar anchor in HMX replay produced residual
  `2.546` per side. The 0-degree HMX build validates structurally
  (`LH=2.546/RH=2.547`, torso HMX `0/5` failures), but direct capture is a
  visual reject: the guitar/arms still do not read as coherent. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r7_hmx_coupled_guitar_anchor_hand_overlay_contract_hmx_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r7_hmx_coupled_guitar_anchor_torso_axis_hmx_20260820.json`, and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r7_hmx_coupled_guitar_anchor_visual_20260820/midori_1_hand_overlay_f010.bmp`.
  Added `tools/gh3_midori_capture_with_loose_dlc_backup.py` so future local
  captures can temporarily deploy analysis MILOs to ordinary loose DLC and
  restore them afterward without using the ISO at game time. Tested a
  visually-guided `+60` HMX twist candidate, chosen as the closest roll to the
  less-bad r6 guitar orientation while preserving the `2.546` contact residual;
  it still fails direct visual review because the guitar reads as a vertical
  black bar through the torso and the arm mesh remains tangled. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r7_hmx_coupled_guitar_anchor_twist60_hand_overlay_contract_hmx_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r7_hmx_coupled_guitar_anchor_twist60_torso_axis_hmx_20260820.json`, and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r7_hmx_coupled_guitar_anchor_twist60_visual_20260820/midori_1_hand_overlay_f010.bmp`.
  The live loose DLC hashes were restored after capture. `analysis` currently
  contains the r7 HMX twist60 lead (`gh3_midori_main.milo_ps2` SHA256
  `F43FD6898EF442A8C85CD3FA8AE72B437C3896C6CACC1D46288002979CA03981`).
  Next work should keep HMX replay as the authoritative structural metric and
  solve against visible guitar body/arm silhouette, not just fret/strum target
  points or direct-mode reports.

- 2026-08-20 r8 silhouette-aware guitar orientation check: added
  `tools/gh3_midori_guitar_silhouette_report.py`, which measures the largest
  dark guitar-like BMP component, its screen-space major-axis angle, aspect,
  and torso-band overlap. It agrees with direct visual inspection: r6 is not a
  vertical bar (`angle=22.743`, `aspect=1.479`, `overlap=0.046`), r7 0-degree
  has heavy torso overlap (`overlap=0.784`), r7 twist60 is vertical-bar-like
  (`angle=80.378`, `aspect=0.539`, `overlap=0.982`), and r8 twist30 is also
  vertical-bar-like (`angle=-71.360`, `aspect=0.557`, `overlap=0.933`).
  Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r8_guitar_silhouette_compare_final_20260820.json`.
  Extended `tools/gh3_midori_final_hand_target_local_solve_report.py` with
  `--coupled-fixed-local-rotation` to solve best-fit translation while
  preserving a chosen guitar local orientation. Using the less-bad r6 local
  orientation under HMX replay gives a symmetric contact compromise
  (`LH=9.504/RH=9.504`) and torso remains green (`0/5` failures). Direct
  capture confirms this fixes the vertical black-bar failure but is still a
  visual reject: the guitar body is too high/right across the chest and the arm
  mesh remains tangled. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r8_hmx_fixed_r6orient_hand_overlay_contract_hmx_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r8_hmx_fixed_r6orient_torso_axis_hmx_20260820.json`, and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r8_hmx_fixed_r6orient_visual_20260820/midori_1_hand_overlay_f010.bmp`.
  `analysis` currently holds this r8 fixed-r6-orientation lead
  (`gh3_midori_main.milo_ps2` SHA256
  `5DEAB638D02C2ADADE67B4813DEF98D69F7DC39138CD79AFD4C5B062F55A5F22`).
  Next work should use the silhouette report as a rejection gate and move from
  guitar-only fitting to arm/hand pose correction: the visible prop orientation
  can be made non-vertical, but the arm chain is still the blocker.

- 2026-08-20 r9 arm-chain probe replay/alias diagnosis: preserved the r8
  non-vertical guitar local and tested `visible-arm-chain-rotpos` overlays from
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`. First, fixed two
  diagnostic issues: `gh3_midori_guitar_frame_hand_bake_probe.py` now accepts
  `--sample-quat-mode direct|hmx`, and
  `gh3_midori_guitar_ik_contract_report.py` maps bare sampled channels such as
  `bone_L-hand.pos` to `.mesh` transforms when needed. Without the alias fix,
  structural reports silently ignored diagnostic arm channels that the runtime
  visibly applied. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r9_r8_current_analysis_hand_overlay_contract_aliasfix_20260820.json`.
  The preserved-guitar HMX arm-chain candidate is a hard reject:
  `LH=33.904/RH=21.904`, `SOLVE-DELTA=117.605`, and direct capture shows a
  coherent upright body/guitar face but both arms detached and stacked on the
  right side. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r9_r8preserve_visible_arm_chain_rotpos_hmx_hand_overlay_contract_aliasfix_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r9_r8preserve_visible_arm_chain_rotpos_hmx_visual_20260820/midori_1_hand_overlay_f010.bmp`, and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r9_hmx_armchain_visual_silhouette_compare_20260820.json`.
  Re-emitting the same HMX arm-chain probe with explicit `.mesh` channel names
  produces the same bad structural result, so the problem is not just bare vs
  mesh channel naming. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r9_r8preserve_visible_arm_chain_rotpos_hmx_mesh_hand_overlay_contract_20260820.json`.
  `analysis` remains on the r8 fixed-r6-orientation lead; do not promote the
  r9 arm-chain diagnostic candidates. Next work should solve the arm chain in
  the actual overlay layer parent/local convention, probably by comparing
  emitted `bone_*upperArm/foreArm/hand` channel locals against final HMX replay
  worlds before and after runtime application.

- 2026-08-20 r10 full-stage final-hand emission diagnosis: fixed
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py` so final hand solving
  replays the full main clip first and emits the solved final hand values into
  the packed ACP sample rows. Lightweight r10 one-clip candidates are invalid
  because they discard the full r8 main/guitar context, so the useful proof was
  rebuilt as a full-stage candidate with only
  `gh3_hnd_guit_chord_mid_bar3_d.acp` and
  `gh3_hnd_guit_strum_mido_norm_m01_d.acp` swapped. This solves the HMX hand
  contract for `midori_1_hand_overlay_f010` exactly (`LH=0.000`,
  `RH=0.000`) but is still a visual reject: upright/non-vertical guitar, yet
  the upper body, head/hair, and arms collapse through the chest/shoulder
  frame. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r10_fullstage_finalhands_emitfix_hand_overlay_contract_hmx_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r10_fullstage_finalhands_emitfix_visual_20260820/midori_1_hand_overlay_f010.bmp`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r10_fullstage_finalhands_emitfix_silhouette_compare_20260820.json`.
  Do not promote r10; `analysis` remains on the r8 fixed-r6-orientation lead.
  Resume by diagnosing the matrix-local `Control_Root`/pelvis convention before
  more arm/guitar fitting.

- 2026-08-20 r12 actual-MILO `Control_Root`/pelvis replay check: added
  `--control-root-pelvis-parent` to the root/pelvis basis diagnostics and
  reran them as
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r12_control_root_parented_basis_sweep_20260820.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r12_root_pelvis_parented_bridge_diagnostic_20260820.json`.
  The bridge-space `122.666` degree source pelvis local/world mismatch remains,
  but inspecting the actual r8 MILOs changes the active diagnosis. New tool
  `tools/gh3_midori_actual_milo_parent_replay_report.py` proves the built
  character and clipset both parent `bone_pelvis.mesh` under `Control_Root`;
  the stale `analysis/gh3_midori_current_midori1_rig.json` says otherwise.
  Actual samples for `gh3_guit_mido_a_attackl` and `stand_medium_01` show the
  pelvis channel is static across checked samples while `bone_spine1.mesh` is
  dynamic. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r12_actual_milo_parent_replay_attackl_20260820.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r12_actual_milo_parent_replay_stand_medium_20260820.json`.
  Do not keep diagnosing the current r8/r10 rejects as an active pelvis
  animation explosion. The live lead has a stable parented pelvis; the next
  useful hierarchy rung is spine/chest/head/arm/guitar replay above that static
  frame, using actual MILO inspection or a regenerated rig JSON.

- 2026-08-20 r13 upper-chain actual-layer replay and head/neck freeze: added
  `tools/gh3_midori_actual_layer_replay_report.py` to compose actual built
  main/fret/strum samples on the actual character MILO graph. For
  `midori_1_hand_overlay_f010`, translations are plausible but upper rotation
  deltas from bind identify the bad tier: neck `150.886`, head `134.410`,
  L-clavicle `121.027`, R-clavicle `117.117` degrees. Source attribution:
  neck/head/L-clavicle come from main, R-clavicle from strum. Added
  `tools/gh3_midori_freeze_acp_channels_to_bind.py` and tested diagnostic
  freezes. Freezing head/neck/clavicles gives a coherent head but wrecks hand
  reach (`LH=33.540/RH=27.957`), so clavicles cannot simply be bound. Freezing
  only head/neck removes the head/neck warning, preserves the r8 hand contract
  (`LH=9.504/RH=9.504`), and the visual is a clear bipedal improvement, but it
  still rejects because clavicles/arms and guitar remain wrong. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r13_actual_layer_replay_hand_overlay_f010_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r13_headneck_freeze_layer_replay_f010_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r13_headneck_freeze_hand_overlay_contract_hmx_20260820.json`, and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r13_headneck_freeze_visual_20260820/midori_1_hand_overlay_f010.bmp`.
  Do not promote r13 as final; the useful next branch is to keep the head/neck
  treatment and solve clavicle/upper-arm rotations in the actual layer replay
  frame while preserving r8 hand/guitar metrics.

- 2026-08-20 r14 fractional clavicle-to-bind blend rejection: extended
  `tools/gh3_midori_freeze_acp_channels_to_bind.py` with a
  `head-neck-clavicle-blend` profile and tested 10%, 15%, 25%, and 50%
  clavicle blends while keeping r13 head/neck bind treatment. Results show the
  wrong tradeoff. 10% still leaves both clavicle warnings and worsens hand
  contract to `LH=10.008/RH=11.761`; 15% worsens to `LH=10.805/RH=12.927`;
  25% clears only R-clavicle and worsens to `LH=13.126/RH=15.261`; 50% clears
  clavicle warnings but worsens to `LH=20.577/RH=20.696`. Direct captures for
  10% and 25% are still rejects and look only marginally different from r13.
  Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r14_clavblend10_visual_20260820/midori_1_hand_overlay_f010.bmp`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r14_clavblend25_visual_20260820/midori_1_hand_overlay_f010.bmp`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r14_clavblend_visual_silhouette_compare_20260820.json`, and
  `r14_clavblend*_hand_overlay_contract_hmx_20260820.json`.
  Do not continue simple clavicle-to-bind damping. The next branch needs a
  clavicle/upper-arm solve in the actual layered arm/guitar frame, preserving
  the r13 head/neck fix and r8 hand/guitar reach.

- 2026-08-20 r15/r16 all-body head-neck and guitar-frame diagnostics:
  generalized the r13 head/neck bind-local patch into
  `head-neck-all-body`, covering `guitar-main` and `guitar-ui` staged ACP
  clips from local `analysis/gh3_midori_acp_stage`. r15 preserves the
  representative hand-overlay contract (`LH=9.504/RH=9.504`) and fixes
  head/neck rotation warnings across checked main-only cases; medium idle head
  and hair are visually upright. It is still rejected as final because the
  guitar/arm mass remains wrong in direct captures. Added
  `--skip-fret/--skip-strum` to `tools/gh3_midori_actual_layer_replay_report.py`
  so main-only body/guitar failures can be separated from hand overlays.
  r15 main-only diagnostics show remaining clip-dependent clavicle warnings
  and a constant `bone_pos_guitar.mesh` rotation delta of `61.894` degrees.
  r16 additionally bound `bone_pos_guitar.mesh.quat` and is a structural reject
  (`LH=21.923/RH=17.106`), proving the guitar body rotation cannot be fixed
  independently of fret/strum targets and arm chain. Next branch should keep
  r15 head/neck as a partial layer and solve guitar rotation, target proxies,
  clavicles, and upper arms together in the actual layered frame.

- 2026-08-20 r17-r20 bake-probe channel merge diagnostics: added
  `tools/gh3_midori_patch_bake_probe_positions_into_stage.py` to merge
  one-frame bake-probe `.pos/.quat` channels into a full staged ACP tree
  without replacing whole clips. r17 found a promising probe-level structural
  row (`visible-arm-chain + stock-prop-comp-strings`, `LH=2.098/RH=4.351`),
  but whole-ACP replacement dropped base context. r18 inserted missing channels
  and solved desired probe world positions back into the actual r15 local
  frame; packed result improved to `LH=7.599/RH=10.860` but direct visual still
  rejected. r19 inserted rot+pos channels and flipped the tradeoff
  (`LH=18.883/RH=4.524`). r20 hybridized r18-left with r19-right and is the
  best packed structural result in this branch (`LH=7.599/RH=4.524`), but
  direct capture still rejects: face remains fixed from r15, while the black
  guitar mass and tan arm shards stay incoherent. Keep the channel-merge tool;
  do not promote r18/r20. Next work needs a coupled rotation solve that includes
  clavicle local quats and visible guitar frame, not only forearm/hand endpoint
  channels.

- 2026-08-20 r21-r23 clavicle-first arm/guitar solve: applying clavicle changes
  after r20-style arm patches was the wrong order. Full bind clears rotation
  warnings but wrecks reach (`LH=37.046/RH=24.118`), and 25%/10% blends still
  trade away hand contract without visual approval. Reversing the order is the
  useful lead: bind clavicles first, regenerate the stock-strings arm-chain
  probe in that fixed-clavicle frame, then merge it into the full stage. r22
  packed as `LH=5.847/RH=12.453` with no suspicious rotation nodes. Direct
  capture remains rejected, but it is the first real visual movement in this
  branch: coherent body/face and guitar moved out of the torso; remaining
  failure is too-high/right guitar plus broken strum-side arm shards. A r23
  `down4` offset made the visual worse by crossing the face, so do not continue
  naive whole-frame down offsets. Current visual lead is r22; next work should
  keep clavicle-first ordering and solve strum/right arm plus guitar placement
  in that frame.

- 2026-08-20 r24-r27 fixed-clavicle proxy/placement diagnostics: added
  `--include-target-proxies` to
  `tools/gh3_midori_patch_bake_probe_positions_into_stage.py`, but explicit
  fret/strum proxy promotion worsens the packed contract
  (`LH=16.612/RH=15.834`), so missing target proxy channels are not the r22
  survival gap. A fixed-clavicle right rot+pos hybrid also worsens the right
  side (`LH=5.847/RH=17.331`). Placement offsets from the r22 frame are not a
  final path: `front4_down6` pulls arm/guitar mass across the face, while
  `back4_down6` is more coherent but still rejects with guitar high/right and
  arm mass draped across the chest (`LH=10.075/RH=12.453`). Keep r22/r27 as
  visual evidence, but do not continue broad translation offsets or proxy-only
  insertion. Next work should solve a rotation/attachment frame for visible
  guitar/strum arm inside the fixed-clavicle order.

- 2026-08-20 r28-r31 fixed-clavicle guitar-rotation diagnostics: added
  `bone_pos_guitar.mesh.quat` promotion to the probe-to-stage merge tool, so
  guitar rotation probes now become real packed MILO tests. In the fixed-clavicle
  frame, stock runtime attach-world, stock-rig local, pair-fit, and x-post
  correction rows all reject after packing. Attach-world packs as
  `LH=9.487/RH=14.738` and flags `bone_pos_guitar.mesh`; stock-rig packs as
  `LH=7.894/RH=15.160` and also flags `bone_pos_guitar.mesh`; x30post packs as
  `LH=10.812/RH=13.875` and flags `bone_pos_guitar.mesh`. Do not continue simple
  one-frame guitar quat overrides. r22/r27 remain the visual leads; next work
  should solve visible strum arm plus guitar attachment together, not rotate
  `bone_pos_guitar` independently.

- 2026-08-20 r32-r35 fixed-clavicle target-proxy survival diagnosis:
  reproduced r22 exactly with HMX sampling and mesh channel names
  (`r32l_r22_repro_hmx_mesh`), confirming `LH=5.850/RH=12.450`. Comparing the
  probe manifest to actual packed-layer replay showed the visible guitar,
  forearms, and hands survive packing exactly (`Delta=0.000`), while
  `bone_fret_hand.mesh` and `bone_strum_hand.mesh` do not. The earlier
  `--include-target-proxies` reject was caused by solving target proxy locals
  against the pre-move `bone_fret.mesh`/`bone_strum.mesh` parent frames.
  Patched `tools/gh3_midori_patch_bake_probe_positions_into_stage.py` so target
  locals under one-level guitar children are solved with the emitted guitar
  translation delta. r34 then packed as `LH=1.337/RH=0.000`, with no suspicious
  rotations and exact right target survival. Direct capture still rejects:
  Midori is bipedal and coherent, but the guitar sits over/behind the right
  shoulder and strum-side proxy/arm geometry tangles around the waist. r35
  corrected-target offsets (`down8`, `back4_down8`, `front4_down8`) also reject
  visually. Keep r34 as the new structural lead over r22, but do not promote it
  visually. Next branch should solve the guitar/strum attachment orientation or
  source-frame basis with corrected target-parent solving enabled, not return to
  missing-proxy or broad-offset hypotheses.

- 2026-08-20 r36-r39 corrected target-parent rotation/hand diagnostics:
  generalized `tools/gh3_midori_patch_bake_probe_positions_into_stage.py` so
  target-proxy local solves recompute parent worlds under the full emitted
  `bone_pos_guitar.mesh` local pos/quat, not only a translation delta. Retested
  the formerly rejected fixed-clavicle guitar rotation rows with corrected
  target solving. Structurally, they now survive much better:
  `r36b_attachworld_correct_targets` packs `LH=1.340/RH=2.830`,
  `r36c_stockrig_correct_targets` packs `LH=1.340/RH=0.000`, and
  `r36d_x30post_correct_targets` packs `LH=1.340/RH=0.000`. Direct captures
  still reject: attach/stock-rig produce suspicious guitar rotation or obvious
  over-shoulder guitar placement, and x30post visually matches the r34 failure.
  Source pelvis-relative orientation/placement bases (`direct`, `anim`,
  `helper`) are probe-level rejects, with large hand-target distances before
  packing. Added canonical-quat fallback plus `--channel-filter-regex` to the
  patch tool, then built `r39_handfinger_quats_on_correct_targets` by applying
  only hand/finger/target quats over the corrected r34-style target solve. r39
  remains structurally clean (`LH≈0/RH≈0`, no suspicious rotations) but direct
  capture visually rejects and looks effectively like r34. Diagnosis: the
  remaining failure is not missing target proxy survival, broad placement
  offset, simple guitar quat override, source pelvis basis, or target/finger
  quat promotion. Next branch should inspect the actual guitar/strum attachment
  parent model/mesh orientation and the relation between `bone_pos_guitar`,
  `bone_strum`, visible right arm, and rendered prop geometry.

- 2026-08-20 r40-r41 front-anchor recombination diagnostics: inspected the
  hardcoded guitarist runtime graph in ihatecompvir's `milo_convert_tool` and
  confirmed Midori's controllers use the expected `CharIKHand` targets and
  stock proxy graph. Reopened old coupled-anchor visuals: r8 is non-bipedal,
  while r10 puts the guitar in front but has broken/detached arms. Recombined
  only that front-guitar anchor idea with the corrected r34 target-parent solve.
  r40 moved `bone_pos_guitar.mesh` to the old r10 front-anchor world
  (`-6.014,2.106,48.814`), packing cleanly with `LH=7.716/RH=0.000`; the
  direct visual is bipedal and moves the guitar in front of the torso, but the
  left side and tan proxy/hand geometry tangle badly. A contract-guided offset
  from r40 produced r41b at guitar world roughly `-7.601,3.993,51.964`, packing
  as `LH=3.832/RH=0.000` with clean actual-layer replay. r41b is the best
  front-guitar visual so far, but still rejects: body/head are coherent and the
  guitar is no longer over the shoulder, yet the guitar remains too high/right
  across the chest and both arms/proxy meshes are tangled around the waist.
  Evidence: `r40_r34_targets_r10_frontanchor_contract_hmx_20260820.json`,
  `r40_r34_targets_r10_frontanchor_layer_replay_f010_20260820.json`,
  `r41b_r40_full_contract_offset_contract_hmx_20260820.json`,
  `r41b_r40_full_contract_offset_layer_replay_f010_20260820.json`, and the
  corresponding visual BMP directories. Next branch should combine r41b's
  front anchor with a visible arm solve in the final packed local frame, or
  inspect/hide the visible proxy/hand helper geometry if those tan meshes are
  non-rendered helpers in stock GH2 but visible in the converted Midori model.

- 2026-08-20 r47-r49c exact overlay source bridge / visible-hand position
  diagnosis: r47 established clean source-side Blender/NXTools GLB bridges for
  frame-10 exact overlay on main/fret/strum clips. r48 proved that feeding those
  bridges through ACP/MILO without visible palm position promotion still rejects:
  visible `bone_L-hand.mesh` / `bone_R-hand.mesh` had empty `sample_sources`,
  `hand_ratio=0.074428`, and the gate rejected like the old hidden-target
  local-only attempts. Added diagnostic
  `--bridge-visible-hand-position-bones` to `tools/gh3_midori_acp_stage.py`.
  The first r49 attempt showed the knob was accepted but initially wrote too
  late / through the ordinary hand-root mapping. r49c now writes explicit
  source-palm bridge positions into `bone_L-hand.mesh.pos` and
  `bone_R-hand.mesh.pos` before channel ordering. The temporary packed replay
  passes the no-capture overlay gate:
  `status=overlay_visual_sanity_pass`, `failures=0`,
  `hand_ratio=0.374905`, `target_ratio=-0.376701`,
  `guitar_ratio=0.141367`, `delta=-0.233538`, with visible hand mesh sources
  `left=pos:fret` and `right=pos:strum`. Evidence:
  `r49c_bridge_visible_hand_layer_replay_f010_20260820.json` and
  `r49c_bridge_visible_hand_visual_sanity_gate_20260820.json`. This is a real
  structural lead, but it is not direct visual approval and has not been
  promoted to loose DLC. Next work should build a full external candidate from
  the r49c visible-hand bridge path and run direct visual capture/review from
  loose DLC, not from an ISO.

- 2026-08-20 r50-r51 loose-DLC visual follow-up: r50 built a full temporary
  stock-all candidate from the r49c visible-hand ACP path and captured it through
  the guarded loose-DLC deploy/restore wrapper. The packed overlay gate still
  passed for both exact `gh3_guit_mido_c_med_idle` and runtime alias
  `stand_medium_01`, but direct native capture rejected hard: idle and overlay
  were upside down/collapsed. This proves the scalar overlay gate is too narrow
  and that r49c alone must not be promoted without the known Control_Root /
  model-parent-compensated branch. Added camera/offset passthroughs to
  `tools/gh3_midori_capture_with_loose_dlc_backup.py` for restore-safe centered
  recaptures. r51 then rebuilt the analysis pipeline with
  `--no-gh2-animation-rig --control-root-pelvis-parent
  --model-parent-compensate-acp --stock-bind-scope upper-limbs-guitar` plus
  new `--bridge-visible-hand-position-bones Bone_Palm_L,Bone_Palm_R` passthrough
  in `tools/gh3_midori_build_pipeline.py`; `tools/gh3_midori_acp_stage.py` now
  role-filters visible bridge palms as main=both, fret=left, strum=right.
  r51 direct capture from loose DLC passed automated load/framing for idle plus
  overlay (`failures=0`, min margin 40), but visual inspection still rejects:
  the body is upright again, yet head/upper torso are bent backward and the
  guitar/hands remain tangled/off-contact. r51 packed replay also rejects the
  hand-target contract (`fret_hand_to_left_hand=19.102769`,
  `strum_hand_to_right_hand=21.540285`). Evidence:
  `r50_r49c_full_candidate_pose_review_z40_20260820.json`,
  `r51_controlroot_stockupper_r49c_pose_review_20260820.json`, and
  `r51_controlroot_stockupper_r49c_visual_decision_20260820.json`. Loose DLC
  hashes were restored after every capture. Next work should combine the
  Control_Root/model-parent-compensated bipedal branch with a correct
  hand/guitar target-space solve; do not rely on the overlay center-ratio gate
  alone, and do not return to stock-all r50.

- 2026-08-20 r52 visible-hand-to-current-target diagnostic: added
  `tools/gh3_midori_patch_visible_hands_to_runtime_targets.py`, a focused ACP
  patcher that replays the current packed candidate, solves desired world
  positions back into the visible hand parents, and writes only
  `bone_L-hand.mesh.pos` / `bone_R-hand.mesh.pos` for the reviewed hand-overlay
  frame. Applied it to the r51 Control_Root/model-parent-compensated stage and
  rebuilt only fret/strum temp MILOs. Packed replay proves the local solve works:
  `fret_hand_to_left_hand=0.000034` and
  `strum_hand_to_right_hand=0.000025`. Direct loose-DLC capture still visually
  rejects even though automated load/framing passes. The overlay remains an
  upright but tangled pose: the visible hands can be forced onto the current
  fret/strum targets, yet the head/upper torso/guitar/target cluster is still
  wrong. Evidence:
  `r52_visiblehands_to_targets_patch_report_20260820.json`,
  `r52_visiblehands_to_targets_layer_replay_f010_20260820.json`,
  `r52_visiblehands_to_targets_pose_review_20260820.json`, and
  `r52_visiblehands_to_targets_visual_decision_20260820.json`. Decision: do not
  spend the next branch on visible hand `.pos` survival/local solving alone;
  solve the guitar plus fret/strum target frame under the Control_Root/model
  parent branch.

- 2026-08-21 r115 source-orientation apply-blend structural rejection:
  lower body / pelvis / Control_Root is treated as solved and locked to
  regression coverage only; the active failure is upper-chain silhouette and
  fret/strum arm coherence. Finished wiring
  `source_orientation_apply_blend` through
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py`,
  `tools/gh3_midori_synth_built_transform_equivalence_report.py`, and the
  direct `solve_side` tests. Also fixed the source-load guard so source rows
  load when `--source-orientation-apply-to != none` under
  `hand_orientation_mode=reference-relative`. `py_compile` passed and
  `tools/gh3_midori_pipeline_test.py` passed 126 tests. A low-priority
  report-only sweep against the five-case GLB source bridge loaded source rows
  for all five cases, but any nonzero post-solve source orientation blend broke
  left/fret endpoint contact immediately: `forearm-and-hand` blend `0.05`
  raised left max/avg contact error to `1.408984/1.255321`; `0.10` to
  `2.812145/2.507008`; `upper-forearm-hand` blend `0.05` to
  `4.174171/3.664875`. Decision: reject without visual capture. Do not reopen
  pelvis/root/lower-body diagnosis unless a new regression appears. Next
  experiment should preserve endpoint contact after any source-pose influence,
  not copy source rotations after the contact solve.

- 2026-08-21 r116 post-source contact recovery visual rejection: added
  default-off `--source-orientation-contact-refine-iterations` and
  `--source-orientation-contact-refine-strength` to
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py`, wired through
  synth reports, CLI, `tools/gh3_midori_synth_built_transform_equivalence_report.py`,
  and direct `solve_side` tests. `py_compile` passed and the pipeline suite
  passed 126 tests. Structural sweep from the current loose DLC candidate
  proved this fixes the r115 contact blow-up: `forearm-and-hand` blend `0.10`
  with 20 post-source recovery passes loaded source rows for all five cases,
  kept left contact max/avg at `0.052434/0.051664`, and changed generated arm
  quats up to `11.498885` degrees versus zero blend. A first sparse-overlay
  capture was invalid because it replaced main with a four-clip overlay-only
  clipset and produced idle-like frames. Rebuilt the proper full 266-clip
  stockattach main stage, merged the r116 overlay channels into the four review
  clips, and captured from local `GEN` plus loose `DLC`; no ISO was used and
  live hashes were restored. Built equivalence was close
  (`max_hand_delta=0.012660`, `max_distance_gap=0.012658`), but direct visual
  rejects all five full-main proofs: lower body/guitar are stable, yet the
  fretting arm remains hidden/mechanically pinned and the frames are too
  samey to read as stock-quality performance. Evidence:
  `analysis/gh3_midori_r116_contactrefine_visual_triage.json`,
  `analysis/gh3_midori_r116_full_contactrefine_visual_probe.json`, and
  `analysis/gh3_midori_r116_full_contactrefine_visual_probe_proofs/*.bmp`.
  Next branch should change the visible elbow/forearm path itself, likely from
  constrained source positions or mesh attribution, not source-rotation blend
  plus endpoint recovery.

- 2026-08-21 r117 source-position elbow-path rejection: swept existing
  `--source-elbow-hint` modes using the five-case GLB source bridge from the
  current loose DLC candidate, with source orientation copy disabled. Baseline
  no-hint left contact max/avg was `0.019064/0.018886`. The captured source
  position variant, `source-forearm-position` with
  `source_position_space=fit-stock-hand-targets` and
  `source_orientation_space=glb-target-basis`, loaded source rows for all five
  cases, changed generated arm quats up to `15.561263` degrees versus no hint,
  and kept left contact bounded at `0.563408/0.206280`. Built a valid full
  266-clip stockattach main candidate with the r117 overlay merged into the
  four review clips, captured from local `GEN` plus loose `DLC`, and restored
  live hashes. Direct visual rejects all five proofs: lower body/guitar remain
  stable, but the fretting arm is still hidden/mechanically pinned and the
  cases remain too samey. Built equivalence reports bounded but larger
  divergence (`max_hand_delta=0.581660`, `max_distance_gap=0.576327`).
  Added a focused runtime pose-mesh dump for the attack frame; summary shows
  the rendered left forearm/hand parts are present near guitar/face height.
  `midori_1_mesh0_part13.mesh` is dominated by `bone_L-foreArm.mesh`
  (`94.6875`) and `bone_L-upperArm.mesh` (`20.8125`) with positive-y
  face-band vertices, while left hand/finger parts are also present. Decision:
  reject r117. Next branch should track and tune posed bounds for these
  rendered mesh parts directly, not rely on endpoint contact/source elbow hints
  as the visual proxy. Evidence:
  `analysis/gh3_midori_r117_sourceforearm_visual_triage.json`,
  `analysis/gh3_midori_r117_meshdump_focus_report.json`,
  `analysis/gh3_midori_r117_sourceforearm_visual_probe.json`, and
  `analysis/gh3_midori_r117_sourceforearm_visual_probe_proofs/*.bmp`.

- 2026-08-21 r118 explicit elbow-vector mesh-bound rejection: lower body/root is
  solved and regression-only. Added
  `tools/gh3_midori_pose_mesh_part_report.py` for focused runtime mesh-dump
  summaries. Tested no hint, `0,12,0`, `-10,12,-4`, and `0,8,-10` explicit
  left elbow vectors on the attack frame. All four produced the same attack ACP
  hash (`4DA99D02D06DB4F0EC7AD057AC53D326C3977578381CF2926517C9355079B271`);
  captured `outup` and `forwardup` both failed visual difference from idle and
  had identical rendered mesh bounds. `midori_1_mesh0_part13.mesh` center was
  `[-7.90928, 3.24124, 53.5188]`, essentially unchanged from r117. Diagnosis:
  the left target is unreachable by the rotation-only chain (`14.788225` final
  hand-to-target, `14.773075` overreach), so the two-bone solver fully extends
  and elbow hints cannot affect silhouette. Evidence:
  `analysis/gh3_midori_r118_elbow_mesh_probe_report.json` and
  `analysis/gh3_midori_r118_latest_elbow_mesh_probe.bmp`. Capture used loose
  `GEN`/`DLC`, no ISO, and live DLC hashes were restored/matched afterward.
  Next work should make the left target reachable or switch to direct
  rendered-part/GLB-to-MILO retargeting rather than sweeping more explicit elbow
  vectors.

- 2026-08-21 r119 source-position bridge guard fixes and rejection: lower
  body/root remains solved and regression-only. Fixed
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py` so source bridge
  rows load when `--source-position-apply-to != none`, even without source
  orientation copying or elbow hints. Also fixed `fit-runtime-hand-targets` so it
  applies whenever that source position space is requested, not only under
  `hand_orientation_mode=source-guitar-local`, and added source-position world
  targets/emitted local positions to the synth report. `py_compile` passed and
  `tools/gh3_midori_pipeline_test.py` passed 126 tests. One-case attack probes
  still reject visually: `posbase` moved part13 to
  `[-7.01847, 1.38752, 56.3348]`; direct runtime source-position fit now loads
  rows but explodes (`fit_scale=76.347234`, left source hand near
  `[39.019, 40.396, 93.65]`) and was rejected before capture; stock-fit source
  position is bounded (`fit_scale=5.148025`) but fails visual with part13
  `[-7.08176, 1.63781, 56.1226]` and most left-hand mesh parts still pinned.
  Evidence: `analysis/gh3_midori_r119_source_position_bridge_report.json` and
  `analysis/gh3_midori_r119_source_position_probe.bmp`. Capture used loose
  `GEN`/`DLC`, no ISO, and live hashes were restored/matched. Next branch should
  use constrained source arm shape/direction from
  `Bone_Bicep_L`/`Bone_Forearm_L`/`Bone_Palm_L`, anchor palm to the runtime fret
  target, and clamp source scale/forearm local translation before capture.

- 2026-08-21 r120 constrained anchored source-position rejection: lower
  body/root remains solved and regression-only. Added
  `anchored-runtime-hand-targets` to
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py`, plus
  `--source-position-max-scale`, report serialization for the selected max
  scale, and a focused pipeline test. The new fit anchors each palm to the
  runtime fret/strum target and clamps source arm-shape scale. `py_compile`
  passed and `tools/gh3_midori_pipeline_test.py` passed 127 tests. One-case
  attack probes with `anchored5` and `anchored2` achieved exact left/right hand
  contact after clamping raw source scale `76.347234` to `5.0` or `2.0`, but
  failed visually as a long dangling forearm strand behind the guitar. `anchored2`
  part13 center was `[-10.4532, 9.40646, 48.3732]` with only `7` positive-y
  face-band vertices. `anchoredhand` removed that strand by applying only the
  palm anchor, but threw the forearm across the face/head area; part13 center was
  `[2.54073, 3.79444, 56.4967]`. Evidence:
  `analysis/gh3_midori_r120_constrained_anchor_report.json` and
  `analysis/gh3_midori_r120_constrained_anchor_probe.bmp`. Capture used loose
  `GEN`/`DLC`, no ISO, and live hashes were restored/matched. Next branch should
  keep the palm anchored but constrain the left forearm/upper-arm silhouette
  directly against rendered part13 bounds, steering it away from face/head and
  guitar occlusion.

- 2026-08-21 r121 explicit forearm override rejection: lower body/root remains
  solved and regression-only. Added explicit source forearm guitar-local
  overrides to `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py` so
  the anchored-palm bridge can sweep the forearm target independently. Tested
  five one-case attack variants; all hit exact hand contact and produced
  distinct ACP hashes, but the two captured variants (`fore_mid` and
  `fore_lowback`) visually reject as default-ish poses with a non-bipedal
  dangling sleeve/forearm segment. `fore_mid` part13 center was
  `[-10.2383, 9.81599, 49.2079]`; `fore_lowback` part13 center was
  `[-9.47367, 9.85317, 51.0849]`; both had only `7` positive-y face-band
  vertices. `py_compile` passed and `tools/gh3_midori_pipeline_test.py` passed
  127 tests. Evidence: `analysis/gh3_midori_r121_forearm_override_report.json`
  and latest capture `analysis/gh3_midori_r121_forearm_override_probe.bmp`.
  Capture used loose `GEN`/`DLC`, no ISO. Next branch should stop treating hand
  contact as sufficient and validate/solve against the rendered part13/upper-arm
  mesh path directly, or use a GLB-to-MILO bridge with explicit bone hierarchy
  and mesh-part checks.

- 2026-08-21 r122 source-position unit-scale diagnosis and rejection: lower
  body/root remains solved and regression-only. Added
  `tools/gh3_midori_visible_chain_bridge_report.py`, which compares retained
  source GLB/pose arm chains against the GH2 visible `.mesh` chain and records
  GLB parent nodes. The report showed the unscaled attack source arm segment
  ratios average `0.025224` of the GH2 visible bind chain, matching the
  reciprocal of the known GH3 skeleton-to-GH2 scale (`39.447833`). Added
  `--source-position-unit-scale` to
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py` and pinned it
  with a pipeline regression test. With scale `39.447833`, anchored runtime
  source positions dropped raw fit scale from `76.347234` to `1.935397` and
  achieved exact hand contact across all five synth cases. Built a one-case
  scratch candidate and captured the attack frame from loose `GEN`/`DLC`, no
  ISO. Direct visual still rejects: the arm now moves with correct magnitude
  but rotates up through the head/face. Runtime part13 center was
  `[6.25721, 4.3473, 54.3231]` with `103` positive-y face-band vertices.
  `py_compile` passed and `tools/gh3_midori_pipeline_test.py` passed 128 tests.
  Evidence: `analysis/gh3_midori_r122_visible_chain_bridge_report.json`,
  `analysis/gh3_midori_r122_scaled_source_position_report.json`, and latest
  proof `analysis/gh3_midori_r122_scaled_source_position_probe.bmp`. Next branch
  should keep scaled anchored source positioning but add a visible-chain
  bend-plane/head-exclusion constraint for part13/upper-arm.

- 2026-08-21 r123 directed bend-plane probe: lower body/root remains solved and
  regression-only. No source edits were made; used the r122 scaled anchored
  source-position controls plus explicit left forearm guitar-local overrides.
  The r122 base forearm target `[1.9907, -16.0899, 17.7256]` mapped to world
  `[1.5018, 5.5807, 58.0163]` and hit the head. Report-only overrides all kept
  exact hand contact. Captured `chest_low` (`[-3.5, -6.0, 18.7]`) and
  `lowback_old` (`[-5.1, -2.2, 18.4]`) from loose `GEN`/`DLC`, no ISO.
  `chest_low` eliminated face-band overlap (`part13` center
  `[-1.30219, 6.02509, 48.5576]`, positive-y face vertices `0`) but looked less
  readable. `lowback_old` was the best r123 silhouette and is retained as the
  latest proof, but still visually rejects because the fretting hand/wrist is
  detached/curled at the neck. Evidence:
  `analysis/gh3_midori_r123_bend_plane_report.json` and
  `analysis/gh3_midori_r123_bend_plane_probe.bmp`. Next branch should preserve
  the low/back forearm plane and target visible hand/wrist orientation/finger
  pose at the neck.
