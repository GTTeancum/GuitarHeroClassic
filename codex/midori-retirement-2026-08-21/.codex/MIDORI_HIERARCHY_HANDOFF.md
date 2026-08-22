# Midori hierarchy retarget handoff

## Goal

Finish GH3 PS2 Midori as ordinary ark-external Guitar Hero Classic DLC: both
main outfits, all required animations, stock-quality scale/skin/hair alpha,
guitar and hands, no Midori-specific runtime code. The goal is not complete.
Visual approval must come from direct sequential inspection in Guitar Hero
Classic. Diagnose in hierarchy order and verify each tier before adding the
next: pelvis; thighs; knees; ankles; toes; spine; chest; neck/head; arms;
hands/guitar/controllers.

Do not block the active goal unless the user explicitly instructs that again.
The earlier "block at step 4/4" pause request is stale.

## 2026-08-21 retirement pause: r181-r182 arm findings

- User requested commit/push and retirement of this effort until a future LLM
  model. The active goal remains incomplete and should not be marked complete.
- No ISO was mounted or used. All r181/r182 testing used loose-DLC staging and
  the wrapper restored/deleted staged files afterward.
- Patched `tools/gh3_midori_build_fullclip_coupled_contact_candidate.py` so
  builder subprocesses use `IDLE_PRIORITY_CLASS`, matching the user's low-CPU
  priority request more strictly.
- Added `--skip-contact-patches` to both fullclip builders. This leaves
  `bone_fret_hand.mesh.pos`, `bone_pos_guitar.mesh.pos`,
  `bone_strum_hand.mesh.pos`, and `bone_pos_guitar.mesh.quat` unchanged for
  arm-only diagnostics. This is important because guitar attachment is supposed
  to remain after pelvis/head/arms are solved.
- Built non-live r181 outfit-1 attack candidate:
  `analysis/gh3_midori_gh2_milos/gh3_midori_1_main_staticface_attack_r181.milo_ps2`
  SHA256 `9CD65B3D781331B3DAEC7D616B9C591E8A90A14A63612019071458474FE4F105`.
  It proved the per-case `main_milo` review/staging path loads correctly.
- Captured r181 one-case no-face screenshot:
  `analysis/gh3_midori_r181_outfit1_attack_proofs/midori_1_attack_left_r181_f030.bmp`.
  Visual decision:
  `analysis/gh3_midori_r181_outfit1_staticface_attack_visual_decision.json`.
  Verdict: reject. Lower body remains bipedal, but the left arm rises through
  the head/face region and the guitar body still cuts across the torso.
- Built non-live r182 outfit-1 arm-only attack candidate:
  `analysis/gh3_midori_gh2_milos/gh3_midori_1_main_staticface_armonly_attack_r182.milo_ps2`
  SHA256 `BB05A8641909AC3BF70B9F35BBB3FE1058BA89C8469E0915878DCD270977B403`.
  It was not captured or visually approved before retirement. Status artifact:
  `analysis/gh3_midori_r182_outfit1_staticface_armonly_attack_status.json`.
- Next resume point: use r182's arm-only build mode for a direct visual capture,
  then adjust visible upper-arm/forearm/clavicle objective if the arm still
  goes through head/face. Do not broaden to outfit 2, jump/solo/transition,
  hands/fingers, or guitar attachment until outfit-1 attack is visually
  bipedal and coherent.
- Git blocker at retirement: `C:\Programming\GitHub\Guitar Hero II\.git` is an
  empty directory with no `HEAD` or `config`; `git rev-parse --show-toplevel`
  fails at the workspace root. Nested repos exist, but they do not own the
  Midori `.codex`, `tools`, and `analysis` findings. Commit/push cannot
  proceed from this workspace until repository metadata or a remote is
  restored/provided.

## 2026-08-21 r180 static-face arm tooling pivot

- User rejected the face crop sheet: when face calls change, the visible face
  distorts in the wrong direction. Freeze face for now. Do not spend the next
  iteration on face crops or viseme matching; do not include `face_clip` in the
  arm diagnostic cases unless explicitly resuming face work.
- Current phase is arms only from the already-accepted torso/head chain
  outward. Guitar attachment remains later; hands/fingers follow the broader
  arm solve.
- No r180 MILO was built or promoted. Live DLC remains unchanged.
- Patched `tools/gh3_midori_pose_review.py` so custom case JSON may select
  per-case `main_milo`, `strum_milo`, `fret_milo`, and `face_milo` paths while
  preserving the previous shared-bank defaults.
- Patched `tools/gh3_midori_capture_with_loose_dlc_backup.py` with
  `--extra-candidate-file SOURCE_NAME=DLC_REL_PATH` so a capture can stage
  extra outfit-specific animation banks into loose DLC and automatically
  restore or delete them afterward.
- Patched both fullclip builders with repeatable `--case-name` filters. This
  lets the next arm candidate build outfit-/pose-specific banks without
  hand-editing solve reports and without disabling duplicate-clip conflict
  errors.
- Runtime/manifest check: ordinary addon variants carry independent
  `main_anim` paths, and gameplay receives `variant.main_anim_path`. Therefore
  outfit-specific main animation banks are expressible in the existing DLC
  model if authored/staged under distinct paths.
- Local ihatecompvir audit remains:
  `tools/gh3_midori_ihatecompvir_bridge_audit.py`. It says MiloLib/GH2 notes
  are useful references for GH2 PS2 `CharClipSamples`, but public glTFMilo is
  not a drop-in final GH2 PS2 converter. GLB is still acceptable as an
  automated intermediate if it feeds our final GH2 writer/validator.
- Retained decision artifact:
  `analysis/gh3_midori_r180_staticface_arm_tooling_decision.json`.
- A non-live r181 outfit-1 arm-bank build was started with the new
  `--case-name` filter and then interrupted during the slow MILO replace stage
  because of the user's lag concern. No r181 MILO output was retained; the
  temporary ACP scratch was deleted. The failed first command also confirmed
  that this branch must pass the pinned r176 `--source-bridge` path rather than
  the builder default.
- Next arm branch: build or duplicate static-face main banks into distinct
  outfit-specific filenames, stage them with `--extra-candidate-file`, run a
  small no-face arm capture (`attack`, `jump`, `solo`, `transition`), and
  reject immediately if any frame is non-bipedal, neck/head is misplaced, or
  arms collapse into body/guitar.

## 2026-08-21 r179 shared-attack upper-chain diagnostics

- No r179 MILO was built or promoted. Face remains static/no-op and live DLC
  remains unchanged.
- Exposed `--solve-visible-clavicles`, `--visible-clavicle-blend`, and
  hand-rotation toggles in `tools/gh3_midori_score_visible_forearm_planes.py`
  so render-feedback scoring can test currently-supported upper-chain channel
  leverage.
- A broad mode sweep was stopped for being too slow. Retained compact report:
  `analysis/gh3_midori_r179_shared_attack_upperchain_compact_report.json`.
  It tested five modes over the key shared attack vectors: forearm-only,
  clavicle blend 0.5, clavicle blend 1.0, clavicle+target-hand rotation, and
  clavicle+axis-hand rotation. All modes selected the same shared vector and
  same bad max score `53.3016`.
- Retained duplicate-signature report:
  `analysis/gh3_midori_r179_shared_attack_upperchain_duplicate_signature_report.json`.
  With one shared vector, both outfit attack rows emit identical patch
  signatures even with clavicle/hand channels enabled. So the problem is not
  duplicate ACP conflict; the same emitted pose renders differently on the two
  outfit meshes.
- Metadata inspection of current tools shows the review/build path uses one
  `gh3_midori_main.milo_ps2` clipset for both `gh3_midori_1` and
  `gh3_midori_2`, and both character MILOs expose the same visible arm
  skeleton names. Under current ordinary DLC/tooling,
  `gh3_guit_mido_a_attackl` is a shared animation.
- Decision artifact:
  `analysis/gh3_midori_r179_upperchain_shared_attack_decision.json`.
- Next branch: build a true shared-clip objective that evaluates actual
  rendered captures for both outfits, or investigate whether separate
  outfit-specific main clipsets can be authored in MILO. Do not continue simple
  forearm-only or existing clavicle-toggle sweeps for `attackl`.

## 2026-08-21 r177-r178 render-aware arm scorer diagnostics

- No r177/r178 MILO was built or promoted. Face remains static/no-op and live
  DLC remains unchanged.
- Patched `tools/gh3_midori_score_visible_forearm_planes.py` to consume r176
  render feedback via `--render-feedback-candidate-report` and
  `--render-feedback-meshpart-summary`. It predicts rendered part13 center
  using captured part13-minus-elbow offsets, then scores predicted part13
  height/front/far placement as well as joint target distance.
- Added sparse render-feedback local-vector grid candidates around the observed
  r176 vector and retained r123 seeds. Added shared-clip conflict resolution so
  duplicate main clips pick one vector by minimax score instead of producing
  incompatible per-case map entries.
- Patched fullclip and seed-ACP builders to default to
  `--duplicate-clip-policy error` for conflicting duplicate clip patches. Use
  `first`/`last` only for explicit diagnostics.
- Retained r177 shared report/map:
  `analysis/gh3_midori_r177_visible_forearm_renderfeedback_grid_shared_score_report.json`
  and
  `analysis/gh3_midori_r177_visible_forearm_renderfeedback_grid_shared_override_map.json`.
  The shared `gh3_guit_mido_a_attackl` clip must serve both outfit attack
  cases; its minimax vector still scored `53.3016`, so r177 was not built.
- Ran focused r178 shared-attack grid over the current forearm-vector family.
  Retained:
  `analysis/gh3_midori_r178_shared_attack_forearm_grid_report.json` and
  `analysis/gh3_midori_r178_shared_attack_forearm_grid_decision.json`.
  Best shared attack vector was `[-1.1737,-5.1629,-4.8704]`, still max score
  `53.3016`.
- Next branch: stop treating attack as independent per-outfit forearm
  placement. Solve `gh3_guit_mido_a_attackl` as one shared clip objective
  across both outfits, likely requiring upper/clavicle/hand rotation channels
  or confirming whether separate outfit-specific main animation assets can be
  authored in MILO.

## 2026-08-21 r176 per-case forearm-plane capture rejected

- Captured r176 via temporary loose-DLC substitution only; no ISO was mounted
  or used. The wrapper restored live DLC afterward, and live main hashes remain
  the accepted static-face SHA256
  `2902C712F81ED32A8D7D14E147453F16196C7AE935E5F3D029C179648A36AC8B`.
- Retained sheet:
  `analysis/gh3_midori_r176_staticface_arm_percase_forearm_sheet.jpg`.
  Sheet manifest:
  `analysis/gh3_midori_r176_staticface_arm_percase_forearm_sheet_manifest.json`.
  Pose-review manifest copy:
  `analysis/gh3_midori_r176_staticface_arm_percase_forearm_pose_review_manifest.json`.
  Decision artifact:
  `analysis/gh3_midori_r176_staticface_arm_percase_forearm_visual_decision.json`.
- Visual verdict: reject. `midori_1_attack_left_f030` still folds the left arm
  across/into the guitar-body area; `fast_jump`/`fast_solo` are upright but not
  solved performance-arm poses; `transition_out` occludes/replaces the arm
  silhouette with the guitar; `midori_2_attack_left_f030` is visibly
  non-bipedal/tilted.
- Mesh summary:
  `analysis/gh3_midori_r176_staticface_arm_percase_forearm_meshpart_summary.json`.
  Part13 confirms why the solver-only r175 score was misleading:
  `midori_1_attack_left_f030` part13 center `[8.19383,2.48217,56.6356]` with
  `59` positive-y face-band vertices, and `midori_1_fast_jump_f040` part13
  center `[-10.9078,3.43151,55.4187]` with `44` positive-y face-band vertices.
- Next branch: make the forearm-plane scorer capture/render-aware. Candidate
  selection must use rendered part13/hand mesh bounds and occlusion, not only
  solved joint world targets, before building r177.

## 2026-08-21 r175-r176 per-case forearm-plane arm candidate

- Face remains static/no-op for this phase. Current priority is visible
  arms/wrists/hands/fingers from the accepted torso/head chain outward; guitar
  attachment remains last.
- Added `--visible-arm-forearm-override-map` to both fullclip builders. The map
  is keyed by diagnostic case name and lets each case use its own visible
  forearm guitar-local target.
- Added `tools/gh3_midori_score_visible_forearm_planes.py`. It runs a
  report-only solver screen, treats rejected `r122_base` as reference-only, and
  generates per-case fitted left-forearm vectors targeting hand-relative safe
  height before any MILO build.
- Retained:
  `analysis/gh3_midori_r175_visible_forearm_plane_score_report.json` and
  `analysis/gh3_midori_r175_visible_forearm_override_map.json`.
  Selected left vectors are:
  `attack`/`midori_2_attack` `[-1.1737,-5.1629,-10.8704]`,
  `fast_jump` `[4.8223,-7.7020,7.4480]`,
  `fast_solo` `[6.6046,-4.0225,11.7787]`,
  `transition_out` `[-5.1,-2.2,18.4]`.
- Built non-live candidate:
  `analysis/gh3_midori_gh2_milos/gh3_midori_main_staticface_arm_percase_forearm_r176.milo_ps2`.
  SHA256 `26917D91D9E4A1511622D2B4B5210F4E7ECABD50D3B168AF80EF36CEA78380C0`,
  size `20234418`. Candidate report:
  `analysis/gh3_midori_r176_staticface_arm_percase_forearm_candidate_report.json`.
- r176 has not been captured or visually reviewed. Next step is a loose-DLC
  capture sheet substituting only this r176 main MILO, then direct rejection or
  promotion based on rendered bipedal arm silhouette. No ISO was mounted or
  used; build work ran below-normal priority.

## 2026-08-21 r174 static-face arm objective

- User rejected continued face crop work. Face is intentionally static/no-op
  for now; later face work should use GH2-sized translation/control mapping,
  not direct GH3 facial control parity.
- Current hierarchy priority: keep pelvis/root through head regression-only,
  solve visible arms/wrists/hands/fingers next, and leave guitar attachment
  until after the arms/hands read as bipedal.
- Added opt-in automated fullclip controls:
  `--visible-arm-left-forearm-guitar-local` and
  `--visible-arm-right-forearm-guitar-local` in both fullclip and seed-ACP
  builders. This brings the r123 low/back forearm-plane lever into the current
  static-face fullclip pipeline.
- Compile check passed for the two builders plus
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`.
- A full r174 candidate build was attempted at low priority but did not produce
  ACP/donor/MILO/report artifacts; zero-byte logs were cleaned. No ISO was
  mounted or used, and live DLC was unchanged.
- Retained report-only artifact:
  `analysis/gh3_midori_r174_staticface_arm_lowback_objective_report.json`.
  It runs the current five diagnostic cases with r172 fitted source-palm hand
  targets, source-pose elbow hints, and r123 `lowback_old`
  `--visible-arm-left-forearm-guitar-local -5.1,-2.2,18.4`.
- Diagnosis: the r123 global lowback vector is now reproducible in the
  fullclip solver, but it is not obviously safe per case. `fast_jump` and
  `fast_solo` drive the left forearm high/back again. Next branch should derive
  per-case forearm-plane candidates from rendered part13/hand bounds, then
  build/capture only the candidate that clears the face/head and preserves a
  bipedal arm silhouette.

## 2026-08-21 r172-r173 fit-stock source-palm diagnostics

- Commands ran at below-normal/low priority. No ISO was mounted or used.
  Captures staged temporary loose DLC and substituted only the tested main MILO.
  Face remains static/no-op by design.
- Fixed source bridge compatibility in
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`: bridge readers now match
  either `bone` or `source_bone`, needed by merged hand/guitar bridge records.
- Added
  `--visible-arm-target-mode source-palm-fit-stock-hand-targets-per-case` and
  `--visible-arm-source-coordinate-map` to the full-clip and seed-ACP builders.
  The mode maps per-case source IK-helper hand spacing onto GH2 stock
  fret/strum hand target locals, then transforms source palm positions through
  that fitted frame.
- Fit preview:
  `analysis/gh3_midori_r172_fitstock_sourcepalm_target_preview.json`. Fitted
  offsets were small and deterministic; scale was `5.148` for all five cases.
- r172 used the merged hand/guitar bridge root,
  `palm-fit-negx-y-negz`, source-pose elbow hints, and visible-axis hand
  calibration. It was contract-green for the five patched diagnostic cases but
  visually rejected.
- r173 repeated r172 with the `direct` coordinate map. Patch values and final
  MILO were byte-identical to r172, so no duplicate capture was kept.
- Latest retained sheet:
  `analysis/gh3_midori_r172_staticface_fixedliveguitar_fitstock_sourcepalm_sourceelbow_sheet.jpg`.
  Decision artifact:
  `analysis/gh3_midori_r172_r173_staticface_fitstock_sourcepalm_decision.json`.
- Next branch: use rendered mesh part bounds and visual silhouette objectives
  for visible arms rather than only endpoint/contact target fitting. Earlier
  mesh-dump evidence around r117 showed the left forearm/hand parts are present
  but not forming a readable silhouette, so optimize the arm path against those
  rendered parts.

## 2026-08-21 r168-r171 source-derived arm diagnostics

- Commands ran at below-normal/low priority. No ISO was mounted or used.
  Captures staged temporary loose DLC and substituted only the tested main MILO.
  Face remains static/no-op by design.
- Added `--visible-arm-elbow-hint-mode source-pose-per-case` to the full-clip
  and seed-ACP builders. The mode maps per-case source bicep-to-forearm pose
  offsets into the GH2 visible shoulder frame, then uses that as the two-bone
  IK bend-plane hint.
- r168 used the existing explicit guitar-local grip map plus source-pose elbow
  hints. It stayed contract-green for the five patched diagnostic cases but
  remained visually rejected.
- r169 used source-palm targets plus source-pose elbow hints. It produced more
  natural/non-stiff arm movement, but the hands moved away from guitar contact.
  This is useful diagnostic evidence but not promotable.
- Derived
  `analysis/gh3_midori_explicit_guitar_grip_map_r170_hybrid_sourcepalm65contact.json`
  by blending r169 source-palm targets 35% with r168 explicit contact targets
  65%, then converting the blended worlds to per-case guitar-local offsets.
- r170 used that hybrid grip map with source-pose elbow hints. r171 added 50%
  clavicle solve. Both stayed contract-green for the five patched cases and
  both were visually rejected.
- Latest retained sheet:
  `analysis/gh3_midori_r171_staticface_fixedliveguitar_hybrid_sourcepalm65contact_sourceelbow_clav50_sheet.jpg`.
  Useful comparison sheet:
  `analysis/gh3_midori_r169_staticface_fixedliveguitar_sourcepalm_sourceelbow_sheet.jpg`.
  Decision artifact:
  `analysis/gh3_midori_r168_r171_staticface_sourcearm_decision.json`.
- Next branch: keep source-pose elbow mode and the hybrid-map derivation code
  path, but fit source palm/IK-helper hand positions into GH2 guitar space from
  source guitar-neck/body landmarks instead of blending against rejected
  explicit grip offsets.

## 2026-08-21 r166-r167 elbow-hint diagnostics

- Commands ran at below-normal/low priority. No ISO was mounted or used.
  Captures staged temporary loose DLC and substituted only the tested main MILO.
  Face remains static/no-op by design.
- Fixed a real visible-arm IK bug in
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`:
  `solve_two_bone_position_chain` now uses the requested elbow hint to compute
  bend direction. Before this, `down-out` projected the hint along the
  shoulder-target aim but still used the current elbow for the bend vector.
- r166 rebuilt the r165 explicit grip-map setup with the corrected elbow hint
  solver and mild down/out hints. r167 used the r166 ACP seed and a stronger
  up/out elbow plane (`side=12`, `down=-8`). Both stayed contract-green for the
  five patched diagnostic cases but were visually rejected.
- Latest retained sheet:
  `analysis/gh3_midori_r167_staticface_fixedliveguitar_elbowupout_axisgrip_sheet.jpg`.
  Decision artifact:
  `analysis/gh3_midori_r166_r167_staticface_elbowhint_decision.json`.
- Next branch: keep the elbow-hint fix, but do not keep tuning elbow plane in
  isolation. Derive per-case hand targets and elbow planes together from
  source/reference geometry, or first run an exaggerated visual sensitivity
  probe for visible arm channels to rule out runtime override/suppression.

## 2026-08-21 r164-r165 explicit guitar-local grip diagnostics

- Commands ran at below-normal/low priority. No ISO was mounted or used.
  Captures staged a temporary loose-DLC add-ons tree and substituted only the
  tested main MILO. Face remains static/no-op by design for now.
- Added `--visible-arm-target-mode explicit-guitar-local-grip` and
  `--visible-arm-grip-map` to the full-clip builder and seed-ACP builder. Grip
  maps define fret/strum hand targets as guitar-local row-vector offsets plus
  `current` or `swapped` visible-hand role per case.
- Added diagnostic maps:
  `analysis/gh3_midori_explicit_guitar_grip_map_r164.json` and
  `analysis/gh3_midori_explicit_guitar_grip_map_r165.json`. Both shorten the
  fretting target from the older proxy `z=22.41567` to `z=16.0`.
- r164 used attack current and jump/solo/transition swapped. r165 used all
  reviewed cases swapped. Both stayed contract-green but were visually rejected.
  The shortened grip slightly reduces overextension in jump/solo, but the
  posture remains stiff and attack/transition still look wrong.
- Latest retained sheet:
  `analysis/gh3_midori_r165_staticface_fixedliveguitar_explicitgrip_all_swapped_axisgrip_sheet.jpg`.
  Decision artifact:
  `analysis/gh3_midori_r164_r165_staticface_explicitgrip_decision.json`.
- Next branch: keep explicit grip-map support, but derive per-case grip offsets
  from source geometry or stock-performance reference frames. Hand-tuned
  shortening of the old proxy points is not enough.

## 2026-08-21 r162-r163 calibrated hand-axis diagnostics

- Commands ran at below-normal/low priority. No ISO was mounted or used.
  Captures staged a temporary loose-DLC add-ons tree and substituted only the
  tested main MILO. Face remains static/no-op by design for now.
- Added `--visible-hand-rotation-mode visible-axis-calibration` to the
  full-clip builder and seed-ACP builder. It consumes
  `analysis/gh3_midori_visible_hand_axis_calibration_r68.json` and orients each
  visible hand by matching calibrated hand-local finger/palm axes to calibrated
  guitar-local finger/palm axes in the current guitar frame. Optional
  guitar-local grip axis bias flags are available.
- r162 rebuilt the r161-style mapped jump/solo target roles with aim rotations,
  down/out elbow hints, and calibrated guitar-local hand-axis quats instead of
  proxy-derived hand quats. Contract stayed green but the visual pose was
  essentially unchanged from r161 and rejected.
- r163 additionally swapped transition target roles using the fast seed path.
  Contract stayed green, but transition/attack remained visibly wrong and
  jump/solo were still only partially guitar-like. Rejected.
- Latest retained sheet:
  `analysis/gh3_midori_r163_staticface_fixedliveguitar_mapped_jump_solo_transition_aim_elbow_axisgrip_sheet.jpg`.
  Decision artifact:
  `analysis/gh3_midori_r162_r163_staticface_axisgrip_decision.json`.
- Next branch: hand orientation alone is not enough. Define explicit
  per-case/per-clip guitar-local grip positions and wrist orientations, then
  solve visible hands and target proxies from those grip definitions together.

## 2026-08-21 r160-r161 mapped target + hand-quat diagnostics

- Commands ran at below-normal/low priority. No ISO was mounted or used.
  Captures staged a temporary loose-DLC add-ons tree and substituted only the
  tested main MILO. Face remains static/no-op by design for now.
- Replaced `tools/gh3_midori_patch_acp_constant_channel.py` with an ACP
  parser/repacker that supports adding missing constant channels through
  `--allow-add`. The seed-ACP builder exposes this as `--allow-add-channels`.
  This closes the r156 blocker where the seed lacked `bone_L/R-hand.mesh.quat`.
- Added `--visible-arm-target-mode mapped-current-proxies` and repeated
  `--visible-arm-target-swap-case` so target role swaps can be selected per
  case/clip.
- r160 did a full rebuild with visible hand quat channels, mapped swaps for
  `midori_1_fast_jump_f040` and `midori_1_fast_solo_f090`, 25% source-pose
  rotation blend, and down/out elbow hints. Contract stayed green, but visual
  review still rejected.
- r161 reused the r160 hand-quat ACP seed and removed source-pose rotation blend
  (aim-only rotations) while keeping mapped jump/solo swaps and hand quats.
  Contract stayed green. Visual review is slightly cleaner than r160 but still
  rejected: jump/solo are the most guitar-like in this line, while
  attack/transition remain wrong and the arms still read stiff.
- Latest retained sheet:
  `analysis/gh3_midori_r161_staticface_fixedliveguitar_mapped_jumpsolo_aim_elbow_handrot_sheet.jpg`.
  Decision artifact:
  `analysis/gh3_midori_r160_r161_staticface_mapped_handquat_decision.json`.
- Next branch: use mapped-role infrastructure, but move to explicit per-case or
  per-clip guitar-local grip/hand-orientation definitions. Do not treat the
  jump/solo swap as a global solution.

## 2026-08-21 r154-r159 elbow/rotation/swap arm diagnostics

- Commands ran at below-normal/low priority. No ISO was mounted or used.
  Captures staged a temporary loose-DLC add-ons tree and substituted only the
  tested main MILO. Face remains static/no-op by design for now.
- Extended `tools/gh3_midori_build_fullclip_coupled_contact_candidate.py` and
  `tools/gh3_midori_build_fullclip_candidate_from_seed_acp.py` with:
  `--visible-arm-elbow-hint-mode down-out`,
  `--visible-arm-elbow-side-offset`,
  `--visible-arm-elbow-down-offset`,
  `--visible-arm-source-rotation-blend-with-aim`, and
  `--visible-arm-target-mode swapped-current-proxies`.
- r154 added the down/out elbow hint to the r153 source-palm anim blend. It
  stayed contract-green but did not visually improve the pose; elbow plane alone
  is not enough.
- r155/r157 blended per-case source-pose rotations into the aim solve at
  25%/50% using current proxy targets. Both stayed contract-green but remained
  stiff/incorrect.
- r156 tried enabling visible hand rotations through the fast seed path, but the
  seed ACPs did not contain `bone_L/R-hand.mesh.quat`; adding channels still
  requires a full repack path or an ACP add-channel tool.
- r158 swapped current proxy target roles and r159 combined swapped targets with
  25% source-pose rotation blend. Both stayed contract-green. r159 is the best
  diagnostic in this set because jump/solo are more guitar-like, but attack and
  transition are still visibly wrong. Do not promote.
- Latest retained sheet:
  `analysis/gh3_midori_r159_staticface_fixedliveguitar_swappedtargets_sourceposeblend025_elbow_sheet.jpg`.
  Decision artifact:
  `analysis/gh3_midori_r154_r159_staticface_arm_swap_rotation_decision.json`.
- Next branch: treat left/right target role mapping as per-case/per-clip data,
  not a global toggle. Add hand-quat channels to the seed/repack path and solve
  visible hand orientation plus reachable guitar-local grip together, then align
  target proxies after the visible chain.

## 2026-08-21 r148-r153 source-palm arm-target diagnostics

- Commands ran at below-normal/low priority. No ISO was mounted or used.
  Captures staged a temporary loose-DLC add-ons tree and substituted only the
  tested main MILO. Face remains static/no-op by design for now.
- Extended `tools/gh3_midori_build_fullclip_coupled_contact_candidate.py` with
  per-case source-palm visible-arm targets and
  `--visible-arm-target-blend-with-current`. The target mode computes
  `Bone_Palm_L/R` offsets relative to source `Bone_Chest` from the bridge
  `pose` matrices and reapplies them from target `bone_spine3.mesh`.
- Added `tools/gh3_midori_patch_acp_constant_channel.py` and
  `tools/gh3_midori_build_fullclip_candidate_from_seed_acp.py`. Use the seed
  builder for the next arm iterations; it reuses an expanded ACP directory and
  overwrites only patch channels, avoiding the slow frame-by-frame full-clip
  sampler.
- r148 showed full helper-basis source-palm targets are overextended and also
  used the older vertical-guitar solve. r149 patched fixed-live guitar after
  the fact, proving the visual issue but breaking the contract because proxies
  were not recomputed under that guitar frame.
- r150 helper basis blend `0.35`, r151 helper basis blend `0.15`, r152 direct
  basis blend `0.35`, and r153 anim basis blend `0.35` all kept packed HMX
  contract gaps at zero with fixed/non-vertical guitar. All are visually
  rejected: arms remain stiff, side-pointing, hidden, or crossing the guitar/head
  rather than forming a natural grip. r153 has zero average guitar offset and is
  the least-bad source-palm basis check, but still not acceptable.
- Latest retained sheet:
  `analysis/gh3_midori_r153_staticface_fixedliveguitar_sourcepalm_anim_blend035_sheet.jpg`.
  Next branch should abandon raw source-palm targets as the main driver and use
  a constrained elbow/hand comfort solve around reachable guitar-local grip
  points, then align target proxies after the visible arm chain.

## 2026-08-21 r144-r147 per-case source-pose arm diagnostics

- Commands ran at below-normal/low priority. No ISO was mounted or used.
- Found existing per-case source bridge JSON/GLB evidence under
  `.codex/current-evidence/midori-review-source-bridges-fresh-targetlength-20260818-5case`.
  These files cover the reviewed frames for attack, jump, solo, transition, and
  idle and include biceps/forearms/palms/collars.
- Extended `tools/gh3_midori_build_fullclip_coupled_contact_candidate.py` with
  `source-per-case` and `source-pose-per-case` visible arm rotation modes. r144
  proved the old `matrix_local` source route is static by producing the exact
  same hash as rejected r143. r145-r147 use pose-parent local rotations from
  per-case bridge `pose` matrices and test `direct`, `anim`, and `helper`
  bases.
- r145-r147 kept zero HMX contract gaps and non-vertical guitar silhouettes,
  but all failed direct visual review. The source arm rotations now vary by
  frame, but substituting those rotations alone still does not make a natural
  visible guitar grip.
- Decision artifact:
  `analysis/gh3_midori_r144_r147_staticface_percase_source_pose_arm_decision.json`.
  Latest retained sheet:
  `analysis/gh3_midori_r147_staticface_fullclip_fixedguitar_percase_posearmrot_helper_sheet.jpg`.
- Next branch: keep source-pose-per-case extraction, but invert the problem.
  Derive hand/guitar comfort targets from source pose palms or solve the guitar
  toward source visible hand positions/orientations, then update target proxies
  around that guitar pose.

## 2026-08-21 r141-r143 clavicle/source-arm diagnostics

- Commands ran at below-normal/low priority. No ISO was mounted or used.
- Extended `tools/gh3_midori_build_fullclip_coupled_contact_candidate.py` with
  optional visible clavicle solving, `--visible-clavicle-blend`, and
  `--visible-arm-rotation-mode source`. Source mode uses the existing attack
  bridge frame data; available source bridge frames are only `0,15,30,45`, so
  this is not full per-animation coverage.
- r141 full clavicle aim, r142 half clavicle blend, and r143 source frame-30
  arm rotations all kept packed HMX contract gaps at zero and avoided vertical
  guitar silhouettes, but all were rejected visually. r141 overdrives
  shoulders/arms; r142 damps the useful r141 change without improving grip;
  r143 does not generalize and produces wrong arm behavior.
- Decision artifact:
  `analysis/gh3_midori_r141_r143_staticface_arm_branch_decision.json`.
  Latest retained sheet:
  `analysis/gh3_midori_r143_staticface_fullclip_fixedguitar_sourcearmrot_sheet.jpg`.
- Next branch: keep fixed live r133 guitar orientation and full-clip donors, but
  stop grinding static arm rotations. Either obtain/use per-frame or per-clip
  source/GLB arm pose data, or invert the comfort solve so guitar/target points
  move toward reachable natural arm poses instead of forcing visible arms into
  fixed guitar target points.

## 2026-08-21 r136-r140 static-face arms branch

- Commands ran at below-normal/low priority. No ISO was mounted or used.
- Extended `tools/gh3_midori_build_fullclip_coupled_contact_candidate.py` with
  revision labeling and optional visible-arm solving. The builder now preserves
  all selected current r133 main clip channels, then can add fixed guitar,
  fret/strum proxy, visible upper/forearm quat, visible forearm/hand pos, and
  visible hand quat channels in the same full-clip donor path.
- r136 used the `+90` coupled twist report. Packed HMX contract stayed green
  (`LH-FRET=0.000`, `RH-STRUM=0.000`, `SOLVE-DELTA=0.000`), but visual capture
  still rejected: fast-jump and fast-solo were vertical guitar failures.
- r137 used a fixed guitar local rotation from the live r133
  `bone_pos_guitar.mesh.quat` sample
  (`-0.1385,-0.61892,-0.507771,0.58303`, HMX interpreted). This removed the
  vertical guitar silhouettes and kept zero contract gaps, but visible arms were
  not solved to the guitar.
- r138 added visible arm rotations and visible forearm/hand position channels.
  This is the current best branch direction: non-vertical guitar and visible
  arms move toward the instrument. It is still rejected visually because the
  arms remain stiff/warped and not a natural grip.
- r139 changed arm reach scale from `0.94` to `1.0`; r140 added visible hand
  quats matching target worlds. Both were visually indistinguishable from r138.
- Decision artifact:
  `analysis/gh3_midori_r136_r140_staticface_arm_branch_decision.json`.
  Latest retained sheet:
  `analysis/gh3_midori_r140_staticface_fullclip_fixedguitar_visiblearms_handrot_sheet.jpg`.
  Better representative sheet:
  `analysis/gh3_midori_r138_staticface_fullclip_fixedguitar_visiblearms_sheet.jpg`.
- Next branch: keep fixed live r133 guitar local orientation and the full-clip
  donor path; add elbow-hint/clavicle/source-arm orientation before solving arm
  positions. Also cache full clip samples because repeated r136-r140 builds are
  dominated by frame-by-frame `sample-clip` calls.

## 2026-08-21 r135 full-clip contact probe

- Commands ran at below-normal/low priority. No ISO was mounted or used.
- r134 was diagnosed as methodologically flawed: sparse guitar-only donor clips
  replaced full main clips and dropped the body channels. Added
  `tools/gh3_midori_build_fullclip_coupled_contact_candidate.py`, which samples
  the current live r133 static-face main MILO and reconstructs full selected
  donor clips before adding coupled guitar/contact target channels.
- Built unpromoted r135
  `analysis/gh3_midori_gh2_milos/gh3_midori_main_staticface_fullclip_contact_r135.milo_ps2`
  (SHA256
  `9D35E0D2B36473AC962EA0BA8C62F9A324AAD7DB55E0FE8A0FD6417C62A354B4`,
  size `20236639`). The rejected candidate MILO/donor ACP scratch was cleaned
  after compact reports and the contact sheet were retained. The HMX contract
  is perfectly green for the five main-only cases: `LH-FRET=0.000`,
  `RH-STRUM=0.000`, and
  `SOLVE-DELTA=0.000` in
  `analysis/gh3_midori_r135_staticface_fullclip_contact_contract_report.json`.
- Native five-frame capture still rejects. Contact sheet:
  `analysis/gh3_midori_r135_staticface_fullclip_contact_sheet.jpg`.
  Silhouette report:
  `analysis/gh3_midori_r135_staticface_fullclip_contact_guitar_silhouette_report.json`.
  Fast-jump and fast-solo are vertical guitar failures (`72.004` and `86.92`
  degrees). Attack and outfit-2 attack are bipedal/diagonal; transition is
  bipedal but still not a natural grip.
- Decision artifact:
  `analysis/gh3_midori_r135_staticface_fullclip_contact_decision.json`.
  Do not promote r135.
- Report-only twist checks at `-90` and `+90` degrees preserve the same coupled
  residuals (`max_residual_error=4.958501`, mean `2.577198`). Next branch:
  keep the r135 full-clip donor path, but constrain the coupled solve with a
  stock-attach/diagonal guitar roll prior so jump/solo avoid vertical side-bar
  orientation while retaining the zero HMX contact contract.

## 2026-08-21 r133 static face + r134 arm/guitar probe

- Commands ran at below-normal/low priority. No ISO was mounted or used.
- User rejected the animated face crop sheets because the frames where the face
  changed distorted in the wrong direction. Current policy is to freeze GH2
  face calls so the branch can move on to arms.
- Built r133 static/no-op face main:
  `analysis/gh3_midori_gh2_milos/gh3_midori_main_face_static_r133.milo_ps2`.
  It materializes 14 GH2 face-call clips, including `EyesClosed`, as
  zero-channel no-ops. Static validator passed with `static_calls=14`,
  `errors=0`; live clips inspect with `sample_bytes=0`.
- Promoted r133 to analysis, source-tree DLC, and hybrid loose DLC. Current
  main hash in all three locations:
  `2902C712F81ED32A8D7D14E147453F16196C7AE935E5F3D029C179648A36AC8B`, size
  `20235667`. Face matrix validator now treats r133 static face as current and
  passes with zero errors/warnings.
- Moved to arms/guitar. Current static-live five-case coupled
  hand-target/guitar-anchor solve:
  `analysis/gh3_midori_r133_staticface_mainonly_coupled_guitar_solve_report.json`.
  Report-only fit looked plausible: max residual `4.958501`, mean `2.577198`.
- Emitted r134 guitar-only packed probe from that solve, then measured it
  before capture. It is rejected structurally:
  `analysis/gh3_midori_r134_staticface_coupled_guitar_contract_report.json`
  still has whole-limb hand gaps and solve deltas (`33.350-52.052`). Decision:
  `analysis/gh3_midori_r134_staticface_coupled_guitar_decision.json`.
- Next branch: do not emit guitar-only coupled fits. Emit or solve
  `bone_pos_guitar.mesh`, visible arms, and fret/strum target proxies together
  in one per-main-frame ACP path, then require packed HMX replay to keep both
  hand gaps and solved guitar-anchor delta at visual-contact scale before any
  visual capture.

## 2026-08-21 r132 face-control routing diagnosis

- Commands ran at below-normal/low priority. No ISO was mounted or used. The
  candidate capture staged a temporary loose DLC copy from
  `gh2_ps2_hybrid_assets/DLC`, substituted only r132 main, and deleted the temp
  tree afterward.
- User clarified that GH2 does not have the Neversoft facial-control count. The
  face target is a limited GH2-call translation matrix with honest gaps, not
  one-to-one Neversoft facial parity.
- Built unpromoted r132
  `analysis/gh3_midori_gh2_milos/gh3_midori_main_facealiases_jawdelta_gain2_r132.milo_ps2`
  from r131 with `--jaw-delta-gain 2.0`. Validator passed with two open calls,
  zero errors/warnings, and sampled max jaw delta `14.360422` degrees.
- Found and fixed a native runtime filtering problem. Before the patch,
  `gh2_face_call_open` reached only `ch=14:out=7`; after patching the known
  Midori hashed face controls into app/gameplay/CharClip filters, the viewer
  publishes `gh2_face_call_open` as `ch=15:out=10`.
- Added hash-aware face debug classification. Focused debug proof
  `analysis/gh3_midori_r132_facefilter_debug_after_classifier_report.json`
  fails only the single-case idle-difference harness check, but its log proves
  `faceRows=8`, `faceOutputBones=10`, and publisher layer
  `gh2_face_call_open@f120:w=1.000:ch=15:out=10`.
- Patched-filter visual comparison still shows tiny default/open pixel deltas
  (`0.00085178`, `0.00084744`, `0.00085612`, `0.00099935`). Treat r132 as
  routing/control proof, not visual face approval and not a promoted build.
- Next face step: inspect mesh influence/output application for the hashed face
  controls, or build the explicit GH2-call-to-available-Midori-control matrix
  from the now-proven routing path. Do not prioritize guitar attachment or
  arms/hands until pelvis-through-head/face control is accepted.

## 2026-08-21 r86/r87 side-specific blend branch

- Commands ran at Idle/low priority. Capture used local
  `gh2_ps2_hybrid_assets/GEN` plus loose DLC at
  `gh2_ps2_hybrid_assets/DLC`; no ISO was used at game time, and loose package
  files were restored afterward.
- Added side-specific blend overrides to
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py`:
  `--left-visible-arm-position-blend` and
  `--right-visible-arm-position-blend`. Unset sides inherit the global
  `--visible-arm-position-blend`. Added a focused test for defaulting behavior.
- r86 tested left/fret `0.35`, right/strum `0.75`. It is the new best partial
  branch: fixed diagonal stockattach guitar stays stable, attack is close to r85
  but with better strum-side contact, and the visual/contact balance beats r87.
  Serialized contact: attack `LH=1.410 RH=2.559`, fast-jump `4.997/2.572`,
  fast-solo `4.174/4.265`, transition `4.724/0.835`, outfit-2 attack
  `1.410/2.559`.
- r87 tested left `0.25`, right `1.0`. It improves strum target distance but
  worsens fret contact and does not visibly beat r86. Reject it.
- Direct visual still rejects. Remaining failure is fretting hand/forearm
  neck-grip quality in jump/solo/transition, not lower body, torso, or guitar
  silhouette.
- Decision artifact:
  `analysis/gh3_midori_r86_r87_sideblend_visual_triage.json`.
- Next branch: keep stockattach guitar fixed, keep r86 side blend, and tune
  fretting-hand orientation or add a neck-grip pose prior. Do not keep
  increasing right-side position forcing.

## 2026-08-21 r83-r85 arm-position blend branch

- Commands ran at Idle/low priority. Capture used local
  `gh2_ps2_hybrid_assets/GEN` plus loose DLC at
  `gh2_ps2_hybrid_assets/DLC`; no ISO was used at game time, and loose package
  files were restored afterward.
- Added `--visible-arm-position-blend` to
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py`. Default `1.0`
  keeps old full solved-position behavior. Intermediate values blend emitted
  forearm/hand local positions between original locals and solved target locals.
  Added a focused unit for the blend helper in
  `tools/gh3_midori_pipeline_test.py`.
- r83 tested source upper/forearm/hand orientation transfer on the fixed
  stockattach guitar frame in both guitar-body and GLB target-basis spaces.
  Contact stayed green, but visuals got worse: arms fold into the
  torso/chest/head area. Reject r83 as worse than r81.
- r84 tested rotation-only arm channels (`--no-emit-visible-arm-positions`).
  It improves the fretting-arm path, proving full position forcing contributes
  to the ugly r81 arm silhouette, but contact weakens too much.
- r85 tested `--visible-arm-position-blend 0.5`. It is the new best partial
  branch: diagonal stockattach guitar stays fixed, arm path is cleaner than r81,
  and contact is better than r84. Contact summary:
  attack `LH=1.085 RH=5.119`, fast-jump `3.844/5.144`, fast-solo
  `3.614/4.654`, transition `3.634/1.670`, outfit-2 attack `1.085/5.119`.
  Direct visual still rejects because hands/forearms remain stiff and
  non-performance-quality.
- Decision artifact:
  `analysis/gh3_midori_r83_r85_stockframe_armblend_visual_triage.json`.
- Next branch: keep the stockattach guitar frame fixed and sweep position blend
  by side/case. Likely direction is lower blend on the fretting arm to preserve
  silhouette, higher blend or a separate pose prior for strum-hand contact. Do
  not continue the r83 source upper-chain orientation transfer.

## 2026-08-21 r81/r82 stockframe main-arm partial improvement

- Commands ran at Idle/low priority. Capture used local
  `gh2_ps2_hybrid_assets/GEN` plus loose DLC at
  `gh2_ps2_hybrid_assets/DLC`; no ISO was used at game time, and loose package
  files were restored afterward.
- r81 held the upright r140/r144 stockattach guitar frame and generated
  main-layer visible arm channels with
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py --target-layer main`
  against the actual runtime fret/strum targets. It used canonical visible
  hand-axis calibration and did not override the guitar frame.
- Structural gates passed. Torso-axis stayed green. Contact distances:
  attack `LH=0.019 RH=0.000`, fast-jump `0.000/0.000`, fast-solo
  `2.312/4.025`, transition `0.000/0.000`, outfit-2 attack `0.019/0.000`.
  `analysis/gh3_midori_r81_stockframe_meshaxis_guitar_silhouette_report.json`
  marks all five guitars non-vertical, roughly a -30 degree diagonal.
- Direct visual review rejects r81 but confirms a real improvement over r80:
  attack is the best partial frame so far with upright body, real guitar frame,
  fret hand near neck, and strum hand near body. Jump/solo/transition still fail
  because the visible fretting arm disappears or overextends behind the
  head/body; the two-bone arm solve reaches targets but does not produce a
  performance-quality upper-chain pose.
- r82 repeated r81 with `--source-elbow-hint source-arm-direction`. It preserved
  the metrics and diagonal guitar frame but visibly bent the arm farther behind
  the torso/head. Reject r82 as worse than r81.
- Decision artifact:
  `analysis/gh3_midori_r81_r82_stockframe_main_arm_visual_triage.json`.
- Next branch: keep the stockattach guitar frame fixed and solve/transfer a
  fuller source/performance upper chain before applying final hand contact.
  Do not return to pelvis/root/torso, and do not return to guitar-to-current-hand
  fitting as the primary objective.

## 2026-08-21 r80 upright-transpose guitar fit rejected

- Commands ran at Idle/low priority. Capture used local
  `gh2_ps2_hybrid_assets/GEN` plus loose DLC at
  `gh2_ps2_hybrid_assets/DLC`; no ISO was used at game time, and loose package
  files were restored afterward.
- Corrected the r79 diagnosis: live/promoted Midori and the r140/r144
  stockattach builder stage pass HMX torso-axis. The sideways r79 captures came
  from materializing the guitar-fit ACPs onto raw `analysis/gh3_midori_acp_stage`,
  the old horizontal-body family. Lower body/pelvis/torso remain solved and
  regression-only.
- Rebuilt the upright stockattach stage with
  `tools/gh3_midori_build_allmain_stockattach_candidate.py`, regenerated the
  r79 coupled guitar-fit ACPs, and tested direct, transpose, and mixed policies
  on the upright base. On this correct base, all-transpose is the structural
  winner:
  attack `L=4.959 R=4.959 D=9.917`, fast-jump
  `L=1.569 R=1.569 D=3.138`, fast-solo `L=0.723 R=0.723 D=1.445`,
  transition `L=0.677 R=0.677 D=1.355`, outfit-2 attack matching outfit 1.
- Direct sequential visual review still rejects all five r80 proofs in
  `.codex/current-evidence/gh3_midori_r80_upright_transpose_proofs`.
  `analysis/gh3_midori_r80_upright_transpose_guitar_silhouette_report.json`
  marks fast-jump and fast-solo as vertical-bar-like, matching manual review.
  Decision artifact:
  `analysis/gh3_midori_r80_upright_transpose_visual_triage.json`.
- Next branch: keep using the upright stockattach builder stage, not raw
  `gh3_midori_acp_stage`. Do not keep fitting the guitar to the current visible
  hands as the primary objective: that greens hand-target distance while leaving
  the prop/arms visually wrong. Hold or derive a playable guitar frame first,
  then solve visible upper arms, forearms, hands, and controller targets onto
  that frame.

## 2026-08-21 r79 coupled guitar-anchor mixed fit rejected

- Commands ran at Idle/low priority. Capture used local
  `gh2_ps2_hybrid_assets/GEN` plus loose DLC at
  `gh2_ps2_hybrid_assets/DLC`; no ISO was used at game time, and loose package
  files were restored afterward.
- Lower body/pelvis is still solved for this branch and remains regression-only.
  The captured failures are not a pelvis-child hierarchy collapse: legs stay
  coherent, but the whole pose/root/torso orientation is sideways or recumbent
  and the arms/guitar are not in a playable relationship.
- Added `tools/gh3_midori_emit_coupled_guitar_fit_acp.py` to materialize the
  coupled two-hand `bone_pos_guitar.mesh.pos/.quat` solve from
  `tools/gh3_midori_final_hand_target_local_solve_report.py` into patchable ACP
  channels. Added a focused unit in `tools/gh3_midori_pipeline_test.py`.
- Structural results:
  `analysis/gh3_midori_r79_coupled_guitar_contract_report.json` proved the
  transpose-HMX branch makes fast-jump/fast-solo/transition structurally close
  but leaves attack weak. The direct-HMX branch improves attack/transition but
  breaks fast-jump/fast-solo. A mixed branch using direct for
  `gh3_guit_mido_a_attackl` and `gh3_guit_midori_tran_atoout`, transpose for
  `gh3_guit_mido_a_fst_jump01` and `gh3_guit_mido_a_fst_solo01`, produced:
  attack `L=19.031 R=10.257 D=18.257`, fast-jump
  `L=1.569 R=1.569 D=3.138`, fast-solo `L=0.723 R=0.723 D=1.445`,
  transition `L=6.152 R=0.875 D=6.697`.
- Direct sequential visual review rejects all five r79 mixed proofs in
  `.codex/current-evidence/gh3_midori_r79_coupled_guitar_mixed_proofs`.
  Decision artifact:
  `analysis/gh3_midori_r79_coupled_guitar_mixed_visual_triage.json`.
- Next branch should keep the coupled guitar-anchor fit as a useful sub-result
  but gate it behind an upright/root-orientation solve for main animation
  samples. Do not return to pelvis-only matrix-local/`Control_Root` work unless
  a new candidate visibly regresses the lower-body chain.

## 2026-08-20 r140 all-main stock-attach guitar-frame diagnostic

- Commands ran at Idle/low priority. Captures used local
  `gh2_ps2_hybrid_assets/GEN` plus package-layout loose DLC only; no ISO was
  used, and package files were restored afterward.
- Extended `tools/gh3_midori_merge_acp_channel_samples.py` so it can patch
  multiple clips in one stage copy (`--target-clips`, `--target-all`) and can
  safely add missing channels (`--allow-add-channels`). Added-channel rebuilds
  now use `gh3_midori_acp_stage.channel_order`; appending channels at the end
  made `milo_convert_tool` reject the ACP as out of GH2 CharBones type order.
- r140 applies the stock-attach `bone_pos_guitar.mesh.pos/.quat` frame to all
  266 guitar-main clips with `--source-sample-mode repeat-first`, adding the
  missing guitar channels to 12 clips. It preserves the r139 hand-overlay
  fret/strum arm-chain replacement for
  `gh3_hnd_guit_chord_mid_bar3_d` and
  `gh3_hnd_guit_strum_mido_norm_m01_d`.
- Best visual evidence:
  `r140_allmain_guitar_full_visual_20260820/contact_sheet.jpg`. Direct visual
  shows coherent bipedal posture across the reviewed 9-case sheet, diagonal
  guitar placement instead of the vertical face/prop failure, and the accessory
  vertical-sliver fallback is fixed. Wrapper result is 8/9 passing; only outfit
  2 attack fails `framing_has_margins`.
- This is still diagnostic, not complete. The stock-attach guitar frame is
  constant across all main clips. Next work should either broaden direct review
  enough to decide this constant frame is acceptable, or replace it with a real
  per-clip/per-frame policy before promoting the live DLC package.

## 2026-08-20 r141 reproducible all-main stock-attach builder

- Commands ran at Idle/low priority. No ISO was used.
- Added `tools/gh3_midori_build_allmain_stockattach_candidate.py`. It
  reconstructs the r140-style diagnostic candidate from source inputs:
  lower/pelvis bind freeze, targeted head/clavicle identity patches, stock
  attach-world guitar/hand overlay probe, all-main guitar channel merge, and
  final main/fret/strum MILO builds.
- Builder output rebuilt cleanly at
  `analysis/gh3_midori_allmain_stockattach_candidate` during this turn, then
  the rebuildable candidate was cleaned. Recreate it with:
  `python tools/gh3_midori_build_allmain_stockattach_candidate.py --work-dir analysis/_midori_builder_allmain_stockattach_work --output-candidate analysis/gh3_midori_allmain_stockattach_candidate --reports-dir .codex/current-evidence/midori-fresh-attack-targetlength-20260818/r141_builder_reports_20260820 --clean-output`
- Post-MILO runtime sanity passed:
  `r141_builder_reports_20260820/allmain_stockattach_anim_runtime_sanity.json`
  has `status=ok_bridge_validated_child_positions`,
  `samples=105`, `child_pos=98`, `max_child_pos=39.406`.
- Packed main MILO coverage was verified:
  `r141_builder_reports_20260820/allmain_stockattach_packed_channel_coverage.json`
  proves `clip_count=266`,
  `bone_pos_guitar_mesh_pos_channel_count=266`, and
  `bone_pos_guitar_mesh_quat_channel_count=266`.
- Next useful work: generate a fresh review packet/gallery from the rebuilt
  candidate or broaden direct visual sampling beyond the 9-case sheet before
  deciding whether the constant stock-attach frame is acceptable enough to
  promote.

## 2026-08-20 r142 added-channel extended visual coverage

- Commands ran at Idle/low priority. Captures used local
  `gh2_ps2_hybrid_assets/GEN` plus package-layout loose DLC only; no ISO was
  used, and package files were restored afterward.
- Added data-driven visual coverage support:
  `tools/gh3_midori_pose_review.py --cases-json` accepts either a JSON list of
  cases or an object with a `cases` list, and
  `tools/gh3_midori_capture_with_loose_dlc_backup.py --cases-json` forwards it
  through the temporary loose-DLC deploy/restore path.
- Added
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r142_added_channel_cases_20260820.json`
  for the 12 r140 clips that required added `bone_pos_guitar` channels:
  `gh3_guitarist_midori_acc01`, `acc02`, `default`, `drag_reaction`,
  `dragon_climb`, `happy`, `idle`, `kiss`, `pout`, `satisfied`, `testidle`,
  and `yeah`.
- Rebuilt the all-main stock-attach candidate with the r141 builder and
  captured the 12-case set. Evidence:
  `r142_added_channel_visual_20260820/contact_sheet.jpg` and
  `pose_review_manifest.json`. The wrapper reports 12 failures, but all are the
  `visible_difference_from_idle` heuristic; framing/load/texture checks are
  clean. Direct visual inspection shows the old vertical fallback is gone in
  all 12, bodies remain bipedal, and the guitar is consistently diagonal.
- Continue by broadening from these added-channel/special clips toward either a
  larger sampled animation gallery or a promotion decision for the constant
  stock-attach frame. Do not treat the idle-difference heuristic alone as a
  visual rejection when the direct images are coherent.

## 2026-08-20 r143 sampled main-bank visual coverage

- Commands ran at Idle/low priority. Captures used local
  `gh2_ps2_hybrid_assets/GEN` plus package-layout loose DLC only; no ISO was
  used, and package files were restored afterward.
- Added
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r143_sampled_main_cases_20260820.json`,
  a 26-case sampled main-bank review set covering bad/fst/med/slw clip
  families, reactions, B/C variants, transitions, `band_jump`,
  `stand_medium_01`, and `idle_medium_01`.
- Rebuilt the all-main stock-attach candidate with the r141 builder and
  captured the sampled set. Evidence:
  `r143_sampled_main_visual_20260820/contact_sheet.jpg` and
  `pose_review_manifest.json`. The wrapper reports 26 failures, all
  `visible_difference_from_idle`; framing/load/texture/layer checks are clean.
- Direct visual inspection shows no non-bipedal collapses and no vertical
  guitar fallback in the sampled main-bank families. The guitar stays diagonal
  across the body. This is broader support for the constant stock-attach frame,
  but not final approval for every frame of every clip.
- Next useful work is to decide whether to promote the constant stock-attach
  candidate to the live package for a full representative review packet, or to
  build a larger sampled gallery before that decision.

## 2026-08-20 r144 live DLC promotion

- Commands ran at Idle/low priority. Captures used local
  `gh2_ps2_hybrid_assets/GEN` plus package-layout loose DLC only; no ISO was
  used.
- Rebuilt the all-main stock-attach candidate with
  `tools/gh3_midori_build_allmain_stockattach_candidate.py`, verified
  post-MILO runtime sanity, then intentionally promoted the rebuilt MILOs into
  the live loose-DLC package:
  `gh2_ps2_hybrid_assets/DLC/community.gh3.midori`.
- The prior r137-era live hashes are now stale. Current live promoted hashes
  are recorded at
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r144_promoted_live_reports_20260820/promoted_live_hashes.json`.
  Summary:
  main `AD3239C1D10F790C8057D61FE603DBF92AD6F097AF17FC43FD02A838C85BC7C6`;
  fret `329E306F261646C709691313D50098DC243E1DC01E7F3A3F9F9A0CDE88692090`;
  strum `C6F837D5779C8BCA70E0EACFB3672D5976C393589FDB5F4B9BBD4D01DD1E2C16`;
  ui `0CDF50C1EFA0B621DD429E961D3611A23632188C685383DBED8AFBE1611216B7`;
  model1 `8C7E2164D8349C547944BA157840DBA43C8C478B741372D059E9AB2E96882701`;
  model2 `4D9DC087EE40BC079362F6A46842EC4CEBAF1EE6F7DB947E2612784EDD1A5F6D`.
- Verified the promoted live package directly with
  `tools/gh3_midori_pose_review.py --capture`, not via the temporary deploy
  wrapper. Evidence:
  `r144_promoted_live_visual_20260820/contact_sheet.jpg` and
  `pose_review_manifest.json`. Wrapper result is 8/9 representative cases
  passing; the only failure remains outfit 2 attack framing. Direct visual
  confirms the live package has the coherent bipedal/diagonal-guitar state.
- The goal remains open. Next useful work is a review packet/gallery tied to
  the promoted live package and direct user visual approval, or a final
  promotion audit that decides whether the constant stock-attach frame is
  acceptable for release.

## 2026-08-20 r145 promoted-live approval refresh

- Commands ran at Idle/low priority. No ISO was used; proof capture read local
  `gh2_ps2_hybrid_assets/GEN` plus loose DLC at
  `gh2_ps2_hybrid_assets/DLC`.
- Lower body is solved for the promoted main-body path. Continue treating
  pelvis/lower-limb posture as a regression gate only; do not resume
  pelvis-only/Control_Root diagnosis unless a later visual candidate visibly
  loses upright bipedal posture.
- Regenerated the default nine native proof frames directly from the promoted
  live loose-DLC package at `camera-distance 150` and built
  `analysis/gh3_midori_pose_review_proofs/contact_sheet.jpg`. Fresh manifest:
  `analysis/gh3_midori_pose_review_proofs/pose_review_proof_manifest.json`,
  status `native_viewer_representative_pose_framing_review_passed`, proofs `9`,
  failures `0`.
- Refreshed direct bipedal notes and precheck:
  `analysis/gh3_midori_pose_bipedal_manual_verdicts.json` and
  `analysis/gh3_midori_pose_bipedal_precheck.json`. Result:
  `sequential_visual_bipedal_precheck_passed`, proofs `9`, failures `0`.
- Added `tools/gh3_midori_promoted_review_packet.py` and generated the current
  promoted-live review artifacts:
  `analysis/gh3_midori_review_packet.json`,
  `analysis/GH3_MIDORI_REVIEW_PACKET.md`,
  `analysis/GH3_MIDORI_VISUAL_APPROVAL.html`, and
  `analysis/gh3_midori_visual_approval_gallery.json`.
- Direct approval gate now reports
  `status=pending_user_visual_approval approval_exists=False failures=0` in
  `analysis/gh3_midori_direct_visual_approval_gate.json`. Goal remains open
  until direct user visual approval is recorded, or until specific rejected
  frames define the next solver branch.
- Follow-up: updated `tools/gh3_midori_visual_approval_gallery.py` so summary
  sheets include all `*_contact_sheet` proof files rather than only the default
  legacy sheet names. Regenerated `analysis/GH3_MIDORI_VISUAL_APPROVAL.html`
  and `analysis/gh3_midori_visual_approval_gallery.json`; the gallery now
  shows 4 summary sheets (`pose_review`, `promoted_live`, `sampled_main`, and
  `added_channel`) plus the 9 ordered pose frames. Direct approval gate remains
  `pending_user_visual_approval` with failures `0`. `pytest` is not installed;
  verification used `py_compile`, regeneration, gate status, and HTML link
  checks.
- Added `tools/gh3_midori_promoted_completion_readiness_audit.py`. Current
  output:
  `analysis/gh3_midori_promoted_completion_readiness_audit.json` reports
  `status=pending_direct_user_visual_approval`,
  `completion_allowed=False`, failures `0`, pending `1`, items `8`. The seven
  passed rows verify promoted live hashes, ordinary path-based loose DLC, no
  shipped ISO/base archives, local GEN plus loose DLC runtime evidence,
  review-packet readiness, gallery coverage, and the bipedal regression gate.
  The sole pending row is direct user visual approval for the current
  packet/gallery fingerprints. Do not call the active goal complete until this
  audit changes to `completion_ready_for_goal_close` via an accepted approval
  gate.
- Added `tools/gh3_midori_direct_visual_decision.py` for fingerprint-safe
  direct visual decisions. It writes accepted decisions to
  `analysis/gh3_midori_direct_visual_approval.json` only when invoked without
  `--dry-run`; rejected decisions default to
  `analysis/gh3_midori_direct_visual_rejection.json` and require at least one
  `--reject-item`. Also refreshed the approval-gate template wording to match
  the current summary-sheet plus 9-pose-frame gallery. Dry-run acceptance
  simulated `direct_user_visual_approval_accepted`; dry-run rejection accepted
  `pose_midori_1_attack_left_f030`; no real approval/rejection file exists.
  If the user explicitly approves the current gallery, run:
  `python tools/gh3_midori_direct_visual_decision.py --decision accepted --reviewer USER --notes "explicit visual approval" --validate-gate --update-gates --print-summary`.
- Added focused regression tests for the promoted-live decision/readiness
  bridge in `tools/gh3_midori_pipeline_test.py`, and added source anchors for
  `tools/gh3_midori_direct_visual_decision.py`,
  `tools/gh3_midori_promoted_review_packet.py`, and
  `tools/gh3_midori_promoted_completion_readiness_audit.py` in
  `tools/gh3_midori_build_pipeline.py`. Targeted unittest command passed:
  `test_direct_visual_decision_acceptance_validates_gate`,
  `test_direct_visual_decision_rejects_unknown_or_empty_rejections`,
  `test_promoted_completion_readiness_audit_pending_until_approval`, and
  `test_production_pipeline_uses_generic_retarget_contract`.
- Updated `tools/gh3_midori_visual_approval_gallery.py` so every summary sheet
  and native pose card shows the exact review item id accepted by
  `tools/gh3_midori_direct_visual_decision.py --reject-item`. The gallery
  manifest now includes `sheet_review_item_ids`, `pose_review_item_ids`, and
  `review_item_ids`; the current regenerated gallery has 13 review items
  (4 contact sheets plus 9 pose frames). Updated
  `tools/gh3_midori_direct_visual_decision.py` so sheet item order matches the
  gallery order. Targeted gallery/decision/readiness unittest coverage passed
  (`6 tests OK`). Live accepted dry-run still validates as
  `direct_user_visual_approval_accepted` without writing an approval JSON; the
  real gate remains pending.
- Added `--list-items` to `tools/gh3_midori_direct_visual_decision.py`; it
  lists id, kind, and path for every accepted `--reject-item` without requiring
  or writing a decision. It uses
  `analysis/gh3_midori_visual_approval_gallery.json` when present, so terminal
  list order matches the visible gallery order. Current live list summary:
  `review_items=13 summary_sheets=4 pose_frames=9`. Targeted unittest coverage
  for gallery IDs, decision item ordering, list metadata, acceptance/rejection,
  and readiness now passes (`7 tests OK`). No real approval/rejection file
  exists.
- Added `tools/gh3_midori_direct_visual_review_checklist.py`. Current output:
  `analysis/GH3_MIDORI_DIRECT_VISUAL_REVIEW_CHECKLIST.md` and
  `analysis/gh3_midori_direct_visual_review_checklist.json`. The checklist
  records all 13 review items with kind/path, links the gallery/review
  packet/gate/readiness audit, and includes exact accept/reject/list commands.
  Generated status: `items=13 sheets=4 pose=9`,
  `gate=pending_user_visual_approval`,
  `readiness=pending_direct_user_visual_approval`. Targeted checklist/decision
  unittest coverage passed (`4 tests OK`). No real approval/rejection file
  exists.
- Added `--update-gates` to `tools/gh3_midori_direct_visual_decision.py`.
  After an explicit accepted decision is written, it refreshes
  `analysis/gh3_midori_direct_visual_approval_gate.json` and
  `analysis/gh3_midori_promoted_completion_readiness_audit.json`. A synthetic
  regression proves this accepted path reaches
  `direct_user_visual_approval_accepted` and
  `completion_ready_for_goal_close`. The regenerated review checklist now uses
  the one-command accept path with `--validate-gate --update-gates`. Live
  dry-run still writes no approval file; the real readiness audit remains
  pending.
- Updated `tools/gh3_midori_visual_approval_gallery.py` so
  `analysis/GH3_MIDORI_VISUAL_APPROVAL.html` links directly to
  `analysis/GH3_MIDORI_DIRECT_VISUAL_REVIEW_CHECKLIST.md` from the summary
  panel. Regenerated gallery, approval template/gate, readiness audit, and
  checklist. Current gallery manifest includes
  `direct_visual_review_checklist=analysis/GH3_MIDORI_DIRECT_VISUAL_REVIEW_CHECKLIST.md`
  and `review_items=13`. Targeted gallery/checklist/decision/readiness tests
  passed (`4 tests OK`). No real approval/rejection file exists.
- Full `python -m unittest tools.gh3_midori_pipeline_test` now passes
  (`99 tests OK`). During the full run, fixed a GLB/MILO route-gate regression:
  `tools/gh3_midori_glb_milo_route_gate.py` now treats missing staging bridge
  evidence as explicit pending state and lets the current promoted-live
  readiness audit supersede stale targetlength visual-packet failures. Updated
  the synthetic route-gate test fixtures in `tools/gh3_midori_pipeline_test.py`
  to use the current targetlength evidence filenames and route status. Live
  `tools/gh3_midori_glb_milo_route_gate.py --print-summary` now reports
  `status=glb_to_milo_route_guarded failures=0 glb_promotable=True
  route=targetlength_route_guarded_pending_user_visual_approval`.
- Added `tools/gh3_midori_promoted_release_package.py` and generated
  `analysis/community.gh3.midori.promoted-live.zip` plus
  `analysis/gh3_midori_promoted_release_package_manifest.json`. The manifest
  reports `status=promoted_release_package_ready`, `files=7`,
  `zip_bytes=21812132`, failures `0`, forbidden files `0`, ZIP SHA-256
  `5AC3E561FEE916876570817692079146B52037DE9D30EB75FC72FA59698D0E2A`.
  The ZIP contains only the loose DLC folder `community.gh3.midori/`: six
  promoted MILOs and package `manifest.json`; no ISO/base archive payloads.
  Added a release-packager regression test. Full
  `python -m unittest tools.gh3_midori_pipeline_test` now passes
  (`100 tests OK`).
- Updated `tools/gh3_midori_promoted_completion_readiness_audit.py` so the
  release ZIP is part of the final readiness boundary. The audit now verifies
  `analysis/gh3_midori_promoted_release_package_manifest.json` plus the ZIP
  hash/shape. Current readiness output:
  `status=pending_direct_user_visual_approval completion_allowed=False
  failures=0 pending=1 items=9`. Eight rows pass, including
  `Promoted loose-DLC release ZIP is built and verified`; the only pending row
  remains direct user visual approval. Updated synthetic readiness and
  `--update-gates` tests so accepted approval cannot mark completion unless
  the release package audit input is present. Full unittest remains
  `100 tests OK`.
- Added `tools/gh3_midori_promoted_release_notes.py`; generated
  `analysis/GH3_MIDORI_RELEASE_NOTES.md` and
  `analysis/gh3_midori_promoted_release_notes.json`. The notes capture the ZIP
  path/hash/size, install target, no-ISO/game-time archive boundary, readiness
  status, direct visual checklist link, and one-command approval path with
  `--update-gates`. Current notes summary:
  `status=promoted_release_package_ready zip_bytes=21812132
  readiness=pending_direct_user_visual_approval completion_allowed=False
  zip_exists=True`. Added notes regression coverage. Full
  `python -m unittest tools.gh3_midori_pipeline_test` now passes
  (`101 tests OK`).
- Linked `analysis/GH3_MIDORI_RELEASE_NOTES.md` from both
  `analysis/GH3_MIDORI_VISUAL_APPROVAL.html` and
  `analysis/GH3_MIDORI_DIRECT_VISUAL_REVIEW_CHECKLIST.md`. Regenerated gallery,
  approval template/gate, readiness audit, review checklist, and release notes.
  Current gallery manifest includes
  `promoted_release_notes=analysis/GH3_MIDORI_RELEASE_NOTES.md`,
  `direct_visual_review_checklist=analysis/GH3_MIDORI_DIRECT_VISUAL_REVIEW_CHECKLIST.md`,
  and `review_items=13`. Full unittest remains `101 tests OK`; no approval or
  rejection file exists.
- Refreshed `tools/gh3_midori_completion_audit.py` so its legacy targetlength
  rows are superseded by the promoted-live release/readiness artifacts instead
  of re-failing already-rejected experimental sheets. Current output is
  `status=review_ready_pending_user_acceptance`, `proven=17`, `pending=1`,
  `failed=0`. The only pending row is direct user visual approval. Lower
  body/pelvis is solved for the promoted main-body path and should be treated
  as a regression gate only unless a new visual candidate visibly loses bipedal
  posture. Full `python -m unittest tools.gh3_midori_pipeline_test` remains
  `101 tests OK`.
- Re-inspected the latest promoted-live approval sheet and individual enlarged
  pose frames after the user's visual concern. The sheet is not an approval
  candidate. Lower body/pelvis remains bipedal enough to keep as a regression
  gate, but the stock-attach guitar/hand candidate visibly fails upper-body,
  arm, and guitar-contact coherence in six review items. Added
  `analysis/gh3_midori_promoted_visual_triage.json`:
  `status=agent_visual_triage_failed`, `approval_candidate=False`,
  `failure_count=6`. `tools/gh3_midori_promoted_completion_readiness_audit.py`
  now consumes this optional triage file and the live readiness output is
  `status=failed completion_allowed=False failures=1 pending=1`. The canonical
  completion audit also fails (`proven=15 pending=1 failed=2`) with details
  pointing to the promoted triage rather than stale targetlength evidence.
  Regenerated release notes and direct visual checklist; both report readiness
  `failed`. Next work: resume upper-body/arm/guitar-contact retargeting from
  GLB/source bridge or a validated intermediate skeleton. Do not return to
  pelvis-only/Control_Root unless a new candidate loses bipedal posture.
- Contact diagnosis for the promoted live package is now quantified. A
  temporary flat staging of the live loose-DLC MILOs was measured, then cleaned.
  Evidence:
  `analysis/gh3_midori_promoted_contact_contract_report.json`,
  `analysis/gh3_midori_promoted_visible_arm_solve_report.json`, and
  `analysis/GH3_MIDORI_PROMOTED_CONTACT_NEXT_STEPS.md`. Across six visually
  rejected review cases, hand-target gaps are large (`L=27.939-57.163`,
  `R=16.346-41.621`) and per-hand solved guitar anchors disagree by
  `99.868-130.034`, proving a simple global guitar offset is not sufficient.
  On the explicit hand-overlay case, `current_strum_guitar` clamps both arms
  across all checked frames, while `forearm_hand` keeps both arms unclamped and
  reduces average misses to `L=2.766`, `R=0.790`. Fixed
  `tools/gh3_midori_visible_arm_solve_report.py` to pass `sample_quat_mode`
  through to shared transform loading. Next concrete rebuild branch: try a
  `forearm_hand` parent-space/rebase overlay candidate, then extend the same
  contract to main-only clips that currently lack fret/strum overlays.
- Follow-up r146/r147 emitted candidates tested that branch and rejected it
  structurally before visual capture. Added `--probe-recipe` to
  `tools/gh3_midori_build_allmain_stockattach_candidate.py`, then built
  `forearm-hand-rebase` and `forearm-hand-rebase-postarm-align` candidates.
  Both generated ordinary MILOs, but both worsened the hand-overlay contract:
  `L hand to fret=27.481`, `R hand to strum=56.884`,
  `solved guitar anchor delta=154.283`, with both arms clamped in the
  post-build `forearm_hand` visible-arm report. Decision artifact:
  `analysis/gh3_midori_r146_r147_forearm_rebase_decision.json`. Do not capture
  or promote these candidates. Next branch should inspect emitted fret/strum
  ACP samples and final replay contacts directly, then preserve the low-error
  replay-only target through actual `build-clipset-from-acp` emission.
- Corrected contact diagnostics after finding a replay-mode mismatch. Added
  `--sample-quat-mode` to `tools/gh3_midori_visible_arm_solve_report.py` and
  regenerated promoted contact/visible-arm reports with HMX sample replay.
  Evidence:
  `analysis/gh3_midori_forearm_emission_loss_diagnostic.json`,
  `analysis/gh3_midori_promoted_contact_contract_report.json`, and
  `analysis/gh3_midori_promoted_visible_arm_solve_report.json`. Direct replay
  was falsely inflating emitted-contact failures. With HMX replay, the explicit
  hand-overlay case is close (`L=0.000`, `R=1.454`) and should not be treated
  as the primary contact failure. The remaining contact failures are main-only
  clips without fret/strum overlay layers: attack-left, fast-jump, fast-solo,
  transition-out, and outfit-2 attack-left. Next concrete branch: synthesize or
  derive per-frame fret/strum contact overlays for those main-only clips, then
  rebuild ordinary MILOs and gate with HMX contact reports before capture.
- Existing-overlay reuse is rejected. Evidence:
  `analysis/gh3_midori_main_only_contact_overlay_cases.json`,
  `analysis/gh3_midori_main_only_contact_overlay_contract_report.json`, and
  `analysis/gh3_midori_main_only_overlay_policy_decision.json`. Reusing
  `gh3_hnd_guit_chord_mid_bar3_d` plus
  `gh3_hnd_guit_strum_mido_norm_m01_d` helps right/strum by `11-30` units but
  worsens left/fret on fast-jump, fast-solo, and transition-out. Do not pursue
  wholesale overlay reuse; generate per-main-frame fret targets next.
- Per-main-frame target-proxy synthesis was tested next and is also rejected
  before capture. Added `tools/gh3_midori_synthesize_main_contact_overlays.py`;
  evidence:
  `analysis/gh3_midori_synth_contact_overlay_acp_report.json`,
  `analysis/gh3_midori_synth_contact_overlay_cases.json`,
  `analysis/gh3_midori_synth_contact_overlay_contract_report.json`, and
  `analysis/gh3_midori_synth_contact_overlay_decision.json`. The probe moves
  fret/strum target proxies onto the current visible hand positions, so
  hand-to-target gaps become `0.000`, but solved guitar-anchor deltas remain
  `110-129` units. This proves target-proxy synthesis alone greens the wrong
  metric. Next concrete branch: fit or animate `bone_pos_guitar.mesh` per
  failed main frame from the synthesized contact pair, then require both
  hand-to-target gaps and solved guitar-anchor delta to pass before visual
  capture.

## 2026-08-20 r139 stock-attach guitar-frame probe

- Commands ran at Idle/low priority. Captures used local
  `gh2_ps2_hybrid_assets/GEN` plus package-layout loose DLC only; no ISO was
  used, and package files were restored afterward.
- Rebuilt the r138 base patch stack in scratch, then ported the old
  `stock-prop-debug-attach-world` guitar frame onto the current coherent
  r138 body/arm-chain branch. Best probe settings:
  `--placement-mode stock-prop-debug-attach-world`,
  `--rotation-mode stock-prop-debug-attach-world`,
  `--overlay-channel-mode canonical-hands-visible-arm-rot`,
  `--emit-visible-arm-positions`,
  `--recompute-visible-arm-positions-after-arm-rot`,
  `--emit-hand-target-proxies`, and
  `--align-target-proxies-to-post-arm-hands`.
- Added `--source-sample-mode repeat-first` to
  `tools/gh3_midori_merge_acp_channel_samples.py`, then reused the stock-attach
  `bone_pos_guitar.mesh.pos/.quat` frame across six reviewed main clips:
  `stand_medium_01`, `gh3_guit_mido_a_med_idle01`,
  `gh3_guit_mido_a_attackl`, `gh3_guit_mido_a_fst_jump01`,
  `gh3_guit_mido_a_fst_solo01`, and `gh3_guit_midori_tran_atoout`.
  `gh3_guitarist_midori_acc01` has no `bone_pos_guitar` channels, so it was
  not patched by this method.
- Best visual evidence:
  `r139_mainwide_stockattach_full_visual_20260820/contact_sheet.jpg`. The
  guitar-bearing cases now show a diagonal guitar across the torso rather than
  a vertical prop through the face. Eight of nine wrapper cases pass; the only
  automated failure is outfit 2 attack framing. Direct visual is much improved
  but still not final approval: the guitar frame is constant/diagnostic, and
  accessory/no-guitar-channel behavior plus full animation coverage still need
  a proper promoted solution.
- Continue by turning the stock-attach guitar-frame diagnostic into a real
  per-clip/per-frame policy, or by explicitly deciding that a constant
  stock-attach frame is acceptable for the reviewed main clips before expanding
  to all required animations.

## 2026-08-20 r138 hand-overlay/guitar-frame diagnosis

- Commands ran at Idle/low priority. Captures used local
  `gh2_ps2_hybrid_assets/GEN` plus package-layout loose DLC only; no ISO was
  used, and package files were restored afterward.
- Added `tools/gh3_midori_merge_acp_channel_samples.py`. It copies a full ACP
  stage and merges selected channel samples from a probe ACP into the matching
  target clip, preserving all other body channels. Use it when a
  `gh3_midori_guitar_frame_hand_bake_probe.py` one-frame output needs to be
  tested inside a full candidate.
- r137 hand-overlay replay of `stand_medium_01@10` +
  `gh3_hnd_guit_strum_mido_norm_m01_d@10` +
  `gh3_hnd_guit_chord_mid_bar3_d@0` rejects on four metrics:
  visible hand center too high, left/right hand-to-target distances too large,
  and `guitar_to_head` too large. Suspicious replay nodes: spine1/2/3, both
  clavicles, and `bone_pos_guitar.mesh`.
- Extending the r137 stand/main treatment to `stand_medium_01` improves the
  overlay numerically but does not solve hand/guitar placement. Best full
  built-file transform candidate so far is
  `pos04_canon_visible_emitpos_recalc`: it passes
  `gh3_midori_overlay_visual_sanity_gate.py` with zero failures after merging
  generated main/fret/strum overlay channels into the full stage.
- Direct visual still rejects. `r138_pos04_overlay_visual_20260820` is upright
  and bipedal, but the guitar remains vertical through/above the face and the
  arms do not look like a playable guitar pose. Pair-fit rotation variants
  (`rot01_pair_current`, `rot02_pair_canon`) emit guitar quats, but `rot02`
  also visually rejects with the prop/arms above the face.
- Continue from the guitar-frame orientation/prop-axis solve plus arm-chain
  placement. Do not return to pelvis/lower-body diagnosis unless a new probe
  loses the r135/r137 upright bipedal posture.

## 2026-08-20 r134-r137 pelvis/lower/full-stage main-body repair

- Commands ran at Idle/low priority. Captures used local
  `gh2_ps2_hybrid_assets/GEN` plus package-layout loose DLC only; no ISO was
  used, and package files were restored afterward.
- Added safer patch controls:
  `tools/gh3_midori_patch_acp_quats_identity.py` now supports
  `--target-clips`; `tools/gh3_midori_patch_acp_channels_to_bind.py` now
  supports `--roles` and `--target-clips`.
- r134 froze lower-limb quats only on the full retained
  `analysis/gh3_midori_acp_stage` main bank. It visually rejected as horizontal,
  proving lower quats alone are not enough and full-stage pelvis rotation must
  be repaired first.
- r135 froze `bone_pelvis.mesh.quat` plus both legs' thigh/knee/ankle/toe quats
  to bind. This passed the 9-case native framing/load gate and restored upright
  bipedal lower body, but head/upper body/guitar still rejected.
- r136 added selective upper identity patches on only
  `gh3_guit_mido_a_med_idle01` and `gh3_guit_mido_a_attackl`. It was visually
  much better, but the main-body gate still rejected only
  `bone_pos_guitar.mesh angle 120.718257`.
- r137 changed those two clips' `bone_pos_guitar.mesh.quat` from identity to
  character bind. Main-only idle/attack now pass
  `gh3_midori_main_body_visual_sanity_gate.py` with zero failures. Evidence:
  `r137_idle_main_body_gate_20260820.json`,
  `r137_attack_main_body_gate_20260820.json`, and
  `r137_pelvis_lower_upper_guitarbind_visual_20260820`.
- Decision: r137 is the current best structural branch and proves pelvis,
  lower-body, head/clavicle suppression, and guitar-attach local rotation can
  pass the main-body sanity gate for the two key clips. It is **not** direct
  visual approval: the guitar is vertical and hands/controllers are not in
  playable contact, especially in `midori_1_hand_overlay_f010.bmp`. Continue
  with arms/guitar/controller placement from r137, not with more pelvis-only
  sweeps unless a later change regresses upright/bipedal body posture.

## 2026-08-20 r132-r133 lower-body bind-freeze diagnosis

- Commands ran at Idle/low priority. Captures used local
  `gh2_ps2_hybrid_assets/GEN` plus package-layout loose DLC only; no ISO was
  used, and package files were restored afterward.
- r132 repeated the r131 two-clip selective identity scratch candidate with
  pose mesh dumps. The result is still not approval-grade: animation is applied,
  but it reads like near-bind/default posture with broken arms/guitar and
  deformed lower limbs.
- The old "lower body solved" assumption is now invalid for direct visual
  approval. Inspection proves the model has lower-limb bones and the main
  clipset has lower-limb position/quaternion channels, so the failure is not
  missing bones or absent animation.
- Concrete clue: r132 attack samples include sign/basis-inverted leg locals,
  e.g. `bone_L-ankle.mesh.pos = -17.5607,...` versus model bind local
  `+16.8798,...`.
- Added `tools/gh3_midori_patch_acp_channels_to_bind.py` to freeze selected
  ACP `.pos` and `.quat` channels to character bind locals. r133 froze both
  thighs/knees/ankles/toes while leaving the selective head/guitar/clavicle
  identity patch in place. The resulting idle/attack screenshots restore
  coherent bipedal legs, proving the model skin/bind hierarchy is usable.
- Decision: do not promote r133. It is a diagnostic proof only. The next
  hierarchy step is a real lower-limb local-axis conversion before declaring
  pelvis/thigh/knee/ankle/toe solved again; arms/guitar remain unsolved.

## 2026-08-20 r123-r131 selective upper-channel diagnosis

- Commands ran at Idle/low priority. Captures used local
  `gh2_ps2_hybrid_assets/GEN` plus package-layout loose DLC only; no ISO was
  used, and package files were restored afterward.
- Rechecked the later r3 contract-fix body candidate against the new main-body
  gate. It rejects too. Direct inspection of the retained r3 idle/attack images
  confirms the old "body orientation review ready" verdict was too permissive:
  the legs are centered, but upper body/head/hands/guitar are still folded.
- Built three no-capture two-clip main-policy probes from pinned forcepartial
  bridges plus model-parent compensation:
  `targetlength`, `rootlocalaim`, and `axislocal`. `targetlength` was best,
  reducing neck angles, but head and guitar still failed. Evidence:
  `r126_mainbody_policy_sweep_summary_20260820.tsv`.
- Added `tools/gh3_midori_patch_acp_quats_identity.py` for scratch ACP
  diagnostics. Selective identity patch:
  idle = `bone_head.mesh.quat` + `bone_pos_guitar.mesh.quat`;
  attack = those plus both clavicle quats. This reduces the main-body gate to
  only guitar angle `120.72`; with diagnostic `--max-guitar-angle 121`, the
  two-clip gate passes.
- Direct capture of combined scratch candidate
  `r131_selective_identity_mainbody_visual_20260820` still rejects, but the
  improvement is real: head/torso are upright and face-forward instead of
  folded backward. Remaining failures are lower-limb deformation plus
  arms/guitar silhouette/contact.
- Decision: do not promote identity patches as final. The next aligned step is
  to convert this into a proper per-channel bind/local solve or selective
  channel suppression policy, then solve lower limbs and arms/guitar before
  returning to hand-overlay placement.

## 2026-08-20 r120-r122 main-body-only diagnosis

- Commands ran at Idle/low priority. Captures used local
  `gh2_ps2_hybrid_assets/GEN` plus package-layout loose DLC only; no ISO was
  used, and package files were restored afterward.
- Captured `controlroot_stockupper_full_candidate_20260819` with only
  `midori_1_medium_idle_f060` and `midori_1_attack_left_f030`. The native proof
  passed and showed visible attack-vs-idle motion (`sampled_idle_difference`
  about `0.05`), so main-body animation is being applied.
- Visual inspection still rejects the pair: the main bank already folds
  neck/head/arms/guitar before any fret/strum overlay. This invalidates the
  older "bipedal/framing pass" as an approval-grade gate for this candidate.
- Added `tools/gh3_midori_main_body_visual_sanity_gate.py`. It consumes
  actual-layer replay JSON and rejects excessive neck/head/clavicle/guitar
  angles from bind. Unit coverage was added in
  `tools/gh3_midori_pipeline_test.py`.
- r121/r122 evidence:
  `r120_mainbody_idle_attack_visual_20260820`,
  `r121_mainonly_idle_layer_replay_20260820.json`,
  `r121_mainonly_attack_layer_replay_20260820.json`,
  `r122_main_body_visual_sanity_20260820.json`, and
  `r122_main_body_visual_decision_20260820.json`.
- Gate result: reject with idle neck/head/guitar
  `150.20/123.53/129.22` degrees and attack
  neck/head/clavicles/guitar `135.95/97.97/107.25/114.64/129.22`.
- Decision: stop grinding hand-overlay roll/offsets until main-only
  idle/attack pass this upper-body sanity gate. Next work should rebuild or
  retarget the main bank's upper-body/guitar transform convention under the
  Control_Root/model-parent branch.

## 2026-08-20 r111-r119 roll/offset follow-up

- Commands ran at Idle/low priority. Captures used local
  `gh2_ps2_hybrid_assets/GEN` plus package-layout loose DLC only; no ISO was
  used, and the guarded wrapper restored package files after capture.
- Offset-only probes/captures (`r113`, `r115`) confirmed that lowering the
  whole cluster can satisfy coarse ratios without becoming a coherent pose.
  They remain visual rejects.
- A roll sweep around the pair-fit hand axis found two package-level structural
  passes, `-90` and `-60`. `-60` was the best capture candidate:
  `visible_hand_center_z_ratio=0.780404`,
  `target_center_z_ratio=0.800100`, `guitar_z_ratio=0.690948`,
  `guitar_head_distance=8.634809`, target-hand distances about
  `1.337/1.063`, and no tightened overlay-gate failures.
- Direct loose-DLC capture `r119_roll_m60_visual_20260820` still rejected.
  Runtime logs prove three active layers were loaded:
  `stand_medium_01`, `gh3_hnd_guit_strum_mido_norm_m01_d`, and
  `gh3_hnd_guit_chord_mid_bar3_d`, but the proof manifest failed
  `visible_difference_from_idle`. Visually this is idle/default body posture
  with the upper arms/head/guitar folded through each other.
- Decision: the user read is correct. Do not count r119 as animation success.
  The next diagnosis should split main-body pose application from hand-overlay
  application and prove visible body motion before further guitar roll/offset
  grinding.

## 2026-08-20 r96-r110 engine-HMX hand/guitar replay diagnosis

- Commands ran at Idle/low priority. Captures used local
  `gh2_ps2_hybrid_assets/GEN` plus package-layout loose DLC through
  `tools/gh3_midori_capture_with_loose_dlc_backup.py`; no ISO was mounted or
  used, and package hashes were restored afterward.
- Added `engine_hmx_replay_world` plus explicit `sample_quat_mode` and
  `clip_quat_storage_order` fields to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`. This catches solver-space
  false positives where direct replay reports hand/proxy contact but the GHC
  HMX-style replay does not.
- Key structural correction: the previous direct/xyzw branch could report
  `0.00005/0.00008` hand-target distances in direct solver replay while the
  engine-HMX replay was still `30.39/21.54`. A four-combo structural sweep found
  `--hmx-quat-mode transpose --clip-quat-storage-order xyzw --sample-quat-mode hmx`
  as the first near-contact engine-space convention (`0.000/1.821` before
  offset tuning).
- Built and captured corrected-model one-frame diagnostics:
  `r102_visual_20260820`, `r105_visual_20260820`, and `r110_visual_20260820`.
  All are upright/bipedal and fully layered. All remain direct visual rejects:
  the guitar/hand cluster is still above or behind the head rather than in a
  playable chest-level grip. r110 is framed but still overhead.
- Tightened `tools/gh3_midori_overlay_visual_sanity_gate.py` with maximum
  visible-hand, target-hand, and guitar z-ratio checks; r110 now rejects before
  capture via `guitar z ratio 1.179694 exceeds 1.150000`. Focused tests passed.
- Next branch: keep the engine-HMX convention (`transpose + xyzw + hmx`) and
  solve the cluster placement/orientation, not the body. The useful state is
  "upright body plus close hand/proxy contacts, but the guitar frame is
  over/behind the head." Do not capture more candidates unless the tightened
  overlay sanity gate passes.

## 2026-08-20 r93-r95 Control_Root model/clipset deployment diagnosis

- Commands ran at Idle/low priority. Captures used local
  `gh2_ps2_hybrid_assets/GEN` plus package-layout loose DLC through
  `tools/gh3_midori_capture_with_loose_dlc_backup.py`; no ISO was mounted or
  used. The wrapper restored the live package hashes after each capture.
- Root cause found for the r84-r92 horizontal/non-bipedal captures:
  candidate clipsets had `bone_pelvis.mesh` parented under `Control_Root`, but
  the deployed character model MILOs still had `bone_pelvis.mesh` root-parented.
  The retained upright source model has character and clipset pelvis parents
  both under `Control_Root`.
- Built corrected-model diagnostics by pairing retained
  `controlroot_stockupper_full_candidate_20260819` character models with prior
  test clipsets:
  `r93_r92clips_controlroot_models_candidate_20260820`,
  `r94_r86fullhands_controlroot_models_candidate_20260820`, and
  `r95_r87fullhands_controlroot_models_candidate_20260820`.
- Visual result: all three corrected-model captures are upright/bipedal again,
  so the old horizontal captures for the same animation branches were
  contaminated by the deployment contract mismatch. They are still direct visual
  rejects: r93 has disconnected/static diagnostic arms, r94 jams full-hand/guitar
  into the head/torso, and r95 leaves guitar/hands detached behind/right of the
  torso.
- Added a pre-deploy guard to
  `tools/gh3_midori_capture_with_loose_dlc_backup.py`: if the main clipset
  pelvis is parented to `Control_Root`, both outfit model MILOs must also parent
  `bone_pelvis.mesh` to `Control_Root`. The guard was regression-tested and
  confirmed to reject old r86 before any DLC deployment.
- Next branch: keep the corrected Control_Root character models for every
  capture. Continue hand/guitar diagnosis from upright r94/r95 evidence rather
  than from the invalid horizontal screenshots.

## 2026-08-20 r84 full-hand-bank quat convention / visual reject

- Commands ran at Idle/low priority. One single-case capture used local
  `gh2_ps2_hybrid_assets/GEN` plus package-layout loose DLC through
  `tools/gh3_midori_capture_with_loose_dlc_backup.py`; no ISO was mounted or
  used.
- Added diagnostic switches to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`:
  `--fit-guitar-to-final-replay-contacts`,
  `--final-replay-contact-fit-rotation-order`, and
  `--clip-quat-storage-order`. The default storage order preserves older probe
  output.
- Key structural result: retained upright body plus full canonical
  fret/strum hand and finger banks can replay with exact hand/proxy contact at
  the raw ACP level using `--hmx-quat-mode direct`,
  `--clip-quat-storage-order xyzw`, and direct replay. The compact retained
  baseframe structural probe is
  `r84_direct_xyzw_baseframe_probe_20260820`.
- Built and captured a one-frame candidate from that baseframe ACP probe:
  `r84_direct_xyzw_baseframe_visual_20260820/midori_1_hand_overlay_f010.bmp`.
  Direct visual reject: Midori is horizontal/non-bipedal at the bottom of the
  frame. The log confirms all three expected layers loaded, so this is a body
  preservation/build-path failure, not a layer-load failure.
- Next branch: do not repeat `build-clipset-from-acp` baseframe body rebuilds
  for this convention. Carry the direct+xyzw generated-quat finding into the
  retained-MILO sample/patch route used by r82, which kept the body upright,
  then patch only the full hand-bank/guitar channels.

## 2026-08-20 r83 retained canonical-hand structural probe

- Commands ran at Idle/low priority. No capture/game runtime was started. The
  current loose DLC is package-layout
  `gh2_ps2_hybrid_assets/DLC/community.gh3.midori`; package hashes match the
  known-good restored Midori MILOs. Do not use the stale flat `DLC/MIDORI` path
  as the hash probe.
- The r82/latest image is a one-frame retained-body diagnostic, not meaningful
  animation playback. It repeats retained upright `stand_medium_01` frame 10
  and patches guitar/hand probe channels, so default-pose body plus mangled
  arms is expected and is still a reject.
- Older minimal arm-chain branches were rechecked:
  `controlroot_stockupper_guitar_frame_hand_bake_srcanim_stockattach_propcomp_strings_armchain_pose_review_20260819`,
  `armchain_rotpos_pose_review_20260819`, and
  `mesh_armchain_rotpos_pose_review_20260819`. Position-only and rot+pos
  arm-chain payloads loaded but left visible hands near lower-body space. Do
  not repeat minimal endpoint/arm-chain-only bakes.
- Structural-only r83 probes:
  `r83_retained_canonical_hands_structural_probe_20260820` and
  `r83_retained_canonical_hands_finalreplay_structural_probe_20260820`.
  They use retained upright `controlroot_stockupper_full_candidate_20260819`,
  mesh channel names, `canonical-hands-visible-arm-rot`, full canonical
  fret/strum hand and finger banks, and constrained guitar contact frame.
- The final-replay variant can solve visible hands exactly to the final proxy
  worlds, but it is not capture-worthy: before that force-solve, left/right
  hands are 25.348 and 16.062 units from proxies, with target-center delta
  `[21.807, -4.106, 14.698]`. That is a structural warning that the guitar
  transform is still wrong and the final hand solve would likely mangle arms.
- Next branch: keep retained upright body and full native hand banks, but solve
  guitar placement/face around those full-bank hand/contact worlds before any
  visual capture. A GLB intermediate remains acceptable if automated, but the
  key missing piece is the native arm/hand channel contract plus a guitar frame
  constrained to those contacts.

## 2026-08-20 r82 upright-base constrained-frame isolation

- Commands ran at Idle/low priority. No ISO was mounted or used. Captures used
  local `gh2_ps2_hybrid_assets/GEN` plus ordinary loose DLC through the
  backup/restore wrapper, and loose hashes were verified restored afterward.
- Added `--include-runtime-aliases-for-selected` to
  `tools/gh3_midori_acp_stage.py` so selected one-clip subsets can emit runtime
  aliases such as `stand_medium_01` without staging all 331 clips. Added a
  source-level regression in `tools/gh3_midori_pipeline_test.py`.
- Diagnosed the subset rebuild problem: newly regenerated `stand_medium_01`
  differed from retained `controlroot_stockupper_full_candidate_20260819`.
  Retained frame 10 has identity `bone_pelvis.mesh.quat`; regenerated subset
  had a large pelvis rotation and rendered sideways.
- Built a diagnostic `stand_medium_01` ACP by sampling retained
  `controlroot_stockupper_full_candidate_20260819/gh3_midori_main.milo_ps2`
  frame 10 and repeating it over a short clip. Patched the r81 constrained
  guitar/hand channels onto that retained-body base. The candidate preserved the
  retained upright pelvis sample and loaded all three expected layers.
- Direct visual reject:
  `r82_retained_stand_constrained_visual_20260820/midori_1_hand_overlay_f010.bmp`.
  Body is upright/bipedal again, but the guitar is still behind/through the
  torso and hands/arms do not grip. The constrained relation-target `+Z/+Y`
  frame is not enough.
- Next branch: keep the retained-MILO sampling path for bounded one-frame
  diagnostics, but stop treating the relation-fit target line plus projected
  body-front hint as sufficient. Solve visible arms/hands and guitar together,
  or derive a stronger source/stock prop face/front constraint before the next
  capture.

## 2026-08-20 r81 constrained contact-frame probe

- Commands ran at Idle/low priority. No ISO was mounted or used. The single
  runtime capture used local `gh2_ps2_hybrid_assets/GEN` plus ordinary loose
  DLC through the backup/restore wrapper, and loose hashes were verified
  restored afterward.
- Added `constrained-contact-frame` rotation mode to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`. It maps stock model
  local `+Z` to the intended relation-fit neck/contact line and locks roll by
  mapping local `+Y` to a projected body-front/depth hint.
- Added regression
  `test_constrained_contact_frame_maps_stock_z_to_neck_axis`; focused tests for
  relation-fit registration, r80 axis diagnosis, and constrained-frame math
  passed.
- Structural probe retained:
  `r81_constrained_contact_frame_probe_20260820/guitar_frame_hand_bake_manifest.json`.
  It has positive determinant and the final-parent relation-fit resolver lands
  visible palms and fret/strum proxies exactly on the desired relation-fit
  worlds.
- A temporary visual candidate built from the generic
  `analysis/gh3_midori_acp_stage` was captured and rejected:
  `r81_constrained_contact_frame_visual_20260820/midori_1_hand_overlay_f010.bmp`.
  The body is sideways/non-bipedal, so this proves the generic stage is the
  wrong base for visual judging. It does not prove the constrained guitar frame
  itself is visually bad.
- Attempted to regenerate a full Control_Root/source-IK-helper stage from local
  retained inputs; the all-clip stage command ran too long without bounded
  output and was stopped. Rebuildable r81 stage/candidate directories were
  deleted.
- Next branch: patch the constrained frame into the retained upright
  `controlroot_stockupper_full_candidate_20260819` MILOs directly, or regenerate
  only the needed upright Control_Root clip/stage subset. Avoid more roll/offset
  visual sweeps and do not use the sideways generic-stage capture as a quality
  signal.

## 2026-08-20 r80 guitar-axis diagnostic

- Commands were run at Idle/low priority. No ISO was mounted or used. This
  branch read retained source/stock JSON/log evidence and verified the loose
  DLC hashes still match restored r74.
- Latest r79 capture context: the contact sheet is a one-frame animation
  diagnostic, not a pure static default pose. `--emit-base-frame-channels`
  preserves the retained Control_Root candidate's sampled main/fret/strum
  channels, then probe channels override guitar/hand targets. Direct visual
  rejection remains correct because the result still reads default-like with
  mangled arms and a bad prop frame.
- Added `tools/gh3_midori_guitar_axis_diagnostic.py` and regression
  `test_guitar_axis_diagnostic_derives_stock_z_neck_axis`.
- Diagnostic result: stock GH2 Xplorer local neck/contact axis is consistently
  `+Z` under `bone_pos_guitar.mesh` (`strum_to_fret`,
  `strum_hand_to_fret_hand`, `fret20_to_fret01`). The source `anim`-basis IK
  hand vector is closest to `-X`, and the accepted relation-fit
  right-target-to-left-target vector is `+X`.
- Evidence retained:
  `r80_guitar_axis_diagnostic_20260820.json`.
- Next branch: solve a complete target guitar world frame from hard axes before
  any capture. Map stock model local `+Z` to the intended hand/neck contact
  axis, add a face/front/depth constraint, then emit `bone_pos_guitar.mesh.quat`.
  Do not continue roll/offset-only visual sweeps.

## 2026-08-20 r79 guitar-frame sweep rejection

- Commands were run at Idle/low priority. No ISO was mounted or used for
  runtime. Temporary captures used local `gh2_ps2_hybrid_assets/GEN` plus
  ordinary loose DLC and restored the r74 hashes afterward.
- Added `--relation-fit-world-offset` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py` so the accepted
  relation-fit guitar, visible palms, and proxy contacts can move together.
- Tested bounded variants after r78:
  prop-only offsets, whole relation-cluster offsets, alternate source rotation
  bases (`anim`, `helper`), stock GH2 local guitar frame, visible-arm solve on
  the best `anim` source basis, and accepted relation-fit roll rows 1-5.
  Direct visual rejects all of them.
- Best gross orientation is still source rotation basis `anim`; it creates a
  diagonal guitar-like plane instead of a horizontal/vertical blade, but the
  prop still cuts through Midori's torso and the arms do not grip it.
- Evidence retained as contact sheets and a decision JSON:
  `r79_sourceguitarrot_offset_contact_sheet_20260820.jpg`,
  `r79_relation_cluster_offset_contact_sheet_20260820.jpg`,
  `r79_sourcebasis_contact_sheet_20260820.jpg`,
  `r79_anim_vs_stock_guitar_frame_contact_sheet_20260820.jpg`,
  `r79_anim_armrot_contact_sheet_20260820.jpg`,
  `r79_anim_basis_relation_roll_contact_sheet_20260820.jpg`, and
  `r79_guitar_frame_sweeps_visual_decision_20260820.json`.
- Next branch: stop doing visual roll/offset sweeps. Derive the guitar mesh or
  prop local axes from source/stock geometry, identify neck axis and face normal
  explicitly, then solve a target world frame with neck/contact/body-front
  constraints before emitting `bone_pos_guitar.mesh.quat`.

## 2026-08-20 r78 full-frame/HMX relation-fit rotation rejection

- Commands were run at Idle/low priority. No ISO was mounted or used for
  runtime. Temporary captures used local `gh2_ps2_hybrid_assets/GEN` plus
  ordinary loose DLC and restored the r74 hashes afterward.
- Extended `tools/gh3_midori_guitar_frame_hand_bake_probe.py` with
  `--emit-base-frame-channels` so one-case ACP diagnostics can preserve the
  retained Control_Root candidate's sampled main/fret/strum frame before
  applying probe overrides. Also resolved relation-fit palms/proxies under the
  final HMX parent-space replay, matching `actual_layer_replay_report`'s packed
  XYZW interpretation.
- Full-frame/HMX parent-resolved position-only capture is bipedal and no
  longer sparse/default-only, but visual rejects it: the guitar is still
  vertical/through the body. Evidence:
  `r78_relation_fit_fullframe_hmx_parentresolved_visual_decision_20260820.json`.
- Tested the exact source `bone_guitar_body` frame 10 rotation from the r47
  `c_med_idle` bridge. It moved the guitar from vertical to horizontal, proving
  rotation is being emitted, but the guitar remains edge-on/black and slices
  through the torso/head. Direct visual rejects it.
- Ran a bounded six-shot source-guitar post-roll sweep:
  `x-90`, `x-270`, `y-90`, `y-270`, `z-90`, `z-270`. All rejected; none put
  the guitar into a believable playing pose. Evidence:
  `r78_sourceguitarrot_sweep_contact_sheet_20260820.jpg` and
  `r78_sourceguitarrot_sweep_visual_decision_20260820.json`.
- Next branch: stop treating this as a simple source rotation roll. Derive a
  full guitar world frame/offset from visible hand/contact axes plus body/camera
  facing, then solve visible arms/hands to that frame. Position-only relation
  fit and raw source-local guitar rotation are both insufficient.

## 2026-08-20 r77 relation-fit probe and sparse visual rejection

- Commands were run at Idle/low priority. No ISO was mounted or used for
  runtime. Temporary loose-DLC capture used local `gh2_ps2_hybrid_assets/GEN`
  plus the ordinary loose DLC backup/restore harness, and restored the r74
  hashes afterward.
- Fixed `tools/gh3_midori_relation_fit_feasibility.py`: the sweep row append
  was accidentally outside the `roll_degrees` loop, and the diagnostic now
  supports `spine3` and `visible-hand-center` anchors. The r77 upper-anchor
  sweep reports `accepted=18` structural passes; the best tested row anchors
  the source hand/guitar cluster near `spine3` at scale `0.18`.
- A full ACP-stage promotion using the current `analysis/gh3_midori_acp_stage`
  was rejected before capture because it collapsed the upper body in actual
  replay (`head_spine3=6.513`). Rebuilding the prior-looking Control_Root knobs
  also did not reproduce the retained bipedal candidate (`head_spine3=13.132`,
  head mostly sideways), so do not use that rebuilt stage as approval evidence.
- Added `relation-fit-world` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py` and built a sparse
  one-case overlay from the retained bipedal Control_Root+r74-proxy MILOs. It
  samples structurally clean (`suspicious=none`, `guitar_head=11.942`), but
  direct visual capture rejects it: body reads default/sparse and the guitar
  appears as a vertical strip through the head/body rather than a playable
  attachment. Evidence:
  `r77_relation_fit_world_onecase_visual_decision_20260820.json` and
  `r77_relation_fit_world_onecase_visual_20260820/midori_1_hand_overlay_f010.bmp`.
- Next branch: keep the relation-fit result as a useful source-contact cluster,
  but do not promote sparse one-case overlay output. Carry the fitted
  guitar/contact frame into the known-good full Control_Root animation basis
  or add a MILO-level/full-stage patch path that preserves that basis.

## 2026-08-20 r76 Control_Root pair-fit one-case rejection

- No ISO was mounted or used. Runtime capture used local
  `gh2_ps2_hybrid_assets/GEN` plus ordinary loose DLC only at Idle/low
  priority, and loose DLC was restored to the r74 hashes afterward.
- Ran guitar/IK contract reports on the r75 Control_Root+r74-proxy branch.
  `midori_1_hand_overlay_f010` direct mode reports `LH-FRET=25.552`,
  `RH-STRUM=25.889`, and `solved_guitar_anchor_fret_vs_strum_delta=147.164`.
  Forcing stock hand-target locals still leaves `136.547`; HMX quat
  materialization worsens to `152.704`. This proves the failure is not a
  simple whole-guitar offset or the stale r74 strum local.
- Built a one-case pair-fit probe with
  `gh3_midori_guitar_frame_hand_bake_probe.py`: `hand-target-pair-midpoint`,
  `hand-target-pair-fit`, `current-proxies`,
  `canonical-hands-visible-arm-rot`, mesh channel names, direct quat mode. It
  moved the target center by `[16.654067, -10.170646, 13.806205]` and solved
  guitar world near `[7.619, -13.402, 56.205]`.
- Direct one-case visual capture rejects it. The body stays upright, but the
  guitar is still slung behind/over the left shoulder and the visible arms do
  not grip it. Evidence:
  `r76_pairfit_onecase_visual_decision_20260820.json` and
  `r76_controlroot_pairfit_onecase_visual_20260820/midori_1_hand_overlay_f010.jpg`.
- Next branch: do not continue plain current-proxy two-point pair-fit. Use a
  source-authored guitar orientation/contact frame or relation-fit solve that
  constrains prop front/neck direction as well as fret/strum contact.

## 2026-08-20 r75 post-r74 visual/Control_Root probe

- No ISO was mounted or used. Runtime captures used local
  `gh2_ps2_hybrid_assets/GEN` plus ordinary loose DLC only at Idle/low
  priority.
- Fresh capture of the currently deployed r74 loose DLC passed the automated
  pose-review manifest (`9` proofs, `0` failures) but fails direct visual
  inspection: most full-body captures are sideways/non-bipedal; the hand-overlay
  case is upright but still has obvious arm/guitar alignment failure. Evidence:
  `r75_post_r74_pose_review_20260820/contact_sheet.jpg`.
- Actual current loose replay shows the remaining body-frame mismatch:
  `bone_pelvis.mesh` is still package-root parented in both character and
  clipset (`pelvis_parented=False`), while source `Bone_Pelvis` is under
  `Control_Root`. Multiclip root/pelvis diagnostics confirm the guitar clips
  exercise large pelvis/lower-body motion on that mismatched target graph.
- Reversible probe: copied retained
  `controlroot_stockupper_full_candidate_20260819`, patched only its two model
  MILOs with the r74 guitarist-proxy fix, temporarily deployed it, captured the
  same 9 cases, then restored the r74 loose DLC hashes. The probe replays as
  `pelvis_parented=True`, `pelvis_static=True`, `spine_dynamic=True` and
  restores upright/bipedal body captures, but direct visual inspection still
  rejects final approval because guitar/hand placement is visibly offset or
  behind the performer. Evidence:
  `r75_controlroot_r74proxy_pose_review_20260820/contact_sheet.jpg`.
- Restored loose DLC hashes:
  main `F1A06A0E9507023D7F631598693D9F43C47C00A2982068737EDE67CF7452F598`;
  fret `6B3E7A20C72F6E3EADB62A329131FEE324A475F0C219637B9B2A4BC7BBAE3252`;
  strum `06340333FEA39BBAFC83FB4B4F508A9793B483A98B2FEE0D6C30A79C4A14715F`;
  ui `4A1B16DA5EC1FDC52CB7B66B4E0D0F49569259C43631FD9C07F1A17A13DFD32F`;
  model1 `B7014B8CD499EC27434D0E428716FA00DDED311E762034B08C3C1AA58EA6E759`;
  model2 `D7A45AB0D004ABDE1C71E414766F79BF7DE8C568F18ADC18CA5FA3A59E0230FE`.
- Next branch: use the Control_Root-parented bipedal branch as body-frame base
  and continue solving guitar/hand attachment alignment. Do not treat current
  deployed r74 flattened-parent captures as approval.

## 2026-08-20 r74 strum-hand proxy model fix

- No ISO was mounted or used. Rebuilt `milo_convert_tool`, patched model MILOs
  from the existing loose DLC tree in scratch, and deployed only the two model
  MILOs after graph verification.
- Current loose pre-fix graph isolated one model bug: `bone_strum_hand.mesh`
  local was `[-1.45011, -0.169293, 10.8653]` instead of stock GH2
  `[-6.73834944, -1.31678379, -3.08711553]`, causing
  `14.965029589335648` local distance. Other measured guitar/controller anchors
  already matched stock.
- Fixed stale hardcoded strum-hand proxy local in
  `GuitarHeroOGX-main-ui-engine/tools/milo_convert/milo_convert_tool.cpp` in
  both `patch_guitarist_proxy_transforms()` and
  `generated_guitar_controller_local()`.
- Applied patched `milo_convert_tool patch-guitarist-proxies` to both loose
  model MILOs and deployed the outputs. New model hashes:
  `gh3_midori_1.milo_ps2 = B7014B8CD499EC27434D0E428716FA00DDED311E762034B08C3C1AA58EA6E759`;
  `gh3_midori_2.milo_ps2 = D7A45AB0D004ABDE1C71E414766F79BF7DE8C568F18ADC18CA5FA3A59E0230FE`.
  Existing main/fret/strum/ui animation MILO hashes stayed unchanged.
- Post-deploy graph reports for both outfits pass: pelvis, guitar, fret,
  strum, fret-hand, and strum-hand anchors all report `local=0.000 world=0.000`
  against stock GH2 after xplorer overrides. Diagnosis now says the
  guitar-hand parent graph matches stock within threshold.
- Added pipeline regression
  `test_milo_convert_uses_stock_strum_hand_proxy_local` and fixed stale
  diagnosis text in `tools/gh3_midori_guitar_anchor_graph_report.py`.
- Evidence:
  `r74_current_loose_guitar_anchor_graph_report_20260820.json`,
  `r74_deployed_guitar_anchor_graph_report_20260820.json`, and
  `r74_deployed_guitar_anchor_graph_report_midori2_20260820.json`.
- Next branch: resume runtime/candidate review with the guitar controller
  parent graph fixed. r74 is not visual approval.

## 2026-08-20 r73 hand-root correspondence audit

- No ISO was mounted or used. This pass consumed retained r72 bridge JSON and
  existing candidate MILOs through local `milo_convert_tool.exe` only.
- Fixed `tools/gh3_midori_source_local_frame_bridge_report.py` after bake-probe
  API drift by supplying `sample_quat_mode="direct"` in its internal probe args;
  added a focused pipeline regression.
- Ran source-local fit on r72 `midori_1_attack_left_f030` frames `0,30` against
  the existing `controlroot_stockupper_full_candidate_20260819` candidate.
  Direct source-local pairs reject: `ik_helper rms=13.793709 max=18.355553`;
  `palm rms=11.03664 max=14.35687`.
- Ran r72 source guitar contract. Source IK-helper guitar locals are much too
  small for GH2 stock controller locals: left source mean
  `[0.077938, 0.257191, 0.019462]` vs stock
  `[-6.27464, -0.453556, -4.32057]` (`delta_len=7.726337`); right source mean
  `[-0.087862, 0.000002, 0.019791]` vs stock
  `[-6.73835, -1.31678, -3.08712]` (`delta_len=7.4576`).
- Added `tools/gh3_midori_hand_root_mode_report.py` to compare stage
  `--hand-root-position-source` modes against GH2 hand-reference rows. For
  `midori_1_attack_left_f030`, source helper modes are not better:
  left `prop-local=5.666294`, `source-ik-helper=5.760625`,
  `source-ik-helper-gh2scale=9.693967`; right `prop-local=2.884601`,
  `source-ik-helper=4.74687`, `source-ik-helper-gh2scale=8.280853`.
- Decision: do not emit direct source-helper hand-root proxies in the next
  visual candidate. Keep GH2 hand-reference/proxy locals for
  `bone_fret_hand`/`bone_strum_hand`; use GLB source helpers as guitar-frame
  diagnostics unless a richer role-specific rebase is implemented.
- Evidence:
  `r73_source_local_frame_bridge_attack_report_20260820.json`,
  `r73_source_guitar_contract_report_20260820.json`, and
  `r73_hand_root_mode_report_20260820.json`.

## 2026-08-20 r72 guitar-neck anchor/frame-fit diagnostic

- Added `bone_guitar_neck`, `BONE_GUITAR_FRET_POS`, and
  `BONE_GUITAR_STRUM_POS` to the focused source bridge export defaults.
  Re-exported `midori_1_attack_left_f030` frames `0,30` with pinned NXTools at
  `C:\Users\smmel\AppData\Local\Temp\nxtools_ref`; export succeeded with `58`
  pose records. The GH3 ISO was extraction-only for Blender/NXTools, not
  runtime. Extracted inputs, generated GLB, and logs were removed after
  retaining the small pose JSON/manifest.
- Extended `tools/gh3_midori_guitar_helper_contract_report.py` with
  parent-anchor and source-only rows. r72 proves the source guitar endpoints:
  `bone_guitar_body` coincides with `BONE_GUITAR_STRUM_POS`, and
  `bone_guitar_neck` coincides with `BONE_GUITAR_FRET_POS`.
- Added `tools/gh3_midori_guitar_frame_fit_report.py`. Best frame-30 fit from
  source guitar frame to GH2 helper bind frame uses `scale=91.183463`,
  `source_hint=fret_hand`, `target_hint=strum_hand`,
  `target_normal_sign=-1`, with `rms_error=4.884814` and
  `max_error=6.431115` GH2 units. Residuals: neck/fret `3.146083`, fret hand
  `6.431115`, strum hand `4.508564`.
- Focused tests/compile passed for the modified/default export path and new
  diagnostics.
- Evidence:
  `r72_guitar_neck_anchor_contract_report_20260820.json`,
  `r72_guitar_frame_fit_report_20260820.json`,
  `r72_guitar_neck_anchor_bridge_20260820/review_source_bridge_batch_manifest.json`,
  and
  `r72_guitar_neck_anchor_bridge_20260820/midori_1_attack_left_f030/gh3_guit_mido_a_attackl.ska.pose_bridge.json`.
- Next branch: do not promote a visual candidate from r72. Either solve a
  non-uniform/role-specific guitar helper mapping from the source frame, or
  audit whether GH2 `bone_fret`/`bone_strum`/hand-helper correspondences are the
  right targets for GH3 `bone_ik_hand_guitar_l/r`.

## 2026-08-20 r70 guitar-helper bridge/contract diagnostic

- Added `bone_guitar_body`, `bone_ik_hand_guitar_l`, and
  `bone_ik_hand_guitar_r` to the default export list in
  `tools/gh3_midori_review_source_bridge_batch.py`.
- Re-exported focused `midori_1_attack_left_f030` source bridge frames `0,30`
  with pinned NXTools at `C:\Users\smmel\AppData\Local\Temp\nxtools_ref`.
  Export succeeded with `52` pose records, including the three guitar helpers,
  `Control_Root`, and `Bone_Head`. The GH3 ISO was used only to extract source
  inputs for this bridge; no game/runtime capture used an ISO. Extracted source,
  generated GLB, and logs were deleted after retaining the pose JSON/manifest.
- Added `tools/gh3_midori_guitar_helper_contract_report.py` to compare mapped
  source helper vectors against GH2 target helper bind vectors for
  `bone_pos_guitar.mesh`, `bone_fret_hand.mesh`, and `bone_strum_hand.mesh`.
- r70 contract result: helper records exist but do not match the GH2 helper
  target-space directly. At attack frame 30, worst segment is
  `fret_to_strum_hand` with `direction_dot=-0.866264`,
  `angle=150.027336`, `length_ratio=0.002058`; `guitar_to_fret_hand` is also
  opposed (`dot=-0.846395`), and `pelvis_to_guitar` is opposed
  (`dot=-0.778258`).
- Focused tests passed:
  `python -m unittest tools.gh3_midori_pipeline_test.MidoriPipelineTest.test_review_source_bridge_defaults_include_guitar_helpers tools.gh3_midori_pipeline_test.MidoriPipelineTest.test_target_bind_pose_preserves_structural_head_source`.
- Evidence:
  `r70_guitar_helper_contract_report_20260820.json`,
  `r70_guitar_helper_bridge_20260820/review_source_bridge_batch_manifest.json`,
  and
  `r70_guitar_helper_bridge_20260820/midori_1_attack_left_f030/gh3_guit_mido_a_attackl.ska.pose_bridge.json`.
- Next branch: explain the source guitar-local parent frame by exporting or
  reconstructing `BONE_GUITAR_FRET_POS` / `BONE_GUITAR_STRUM_POS`, or derive
  the equivalent prop-anchor basis, then solve GH2
  `bone_pos_guitar`/fret/strum helper target-space before another visual
  candidate capture.

## 2026-08-20 r69 structural head bind/target-graph diagnostic

- Found a target-bind blind spot: structural `Bone_Head` was absent from
  `target_bind_pose_by_source_name()` because multiple GH3 facial aliases map to
  GH2 `bone_head`, leaving `bone_head(.mesh)` tagged as `Bone_Lip_Upper_Mid`.
- Added a narrow fallback in `tools/gh3_midori_acp_stage.py` that preserves
  `Bone_Head` from `bone_head.mesh`/`bone_head` under `Bone_Neck`, plus a
  regression in `tools/gh3_midori_pipeline_test.py`.
- Extended `tools/gh3_midori_target_graph_solve_validator.py` with
  `target_graph_head`, adding the `Bone_Neck -> Bone_Head` segment to the
  existing pelvis/torso/leg solve.
- Five-case GLB/source bridge validator now passes with head included:
  `status=target_graph_solve_valid`, `passing=40`; best row has
  `compose=local_desired_t`, `pelvis=bind`, `aimfix=transpose_post_t`,
  `max_position_error=0.0`, `min_segment_dot=1.0`,
  `min_selected_child_aim_dot=1.0`, `max_rot=0.021455`.
- Focused tests passed:
  `python -m unittest tools.gh3_midori_pipeline_test.MidoriPipelineTest.test_target_bind_pose_preserves_structural_head_source tools.gh3_midori_pipeline_test.MidoriPipelineTest.test_torso_bridge_leg_ik_policy_is_registered`.
- Evidence:
  `r69_head_target_graph_contract_report_20260820.json`. The full raw
  validator dump was deleted as rebuildable bulk evidence after its summary was
  retained in this report.
- Next branch: export or consume source-authoritative guitar helper pose records
  (`bone_guitar_body`, `bone_ik_hand_guitar_l`, `bone_ik_hand_guitar_r`) and
  extend the contract diagnostic to GH2 `bone_pos_guitar.mesh`,
  `bone_fret_hand.mesh`, and `bone_strum_hand.mesh`. Do this before another
  visual candidate capture.

## 2026-08-20 r68 runtime pose/mesh diagnostic boundary

- Added diagnostic-only native viewer support for
  `--char-pose-mesh-dump <jsonl>` in
  `GuitarHeroOGX-main-ui-engine/engine/src/app/app_main.cpp`, plus
  `tools/gh3_midori_pose_review.py --pose-mesh-dump-dir` passthrough. The dump
  records actually drawn skinned mesh posed bounds and face-height-band bone
  attribution at the screenshot frame.
- Built `ghogx_app` successfully under the VS developer environment and ran one
  focused `midori_1_hand_overlay_f010` loose-DLC diagnostic capture from local
  `gh2_ps2_hybrid_assets/gen` with `GHOGX_ADDONS_DIR=gh2_ps2_hybrid_assets/DLC`;
  no mounted ISO path was used at runtime.
- r68 is diagnostic only, not a candidate promotion. The pose-review manifest
  still fails `visible_difference_from_idle` with min margin `38`.
- Runtime face-band attribution rejects the stale forearm/palm lead:
  aggregate face-height weights are `bone_head.mesh=1372.844`,
  `bone_spine3.mesh=400.8`, `bone_neck.mesh=83.1`,
  `bone_L-clavicle.mesh=63.1`, `bone_L-upperArm.mesh=51.9`, and only
  `bone_L-foreArm.mesh=2.2`. The only positive-y face-band cluster is
  `midori_1_mesh0_part13.mesh`, mostly left upper arm; the broad
  `midori_1_mesh0_part16.mesh` bound has head/detail-weighted face-band
  vertices.
- Evidence:
  `r68_runtime_pose_mesh_focus_report_20260820.json`,
  `r68_pose_mesh_dump_jsonl_20260820/midori_1_hand_overlay_f010.jsonl`,
  `r68_pose_mesh_dump_visual_20260820/midori_1_hand_overlay_f010.bmp`, and
  `r68_pose_mesh_dump_review_20260820.json`.
- Next branch: return to the shared Control_Root/pelvis/neck/head/guitar
  target-space contract using GLB/source-authoritative pose comparison or a
  target-graph local transform validator. Do not spend the next iteration on
  palm-only or simple arm-aim edits.

## 2026-08-20 r67 arm-aim and mesh-influence boundary

- Added `--aim-arm-rotations` and `--aim-arm-rotation-side` to
  `tools/gh3_midori_patch_relation_fit_worlds_into_stage.py`. This computes
  upper/forearm aim rotations from solved segment vectors and emits them as ACP
  `.quat` samples.
- r67 reused r66's reference-biased left hand and both-forearm solve, then
  aimed left upperArm/foreArm. Offline replay passed the segment-aware gate and
  confirmed the left upper/forearm rotations changed to about `39/45` degrees
  with reference-like lengths.
- Direct visual still rejects r67. Runtime layer signatures changed, so the
  candidate clips loaded, but the r66/r67 BMPs differ by only `247` bytes and
  the face-crossing silhouette is effectively unchanged.
- Added `r67_mesh_influence_focus_report_20260820.json`: many left
  palm/forearm-weighted mesh parts are low in bind space, while face-height
  purple/occluding regions include clavicle/upper-arm and GH3/head detail
  weights.
- Resume by adding a runtime-space pose/mesh debug dump or by targeting the
  exact face-height occluder mesh/bone influence set. Do not spend the next
  branch on palm-only or simple arm-aim candidate captures.

## 2026-08-20 r65-r66 constrained/reference hand diagnostics

- Added optional hand-position controls to
  `tools/gh3_midori_patch_relation_fit_worlds_into_stage.py`:
  `--min-upper-hand-distance`/`--constrain-upper-hand-side`, and
  `--reference-hand-distance`/`--reference-hand-direction-side`.
- r65 pushed the smaller accepted left palm outward to clear the segment gate
  (`left_upper_to_hand=6.999989`) while keeping right-arm IK. Direct visual
  still rejects it because the left sleeve/hand remains across the face.
- r66 changed strategy: left palm was placed along the retained reference
  upper-arm-to-hand direction at distance `10.0`, with both forearms solved.
  Offline replay moved the left hand substantially to
  `(-1.232919, -5.329112, 46.440652)` and passed the segment-aware gate.
  Runtime fret-layer signature changed versus r64, confirming the clip loaded,
  but the r64/r66 screenshots differ by only `389` bytes. Direct visual still
  rejects r66.
- Resume by stopping palm-position-only grinding. Patch visible left
  upperArm/foreArm aim rotations from the solved segment vectors, or add a
  runtime-space pose dump to identify which visible mesh/bone drives the
  face-crossing sleeve. Treat offline hand-position replay as insufficient until
  it correlates with rendered pixels.

## 2026-08-20 r62-r64 arm IK and segment-aware rejection

- Added arm segment checks to `tools/gh3_midori_overlay_visual_sanity_gate.py`.
  Actual MILO replays now reject:
  `forearm_to_hand > 14.0` and `upper_to_hand < 6.0`. This catches the visual
  classes that produced r61 hand spikes and r62/r64 face-folding before capture.
  Focused overlay tests pass.
- Added `--solve-arm-forearms` plus `--solve-arm-forearm-side` to
  `tools/gh3_midori_patch_relation_fit_worlds_into_stage.py`. The solver places
  forearm/elbow points with reference upper-arm and forearm segment lengths
  while retaining the accepted hand position.
- r62 solved both forearms. It fixed the measured right-hand stretch but folded
  the left arm across the face. The new gate rejects it:
  `left_upper_to_hand=4.341531 < 6`.
- r63 solved only the right forearm. It fixed the right segment length without
  improving the rendered pose enough; direct visual still rejects it.
- r64 used the smaller accepted relation-fit row (`prop_scale_ratio=0.10`) plus
  right-arm IK. The smaller cluster reduced the target distances, but direct
  visual still rejects it and the new gate catches the left-arm compression:
  `left_upper_to_hand=4.511182 < 6`.
- Resume by keeping the r61 body/arm rotation solve and the new segment-aware
  gate, but do not blindly preserve the accepted left palm from either accepted
  relation-fit row. Next branch should constrain both forearm-hand and
  upper-hand distances before capture, or split the fit: reference-biased
  left/fret arm plus independently fitted guitar/right-strum side.

## 2026-08-20 r59-r61 relation-fit body/arm rotations and visual rejection

- Extended `tools/gh3_midori_patch_relation_fit_worlds_into_stage.py` to solve
  local `.quat` ACP samples from retained replay world rotations. This preserves
  the old position-only behavior unless the new rotation flags are supplied.
- r59 added r41b body rotations plus guitar rotation to r58's body-position and
  accepted relation-fit cluster. Replay passed and restored upright/bipedal
  body coherence, but direct visual capture still rejected it because the
  forearm/hand/guitar cluster remained malformed.
- r60 added arm-frame rotations and forearm positions, but the strum layer
  solved right hand position before applying the right forearm rotation. It was
  rejected pre-capture by the overlay gate (`hand_ratio=1.204236`).
- r61 corrected the strum-layer order so right clavicle/upper/forearm rotations
  are applied before right hand/target positions. Replay passed with no
  suspicious nodes and retained the accepted hand/target cluster:
  left hand `(-3.865619, 2.787378, 53.463442)`,
  right hand `(-8.603565, 5.437462, 57.980519)`,
  fret target `(-2.967461, 2.137803, 49.686966)`,
  strum target `(-4.224256, 2.304937, 50.372323)`.
- One no-ISO local-GEN visual capture was run for r61. Direct visual inspection
  still rejects it: body/left arm are improved, but the guitar still occludes
  too much upper body and the right hand/finger geometry is spiky/incoherent.
- Resume from the r61 branch by keeping body/arm-frame rotations, then diagnose
  guitar local rotation/scale and hand-detail/finger rotations. Do not return to
  pelvis-only/body-position patching as the next branch.

## 2026-08-20 r57/r58 relation-fit ACP replay and visual rejection

- Added `tools/gh3_midori_patch_relation_fit_worlds_into_stage.py` to patch
  accepted r56 relation-fit worlds into editable ACP `.pos` rows.
- r57 patched only the accepted r56 guitar/hand/target cluster into the current
  matrix-local stage. The worlds survived local solving, but the current body
  frame is still collapsed: `head_spine3=6.512686`, `guitar_head=17.339225`.
  Rejected before capture.
- r58 patched r41b body world-position rows
  (`pelvis/spine1/spine2/spine3/neck/head`) plus the accepted cluster. It
  passed the stricter post-MILO overlay gate:
  `head_spine3=11.674564`, `guitar_head=11.955744`,
  `LH_to_fret=3.935799`, `RH_to_strum=9.320675`.
- One no-ISO local-GEN capture was run for `midori_1_hand_overlay_f010` because
  r58 passed the gate. Direct visual inspection rejects it:
  body is folded/side-on, guitar occludes the upper body, and the silhouette is
  not bipedal/coherent. Evidence:
  `r58_relation_fit_body_visual_20260820/midori_1_hand_overlay_f010.bmp` and
  `r58_relation_fit_body_visual_decision_20260820.json`.
- Resume by solving matching body rotations/orientation with the accepted
  relation-fit cluster, or add a body-orientation/silhouette gate before
  another capture. Position-only body-frame patching is insufficient.

## 2026-08-20 r56 same-frame relation fit feasibility

- Added `tools/gh3_midori_relation_fit_feasibility.py` to test source
  palms/IK helpers/`bone_guitar_body` as one same-frame cluster before MILO
  packing.
- Pelvis-anchored source preservation is impossible at the retained final body
  scale: source pelvis-to-head is `0.520488`, r41b final pelvis-to-head is
  `21.591641`, and even reduced pelvis-anchored fits miss the stricter
  `guitar_to_head <= 14` gate.
- r41b base-guitar-anchored cluster fitting is feasible:
  `prop_scale_ratio=0.15`, `roll_degrees=330`,
  `guitar_head=11.955784`, left hand/target distance `3.935828`, right
  hand/target distance `9.320717`. A smaller `0.10` cluster also passes.
- Accepted `0.15` target worlds:
  `guitar=(-7.601438, 3.992937, 51.964199)`,
  `left_palm=(-3.865620, 2.787372, 53.463444)`,
  `right_palm=(-8.603568, 5.437445, 57.980525)`,
  `left_target=(-2.967433, 2.137773, 49.686935)`,
  `right_target=(-4.224255, 2.304934, 50.372321)`.
- `milo_convert_tool` has no ACP extraction command, so retained MILO
  candidates cannot be decompiled into editable stages directly.
- Resume by synthesizing an editable one-frame ACP/probe manifest from the
  accepted r41b-anchored cluster and feeding that through the existing ACP
  writer. Do not use pelvis-anchored source scale or retained candidate MILO
  decompilation as the next route.

## 2026-08-20 r55 retained-candidate audit and stricter no-capture gate

- No r48/r41b full ACP stage is retained. Retained evidence includes compact
  reports, r47 source bridge JSON/GLB files, visuals, and a few older candidate
  MILO directories.
- Replayed retained candidates directly. They are not suitable bases:
  `controlroot_stockupper_full_candidate_20260819` has
  `head=11.592237`, `guitar=23.142596`, `LH=47.388798`,
  `RH=34.429409`; `controlroot_nostock_full_candidate_20260819` has
  `head=11.562576`, `guitar=23.235512`, `LH=38.465553`,
  `RH=21.875854`; exactbase/repro candidates have `guitar=26.510836` and
  `LH=52.016120`.
- Directly inspected retained r34/r36c/r40/r41b overlays. r34/r36c are bipedal
  but keep the guitar behind/above the head. r40/r41b bring the guitar forward
  but preserve the waist/forearm cluster collapse. None should be captured.
- Tightened `tools/gh3_midori_overlay_visual_sanity_gate.py`: large
  visible-hand-to-target distance and large `guitar_to_head` distance are now
  hard failures. Defaults are `max_target_hand_distance=12.0` and
  `max_guitar_head_distance=14.0`.
- Added focused regression coverage in `tools/gh3_midori_pipeline_test.py`;
  compile and unit test pass. Representative gate checks reject r34, r36c,
  r41b, r48, r54, and retained old candidates before capture.
- Resume from a new same-frame relation solve; do not use retained candidate
  directories as bases and do not capture anything failing the stricter gate.

## 2026-08-20 r54 source/final relation and source-IK bake rejection

- Refreshed the ihatecompvir source audit. Current answer: local
  `ihatecompvir-public-milo-sources` is present and useful as source/reference
  material, but public `glTFMilo` is still not the final GH2 PS2 converter.
  Final output remains the OGX GH2 `CharClipSet`/`CharClipSamples` writer path,
  validated against MiloLib/GH2 notes.
- Added `tools/gh3_midori_source_final_relation_report.py` to compare r47 exact
  source bridge relations against final MILO replay in normalized torso units.
  r48, r52, and r53b all mismatch. r52 proves the visible-hand-to-target patch
  solves the local hand rows but erases the source target-to-palm offset and
  leaves the guitar/hand relation wrong.
- Ran one bounded r54 source-IK visible-arm-chain bake probe from the exact r47
  main bridge using `source-ik-helper-locals`, `visible-arm-chain-rotpos`, and
  emitted target proxies. The stage patch applied 12 channels with no missing
  channels, then a temp MILO build/replay rejected before capture:
  `head_spine3=6.512686`, `guitar_head=15.852803`,
  `LH_to_fret=20.460141`, `RH_to_strum=33.400034`,
  overlay gate `status=reject`, source/final relation `mismatches=7`.
- Decision: do not capture r54 and do not repeat direct source-IK-helper-local
  bakes. Resume by preserving the source-authored relation between palms, IK
  helpers, and `bone_guitar_body` in one emitted character frame before layer
  separation. A GLB intermediate is still acceptable, but it must feed that
  same-frame relation solve rather than act as a direct `glTFMilo` shortcut.

## 2026-08-20 r53 r41b replay-on-current-stage rejection

- Replayed the retained r41b front-placement patch report onto the current
  matrix-local / Control_Root-pelvis ACP stage. The diagnostic froze
  `upper-head-clavicles`, applied
  `r41b_r40_full_contract_offset_patch_report_20260820.json`, built temporary
  main/ui/fret/strum MILOs, and replayed frame 10. All outputs were temporary
  `%TEMP%` scratch; no ISO was used and no loose DLC was replaced.
- r53 rejects before capture:
  `head_spine3=6.515129`, `guitar_head=11.710696`,
  `LH_to_fret=31.788181`, `RH_to_strum=23.812096`, and
  `r53_r41like_visual_sanity_gate_20260820.json` reports
  `status=reject`.
- r53b patched visible hands directly to runtime fret/strum targets. The hand
  local solve succeeds again (`LH_to_fret=0.000011`,
  `RH_to_strum=0.000050`), but the overlay still rejects because the upper body
  remains collapsed and the guitar anchor stays far above the corrected
  hand/target cluster (`head_span=3.547160`, guitar/hand delta ratio
  `3.329834`).
- Decision: do not capture r53/r53b and do not replay the old r41b report onto
  the current stage again. The historical r41b visual lead is not portable to
  this current Control_Root/model-parent branch by channel replay alone.
- Next resume: solve visible hands and `bone_pos_guitar.mesh` together in one
  final character-space frame before ACP layer separation, or install/test a
  real automated GLB/MiloLib bridge end-to-end. Current local output still uses
  OGX `milo_convert_tool`; ihatecompvir/MiloLib is reference material only in
  this workspace, not an active executable bridge.

## 2026-08-20 r48 exact overlay ACP/MILO no-capture rejection

- Fed the r47 exact overlay source bridge manifest into ACP staging for a tiny
  three-clip probe: `gh3_guit_mido_c_med_idle`,
  `gh3_hnd_guit_chord_mid_bar3_d`, and
  `gh3_hnd_guit_strum_mido_norm_m01_d`. Built temporary MILOs only under
  `%TEMP%`; nothing was deployed, and no ISO was used.
- Baseline r47-manifest ACP/MILO replay fails the r46 overlay gate:
  visible hand center ratio `0.074428`, target center ratio `-0.376701`,
  guitar ratio `0.141367`, status `reject`. The new bridge is coherent, but
  visible `bone_L-hand.mesh` / `bone_R-hand.mesh` still have no sampled source
  positions in the final replay, so the hands remain in waist space.
- Regenerated exact r47 source guitar contract reports and tested fret/strum
  `source-ik-helper-gh2scale` and unscaled `source-ik-helper` hand-root modes.
  Both reject before capture; they only move target locals and do not move the
  visible hands.
- Evidence:
  `r48_r47_exact_overlay_visual_sanity_gate_20260820.json`,
  `r48_r47_exact_overlay_sourceik_visual_sanity_gate_20260820.json`, and
  `r48_r47_exact_overlay_sourceikraw_visual_sanity_gate_20260820.json`.
- Resume by baking coherent r47 source `Bone_Palm_L/R` into visible
  `bone_L-hand.mesh` / `bone_R-hand.mesh` or the full visible arm chains. Do
  not repeat hand-target-local-only source-IK helper variants.

## 2026-08-20 r47 exact overlay source bridge pass

- Re-exported the exact frame-10 overlay source clips with the current
  Blender/NXTools exporter and explicit body/arms/palms/IK/guitar helper
  requests:
  `gh3_guit_mido_c_med_idle`, `gh3_hnd_guit_chord_mid_bar3_d`, and
  `gh3_hnd_guit_strum_mido_norm_m01_d`. Each bridge produced 36 records for
  frames `0,10` and a tiny GLB. No ISO was used; input raw files came from the
  pre-existing `%TEMP%/gh3_midori_source_visual_20260816_211144/source`
  extraction.
- All three exact overlay bridges pass
  `tools/gh3_midori_source_handguitar_bridge_sanity_gate.py`:
  main center ratio `0.184920`, fret `0.779794`, strum `0.657953`; combined
  manifest `failures=0`, worst center ratio `0.779794`.
- Added manifest:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r47_exact_overlay_handguitar_bridge_manifest_20260820.json`.
  Sanity report:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r47_exact_overlay_handguitar_bridge_manifest_sanity_20260820.json`.
- Updated the source bridge sanity gate so it accepts `--pose-json` for
  one-off checks and resolves custom manifest-relative paths correctly.
- Resume by feeding the r47 exact overlay bridge manifest into ACP/MILO
  generation, then run the r46 overlay visual sanity gate before any emulator
  capture. This supersedes the stale pinned merged bridge as the usable
  GLB/pose intermediate route.

## 2026-08-20 r46 overlay/source bridge sanity gates

- Added `tools/gh3_midori_overlay_visual_sanity_gate.py`, a torso-relative
  no-capture gate for actual-layer replay JSON. r41b now fails this gate even
  though its old IK distances were internally coherent: visible hand center
  ratio `0.107244`, target center ratio `0.041469`, guitar ratio `0.589405`,
  guitar-minus-hand delta `0.482161`. This encodes the visual waist-cluster
  rejection so non-bipedal/obviously incoherent candidates are rejected before
  emulator capture.
- Added `tools/gh3_midori_source_handguitar_bridge_sanity_gate.py`, a source
  pose bridge gate for GLB/intermediate routes. It requires body, visible
  palms, IK helpers, and guitar helper to live in one coherent body-scale
  frame. The retained pinned force-partial merged hand/guitar 5-case bridge is
  rejected: worst hand/guitar center ratio `268.578881`, medium idle
  `249.727645`. Those retained GLBs are mismatched-space evidence and must not
  be treated as the true same-frame source bridge.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r46_overlay_visual_sanity_gate_r41b_20260820.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r46_source_handguitar_bridge_sanity_gate_20260820.json`.
- Resume by producing or locating a coherent same-frame bridge that passes both
  r46 gates before any capture: body + visible palms/hands + IK helpers +
  guitar helper together, then ACP samples, then GH2 PS2 `milo_convert_tool`.

## 2026-08-20 r45 shared prop-frame pair-fit rejection

- Rebuilt r41b again from local evidence only, using the retained r40 freeze
  and r41b replay path. Temporary outputs were under `%TEMP%` and must not be
  treated as deliverables.
- Rejected the shared prop-frame pair-fit probe before build/capture. It tried
  to pair-fit stock prop-string offsets to the r41b visible hand pair, but the
  lengths are incompatible: prop target pair `9.611476`, desired visible-hand
  pair `18.423591`. The solve moves `bone_pos_guitar.mesh` from
  `[-7.601,3.993,51.964]` to `[-25.200,-12.949,61.791]`; targets/hands then
  land in an obviously invalid frame. The no-proxy-emission variant produces
  the same invalid solve.
- Source IR limitation: exact main/fret/strum JSONL records exist, but the
  exact main clip channel list does not include the guitar-body/IK helper nodes
  needed to emit visible arms/hands and guitar prop from one same-frame source
  solve. Existing GLB bridges are attack/med-idle evidence, not the exact
  stand/fret/strum overlay. Raw source `.ska.ps2` / `.ske.ps2` / skin files
  were not present in `analysis/gh3_midori_source_ir`.
- ihatecompvir/MiloLib sources are being used as reference for the GLB-to-GH2
  bridge, but public `glTFMilo` is not the final converter because it targets
  later HMX games/platforms. Final automated output should still pass through
  the repo's GH2 PS2 `milo_convert_tool` CharClipSet/CharClipSamples writer.
- Resume from a real shared-frame source: either automate a true raw-source or
  GLB bridge that includes the prop and hands in the same frame, or instrument
  runtime/pose reporting for actual final prop/hand positions. Do not repeat
  target-only offsets, prop-string blends, or mismatched hand/prop pair-fit
  probes.

## 2026-08-20 r44 target-only correction rejection

- Added `tools/gh3_midori_patch_stage_from_report.py` so retained patch reports
  can recreate cleaned ACP stages. Rebuilt r41b from local
  `analysis/gh3_midori_acp_stage` plus retained r40/r41b reports and verified
  it reproduces the retained structural gate (`suspicious=none`,
  `head_spine3=11.674564`, `guitar_head=11.955784`) and runtime-aware contract
  (`LH-FRET=3.832`, `RH-STRUM=0.000`).
- Added `--hand-target-blend-with-current`,
  `--hand-target-world-offset`, and generic visible-arm-chain proxy emission to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`; updated
  `tools/gh3_midori_patch_bake_probe_positions_into_stage.py` to consume the
  new generic emitted-proxy manifest record.
- Rejected fixed-r41b-guitar prop-string targets: full prop-string snap is far
  too aggressive, and the 35% blend moves the target frame in the wrong
  direction for the waist-collapse failure.
- Rejected fixed-r41b-guitar target-only `+8` world-Z lift. Runtime-aware
  contract worsens to `LH-FRET=11.593`, `RH-STRUM=5.551`, and direct screenshot
  rejects: hand/forearm mass moves into chest/neck space while guitar remains
  too high/right. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r44_targets_zplus8_direct_visual_20260820/midori_1_hand_overlay_f010.bmp`.
- Resume from the shared final-frame problem: orient/position the guitar prop
  and visible hands together. Do not repeat target-only vertical offsets or
  prop-string blends.

## 2026-08-20 pelvis / Control_Root r43 diagnosis

- No new build/capture was run. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r43_pelvis_control_root_diagnosis_20260820.md`.
- r41b is still the current visual lead but remains rejected. Its actual-layer
  replay proves the built IK target graph is internally coherent:
  `fret_hand_to_left_hand = 3.548390` and
  `strum_hand_to_right_hand = 0.000023`.
- The failure is not a missing `CharIKHand`, missing `bone_fret_hand` /
  `bone_strum_hand`, or simple orphaned `bone_pos_guitar.mesh`. OGX
  `milo_convert_tool` regenerates the stock-like guitar proxy graph and points
  `left_hand.ik` / `right_hand.ik` at the expected targets.
- The active hypothesis is that main/fret/strum channels have been made
  coherent in a pelvis/Control_Root-local frame that still places the
  hand/target cluster around waist space. r42 mesh filtering rejected the idea
  that three obvious mixed-bone debris chunks were the primary cause.
- Resume by building an automated same-frame bridge that solves visible
  arms/hands and `bone_pos_guitar.mesh` in one final character-space frame
  before ACP layer separation. GLB may be used as an intermediate, but the end
  state still must be automated MILO generation.

## 2026-08-19 stock hand-target override rejection

- Added diagnostic-only post-sample stock target overrides to
  `tools/gh3_midori_guitar_ik_contract_report.py`:
  `--override-fret-hand-target-stock-local` and
  `--override-strum-hand-target-stock-local`.
- Evidence:
  `stock_hand_target_override_decision_20260819.json`,
  `meshmain_fullhands_baseline_rerun_contract_20260819.json`,
  `meshmain_fullhands_strum_stocktarget_contract_20260819.json`,
  `meshmain_fullhands_fret_stocktarget_contract_20260819.json`, and
  `meshmain_fullhands_both_stocktarget_contract_20260819.json`.
- Results: baseline generated-main/fullhands is still `LH->fret 40.978`,
  `RH->strum 13.162`, anchor split `112.450`. Forcing stock strum target
  worsens right-hand distance to `15.621`. Forcing stock fret target improves
  left-hand distance only to `35.971` and worsens anchor split to `120.564`.
  Forcing both does not solve the contract.
- Next resume: do not rebuild/capture stock hand-target local overrides. The
  remaining failure is a parent-frame/space interpretation problem for the
  full hand-bank target positions, not a simple replacement with stock GH2
  controller locals.

## 2026-08-19 guitar anchor graph diagnosis

- Added `tools/gh3_midori_guitar_anchor_graph_report.py` and evidence:
  `guitar_anchor_graph_decision_20260819.json`,
  `guitar_anchor_graph_report_20260819.json`, and
  `stockattach_fullhands_anchor_contract_report_20260819.json`.
- Stock-vs-Midori graph result: after the xplorer fret override, the fret
  subtree matches stock, but the deployed Midori model's
  `bone_strum_hand.mesh` local target differs from stock glam1 by `14.965`
  units. This is in the model bind graph, before hand-bank samples are applied.
- The old `stockattach_frame` visual proof was not a canonical full hand-bank
  proof. A new stock attach-world plus canonical full fret/strum hand-bank
  structural candidate was built and measured, but not captured because it
  worsened the contract: left hand to fret target `43.106`, right hand to strum
  target `24.875` versus `40.978` and `13.162` for the generated
  meshmain/fullhands branch.
- Next resume: do not capture stockattach/fullhands. Correct or compensate the
  `bone_strum_hand.mesh` stock-vs-Midori target graph mismatch, then remeasure
  the full-hand contract before any visual run.

## 2026-08-19 canonical-fret visible-left-arm rejection

- Extended `tools/gh3_midori_guitar_frame_hand_bake_probe.py` with canonical
  fret merge modes:
  `canonical-fret-visible-left-arm`,
  `canonical-fret-visible-left-arm-pos`, and
  `canonical-fret-visible-left-arm-rot`. The merged fret ACP preserves
  canonical `bone_fret_hand.mesh` and left finger channels and appends visible
  left-arm channels in GH2 CharBones type order.
- Structural result: the best branch was `canonical-fret-visible-left-arm-rot`
  with `--arm-chain-reach-scale 1.5`; it reduced left visible hand to fret
  target distance to `10.782` in
  `canfret_leftarm_rot_r150_anchor_contract_report_20260819.json`.
  Position-only and rot+pos variants were worse.
- Runtime proof:
  `canfret_leftarm_rot_r150_pose_review_20260819/visual_decision.json`.
  Captured through local `gh2_ps2_hybrid_assets/GEN` plus loose DLC only, low
  priority, then restored canonical loose hashes. The proof loaded the merged
  fret layer (`ch=19`) but visually rejects: upright/bipedal, yet guitar still
  points down through the legs and visible hands remain near shin/foot space.
- Next resume: do not keep iterating simple left-arm merge variants. Diagnose
  the guitar prop/main anchor frame against the native stock hand-bank parent
  graph, especially `bone_fret.mesh`, `bone_strum.mesh`, and their relationship
  to `bone_pos_guitar.mesh` after the external xplorer prop attach.

## 2026-08-19 hand-overlay checkpoint

- Active Midori status remains experimental and not complete. Do not block this
  goal unless the user explicitly asks again.
- Runtime proof must continue to use local `gh2_ps2_hybrid_assets/GEN` plus
  loose DLC only, never a mounted GH2 ISO at game time. Keep capture/build CPU
  priority low/Idle.
- Current useful branch family: generated narrow mesh-channel guitar-main plus
  canonical full fret/strum hand banks. Canonical full hands visibly engage;
  endpoint-only, proxy-only, and minimal arm-chain payloads are exhausted.
- New z-offset probes are rejected:
  `fullhands_zminus8_pose_review_20260819/visual_decision.json` and
  `fullhands_zplus8_pose_review_20260819/visual_decision.json`. `zminus8`
  remains upright but the guitar still runs through the legs and the picking
  hand is near knee/foot space; `zplus8` is lower/partly clipped. Simple
  world-Z offsets are not the fix.
- Next resume: keep canonical full hand banks and diagnose the generated
  narrow guitar-main placement/anchor contract. Do not revisit pelvis,
  Control_Root, endpoint-only probes, minimal arm-chain probes, canonical main
  plus full hands, or accepted body-main plus full hands.

## 2026-08-19 hand-bank anchor contract diagnosis

- Added `--case-name` to `tools/gh3_midori_guitar_ik_contract_report.py` and
  measured the single hand-overlay branch without requiring a full review
  candidate. Evidence:
  `hand_bank_anchor_contract_decision_20260819.json`,
  `meshmain_fullhands_anchor_contract_report_20260819.json`,
  `canonical_fullhands_anchor_contract_report_20260819.json`, and the matching
  no-viewer-override reports.
- Generated narrow main plus canonical full hands: left visible hand to
  `bone_fret_hand.mesh` target is `40.978`, right visible hand to
  `bone_strum_hand.mesh` target is `13.162`, and solving guitar anchor from
  fret versus strum disagrees by `112.450`. Canonical main plus full hands also
  disagrees (`66.774`), and disabling viewer prop overrides leaves the same
  shape of failure.
- Important channel contract: canonical fret hand clips contain
  `bone_fret_hand.mesh.pos/.quat` plus left finger quats, but no visible
  left-arm channels (`bone_L-upperArm.mesh`, `bone_L-foreArm.mesh`,
  `bone_L-hand.mesh`). Canonical strum clips do contain right clavicle,
  upper-arm, forearm, hand, and finger quats. A whole-guitar offset cannot make
  the left proxy/finger target drag the visible left arm in this viewer path.
- Next resume: build a diagnostic candidate that preserves canonical full
  fret/strum hand and finger channels, then merges a visible left-arm chain
  bake toward `bone_fret_hand.mesh`. Do not return to simple offsets or to
  minimal arm-chain-only replacements.

## 2026-08-18 continuation checkpoint

- The targetlength packed-parent candidate
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-targetlength-bakepos-bind`
  is now rejected before user approval. A new Codex bipedal verdict layer in
  `targetlength_packedparent_codex_bipedal_verdicts.json` rejects
  `midori_1_medium_idle_f060` and `midori_2_medium_idle_f060` because their
  native medium-idle captures are horizontal/sideways rather than upright idle
  stances. The packet precheck is `failed_reject_before_user_approval`; the
  direct visual approval gate is `failed`.
- Do not treat the current targetlength packet as pending approval. Completion,
  rollout, and review packet have been refreshed to failed status. A dry-run
  accept no longer validates because the packet itself is rejected.
- Med-idle diagnosis: the fresh source bridge is upright, but the targetlength
  med-idle frame 60 report shows max pose error `0.540991` while child aim and
  shin dots remain good, so local child-length metrics missed a global
  root/pelvis orientation problem. Simply enabling
  `--control-root-pelvis-parent` worsens the report (`max_pose_error=49.412737`,
  min child aim `-0.048227`). Continue with Control_Root basis compensation,
  not a raw pelvis reparent.
- Existing rootlocal/rootbasis/rootyawfold/upalign/axisblend policies were
  swept on med-idle and attack-left. None is promotable. A new diagnostic
  targetlength `stablepelvis30` branch that swaps low-delta pelvis frames to
  GH2 bind world rotation is also rejected: it keeps child aim but explodes
  med-idle and attack-left pose error. Evidence:
  `stablepelvis30_diagnostic_decision.json`. Next check should compare
  packed/runtime pelvis mesh basis and visual screenshot orientation, because
  replacing the solved pelvis world with bind rotation is not the missing
  Control_Root compensation.
- New orientation diagnostic evidence:
  `targetlength_idle_orientation_diagnostic.json`. It quantifies the rejected
  medium-idle visual failure: source med-idle render is near vertical
  (`upright_score=1.022727`, PCA delta from vertical `12.900003`), while the
  target medium-idle captures are horizontal enough to fail (`upright_score`
  `0.767699` / `0.812095`, PCA delta from vertical `64.730045` /
  `57.226530`). This diagnostic is now part of the targetlength packet
  precheck. It also records source `bone_pelvis` parentage under
  `Control_Root` versus current target runtime `Bone_Pelvis.parent_source=""`.
- New parentage/basis diagnostic evidence:
  `targetlength_parentage_basis_diagnostic.json`. Important correction: the
  current Midori model rig itself has `bone_pelvis.parent="Control_Root"` and
  source Midori also evaluates `Bone_Pelvis` under `Control_Root`; the mismatch
  is that the targetlength animation/skeleton diagnostic still uses
  `Bone_Pelvis.parent_source=""`. Status:
  `target_model_animation_parentage_mismatch_confirmed`. Next branch should
  make the animation bind graph match the deployed model hierarchy or explicitly
  compensate that mismatch before packing.

## Source-grounded fixes already made

- GH3 PS2 partial-animation block is now decoded little-endian and skipped
  before quaternion/translation streams. All 280 masks match nonzero channel
  tables; corrected IR has 291,560 quaternion keys, 31,062 translation keys,
  and zero old boundary-snap artifacts.
- Sources used: pinned Fretworks NXTools `6cea808a...`, Addy Mills GH Toolkit
  `db0b8a43...`, and old GH3 tools `958cf53f...`.
- `python tools/gh3_midori_pipeline_test.py` passes 8 tests.
- `tools/gh3_midori_acp_stage.py` now supports diagnostic
  `--only-source-bones` and records the whitelist in its manifest.
- `tools/gh3_midori_source_visual.py` now supports `--only-bones`, removes all
  other Blender curves, resets excluded bones to bind, validates retained
  curves, and prints source edit/basis/pose matrices plus rest/pose parents and
  Blender `matrix_local`.

## Pelvis gate

Use clip `gh3_guit_mido_a_attackl`, frames 0, 15, 30, 45. Raw GH3 source must
be rendered through patched NXTools with only `Bone_Pelvis` active; target must
be stock Glam in Guitar Hero Classic with only `Bone_Pelvis` converted.

Direct visual result from the original pelvis-only gate:

- Source frame 15 appeared upright in the old pelvis-only source gate, but this
  was a partial-source blind spot. The 2026-08-17 evaluated GLB bridge check
  proves the full source attack pose is horizontal at frames `15/30/45`.
- Current production conversion is already rolled about 50 degrees in the
  image plane at frame 15 and nearly horizontal later. Pelvis gate fails.
- Source pelvis alone is genuinely diagonal at frames 30/45; do not require it
  to remain upright. Match the source sequence, not a preconceived pose.

Rejected pelvis-only formulas:

- production: edit-local, source, bind-delta, edit-inv-frame, transpose
- global axis conjugation: edit-local, target, bind-delta
- world delta: source, delta-bind, frame-edit-inv
- transposed local delta: source-row, delta-bind, edit-inv-frame
- transposed world delta: source-row, bind-delta, frame-edit-inv
- direct Blender matrix-local pose: matrix-local-pose, source/target,
  bind-delta, edit-inv-frame
- matrix-local plus target bind: matrix-local-bind, source, bind-delta,
  edit-inv-frame
- Control_Root parent conjugation: edit-local-parent-conjugate, source,
  bind-delta, edit-inv-frame, transpose
- Control_Root parent conjugation with direct Hmx storage:
  edit-local-parent-conjugate, source, bind-delta, edit-inv-frame, direct
- Control_Root parent conjugation with delta-bind compose:
  edit-local-parent-conjugate, source, delta-bind, edit-inv-frame, transpose
- Control_Root parent conjugation with target-basis rebase:
  edit-local-parent-conjugate, target, bind-delta, edit-inv-frame, transpose
- Control_Root parent conjugation without target bind:
  edit-local-parent-conjugate-nobind, source, edit-inv-frame, transpose
- Matrix-local rest delta before target bind:
  matrix-local-rest-delta-bind, source, edit-inv-frame, transpose

All were inspected at frames 0, 15, 30, 45. None passes.

## Most important next check

NXTools computes SKA basis with `armature.data.bones[name].matrix`, but Blender
evaluates the pose through the armature-space rest chain (`matrix_local`) and
the parent `Control_Root`. For pelvis, NXTools' post-fix `bone.matrix` and our
reconstructed edit rotation are both effectively identity, so the suspected
edit-matrix reconstruction mismatch was disproven. However, pelvis
`matrix_basis` and final `pose.matrix` differ strongly even with the parent
animation reset, proving the rest-chain basis still matters.

At attack frame 15:

- `bone_pelvis` edit matrix is identity to about 2e-7.
- basis 3x3 rows:
  `[.6837659,-.1723899,.7090458]`
  `[.1406035,.98461,.1037971]`
  `[-.7160271,.0287215,.6974813]`
- final pose 3x3 rows:
  `[.683766,-.1723898,.7090456]`
  `[.716027,-.0287213,-.6974815]`
  `[.1406035,.98461,.1037971]`

Next: print `Bone.matrix_local` for `Control_Root` and `bone_pelvis`, plus both
rest parents, and derive the source parent-space alignment into GH2's pelvis
parent space. Do not add thighs until pelvis-only frames match visually.

## 2026-08-16 resume notes

`Control_Root.matrix_local` is a non-identity rest basis, roughly:

`[[1,0,0],[0,0,-1],[0,1,0]]`

`bone_pelvis.matrix_local` is in that parent space, with rest parent and pose
parent both `Control_Root`. Blender's final pelvis pose is effectively
`Control_Root.matrix_local * bone_pelvis.matrix_basis`, which explains why
raw edit-local basis and final source pose differ.

Visual checks after adding matrix-local diagnostics:

- Directly emitting `matrix_local * edit_local_delta` makes GHC frame 0 lie
  horizontal, so matrix-local cannot be used as the target pelvis local.
- `edit-local-parent-conjugate` was a useful partial clue: frames 0/15 looked
  upright/profile instead of badly rolled, but frames 30/45 stayed upright
  while the Blender source leaned back. It fails the pelvis gate.
- `hmx-quat-mode direct`, `compose-order delta-bind`, and target-basis rebase
  were separately checked at frame 30 and rejected: direct stays front/upright;
  delta-bind and target-basis lie the model down.

Next best diagnostic: compare the exact per-frame source `pose.matrix` rows to
the exact matrix GHC reconstructs after `Hmx::Quat` decode for `bone_pelvis`.
If the native matrix path keeps collapsing, an automated Blender/.glb bridge is
acceptable: export the NXTools-evaluated source pose/animation to an
intermediate `.glb`, then derive GH2 local matrices from that authoritative
evaluated pose stream rather than from raw GH3 SKA quaternions alone.

## 2026-08-17 matrix round-trip and bridge notes

Added `tools/gh3_midori_pelvis_matrix_diagnostic.py`. It stages the candidate
pelvis matrices in Python, converts them to packed GH2/GHC XYZW samples, then
decodes them with the same row-matrix convention as GHC. Result: every tested
candidate has `emit_vs_ghc_max` near zero (`~1e-8` or smaller), so the remaining
failure is not MILO packing or GHC `Hmx::Quat` decode.

Added `--pose-json` to `tools/gh3_midori_source_visual.py`. Blender now exports
machine-readable evaluated source pose records for the same frame/bone set that
it renders. Fresh pelvis-only bridge export showed:

- IR reconstruction vs Blender evaluated pose max delta: frame 0 `0.00014`,
  frame 15 `0.01607`, frame 30 `0.00016`, frame 45 `0.00402`.
- `matrix-local-pose` matches Blender numerically (`<=0.016` because of the
  IR-vs-Blender sampler delta), but was already visually rejected in GHC as
  horizontal. Therefore Blender armature-space `pose.matrix` is not directly
  GH2/GHC pelvis local space.
- `edit-local-parent-conjugate` remains the closest late-frame family
  numerically (`blender_pose_vs_ghc_max` about `0.283` at frame 30 and `0.464`
  at frame 45) but it still fails visually because it stays too upright.
- `edit-local-parent-conjugate-nobind` reconstructs exactly in GHC but is worse
  against Blender (`~1.3` late-frame max delta), so do not promote it.

Next: derive a target-local matrix from Blender evaluated pose plus the GH2
stock pelvis bind/parent basis. Known target Glam pelvis bind local is roughly
`[[0,.076,.997],[0,.997,-.076],[-1,0,0]]`; known source
`Control_Root`/pelvis rest basis is roughly
`[[1,0,0],[0,0,-1],[0,1,0]]`. The missing piece is the constant alignment
between Blender armature space and GH2 pelvis parent-local space, not sample
storage.

## 2026-08-17 rest-delta-bind visual probe

Added diagnostic `matrix-local-rest-delta-bind`, equivalent to:

`target_local = (source_rest^-1 * source_pose) * target_bind`

This was the unisolated rest-preserving compose order after the matrix
round-trip proof. It built and loaded as a pelvis-only ACP/MILO, but direct GHC
visual inspection rejects it:

- Frame 0: target is too front/upright; source has a mild whole-body tilt.
- Frame 15: target is already diagonally screen-rolled; source is upright in
  profile.
- Frame 30: target finally leans instead of standing upright, but the lean is
  screen-plane roll rather than source's leaned-back/profile pose.
- Frame 45: target is nearly horizontal across the screen; source remains a
  leaned-back profile.

This means the missing alignment is not simply "apply source local delta before
target bind." The next derivation should solve for the constant axis alignment
that turns the current screen-plane roll into the source profile/backward lean,
using the Blender `--pose-json` evaluated matrices and the target Glam bind.

## 2026-08-17 signed-axis bind alignment probe

Added diagnostic `matrix-local-axis-align-bind`. It automatically chooses the
proper signed-axis permutation `C` that makes `C^T * source_rest * C` closest to
the target Glam pelvis bind, then applies a small bind correction so rest maps
exactly to the target bind:

`target_local = (C^T * source_pose * C) * ((C^T * source_rest * C)^-1 * target_bind)`

For Midori pelvis vs Glam pelvis it selected:

`C = [[0,1,0],[0,0,-1],[-1,0,0]]`

The source-rest-to-target-bind max residual before the bind correction is
`0.0763737`, matching Glam's small pelvis tilt.

Direct GHC pelvis-only visual read:

- Frame 0: not catastrophic, but target is more neutral/front-facing than the
  source's mild whole-body tilt.
- Frame 15: source is upright/profile; target is upright/profile with no large
  screen-plane roll. This is much closer than previous candidates.
- Frame 30: source leaned-back/profile; target now also reads leaned-back/profile.
- Frame 45: source remains leaned-back/profile; target remains leaned-back/profile
  instead of becoming horizontal.

This is the first viable pelvis-only candidate. Do not call the full gate
approved yet because frame 0 still needs scrutiny, but this is the branch to
continue from. Next: either confirm frame 0 under tighter source/GHC camera
parity, or add thighs using the same `matrix-local-axis-align-bind` policy and
check whether the lower-body hierarchy remains coherent.

## 2026-08-17 pelvis + thighs tier probe

Staged `matrix-local-axis-align-bind` with only:

`Bone_Pelvis,Bone_Thigh_L,Bone_Thigh_R`

Numeric precheck:

- Both thighs round-trip through GHC/Hmx with `emit_vs_ghc_max` about `4e-8`.
- Both thighs choose the same signed-axis permutation
  `[[0,0,1],[0,-1,0],[1,0,0]]`.
- The signed-axis rest residual before bind correction is large (`0.952174`),
  so visual inspection is required; do not trust the number alone.

Direct GHC four-frame read against Blender source with the same active bones:

- Frame 0 remains inconclusive/weak: source shows thigh/skirt motion, while the
  GHC capture is front/upright and crops much of the lower body.
- Frame 15 keeps the pelvis profile and visible legs do not explode or mirror
  badly. Source legs angle more than target, but the tier does not break the
  posture.
- Frames 30 and 45 preserve the leaned-back/profile family, and visible legs
  remain coherent under the pelvis. The GHC crop hides the far foot/lower leg,
  so this is provisional, not final lower-body approval.

Continue from `matrix-local-axis-align-bind`. Next choices:

- improve GHC/source camera parity to settle frame 0 and lower-leg visibility;
  or
- add knees as the next hierarchy tier, but only under this signed-axis policy.

## 2026-08-17 pelvis + thighs + knees tier probe

Staged `matrix-local-axis-align-bind` with:

`Bone_Pelvis,Bone_Thigh_L,Bone_Thigh_R,Bone_Knee_L,Bone_Knee_R`

Numeric precheck:

- Both knees select the same signed-axis permutation
  `[[-1,0,0],[0,-1,0],[0,0,1]]`.
- Both knees round-trip through Hmx/GHC successfully, with
  `emit_vs_ghc_max` roughly `1e-8` on left knee and `5e-8` on right knee.
- The signed-axis rest residual before bind correction is very large
  (`1.98605`) on both knees, so the math only proves encoding stability, not
  anatomical correctness.

Direct visual read with widened/lowered GHC camera:

- The tier is stable: no knee explosion, mirror flip, or detached lower leg was
  visible across frames 0, 15, 30, and 45.
- The profile/back-lean family from the pelvis and thigh tier still survives,
  especially visible at frame 30 from yaw 0/180.
- The lower-body silhouette remains too straight compared with the Blender
  source crouch. A same-camera pelvis+thigh-only comparison showed that adding
  knee animation is not an obvious improvement; it changes the legs slightly but
  does not recover the sharp source knee bends.

Treat pelvis+thigh+knees as "stable but not approved." The next useful branch is
not another blind local-matrix permutation. Prefer an automated joint/world-space
solve for the lower-body chain, potentially bridged through Blender/GLB as an
intermediate format, then emit the solved GH2 local rotations back through ACP.

## 2026-08-17 lower-body world-chain probes

Added diagnostic policies:

- `matrix-world-axis-align-bind`
- `matrix-knee-world-axis-align-bind`
- `matrix-knee-world-axis-blend-bind`

All three preserve Hmx/GHC matrix round-trip stability. Focused pipeline tests
still pass (`python tools/gh3_midori_pipeline_test.py`, 8 tests).

Visual results:

- `matrix-world-axis-align-bind` keeps the pelvis/back-lean family but drives
  the lower legs far too high/tucked at frames 15, 30, and 45. It proves that a
  world-chain solve can recover bend energy, but the thigh/knee world bases are
  over-rotated.
- `matrix-knee-world-axis-align-bind` keeps pelvis and thighs identical to the
  viable local-axis branch, and applies the world solve only at knees/distal
  bones. It improves numeric knee agreement versus local-axis knees, but direct
  GHC frames still over-curl the legs toward the torso.
- `matrix-knee-world-axis-blend-bind` blends local-axis knees and knee-world
  knees 50/50. It avoids the full tucked pose but produces a sideways/splayed
  leg silhouette instead of the Blender source crouch.

Conclusion: local-axis knees are too straight; world knees are too curled; the
simple rotation-space midpoint is not anatomically valid. The next branch should
derive knee rotations from joint positions/bone vectors or a small IK-style
chain solve using Blender-evaluated pose data (GLB remains acceptable as an
automated bridge), then emit the resulting GH2 local matrices through ACP.

## 2026-08-17 evaluated child-pose diagnostics

Added diagnostic policies:

- `matrix-eval-world-axis-align-bind`
- `matrix-eval-knee-world-axis-align-bind`

Important source-space finding:

- The old child-bone matrix diagnostics used `bone.matrix_local * basis`, which
  matches the pelvis but does not match Blender for children.
- Blender/NXTools child pose rotation is recursive:
  `parent_pose * parent_rest^-1 * child_rest * child_basis`.
- The new recursive evaluator matches Blender pose rotations for frame 30 within
  about `1e-5` on thighs and knees.
- Source pose positions can also be reconstructed from skeleton offsets through
  thighs with `parent_pose_rotation * nxtools_offset`; pelvis translation maps
  into Blender coordinates as `[x, -z, y]`.

Visual results:

- `matrix-eval-world-axis-align-bind` is rejected. It uses the correct evaluated
  source child rotations, but applying the world solve to the whole chain rolls
  the character horizontal at frames 30/45.
- `matrix-eval-knee-world-axis-align-bind` keeps pelvis/thighs on the viable
  local-axis branch and uses evaluated source pose only at knees/distal bones.
  It is more stable than full eval-world, but still not approved: lower legs
  cross/splay instead of matching the Blender crouch.

Next branch: use the newly verified source evaluated positions/vectors to solve
the GH2 thigh/knee chain anatomically. The rotation-only axis alignment is now
exhausted; solve target knee and ankle directions from source joint vectors or a
two-bone IK fit, then emit GH2 local rotations.

## 2026-08-17 vector-aim lower-body probe

Added diagnostic policy:

- `matrix-vector-aim-bind`

Implementation notes:

- GH2 bind convention was confirmed:
  - child world position = `parent_world_position + parent_world_rotation^T *
    child_local_translation`
  - child world rotation = `child_local_rotation * parent_world_rotation`
- Source rest and animated joint positions are reconstructed from the verified
  recursive source evaluator plus `nxtools_offset`; pelvis translation maps as
  `[x, -z, y]`.
- A lower-body source-to-target frame is built from source rest thigh/knee
  vectors and target GH2 bind thigh/knee vectors. It maps source rest
  thigh-to-knee directions to GH2 bind directions within about `0.024` max
  component error.
- The policy starts from the viable pelvis/thigh local-axis branch, then rotates
  each thigh/knee world matrix so its child bind vector aims toward the mapped
  animated source child vector.

Visual result:

- Stable: no explosion, no horizontal full-body roll, and pelvis/back-lean still
  reads in the same family.
- Rejected for approval: the legs are driven into a wide horizontal splay at
  frames 30/45. This improves over the too-straight local-axis knees but shows
  that independent per-segment aiming is not sufficient.

Next branch: solve each leg as a constrained two-bone chain/IK plane. Use source
hip-knee-ankle positions to derive a thigh direction, knee bend plane, and ankle
direction together, rather than aiming thigh and knee segments independently.

## 2026-08-17 leg-plane and knee-only vector probes

Added diagnostic policies:

- `matrix-leg-plane-aim-bind`
- `matrix-knee-vector-aim-bind`
- `matrix-knee-vector-aim-gated-bind`

Implementation notes:

- `matrix-leg-plane-aim-bind` constructs a full local bind frame and desired
  animated frame from hip-knee-ankle vectors. It solves GH2 row/local convention
  as `world_rotation = local_frame * desired_frame^T`, consistent with child
  position `parent_world_position + parent_world_rotation^T *
  child_local_translation`.
- `matrix-knee-vector-aim-bind` keeps pelvis and thighs on the viable
  `matrix-local-axis-align-bind` branch, then aims only `Bone_Knee_L/R` toward
  the mapped source knee-to-ankle vector. This isolates the missing crouch from
  the hip/thigh reorientation failure.

Visual result:

- `matrix-leg-plane-aim-bind` is rejected. It loads through the actual
  ark-external GHC path and stays stable, but frames 30/45 drive the legs into a
  hard horizontal extension. Source Blender reference frames show bent
  back/down legs, so full-chain plane aiming is solving the wrong target-space
  direction for this retarget.
- `matrix-knee-vector-aim-bind` was briefly provisional on
  `gh3_guit_mido_a_attackl`, but broad review rejects it. Full-bank direct GHC
  captures for idle, attack, jump, solo, and transition show that the branch
  improves attack-like crouch but turns neutral source legs into a seated /
  horizontal-shin pose.
- The first `matrix-knee-vector-aim-gated-bind` review was invalid as a final
  judgment because the helper skipped to the old distal world-axis base, not to
  the broad `matrix-local-axis-align-bind` baseline.
- A true local-axis world-chain helper now exists. With that corrected base,
  `matrix-knee-vector-aim-gated-bind` at threshold `0.94` preserves neutral
  idle/jump/transition shapes and adds some correction to attack/solo. It is a
  live diagnostic candidate, not approved: attack is still not source-matched
  enough for final signoff.
- Retesting correction order after the true-base fix shows real differences:
  `matrix-knee-vector-aim-pre-bind` preserves idle best but barely improves
  attack; `matrix-knee-vector-aim-post-bind` gives stronger attack bend but
  over-curls and is unsafe without a gate. `matrix-knee-vector-aim-gated-post-bind`
  preserves neutral frames but overdoes attack/solo. The new
  `matrix-knee-vector-aim-gated-postblend-bind` blends default and post
  corrections 50/50 under the same `0.94` gate. Strength retest across 50/65/75
  and post shows 65% toward post is the better current candidate:
  `matrix-knee-vector-aim-gated-postblend65-bind` preserves neutral frames and
  adds more attack/solo bend than 50% without the obvious post-order overcurl.
  It is still not approved.
- Broad `matrix-local-axis-align-bind` is still the safer baseline for neutral
  idle/jump/transition clips, even though attack remains too straight.

Evidence retained:

- `.codex/current-evidence/midori-knee-vector-aim-20260817/`
- `.codex/current-evidence/midori-broad-knee-diagnostics-20260817/`
- `.codex/current-evidence/midori-truebase-knee-gate-20260817/`
- `.codex/current-evidence/midori-truebase-knee-variant-20260817/`
- `.codex/current-evidence/midori-truebase-knee-strength-20260817/`

Next branch:

- Compare `matrix-knee-vector-aim-gated-postblend65-bind` directly against
  source at attack and solo frames, then decide whether to promote it for the
  knee tier or move to a source-authoritative automated bridge. Blender
  constraints, baked matrices, or `.glb` as an intermediate are acceptable if
  the final pipeline remains scripted end-to-end and emits ordinary
  ark-external GHC DLC assets.

## 2026-08-17 continuation: postblend65 and bridge setup

`matrix-knee-vector-aim-gated-postblend65-bind` was staged across the full
external animation set (`331` staged clips; guitar-main bank `266` clips) and
captured directly in GHC for idle, attack, jump, solo, and transition at yaws
0/180. It remains stable and neutral-safe, but still does not match the deeper
source knee tuck in attack/solo. Do not promote it as final. Compact evidence:
`.codex/current-evidence/midori-postblend65-fullbank-20260817/full65_contact.png`.

Added diagnostic `matrix-local-rest-bind-delta`, equivalent to:

`target_local = target_bind * (source_rest^-1 * source_pose)`

The pelvis matrix round-trip is clean (`emit_vs_ghc_max` around `1e-8`) and the
source local delta matches numerically, but direct GHC pelvis-only attack frames
0/15/30/45 reject the branch: frames 30/45 become nearly horizontal in screen
space. Compact evidence:
`.codex/current-evidence/midori-pelvis-restbinddelta-20260817/restbinddelta_pelvis_contact.png`.
This confirms the pelvis failure is not Hmx quat storage and not simple source
local-delta preservation; the missing piece is still the constant target-side
alignment/constraint solve between Blender evaluated source pose and GH2/GHC
pelvis parent space.

Added automated bridge hooks to `tools/gh3_midori_source_visual.py`:

- `--export-glb PATH` exports the NXTools-evaluated source scene as a skinned
  GLB with animation.
- `--no-render` imports/evaluates/exports without source proof PNG rendering.

Blender was not callable from this shell (`blender` not on PATH and no
`blender.exe` in the common checked install slots), so the GLB hook is
syntax-tested but not execution-tested in this turn. Exact source-root inputs
for the next bridge export are under
`C:\Users\smmel\AppData\Local\Temp\gh3_midori_source_visual_20260816_211144`:
`nxtools`, `source\skeletons\gh3_guitarist_midori.ske.ps2`,
`source\models\guitarists\midori_1.skin.ps2`, and
`source\anims\band\guitarist\midori\gh3_guit_mido_a_attackl.ska.ps2`.

## 2026-08-17 continuation: Control_Root target graph sweep

Confirmed the staged GH2/GHC target graph differs from the GH3 source graph at
the root: `Bone_Pelvis` maps to `bone_pelvis`, whose target parent is empty,
while GH3 source has `Bone_Pelvis -> Control_Root`. Added
`target_parent_source` to target bind records so diagnostics can distinguish
source hierarchy from emitted target hierarchy.

Added and rejected diagnostic `matrix-local-axis-target-parent-bind`, which
emits the source/Control_Root-composed pelvis world as local when the target
pelvis has no parent. Direct GHC pelvis-only captures at attack frames
0/15/30/45 roll the character upside-down / horizontal, so the missing
alignment is not "apply the Control_Root world directly." Evidence:
`.codex/current-evidence/midori-pelvis-targetparent-20260817/targetparent_pelvis_contact.png`.

Added diagnostic env override `GH3_MIDORI_SIGNED_AXIS_INDEX` and swept all 24
proper signed-axis bases for pelvis-only `matrix-local-axis-align-bind` at
attack frames 15/30. Most bases are obvious screen-roll rejects. Axes 12-15 are
the only useful family; full-gate frames 0/15/30/45 for axes 12-15 show the
same safe orientation family as the existing automatic pelvis axis. Axis 14 is
the automatic pelvis choice and remains the best current pelvis constant:
`[[0,1,0],[0,0,-1],[-1,0,0]]`. Evidence:
`.codex/current-evidence/midori-pelvis-axis-sweep-20260817/`.

Added diagnostic `matrix-knee-vector-aim-targetgraph-gated-postblend65-bind`,
which recomputes the knee-vector postblend65 world chain through the target
parent graph instead of the GH3 source parent graph. After fixing the helper to
apply to the knee-vector path, direct GHC attack frames 30/45 are stable but
regress to the too-straight local-axis silhouette. Reject it as an improvement
over `matrix-knee-vector-aim-gated-postblend65-bind`. Evidence:
`.codex/current-evidence/midori-targetgraph-knee-20260817/targetgraph65_fixed_attack_contact.png`.

Next: keep pelvis on the automatic axis-14 `matrix-local-axis-align-bind`
family, keep old postblend65 as the best knee diagnostic only, and move the
source-authoritative bridge/constraint work toward a real two-bone leg solve
that preserves the pelvis constant while reproducing the source hip-knee-ankle
shape. Do not spend more time on Hmx packing, direct Control_Root world
application, or target-graph-only knee-vector retests.

## 2026-08-17 continuation: two-bone IK diagnostics

Added analytic two-bone leg IK diagnostics in `tools/gh3_midori_acp_stage.py`.
The solver maps source hip/knee/ankle positions through the lower-body
source-to-target basis, clamps source ankle distance to GH2 thigh/shin bind
lengths, preserves the source bend side (or flipped bend side), then aims
thigh and knee segments together from the solved target-length chain.

Policies added:

- `matrix-leg-ik-bind`
- `matrix-leg-ik-flip-bind`
- `matrix-leg-ik-flipblend35-bind`
- `matrix-leg-ik-flipblend50-bind`
- `matrix-leg-ik-pelvisgate-flipblend35-bind`
- `matrix-leg-ik-pelvisgate-flipblend50-bind`

Visual result:

- Raw `matrix-leg-ik-bind` creates real attack tuck but crosses/folds the legs
  too hard.
- `matrix-leg-ik-flip-bind` is less crossed in attack but breaks neutral
  frames into a crouched/seated pose; reject as broad policy.
- Ungated 35/50 flipped blends still damage idle, so strength alone is not the
  right control.
- Source pelvis delta is a useful gate: idle/jump/transition are about
  `4/15/6` degrees, while attack and solo are about `79-108` and `44` degrees.
- `matrix-leg-ik-pelvisgate-flipblend50-bind` preserves idle/jump/transition
  and adds attack/solo bend, but pushes solo/attack more aggressively.
- `matrix-leg-ik-pelvisgate-flipblend35-bind` is the better current IK
  candidate: neutral frames stay on the safe local-axis baseline and
  attack/solo gain visible bend without the full 50% overpush. It is not final
  and has not passed full-bank visual approval.

Evidence retained:
`.codex/current-evidence/midori-leg-ik-20260817/`.

Next: broaden `matrix-leg-ik-pelvisgate-flipblend35-bind` beyond the
representative six-frame sample. If it remains stable, compare it directly
against source and postblend65 for attack/solo, then decide whether to promote
it as the lower-body diagnostic candidate or continue with a Blender/GLB
constraint bake.

## 2026-08-17 continuation: gated IK full-bank check

`matrix-leg-ik-pelvisgate-flipblend35-bind` was staged across the full external
animation set and the full `guitar-main` bank was built:

- ACP stage: `331` clips
  (`guitar-main:266`, `guitar-fret:36`, `guitar-strum:23`, `guitar-ui:6`)
- main MILO: `266` clips, `5` lower-body bones, `279` entries

Direct GHC captures from that full main bank for idle, attack frame 30, attack
frame 45, jump, solo, and transition at yaws 0/180 match the per-clip
diagnostic: idle/jump/transition remain on the neutral-safe local-axis family,
and attack/solo gain more leg bend than postblend65. This is the best current
lower-body diagnostic candidate.

It is still not approved: compared with the source sheet, the candidate moves
toward the GH3 crouch but does not yet reproduce the full source hip-knee-ankle
shape. Evidence:
`.codex/current-evidence/midori-leg-ik-fullbank-20260817/full_legikgate35_contact.png`.

Next: either tune the gated IK solve against source attack/solo directly
(likely knee-plane/bend-axis and per-side strength) or move to the automated
Blender/GLB constraint bake. Do not regress to postblend65 unless a later
branch breaks neutral safety.

## 2026-08-17 continuation: scaled IK distance probe

The first IK solver used mapped source hip-to-ankle distance directly while GH2
thigh/shin bind lengths are in a much larger target scale. Source rest
hip-ankle maps to about `0.7814`, while GH2 target rest hip-ankle is about
`33.2247` (`~42.5x`). Added scaled-distance diagnostics:

- `matrix-leg-ik-scaled-pelvisgate-flip-bind`
- `matrix-leg-ik-scaled-pelvisgate-flipblend35-bind`
- `matrix-leg-ik-scaled-pelvisgate-flipblend50-bind`

These convert source animated hip-ankle distance to a source-rest ratio, then
apply that ratio to GH2 target rest hip-ankle distance before solving the
target-length IK triangle.

Visual result: reject the scaled-distance branch. Raw scaled produces a
seated/horizontal attack and solo pose; scaled 35/50 move back toward the
too-straight silhouette. Evidence:
`.codex/current-evidence/midori-scaled-ik-20260817/scaled_ik_all_contact.png`.

The best live lower-body candidate remains the full-bank-stable unscaled
`matrix-leg-ik-pelvisgate-flipblend35-bind`. The next useful work is not source
distance scaling; it is bend-plane/per-side tuning or a Blender/GLB constraint
bake.

## 2026-08-17 continuation: ankles and toes under gated IK

Expanded the current best lower-body branch down the hierarchy while keeping
the same `matrix-leg-ik-pelvisgate-flipblend35-bind` policy:

- pelvis/thigh/knee/ankle broad probe
- pelvis/thigh/knee/ankle/toe broad probe
- full all-clips pelvis/thigh/knee/ankle/toe bank

Visual result: ankles and toes are stable additions. Idle and transition remain
neutral-safe, attack/jump/solo keep the same readable lower-body family, and no
foot flips or hierarchy explosions appeared. Full all-clips staging/build
succeeded with `331` staged clips and a `266`-clip main bank; the rebuilt main
MILO is `9,500,500` bytes.

This is a safe hierarchy expansion, not final approval. The candidate still
does not reproduce the full GH3 source hip-knee-ankle silhouette. Evidence:
`.codex/current-evidence/midori-leg-ik-ankletoe-20260817/`.

Next: do not retest source distance scaling. Continue with either
bend-plane/per-side tuning of the full-bank-stable unscaled gated IK solve, or
an automated Blender constraint bake. A `.glb` intermediate is acceptable if the
pipeline remains automated and the final output is ordinary ark-external GHC DLC
assets.

## 2026-08-17 continuation: bend-plane gated IK sweep

Added pelvis-gated unscaled IK strength and bend-plane variants:

- `matrix-leg-ik-pelvisgate-flipblend42-bind`
- `matrix-leg-ik-pelvisgate-flipblend35-bendin15-bind`
- `matrix-leg-ik-pelvisgate-flipblend35-bendout15-bind`
- `matrix-leg-ik-pelvisgate-flipblend42-bendin15-bind`
- `matrix-leg-ik-pelvisgate-flipblend42-bendout15-bind`
- `matrix-leg-ik-pelvisgate-flipblend35-bendin45-bind`
- `matrix-leg-ik-pelvisgate-flipblend35-bendout45-bind`
- `matrix-leg-ik-pelvisgate-flipblend42-bendin45-bind`
- `matrix-leg-ik-pelvisgate-flipblend42-bendout45-bind`

The 15-degree bend-plane sweep was visually stable but too subtle. The 45-degree
sweep showed the best improvement at
`matrix-leg-ik-pelvisgate-flipblend42-bendout45-bind`: attack and solo are a
little more compact than the old 35% full-bank candidate, while idle, jump, and
transition remain on the neutral-safe baseline.

Full all-clips staging/build of
`matrix-leg-ik-pelvisgate-flipblend42-bendout45-bind` succeeded:

- ACP stage: `331` clips
  (`guitar-main:266`, `guitar-fret:36`, `guitar-strum:23`, `guitar-ui:6`)
- main MILO: `266` clips, `9` lower-body bones, `283` entries, `9,498,516`
  bytes

This is now the best live lower-body diagnostic candidate, but it is still not
source-approved. It improves the fold slightly; it does not yet reproduce the
full GH3 source hip-knee-ankle silhouette. Evidence:
`.codex/current-evidence/midori-leg-ik-bendplane-20260817/`.

Next: continue from `matrix-leg-ik-pelvisgate-flipblend42-bendout45-bind` only
if doing another local IK refinement. Otherwise move to the automated
Blender/GLB bridge path and bake source-authoritative evaluated poses into
ordinary ark-external GHC DLC assets.

## 2026-08-17 continuation: source bridge export proof

Blender is now callable from this shell at:

`C:\Program Files\Blender Foundation\Blender 4.5\blender.exe`

Hardened `tools/gh3_midori_source_visual.py` so `--pose-json` records include
both the actual Blender pose bone (`bone`) and the GH3 source alias
(`source_bone`). This fixes the lower-body name mismatch where NXTools imports
some source bones as `bone_pelvis`, `bone_thigh_l`, etc., while the retargeting
pipeline keys them as `Bone_Pelvis`, `Bone_Thigh_L`, etc.

Added `tools/gh3_midori_source_bridge_export.py`, a one-command automated
wrapper around Blender/NXTools source export. It writes:

- source-keyed evaluated pose JSON
- an animated skinned GLB bridge asset
- bounded stdout/stderr logs

It also validates the requested frames/source bones and confirms the GLB has
skins, animations, and meshes.

Executed the wrapper against the exact attack diagnostic source inputs:

- clip: `gh3_guit_mido_a_attackl`
- frames: `0,15,30,45`
- bones: pelvis, thighs, knees, ankles, toes
- result: `36` pose records, `9` source bones, GLB v2 with `1` skin,
  `1` animation, `2` meshes, `76` nodes, and zero stderr bytes

Evidence:
`.codex/current-evidence/midori-source-bridge-export-20260817/`.

Next: consume this pose bridge in the retargeting side. Either add an ACP-stage
diagnostic that can read source bridge pose records for sampled frames, or
build a Blender/GLB constraint-bake path that writes ordinary GH2/GHC local
rotations directly.

## 2026-08-17 continuation: bridge-fed retarget proof

Added bridge-pose consumption to `tools/gh3_midori_acp_stage.py`:

- `--source-pose-bridge-json PATH` loads
  `gh3_midori_source_pose_bridge_v1` pose JSON.
- Bridge positions are keyed by `source_bone` and frame.
- The override is clip-guarded: bridge positions are used only when the staged
  source clip matches the bridge animation clip; missing frames fall back to the
  existing recursive evaluator.
- The stage manifest records `source_pose_bridge_json`,
  `source_pose_bridge_animation_clip`, and per-clip
  `source_pose_bridge_active` / `source_pose_bridge_frames`.

Added explicit diagnostic policy:

`matrix-leg-ik-bridge-pelvisgate-flipblend42-bendout45-bind`

This policy follows the current best local IK branch
`matrix-leg-ik-pelvisgate-flipblend42-bendout45-bind`, but can be driven by the
Blender/NXTools bridge positions at matching frames.

Focused validation:

- clip: `gh3_guit_mido_a_attackl`
- bridge JSON:
  `.codex/current-evidence/midori-source-bridge-export-20260817/gh3_guit_mido_a_attackl.pose_bridge.json`
- staged bones: pelvis, thighs, knees, ankles, toes
- result: `source_pose_bridge_active=True`, bridge frames `0,15,30,45`
- single-clip main MILO built: `1` clip, `7` bones, `9` entries
- GHC captures succeeded for frames `0,15,30,45` at yaws `0/180`

Visual result: stable and in the same family as `out42_45`; this proves the
source bridge now reaches ordinary ACP/MILO/GHC assets, but it is not a final
source match. Evidence:
`.codex/current-evidence/midori-bridgefed-retarget-20260817/`.

Next: use the bridge-fed path to solve rotations from source-authoritative
evaluated transforms, not only positions. The useful next diagnostic is a
bridge-pose local/world alignment solve against target Glam bind for the same
attack frames, then GHC visual comparison against the current `out42_45`
candidate.

## 2026-08-17 continuation: bridge evaluated-rotation probe

Extended `--source-pose-bridge-json` consumption in
`tools/gh3_midori_acp_stage.py` so the bridge loader stores evaluated rotation
matrices in addition to source-bone/frame positions. Added two explicit
bridge-eval diagnostics:

- `matrix-bridge-eval-world-axis-align-bind`
- `matrix-bridge-eval-knee-world-axis-align-bind`

Both require `--source-pose-bridge-json`, use the bridge rotations when the
clip/frame/source-bone matches, and fall through the existing evaluated-pose
alignment code.

Focused validation:

- clip: `gh3_guit_mido_a_attackl`
- frames: `0,15,30,45`
- bones: pelvis, thighs, knees, ankles, toes
- both stages reported `source_pose_bridge_active=True`
- both single-clip main MILOs built
- GHC captures succeeded at yaws `0/180`

Visual result:

- Reject `matrix-bridge-eval-world-axis-align-bind`: it rolls the whole
  character horizontal, matching the old failure family for direct evaluated
  world alignment.
- `matrix-bridge-eval-knee-world-axis-align-bind` is stable, but it does not
  improve over the current `out42_45` lower-body candidate.

Evidence:
`.codex/current-evidence/midori-bridge-evalrot-20260817/`.

Next: do not repeat direct bridge-evaluated world alignment. Use the bridge
rotations to derive target-local deltas or constraints, with target Glam bind
as the destination basis, then compare against `out42_45`.

## 2026-08-17 continuation: bridge target-local rotation probe

Added target-local bridge rotation diagnostics:

- `matrix-bridge-eval-local-axis-align-bind`
- `matrix-bridge-eval-knee-local-axis-align-bind`

These derive source parent-space local rotations from the bridge-evaluated
parent/child pose matrices, then align those source local rotations into target
Glam local bind space. This tests the "use bridge rotations as local deltas"
idea without feeding armature/world space directly to GHC.

Focused validation:

- clip: `gh3_guit_mido_a_attackl`
- frames: `0,15,30,45`
- bones: pelvis, thighs, knees, ankles, toes
- both stages reported `source_pose_bridge_active=True`
- both single-clip main MILOs built
- GHC captures succeeded at yaws `0/180`

Visual result:

- Reject `matrix-bridge-eval-local-axis-align-bind`: late frames roll/horizontal
  again.
- `matrix-bridge-eval-knee-local-axis-align-bind` is stable, but it regresses
  toward the too-straight silhouette and is worse than `out42_45`.

Evidence:
`.codex/current-evidence/midori-bridge-localrot-20260817/`.

Next: stop trying direct bridge rotation alignment, whether world or local.
The remaining bridge path should use the source-authoritative bridge as a
constraint target for solved target-space bones: fit target hip/knee/ankle
vectors and bend planes under Glam bind lengths, then emit GH2 local rotations.
The current best comparison baseline remains full-bank-stable
`matrix-leg-ik-pelvisgate-flipblend42-bendout45-bind`.

## 2026-08-17 continuation: bridge IK blend strength sweep

Added stronger bridge-constrained IK blend policies:

- `matrix-leg-ik-bridge-pelvisgate-flipblend46-bendout45-bind`
- `matrix-leg-ik-bridge-pelvisgate-flipblend50-bendout45-bind`

These use the same source-authoritative bridge positions, Glam thigh/shin
lengths, pelvis-motion gate, and bendout45 plane as the existing bridge-fed
`matrix-leg-ik-bridge-pelvisgate-flipblend42-bendout45-bind`; only the blend
strength differs.

Focused validation:

- clips: `gh3_guit_mido_a_attackl`, `gh3_guit_mido_a_med_idle01`,
  `gh3_guit_midori_tran_atoout`
- frames/cases: idle `60`, attack `15/30/45`, transition `50`
- yaws: `0/180`
- all single-clip MILOs built and captured in GHC

Visual result: all three strengths are stable. However, 46% and 50% add little
or no material improvement over bridge42 / current `out42_45`; the remaining
source-fit gap is not solved by simply increasing blend strength in this
target-length IK family. Evidence:
`.codex/current-evidence/midori-bridge-ik-blend-20260817/`.

Next: the useful bridge branch should change the constraint model, not just
blend amount. Candidate directions: solve pelvis/thigh orientation and leg IK
as a coupled system from bridge hip/knee/ankle/torso vectors, or bake a
Blender/GLB target-skeleton constraint solve that emits ordinary GH2 local
rotations. Keep `out42_45` as the stable comparison baseline.

## 2026-08-17 continuation: bridge lower-body frame probe

Added coupled pelvis/lower-body frame diagnostic:

`matrix-leg-ik-bridgeframe50-pelvisgate-flipblend42-bendout45-bind`

This derives a dynamic lower-body frame from bridge thigh/knee positions, maps
that frame into target lower-body space, applies the Glam pelvis bind
correction, blends it 50% with the stable baseline pelvis, then uses the current
bridge-fed `out42_45` leg IK.

Focused validation:

- clips: `gh3_guit_mido_a_attackl`, `gh3_guit_mido_a_med_idle01`,
  `gh3_guit_midori_tran_atoout`
- frames/cases: idle `60`, attack `15/30/45`, transition `50`
- yaws: `0/180`
- single-clip MILOs built and captured in GHC

Visual result: stable, but almost identical to bridge42 / current `out42_45`.
A simple dynamic lower-body pelvis-frame blend does not close the source-fit
gap. Evidence:
`.codex/current-evidence/midori-bridge-frame-20260817/`.

Next: stop small local parameter sweeps around `out42_45`. The next meaningful
bridge route is a true target-skeleton constraint bake or a coupled solver that
solves target pelvis/thigh/knee rotations together from bridge hip/knee/ankle
and torso vectors, then emits ordinary GH2 local rotations.

## 2026-08-17 continuation: bridge constraint-chain probe

Added target-length solved-chain diagnostic:

`matrix-leg-constraint-bridge-pelvisgate-bind`

This keeps pelvis on the known-good `matrix-local-axis-align-bind`/bridge42
family, uses the Blender/NXTools source pose bridge for hip/knee/ankle
positions, solves target-length thigh/shin points, then emits full thigh/knee
local rotations from the solved leg plane. It is a constraint-model branch, not
another local blend-strength sweep.

Validation completed:

- `python -m py_compile tools\gh3_midori_acp_stage.py tools\gh3_midori_pipeline_test.py`
- `python tools\gh3_midori_pipeline_test.py` (`9` tests OK)
- staged `gh3_guit_mido_a_attackl` with
  `source_pose_bridge_active=true`
- built both bridge42 baseline and constraint one-clip `guitar-main`
  CharClipSamples MILOs with `milo_convert_tool`

Data result: the branch is not a no-op. At frame `30`, pelvis matches bridge42
exactly while thigh/knee quats change materially:

- bridge42 `bone_L-thigh`: `0.444274,-0.196276,0.810942,-0.326297`
- constraint `bone_L-thigh`: `0.672149,-0.388658,-0.237332,0.58381`
- bridge42 `bone_L-knee`: `0.0433078,0.726088,-0.327997,0.602776`
- constraint `bone_L-knee`: `8.6906e-08,-0.651884,0.758318,-0.000303791`

First data evidence:
`.codex/current-evidence/midori-constraint-bridge-20260817/`.

## 2026-08-17 continuation: constraint-chain direct visual rejection

Mounted `Guitar Hero II PS2 (USA).iso`, which restored the old native-viewer
base archive as `D:\GEN`. Re-staged bridge42 and
`matrix-leg-constraint-bridge-pelvisgate-bind` with the correct
`--translation-policy animated` setting from the earlier bridge-fed proof, built
one-clip main MILOs, temporarily swapped each into the loose Midori DLC, captured
`gh3_guit_mido_a_attackl` frames `15/30/45` at yaws `0/90/180/270`, and restored
the deployed main MILO afterward.

All captures loaded the intended clip and emitted pose-publisher output, so this
is a valid applied-clip visual check. Result: rejected. The constraint branch
tracks bridge42 and does not move toward the source f30/f45 lifted-leg/back-lean
silhouette. It also does not fix the remaining source-fit gap from the stable
non-bridge `out42_45` family. Evidence:
`.codex/current-evidence/midori-constraint-bridge-visual-20260817/`.

Next: stop pursuing this solved-plane constraint-chain branch. The next useful
route is a real target-skeleton bake/solve, likely via Blender/GLB or an
equivalent target rig pass, that solves pelvis/spine/legs together and emits
ordinary GH2 local rotations. Keep `matrix-leg-ik-pelvisgate-flipblend42-bendout45-bind`
as the stable comparison baseline, not the rejected bridge constraint branch.

## 2026-08-17 continuation: torso-aware source bridge

Patched `tools/gh3_midori_source_visual.py` so the Blender/NXTools loader can
load a checkout through an explicit package alias instead of resolving a temp
junction back to `nxtools_ref`. This restores headless bridge export when the
pinned NXTools checkout is exposed as `%TEMP%\gh3_midori_nxtools_checkout`.

Regenerated a source bridge for `gh3_guit_mido_a_attackl` with pelvis, spine,
head, and lower-body bones active:

- frames: `0,15,30,45`
- bones: `Control_Root`, pelvis, stomach lower/upper, chest, neck/head,
  thighs, knees, ankles, toes
- evidence: `.codex/current-evidence/midori-source-bridge-torso-20260817/`

Important result: the honor-partial Blender/NXTools export retained only neck
and one ankle curve, despite local IR showing `668` quaternion keys and `91`
translation keys. The useful bridge variant must use `--ignore-partial-anims`.
That variant validates as `60` pose records, GLB v2 with `1` skin, `1`
animation, `2` meshes, `76` nodes, zero stderr bytes, and retained pelvis,
stomach, chest, neck/head, and leg curves. At frame `30`, source pelvis basis
row0 is approximately `0.25,-0.429,0.868`; at frame `45`, row0 is
approximately `-0.281,-0.37,0.885`, so the torso bridge now carries the
source-authoritative body lean needed by the next solve.

Next: consume
`.codex/current-evidence/midori-source-bridge-torso-20260817/gh3_guit_mido_a_attackl.torso.ignorepartial.pose_bridge.json`
in a real target-skeleton bake/solve. Prefer solving pelvis/spine/legs
together from these evaluated matrices, then emitting ordinary GH2 local ACP
rotations. Do not use the honor-partial bridge or the rejected
`matrix-leg-constraint-bridge-pelvisgate-bind` branch as the next baseline.

## 2026-08-17 continuation: torso bridge + leg IK diagnostic

Added diagnostic policy:

`matrix-torso-bridge-leg-ik-pelvisgate-flipblend42-bendout45-bind`

This is the first ACP-side consumer of the ignore-partial torso bridge that
couples the upper hierarchy and legs in one target-space solve. It uses
bridge-evaluated local axis alignment for pelvis/spine/neck/head, uses that
moving pelvis/torso chain as the target parent for the legs, and applies the
previously stable `42`/`bendout45` leg IK correction under that parent. It
still emits ordinary GH2 local quaternion channels; production defaults are not
changed.

Validation completed:

- `python -m py_compile tools\gh3_midori_acp_stage.py tools\gh3_midori_pipeline_test.py tools\gh3_midori_source_visual.py tools\gh3_midori_source_bridge_export.py`
- `python tools\gh3_midori_pipeline_test.py` (`10` tests OK)
- staged one-clip `gh3_guit_mido_a_attackl` with
  `source_pose_bridge_active=true`, `translation_policy=animated`, and the
  ignore-partial torso pose bridge

Numeric result: at frames `30/45`, pelvis and spine quats now follow the
torso bridge rather than bridge42, while leg channels stay closer to the stable
IK family than the direct `matrix-bridge-eval-local-axis-align-bind` leg output.
Evidence:
`.codex/current-evidence/midori-torso-bridge-legik-20260817/`.

Next: build this one-clip candidate into a temporary `guitar-main` MILO and run
the same native GHC yaw/frame visual capture used for the constraint rejection.
Do not promote it until direct source-side visual comparison says it improves
the f30/f45 lifted-leg/back-lean silhouette without breaking stability.

## 2026-08-17 continuation: valid grouped visual reject

Visual capture uncovered a harness issue before the actual pose result: the
one-clip generated `guitar-main` MILO had the requested
`gh3_guit_mido_a_attackl` clip, but no `CharClipGroup`, because the staged ACP
had `clip_flags=0`. GHC therefore reported `active=<none>` and the diagnostic
clip override as missing. `milo_convert_tool build-clipset-from-acp` now emits
a fallback `normal` `CharClipGroup` containing all generated guitar-main clips
when no source clip carries group masks. The regenerated candidate inspected as
`entries=15` with `group normal clips=gh3_guit_mido_a_attackl`.

After that harness fix, valid GHC captures at attack frames `15`, `30`, and
`45` all accepted the diagnostic performer clip and restored the deployed loose
DLC main MILO afterward. The pose is rejected: frame `15` still shows vertical
leg failure, and frames `30/45` floor-fold or roll the character sideways
instead of matching the upright arched source silhouette. Evidence:
`.codex/current-evidence/midori-torso-bridge-legik-visual-20260817/`.

Next: keep the grouped-clipset fallback, but do not continue this torso policy
as a candidate. The failure is now a real target solve failure, not an invalid
runtime clip-selection capture. Resume with a deeper automated target-skeleton
solve from the ignore-partial GLB/pose-bridge matrices, likely solving pelvis
orientation and root/control-frame placement together before leg IK.

## 2026-08-17 continuation: target-graph Control_Root diagnosis

Root/pelvis diagnosis: `Control_Root` has a GH2 bind record, but guitar-main
suppresses `Control_Root` rotation/translation channels, and the target Glam
`bone_pelvis` is root-parented (`target_parent_source=""`). The rejected torso
policy incorrectly localized pelvis through source parent `Control_Root`, so
the emitted pelvis channel was relative to a parent rotation GHC never applies.

Added diagnostic policy:

`matrix-torso-targetgraph-bridge-leg-ik-pelvisgate-flipblend42-bendout45-bind`

This keeps the bridge torso/leg logic but composes and localizes through the
actual GH2 target parent graph. One-clip staging/build/capture is valid and the
clip override is accepted, but visual result remains rejected: frame `15` still
has the vertical-leg failure and frame `45` still floor-folds/horizontals. This
proves target-graph parent localization is necessary but not sufficient.
Evidence:
`.codex/current-evidence/midori-torso-targetgraph-bridge-20260817/`.

Added follow-up diagnostic:

`matrix-torso-targetgraph-stablepelvis-bridge-leg-ik-pelvisgate-flipblend42-bendout45-bind`

This keeps `Bone_Pelvis` on the known-good `matrix-local-axis-align-bind`
family while applying target-graph bridge torso/leg IK below that stable pelvis.
It stages/builds/captures validly and changes the late-frame failure shape, but
is still rejected: frames `15/30/45` do not match the source arched lifted-leg
silhouette. Evidence:
`.codex/current-evidence/midori-torso-targetgraph-stablepelvis-20260817/`.

Next: retain the converter `normal` group fallback and target-graph
parent-localization for root-flattened policies. Do not drive pelvis directly
from bridge evaluated local torso rotation, and do not rely on stable pelvis
alone. The next useful branch should constrain bridge leg endpoints in target
world/camera space under the stable pelvis, or otherwise solve root-frame,
pelvis, and leg endpoints together before converting to GH2 locals.

## 2026-08-17 continuation: stable pelvis bridge endpoint variants

Added endpoint-map diagnostics under the stable target-graph pelvis:

- `matrix-torso-targetgraph-stablepelvis-bridgeendpoints-leg-ik-pelvisgate-flipblend42-bendout45-bind`
- `matrix-torso-targetgraph-stablepelvis-bridgeendpointsaxis-leg-ik-pelvisgate-flipblend42-bendout45-bind`

Both map bridge knee/ankle endpoints through the source evaluated pelvis-local
frame, then rotate those endpoint directions into the stable target pelvis
frame before the existing 42/bendout45 leg IK. The first uses the transpose
signed-axis vector map; the second uses the direct signed-axis vector map. Both
stage, build, load with the generated `normal` clip group, accept the diagnostic
clip override, and restore the deployed loose DLC main MILO.

Visual result: both are rejected. They stabilize frames `30/45` compared with
the earlier floor-fold branches, so the pelvis-frame endpoint diagnosis is
useful, but neither matches the source arched lifted-leg silhouette and frame
`15` remains in the vertical-leg failure family. Evidence:
`.codex/current-evidence/midori-torso-targetgraph-stablepelvis-bridgeendpoints-20260817/`
and
`.codex/current-evidence/midori-torso-targetgraph-stablepelvis-bridgeendpointsaxis-20260817/`.

Next: both signed-axis endpoint directions have now failed as simple aim
corrections. Continue with a fuller automated pose reconstruction: solve the
root/control frame, pelvis, and leg endpoints together from the evaluated
bridge, then emit GH2 locals under the actual target graph. A `.glb`
intermediate remains acceptable as long as the final DLC assets are automated
ordinary ark-external GHC assets.

## 2026-08-17 continuation: GLB source row and root-world posbind

Added `tools/gh3_midori_glb_source_render.py` and rendered the ignore-partial
source bridge GLB directly at attack frames `15/30/45`. This corrected the
working visual target: the full evaluated source pose is a horizontal arched
pose at all three frames, including frame `15`. Evidence:
`.codex/current-evidence/midori-source-glb-reference-20260817/`.

Added root-world torso diagnostic:

`matrix-torso-targetgraph-rootworld-bridgeendpointsaxis-leg-ik-pelvisgate-flipblend42-bendout45-bind`

It maps evaluated source world/root rotations for pelvis/torso into the GH2
target graph, then uses the direct signed-axis bridge endpoint IK below that
pelvis. It captures validly and is the first branch in this group to make target
frame `15` horizontal like the GLB source, proving the old stable-pelvis f15
upright result was wrong. It is rejected because the one-clip stage emitted
`bone_pelvis.pos` near zero, so the character collapsed into/through the stage.
Evidence:
`.codex/current-evidence/midori-torso-targetgraph-rootworld-bridgeendpointsaxis-20260817/`.

Added follow-up placement diagnostic:

`matrix-torso-targetgraph-rootworld-posbind-bridgeendpointsaxis-leg-ik-pelvisgate-flipblend42-bendout45-bind`

This keeps the same root-world rotations but composes the animated pelvis
translation with the GH2 target bind translation. The emitted `bone_pelvis.pos`
returns to bind-height scale (`z` around `39.4` in the diagnostic one-clip MILO
instead of near zero), and GHC captures load/restore correctly. Visual result is
still rejected: bind-height fixes floor collapse, but target frames `15/30/45`
do not match the GLB source silhouette and remain offset/intersecting the
gameplay scene. Evidence:
`.codex/current-evidence/midori-torso-targetgraph-rootworld-posbind-bridgeendpointsaxis-20260817/`.

Next: stop treating the current failure as a rotation-only problem. Bake a
target-skeleton pose from the evaluated bridge/GLB positions: solve pelvis
placement, torso, neck/head, and leg endpoints together in target bind space,
then emit ordinary GH2 local `CharClipSamples` under the actual target graph.

## 2026-08-17 continuation: position-bake no-op

Added two evaluated-position bake diagnostics:

- `matrix-torso-targetgraph-rootworld-posbind-positionbake-bind`
- `matrix-torso-targetgraph-rootworld-posbind-positionframe-bind`

`positionbake` keeps the root-world/posbind base and corrects each staged
torso/leg bone toward its mapped evaluated bridge child vector. It stages,
builds, loads, captures, and restores validly. Visual result is rejected and
pixel-identical to the previous rootworld-posbind contact sheet at frames
`15/30/45`, proving this correction is mathematically redundant with the
evaluated root-world rotations already being emitted.

`positionframe` derives each aimed bone's world frame directly from mapped
bridge parent/child position frames and target bind frames. It stages/builds to
the exact same one-clip MILO SHA256 as `positionbake`, so capture was skipped
as a proven duplicate. Evidence:
`.codex/current-evidence/midori-torso-targetgraph-rootworld-posbind-positionbake-20260817/`
and
`.codex/current-evidence/midori-torso-targetgraph-rootworld-posbind-positionframe-20260817/`.

Next: the evaluated bridge rotations and evaluated bridge position frames agree.
The remaining root-world failure is no longer solved by local orientation bake.
Continue by deriving the pelvis/root placement contract from the source bridge
absolute/evaluated `Control_Root` + `Bone_Pelvis` translations into GH2
guitar-main `bone_pelvis.pos`, then verify in GHC.

## 2026-08-17 continuation: bridge pelvis-position probes

Added pelvis-placement diagnostics:

- `matrix-torso-targetgraph-rootworld-bridgepos-bind`
- `matrix-torso-targetgraph-rootworld-bridgepos-unscaled-bind`

Both keep the root-world torso rotations, but replace `bone_pelvis.pos` with
the evaluated bridge `Bone_Pelvis` delta mapped through the existing
source-to-target lower-body basis. `bridgepos` applies the legacy
`72/467.25` GH3-to-GH2 translation scale. `bridgepos-unscaled` uses the same
mapped bridge delta without that scale.

Both stage/build/load/capture/restore validly and are rejected. The scaled
branch changes emitted pelvis samples and only frame `30` pixels differ
materially from posbind; frames `15/45` remain visually identical. The unscaled
branch moves the pelvis channel much more, but again only frame `30` changes
materially and the source silhouette still rejects. Evidence:
`.codex/current-evidence/midori-torso-targetgraph-rootworld-bridgepos-20260817/`
and
`.codex/current-evidence/midori-torso-targetgraph-rootworld-bridgepos-unscaled-20260817/`.

Next: `bone_pelvis.pos` alone is not the missing placement contract. Continue
by tracing how GHC applies guitar-main root/recenter/move_self performer
placement and whether the source `Control_Root` absolute translation must be
represented outside `bone_pelvis.pos` or via clip/character root metadata.

## 2026-08-17 continuation: pelvis-only move_self diagnosis

Traced GHC guitar-main application:

- `build-clipset-from-acp` was forcing `move_self=1` for every generated
  `guitar-main` clipset.
- Gameplay keeps performer placement in `perf.world_transform` and applies
  main animation through a `ClipChannelLayerStack` into
  `apply_character_pose_controller_frame`; changing clipset self-motion does
  not rewrite the performer world transform for these diagnostic captures.

Added a diagnostic converter option:

`milo_convert_tool build-clipset-from-acp ... --move-self 0|1`

Default behavior is unchanged. Built the same
`matrix-torso-targetgraph-rootworld-bridgepos-bind` one-clip MILO twice and
verified that samples are identical while root metadata differs:
default `move_self=1`; diagnostic `move_self=0`.

Temporarily swapped the `move_self=0` one-clip MILO into the loose Midori DLC,
mounted the GH2 ISO only for the capture window, captured frames `15/30/45`,
and restored the deployed main MILO to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
The diagnostic clip override and pose publisher were active. Pixel comparison
against the prior `bridgepos` rejection is exactly zero changed pixels at all
three frames:

`.codex/current-evidence/midori-moveself0-rawstage-20260817/moveself0_vs_bridgepos_metrics.tsv`

Conclusion: root `move_self` is not swallowing the local pelvis placement in
this failure. Continue at the performer/root placement contract: source
`Control_Root` absolute translation must likely be represented outside
`bone_pelvis.pos` (for example as an automated GLB/Control_Root placement
bridge or runtime-compatible character root placement metadata), rather than as
another pelvis-local `.pos` variant.

## 2026-08-17 continuation: root placement channel probes

Added diagnostics:

- `matrix-torso-targetgraph-rootworld-facingpos-bridgepos-bind`
- `matrix-torso-targetgraph-rootworld-absposepos-bind`

`facingpos` keeps the prior rootworld `bridgepos` pelvis placement and emits
mapped absolute evaluated `Control_Root` placement as synthetic
`bone_facing.pos` (`-11.1802, 8.66232, 3.31421`). The generated clipset has the
channel and GHC accepts the diagnostic clip override, but all captured frames
`15/30/45` are pixel-identical to the prior `bridgepos` rejection. Evidence:
`.codex/current-evidence/midori-facingpos-rawstage-20260817/`.

`absposepos` composes mapped absolute evaluated `Bone_Pelvis` placement
(including the constant `Control_Root` offset) into `bone_pelvis.pos`. It emits
pelvis positions around `(-11.3, 8.76, 42.7)` instead of `(0, 0, 39.4)`.
GHC captures validly and frame `30` changes materially, but frames `15/45` are
pixel-identical to `bridgepos` and the silhouette remains rejected. Evidence:
`.codex/current-evidence/midori-absposepos-rawstage-20260817/`.

Conclusion: neither clipset `move_self`, standalone `bone_facing.pos`, nor
absolute pelvis pose offset is the complete root placement contract. The next
branch should stop adding raw root-position channels and instead change the
generated output graph or solve the target skeleton pose from the evaluated
GLB positions: e.g. test a generated `bone_facing -> bone_pelvis` CharBone
hierarchy/parenting contract, or bake the GLB-derived target pose before MILO
emission.

## 2026-08-17 continuation: GLB-derived local position bake

Added diagnostic:

- `matrix-torso-targetgraph-rootworld-glbposepos-bind`

This keeps the root-world target-graph rotation path but emits synthetic `.pos`
channels for pelvis, spine/neck, and both leg chains from evaluated source GLB
world positions localized under the actual GH2 target graph. The staged sample
set includes `bone_pelvis.pos`, `bone_spine1.pos`, `bone_spine2.pos`,
`bone_spine3.pos`, `bone_neck.pos`, and thigh/knee/ankle/toe channels for both
legs.

GHC accepts the one-clip override and all tested frames `15/30/45` change
materially against the prior `bridgepos` rejection (`~802k`, `~817k`, and
`~809k` changed pixels respectively). This is the first root/pose placement
branch in this sequence to affect every tested frame through the visible
guitar-main path. Evidence:
`.codex/current-evidence/midori-glbposepos-rawstage-20260817/`.

Direct visual inspection rejects it. Raw GLB-derived local position tracks
over-extend the lower body and torso into non-source-like stretched shapes. The
result proves the position channel path can move the visible pose, but it is not
an anatomically valid target-skeleton bake.

Conclusion: GLB remains acceptable as an automated bridge format, but do not
promote raw per-joint GLB local positions. Next branch should use evaluated
GLB/source positions as constraints for a target-length-preserving skeleton
solve: preserve GH2 bone lengths, solve root/pelvis/torso and two-bone leg
endpoints together, then emit ordinary GH2 local rotations/limited translations
under the actual target graph.

## 2026-08-17 continuation: GLB-constrained target-length IK

Added diagnostic:

- `matrix-torso-targetgraph-rootworld-glbik-bind`

This reuses evaluated GLB/source hip-knee-ankle positions as constraints but
does not emit the rejected raw per-joint `.pos` tracks. The staged one-clip
`guitar-main` output has `13` channels: `bone_pelvis.pos` plus ordinary local
`.quat` channels for pelvis, spine/neck/head, and both thigh/knee/ankle chains.
The leg solve preserves GH2 target thigh/shin lengths through the existing
two-bone IK path.

Focused tests now pass `24/24`. GHC accepted the one-clip override for frames
`15/30/45` (`role=guitarist0`, clip `gh3_guit_mido_a_attackl`) and the deployed
main MILO was restored afterward to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
Evidence:
`.codex/current-evidence/midori-glbik-rawstage-20260817/`.

Direct visual inspection rejects it. Frame `15` lies the body sideways across
the stage, and frames `30/45` collapse near the floor/vertical leg line instead
of matching the source back-lean and lifted-leg attack silhouette. This means
target-length leg IK alone is not enough; it inherits a bad root/pelvis frame.

Next branch: solve the root/pelvis orientation and leg plane together before
IK. GLB/evaluated positions remain valid automated bridge inputs, but the solve
must establish the target pelvis/root frame first, then fit torso and legs under
that frame. For future GHC captures in this diagnostic path, do not mount the
GH2 ISO and keep app/build processes at Idle priority.

## 2026-08-17 continuation: GLB frame-first IK candidate

Added staged diagnostic:

- `matrix-torso-targetgraph-rootworld-glbframeik-bind`

This branch keeps the `glbik` target-length two-bone solve, but fixes the root
ordering error exposed by that rejection: pelvis `.pos` now uses the
bridge/bind-height placement (`z ~= 39.43` at frames `15/30/45`) and pelvis /
torso rotations are derived from the evaluated GLB position frame before leg IK
runs. The one-clip `guitar-main` stage remains ordinary clip data: `13`
channels, `bone_pelvis.pos` plus local `.quat` channels; it does not emit raw
per-joint GLB `.pos` tracks.

Focused tests pass `25/25`. Staged evidence:
`.codex/current-evidence/midori-glbframeik-rawstage-20260817/`.
Representative decoded samples are recorded in `staging_decision.json`.

Status: staged pending visual. Next step is to build a one-clip MILO and run
direct GHC frames `15/30/45` without mounting the GH2 ISO. Keep the converter
build and app capture processes at Idle priority.

No-ISO visual gate completed from local
`gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark`; the ISO was not mounted
for game-time (`D:\GEN=False` afterward). GHC accepted the one-clip override
for frames `15/30/45` (`role=guitarist0`, clip
`gh3_guit_mido_a_attackl`) and saved all screenshots. The deployed main MILO
was restored afterward to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.

Direct visual inspection rejects it. The branch keeps pelvis translation at GH2
bind height, but the body is still rotated sideways/upside-down in GHC rather
than reading as the source back-lean/lifted-leg attack pose. Evidence:
`.codex/current-evidence/midori-glbframeik-rawstage-20260817/`.

Conclusion: root placement height is no longer the immediate failure; the
GLB-derived pelvis/root frame still has the wrong handedness/orientation.
Next branch should solve a signed-axis/handedness correction for the GLB pelvis
frame before applying torso/leg IK. Do not retune limb IK until the pelvis frame
itself reads upright/profile in direct GHC captures.

## 2026-08-17 continuation: corrected Control_Root-local pelvis diagnostics

Fixed a diagnostic dispatch bug in `tools/gh3_midori_acp_stage.py`: the early
`matrix-bridge-eval-world-axis-align-bind` branch was shadowing
`ROOTWORLD_TORSO_POLICIES`, and newly added GLB policies were not included in
the outer matrix-policy dispatch list. That made `glbframeik`, `glbaxisik`,
composition-order probes, and early raw-local probes decode to the same pelvis
quaternion family. Added a static regression test; focused tests now pass
`31/31`.

Added diagnostic policy surface:

- `matrix-torso-targetgraph-rootworld-glbevalik-bind`
- `matrix-torso-targetgraph-rootworld-glblocalik-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-bind`
- `matrix-torso-targetgraph-rootworld-glbframeik-desired-localt-bind`
- `matrix-torso-targetgraph-rootworld-glbframeik-desiredt-local-bind`
- `matrix-torso-targetgraph-rootworld-glbframeik-localt-desired-bind`

After the dispatch fix, evaluated/position-frame variants still collapse to
the prior `glbframeik` pelvis samples, proving that those formulas are genuinely
equivalent for this pelvis frame. The raw Control_Root-local bridge pose finally
produces a distinct pelvis-only family. Representative decoded samples for
`glblocalraw`:

- default/HMX transpose frame `15`: `quat_wxyz=(0.917314,-0.338224,0.192468,-0.084239)`
- default/HMX transpose frame `45`: `quat_wxyz=(0.590090,-0.715075,0.332987,-0.171992)`
- direct storage frame `15`: `quat_wxyz=(0.917314,0.338224,-0.192468,0.084239)`
- direct storage frame `45`: `quat_wxyz=(0.590090,0.715075,-0.332987,0.171992)`

Both visual captures were run from local
`gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark`; no GH2 ISO was mounted at
game time and `D:\GEN=False` afterward. The deployed main MILO was restored
after each capture to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.

Visual result: default raw-local rejects because frame `45` pitches nearly
vertical/upside-down. Direct-storage raw-local is the best branch in this
mini-sweep because it keeps frame `45` horizontal, but it still appears mirrored
relative to the source GLB reference and is not promoted as a passed pelvis
gate. Compact evidence:

- `.codex/current-evidence/midori-glblocalraw-source-pelvisonly-fixed-visual-20260817/glblocalraw_pelvisonly_contact.png`
- `.codex/current-evidence/midori-glblocalraw-direct-pelvisonly-fixed-visual-20260817/glblocalraw_direct_pelvisonly_contact.png`

Next branch: sweep fixed `180` degree post-corrections around the raw
Control_Root-local pelvis pose before applying torso/leg IK. Start with direct
storage plus local/world X/Y/Z half-turns; keep it pelvis-only until the
source-facing/head-feet orientation matches frames `15/30/45`.

## 2026-08-17 continuation: pelvis half-turn sweep and first thigh gate

Added explicit half-turn diagnostics around the raw Control_Root-local bridge
pose:

- `matrix-torso-targetgraph-rootworld-glblocalraw-localx180-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localy180-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-worldx180-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-worldy180-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-worldz180-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-alllocalz180-bind`

The ordinary `localz180` correction is intentionally pelvis-only; the
`alllocalz180` variant is separate diagnostic surface for child-local testing.
Focused tests now pass `32/32`.

Frame-45 local-GEN triage across all six direct-storage half-turns narrowed the
pelvis candidates to `localz180` and `worldz180`; X/Y variants were visibly out
of the source horizontal side-fall family. Full local-GEN captures at frames
`15/30/45` selected `localz180`: it preserves the source-facing head/feet
relationship most consistently and is the current pelvis-only pass candidate.
This is not full DLC approval; it only advances the hierarchy to thighs.
Evidence:

- `.codex/current-evidence/midori-glblocalraw-halfturn-triage-20260817/halfturn_f45_triage_contact.png`
- `.codex/current-evidence/midori-glblocalraw-halfturn-finalists-20260817/halfturn_finalists_contact.png`

First thigh rung tested `Control_Root,Bone_Pelvis,Bone_Thigh_L,Bone_Thigh_R`:

- `localz180` pelvis-only correction plus raw thigh locals rejects; the pelvis
  stays in the accepted family, but frame `45` sends legs into the wrong plane.
- `alllocalz180` applies the same local Z half-turn to pelvis and thigh locals.
  It improves the thigh plane, but frames `30/45` still do not match the source
  side-fall thigh direction closely enough to pass.

All GHC captures in this continuation used local
`gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark`; no ISO was mounted at
game time, `D:\GEN=False` afterward, and the deployed main MILO restored to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.

Next branch: keep `localz180` as the pelvis root correction, but solve thigh
local rotations under the corrected pelvis from evaluated hip-knee vectors
rather than raw bridge-local thigh quats. Do not proceed to knees until left
and right thigh directions pass frames `15/30/45`.

## 2026-08-17 continuation: thigh vector and thigh-frame rejection

Added diagnostic
`matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighvec-bind`.
It keeps the accepted pelvis-only `localz180` correction, then aims target
thighs from evaluated/mapped source hip-knee vectors under the corrected pelvis.
Focused tests passed `33/33`. Direct local-GEN capture rejects it: pelvis stays
in the accepted family, but frames `15/45` throw the legs nearly vertical.
Evidence:

- `.codex/current-evidence/midori-glblocalraw-localz180-thighvec-visual-20260817/localz180_thighvec_contact.png`

Added four thigh plane-frame composition diagnostics:

- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighframe-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighframe-desired-localt-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighframe-desiredt-local-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighframe-localt-desired-bind`

All four staged to identical thigh quaternion samples for
`gh3_guit_mido_a_attackl`, so only representative `thighframe` was visually
captured. Focused tests now pass `34/34`. Direct local-GEN capture rejects it:
the accepted pelvis/root fall family remains, but the source-like sideways leg
extension is lost at frame `15` and frames `30/45` collapse toward an
upright/dangling-leg pose. Evidence:

- `.codex/current-evidence/midori-glblocalraw-localz180-thighframe-visual-20260817/thighframe_contact.png`
- `.codex/current-evidence/midori-glblocalraw-localz180-thighframe-visual-20260817/visual_decision.json`

All captures used local `gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark`.
The GH2 ISO was not mounted or used at game time, `D:\GEN=False` afterward,
GHC was launched at Idle priority, and the deployed main MILO restored to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.

Next branch: keep pelvis-only `localz180`, but diagnose the thigh as a
pelvis-corrected local-basis conversion problem. Do not replace the thigh world
frame directly from hip-knee aim or leg-plane frames; those two routes are now
visually rejected.

## 2026-08-17 continuation: thigh local-basis under corrected pelvis

Added two signed-axis thigh local-basis diagnostics:

- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-bind`

Both keep pelvis-only `localz180`. The plain `thighbasis` branch converts
evaluated source thigh local rotation through `source_pose_to_axis_aligned_target_bind`
against the target thigh bind local. The `parentcomp` branch additionally
compensates the converted thigh basis against the pelvis half-turn, so the
child world is not dragged by the accepted root correction. Focused tests now
pass `35/35`.

Direct local-GEN capture result:

- Plain `thighbasis` rejects; it still has a vertical/wrong-plane leg failure,
  especially at frame `45`.
- `thighbasis-parentcomp` is an informative partial improvement; it keeps the
  legs in the sideways fall plane better than `thighvec`, `thighframe`, or
  plain `thighbasis`, but frames `30/45` remain too extended and non-source-like
  to pass the thigh gate.

Evidence:

- `.codex/current-evidence/midori-glblocalraw-localz180-thighbasis-visual-20260817/thighbasis_contact.png`
- `.codex/current-evidence/midori-glblocalraw-localz180-thighbasis-visual-20260817/visual_decision.json`

All captures used local `gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark`.
The GH2 ISO was not mounted or used at game time, `D:\GEN=False` afterward,
GHC was launched at Idle priority, and the deployed main MILO restored to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.

Next branch: preserve the `parentcomp` insight but damp it. Test a blended
plain-basis/parentcomp compensation sweep, or add a knee-aware diagnostic only
if the thigh directions can be judged independently from the bind lower legs.

## 2026-08-17 continuation: damped thighbasis-parentcomp sweep

Added damped compensation policies:

- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp35-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp50-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp65-bind`

These slerp between plain `thighbasis` and full `thighbasis-parentcomp` in
world space for thigh bones only. Focused tests still pass `35/35`; the
registration test now also checks the blend amount map.

Frame-45 local-GEN triage rejects all three damped variants. They do not
visually interpolate toward the useful full-parentcomp branch; instead all
three reintroduce a vertical-leg failure. Evidence:

- `.codex/current-evidence/midori-glblocalraw-localz180-thighbasis-blend-visual-20260817/thighbasis_blend_f45_contact.png`
- `.codex/current-evidence/midori-glblocalraw-localz180-thighbasis-blend-visual-20260817/visual_decision.json`

All captures used local `gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark`;
no GH2 ISO at game time, `D:\GEN=False` afterward, GHC at Idle priority, and
the deployed main MILO restored to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.

Next branch: stop damping this slerp route. Keep full `thighbasis-parentcomp`
as the only useful local-basis clue and run a knee-aware minimal diagnostic
(`Control_Root,Bone_Pelvis,Bone_Thigh_*,Bone_Knee_*`) to determine whether
bind lower legs are exaggerating the apparent thigh over-extension.

## 2026-08-17 continuation: knee-aware minimal diagnostic

Added:

- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-kneebasis-bind`

This keeps pelvis-only `localz180` and full `thighbasis-parentcomp` for thighs,
then converts `Bone_Knee_L/R` local rotations through the same signed-axis
target-bind helper instead of leaving knees as raw bridge-local rotations.
A regression test now checks that all localz180 thighbasis policies are present
in the pelvis half-turn correction map; focused tests pass `36/36`.

Staged and captured two minimal thighs+knees candidates:

- full `thighbasis-parentcomp` with raw knees, frame `45` only
- `thighbasis-parentcomp-kneebasis`, frames `15/30/45`

Result: rejected partial improvement. Knee basis conversion improves the
raw-knee lower-leg extension and gives a better frame `15` read, but frames
`30/45` still throw one leg vertically instead of matching the source bent
side-fall silhouette. Evidence:

- `.codex/current-evidence/midori-glblocalraw-localz180-kneebasis-visual-20260817/kneebasis_f45_contact.png`
- `.codex/current-evidence/midori-glblocalraw-localz180-kneebasis-visual-20260817/kneebasis_full_contact.png`
- `.codex/current-evidence/midori-glblocalraw-localz180-kneebasis-visual-20260817/visual_decision.json`

All captures used local `gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark`;
no GH2 ISO at game time, `D:\GEN=False` afterward, GHC at Idle priority, and
the deployed main MILO restored to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.

Next branch: stay under pelvis-only `localz180` plus full
`thighbasis-parentcomp`, but diagnose lower-leg handedness/chain composition.
Either add ankles to the minimal diagnostic or test side-specific knee/ankle
axis flips before moving back to broader leg IK.

## 2026-08-17 continuation: ankle-chain basis diagnostic

Added:

- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-legbasis-bind`

This keeps pelvis-only `localz180`, full `thighbasis-parentcomp` for thighs,
signed-axis target-bind knees, and extends the same local-basis conversion to
`Bone_Ankle_L/R`. Focused tests still pass `36/36`.

Frame-45 local-GEN comparison against `...kneebasis-bind` rejects the ankle
extension. It does not materially change the bad silhouette: the vertical leg
remains, while ankle basis mostly shifts foot/ankle twist. Evidence:

- `.codex/current-evidence/midori-glblocalraw-localz180-legbasis-visual-20260817/legbasis_f45_contact.png`
- `.codex/current-evidence/midori-glblocalraw-localz180-legbasis-visual-20260817/visual_decision.json`

All captures used local `gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark`;
no GH2 ISO at game time, `D:\GEN=False` afterward, GHC at Idle priority, and
the deployed main MILO restored to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.

Next branch: stop adding chain depth on this basis route. Test side-specific
lower-leg handedness/axis flips under the same pelvis-only `localz180` plus
full `thighbasis-parentcomp` root, because the remaining failure is a leg-plane
handedness/side issue rather than missing ankle depth.

## 2026-08-17 continuation: side-specific knee handedness flips

Added six side/axis local half-turn probes under pelvis-only `localz180` plus
full `thighbasis-parentcomp`:

- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-kneeflip-lx180-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-kneeflip-ly180-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-kneeflip-lz180-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-kneeflip-rx180-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-kneeflip-ry180-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-kneeflip-rz180-bind`

Each uses knee basis for both knees and applies one local 180-degree correction
to the selected knee. Focused tests still pass `36/36` and now cover the
six-entry flip spec map.

Frame-45 local-GEN triage rejects left-knee X/Y/Z and right-knee X: they
preserve the vertical-leg failure. Right-knee Y and Z are useful partial
improvements, so both were promoted to full frames `15/30/45`. Full capture
still rejects: frames `30/45` fold or cross the problem leg into a non-source
side extension instead of the source bent side-fall silhouette. Evidence:

- `.codex/current-evidence/midori-glblocalraw-localz180-kneeflip-visual-20260817/kneeflip_f45_contact.png`
- `.codex/current-evidence/midori-glblocalraw-localz180-kneeflip-visual-20260817/kneeflip_finalists_contact.png`
- `.codex/current-evidence/midori-glblocalraw-localz180-kneeflip-visual-20260817/visual_decision.json`

All captures used local `gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark`;
no GH2 ISO at game time, `D:\GEN=False` afterward, GHC at Idle priority, and
the deployed main MILO restored to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.

Next branch: keep the right-knee Y/Z result as the new handedness clue. Test a
right-leg plane/bend-axis solve, or combine right-knee handedness with source
hip-knee-ankle plane constraints instead of adding more independent local
half-turns.

## 2026-08-17 continuation: right-knee bend-axis vector solve

Added:

- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-kneebend-r-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-kneebend-ry180-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-kneebend-rz180-bind`

These keep pelvis-only `localz180` plus full `thighbasis-parentcomp`, convert
knees through the signed-axis target-bind helper, optionally seed right knee
with the useful Y/Z half-turn, then vector-aim the right shin toward the mapped
evaluated source knee-to-ankle vector. Focused tests still pass `36/36`.

Frame-45 local-GEN triage rejects all three. They remove the pure vertical
pole, but still fold/cross the right leg into a non-source side extension
rather than the source bent side-fall silhouette. Evidence:

- `.codex/current-evidence/midori-glblocalraw-localz180-kneebend-visual-20260817/kneebend_f45_contact.png`
- `.codex/current-evidence/midori-glblocalraw-localz180-kneebend-visual-20260817/visual_decision.json`

All captures used local `gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark`;
no GH2 ISO at game time, `D:\GEN=False` afterward, GHC at Idle priority, and
the deployed main MILO restored to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.

Next branch: stop single-knee corrections. Use a coupled right-leg
hip-knee-ankle plane constraint under pelvis-only `localz180` plus full
`thighbasis-parentcomp`.

## 2026-08-17 continuation: right-leg hip-knee-ankle plane solve

Added:

- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-rightlegplane-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-rightlegplane-ry180-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-rightlegplane-rz180-bind`

These keep pelvis-only `localz180` plus full `thighbasis-parentcomp`, then
replace only `Bone_Knee_R` with a hip-knee-ankle plane-frame solve derived from
the mapped source thigh/shin plane. The `ry180` and `rz180` variants seed the
right-knee local plane frame using the two useful handedness clues from the
previous knee-flip sweep. Focused tests still pass `36/36`.

Frame-45 local-GEN triage rejects all three. The base right-leg-plane policy
still shows non-source lower-body crossing/extension; the Y/Z seeded variants
worsen the fold into a high kicked leg. Evidence:

- `.codex/current-evidence/midori-glblocalraw-localz180-rightlegplane-rawstage-20260817/rightlegplane_f45_contact.png`
- `.codex/current-evidence/midori-glblocalraw-localz180-rightlegplane-rawstage-20260817/visual_decision.json`

All captures used local `gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark`;
no GH2 ISO at game time, `D:\GEN=False` afterward, GHC/build at Idle priority,
and the deployed main MILO restored to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.

Next branch: stop replacing only the right knee world frame. The useful
diagnostic should bridge the gap one level higher: compare emitted target world
joint positions after conversion against the source bridge, or use an automated
intermediate skeleton/GLB solve that still emits ordinary ACP/MILO/DLC output.

## 2026-08-17 continuation: emitted pose-position report and localz180 posepos

Added `tools/gh3_midori_pose_report.py`. It stages a policy in memory, decodes
the emitted ACP local samples, reconstructs target world joint positions through
the GH2 target graph, and compares lower-body joints against the mapped source
pose bridge. The corrected report confirms:

- pelvis under `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-bind`
  is exact after using the same bridge base-frame delta and `72/467.25` scale as
  the staging path;
- the remaining visible divergence begins below pelvis;
- older rootworld position policies are not clearly better numerically;
- combining the solved localz180 rotation family with GLB/evaluated `.pos`
  channels sharply reduces right knee/ankle target-space position error.

Evidence:

- `.codex/current-evidence/midori-pose-report-20260817/pose_report.json`
- `.codex/current-evidence/midori-pose-report-20260817/pose_report_position_policies.json`
- `.codex/current-evidence/midori-pose-report-20260817/pose_report_localz180_posepos.json`

Added diagnostic policies:

- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-posepos-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-posepos-bind`

Both emit ordinary ACP/MILO output and add synthetic GLB/evaluated local `.pos`
channels while preserving the pelvis-only localz180 rotation branch. Focused
tests still pass `36/36`.

Numeric triage is promising: both new policies reduce right knee/ankle errors
to near zero at frames `15/30/45`, with max visible-pose error around `17`
units once the known `Control_Root` offset is excluded. Direct local-GEN visual
capture at frames `15/30/45` is improved over the vertical/kicked-leg failures:
frame `45` no longer has the tall vertical-leg pose. However, the branch still
rejects visually because the body is heavily crouched/compressed and frame `30`
shows non-source-like guitar/body collision. Evidence:

- `.codex/current-evidence/midori-localz180-posepos-rawstage-20260817/localz180_posepos_f45_contact.png`
- `.codex/current-evidence/midori-localz180-posepos-rawstage-20260817/localz180_posepos_full_contact.png`
- `.codex/current-evidence/midori-localz180-posepos-rawstage-20260817/visual_decision.json`

All captures used local `gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark`;
no GH2 ISO at game time, `D:\GEN=False` afterward, GHC/build at Idle priority,
and the deployed main MILO restored to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.

Next branch: keep localz180 posepos as the first promising position bridge, but
do not raw-bake every GLB joint position. Test a restricted lower-body-only
posepos/IK solve, or an intermediate skeleton bake that preserves target bone
lengths while using mapped source joint positions as constraints.

## 2026-08-17 continuation: localz180 target-length IK rejection

Added diagnostic policies:

- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-ik-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-ik-bind`

These keep the exact pelvis-localz180 base and use mapped GLB/source leg
positions only as target-length two-bone IK constraints. They do not emit raw
per-joint `.pos` tracks. Focused tests still pass `36/36`.

Numeric report:

- `.codex/current-evidence/midori-localz180-ik-20260817/pose_report_localz180_ik.json`

Compared with `localz180-posepos`, both IK policies have worse lower-body
target-space position error. The thighbasis-parentcomp IK variant improves
right ankle error at frame `45`, but not enough to beat the posepos partial.

Frame-45 local-GEN visual triage rejects both IK policies. They avoid raw
posepos compression but reintroduce non-source leg crossing/verticality; the
thighbasis-parentcomp IK variant visibly folds the green leg upright/crossed.
Evidence:

- `.codex/current-evidence/midori-localz180-ik-20260817/localz180_ik_f45_contact.png`
- `.codex/current-evidence/midori-localz180-ik-20260817/visual_decision.json`

All captures used local `gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark`;
no GH2 ISO at game time, `D:\GEN=False` afterward, GHC/build at Idle priority,
and the deployed main MILO restored to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.

Next branch: keep `localz180-posepos` as the best partial. Do not continue pure
target-length IK under this base. Instead restrict posepos to the subset that
fixed leg placement while preserving torso/upper-body bind lengths, or use an
intermediate target-skeleton bake with length constraints before emitting
selected local translations.

## 2026-08-17 continuation: restricted posepos and sampler correction

Fixed an important diagnostic bug in `tools/gh3_midori_acp_stage.py`: synthetic
GLB pose-position channels were created for all `ROOTWORLD_GLBPOSEPOS_POLICIES`,
but the sample loop only applied `target_glb_pose_local_translation` when
`rotation_policy == matrix-torso-targetgraph-rootworld-glbposepos-bind`. The
newer `localz180-posepos` policies therefore emitted default/non-GLB
translations. Treat the prior promising `localz180-posepos` visual evidence as
a default/zero-position artifact, not true GLB posepos.

Added restricted posepos diagnostics:

- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-lowerposepos-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-distalposepos-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-lowerposepos-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-distalposepos-bind`

`lowerposepos` emits GLB/evaluated local `.pos` channels only for pelvis and
thigh/knee/ankle chains. `distalposepos` emits them only for knees/ankles.
Focused tests still pass `36/36`.

Numeric report:

- `.codex/current-evidence/midori-restricted-posepos-20260817/pose_report_restricted_posepos.json`

After the sampler fix, true full/lower GLB posepos is worse than the earlier
artifact. Distal-only posepos reduces numeric knee/ankle error compared with
rotation-only branches, but frame-45 local-GEN visual triage rejects all four
restricted policies: lowerposepos pulls the green leg downward/vertical, and
distalposepos still leaves a non-source hanging/crossed leg silhouette. Evidence:

- `.codex/current-evidence/midori-restricted-posepos-20260817/restricted_posepos_f45_contact.png`
- `.codex/current-evidence/midori-restricted-posepos-20260817/visual_decision.json`

All captures used local `gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark`;
no GH2 ISO at game time, `D:\GEN=False` afterward, GHC/build at Idle priority,
and the deployed main MILO restored to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.

Next branch: if pursuing the accidentally promising prior partial, formalize it
as an explicit controlled/default/blended position policy instead of relying on
the fixed `posepos` name. Otherwise move to an intermediate target-skeleton bake
that blends source constraints with GH2 bind/local translation limits.

## 2026-08-17 continuation: controlled zero/blended posepos rejection

Formalized the accidentally promising default-position artifact as explicit
diagnostic policies:

- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-zeroposepos-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-posepos25-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-zeroposepos-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-posepos25-bind`

`zeroposepos` emits the same synthetic full GLB `.pos` channel coverage as the
old artifact, but intentionally writes zero local positions for non-pelvis
channels while preserving the solved pelvis bridge translation. `posepos25`
blends those non-pelvis zeros 25% toward true GLB/evaluated local positions.
Focused tests still pass `36/36`.

Numeric report:

- `.codex/current-evidence/midori-controlled-posepos-20260817/pose_report_controlled_posepos.json`

Frame-45 local-GEN visual capture rejects the branch. `zeroposepos` faithfully
reproduces the earlier compressed/crouched artifact, and the 25% GLB position
blend does not rescue the silhouette. Evidence:

- `.codex/current-evidence/midori-controlled-posepos-20260817/controlled_posepos_f45_contact.png`
- `.codex/current-evidence/midori-controlled-posepos-20260817/visual_decision.json`

All captures used local `gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark`;
no GH2 ISO at game time, `D:\GEN=False` afterward, GHC/build at Idle priority,
and the deployed main MILO restored to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.

Next branch: move to an automated intermediate target-skeleton bake. A `.glb`
bridge is acceptable if it is generated automatically and ultimately emits
ordinary ACP/MILO/DLC output. The bake should fit mapped source joint
constraints to GH2-length/local-translation limits before ordinary conversion.

## 2026-08-17 continuation: first target-bakepos numeric rejection

Added first-pass automated target-skeleton bake diagnostics:

- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-targetbakepos-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-targetbakepos-altlocal-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetbakepos-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetbakepos-altlocal-bind`

These policies emit synthetic lower-body `.pos` channels for pelvis, thighs,
knees, and ankles. Pelvis remains on the solved bridge translation. Thighs use
the mapped source bridge hip positions. Knees/ankles use the existing GH2-length
two-bone solve, then bake the solved world offsets back into local translations.
The `altlocal` variants test the opposite parent-rotation convention. Focused
tests now pass `37/37`.

Numeric report:

- `.codex/current-evidence/midori-target-bakepos-20260817/pose_report_target_bakepos.json`
- `.codex/current-evidence/midori-target-bakepos-20260817/numeric_decision.json`

Result: numeric rejection, no visual capture. The best row improves only frame
`15`; frames `30/45` remain worse than the controlled zero/default artifact,
with large knee/ankle errors and unstable shin direction. This proves that simply
injecting solved lower-body local `.pos` channels into the existing GH2 target
hierarchy is not enough.

Next branch: generate a true intermediate constrained skeleton outside the
existing target hierarchy, with `.glb` acceptable as an automatically generated
bridge. The solved skeleton should satisfy GH2-length constraints before ordinary
ACP/MILO emission instead of trying to patch only selected local `.pos` channels.

## 2026-08-17 continuation: target-skelrot numeric rejection

Added constrained target-skeleton rotation diagnostics:

- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-targetskelrot-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-targetskelrot-bakepos-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetskelrot-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetskelrot-bakepos-bind`

These aim thigh/knee rotations at the GH2-length solved target joints from the
target-bake skeleton. The `bakepos` variants also emit the solved
pelvis/thigh/knee/ankle local `.pos` channels, so rotations and lower-body
translations are driven by the same constrained target points. Focused tests
pass `38/38`.

Numeric report:

- `.codex/current-evidence/midori-target-skelrot-20260817/pose_report_target_skelrot.json`
- `.codex/current-evidence/midori-target-skelrot-20260817/numeric_decision.json`

Result: numeric rejection, no visual capture. The combined thighbasis variant
improves isolated right-leg rows, but the full frame set remains worse than the
already rejected controlled zero/default artifact, especially frame `45` and
shin-direction stability.

Next branch: stop patching the existing GH2 target hierarchy with selected
local rotations/translations. Generate an explicit intermediate constrained
skeleton/GLB whose solved joint transforms are the source-of-truth animation,
then emit ordinary ACP/MILO from that skeleton.

## 2026-08-17 continuation: direct solved-skeleton visual rejection

Added direct solved-skeleton policies:

- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-targetsolveskel-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-targetsolveskel-bakepos-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-bakepos-bind`

These derive thigh/knee world frames directly from the solved hip/knee/ankle
target skeleton, then localize those frames against the solved skeleton rather
than applying a correction to an inherited base rotation. Also fixed a bug in the
target-skeleton bake: thigh targets now use pelvis-relative source bridge
positions, matching the verifier and solved pelvis, instead of absolute GLB
positions. Focused tests pass `39/39`.

Numeric/visual evidence:

- `.codex/current-evidence/midori-target-solveskel-20260817/pose_report_target_solveskel.json`
- `.codex/current-evidence/midori-target-solveskel-20260817/target_solveskel_f45_contact.png`
- `.codex/current-evidence/midori-target-solveskel-20260817/visual_decision.json`

Result: visual rejection. The best corrected candidate,
`...thighbasis-parentcomp-targetsolveskel-bakepos-bind`, loads and renders from
local `GEN`, but frame `45` curls the lower body into large non-source arcs
across yaw views rather than matching the GH3 side-fall silhouette. The deployed
main MILO was restored to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`, and
`D:\GEN=False` afterward.

Next branch: build the intermediate skeleton as a separate generated source
asset (JSON and, if useful, automated `.glb`) so its local joint axes and
rest-frame basis can be inspected/exported before ACP/MILO emission. The problem
is now less about selecting another in-place policy and more about making the
generated skeleton's local axes sane before conversion.

2026-08-17 continuation: added an automated target-solved skeleton diagnostic
export to `tools/gh3_midori_acp_stage.py`:

- `--target-skeleton-diagnostic-frames`
- `--target-skeleton-diagnostic-bones`
- `--target-skeleton-diagnostic-json`

This generated
`.codex/current-evidence/midori-target-skeleton-diagnostic-20260817/target_skeleton_diagnostic.json`
for the rejected `...thighbasis-parentcomp-targetsolveskel-bakepos-bind`
candidate at frames `0/15/30/45`. The diagnostic proved the solved thigh/knee
rotations were already aiming exactly at their generated child vectors, but the
synthetic `.pos` channels for knees/ankles were localized against the old base
parent rotation instead of the solved parent rotation. That made the generated
skeleton coherent mathematically, then scrambled it when GHC reconstructed child
positions under the emitted solved parent.

Fixed `target_skeleton_bake_local_translation()` so
`ROOTWORLD_TARGET_SOLVED_SKELETON_POLICIES` localize baked `.pos` channels
through `target_solved_skeleton_world_rotation(parent, frame)`. Regenerated the
diagnostic: knees/ankles now reconstruct to the generated world positions with
max error about `0.000005`, and thigh/knee child aim is exact to about
`0.000001` degrees. Focused tests pass `40/40`. Numeric pose report after the
fix:

- `.codex/current-evidence/midori-target-skeleton-diagnostic-20260817/pose_report_target_solveskel_parentfix.json`

Frame `45` right ankle error improved materially (`~28.28` before to `5.21`
after); frames `15/30` are still mixed. Direct GHC visual approval is still
pending. Next best action: build/capture this corrected one-clip candidate from
local `gh2_ps2_hybrid_assets/GEN` only, at Idle priority, then decide from the
actual f45 silhouette.

Follow-up direct capture was run from local
`gh2_ps2_hybrid_assets/GEN` with `GHOGX_ADDONS_DIR=gh2_ps2_hybrid_assets/DLC`,
`--diagnostic-character-variant gh3_midori_1`, and
`GHOGX_DIAGNOSTIC_PERFORMER_CLIP=guitarist0=gh3_guit_mido_a_attackl`.
The temporary one-clip main MILO was swapped into the loose DLC, captured at
frame `45` / clip time `0.75s` for yaws `0/90/180/270`, and the deployed main
MILO was restored to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`; `D:\GEN`
remained false. Evidence:

- `.codex/current-evidence/midori-target-skeleton-diagnostic-20260817/parentfix_f45_midori_contact.png`
- `.codex/current-evidence/midori-target-skeleton-diagnostic-20260817/parentfix_visual_decision.json`

Visual decision: not approved, but materially improved. The catastrophic
targetsolveskel lower-body curl is gone and the pose is back in the intended
horizontal side-fall family. Remaining problem: the visible lower body is still
too straight/overextended versus the retained source GLB f45 silhouette, and
frames `15/30` remain numerically mixed and visually unchecked. Continue from
the corrected parent-frame localization; next solve/verify the remaining leg
bend, ankle, and toe basis before running the full `15/30/45` visual gate.

Follow-up numeric branch added three targetsolveskel variants under the same
corrected parent-frame localization:

- `...targetsolveskel-scaled-bakepos-bind`
- `...targetsolveskel-bendout45-bakepos-bind`
- `...targetsolveskel-scaled-bendout45-bakepos-bind`

Focused tests still pass `40/40`. Numeric report:
`.codex/current-evidence/midori-target-skeleton-diagnostic-20260817/pose_report_target_solveskel_bendvariants.json`;
decision:
`.codex/current-evidence/midori-target-skeleton-diagnostic-20260817/bendvariants_numeric_decision.json`.
Result is numeric rejection without visual capture: `bendout45` materially
improves frames `15/30` (`max_pose` about `18.78/13.31` versus baseline
`29.02/25.42`) but regresses frame `45` (`right_ankle` about `25.77` versus
baseline `5.21`). Scaled variants do not dominate. Next action: derive a
frame/source-driven bend plane or generate an automated intermediate constrained
skeleton/GLB as the source-of-truth, then emit ordinary ACP/MILO/DLC from that.
The remaining failure is lower-leg bend/ankle basis across the clip, not
pelvis/root hierarchy or GHC reconstruction.

Source-driven bend-gate continuation added two more corrected parent-frame
targetsolveskel policies:

- `...targetsolveskel-bridgescale-bakepos-bind`
- `...targetsolveskel-sourcebendgate-bakepos-bind`

`bridgescale` is a negative control: it is numerically identical to the
parent-frame baseline because the generated ankle endpoint is already clamped by
the GH2 leg length limit. `sourcebendgate` gates the existing outward-45 bend
from mapped source thigh/shin fold: folded source legs blend toward the outward
bend; straighter legs fade back to the baseline solved skeleton. Focused tests
still pass `40/40`. Evidence:

- `.codex/current-evidence/midori-target-skeleton-diagnostic-20260817/pose_report_target_solveskel_sourcebendgate.json`
- `.codex/current-evidence/midori-target-skeleton-diagnostic-20260817/sourcebendgate_numeric_decision.json`

Numeric result promotes `sourcebendgate` to direct visual capture: it preserves
the corrected baseline at frame `45` (`max_pose 17.33`, right ankle `5.21`) and
improves frames `15/30` to `max_pose 13.92/13.31` versus baseline
`29.02/25.42`. Next step: build a one-clip `guitar-main` MILO for
`sourcebendgate`, swap it into the loose Midori DLC, and capture frames
`15/30/45` from local `gh2_ps2_hybrid_assets/GEN` only. Keep converter/GHC
processes at Idle priority and do not mount the GH2 ISO.

Direct local-GEN smoke captures continued from this point. `sourcebendgate`
builds, loads, and restores correctly, but f45/yaw0 is visually the same
overextended family as the parent-frame baseline. Added
`...sourcebendgate-ankletoe-bakepos-bind` to aim ankle quats at mapped source
toe vectors; diagnostics prove zero-degree ankle-to-toe aim at frames
`15/30/45`, but the f45 render is pixel-identical to sourcebendgate, so ankle
twist is not the visible blocker.

Added `...sourcebendgate-leftlate-bakepos-bind`, which keeps sourcebendgate for
frames `15/30` and applies the useful left-leg `bendout45` only when both source
legs are in the late straightened/fall state. Numeric report:
`.codex/current-evidence/midori-ankletoe-visual-20260817/pose_report_target_solveskel_leftlate.json`.
It improves f45 `max_pose` from `17.33` to `13.31` while preserving the good
right ankle (`5.21`). Direct visual is not yet approved: yaw0 is still
pixel-identical to sourcebendgate, and the attempted yaw90 smoke was also
identical because the current `--diagnostic-front-camera` command overrides
`GHOGX_DEBUG_GAMEPLAY_CAMERA_YAW`. Evidence/decision:
`.codex/current-evidence/midori-leftlate-visual-20260817/visual_decision.json`.
Next action: restore the earlier proven yaw-sweep capture harness or use a
direct character/clip viewer that can force clip frame and yaw, then visually
compare `leftlate` at f45 from side/back views before promoting or rejecting it.
All runtime captures used local `gh2_ps2_hybrid_assets/GEN`; `D:\GEN=False`;
the deployed main MILO was restored to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.

Follow-up direct visual used the gameplay debug camera instead of
`--diagnostic-front-camera`: `GHOGX_DEBUG_GAMEPLAY_CAMERA=1`,
`GHOGX_DEBUG_GAMEPLAY_CAMERA_TARGET=guitarist0:bone_spine1.mesh`, and explicit
yaw values `0` and `1.57079632679`. This validates a local-GEN yaw harness
without mounting or reading an ISO at game time. The `leftlate` one-clip MILO
loaded from loose DLC and the clip override was accepted, but direct yaw90
visual rejects it: the torso/root collapses under the guitar while the lower
body towers above, so the remaining blocker is Control_Root/pelvis/root-motion
placement/orientation rather than ankle/toe twist or late left-leg bending.
Evidence:
`.codex/current-evidence/midori-leftlate-yawdebug-20260817/visual_decision.json`.
Keep the no-front-camera debug-yaw harness for future visual checks from local
`gh2_ps2_hybrid_assets/GEN` only.

Important correction from the follow-up rootdiag/direct-storage pass: the
default visual candidate above used `gh3_midori_acp_stage.py`'s default HMX
storage mode, while the promising numeric `leftlate` report had used
`--hmx-quat-mode direct`. Runtime `GHOGX_DEBUG_CHARBONE_OUTPUT_MAP` rows for
the default-storage candidate prove the loose-DLC clip loaded and published
live target rows; the failure is generated pose data, not a post-publication
runtime gap. Rebuilding the same one-clip candidate with
`--hmx-quat-mode direct` changes the f45 side view materially toward the source
horizontal attack family. It is still not approved: yaw0 remains tangled and
occluded, so it needs the full direct-storage f15/f30/f45 yaw contact sheet
before promotion. Evidence:
`.codex/current-evidence/midori-leftlate-rootdiag-20260817/rootdiag_summary.json`
and
`.codex/current-evidence/midori-leftlate-direct-visual-20260817/visual_decision.json`.
Next action: run the direct-storage `leftlate` f15/f30/f45 contact sheet using
the validated no-front-camera debug-yaw harness and local
`gh2_ps2_hybrid_assets/GEN` only.

Direct-storage contact sheet completed for `leftlate` at frames `15/30/45` and
yaws `0/90`. Each capture used local `gh2_ps2_hybrid_assets/GEN`, loaded
Midori from loose DLC, accepted the `gh3_guit_mido_a_attackl` diagnostic clip
override, and restored the deployed main MILO afterward. Direct HMX storage is
still the right convention for this branch and is materially better than the
default-storage visual, but the contact sheet rejects promotion: front views
remain tangled/occluded, and side views still swing the body/legs through
non-source relationships, especially frames `30/45`. Evidence:
`.codex/current-evidence/midori-leftlate-direct-sheet-20260817/visual_decision.json`
and
`.codex/current-evidence/midori-leftlate-direct-sheet-20260817/leftlate_direct_contact_sheet.png`.
Next action: keep `--hmx-quat-mode direct`, but return to Control_Root/pelvis
frame diagnosis. Solve a pelvis/root frame that preserves the source sideways
airborne silhouette across frames `15/30/45` before retuning thigh/knee bends.

Direct root-frame numeric diagnosis resumed at the pelvis/Control_Root branch.
The in-memory pose report now supports `--source-bones`, so the same ACP
decode/reconstruct pass can include torso markers instead of only lower body.
With `--hmx-quat-mode direct`, the plain `sourcebendgate-leftlate` policy still
shows an invariant `Control_Root` row error of `39.417502`, while prior
`facingpos` evidence says synthetic `bone_facing.pos` is not consumed by the
visible guitar-main pose. The important new finding is above the pelvis:
expanding the report to `Bone_Stomach_Lower`, `Bone_Stomach_Upper`,
`Bone_Chest`, `Bone_Neck`, and `Bone_Head` shows plain `leftlate` has
neck/chest/stomach drift around `7-18` target units. Added diagnostic policy
`matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcebendgate-leftlate-torsopos-bakepos-bind`.
It keeps the direct `leftlate` rotation/lower-body branch but also emits
torso bake-position channels from the GLB bridge. Numeric result: torso rows
collapse near zero error while the remaining non-root error is back to the
known knee/ankle profile (`max_pose` about `13.3`). Focused tests pass `40/40`.
Evidence:
`.codex/current-evidence/midori-direct-rootframe-diagnostic-20260817/numeric_decision.json`
and
`.codex/current-evidence/midori-direct-rootframe-diagnostic-20260817/pose_report_direct_leftlate_torsopos.json`.
Next action: build a one-clip direct-storage `torsopos` candidate and run the
validated no-front-camera f15/f30/f45 yaw visual gate from local
`gh2_ps2_hybrid_assets/GEN` only, with Idle priority.

Direct-storage `torsopos` visual gate completed as a one-clip loose-DLC swap.
The temporary main MILO SHA was
`462A84D34DB01AEAF2A65CE7A457F418FBC1E3AEA5FCD99F75E068858C311432`; the
deployed loose-DLC main MILO was restored afterward to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`. All six
captures used local `gh2_ps2_hybrid_assets/GEN`, no ISO mount/runtime access,
`GHOGX_ADDONS_DIR=gh2_ps2_hybrid_assets/DLC`, the validated no-front-camera
debug-yaw harness, and the `gh3_guit_mido_a_attackl` diagnostic clip override.
Visual result is rejected: although `torsopos` fixed the expanded torso rows
numerically, the rendered contact sheet still has tangled/occluded front views
and side views that miss the clean source sideways airborne silhouette across
frames `15/30/45`, especially frames `30/45`. Evidence:
`.codex/current-evidence/midori-torsopos-direct-visual-20260817/visual_decision.json`
and
`.codex/current-evidence/midori-torsopos-direct-visual-20260817/torsopos_direct_contact_sheet.png`.
Next action: keep `torsopos` as a partial/negative result; continue with the
remaining root-space/hierarchy mismatch plus knee/ankle errors. Do not retry
synthetic `bone_facing.pos`, since prior evidence proved it is not consumed by
the visible guitar-main pose.

Added exact bridge-position diagnostic:
`matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-bakepos-bind`.
Unlike `torsopos`, this bypasses the GH2-length IK knee/ankle clamp and bakes
mapped source bridge positions for pelvis, torso, knees, and ankles while still
using the direct/localz180 target-skeleton rotation family. Numeric result is
the strongest so far: expanded direct pose-report non-root `max_pose` falls to
`0.054419/0.087491/0.107007` at frames `15/30/45`, with only the known
unconsumed `Control_Root` diagnostic row remaining at `39.417502`. Focused
tests pass `40/40`. Evidence:
`.codex/current-evidence/midori-sourcepos-direct-diagnostic-20260817/numeric_decision.json`.

Direct-storage one-clip `sourcepos` visual gate also completed from local
`gh2_ps2_hybrid_assets/GEN` only, using the validated no-front-camera debug-yaw
harness. Temporary candidate main MILO SHA:
`E405C0D19371285A339960EEAF7AA98C9FFAD1C2ED34E571B51E017C918C6839`; deployed
main MILO restored to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`. All six
captures exited zero and proved loose-DLC load, Midori variant selection,
`gh3_guit_mido_a_attackl` override, and screenshot save. Visual result is still
rejected: exact position baking improves the offline endpoint data, but the
rendered front views remain tangled/occluded and side views still miss the
source sideways airborne silhouette across frames `15/30/45`. Evidence:
`.codex/current-evidence/midori-sourcepos-direct-visual-20260817/visual_decision.json`
and
`.codex/current-evidence/midori-sourcepos-direct-visual-20260817/sourcepos_direct_contact_sheet.png`.
Next action: run a focused runtime/offline output-map comparison for the
`sourcepos` candidate. The next question is whether GHC reconstructs the baked
`.pos` rows as the offline pose report predicts; if yes, continue with
root/rotation frame composition rather than more position baking.

Focused `sourcepos` rootdiag resumed with low-priority processes and local
`gh2_ps2_hybrid_assets/GEN` only. A first rerun accidentally used an older temp
`milo_convert_tool` from before the fallback `normal` `CharClipGroup` fix, so
the one-clip MILO contained `gh3_guit_mido_a_attackl` but GHC reported the
diagnostic clip override as missing. Rebuilt the converter from current
`GuitarHeroOGX-main-ui-engine/tools/milo_convert` into `%TEMP%` at Idle
priority; the rebuilt candidate exactly matched the prior accepted SHA
`E405C0D19371285A339960EEAF7AA98C9FFAD1C2ED34E571B51E017C918C6839` and
inspected with `group normal clips=gh3_guit_mido_a_attackl`.

The valid one-frame rootdiag loaded loose DLC, accepted
`GHOGX_DIAGNOSTIC_PERFORMER_CLIP=guitarist0=gh3_guit_mido_a_attackl`, saved the
screenshot, and restored the deployed main MILO to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`; `D:\GEN`
remained absent. Result: the candidate MILO does contain the intended sourcepos
rows at sample `45` (`bone_pelvis.pos=(0.038344,0.047077,39.4317)`,
`bone_L-knee.pos≈-0.051856`, `bone_L-ankle.pos≈-0.068597`), but runtime
`GHOGX_DEBUG_CHARBONE_OUTPUT_MAP` for Midori stayed on the startup sample
family across the capture (`bone_pelvis` `outLocal/meshLocal=(0,0,39.406)`,
`bone_L-knee≈-13.264`, `bone_L-ankle≈-17.561`) despite
`GHOGX_DIAGNOSTIC_PERFORMER_CLIP_TIME=guitarist0=0.75`. Evidence:
`.codex/current-evidence/midori-sourcepos-rootdiag-20260817/rootdiag_summary.json`.
Next action: fix/validate forced clip-time sampling, or build a frozen
one-sample f45 diagnostic alias, then rerun output-map to test whether sourcepos
rows are consumed at the intended frame.

Added a diagnostic `--freeze-sample` option to `gh3_midori_acp_stage.py` and
used it to build a one-sample f45 `sourcepos` main MILO. This bridge avoids the
active-player time/seek ambiguity entirely: sample `0` in the generated MILO is
the prior sourcepos f45 pose. The candidate SHA was
`E6BD8D5B0D44EAD603A0606F8B72CB4635EA1D4DA7D51E4845E6D817986E2390` and
inspected as `1 frames`, `group normal clips=gh3_guit_mido_a_attackl`.

Runtime proof from local `gh2_ps2_hybrid_assets/GEN` only: GHC accepted the
diagnostic clip override and `GHOGX_DEBUG_CHARBONE_OUTPUT_MAP` published the
f45 sourcepos rows exactly for Midori: pelvis
`outLocal/meshLocal=(0.038,0.047,39.432)`, left knee `(-0.052,0,0)`, left ankle
`(-0.069,0,0)`, and left thigh `(-2.192,-14.673,-8.792)`. The deployed loose
DLC main was restored afterward to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`; `D:\GEN`
remained absent and no ISO was used at game time. Evidence:
`.codex/current-evidence/midori-sourcepos-timefix-20260817/freeze45_decision.json`
and
`.codex/current-evidence/midori-sourcepos-timefix-20260817/freeze45_yaw90_rootdiag.png`.

Decision: stop pursuing additional position baking. Runtime consumes sourcepos
`.pos` rows when the sampled frame is unambiguous, but the frozen f45 side view
is still visually rejected: Midori is curled/folded near the highway/floor
rather than preserving the source airborne sideways silhouette. The next
diagnosis should use the frozen f45 harness to derive the missing
pelvis/Control_Root/root rotation composition, or compare runtime bone axes
against the source bridge and target bind.

## 2026-08-17 continuation: sourcepos root-frame matrix-order sweep

Added three diagnostic sourcepos variants that keep the proven frozen f45
sourcepos `.pos` rows, but solve `Bone_Pelvis`/torso frames from mapped source
child positions and vary the solved-frame composition order:

- `...targetsolveskel-sourcepos-desired-localt-bakepos-bind`
- `...targetsolveskel-sourcepos-desiredt-local-bakepos-bind`
- `...targetsolveskel-sourcepos-localt-desired-bakepos-bind`

The original `...targetsolveskel-sourcepos-bakepos-bind` remains unchanged and
still rebuilds to the prior frozen f45 SHA
`E6BD8D5B0D44EAD603A0606F8B72CB4635EA1D4DA7D51E4845E6D817986E2390`.
Focused tests pass `40/40`.

Direct local-GEN captures were run at idle priority for the three new variants
with `--ark-dir gh2_ps2_hybrid_assets/GEN`; no ISO was used, `D:\GEN` remained
absent, and the deployed Midori main restored to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
All three variants reject visually. `desiredt-local` is the best of the sweep
because it rotates out of the old curled default family, but it still folds into
the highway/guitar instead of matching the clean source airborne sideways
silhouette. Evidence:
`.codex/current-evidence/midori-sourcepos-rootrot-sweep-20260817/rootrot_decision.json`
and
`.codex/current-evidence/midori-sourcepos-rootrot-sweep-20260817/sourcepos_rootrot_f45_contact.png`.

Decision: matrix multiplication order alone is not the missing
Control_Root/pelvis contract. Continue by deriving a different pelvis basis/root
frame from evaluated `Control_Root` + `Bone_Pelvis` axes, or by using an
automated GLB bridge to compare the source and target root frames before
ordinary MILO emission.

## 2026-08-17 continuation: sourcepos Control_Root fold diagnostics

Added four diagnostic siblings under the frozen sourcepos branch:

- `...targetsolveskel-sourcepos-foldroot-bakepos-bind`
- `...targetsolveskel-sourcepos-rootfold-bakepos-bind`
- `...targetsolveskel-sourcepos-invfoldroot-bakepos-bind`
- `...targetsolveskel-sourcepos-rootinvfold-bakepos-bind`

These keep the proven f45 sourcepos position bake, but fold either the
axis-aligned evaluated `Control_Root` world frame, or its inverse, into the
rootless GH2 `bone_pelvis` channel. Focused tests pass `40/40`.

Numeric triage selected `foldroot` over `rootfold` (`foldroot` f45 non-root
`max_pose=0.052736`; `rootfold` f45 `max_pose=0.204321`). A direct local-GEN
f45 capture then rejected `foldroot`: the pose collapses low across the highway
with limbs/guitar spread instead of preserving the source airborne side-fall
silhouette. The inverse variants were not captured because pose-report triage
was worse than `foldroot` (`invfoldroot` f45 `max_pose=0.178812`; `rootinvfold`
f45 `max_pose=0.112771` and right shin dot only `0.570603`). The capture used
`--ark-dir gh2_ps2_hybrid_assets/GEN`, no ISO, idle priority, and restored the
deployed main to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`; `D:\GEN`
remained absent. Evidence:
`.codex/current-evidence/midori-sourcepos-foldroot-20260817/foldroot_decision.json`
and
`.codex/current-evidence/midori-sourcepos-foldroot-20260817/sourcepos-foldroot_yaw90_f45.png`.

Decision: neither adding nor inverse-stripping the whole aligned
`Control_Root` world frame solves the pelvis/root contract. Next branch should
compare source GLB/evaluated and GH2 target root axes directly and derive a
new pelvis basis, rather than composing the full `Control_Root` frame into
`bone_pelvis`.

## 2026-08-17 sourcepos Control_Root yaw-only diagnostics

Added diagnostic policies:

- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-yawfold-bakepos-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-rootyawfold-bakepos-bind`

These keep the sourcepos/target-skeleton solve path but extract only the
axis-aligned `Control_Root` twist around target root local Z and fold that yaw
delta into rootless GH2 `bone_pelvis`. This intentionally avoids the rejected
full-root pitch/roll fold.

Focused tests pass `40/40`. In-memory pose-report triage at frames `15/30/45`
rejects `rootyawfold` numerically (`max_pose` up to `0.128624`, right shin dot
as low as `0.038974`, left shin dot as low as `0.276683`). `yawfold` is mixed:
it improves frame 15 `max_pose` over sourcepos baseline (`0.050450` versus
`0.054419`) and keeps strong right-shin agreement at frame 45 (`0.907925`),
but does not beat the already visually rejected full `foldroot` on frame 45
`max_pose`. No game runtime/capture was launched, no ISO was used, and
`D:\GEN` stayed absent.

Evidence:
`.codex/current-evidence/midori-sourcepos-yawroot-20260817/yawroot_decision.json`.

Next branch: if spending a visual pass, capture only `yawfold` frozen f45 from
local `gh2_ps2_hybrid_assets/GEN` at idle priority. Otherwise continue toward
a GLB/evaluated bridge comparison that solves the pelvis basis without folding
any root pitch/roll into `bone_pelvis`.

## 2026-08-17 sourcepos pelvis-basis diagnostics

Added diagnostic policies:

- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-axislocal-bakepos-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-axisblend35-bakepos-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-axisblend50-bakepos-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-axisblend65-bakepos-bind`

These keep the sourcepos target-skeleton solve and source-position bake, but
replace rootless GH2 `bone_pelvis` rotation with the bridge-evaluated
axis-aligned local pelvis basis, or blend from the prior `yawfold` pelvis
toward that basis. This tests whether the old visually safe constant pelvis
axis family can be merged into the sourcepos path.

Focused tests pass `40/40`. In-memory pose reports reject the branch
numerically: pure `axislocal` improves frame 45 `max_pose` (`0.052164`) but
catastrophically flips frame 15 left shin (`-0.993090`); `axisblend35` still
worsens frame 15 (`max_pose=0.097295`, right shin dot `0.061172`), and stronger
blends degrade further. No game runtime/capture was launched, no ISO was used,
and `D:\GEN` stayed absent.

Evidence:
`.codex/current-evidence/midori-pelvisbasis-20260817/pelvisbasis_decision.json`.

Next branch: do not visually capture `axislocal`/`axisblend`. Either capture
prior `yawfold` frozen f45 from local `gh2_ps2_hybrid_assets/GEN` at idle
priority, or move to a real automated GLB/constraint bake that solves pelvis
and legs together instead of changing pelvis alone.

## 2026-08-17 pelvis correction probe

Reused the existing ACP staging/reconstruction path to compare `yawfold`
pelvis world rotation against a position-derived pelvis frame from the
evaluated GLB/source bridge at frames `15/30/45`. This tests whether the
remaining mismatch can be fixed by a single constant pelvis correction.

Result: constant correction is numerically rejected. The yawfold-to-position
frame correction changes by `61.985417` degrees from f15/f30, `117.258086`
degrees from f15/f45, and `61.890271` degrees from f30/f45. The older
`matrix-leg-constraint-bridge-pelvisgate-bind` policy was also rechecked and is
not sourcepos-aligned (`max_pose` around `55-57`), so it is not useful for the
current sourcepos pelvis/root branch. No game runtime/capture was launched, no
ISO was used, and `D:\GEN` stayed absent.

Evidence:
`.codex/current-evidence/midori-constraint-bridge-20260817/pelvis_correction_decision.json`.

Next branch: stop spending time on constant pelvis-only matrix variants. Either
capture prior `yawfold` frozen f45 from local `gh2_ps2_hybrid_assets/GEN` at
idle priority, or implement a true frame-wise automated GLB/constraint bake
that solves pelvis and legs as one chain.

## 2026-08-17 yawfold frozen f45 capture attempt

Built the current `milo_convert_tool` in `%TEMP%` at Idle priority, staged a
one-sample f45
`matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-yawfold-bakepos-bind`
guitar-main ACP, and converted it to a one-clip `gh3_midori_main.milo_ps2`
(SHA `AC9CA4EC4D4201C792084972D6240C5381025FC22113393DAF19FEA3E516E299`,
1,610 bytes).

Temporarily swapped only the loose-DLC main MILO, launched GHC from local
`gh2_ps2_hybrid_assets/GEN` with the validated no-front-camera debug yaw90
harness, and kept `ghogx_app` at Idle priority. The deployed main was restored
to `D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`.
No ISO was used and `D:\GEN` stayed absent.

Result: incomplete visual attempt, not a pass or reject. Under the 180-second
Idle cap no screenshot was saved, though logs proved local DLC catalog load,
Midori variant selection, and local GEN runtime. Cleanup removed the temp
converter build and staged ACP trees; retained only the 1,610-byte candidate
MILO and compact JSON.

Evidence:
`.codex/current-evidence/midori-yawfold-visual-20260817/yawfold_capture_attempt.json`.

Next branch: either rerun the same yawfold capture with a longer timeout or
reduce load cost/warm the harness; otherwise proceed to the frame-wise
automated GLB/constraint bake.

Result update: the longer local-GEN yawfold f45 capture eventually produced
`yawfold_f45_yaw90.png` with `ghogx_app` at Idle priority and `D:\GEN=False`.
Direct visual inspection rejects yawfold: Midori still folds/contorts around
the pelvis, legs, torso, and guitar. The deployed main MILO was restored to
`D4C4A43B55AFD8FC39025779956C05BE41DC04EF39FAD8924790D9068D89ADFA`, and no
GHOGX/PCSX2 process remains running. Evidence:
`.codex/current-evidence/midori-yawfold-visual-20260817/yawfold_visual_decision.json`.

Next branch: stop pelvis-only/yaw-only variants. Proceed to an automated
frame-wise GLB/constraint bake or equivalent matrix-local hierarchy solve that
treats pelvis and legs as one chain; an intermediate `.glb` is acceptable as
long as the final pipeline is automated.

## 2026-08-17 explicit intermediate skeleton export

Extended `tools/gh3_midori_pose_report.py` with `--skeleton-output`. The report
tool can now materialize staged ACP output as a separate intermediate
target-skeleton JSON with per-frame local/world rotations and translations.
Added `tools/gh3_midori_intermediate_skeleton_glb.py`, which converts that JSON
into an automated visible GLB line-skeleton diagnostic. This turns the hidden
policy result into an automated bridge asset that can feed visual inspection or
a corrected matrix-local hierarchy solve.

Ran the `glbframeik` compose family and `glbaxisik` at frames 15/30/45 using
the existing torso pose bridge, with the child Python process set to Idle
priority. No game runtime was launched, no ISO was used, and `D:\GEN=False`.
Focused tests pass `41/41`.

Result: numeric reject, keep goal open. The best row is plain `glbframeik` at
frame 30 with `max_pose_error=18.017467`, right-shin dot `0.607422`, and
left-shin dot `0.810763`; the worst row is `glbframeik-localt-desired` at
frame 15 with `max_pose_error=27.951382`.

Exported
`.codex/current-evidence/midori-glbframeik-20260817/intermediate_skeleton_glbframeik.glb`
for the plain `glbframeik` policy. It is 1,640 bytes with 24 vertices, 18 line
edges, and frames 15/30/45 laid out side-by-side; GLB header validation confirms
magic `glTF`, version 2, and declared length matching file size.

Evidence:
`.codex/current-evidence/midori-glbframeik-20260817/glbframeik_decision.json`,
`.codex/current-evidence/midori-glbframeik-20260817/pose_report_glbframeik.json`,
and
`.codex/current-evidence/midori-glbframeik-20260817/intermediate_skeleton_glbframeik.json`,
plus
`.codex/current-evidence/midori-glbframeik-20260817/intermediate_skeleton_glbframeik.glb`.

Next branch: inspect/consume the intermediate skeleton GLB/JSON to identify the
wrong matrix-local basis, then use that visible hierarchy to fix the pelvis/leg
solve rather than adding more pelvis-only policies.

## 2026-08-17 frame-IK bake-position diagnosis

Added three targeted policies:

- `matrix-torso-targetgraph-rootworld-glbframeik-bakepos-bind`
- `matrix-torso-targetgraph-rootworld-glbframeik-sourcepos-bakepos-bind`
- `matrix-torso-targetgraph-rootworld-glbframeik-sourcepos-bakepos-altlocal-bind`

The goal was to separate default GH2 bind-length offsets from source-position
baked lower-body offsets while keeping the frame-IK rotation path.

Result: keep goal open. Plain bakepos is rejected: best `max_pose_error` is
`13.315613`, and the segment audit still shows a `13.21` unit thigh-to-knee
length delta, so it is still effectively bind-length. Sourcepos-bakepos is the
useful narrowed branch: `max_pose_error` is `0.098296/0.130078/0.101254` at
frames 15/30/45, and segment length delta is `0.0` on the worst segments. The
remaining failure is direction/basis, not length: worst direction dots are
`-0.799569` at f15 `Rthigh_to_Rknee`, `0.161688` at f30 `Rthigh_to_Rknee`, and
`0.378556` at f45 `Lthigh_to_Lknee`. Sourcepos altlocal is rejected:
`max_pose_error=23.875582/33.346062/33.674302`.

Focused tests pass `43/43`. No game runtime was launched, no ISO was used, and
`D:\GEN=False`.

Evidence:
`.codex/current-evidence/midori-glbframeik-bakepos-20260817/glbframeik_bakepos_decision.json`.

Next branch: keep `glbframeik-sourcepos-bakepos` as the narrowed diagnostic and
fix the remaining thigh/knee direction basis in the frame-IK rotation solve
before spending local-GEN visual capture time.

## 2026-08-17 sourceposrot frame-IK correction-order sweep

Added four sourceposrot policies:

- `matrix-torso-targetgraph-rootworld-glbframeik-sourceposrot-bakepos-bind`
- `matrix-torso-targetgraph-rootworld-glbframeik-sourceposrot-pre-bakepos-bind`
- `matrix-torso-targetgraph-rootworld-glbframeik-sourceposrot-post-bakepos-bind`
- `matrix-torso-targetgraph-rootworld-glbframeik-sourceposrot-pret-bakepos-bind`

These aim frame-IK rotations at source-position segment vectors and sweep the
correction composition order. Result: reject all four. They share the same
numeric envelope (`max_pose_max=0.126262`, `max_pose_avg=0.112203`) and worsen
shin direction (`min_shin_dot=-0.344378`) compared with plain
`glbframeik-sourcepos-bakepos`.

A comparison against existing sourcepos frame-solve policies reconfirms the
best numeric branch is still
`matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-bakepos-bind`
with `max_pose_max=0.107007`, `max_pose_avg=0.082972`, and
`min_shin_dot=0.541926`. Desired/local transpose variants are rejected by
negative shin direction dots down to `-0.902602`.

Focused tests pass `43/43`. No game runtime was launched, no ISO was used, and
`D:\GEN=False`.

Evidence:
`.codex/current-evidence/midori-glbframeik-sourceposrot-20260817/sourceposrot_decision.json`.

Next branch: stop pursuing frame-IK sourceposrot order variants. Return to the
best sourcepos targetsolveskel branch and diagnose its remaining
`Control_Root`/pelvis basis before visual capture.

## 2026-08-17 sourcepos targetsolveskel pelvis/torso basis diagnosis

Added `tools/gh3_midori_rotation_basis_audit.py` and three targeted policies:

- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-pelvisframe-bakepos-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-torsoframe-bakepos-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-torsoframe-kneeaim-bakepos-bind`

The audit confirmed the baseline best numeric branch has a torso basis bug:
pelvis-to-stomach row-direction dots are
`-0.942928/-0.945080/-0.802354` at frames 15/30/45. Its lower-body summary is
still the best balanced candidate: `max_pose_max=0.107007`,
`max_pose_avg=0.082972`, `min_shin_dot=0.541926`.

`pelvisframe` and `torsoframe` fix torso aim and improve f30/f45 lower-body
pose (`max_pose_max=0.097062`, `max_pose_avg=0.077272`), but regress frame 15
right shin badly (`min_shin_dot=0.094157`). `torsoframe-kneeaim` is rejected:
`max_pose_max=0.153258`, `max_pose_avg=0.122818`, `min_shin_dot=-0.941772`.

Focused tests pass `44/44`. No game runtime was launched, no ISO was used, and
`D:\GEN=False`.

Evidence:
`.codex/current-evidence/midori-sourcepos-targetsolveskel-basis-20260817/sourcepos_targetsolveskel_basis_decision.json`.

Next branch: do not use kneeaim. Either keep baseline sourcepos for balanced
legs despite bad torso aim, or add a gated torsoframe that avoids the frame 15
right-shin regression before spending local-GEN visual capture time.

## 2026-08-17 sourcepos torsoframe right-fold gate

Added
`matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-torsoframe-rightfoldgate-bakepos-bind`.
The gate uses source right thigh/shin fold direction (`dot > 0`) to skip
torsoframe at f15 while enabling it at f30/f45.

Pose report confirms the intended blend: f15 matches baseline
(`max_pose_error=0.054419`, right-shin dot `0.834330`), while f30/f45 match
torsoframe (`max_pose_error=0.069256/0.065499`, right-shin dots
`0.800051/0.863442`). Aggregate is the best diagnostic so far:
`max_pose_max=0.069256`, `max_pose_avg=0.063058`, `min_shin_dot=0.716983`.

This is promising but not visual-ready. Rotation-basis audit still shows the
f15 baseline pelvis-to-stomach inversion (`row_direction_dot=-0.942928`), so
the remaining problem is specifically fixing f15 torso/pelvis basis without
reintroducing the right-shin collapse.

Focused tests pass `44/44`. No game runtime was launched, no ISO was used, and
diagnostics ran at low/Idle priority.

Evidence:
`.codex/current-evidence/midori-sourcepos-torsoframe-rightfoldgate-20260817/rightfoldgate_decision.json`.

Next branch: fix the f15 torso/pelvis basis without reintroducing the
right-shin collapse; do not spend local-GEN visual capture time until that
remaining basis issue is resolved or explicitly accepted for a visual check.

## 2026-08-18 f15 torsoframe right-leg aim diagnosis

Added targeted policies for the remaining f15 torso/leg conflict:

- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-torsoframe-rkneebakeaim-bakepos-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-torsoframe-rkneeaim-bakepos-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-torsoframe-rlegaim-bakepos-bind`
- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-torsoframe-pelvisblend50-bakepos-bind`

Results:

- `rkneebakeaim` is effectively a no-op against the f15 right-shin failure.
- `rkneeaim` improves f15 `max_pose_error` to `0.086690`, but right-shin dot
  remains weak at `0.183920`.
- `pelvisblend50` is rejected by negative f15 shin dots
  (`-0.545129/-0.159121`).
- `rlegaim` is the best new branch: it keeps torsoframe active at f15,
  preserves positive torso rows, raises f15 right-shin dot to `0.999384`, and
  gives aggregate `max_pose_max=0.070177`, `max_pose_avg=0.068311`,
  `min_shin_dot=0.716983`.

Caveat: the intermediate skeleton segment audit still flags the short f15
`Rthigh_to_Rknee` segment direction (`0.082772`), so `rlegaim` is a plausible
visual candidate but not approved.

Focused tests pass `44/44`. No game runtime was launched, no ISO was used, and
diagnostics ran at low/Idle priority.

Evidence:
`.codex/current-evidence/midori-sourcepos-torsoframe-rkneebakeaim-20260818/rlegaim_decision.json`.

Next branch: either use `rlegaim` for a local-GEN visual capture at low
priority, or refine the f15 right-thigh segment direction first. Do not use
`pelvisblend50` or `rkneebakeaim`.

## 2026-08-18 rlegaim local-GEN build/capture

Added automation for the current candidate:

- `tools/gh3_midori_build_pipeline.py` now accepts `--rotation-policy`, so
  candidate ACP/MILO/DLC builds do not require hand-editing the orchestrator.
- `tools/gh3_midori_acp_stage.py` now allows sourcepos targetsolveskel bakepos
  policies to use built-in source position evaluation without diagnostic
  `--source-pose-bridge-json`.
- `tools/gh3_midori_gameplay_proof.py` now accepts `--low-priority` and
  launches GHC captures with Windows below-normal priority.

Ran the rlegaim structural candidate build at Idle priority:

- Staged all `331` ACP clips with
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-torsoframe-rlegaim-bakepos-bind`
  (`guitar-main:266`, `guitar-ui:6`, `guitar-strum:23`, `guitar-fret:36`).
- Rebuilt animation/model MILOs.
- Packaged engine DLC: `midori_dlc_package_assets_verified`,
  `total_asset_bytes=26923450`.
- Deployed byte-verified loose DLC:
  `gh2_ps2_hybrid_assets/DLC/community.gh3.midori`.

Ran local-GEN gameplay proof with the Python wrapper at Idle and GHC at
below-normal priority. Runtime used `gh2_ps2_hybrid_assets/GEN` and
`gh2_ps2_hybrid_assets/DLC`; no ISO was used or mounted at game time. Gameplay
proof status is `in_song_midori_variant_animation_verified`, `failure_count=0`,
`proof_count=2`.

Screenshots for direct visual approval:

- `analysis/gh3_midori_ghc_gameplay_proofs/gh3_midori_1_variant_gameplay_guitarist0_f60.bmp`
- `analysis/gh3_midori_ghc_gameplay_proofs/gh3_midori_2_variant_gameplay_guitarist0_f60.bmp`

Aggregate verifier note: after updating the expected staged rotation policy,
`tools/gh3_midori_pipeline_verify.py` has one known failure:
`catalog_test_matches_current_package_hashes`. The stale input is
`analysis/gh3_midori_catalog_test_summary.json`, which still records the
previous animation MILO hashes and an old ISO-based base-ARK note. Do not call
the aggregate proof final until the catalog test is rerun/refreshed against the
local hybrid `GEN` tree and current rlegaim package.

Evidence:
`.codex/current-evidence/midori-rlegaim-build-candidate-20260818/rlegaim_build_capture_decision.json`.

Next gate: user direct visual approval of the two screenshots. If rejected,
continue from rlegaim visual evidence and the f15 `Rthigh_to_Rknee` segment
direction warning.

## 2026-08-18 catalog refresh for rlegaim

Closed the stale aggregate-verifier gap from the rlegaim rebuild.

The Ninja preset was unavailable in this shell, so configured a separate Visual
Studio 2022 engine build tree at low/Idle priority and built only
`ghogx_character_variant_catalog_test`. Ran:

`ghogx_character_variant_catalog_test --midori-assets-only gh2_ps2_hybrid_assets/GEN/MAIN.HDR gh2_ps2_hybrid_assets/GEN/MAIN_0.ARK`

The catalog test passed: `Midori external assets: 2 models, 2 textures, 331
clips`. Refreshed `analysis/gh3_midori_catalog_test_summary.json` with current
rlegaim package asset hashes and local-GEN evidence.

Reran `tools/gh3_midori_pipeline_verify.py`: status
`guitar_hero_classic_midori_runtime_verified`, `checks=79`, `failures=0`,
`clips=280`, `assets=6`. Focused unit tests still pass `44/44`.

No ISO was used or mounted at game time. Catalog build/test and Python tests ran
low/Idle priority.

Evidence:
`.codex/current-evidence/midori-catalog-refresh-20260818/catalog_refresh_decision.json`.

Next gate remains user direct visual approval of the two rlegaim gameplay
screenshots.

## 2026-08-18 rlegaim representative pose review

Added `--low-priority` to `tools/gh3_midori_pose_review.py` so native viewer
captures launch with Windows below-normal priority.

Ran the pose review from local `gh2_ps2_hybrid_assets/GEN` plus deployed loose
DLC under `gh2_ps2_hybrid_assets/DLC`. Status:
`native_viewer_representative_pose_framing_review_passed`, `proofs=9`,
`failures=0`, `min_margin=10`.

Generated contact sheet:
`analysis/gh3_midori_pose_review_proofs/rlegaim_pose_review_contact_sheet.jpg`.

Reran aggregate verifier: `guitar_hero_classic_midori_runtime_verified`,
`checks=79`, `failures=0`. Unit tests still pass `44/44`.

No ISO was used or mounted at game time.

Evidence:
`.codex/current-evidence/midori-rlegaim-pose-review-20260818/pose_review_decision.json`.

Next gate remains user direct visual approval of the gameplay screenshots and
representative pose contact sheet.

## 2026-08-18 rlegaim visual rejection

User reviewed
`analysis/gh3_midori_pose_review_proofs/rlegaim_pose_review_contact_sheet.jpg`
and reported: only the middle-right pose is coherent.

That contact-sheet case is `midori_1_accessory_acc01_f030`
(`gh3_midori_1`, `gh3_guit_mido_acc01`, frame 30). Treat the current rlegaim
package as visually rejected even though gameplay, pose-framing, catalog, and
aggregate audits are green. The package loads as loose DLC from local
`gh2_ps2_hybrid_assets/GEN`, but normal guitar animation poses remain
incoherent.

Evidence:
`.codex/current-evidence/midori-rlegaim-visual-rejection-20260818/visual_rejection_decision.json`.

Next branch: compare the coherent accessory/static case against the failing
guitar-animation cases, then broaden the matrix-local/Control_Root diagnosis
beyond the sampled `attackl` frames. Do not call Midori complete until direct
user visual approval passes.

Direct visual inspection confirms the rejected contact sheet has multiple
non-bipedal, collapsed guitar poses; the previous pose-review script only proved
framing, not semantic pose quality. Source IR comparison shows the only coherent
contact-sheet case, `gh3_guitarist_midori_acc01`, is sparse
(`animated_bone_count=3`, `trans_changes=6`) and has no sampled
Control_Root/pelvis/torso/leg activity. The failed guitar cases exercise the
full body (`animated_bone_count=16..50`), with pelvis translation keyed
`10..48` times and heavy torso/leg rotation.

Evidence:
`.codex/current-evidence/midori-rlegaim-visual-rejection-20260818/source_clip_activity_comparison.json`.

Conclusion: rlegaim did not solve the full-body animation space; stop treating
accessory/static coherence as a candidate success signal.

Repaired stale diagnostic script `tools/gh3_midori_pose_score.py` after
`compose_rotation_with_bind` gained `hmx_quat_mode`; diagnostic calls now pass
`direct`. Ran the scorer at Idle priority on the rejected contact-sheet body
clips. The failed clips still show high mean body-bone rotation distances even
under the best simple variant (`edit-local`): medium idle `74.42`, attackl
`77.31`, fast jump `69.65`, fast solo `74.50`, transition out `62.11` degrees.
The coherent accessory clip has `body_bone_count=0`, confirming it does not
exercise this space.

Evidence:
`.codex/current-evidence/midori-rlegaim-visual-rejection-20260818/pose_score_multiclip_summary.json`.

Next branch must reopen full-body matrix-local/Control_Root/pelvis transform
evaluation across these clips, or automate GLB/source-bridge comparison after
source file regeneration.

## 2026-08-18 GLB/source bridge reopening

Added automated low-priority bridge generation support. 
`tools/gh3_midori_source_bridge_export.py` now accepts `--low-priority`; new
`tools/gh3_midori_review_source_bridge_batch.py` extracts only the required GH3
Midori source files, emits Blender/NXTools GLB + pose JSON for review clips, and
deletes extracted source scratch.

Recreated the pinned NXTools checkout in `%TEMP%` at commit
`6cea808a27d6773bde55e947c9f0ffd72081e164` and generated source bridges for
the five visually failed body cases: medium idle, attack left, fast jump, fast
solo, and transition out. The accessory case was intentionally excluded from
the clean manifest after Blender/NXTools retained no body curves for the
requested source bones, proving it is not a body-retarget success case.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/review_source_bridge_batch_manifest.json`.

Added `tools/gh3_midori_bridge_pose_compare.py`; bridge-vs-IR comparison shows
the raw IR evaluator is not source-authoritative for these full-body poses
(`ir_evaluated_vs_blender` mean mismatch: idle `140.220`, attack `136.800`,
jump `134.033`, solo `150.824`, transition `125.839` degrees).

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/bridge_pose_compare_summary.json`
and
`.codex/current-evidence/midori-review-source-bridges-20260818/bridge_diagnosis_decision.json`.

Next branch: stop deriving final candidate rotations from raw source IR pose
reconstruction alone. Use the Blender/NXTools GLB pose bridge as the
authoritative source pose stream for the target-local/Control_Root/pelvis
derivation, then automate bridge generation for production scope if the
bridge-fed candidate becomes visually viable.

## 2026-08-18 refreshed two-frame bridge/policy sweep

Updated `tools/gh3_midori_review_source_bridge_batch.py` so review cases export
frame `0` plus the reviewed frame; regenerated the five failed body bridges with
extracted GH3 source scratch removed afterward. This makes bridge-fed pelvis
translation policies meaningful instead of using the review frame as its own
base.

Updated `tools/gh3_midori_pose_report.py` and `tools/gh3_midori_acp_stage.py`
so duplicate target clips are disambiguated by the source-pose bridge animation
path (`frontend/gh3_guit_midori_tran_atoout` vs non-frontend). Added
`tools/gh3_midori_bridge_policy_multiclip_report.py` and swept bridge-fed
policies across all five body cases.

Result: no current policy is promotable. `glbframeik-sourcepos` has low position
error but flips right shin on `fast_jump` (`-0.524948`) and left shin on
`fast_solo` (`-0.400349`). `glbframeik-sourceposrot` flips attack shins
(`right=-0.344378`, `left=-0.080896`). Current `rlegaim`, already visually
rejected, now also shows a bridge-fed right-shin flip on `fast_jump`
(`-0.471900`), despite its tempting average pose error.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/bridge_policy_decision.json`.

Next branch: develop a bridge-position two-bone leg solve or bend-plane sign
gate driven from GLB pose bridge data, and require all five failed body cases to
have positive left/right shin dots before spending capture time.

## 2026-08-18 legbakeaim registration refresh and visual gate

User visual feedback is now a hard gate: all current rlegaim body captures are
rejected as non-bipedal except the middle-right accessory pose. That accessory
pose is sparse/static and has no body curves, so it is not evidence that the
full-body retarget works.

Added a diagnostic full-leg bake-aim policy:
`matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-torsoframe-legbakeaim-bakepos-bind`.
The first sweep looked promising only because the policy was incompletely
registered. After adding it to `ROOTWORLD_TORSO_POLICIES`, the pelvis correction
map, and the bridge multiclip defaults, focused verification passes
(`python -m unittest tools.gh3_midori_pipeline_test`: 44 tests OK), but the
policy is rejected:

- average pose error `0.114641`
- max pose error `0.155886`
- min right shin dot `-0.510482` on `midori_1_fast_solo_f090`
- min left shin dot `0.422834`

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/bridge_legbakeaim_registration_decision.json`.

Current status/tl;dr: no visually acceptable body candidate exists yet. The
useful progress is that the source-authoritative Blender/NXTools GLB bridge is
automated for the five failed body clips, the accessory false-positive is
documented, raw source IR has been ruled out as source-authoritative, and the
current bridge-fed policies are rejected before spending capture time.

Next branch: resume at the pelvis-only matrix-local/Control_Root diagnosis,
using GLB bridge pose data as source-authoritative. If an intermediate `.glb`
bridge is needed, that is acceptable, but the final flow must stay automated.
Do not run from the GH2 ISO at game time; use local `gh2_ps2_hybrid_assets/GEN`
and loose DLC only. Keep Blender, converter builds, and GHC captures at low/Idle
CPU priority.

## 2026-08-18 root/pelvis GLB bridge diagnostic and ihatecompvir route

Added `tools/gh3_midori_root_pelvis_bridge_diagnostic.py` and ran it at Idle
over the five failed body clips using the existing Blender/NXTools pose bridges.
Result:

- Control_Root rotation delta is negligible (`max=0.016083` deg).
- Control_Root mapped translation is zero for medium idle, attack left, fast
  jump, and fast solo; it is large only on transition-out (`16.224950` GH2
  units).
- The repeated signal is pelvis world-vs-Control_Root-local mismatch:
  `122.666104` deg for the four main body cases and `89.996` deg on
  transition-out.
- Pelvis-minus-root translation deltas are tiny, so this is primarily
  parent/local rotation interpretation, not big pelvis travel.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/root_pelvis_bridge_diagnostic.json`.

User called out ihatecompvir tooling for GLB -> MILO. Current answer: partially
used, but not enough. We already use Blender/NXTools for the GH3 source GLB/pose
bridge. Local ihatecompvir sources are available:

- `ihatecompvir-public-milo-sources/glTFMilo`
- `ihatecompvir-public-milo-sources/MiloEditor/MiloLib`
- `ihatecompvir-public-milo-sources/milo_blender`
- `ihatecompvir-public-milo-sources/re-notes/raw_notes/milo/gh2/charclipsamples.txt`

`glTFMilo` is not a drop-in Midori animation converter: its CLI exposes
RB2/RB3/TBRB xbox/ps3 targets and writes GLB animation as generic `TransAnim`
keys. Midori needs GH2 PS2 guitarist `CharClipSet`/`CharClipSamples`. MiloLib
is still directly useful: it models GH2 PS2 `CharClipSet` revision 14 and
`CharClip` raw sample containers. Use those semantics to validate or replace
the existing C++ GH2 CharClipSamples writer, fed by the source-authoritative GLB
pose samples.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/ihatecompvir_glb_milo_assessment.json`.

Next branch: stop treating `glTFMilo` as a command-line shortcut, but actively
use ihatecompvir/MiloLib/notes as the writer reference. Fix the pelvis
parent/local interpretation first, then only spend capture time after the
five-case bridge report clears positive shin dots and the visual biped gate.

## 2026-08-18 GLB-pose bridge to GH2 MILO structural pass

User clarified that an intermediate GLB route is acceptable if the final path is
automated, and specifically asked whether ihatecompvir tooling is being used
for GLB -> MILO. Current working answer:

- Local ihatecompvir sources are present and are used as format/semantics
  references: `glTFMilo`, `MiloEditor/MiloLib`, `milo_blender`, and GH2
  `CharClipSamples` notes.
- `glTFMilo.exe` is not a drop-in GH2 PS2 guitarist animation converter. Its
  documented CLI targets generic Milo scenes for xbox/ps3/RB-era usage, while
  Midori needs GH2 PS2 `CharClipSet`/`CharClipSamples`.
- The automated route that now works structurally is evaluated GLB pose bridge
  -> ACP samples -> repo `milo_convert_tool build-clipset-from-acp` -> ordinary
  GH2 `CharClipSet`/`CharClipSamples` MILOs, with the writer behavior checked
  against the ihatecompvir notes/MiloLib semantics instead of relying on
  `glTFMilo.exe` as a shortcut.

Patched `tools/gh3_midori_build_pipeline.py` so `--skip-tool-build` still
searches the configured CMake build directory for an already-built
`milo_convert_tool.exe`; current CMake output is under the `Release` subfolder.

Focused tests passed at Idle priority:
`python -m unittest tools.gh3_midori_pipeline_test` -> `57` tests OK.

Structural pipeline rerun at Idle priority, no capture, no runtime proof, and
no ISO runtime. It reused the existing converter and completed successfully:

- Bridge biped gate: `avg_pose=0.034704`, `max_pose=0.173514`,
  `min_r_shin=0.963903`, `min_l_shin=0.929721`, `min_child_aim=-0.163184`.
- ACP disk bridge gate on production stock-rig staging passed:
  `avg_pose=0.000001`, `max_pose=0.000002`, `min_r_shin=1.0`,
  `min_l_shin=1.0`, `min_child_aim=-0.684482`.
- ACP staging: `331` clips, roles `guitar-main:266`, `guitar-ui:6`,
  `guitar-fret:36`, `guitar-strum:23`, `39283685` bytes.
- Built animation MILOs:
  `analysis/gh3_midori_gh2_milos/gh3_midori_main.milo_ps2`,
  `gh3_midori_ui.milo_ps2`, `gh3_midori_fret.milo_ps2`,
  `gh3_midori_strum.milo_ps2`.
- Built model MILOs:
  `analysis/gh3_midori_gh2_models/gh3_midori_1.milo_ps2` and
  `analysis/gh3_midori_gh2_models/gh3_midori_2.milo_ps2`.
- Packaged/deployed loose DLC under the engine and hybrid DLC folders.

This is only a structural GLB/ACP/MILO/package pass. It does not satisfy the
goal because direct visual approval is still required. Treat obviously
non-bipedal captures as immediate rejects; numeric gates only promote a
candidate to visual review.

## 2026-08-18 visual rejection of rootyawfold/sourcepos and axis-align recovery

Ran `tools/gh3_midori_pose_review.py` from local loose DLC only
(`gh2_ps2_hybrid_assets/GEN` plus `gh2_ps2_hybrid_assets/DLC`), with the app
process launched at Idle priority. No GH2 ISO was mounted or used at game time.
The rootyawfold/sourcepos build was directly rejected after sequential review of
all nine produced screenshots:

- `midori_1_medium_idle_f060`: folded forward/sideways, hands through torso
  area.
- `midori_1_attack_left_f030`: more legible than earlier failures but still a
  sideways, folded non-biped silhouette.
- `midori_1_fast_jump_f040`: hard reject, mostly horizontal/collapsed.
- `midori_1_fast_solo_f090`: hard reject, balled-up with ambiguous limbs.
- `midori_1_transition_out_f050`: sideways and compacted.
- `midori_1_accessory_acc01_f030`: upside-down/vertical around the prop.
- `midori_1_hand_overlay_f010`: lower half partially bipedal but upper body and
  arms tangled.
- `midori_2_medium_idle_f060`: same folded sideways idle failure.
- `midori_2_attack_left_f030`: same flawed body-space interpretation.

Rendered the five retained source GLB bridge poses at their review frames. Those
source renders are also horizontal/collapsed, proving the sourcepos/rootyawfold
branch was faithfully converting a visually bad GLB/evaluated-space bridge
rather than losing data in ACP or `CharClipSamples`. This keeps the diagnosis
upstream at source/world-basis interpretation, not MILO packing.

Rebuilt and deployed the full bank with the older visually viable policy
`matrix-local-axis-align-bind`, intentionally without the sideways GLB
source-pose manifest. The build completed as ordinary loose DLC:

- ACP staging: `331` clips, roles `guitar-main:266`, `guitar-ui:6`,
  `guitar-fret:36`, `guitar-strum:23`.
- Animation MILOs: `4` packages, `331` clips, `20856442` bytes.
- DLC deploy: `community.gh3.midori`, `6` assets, `21416145` bytes.

Patched `tools/gh3_midori_pose_review.py` so review captures use Idle priority,
wide camera distance can override per-case close-up distances, and a
review-only `--char-offset-z` can frame full-body screenshots. Also relaxed the
minimum visible width from `120` to `100` pixels so a narrow straight-on
accessory pose is not rejected only for being slender.

The best current visual evidence is:
`analysis/gh3_midori_pose_review_axisalign_wide_offset_proofs/pose_review_proof_manifest.json`.
Command shape:

`python tools/gh3_midori_pose_review.py --capture --low-priority --app gh2_ps2_hybrid_assets/ghogx_app.exe --ark-dir gh2_ps2_hybrid_assets/GEN --addons-dir gh2_ps2_hybrid_assets/DLC --camera-distance 230 --char-offset-z 18 --proof-dir analysis/gh3_midori_pose_review_axisalign_wide_offset_proofs --output analysis/gh3_midori_pose_review_axisalign_wide_offset_proofs/pose_review_proof_manifest.json --print-summary`

Result after sequential visual inspection of all nine screenshots:

- All nine are coherent bipedal poses.
- Both outfits pass the immediate non-biped rejection gate.
- The verifier passes: `native_viewer_representative_pose_framing_review_passed`,
  `proofs=9`, `failures=0`, `min_margin=56`.

Focused tests pass at Idle priority:
`python -m unittest tools.gh3_midori_pipeline_test` -> `57` tests OK.

This is major progress, but not goal completion. Remaining work: run/inspect the
required in-song/local-GEN visual approval path for the axis-align deployed DLC
and continue broader animation review beyond the nine representative native
viewer frames.

## 2026-08-18 axis-align local-GEN in-song proof

Patched `tools/gh3_midori_gameplay_proof.py` so `--low-priority` launches the
gameplay capture at Windows Idle priority instead of BelowNormal. Also widened
the allowed diagnostic front-camera `z` target window to include the actual
axis-align target (`z=-332.07`), which the screenshots show is still on stage.

Ran the in-song proof from local assets only:

- app: `gh2_ps2_hybrid_assets/ghogx_app.exe`
- ark dir: `gh2_ps2_hybrid_assets/GEN`
- addons dir: `gh2_ps2_hybrid_assets/DLC`
- no GH2 ISO mounted or used at game time
- capture process at Idle priority

Evidence:
`analysis/gh3_midori_ghc_gameplay_axisalign_proofs/gameplay_proof_manifest.json`.

Result after verifier rerun on the captured screenshots:
`status=in_song_midori_variant_animation_verified`, `failures=0`,
visible pixels `840511,847111`.

Sequential visual read:

- `gh3_midori_1_variant_gameplay_guitarist0_f60.bmp`: coherent visible
  upper-body/guitar pose in song, not folded/collapsed; lower body is hidden by
  normal gameplay/HUD framing.
- `gh3_midori_2_variant_gameplay_guitarist0_f60.bmp`: same for outfit 2.

This proves the axis-align DLC loads and plays in-song as ordinary loose DLC
from local `GEN` plus `DLC`, with sane hand contact and no visible upper-body
collapse. It is still not final approval by itself because the gameplay camera
does not expose enough lower body for full sequential visual signoff. Current
strongest full-body evidence remains the nine-frame native viewer wide-offset
review:
`analysis/gh3_midori_pose_review_axisalign_wide_offset_proofs/pose_review_proof_manifest.json`.

Focused tests still pass:
`python -m unittest tools.gh3_midori_pipeline_test` -> `57` tests OK.

## 2026-08-18 pelvis/root-fold numeric candidate

Target graph inspection confirmed the source/target root mismatch:

- GH3 source `Bone_Pelvis` parent is `Control_Root`.
- GH2 target `bone_pelvis` is root-level in the current mapping.

Swept the existing sourcepos pelvis/root-fold policies over the five failed body
cases using the GLB pose bridges. `rootyawfold` is the first pelvis/root branch
in this pass to clear positive shin dots across all five cases:

- average pose error `0.146616`
- max pose error `0.176421`
- min right shin dot `0.276150`
- min left shin dot `0.315446`

Weak cases remain `fast_jump` and `fast_solo`, so this is only a numeric
candidate, not a success. Direct visual approval is still required.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/bridge_pelvisroot_decision.json`.

Also added per-clip GLB pose bridge manifest support:

- `tools/gh3_midori_acp_stage.py` accepts `--source-pose-bridge-manifest`.
- `tools/gh3_midori_build_pipeline.py` forwards it to ACP staging.
- Focused tests pass: `python -m unittest tools.gh3_midori_pipeline_test` ran
  45 tests OK.
- One-clip `fast_solo` staging probe with `rootyawfold` confirmed
  `source_pose_bridge_active=True` and bridge frames `0,90`; raw ACP probe was
  deleted as rebuildable.

Next branch: build and visually capture a `rootyawfold` candidate from local
loose DLC only, passing
`.codex/current-evidence/midori-review-source-bridges-20260818/review_source_bridge_batch_manifest.json`
as `--source-pose-bridge-manifest`. Do not use the GH2 ISO at game time. If the
capture path needs broader animation coverage, first expand the automated GLB
bridge generation beyond the five review clips or move to the MiloLib-validated
GLB-pose-to-GH2 CharClipSamples writer path.

## 2026-08-18 rootyawfold structural build and pelvis matrix-local diagnosis

Built the `rootyawfold` candidate structurally from local staged/source assets,
with the five-case GLB bridge manifest passed through ACP staging. The command
ran at Idle priority, did not launch the game, and did not mount/use the GH2 ISO
at game time. Output was ordinary loose DLC under the existing engine/hybrid DLC
folders. Structural build passed (`331` staged clips; anim MILOs `26,285,178`
bytes; deployed DLC `26,844,881` bytes), but this is not visual success.

Direct inspection of retained yawfold image
`.codex/current-evidence/midori-yawfold-visual-20260817/yawfold_f45_yaw90.png`
rejects the branch: the body is folded and not recognizably bipedal. Treat
non-bipedal screenshots as immediate rejects before considering numeric pose
metrics.

Added `tools/gh3_midori_pelvis_matrix_local_diagnostic.py` and ran it over the
five retained GLB pose bridges. Result:

- `Control_Root` absolute basis is large (`122.666098` deg from identity).
- `Control_Root` pose delta is negligible (`0.016083` deg max).
- `Bone_Pelvis` world-vs-parent-local mismatch is the same large static basis
  (`122.666104` deg max).
- `Bone_Pelvis.matrix_local` is static rest data; the animated source is the
  evaluated GLB `pose`, not `matrix_local`.
- Source parent is `Bone_Pelvis -> Control_Root`; target parent is
  `bone_pelvis -> <root>`.

This explains why `rootyawfold` did not help: folding root/yaw delta is almost
a no-op because the missing transform is the static/absolute `Control_Root`
basis. A pelvis-only fix must either bake or remove that static basis
consistently across pelvis, children, and positions before writing GH2
`bone_pelvis` channels.

Added diagnostic policy
`matrix-bridge-eval-world-targetgraph-axis-align-bind` so evaluated-world
rotations are converted back to local space using the GH2 target graph rather
than the GH3 source graph. Five-case bridge triage rejected it before capture:
avg pose `57.226479`, max pose `71.423520`, min right shin `-0.580641`, min
left shin `-0.320851`. Focused tests now pass:
`python -m unittest tools.gh3_midori_pipeline_test` ran `46` tests OK.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/pelvis_matrix_local_diagnostic.json`
and
`.codex/current-evidence/midori-review-source-bridges-20260818/rootyawfold_structural_and_pelvis_matrix_decision.json`.

Current answer to the ihatecompvir question: yes, use the local sources as the
format reference, but not `glTFMilo.exe` as a direct converter. `glTFMilo` emits
generic scene `TransAnim`; GH2 PS2 guitarist animation needs
`CharClipSet`/`CharClipSamples`. The shortest useful route is to feed
source-authoritative GLB pose samples into the existing C++ GH2
`CharClipSamples10` writer, validating layout/semantics against
`ihatecompvir-public-milo-sources/MiloEditor/MiloLib` and the GH2
`charclipsamples.txt` notes.

Next branch: stop spending capture slots on rootyawfold. Implement or validate
a direct GLB-pose-to-GH2 `CharClipSamples` path that normalizes the absent
`Control_Root` basis consistently for pelvis, descendants, and positions. Only
promote a candidate to local-GEN capture after the five-case bridge gate is
positive-shin and biped-shape plausible.

## 2026-08-18 bridge-base/sourcepos candidate rejected

The first bridge-base policy registration was initially incomplete: the new
policy strings were present in `ROTATION_POLICIES` but missing from the large
matrix dispatch allowlist, so the branch was not actually executing. Fixed the
dispatch allowlist and added focused tests.

Added diagnostic policies:

- `matrix-bridge-eval-world-rest-targetgraph-axis-align-bind`
- `matrix-bridge-eval-bridgebase-targetgraph-axis-align-bind`
- `matrix-bridge-eval-bridgebase-targetgraph-axis-align-sourcepos-bind`

The bridge-base variant uses the GLB pose bridge frame `0` evaluated pose as
the rest reference instead of comparing evaluated/world pose to static local
`Bone.matrix_local`. The sourcepos variant also uses the existing target
skeleton bake-position writer so the lower body receives GLB bridge-relative
`.pos` rows.

Five-case bridge triage after the dispatch fix:

- `...bridgebase-targetgraph-axis-align-bind`: avg pose `55.131754`, max
  `78.913822`, min right shin `-0.000555`, min left shin `-0.558653`.
- `...bridgebase-targetgraph-axis-align-sourcepos-bind`: avg pose `35.364700`,
  max `46.566020`, min right shin `-0.822559`, min left shin `-0.361987`.
- `rootyawfold` remains numerically best (`avg=0.146616`, positive shins) but
  is visually rejected as non-bipedal, so it is not a success/capture target.

Focused tests pass:
`python -m unittest tools.gh3_midori_pipeline_test` ran `49` tests OK.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/bridgebase_sourcepos_policy_report.json`
and
`.codex/current-evidence/midori-review-source-bridges-20260818/bridgebase_sourcepos_decision.json`.

Next branch remains the GLB-pose-to-GH2 `CharClipSamples` writer/validator:
derive full target-graph local transforms and positions from source-authoritative
GLB pose samples, normalize the absent `Control_Root` basis once at the skeleton
solve level, and validate byte/layout semantics against ihatecompvir MiloLib and
GH2 `charclipsamples.txt`. Do not launch GHC until the automated bridge gate is
positive-shin and the intermediate skeleton is visually/plausibly bipedal.

## 2026-08-18 bridge-base solved-skeleton compose sweep rejected

Extended the bridge-base/sourcepos diagnostics to use the existing target
skeleton solved-rotation path, so thigh/knee rotations are aimed at the
GLB-derived child positions instead of carrying bridge-base local axes. Added:

- `matrix-bridge-eval-bridgebase-targetgraph-axis-align-sourcepos-solve-altlocal-bind`
- `matrix-bridge-eval-bridgebase-targetgraph-axis-align-sourcepos-solve-desired-localt-altlocal-bind`
- `matrix-bridge-eval-bridgebase-targetgraph-axis-align-sourcepos-solve-desiredt-local-altlocal-bind`
- `matrix-bridge-eval-bridgebase-targetgraph-axis-align-sourcepos-solve-localt-desired-altlocal-bind`

These are registered as rotation policies, bake-position policies, altlocal
translation policies, and solved-skeleton rotation policies. Focused tests pass:
`python -m unittest tools.gh3_midori_pipeline_test` ran `52` tests OK.

Five-case bridge sweep:

- default solve-altlocal: avg pose `30.229316`, max `46.566020`, min right shin
  `-0.584740`, min left shin `-0.245391`.
- desired-localT: avg `30.239370`, max `46.566020`, min shins
  `-0.580641` / `-0.320851`.
- desiredT-local: avg `30.232768`, max `46.566020`, min shins
  `-0.855608` / `-0.591522`.
- localT-desired: avg `30.229754`, max `46.566020`, min shins
  `-0.460145` / `-0.420432`.

All are rejected before capture. Segment audits show the remaining failure is a
direction/composition problem with preserved lengths:

- `fast_jump` default solve-altlocal worst segment `Rknee_to_Rankle`,
  direction dot `-0.584736`, length delta `0.000001`.
- `fast_solo` default solve-altlocal worst segment `Lthigh_to_Lknee`,
  direction dot `-0.906775`, length delta `0.0`.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/bridgebase_solve_order_policy_report.json`,
`.codex/current-evidence/midori-review-source-bridges-20260818/fast_jump_solve_altlocal_segment_audit.json`,
`.codex/current-evidence/midori-review-source-bridges-20260818/fast_solo_solve_altlocal_segment_audit.json`,
and
`.codex/current-evidence/midori-review-source-bridges-20260818/bridgebase_solve_order_decision.json`.

Next branch: stop adding single compose-order variants. Build a direct
target-graph skeleton solve from GLB positions that proves each emitted local
translation/rotation reconstructs the desired parent-child vector before it is
packed into `CharClipSamples`; then compare that packed output against MiloLib
layout semantics. The current failures have correct segment lengths but
opposite directions, so the validator must assert per-segment direction dots,
not just byte validity or pose-error averages.

## 2026-08-18 pack-safe targetgraph branch rejected

Added a pack-safe variant:

- `matrix-bridge-eval-bridgebase-targetgraph-axis-align-sourcepos-solve-packsafe-altlocal-bind`

This branch bakes target-graph child translations against the parent world
rotation reconstructed after HMX quaternion pack/unpack, matching the isolated
validator instead of the ideal pre-pack matrix. It is registered as a rotation,
bake-position, altlocal translation, and solved-skeleton policy. Focused tests
pass at Idle priority:
`python -m unittest tools.gh3_midori_pipeline_test` ran `53` tests OK.

Five-case bridge sweep:

- previous solve-altlocal: avg pose `30.229316`, max `46.566020`, min right
  shin `-0.584740`, min left shin `-0.245391`.
- pack-safe solve-altlocal: avg pose `27.766754`, max `46.566020`, min right
  shin `-0.863932`, min left shin `-0.967666`.
- rootyawfold stays numeric-only (`avg=0.146616`, positive shins) but is
  visually rejected as non-bipedal.

The isolated target-graph validator proved that local translations can
reconstruct desired positions after HMX pack/unpack, but this is not enough:
pack-safe made the lower-body direction gate worse. Rejected before capture;
no GHC run and no ISO runtime use.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/target_graph_solve_validator_report.json`,
`.codex/current-evidence/midori-review-source-bridges-20260818/packsafe_policy_report.json`,
and
`.codex/current-evidence/midori-review-source-bridges-20260818/packsafe_policy_decision.json`.

ihatecompvir usage: local `glTFMilo`, `MiloEditor/MiloLib`, and GH2
CharClipSamples notes are present and should be used as source/format
references. `glTFMilo` is still not a drop-in `GLB -> GH2 PS2 guitarist MILO`
converter because it emits later-title generic `TransAnim` scene animation,
not GH2 PS2 `CharClipSet`/`CharClipSamples`. The useful next branch is an
automated GLB-pose-to-GH2 `CharClipSamples` writer/validator, either by
validating the existing C++ writer against MiloLib semantics or replacing the
writer surface with a MiloLib-backed equivalent. Keep the biped segment gate
ahead of every game capture.

## 2026-08-18 fail-closed bridge gate added

`tools/gh3_midori_bridge_policy_multiclip_report.py` now has an explicit
pre-capture gate:

- `--fail-on-reject` exits nonzero when any requested policy fails.
- `--gate-min-shin-dot` rejects folded/non-bipedal limb directions. With
  `--fail-on-reject`, the default is `0.0` if no threshold is provided.
- `--gate-max-pose-error` optionally rejects policies whose worst bridge-case
  pose error exceeds a chosen limit.

Focused tests pass at Idle priority:
`python -m unittest tools.gh3_midori_pipeline_test` ran `55` tests OK.

Verification run at Idle priority against the current rejected solve and
pack-safe solve policies exited `2` as expected:

- solve-altlocal: max pose `46.566020`, min right shin `-0.584740`, min left
  shin `-0.245391`.
- pack-safe solve-altlocal: max pose `46.566020`, min right shin `-0.863932`,
  min left shin `-0.967666`.

This is not a visual success; it is a guardrail so invalid GLB-pose-to-ACP
sample streams cannot advance into GH2 `CharClipSamples`, loose DLC deployment,
or GHC capture by accident. Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/bridge_gate_reject_report.json`
and
`.codex/current-evidence/midori-review-source-bridges-20260818/bridge_gate_decision.json`.

Next branch should wire the gate into the automated build/capture path and then
continue the real writer work: a GLB-pose-to-GH2 `CharClipSamples`
writer/validator using ihatecompvir MiloLib semantics, with this gate as the
minimum pre-capture filter.

## 2026-08-18 pipeline bridge gate wired

`tools/gh3_midori_build_pipeline.py` now runs `bridge_biped_gate` when
`--source-pose-bridge-manifest` is supplied unless `--skip-bridge-gate` is set.
The gate runs immediately after source IR availability is checked and before
converter build, model staging, animation MILO packaging, loose DLC deployment,
or GHC capture. The pipeline also launches subprocesses with
`subprocess.IDLE_PRIORITY_CLASS` on Windows.

Pipeline gate settings mirror production ACP staging:

- translation policy `animated`
- local delta basis `source`
- compose order `bind-delta`
- edit local mode `edit-inv-frame`
- HMX quat mode `transpose`
- min shin dot `0.0`
- max pose error `1.0`

Focused tests pass at Idle priority:
`python -m unittest tools.gh3_midori_pipeline_test` ran `55` tests OK.

Preflight run at Idle priority with the current pack-safe policy stopped at
`bridge_biped_gate` and exited `2`; it did not run `stage_models`,
`build_anim_*`, DLC deploy, or GHC capture. Production-flag gate result for
pack-safe: avg pose `30.321585`, max pose `46.566020`, min right shin
`-0.960752`, min left shin `-0.933769`.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/pipeline_bridge_gate_report.json`
and
`.codex/current-evidence/midori-review-source-bridges-20260818/pipeline_bridge_gate_decision.json`.

## 2026-08-18 disk ACP bridge gate added

Added `tools/gh3_midori_acp_disk_bridge_gate.py`. Unlike the in-memory pose
report, this reads the actual staged `.acp` files referenced by
`analysis/gh3_midori_acp_stage/stage_manifest.json`, reconstructs body world
positions and child-aim directions from the on-disk sample bytes, compares
those positions to the GLB bridge cases, and applies the same
biped/visual-reject gate before `milo_convert build-clipset-from-acp` can
package GH2 `CharClipSamples`.

`tools/gh3_midori_build_pipeline.py` now runs `acp_disk_bridge_gate` after ACP
staging/reuse and before animation MILO packaging whenever
`--source-pose-bridge-manifest` is supplied and `--skip-bridge-gate` is not set.

Focused tests pass at Idle priority:
`python -m unittest tools.gh3_midori_pipeline_test` ran `57` tests OK.

Current staged output diagnosis:

- staged policy:
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-rootyawfold-bakepos-bind`
- disk ACP gate exit: `2`
- avg pose error `6.061952`, max pose error `6.068832`
- min right shin `1.0`, min left shin `1.0`
- min child-aim dot `-0.774455`
- reject reasons: prior direct visual rejection, pose error above `1.0`, and
  child-aim dot below `0.0`

Worst child-aim rows:

- `midori_1_fast_solo_f090` `Bone_Chest->Bone_Neck`: dot `-0.774455`, angle
  `140.756` degrees.
- `midori_1_attack_left_f030` `Bone_Ankle_L->Bone_Toe_L`: dot `-0.229933`,
  angle `103.293` degrees.
- `midori_1_attack_left_f030` `Bone_Ankle_R->Bone_Toe_R`: dot `-0.129497`,
  angle `97.441` degrees.

This is important: the current staged rootyawfold ACP bytes preserve lower-leg
direction, but broader body positions and emitted bone bases fail. The failed
visual review is not explained by ACP sample-byte loss or GH2 `CharClipSamples`
byte preservation; it is upstream in the pose-space/character-space
interpretation of rotations and child bases. The next fix must change the
source-to-target pose solve, especially torso/neck and foot child-basis aiming,
not just the ACP/MILO writer.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/acp_disk_bridge_gate_report.json`
and
`.codex/current-evidence/midori-review-source-bridges-20260818/acp_disk_bridge_gate_decision.json`.

## Runtime/source facts

- GHC `apply_gh2_typed_pose` treats full-weight `.quat` as an absolute local
  rotation (`engine/src/character/char_clip.cpp` around line 11018).
- Hmx sample storage is XYZW and GHC's `quat_to_rot` materializes the transpose
  of a standard column quaternion matrix. `matrix3_to_hmx_quat` already handles
  this correctly.
- The failure reproduces on stock Glam, so it is animation space conversion,
  not Midori skinning.
- Run Blender, converter builds, and GHC captures at Idle priority.

## Files changed but not committed

- `tools/gh3_midori_ir.py`
- `tools/gh3_midori_gh2_bridge.py`
- `tools/gh3_midori_acp_stage.py`
- `tools/gh3_midori_pipeline_test.py`
- `tools/gh3_midori_pelvis_matrix_diagnostic.py`
- `tools/gh3_midori_source_visual.py`
- `GuitarHeroOGX-main-ui-engine/tools/milo_convert/milo_convert_tool.cpp`
- `.codex/MIDORI_HIERARCHY_HANDOFF.md`
- `.codex/CURRENT_STATE.md`

The experimental `edit-local-pose` policy and `source-row` basis are diagnostic
surface only; remove them if the final derivation does not use them. Production
pipeline still requests the old failing source/bind-delta policy and must not be
declared ready.

## 2026-08-18 GLB-to-MILO progress checkpoint

Direct answer to the user correction/question:

- Do not mount or depend on the GH2 ISO at game time. Use local
  `gh2_ps2_hybrid_assets/GEN` plus loose `DLC/community.gh3.midori` only.
- Run heavy Python/build/GHC capture subprocesses at Windows Idle priority.
- Yes, use ihatecompvir's public Milo sources as format/reference material for
  the GLB-to-MILO gap, especially `MiloEditor/MiloLib` and the GH2
  `CharClipSamples` semantics. Do not treat `glTFMilo.exe` as a direct shortcut:
  it emits generic/later-title scene `TransAnim`, while GH2 PS2 guitarist
  runtime requires `CharClipSet`/`CharClipSamples`.

Current TL;DR:

- The ordinary loose-DLC packaging path works.
- The only visually coherent deployed branch remains the older
  `matrix-local-axis-align-bind` axis-align build. It passed the current
  nine-frame native GHC viewer review and two in-song local-GEN screenshots, but
  the goal is still open because broader/direct visual approval is not done.
- The GLB/sourcepos/rootyawfold branch is structurally automated from GLB pose
  samples through ACP and the repo C++ GH2 `CharClipSamples10` writer, but it is
  visually rejected: most captures were non-bipedal. Treat those as immediate
  rejects regardless of numeric pose metrics.
- The pelvis-only matrix-local diagnosis found that `Bone_Pelvis.matrix_local`
  is static rest data; the animated GLB source is evaluated `pose`. The missing
  transform is the large static `Control_Root` basis, not animated root yaw, so
  rootyawfold is nearly a no-op and should not receive more capture time.
- Pre-capture and disk-ACP gates are now fail-closed before MILO packing/capture.
  The staged rootyawfold/sourcepos bytes preserve shin direction but fail body
  pose error and child-aim direction, especially `Bone_Chest->Bone_Neck` and
  ankle-to-toe rows.

Next useful work:

Build or repair the automated GLB-pose-to-GH2 local transform solver so the
emitted `.pos` and `.quat` samples reconstruct desired parent-child vectors
under the target graph before packing. The existing isolated target-graph
validator proves position reconstruction can be perfect, but also reports
180-degree rotation round-trip deltas; positions alone are not enough. The
candidate must pass the disk ACP gate for pose error, shin dots, and child-aim
dots before any GHC capture or DLC deployment.

## 2026-08-18 rootyawfold bakeaim-altlocal diagnostic rejected

Added one focused diagnostic policy:

`matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-rootyawfold-bakeaim-altlocal-bakepos-bind`

Purpose: keep the previously best rootyawfold aim-altlocal position/leg branch,
but use the same transpose-based child-axis convention as the disk ACP gate:
`transpose(world_rotation) * bind_child_local_vector`. This was intended to fix
the known child-aim failures without going back to visual capture.

Results at Idle priority, no game launch and no ISO runtime use:

- Focused tests: `python -m unittest tools.gh3_midori_pipeline_test` -> `58`
  tests OK.
- In-memory five-case bridge gate passed:
  avg pose `0.034704`, max pose `0.173514`, min right shin `0.963903`, min left
  shin `0.929721`, min child aim `1.0`.
- Corrected all-clip ACP staging used
  `review_source_bridge_batch_manifest.json` and confirmed the five review clips
  had active bridge frames.
- Disk ACP gate still rejected the actual staged bytes:
  avg pose `14.577288`, max pose `32.824649`, min right shin `-0.722347`, min
  left shin `0.132683`, min child aim `0.812805`.

Interpretation: the transpose bakeaim correction fixes the child-axis failure
but does not preserve lower-body pose reconstruction when emitted as staged ACP
bytes. This candidate must not be packaged, deployed, or captured. Keep the
child-aim insight; next work must solve the remaining lower-body position and
rotation composition together so the disk ACP gate passes all three checks:
pose error, shin direction, and child-aim direction.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/rootyawfold_bakeaim_altlocal_policy_report.json`,
`.codex/current-evidence/midori-review-source-bridges-20260818/rootyawfold_bakeaim_altlocal_acp_disk_gate_report.json`,
and
`.codex/current-evidence/midori-review-source-bridges-20260818/rootyawfold_bakeaim_altlocal_decision.json`.

## 2026-08-18 rootyawfold bakeaim-altlocal correction after stock-rig/default fixes

Important correction: the previous "rejected before capture" note became stale
after fixing two diagnostic bugs:

- `gh3_midori_pose_report.py` now defaults to
  `analysis/gh3_midori_stock_glam1_rig.json` and stages the same full channel
  set used by production unless an explicit source filter is supplied.
- `gh3_midori_acp_stage.py` now also defaults to that stock rig, so disk ACP
  gates are checking the same target skeleton assumptions as the pipeline.

With those fixes, the same bakeaim-altlocal policy passed the full structural
pipeline from GLB-derived bridge samples through ACP, MILO packaging, and loose
DLC deployment, all at Idle priority and without using the GH2 ISO at runtime:

- In-memory bridge gate: five cases, avg pose `0.034704`, max pose `0.173513`,
  min right shin `0.963904`, min left shin `0.929721`, min child aim `1.0`.
- Disk ACP gate: five cases, avg pose about `0.000001`, max pose about
  `0.000002`, min right shin `1.0`, min left shin `1.0`, min child aim `1.0`.
- Packaged/deployed loose DLC: `community.gh3.midori`, six assets, about
  `32.7 MB`.

However, native GHC visual review rejected the deployed candidate. All nine
review frames were inspected sequentially:

- `midori_1_accessory_acc01_f030`: lower body inverted/upward; torso collapsed.
- `midori_1_attack_left_f030`: sideways/folded, limbs and guitar clustered.
- `midori_1_fast_jump_f040`: compact pile-up, no upright bipedal read.
- `midori_1_fast_solo_f090`: curled into a ball with guitar through body.
- `midori_1_hand_overlay_f010`: near-upright legs only; upper body detached.
- `midori_1_medium_idle_f060`: horizontal body collapse.
- `midori_1_transition_out_f050`: horizontal collapsed figure.
- `midori_2_attack_left_f030`: sideways body and folded torso.
- `midori_2_medium_idle_f060`: collapsed and non-bipedal.

Decision: reject this branch after visual review despite structural gate pass.
The current deployed loose DLC was overwritten by this rejected diagnostic
candidate, so do not treat the working `community.gh3.midori` folder as an
approved playable build. The older `matrix-local-axis-align-bind` branch remains
the only visually coherent branch so far. If a playable proof is needed before
continuing solver work, rebuild/redeploy axis-align from local assets. Otherwise
continue the automated GLB-pose-to-GH2 local transform solver and use direct
visual bipedal sanity as the first post-build gate.

## 2026-08-18 axis-align redeployed as current local baseline

Restored the current loose DLC deployment from the last visually coherent
baseline:

`matrix-local-axis-align-bind`

No GH2 ISO runtime use. The rebuild used local source IR, local
`gh2_ps2_hybrid_assets/GEN`, and loose `DLC/community.gh3.midori`. Heavy
Python/GHC work was run at Idle priority; child processes were checked and
lowered when needed.

Pipeline result:

- Command:
  `python tools/gh3_midori_build_pipeline.py --skip-tool-build --rotation-policy matrix-local-axis-align-bind --skip-runtime-proof --skip-runtime-deploy --print-summary`
- Staged ACP: `331` clips, `27,094,071` staged bytes.
- Animation MILOs: four packages, `331` clips, `20,856,442` bytes.
- Model MILOs: two packages, structurally valid.
- Deployed loose DLC: `community.gh3.midori`, six assets, `21,416,145` bytes.

Fresh local-GEN native viewer proof:

- Manifest:
  `analysis/gh3_midori_pose_review_axisalign_redeploy_proofs/pose_review_proof_manifest.json`
- Result: `native_viewer_representative_pose_framing_review_passed`, nine
  proofs, zero failures, min margin `56`.
- All nine frames were inspected manually and accepted as bipedal/upright:
  `midori_1_accessory_acc01_f030`, `midori_1_attack_left_f030`,
  `midori_1_fast_jump_f040`, `midori_1_fast_solo_f090`,
  `midori_1_hand_overlay_f010`, `midori_1_medium_idle_f060`,
  `midori_1_transition_out_f050`, `midori_2_attack_left_f030`, and
  `midori_2_medium_idle_f060`.

Fresh local-GEN in-song proof:

- Manifest:
  `analysis/gh3_midori_ghc_gameplay_axisalign_redeploy_proofs/gameplay_proof_manifest.json`
- Result: `in_song_midori_variant_animation_verified`, two proofs, zero
  failures, using `gh2_ps2_hybrid_assets/GEN` plus
  `gh2_ps2_hybrid_assets/DLC`.

Decision: current working loose DLC is back on the visually coherent
axis-align baseline. This is a playable baseline, not final completion: the
goal remains open until direct visual approval passes. Continue solver work from
the lesson that broad child `.pos` baking can self-validate numerically while
folding the GH2 runtime skeleton. The new disk gate now tracks large non-root
child-position samples, and the visually rejected bakeaim-altlocal policy is in
the known visual reject set.

## 2026-08-18 post-MILO runtime sanity gate

Added a post-MILO checker:

`tools/gh3_midori_anim_runtime_sanity.py`

Purpose: inspect actual packed GH2 `CharClipSamples` with
`milo_convert_tool sample-clip` after animation MILO generation. This catches
runtime-dangerous child `.pos` samples that can pass self-consistent ACP bridge
math but fold the native GH2 skeleton in GHC.

Pipeline hook:

- `tools/gh3_midori_build_pipeline.py` now runs `anim_runtime_sanity` after
  `anim_manifest` and before `package_engine_dlc`.
- It fails the pipeline unless `--skip-anim-runtime-sanity` is explicitly used.
- Default threshold: non-root/non-guitar child `.pos` magnitude must be
  `<= 8.0`.

Current axis-align baseline result:

- Manifest: `analysis/gh3_midori_anim_runtime_sanity.json`
- Status: `ok`
- Review samples: `7`
- Packed `.pos` rows inspected: `14`
- Non-root child `.pos` rows: `0`
- Max child position magnitude: `0.0`

Regression tests:

- `python -m unittest tools.gh3_midori_pipeline_test`
- Result: `62` tests OK.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/post_milo_runtime_sanity_gate_decision.json`.

## 2026-08-18 retained evidence refreshed for current deploy

Added a proof-path resolver:

`tools/gh3_midori_proof_paths.py`

Purpose: rollout, review, pipeline verification, and completion audit now use
the newest passing retained gameplay/pose proof manifests instead of silently
falling back to stale canonical paths.

Current resolved proof manifests:

- Gameplay:
  `analysis/gh3_midori_ghc_gameplay_axisalign_redeploy_proofs/gameplay_proof_manifest.json`
- Native pose review:
  `analysis/gh3_midori_pose_review_axisalign_redeploy_proofs/pose_review_proof_manifest.json`

Tooling corrections:

- `gh3_midori_rollout_manifest.py` now scopes the forbidden base archive/ISO
  scan to loose DLC. Local `gh2_ps2_hybrid_assets/GEN` is the intended runtime
  base archive source and is not shipping Midori DLC.
- `gh3_midori_review_packet.py` now links gameplay screenshots from the current
  passing gameplay manifest.
- `gh3_midori_pipeline_verify.py` no longer requires cleaned rebuildable ACP
  stage scratch and instead requires the retained post-MILO runtime sanity
  manifest.
- `gh3_midori_completion_audit.py` now uses the current passing gameplay proof.

Fresh verification, all at Idle priority and without ISO runtime use:

- Runtime source audit: `status=no_midori_specific_runtime_hooks`,
  references `38`, disallowed `0`.
- Rollout manifest: `status=midori_external_dlc_rollout_ready`, failures `0`,
  files `7`, bytes `21,417,753`.
- Pipeline verify: `status=guitar_hero_classic_midori_runtime_verified`,
  checks `79`, failures `0`, clips `280`, assets `6`.
- Review packet: `status=review_packet_ready`, failures `0`, proofs `4`.
- Completion audit: `status=review_ready_pending_user_acceptance`, proven `14`,
  pending `0`, failed `0`.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/current_evidence_refresh_decision.json`.

Goal remains open until direct visual approval passes.

## 2026-08-18 target-graph convention sweep

Extended `tools/gh3_midori_target_graph_solve_validator.py` so it can sweep:

- `--hmx-quat-mode transpose|direct|all`
- `--child-aim-convention transpose|row|best`

The build proof chain now runs:

`--hmx-quat-mode all --child-aim-convention best`

Current validator result:

- `status=no_valid_target_graph_solve`
- `passing_count=0`
- `hmx_quat_mode=all`
- `child_aim_convention=best`
- best convention:
  `local_t_desired/bind/transpose/best/parent_t_world/parent_t_delta`
- best max position error: `0.0`
- best min segment dot: `1.0`
- best selected child-aim dot: `-0.899642`
- best transpose child-aim dot: `-0.981257`
- best row child-aim dot: `-0.899642`
- best max rotation roundtrip error: `180.0` degrees

Decision:

The failure is not just HMX direct-vs-transpose storage, nor row-vs-transpose
child vector interpretation. Even the broad obvious convention sweep leaves
child aim negative and the rotation roundtrip inverted. The next solver must
change how the desired local/child frames are constructed, not just how they are
packed or interpreted.

Verification, all run at Idle process priority:

- Python compile checks: passed.
- Target-graph validator:
  `status=no_valid_target_graph_solve passing=0 hmx=all child=best best_child_aim=-0.899642 best_rot=180.0`.
- Route gate:
  `status=glb_to_milo_route_guarded failures=0 glb_promotable=False`.
- Source anchors: `status=source_anchored`, anchors `23`.
- Pipeline verify: `status=guitar_hero_classic_midori_runtime_verified`,
  checks `85`, failures `0`, clips `280`, assets `6`.
- Completion audit: `status=review_ready_pending_user_acceptance`, proven `17`,
  pending `1`, failed `0`.
- Unit tests: `76` tests passed.

No GH2 ISO mount, game-time ISO path, game launch, or capture was used.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/target_graph_validator_convention_sweep_decision.json`.

Goal remains open until direct visual approval passes.

## 2026-08-18 target-graph validator child-aim gate

Strengthened the isolated target-graph reconstruction validator:

- Tool: `tools/gh3_midori_target_graph_solve_validator.py`
- Output: `analysis/gh3_midori_target_graph_solve_validator_report.json`

Changes:

- Added explicit `status`, `passing_count`, `passing`, and thresholds.
- Added child-aim checks after reconstructed/HMX-roundtripped rotations.
- Added `--fail-on-reject`.
- Wired the validator into:
  - `tools/gh3_midori_build_pipeline.py`
  - `tools/gh3_midori_glb_milo_route_gate.py`
  - `tools/gh3_midori_pipeline_test.py`

Current validator result:

- `status=no_valid_target_graph_solve`
- `passing_count=0`
- best convention:
  `desired_local_t/bind/parent_t_world/parent_t_delta`
- best max position error: `0.0`
- best min segment dot: `1.0`
- best min transpose child-aim dot: `-0.903259`
- best max rotation roundtrip error: `180.0` degrees

Interpretation:

The target-graph position solve is perfect in isolation, and segment directions
can be perfect, but the emitted/reconstructed rotation basis still aims child
vectors backward after HMX roundtrip. This proves positions alone are not a
valid promotion criterion. Do not stage, pack, deploy, or capture this solver
family until a candidate clears child aim and rotation roundtrip.

Verification, all run at Idle process priority:

- Python compile checks: passed.
- Target-graph validator:
  `status=no_valid_target_graph_solve passing=0 best_child_aim=-0.903259 best_rot=180.0`.
- Route gate:
  `status=glb_to_milo_route_guarded failures=0 glb_promotable=False`.
- Source anchors: `status=source_anchored`, anchors `23`.
- Pipeline verify: `status=guitar_hero_classic_midori_runtime_verified`,
  checks `85`, failures `0`, clips `280`, assets `6`.
- Completion audit: `status=review_ready_pending_user_acceptance`, proven `17`,
  pending `1`, failed `0`.
- Unit tests: `76` tests passed.

No GH2 ISO mount, game-time ISO path, game launch, or capture was used.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/target_graph_validator_childaim_decision.json`.

Goal remains open until direct visual approval passes.

## 2026-08-18 Control_Root basis sweep

Added a focused pre-staging diagnostic:

- Tool: `tools/gh3_midori_control_root_basis_sweep.py`
- Output: `analysis/gh3_midori_control_root_basis_sweep.json`

Purpose:

- Read the five retained GLB pose bridges.
- Compare simple pelvis-only `Control_Root` normalization candidates before
  ACP/MILO staging or game capture.
- Score each candidate by whether target pelvis child vectors
  (`Bone_Stomach_Lower`, `Bone_Thigh_L`, `Bone_Thigh_R`) point toward the
  mapped GLB/source positions.
- Feed the result into `tools/gh3_midori_glb_milo_route_gate.py`, so the route
  gate now proves the simple basis sweep is not promotable.

Result:

- `status=no_promotable_basis_candidate`
- cases: `5`
- promotable: `false`
- best aggregate candidate: `target_bind`
- best aggregate min direction dot: `-0.026473`
- best aggregate average direction dot: `0.03045`

Per-case best candidates disagree:

- medium idle: `control_root_local_axis_bind`, min best dot `-0.020001`
- attack left: `remove_root_base_pre_axis_bind`, min best dot `0.141709`
- fast jump: `remove_root_base_pre_axis_bind`, min best dot `0.153395`
- fast solo: `remove_root_base_pre_localz180`, min best dot `0.623638`
- transition out: `remove_root_base_pre_localz180`, min best dot `0.219721`

Decision:

Simple pelvis-only `Control_Root` basis removal/localz180 variants are not a
stable solution. The next solver should reconstruct a full target-graph
pelvis/torso/leg frame from GLB positions before writing ordinary GH2
`CharClipSamples`, rather than adding another constant pelvis-only matrix
policy or spending capture time on this family.

Verification, all run at Idle process priority:

- Python compile checks: passed.
- Basis sweep:
  `status=no_promotable_basis_candidate cases=5 best=target_bind min_dot=-0.026473 promotable=False`.
- Route gate:
  `status=glb_to_milo_route_guarded failures=0 glb_promotable=False`.
- Source anchors: `status=source_anchored`, anchors `22`.
- Pipeline verify: `status=guitar_hero_classic_midori_runtime_verified`,
  checks `85`, failures `0`, clips `280`, assets `6`.
- Completion audit: `status=review_ready_pending_user_acceptance`, proven `17`,
  pending `1`, failed `0`.
- Unit tests: `74` tests passed.

No GH2 ISO mount, game-time ISO path, game launch, or capture was used.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/control_root_basis_sweep_decision.json`.

Goal remains open until direct visual approval passes.

## 2026-08-18 GLB-to-MILO route gate

Added a fail-closed route gate:

- Tool: `tools/gh3_midori_glb_milo_route_gate.py`
- Output: `analysis/gh3_midori_glb_milo_route_gate.json`

Purpose:

- GLB remains acceptable as an automated intermediate.
- Final automated output must still be GH2 PS2 MILO
  `CharClipSet`/`CharClipSamples`.
- ihatecompvir `MiloLib`, `glTFMilo`, `milo_blender`, and GH2 notes remain
  format/reference inputs.
- Public `glTFMilo` is explicitly reference-only here, not a drop-in final
  GH2 PS2 guitarist animation converter.
- The unresolved static `Control_Root` basis / pelvis parent-local mismatch is
  retained as a required solver gap, so the rejected GLB/sourcepos branch cannot
  be treated as promotable just because ACP/MILO bytes are structurally valid.

Current route gate result:

- `status=glb_to_milo_route_guarded`
- `failure_count=0`
- `glb_intermediate_allowed=true`
- `gltfmilo_usage=reference_only_not_drop_in_final_converter`
- `ihatecompvir_reference_ready=true`
- `control_root_solver_required=true`
- `glb_solver_promotable=false`
- `current_route_status=axisalign_baseline_review_ready_glb_solver_not_promoted`

Wiring:

- `tools/gh3_midori_build_pipeline.py` final proof chain now regenerates the
  route gate before aggregate verification.
- `tools/gh3_midori_pipeline_verify.py` requires and checks the route gate.
- `tools/gh3_midori_review_packet.py` includes the route gate as review proof.
- `tools/gh3_midori_completion_audit.py` carries the user correction as a
  requirement-level item.
- `tools/gh3_midori_pipeline_test.py` has a direct synthetic test for the route
  policy and the production wiring.

Verification, all run at Idle process priority:

- Python compile checks: passed.
- Route gate:
  `status=glb_to_milo_route_guarded failures=0 glb_promotable=False`.
- Source anchors: `status=source_anchored`, anchors `21`.
- Pipeline verify: `status=guitar_hero_classic_midori_runtime_verified`,
  checks `85`, failures `0`, clips `280`, assets `6`.
- Review packet: `status=review_packet_ready`, failures `0`, proofs `8`.
- Gallery: `pose=9`, `bipedal=sequential_visual_bipedal_precheck_passed`,
  bytes `8,294`.
- Direct visual approval gate: `status=pending_user_visual_approval`,
  `approval_exists=false`, failures `0`.
- Completion audit: `status=review_ready_pending_user_acceptance`, proven `17`,
  pending `1`, failed `0`.
- Unit tests: `73` tests passed.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/glb_milo_route_gate_decision.json`.

No GH2 ISO mount, game-time ISO path, or runtime capture was used in this
update. Current playable/review baseline remains `matrix-local-axis-align-bind`;
the GLB solver is guarded and not promotable until the Control_Root/pelvis
solver and direct visual review are satisfied.

Goal remains open until direct visual approval passes.

## 2026-08-18 sequential visual approval order

Updated the direct visual approval gallery so the native pose frames render in
the same sequence as `analysis/gh3_midori_pose_bipedal_precheck.json`, not by
alphabetical `pose_review_files` key order. This makes the visual gate line up
with the sequential bipedal precheck and makes any non-bipedal frame an
immediate review rejection in order.

Updated files:

- `tools/gh3_midori_visual_approval_gallery.py`
- `tools/gh3_midori_pipeline_test.py`
- `analysis/GH3_MIDORI_VISUAL_APPROVAL.html`
- `analysis/gh3_midori_visual_approval_gallery.json`
- `analysis/gh3_midori_direct_visual_approval_gate.json`
- `analysis/gh3_midori_direct_visual_approval.template.json`
- `analysis/gh3_midori_pipeline_source_anchors.json`

Current gallery pose order:

1. `pose_midori_1_medium_idle_f060`
2. `pose_midori_1_attack_left_f030`
3. `pose_midori_1_fast_jump_f040`
4. `pose_midori_1_fast_solo_f090`
5. `pose_midori_1_transition_out_f050`
6. `pose_midori_1_accessory_acc01_f030`
7. `pose_midori_1_hand_overlay_f010`
8. `pose_midori_2_medium_idle_f060`
9. `pose_midori_2_attack_left_f030`

Verification, all run at Idle process priority:

- Python compile checks: passed.
- Gallery: `pose=9`,
  `bipedal=sequential_visual_bipedal_precheck_passed`, bytes `8,294`.
- Direct visual approval gate: `status=pending_user_visual_approval`,
  `approval_exists=false`, failures `0`.
- Source anchors: `status=source_anchored`, anchors `20`.
- Pipeline verify: `status=guitar_hero_classic_midori_runtime_verified`,
  checks `83`, failures `0`, clips `280`, assets `6`.
- Completion audit: `status=review_ready_pending_user_acceptance`, proven `16`,
  pending `1`, failed `0`.
- Unit tests: `72` tests passed.

Runtime/source policy carried forward:

- Do not mount or use the GH2 ISO at game time. Runtime proof remains local
  `gh2_ps2_hybrid_assets/GEN` plus loose `DLC/community.gh3.midori`.
- A GLB intermediate is acceptable only as part of an automated final path to
  GH2 PS2 MILO `CharClipSet`/`CharClipSamples`.
- The local ihatecompvir `glTFMilo`/`MiloLib`/GH2 notes are source references.
  The public `glTFMilo` CLI is not treated as a drop-in final GH2 PS2 guitarist
  animation converter.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/visual_gallery_sequential_pose_order_decision.json`.

Goal remains open until direct visual approval passes.

## 2026-08-18 direct approval acceptance-path tests

Added regression coverage for the approval gate's non-pending paths.

Updated:

- `tools/gh3_midori_pipeline_test.py`

Covered approval states:

- No approval artifact:
  `pending_user_visual_approval`
- Matching accepted approval artifact:
  `direct_user_visual_approval_accepted`
- Stale approval artifact hash:
  `failed`

Live retained state is intentionally still pending:

- Gate:
  `analysis/gh3_midori_direct_visual_approval_gate.json`
- Gate status:
  `pending_user_visual_approval`
- Approval artifact exists:
  `false`
- Gate failures:
  `0`

Current completion audit:

- Status: `review_ready_pending_user_acceptance`
- Proven: `16`
- Pending: `1`
- Failed: `0`

Verification, run at Idle process priority:

- Python compile checks: passed.
- Direct visual approval gate:
  `status=pending_user_visual_approval approval_exists=False failures=0`
- Completion audit:
  `status=review_ready_pending_user_acceptance proven=16 pending=1 failed=0`
- Unit tests:
  `71` tests passed.

No GH2 ISO mount, emulator run, or game runtime execution was used for this
approval acceptance-path test update.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/direct_visual_approval_acceptance_tests_decision.json`.

Goal remains open until direct visual approval passes.

## 2026-08-18 visual gallery approval boundary

Updated the local visual approval gallery so the direct approval boundary is
visible on the page being reviewed.

Artifacts:

- Gallery:
  `analysis/GH3_MIDORI_VISUAL_APPROVAL.html`
- Gallery manifest:
  `analysis/gh3_midori_visual_approval_gallery.json`
- Generator:
  `tools/gh3_midori_visual_approval_gallery.py`

The gallery now displays:

- `Direct approval`: `required before goal completion`
- Approval gate link:
  `analysis/gh3_midori_direct_visual_approval_gate.json`
- Approval template link:
  `analysis/gh3_midori_direct_visual_approval.template.json`

Current gallery manifest:

- Status: `review_packet_ready`
- Gameplay proofs: `2`
- Summary sheets: `3`
- Native pose frames: `9`
- Bipedal precheck:
  `sequential_visual_bipedal_precheck_passed`
- Direct visual approval required: `true`
- Gallery bytes: `8,240`

Verification, run at Idle process priority:

- Review packet:
  `status=review_packet_ready failures=0 proofs=7`
- Visual approval gallery:
  `gallery=analysis\GH3_MIDORI_VISUAL_APPROVAL.html gameplay=2 sheets=3 pose=9 bipedal=sequential_visual_bipedal_precheck_passed bytes=8240`
- Direct visual approval gate:
  `status=pending_user_visual_approval approval_exists=False failures=0`
- Source anchors:
  `status=source_anchored anchors=20`
- Pipeline verify:
  `status=guitar_hero_classic_midori_runtime_verified checks=83 failures=0 clips=280 assets=6`
- Completion audit:
  `status=review_ready_pending_user_acceptance proven=16 pending=1 failed=0`
- Unit tests:
  `69` tests passed.

No GH2 ISO mount, emulator run, or game runtime execution was used for this
gallery approval-boundary update.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/visual_gallery_approval_boundary_decision.json`.

Goal remains open until direct visual approval passes.

## 2026-08-18 direct visual approval gate

Added an explicit approval gate so completion is no longer represented only by
the aggregate `review_ready_pending_user_acceptance` status.

Artifacts:

- Gate:
  `tools/gh3_midori_direct_visual_approval_gate.py`
- Gate output:
  `analysis/gh3_midori_direct_visual_approval_gate.json`
- Template:
  `analysis/gh3_midori_direct_visual_approval.template.json`
- Expected approval file, when user approval is given:
  `analysis/gh3_midori_direct_visual_approval.json`

The approval gate fingerprints the current:

- Review packet:
  `analysis/gh3_midori_review_packet.json`
- Visual approval gallery:
  `analysis/GH3_MIDORI_VISUAL_APPROVAL.html`
- Sequential bipedal precheck:
  `analysis/gh3_midori_pose_bipedal_precheck.json`
- Runtime input guard:
  `analysis/gh3_midori_runtime_input_guard.json`

Current gate result:

- Status: `pending_user_visual_approval`
- Approval artifact exists: `false`
- Failures: `0`

Completion audit now has a separate pending item:

- "Obtain direct user visual approval for the current review packet and
  gallery."

Current retained completion state:

- Status: `review_ready_pending_user_acceptance`
- Proven: `16`
- Pending: `1`
- Failed: `0`

Verification, run at Idle process priority:

- Pipeline verify:
  `status=guitar_hero_classic_midori_runtime_verified checks=83 failures=0 clips=280 assets=6`
- Review packet:
  `status=review_packet_ready failures=0 proofs=7`
- Visual approval gallery:
  `gallery=analysis\GH3_MIDORI_VISUAL_APPROVAL.html gameplay=2 sheets=3 pose=9 bipedal=sequential_visual_bipedal_precheck_passed bytes=7870`
- Direct visual approval gate:
  `status=pending_user_visual_approval approval_exists=False failures=0`
- Source anchors:
  `status=source_anchored anchors=20`
- Completion audit:
  `status=review_ready_pending_user_acceptance proven=16 pending=1 failed=0`
- Unit tests:
  `69` tests passed.

No GH2 ISO mount, emulator run, or game runtime execution was used for this
approval-gate update.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/direct_visual_approval_gate_decision.json`.

Goal remains open until direct visual approval passes.

## 2026-08-18 local-runtime input guard

Added an explicit guard for the user's GH2 ISO concern: current Midori runtime
and visual proofs must use local `gh2_ps2_hybrid_assets/GEN` plus loose
`gh2_ps2_hybrid_assets/DLC`, not a game-time GH2 ISO path.

Artifacts:

- Guard:
  `tools/gh3_midori_runtime_input_guard.py`
- Guard output:
  `analysis/gh3_midori_runtime_input_guard.json`
- Refreshed hybrid runtime manifest:
  `analysis/gh3_midori_hybrid_runtime_deploy_manifest.json`

Current guard result:

- Status: `local_gen_loose_dlc_runtime_inputs_verified`
- Failures: `0`
- App: `gh2_ps2_hybrid_assets\ghogx_app.exe`
- Base archive dir: `gh2_ps2_hybrid_assets\GEN`
- Addons dir: `gh2_ps2_hybrid_assets\DLC`
- Forbidden shipped DLC files with suffix `.iso`, `.ark`, `.hdr`, `.hed`, or
  `.wad`: `0`

Integration:

- `gh3_midori_pipeline_verify.py` now requires
  `runtime_inputs_are_local_gen_and_loose_dlc_not_iso`.
- `gh3_midori_review_packet.py` now includes `runtime_input_guard` as a gate
  and proof link.
- `gh3_midori_completion_audit.py` now has a separate proven item:
  "Use local GH2 GEN plus loose DLC for current runtime proofs; do not require
  a GH2 ISO mount at game time."
- `gh3_midori_build_pipeline.py` now runs `runtime_input_guard` before
  `pipeline_verify`.
- `gh3_midori_deploy_hybrid_runtime.py` now documents local-GEN runtime proof
  policy and accepts base archive filenames case-insensitively.

Verification, run at Idle process priority:

- Hybrid runtime verify-only:
  `status=hybrid_runtime_app_deployed bytes=6313472 sha256=5670ceb4ca856d08e79218f09d27b8857a800e02bb30bf68f163dbc8f87f0a1a`
- Runtime input guard:
  `status=local_gen_loose_dlc_runtime_inputs_verified failures=0 app=gh2_ps2_hybrid_assets\ghogx_app.exe ark_dir=gh2_ps2_hybrid_assets\GEN addons_dir=gh2_ps2_hybrid_assets\DLC`
- Source anchors:
  `status=source_anchored anchors=19`
- Pipeline verify:
  `status=guitar_hero_classic_midori_runtime_verified checks=83 failures=0 clips=280 assets=6`
- Review packet:
  `status=review_packet_ready failures=0 proofs=7`
- Completion audit:
  `status=review_ready_pending_user_acceptance proven=16 pending=0 failed=0`
- Unit tests:
  `68` tests passed.

No GH2 ISO mount, emulator run, or game runtime execution was used. The hybrid
runtime manifest was refreshed in verify-only mode against local
`gh2_ps2_hybrid_assets/GEN`.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/runtime_input_guard_decision.json`.

Goal remains open until direct visual approval passes.

## 2026-08-18 pipeline visual-review automation

Updated the canonical build pipeline so the final review/proof surface is
regenerated automatically instead of relying on manual follow-up commands.

Updated:

- `tools/gh3_midori_build_pipeline.py`
- `tools/gh3_midori_pipeline_test.py`
- `analysis/gh3_midori_pipeline_source_anchors.json`

New automated final proof steps in `gh3_midori_build_pipeline.py`:

- `pose_bipedal_precheck`
- `pipeline_verify`
- `review_packet`
- `visual_approval_gallery`
- `completion_audit`

Source anchors:

- Source-anchor status: `source_anchored`
- Anchor count: `18`
- Added source anchors:
  - `midori_pose_bipedal_precheck`
  - `midori_visual_approval_gallery`
- Resolved MILO converter anchor:
  `GuitarHeroOGX-main-ui-engine/tools/milo_convert/out/build/win-amd64-release/Release/milo_convert_tool.exe`

Verification, run at Idle process priority:

- Source anchors:
  `status=source_anchored anchors=18`
- Pipeline verify:
  `status=guitar_hero_classic_midori_runtime_verified checks=81 failures=0 clips=280 assets=6`
- Review packet:
  `status=review_packet_ready failures=0 proofs=6`
- Visual approval gallery:
  `gallery=analysis\GH3_MIDORI_VISUAL_APPROVAL.html gameplay=2 sheets=3 pose=9 bipedal=sequential_visual_bipedal_precheck_passed bytes=7870`
- Completion audit:
  `status=review_ready_pending_user_acceptance proven=15 pending=0 failed=0`
- Unit tests:
  `66` tests passed.

No GH2 ISO mount, emulator run, or game runtime execution was used for this
automation update. Existing source input hashes were preserved instead of
re-reading the GH3 ISO.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/pipeline_visual_review_autowire_decision.json`.

Goal remains open until direct visual approval passes.

## 2026-08-18 visual approval gallery bipedal notes

Updated the local visual approval gallery so it no longer shows the retained
pose screenshots without the sequential bipedal verdict context.

Artifacts:

- Gallery:
  `analysis/GH3_MIDORI_VISUAL_APPROVAL.html`
- Gallery manifest:
  `analysis/gh3_midori_visual_approval_gallery.json`
- Generator:
  `tools/gh3_midori_visual_approval_gallery.py`

Current gallery manifest:

- Status: `review_packet_ready`
- Gameplay proofs: `2`
- Summary sheets: `3`
- Native pose frames: `9`
- Bipedal precheck status:
  `sequential_visual_bipedal_precheck_passed`
- Bipedal precheck proofs: `9`
- Bipedal precheck verdicts: `9`
- Bipedal precheck failures: `0`
- Gallery bytes: `7,870`

The gallery now displays:

- Top-level bipedal precheck status/counts.
- Per-frame `pass` badges.
- Per-frame notes from
  `analysis/gh3_midori_pose_bipedal_precheck.json`.

Verification, run at Idle process priority:

- Python compile checks: passed.
- Gallery regeneration:
  `gallery=analysis\GH3_MIDORI_VISUAL_APPROVAL.html gameplay=2 sheets=3 pose=9 bipedal=sequential_visual_bipedal_precheck_passed bytes=7870`.
- Unit tests:
  `66` tests passed.
- Completion audit:
  `status=review_ready_pending_user_acceptance proven=15 pending=0 failed=0`.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/visual_gallery_bipedal_notes_decision.json`.

No GH2 ISO mount, emulator run, or game runtime execution was used for this
gallery update.

Goal remains open until direct visual approval passes.

## 2026-08-18 sequential bipedal visual precheck

Added a per-frame visual precheck so the current review state cannot be reduced
to "one coherent frame in the contact sheet."

Artifacts:

- Manual per-frame verdicts:
  `analysis/gh3_midori_pose_bipedal_manual_verdicts.json`
- Generated validator output:
  `analysis/gh3_midori_pose_bipedal_precheck.json`
- Validator:
  `tools/gh3_midori_pose_bipedal_precheck.py`

Scope:

- This is a Codex sequential visual bipedal precheck across all `9` retained
  native pose-review screenshots.
- It is not direct user visual approval.
- Goal remains open until user visual review accepts the result.

Per-frame criteria:

- `bipedal_silhouette`
- `upright_or_pose_readable_torso_pelvis`
- `coherent_limbs`
- `not_folded_nonhumanoid`
- `guitar_attached_when_visible`

Current result:

- Precheck status: `sequential_visual_bipedal_precheck_passed`
- Proofs: `9`
- Verdicts: `9`
- Failures: `0`

Pipeline/review integration:

- `gh3_midori_pipeline_verify.py` now requires
  `sequential_pose_bipedal_precheck_passed`.
- `gh3_midori_review_packet.py` now includes
  `pose_bipedal_precheck` as a gate and proof link.
- `gh3_midori_completion_audit.py` now has a separate proven item:
  "Retain a sequential bipedal visual precheck for every current native
  pose-review frame."

Verification, run at Idle process priority:

- Python compile checks: passed.
- Precheck:
  `status=sequential_visual_bipedal_precheck_passed proofs=9 failures=0`.
- Pipeline verify:
  `status=guitar_hero_classic_midori_runtime_verified checks=81 failures=0 clips=280 assets=6`.
- Review packet:
  `status=review_packet_ready failures=0 proofs=6`.
- Completion audit:
  `status=review_ready_pending_user_acceptance proven=15 pending=0 failed=0`.
- Unit tests:
  `65` tests passed.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/sequential_bipedal_precheck_decision.json`.

No GH2 ISO mount, emulator run, or game runtime execution was used for this
precheck update.

Goal remains open until direct visual approval passes.

## 2026-08-18 ihatecompvir GLB-to-MILO bridge audit

Added a repeatable source audit:

- Tool:
  `tools/gh3_midori_ihatecompvir_bridge_audit.py`
- Audit output:
  `analysis/gh3_midori_ihatecompvir_bridge_audit.json`

Result:

- Status: `reference_ready_direct_writer_needed`
- `glTFMilo` drop-in GH2 PS2 CharClip converter: `false`
- MiloLib/GH2 CharClip reference ready: `true`

Interpretation:

- GLB is acceptable as an automated intermediate input.
- The public `glTFMilo` CLI targets Xbox/PS3 RB-era `TransAnim` output and is
  not the final GH2 PS2 guitarist `CharClipSamples` converter.
- The usable ihatecompvir path is MiloLib plus GH2 notes as the reference for a
  direct GH2 PS2 `CharClipSet`/`CharClipSamples` writer or validator.
- Final automated output must remain ordinary GH2 PS2 MILO files inside loose
  ark-external DLC.

Verification, run at Idle priority:

- `python -m py_compile tools/gh3_midori_ihatecompvir_bridge_audit.py tools/gh3_midori_pipeline_test.py`
  passed.
- `python tools/gh3_midori_ihatecompvir_bridge_audit.py --print-summary` ->
  `status=reference_ready_direct_writer_needed gltfmilo_drop_in=False milolib_reference=True`.
- `python -m unittest tools.gh3_midori_pipeline_test` -> `63` tests passed.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/ihatecompvir_bridge_audit_decision.json`.

No GH2 ISO mount or game runtime was used for this audit.

Goal remains open until direct visual approval passes.

## 2026-08-18 local visual approval gallery

Added a local HTML gallery for direct visual review:

- Gallery:
  `analysis/GH3_MIDORI_VISUAL_APPROVAL.html`
- Gallery manifest:
  `analysis/gh3_midori_visual_approval_gallery.json`
- Source packet:
  `analysis/gh3_midori_review_packet.json`

The gallery contains:

- In-song gameplay proofs: `2`
- Summary/contact sheets: `3`
- Native pose-review frames: `9`

Regenerated and verified at Idle process priority:

- Python compile checks: passed.
- Pipeline unit tests: `62` tests passed.
- Review packet: `status=review_packet_ready`, failures `0`, proofs `5`.
- Gallery: gameplay `2`, sheets `3`, pose frames `9`, bytes `5,623`.
- Completion audit: `status=review_ready_pending_user_acceptance`, proven `14`,
  pending `0`, failed `0`.

Important runtime/source note:

- Current runtime proof is from local `gh2_ps2_hybrid_assets/GEN` plus loose
  `DLC/community.gh3.midori`.
- The GH2 ISO is not needed at game time and must not be mounted for normal
  Midori runtime proof.
- `glTFMilo`/ihatecompvir sources are useful as MILO/CharClip references, but
  the available CLI is not a direct GH2 PS2 guitarist `CharClipSamples`
  animation converter. A GLB bridge is acceptable only if the final automated
  path emits valid GH2 PS2 MILO/CharClip data.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/visual_approval_gallery_decision.json`.

Goal remains open until direct visual approval passes.

## 2026-08-18 direct visual approval packet

Added a compact current pose-review contact sheet:

- Image:
  `analysis/gh3_midori_pose_review_axisalign_redeploy_contact_sheet.jpg`
- Manifest:
  `analysis/gh3_midori_pose_review_axisalign_redeploy_contact_sheet.json`
- Proof count: `9`
- Byte count: `91,002`

Updated `tools/gh3_midori_review_packet.py` so the retained review packet now
contains:

- Current gameplay screenshots for `gh3_midori_1` and `gh3_midori_2`.
- The pose-review contact sheet.
- Motion contact sheet and stock scale reference.
- All nine individual current native pose-review frames under
  `pose_review_files`.

Regenerated:

- `analysis/gh3_midori_review_packet.json`
- `analysis/GH3_MIDORI_REVIEW_PACKET.md`
- `analysis/gh3_midori_completion_audit.json`

Verification:

- Review packet: `status=review_packet_ready`, failures `0`, top-level proofs
  `5`, pose-review frames `9`.
- Completion audit: `status=review_ready_pending_user_acceptance`, proven `14`,
  pending `0`, failed `0`.
- Unit tests: `python -m unittest tools.gh3_midori_pipeline_test` -> `62`
  tests OK.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/direct_visual_approval_packet_decision.json`.

Goal remains open until direct visual approval passes.

## 2026-08-18 target-graph scope and aim-correction sweep

Broadened the isolated validator instead of staging another visual candidate:

- Tool: `tools/gh3_midori_target_graph_solve_validator.py`
- Official output:
  `analysis/gh3_midori_target_graph_solve_validator_report.json`
- Production proof command now includes:
  `--hmx-quat-mode all --child-aim-convention best --graph-scope target_graph`

Scope changes:

- Added explicit graph scopes: `lower_body` and `target_graph`.
- `target_graph` validates `13` mapped GH2 target bones:
  pelvis, torso chain through neck, both legs, and both toes.
- The production route gate now requires the fuller `target_graph` report,
  source/child-aim counts, and explicit aim/reconstruction convention fields.

Additional sweep surface:

- World rotation reconstruction: `local_parent`, `parent_local`
- Position reconstruction: `parent_local`, `parent_t_local`
- Child-aim correction: `none`, `row_pre`, `row_post_t`,
  `transpose_pre`, `transpose_post_t`

Current official result:

- `status=no_valid_target_graph_solve`
- `passing_count=0`
- Best candidate:
  `desired_local_t/bind/transpose/best/transpose_post_t/parent_t_world/local_parent/parent_t_delta/parent_local`
- `source_count=13`
- `segment_count=12`
- `child_aim_count=10`
- `max_position_error=0.0`
- `min_segment_dot=1.0`
- `min_selected_child_aim_dot=-0.717223`
- `min_transpose_child_aim_dot=-0.960249`
- `max_rotation_roundtrip_error_degrees=179.129689`

Decision:

- The previous seven-bone lower-body graph was too narrow to validate
  pelvis-to-torso aim, so it is no longer sufficient production evidence.
- The fuller torso/leg/feet graph still rejects the solver. Segment positions
  can be perfect, but child aim remains negative after rotation roundtrip.
- Simple per-bone aim correction improves the best child-aim score from
  `-0.899642` to `-0.717223`, but it is still not bipedal/promotable.
- Next work should change hierarchical target-frame construction feeding
  pelvis, torso, legs, and `Control_Root`, not repeat constant basis,
  matrix-order, or per-bone aim-fix variants.

Verification, all run at Idle process priority:

- Python compile checks: passed.
- Target-graph validator:
  `status=no_valid_target_graph_solve passing=0 graph=target_graph best_child_aim=-0.717223 best_rot=179.129689`.
- Route gate:
  `status=glb_to_milo_route_guarded failures=0 glb_promotable=False`.
- Source anchors: `status=source_anchored`, anchors `22`, missing `0`.
- Pipeline verify: `status=guitar_hero_classic_midori_runtime_verified`,
  checks `85`, failures `0`, clips `280`, assets `6`.
- Review packet: `status=review_packet_ready`, failures `0`, proofs `8`.
- Completion audit: `status=review_ready_pending_user_acceptance`, proven `17`,
  pending `1`, failed `0`.
- Unit tests: `77` tests passed.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/target_graph_validator_scope_aim_sweep_decision.json`.

No GH2 ISO mount, game-time ISO path, runtime launch, or visual capture was
used for this diagnostic turn.

Goal remains open until direct visual approval passes.

## 2026-08-18 quat roundtrip fix and staging bridge gate

Fixed an important validator bug:

- `tools/gh3_midori_target_graph_solve_validator.py` now models the writer's
  actual quaternion path: internal WXYZ helper output is reordered to XYZW
  before `packed_xyzw_to_row_matrix`.
- Added a regression test proving identity survives validator packed roundtrip.
- The fuller target graph now has valid candidates instead of the previous
  false 179-degree roundtrip rejection.

Current official validator result:

- `analysis/gh3_midori_target_graph_solve_validator_report.json`
- `status=target_graph_solve_valid`
- `graph_scope=target_graph`
- `passing_count=88`
- Best candidate:
  `local_desired_t/bind/transpose/best/row_pre/world_parent_t/local_parent/parent_t_delta/parent_local`
- `max_position_error=0.0`
- `min_segment_dot=1.0`
- `min_selected_child_aim_dot=1.0`
- `max_rotation_roundtrip_error_degrees=0.021455`

Added a matching staging policy:

`matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-valid-rowaim-bakepos-bind`

Also fixed:

- `tools/gh3_midori_acp_stage.py`
  `target_solved_skeleton_packed_world_rotation` diagnostic now uses the same
  WXYZ-to-XYZW storage conversion.
- `tools/gh3_midori_pose_report.py` tracks transpose, row, and best child-aim
  convention for bridge-gate diagnostics.

Structural build attempt:

- Ran the pipeline at Idle priority with the new staging policy,
  `--skip-runtime-proof`, `--skip-capture`, and `--skip-runtime-deploy`.
- No GH2 ISO mount, game-time ISO path, runtime launch, or visual capture was
  used.
- The pre-staging bridge biped gate rejected before final rebuild/deploy.

Current staging bridge gate:

- `analysis/gh3_midori_bridge_gate_report.json`
- `status=reject`
- policy:
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-valid-rowaim-bakepos-bind`
- `case_count=5`
- `max_pose_error_max=55.182211`
- `min_right_shin_dot=0.963904`
- `min_left_shin_dot=0.929721`
- `min_child_aim_dot=-0.561928`

Interpretation:

- The validator bug was real and is fixed.
- The isolated target graph is valid.
- The staging candidate is still rejected before deployment: leg direction is
  now good, but pelvis/torso/toe reconstruction still breaks the bridge gate.
- A quick attempt to emit toe `.pos` through the existing target-skeleton bake
  position branch made toe pose error worse and was reverted.
- Next work should keep the valid target-graph quaternion path, then solve the
  staging bridge mismatch for pelvis/torso/toe positions/aim before any runtime
  capture.

Route gate now separates these states:

- `analysis/gh3_midori_glb_milo_route_gate.json`
- `status=glb_to_milo_route_guarded`
- `failure_count=0`
- `current_route_status=target_graph_solver_valid_staging_bridge_gate_reject`
- `glb_solver_promotable=false`

Verification, all run at Idle process priority:

- Python compile checks: passed.
- Pipeline verify: `status=guitar_hero_classic_midori_runtime_verified`,
  checks `85`, failures `0`, clips `280`, assets `6`.
- Review packet: `status=review_packet_ready`, failures `0`, proofs `8`.
- Completion audit: `status=review_ready_pending_user_acceptance`, proven `17`,
  pending `1`, failed `0`.
- Unit tests: `78` tests passed.

Evidence:
`.codex/current-evidence/midori-review-source-bridges-20260818/quat_roundtrip_target_graph_staging_decision.json`.

Goal remains open until direct visual approval passes.

## Current resume pointer

Resume at the staging bridge mismatch after the target-graph quaternion
roundtrip fix, not another visual capture batch. Latest state:

- `analysis/gh3_midori_target_graph_solve_validator_report.json`
- `status=target_graph_solve_valid`, `passing_count=88`
- Best isolated candidate:
  `local_desired_t/bind/transpose/best/row_pre/world_parent_t/local_parent/parent_t_delta/parent_local`
- `analysis/gh3_midori_bridge_gate_report.json`
- staging bridge gate still rejects:
  `max_pose_error_max=55.182211`, `min_child_aim_dot=-0.561928`
- `analysis/gh3_midori_glb_milo_route_gate.json`
- `current_route_status=target_graph_solver_valid_staging_bridge_gate_reject`

GLB is acceptable as an automated intermediate, including a GLB > MILO bridge,
but final output must be automated GH2 PS2 `CharClipSet`/`CharClipSamples` in
loose ark-external DLC. ihatecompvir/MiloLib sources are reference inputs for
that writer/validator path; the current public `glTFMilo` CLI is not a drop-in
GH2 PS2 guitarist converter.

Do not mount or run from the GH2 ISO at game time. Run heavy proof commands at
Idle/low CPU priority. Goal remains open until direct visual approval passes.

## 2026-08-18 target-graph solver promoted through staged MILO bridge

Resume at direct visual review/capture of the newly deployed valid-rowaim build,
not at Control_Root or pelvis-only matrix-local diagnosis. The solver gap has
been closed through staged ACP and GH2 PS2 MILO packing.

- Active policy:
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-valid-rowaim-bakepos-bind`
- `analysis/gh3_midori_bridge_gate_report.json`
  `gate.status=pass`, `case_count=5`,
  `max_pose_error_avg=0.034703`, `max_pose_error_max=0.173513`,
  `min_right_shin_dot=0.963904`, `min_left_shin_dot=0.929721`,
  `min_child_aim_dot=1.0`
- `analysis/gh3_midori_acp_disk_bridge_gate_report.json`
  `gate.status=pass`, `case_count=5`,
  `max_pose_error_avg=0.000001`, `max_pose_error_max=0.000002`,
  `min_right_shin_dot=1.0`, `min_left_shin_dot=1.0`,
  `min_child_aim_dot=1.0`
- `analysis/gh3_midori_glb_milo_route_gate.json`
  `status=glb_to_milo_route_guarded`,
  `current_route_status=target_graph_solver_promotable_pending_visual_approval`,
  `failure_count=0`
- Full structural pipeline succeeded at Idle priority with
  `--skip-runtime-proof --skip-capture --skip-runtime-deploy`. It rebuilt 331
  staged ACP clips, packed GH2 MILOs, packaged engine DLC, and deployed loose
  DLC to `gh2_ps2_hybrid_assets\DLC\community.gh3.midori`.
- `analysis/gh3_midori_pipeline_verification.json`
  `status=guitar_hero_classic_midori_runtime_verified`, `checks=85`,
  `failures=0`, `clips=280`, `assets=6`
- `analysis/gh3_midori_completion_audit.json`
  `status=review_ready_pending_user_acceptance`, `proven=17`, `pending=1`,
  `failed=0`

The GLB > MILO route is automated through the target graph source bridge,
staged ACP, then the GH2 `CharClipSet`/`CharClipSamples` MILO writer. Use
ihatecompvir/MiloLib code as reference for format behavior; public `glTFMilo`
is still reference-only here, not a direct GH2 PS2 guitarist converter.
Use the full local source tree at `ihatecompvir-public-milo-sources` for that
reference. The engine vendor copy under
`GuitarHeroOGX-main-ui-engine\third_party\ihatecompvir-public-milo-sources` is
partial and does not include the same CharClip/CharClipSet reference files.

Next proof must use local `gh2_ps2_hybrid_assets\GEN` plus loose DLC, not the
GH2 ISO. Run emulator/proof processes at Idle/low CPU priority. Reject any
non-bipedal capture immediately; only direct visual approval closes the goal.

## 2026-08-18 fresh valid-rowaim visual reject

Do not request direct visual approval for the current `valid-rowaim` deployed
build. Fresh low-priority proof used only local
`gh2_ps2_hybrid_assets\GEN` plus loose `gh2_ps2_hybrid_assets\DLC`; no GH2 ISO
was mounted or used at game time.

- Fresh gameplay proof:
  `analysis/gh3_midori_ghc_gameplay_proofs/gameplay_proof_manifest.json`
  `status=in_song_midori_variant_animation_verified`, `failure_count=0`
- Fresh native pose review:
  `analysis/gh3_midori_pose_review_proofs/pose_review_proof_manifest.json`
  script status passed mechanically, but direct sequential visual inspection
  rejected 7 of 9 frames as sideways/contorted rather than readable bipeds.
- Updated manual visual gate:
  `analysis/gh3_midori_pose_bipedal_manual_verdicts.json`
  and `analysis/gh3_midori_pose_bipedal_precheck.json`
  now report `status=failed`, `failure_count=35`.
- Aggregates intentionally red:
  `analysis/gh3_midori_pipeline_verification.json`
  `status=failed`, one failure at
  `sequential_pose_bipedal_precheck_passed`;
  `analysis/gh3_midori_completion_audit.json`
  `status=failed`.
- Decision record:
  `.codex/current-evidence/midori-review-source-bridges-20260818/valid_rowaim_fresh_visual_reject_decision.json`

Diagnosis from the fresh failure:

- `Control_Root` rotation is effectively static in the failing main clips.
- `Bone_Pelvis` carries the main motion and is authored under `Control_Root`
  in GH3, while target `bone_pelvis` is root-level in GH2.
- Sparse accessory/hand-overlay clips can look upright because they do not
  exercise the failing pelvis motion channel.
- The existing `axislocal` followup candidate was tested structurally and
  rejected before deploy: `max_pose_error_max=6.068832`,
  `min_child_aim_dot=-0.129498`.

Next resume point: continue with a pelvis/root-space candidate that bakes or
removes the static `Control_Root` basis consistently across pelvis, child
rotations, and positions before writing root-level GH2 `bone_pelvis` channels.
Visual bipedal review is the first post-build gate; any non-bipedal frame is an
immediate reject.

## 2026-08-18 pelvisbase candidate rejected

Implemented and tested:
`matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-valid-rowaim-pelvisbase-bakepos-bind`.
This is the `valid-rowaim` path with `Bone_Pelvis` allowed to use the animated
base/foldroot rotation instead of falling back to bind rotation.

Results:

- In-memory bridge gate passed:
  `max_pose_error_avg=0.034705`, `max_pose_error_max=0.173513`,
  `min_child_aim_dot=1.0`
- ACP disk bridge gate passed:
  `max_pose_error_avg=0.000003`, `max_pose_error_max=0.000004`,
  `min_child_aim_dot=1.0`
- Full automated GLB/source bridge > ACP > GH2 MILO > loose DLC deploy
  succeeded at Idle priority, no game-time ISO.
- Fresh native pose review from local `GEN` plus loose `DLC` failed visually.
  Direct inspection rejected 7 of 9 frames as curled/inverted; only
  `midori_1_accessory_acc01_f030` and `midori_1_hand_overlay_f010` were
  bipedal.
- `analysis/gh3_midori_pose_bipedal_precheck.json` now reports
  `status=failed`, `failure_count=37`.
- `analysis/gh3_midori_pipeline_verification.json` now reports `status=failed`
  with one failure: `sequential_pose_bipedal_precheck_passed`.
- Axisblend35/50/65 were probed structurally and all rejected with
  `max_pose_error_max≈6.068832` and `min_child_aim_dot=-0.129498`.
- Decision record:
  `.codex/current-evidence/midori-review-source-bridges-20260818/valid_rowaim_pelvisbase_visual_reject_decision.json`

Conclusion: animated root-level GH2 `bone_pelvis` alone is not the missing
transform and makes the visual result worse. Next candidate should keep pelvis
visually upright while applying the missing static `Control_Root` basis to
descendant child frames/positions consistently.

ihatecompvir source status:

- `python tools\gh3_midori_ihatecompvir_bridge_audit.py --source-root
  ihatecompvir-public-milo-sources --print-summary` reports
  `status=reference_ready_direct_writer_needed`, `gltfmilo_drop_in=False`,
  `milolib_reference=True`.
- This means GLB is acceptable as an automated intermediate, but final output
  still must be the existing GH2 PS2 `CharClipSet`/`CharClipSamples` MILO path
  or a direct replacement validated against MiloLib's GH2 layout.

## 2026-08-18 root-basis descendant probes rejected

Added two tightly scoped candidate policies that keep GH2 `bone_pelvis` on the
upright/bind fallback and apply `Control_Root` basis only to descendant
source-position vectors:

- `...valid-rowaim-rootlocal-bakepos-bind`: uses `transpose(Control_Root)`.
  Bridge gate rejected before deploy: `max_pose_error_max=33.093382`,
  `min_right_shin_dot=-0.487684`, `min_child_aim_dot=-0.619203`.
- `...valid-rowaim-rootbasis-bakepos-bind`: uses the opposite basis direction.
  Bridge gate rejected before deploy: `max_pose_error_max=33.753904`,
  `min_right_shin_dot=-0.682504`, `min_child_aim_dot=-0.768708`.

Decision record:
`.codex/current-evidence/midori-review-source-bridges-20260818/root_basis_descendant_probe_decision.json`

Do not deploy or capture either root-basis descendant candidate. The
Control_Root-only vector-basis explanation is now unlikely. Resume by auditing
the packed GH2 quaternion/matrix convention or a root-level display basis
mismatch in GHC: `valid-rowaim` and `pelvisbase` can pass the bridge math yet
look non-bipedal natively, so the failing surface is likely after the internal
pose report's reconstruction convention.

## 2026-08-18 output-graph/current-rig probe

Added `tools/gh3_midori_output_graph_diagnostic.py` to simulate the
converter-generated GH2 `CharBone` output graph and GHC's `.trans -> .mesh ->
base` target resolver against `analysis/gh3_midori_current_midori1_rig.json`.

Findings:

- The generated guitar-main output graph has 73 contexts and 10 lower-body
  contexts. Lower-body parent resolution is clean: channels such as
  `bone_L-thigh` resolve to `bone_L-thigh.mesh` with the expected
  `.mesh` parent chain.
- There are 10 duplicate stripped output keys where both `.trans` and `.mesh`
  outputs collapse to the same GHC key, including `bone_pelvis`,
  `bone_spine1/2/3`, and upper-arm anchors. GHC's `by_key` map keeps the later
  `.mesh` entry, so `bone_pelvis.quat/pos` is applied to
  `bone_pelvis.mesh`.
- This explains why the native render path must be reasoned about in the
  visible `.mesh` pelvis basis, not only the bare `bone_pelvis` /
  `Control_Root` chain.

Rejected probe:

- A whole-animation-bind swap to
  `analysis/gh3_midori_current_midori1_rig.json` was tested structurally with
  `valid-rowaim`.
- One med-idle disk case packed and decoded cleanly, with
  `bone_pelvis.pos ~= [-0.010, 0.007, 46.180]` instead of the rejected live
  `pelvisbase` sample's stock-rig-ish `z ~= 38`.
- The full in-memory bridge gate rejected before build/deploy:
  `min_right_shin_dot=-0.963903`, `min_left_shin_dot=-0.929721`.
- Decision record:
  `.codex/current-evidence/midori-review-source-bridges-20260818/output_graph_currentrig_probe_decision.json`

Do not deploy the whole current-rig bind candidate. Next candidate should be
pelvis-only and runtime-aware: keep the stock/bridge target rig and
`valid-rowaim` descendants, but pack GH2 `Bone_Pelvis` for the GHC-resolved
`bone_pelvis.mesh`/`Control_Root` local basis instead of animated pelvisbase or
the bare bind fallback.

## 2026-08-18 pelvismeshrot + toe-bake candidate rejected visually

Implemented and tested:
`matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-valid-rowaim-pelvismeshrot-bakepos-bind`.
This uses the GHC-resolved `bone_pelvis.mesh` bind rotation for pelvis solving
and keeps/bakes `Bone_Toe_L/R.pos` channels instead of stripping them under the
valid-rowaim path.

Results:

- First `pelvismesh` variant, with pelvis mesh position and rotation, rejected
  structurally before deploy: `max_pose_error_max=13.720043`.
- Rotation-only `pelvismeshrot` without toe bake rejected structurally before
  deploy: `max_pose_error_max=6.059526`; pelvis/thigh/knee/ankle were near
  zero error, but toe endpoints were ~6 units off.
- After adding toe-position bake for `pelvismeshrot`, the bridge gate passed:
  `max_pose_error_avg=0.034703`, `max_pose_error_max=0.173513`,
  `min_right_shin_dot=0.963904`, `min_left_shin_dot=0.929721`,
  `min_child_aim_dot=1.0`.
- Full automated ACP staging, GH2 `CharClipSamples` MILO build, package, and
  loose DLC deploy completed at Idle priority using local `GEN` + loose `DLC`
  only; no game-time ISO path.
- Fresh native pose review from
  `analysis/gh3_midori_pose_review_pelvismeshrot_toebake_proofs` captured 9
  frames and failed automatically only on framing margins, but sequential
  visual inspection rejected the candidate: anatomy is coherent, yet several
  normal/performance samples are whole-character sideways, including
  `midori_1_medium_idle_f060` and `midori_2_medium_idle_f060`.
- Decision record:
  `.codex/current-evidence/midori-review-source-bridges-20260818/pelvismeshrot_toebake_visual_reject_decision.json`

Conclusion: toe baking fixed the structural endpoint problem, but this is not
direct visual approval. The remaining failure is a runtime-visible pelvis/root
display basis: `Control_Root` rotations are not staged for guitar-main clips,
and the generated output graph resolves `bone_pelvis.trans` channels to visible
`bone_pelvis.mesh`, so the next candidate must keep the coherent descendant
solve while correcting the emitted root-level pelvis display rotation.

## 2026-08-18 pelvis mesh display-order probes rejected

Recorded the skipped-gate `pelvismeshinvrot` visual probe:
`matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-valid-rowaim-pelvismeshinvrot-bakepos-bind`.
Its structural bridge gate failed (`max_pose_error_avg=13.667528`,
`max_pose_error_max=28.249728`, `min_child_aim_dot=-0.043909`), but it was
built once for direct runtime diagnosis at Idle priority using local
`gh2_ps2_hybrid_assets/GEN` plus loose `gh2_ps2_hybrid_assets/DLC`; no
game-time ISO was used. The automatic native framing review passed 9/9, but
sequential visual inspection rejected the candidate because both medium-idle
captures remained sideways. Decision record:
`.codex/current-evidence/midori-review-source-bridges-20260818/pelvismeshinvrot_visual_reject_decision.json`.

Added and single-clip probed three remaining `bone_pelvis.mesh` display-basis
orders on the hard reject frame `gh3_guit_mido_a_med_idle01` frame 60:

- `pelvismeshpre`: `mesh_bind * world` -> collapsed/non-bipedal reject.
- `pelvismeshpreinv`: `inverse(mesh_bind) * world` -> not coherent bipedal
  reject.
- `pelvismeshpost`: `world * mesh_bind` -> upside-down/collapsed reject.

The fast captures are:

- `analysis/gh3_midori_probe_pre_fastcapture/midori_1_medium_idle_f060.bmp`
- `analysis/gh3_midori_probe_preinv_fastcapture/midori_1_medium_idle_f060.bmp`
- `analysis/gh3_midori_probe_post_fastcapture/midori_1_medium_idle_f060.bmp`

Decision record:
`.codex/current-evidence/midori-review-source-bridges-20260818/pelvis_mesh_basis_order_probe_decision.json`.

Important rig finding from the deployed character MILO:
`Control_Root` exists, but no transform is parented under it; `bone_pelvis.mesh`
is parented to `gh3_midori_1`, not `Control_Root`. A Control_Root-only
animation channel is therefore unlikely to rotate the visible body in the
current built rig. The next useful direction is either a rig hierarchy/root
correction that makes the intended root actually parent the visible body, or a
different source-to-target root/pelvis formulation. Do not retest the direct
`bone_pelvis.mesh` bind-basis multiplication family as an approval candidate.

## 2026-08-18 Control_Root model-parent probe

Added a diagnostic model-bundle option, `--control-root-pelvis-parent`, in
`tools/gh3_midori_model_bundle.py`. With `--stock-bind-scope upper-limbs-guitar`
it builds a probe model where `bone_pelvis.mesh parent=Control_Root` instead of
the deployed/default flattened `bone_pelvis.mesh parent=gh3_midori_1`.

The corrected probe model built and inspected successfully, then the rebuildable
probe bundle/model directories were removed during cleanup.
However, a model-only runtime swap using the currently deployed animation MILO
rendered blank/off-camera:
`analysis/gh3_midori_controlroot_model_fastcapture/midori_1_medium_idle_f060.bmp`.
The temporary model swap was restored afterward; local `GEN` + loose `DLC` only,
no game-time ISO, Idle priority.

Decision record:
`.codex/current-evidence/midori-review-source-bridges-20260818/control_root_model_parent_probe_decision.json`.

Conclusion: the Control_Root hierarchy mismatch is a real rig/animation contract
lead, but it cannot be judged with only the model changed. The next candidate
needs a matching animation target rig/target graph for the Control_Root-parented
model, then the same direct visual bipedal review.

## 2026-08-18 Control_Root matched animation graph probe

This pass confirmed the work is both rig and animation-set work, not model-only.
A model-only Control_Root parent change leaves the clipset graph flattened, and
the character can render blank/off-camera or sideways.

New automation added:

- `milo_convert_tool build-clipset-from-acp --control-root-pelvis-parent`
  emits a matching `Control_Root` CharBone and parents `bone_pelvis.trans` /
  `bone_pelvis.mesh` to unsuffixed `Control_Root`.
- `tools/gh3_midori_acp_stage.py` now accepts `--stock-hand-detail-rig` and
  `--stock-bind-scope`, so ACP bind resolution can match the model bundle's
  `--stock-bind-scope upper-limbs-guitar`.
- ACP target bind selection now prefers the active clip target, so
  `--mesh-target-scope all-common` binds `Bone_Pelvis` to `bone_pelvis.mesh`
  instead of silently retaining the flattened `bone_pelvis` target.

Aligned packed sample check:

- `bone_pelvis.mesh.pos` at medium idle frame 60 is now
  `(-39.416, 0.00711903, -0.000602525)`, which is Control_Root-local space.
  The old mismatched path emitted `z≈38`.

Visual retest result:

- Aligned `pelvismeshrot`, `pelvismeshinvrot`, `pelvismeshpre`,
  `pelvismeshpreinv`, and `pelvismeshpost` all remain visual rejects at
  `gh3_guit_mido_a_med_idle01` frame 60. They are visible, but sideways or
  horizontal and not bipedal/playable.
- Proof and decision record:
  `.codex/current-evidence/midori-review-source-bridges-20260818/control_root_aligned_mesh_graph_basis_reject_decision.json`.

Next diagnosis should start from the corrected graph/translation contract and
look beyond the simple `GH2_RUNTIME_PELVIS_MESH_BIND_ROTATION` order family.

## 2026-08-18 Fixed lower-body mesh bind source mapping

The first aligned Control_Root/.mesh probes were still partially invalid:
`Bone_Pelvis` and spine `.pos` channels were emitted, but lower-body mesh
binds were missing from ACP target resolution. The cause was stock `.mesh`
alias copies in `tools/gh3_midori_model_bundle.py` carrying lowercase
`source_name` values such as `bone_L-thigh`. With
`--mesh-target-scope all-common`, ACP preferred those aliases, then could not
associate them with GH3 source bones like `Bone_Thigh_L`.

Patch made:

- Stock `.mesh` alias copies now keep the active Midori parent graph and
  inherit `source_name` from the unsuffixed source transform.
- Verified target bind resolution now maps:
  `Bone_Thigh_L -> bone_L-thigh.mesh`, `Bone_Knee_L -> bone_L-knee.mesh`,
  `Bone_Ankle_L -> bone_L-ankle.mesh`, `Bone_Toe_L -> bone_L-toe.mesh`, and
  equivalent right-leg aliases.

Post-fix packed sample check for `gh3_guit_mido_a_med_idle01` frame 60 includes
lower-body `.pos` channels again, for example
`bone_L-thigh.mesh.pos=(-0.00216648,-0.0236834,14.7797)`.

Post-fix visual probes:

- `rootyawfold-bakeaim-altlocal` with fixed leg channels:
  `analysis/gh3_midori_cr_rootyawfold_bakeaim_altlocal_fixedlegs_20260818_0908_capture/midori_1_medium_idle_f060.bmp`
  -> reject, still horizontal/non-playable.
- `axisblend35` with fixed leg channels:
  `analysis/gh3_midori_cr_axisblend35_fixedlegs_20260818_0912_capture/midori_1_medium_idle_f060.bmp`
  -> reject, less chaotic but not a coherent bipedal playable stance.

Decision record:
`.codex/current-evidence/midori-review-source-bridges-20260818/control_root_fixed_leg_mesh_bind_decision.json`.

Next continuation should keep the fixed lower-body bind map and treat any probe
without lower-body `.pos` channels as invalid before visual review.

## 2026-08-18 Source GLB game-up / Control_Root cancellation check

Rendered the retained medium-idle source bridge GLB directly with Blender using
new helper `tools/gh3_midori_glb_render_probe.py`. The source GLB for
`gh3_guit_mido_a_med_idle01` is itself horizontal/non-standing at frames 0 and
60 from the default Blender Z-up view, and an axis-view contact sheet still
shows a sideways/acrobatic source pose rather than a normal standing biped.

Source visual evidence:

- `analysis/gh3_midori_source_glb_medidle_f60/source_medidle_f60_ortho.png`
- `analysis/gh3_midori_source_glb_medidle_f60/source_medidle_f000_ortho.png`
- `analysis/gh3_midori_source_glb_medidle_f60_views/source_medidle_f60_contact.png`

Added diagnostic ACP policy:

- `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-upalign-bakeaim-altlocal-bakepos-bind`
  rotates mapped source-relative vectors so pelvis-to-chest points toward GH2
  +Z before position/aim bake.

Fixed-lower-body visual probes, all local `GEN` + loose `DLC`, no game-time ISO,
Idle priority:

- `sourcepos-upalign-bakeaim-altlocal`:
  `analysis/gh3_midori_cr_upalign_bakeaim_altlocal_20260818_0924_capture/midori_1_medium_idle_f060.bmp`
  -> reject; correction is active but still compact/non-bipedal.
- `valid-rowaim-rootlocal`:
  `analysis/gh3_midori_cr_rootlocal_fixedlegs_20260818_0930_capture/midori_1_medium_idle_f060.bmp`
  -> reject; curled/non-bipedal.
- `valid-rowaim-rootbasis`:
  `analysis/gh3_midori_cr_rootbasis_fixedlegs_20260818_0930_capture/midori_1_medium_idle_f060.bmp`
  -> reject; curled/non-bipedal.

Decision record:
`.codex/current-evidence/midori-review-source-bridges-20260818/source_glb_gameup_rootbasis_decision.json`.

Next diagnosis should audit source pose extraction/root application before
retargeting: specifically whether the NXTools/GLB bridge is applying
`Control_Root` motion or evaluated pose matrices in a way GH3 runtime would not.

## 2026-08-19 continuation checkpoint

- Resumed at the pelvis-only matrix-local / `Control_Root` diagnosis. No
  CD-ROM/ISO volume was mounted; no emulator/game-time run was started. All
  Python probes were launched at `BelowNormal` priority.
- Added `tools/gh3_midori_model_parent_replay_diagnostic.py`. This diagnostic
  replays saved ACP locals under the deployed model parent map from
  `analysis/gh3_midori_current_midori1_rig.json`.
- New proof artifact:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_model_parent_replay_diagnostic.json`.
  It confirms the actionable mismatch: the current targetlength animation frame
  treats `Bone_Pelvis` as flat/root-parented while the deployed model has
  `bone_pelvis` under `Control_Root`. Replaying the same locals under the model
  parent map diverges by `120.000034` degrees and `60.073724` units; the same
  tool computes an idealized compensated pelvis local that returns to the
  targetlength world pose within `0.061445` degrees and `0.000015` units.
- Added first exporter diagnostic policy
  `matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-targetlength-modelrootcomp-bakepos-bind`
  in `tools/gh3_midori_acp_stage.py`. This branch is **not promotable** and
  should not be visually staged yet. Its replay gates fail:
  `targetlength_modelrootcomp_reference_replay_diagnostic.json` and
  `targetlength_modelrootcomp_reference_replay_diagnostic_transpose.json`.
- Next concrete step: compute final `bone_pelvis.pos` and `bone_pelvis.quat`
  values directly in the packed channel convention proven by
  `gh3_midori_model_parent_replay_diagnostic.py`, then require
  `model_parent_replay_matches_reference` before any MILO/DLC build or emulator
  capture.

### 2026-08-19 model-parent compensation proof

- `tools/gh3_midori_model_parent_replay_diagnostic.py` now has
  `--compensated-skeleton-output`. It can materialize the corrected
  `Bone_Pelvis` local rotation/translation for the deployed
  `Control_Root -> bone_pelvis` model hierarchy while preserving the
  targetlength world pose.
- New passing validation:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_current_modelparentcomp_validation_diagnostic.json`
  has status `model_parent_replay_matches_reference`, max rotation delta
  `0.088075` degrees, max position delta `0.000019` units.
- Generated compensated frame:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_current_medidle_modelparentcomp_skeleton_frame.json`.
  Frame-60 `Bone_Pelvis.local_translation` is
  `[-38.193199, -0.086219, -0.57805]` and `model_parent_source` is
  `Control_Root`.
- The first direct exporter policy hook in `tools/gh3_midori_acp_stage.py`
  still fails replay validation. Treat it as diagnostic only. Next step should
  be an ACP sample-stream compensation pass based on the now-passing skeleton
  transform, then model-parent replay validation before any visual stage.

### 2026-08-19 ACP stream compensation proof

- Added `tools/gh3_midori_acp_model_parent_compensate.py`. It copies a staged
  ACP directory and rewrites an explicit pelvis channel base so flat targetlength
  world pelvis samples reconstruct under an explicit parent transform. This is a
  post-stage diagnostic bridge toward MILO, not a visual-approved candidate yet.
- Ran it on the retained one-clip attack-left stage:
  `analysis/gh3_midori_fresh_attack_targetlength_acp_20260818_run1` ->
  `analysis/gh3_midori_fresh_attack_targetlength_modelparentcomp_acp_20260819_run1`,
  using pelvis base `bone_pelvis.mesh`, parent `Control_Root`,
  `parent_t`, and `hmx_quat_mode=transpose`.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/targetlength_attack_acp_modelparentcomp_report.json`.
  The tool patched `145` pelvis pos samples and `145` pelvis quat samples; its
  built-in replay check reports `0` position and `0` rotation error.
- Spot check frame 30:
  `bone_pelvis.mesh.pos` changed from `[-0.000006, -0.000004, 39.405975]` to
  `[-39.405975, 0.000006, -0.000004]`, and `bone_pelvis.mesh.quat` changed
  from `[-0.5, 0.5, 0.5, 0.5]` to `[0, 0, 0, 1]`.
- Removed the failed direct `targetlength-modelrootcomp` policy from
  `tools/gh3_midori_acp_stage.py`; use the ACP post-process route instead.
  Next step: feed the compensated one-clip ACP stage into the existing
  ACP-to-MILO path as a tiny diagnostic package, then run no-ISO bipedal visual
  precheck before broad rollout.

### 2026-08-19 ACP-to-MILO model-parent probe

- Fed the compensated attack-left ACP stream through ihatecompvir's existing
  `milo_convert_tool build-clipset-from-acp` route, at `BelowNormal` process
  priority and without any ISO/emulator/game runtime. The converter consumes a
  flat directory of `.acp` files; the retained compensated ACP stage remains
  the source of truth.
- Command route:
  `milo_convert_tool build-clipset-from-acp <flat-acp-dir> --name gh3_midori_main --role guitar-main --out <milo> --move-self 0 --control-root-pelvis-parent`.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/modelparentcomp_milo_probe_20260819/modelparentcomp_milo_probe_report.json`.
  Output MILO:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/modelparentcomp_milo_probe_20260819/gh3_midori_main_modelparentcomp_probe.milo_ps2`
  (`36,299` bytes, SHA256
  `72F304DC396C5A10B09517F11CD9663064FEA418400EA6CEB8230DB679E91ECB`).
- `inspect-clipset --channels --events` confirms this is an ordinary GH2
  `CharClipSet`/`CharClipSamples` MILO with one clip, fallback group `normal`,
  `move_self=0`, generated `Control_Root`, and
  `bone_pelvis.mesh parent=Control_Root`. The pelvis sample channels survived:
  frame-30 `bone_pelvis.mesh.pos` is
  `[-39.406, 0.00000642005, -0.00000352317]`, and
  `bone_pelvis.mesh.quat` is
  `[-0.000000089407, -0.0000000298023, -0.0000000298023, 1.0]`.
- This clears the automated intermediate-format bridge (`GLB/ACP -> MILO`) for
  the pelvis/Control_Root hypothesis. It is **not** visual-approved. Next gate:
  temporarily swap only this tiny diagnostic main MILO into the local loose-DLC
  tree, launch from local `gh2_ps2_hybrid_assets/GEN` only, keep all heavy
  processes low priority, and run the bipedal precheck. If any capture is
  non-bipedal, reject immediately and do not broaden to the animation set.

### 2026-08-19 model-parent attack visual precheck

- Ran the first direct-viewer visual gate for the model-parent-compensated
  one-clip MILO. Only the loose-DLC
  `char/gh3_midori/anims/gen/gh3_midori_main.milo_ps2` was temporarily swapped;
  `gh2_ps2_hybrid_assets/GEN/main.hdr` + `main_0.ark` were used directly, no
  GH2 ISO was mounted or used, and `ghogx_app` ran at `Idle` priority.
- Captured `gh3_guit_mido_a_attackl` frames `15/30/45` with the native
  `--char` viewer and `GHOGX_ADDONS_DIR=gh2_ps2_hybrid_assets/DLC`. The initial
  frame-timed capture used `--screenshot-frame` equal to the clip frame and
  timed out before BMP writes; logs still proved clip load and pose application.
  The corrected fast capture freezes `--clip-frame` to the desired animation
  frame and writes the screenshot at render frame `2`.
- The deployed main MILO was restored afterward to canonical SHA256
  `F1A06A0E9507023D7F631598693D9F43C47C00A2982068737EDE67CF7452F598`.
- Evidence directory:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/modelparentcomp_visual_probe_20260819_fast/`.
  Important files:
  `modelparentcomp_attack_f015_fast.bmp`,
  `modelparentcomp_attack_f030_fast.bmp`,
  `modelparentcomp_attack_f045_fast.bmp`,
  `modelparentcomp_attack_contact_sheet.jpg`,
  `visual_decision.json`.
- Visual decision: `pass_attack_only_bipedal_precheck`. All three inspected
  frames are coherent bipedal side-fall poses; this clears the user's immediate
  non-bipedal rejection gate for the one-clip `Control_Root -> bone_pelvis.mesh`
  compensation hypothesis. It is **not** final animation/DLC approval and not
  broad clip coverage.
- Next step: apply `tools/gh3_midori_acp_model_parent_compensate.py` to a
  representative multi-clip `guitar-main` stage, build a diagnostic main MILO
  with `milo_convert_tool build-clipset-from-acp ... --move-self 0 --control-root-pelvis-parent`,
  then run sequential direct-viewer bipedal precheck. Reject immediately on any
  non-bipedal capture before attempting full animation rollout or in-song
  validation.

### 2026-08-19 multi-clip source-bridge diagnosis

- The one-clip attack-left model-parent ACP/MILO path remains the only visual
  pass. The representative five-clip `guitar-main` build from the existing
  fresh targetlength bridge was structurally valid but visually rejected:
  medium idle, attack, jump, and transition were curled/folded rather than
  playable. A hybrid MILO replacing only attack with the known-good one-clip
  ACP restored the coherent attack silhouette, so the global MILO converter and
  `CharBone` union are not the failure source.
- The failure is now narrowed to source bridge/staging input. The known-good
  attack bridge came from pinned NXTools commit
  `6cea808a27d6773bde55e947c9f0ffd72081e164`; current
  `C:\Users\smmel\nxtools` is `eee42e4`. The pinned checkout still exists at
  `C:\Users\smmel\AppData\Local\Temp\nxtools_ref`, and the old extracted GH3
  source tree remains at
  `C:\Users\smmel\AppData\Local\Temp\gh3_midori_source_visual_20260816_211144\source`.
- `tools/gh3_midori_source_visual.py` now suppresses NXTools texture import for
  bridge generation and only repairs checksum-named bones. This gets past the
  DJ Hero texture-module failure and preserves pinned NXTools names, but
  regenerated attack bridges still do not match the old passing artifact's
  evaluated `pose` fields. The staged/compensated numeric reject remains
  `bone_pelvis.mesh.pos [-38.1932, -0.086219, -0.57805]` and
  `bone_pelvis.mesh.quat [0.0270215, 0.706591, 0.706589, -0.0270225]`, not the
  known-good one-clip `[-39.406, 0, 0]` / identity path.
- Resume by reproducing the old bridge `pose` matrices for representative
  clips, or by staging from retained known-good bridge artifacts where
  available. Do not run game-time checks from any GH2 ISO; use local
  `gh2_ps2_hybrid_assets/GEN`, keep heavy processes low priority, and reject
  non-bipedal frames immediately.

### 2026-08-19 pause checkpoint: matched Control_Root/no-stock representative pass

- Supersedes the stale note above about regenerated pinned bridges not
  matching. With pinned NXTools
  `C:\Users\smmel\AppData\Local\Temp\nxtools_ref` at
  `6cea808a27d6773bde55e947c9f0ffd72081e164` plus the new
  `--force-partial-anims` path, regenerated attack bridge `pose`, `basis`, and
  `matrix_local` exactly match the old passing artifact (`max_abs_delta=0`).
- The bad pelvis sample was traced to applying the default stock GH2 body bind
  override during ACP staging. Staging the old passing bridge with the default
  `analysis/gh3_midori_stock_glam1_rig.json` reproduces the bad
  `bone_pelvis.mesh` sample `[-38.1932, -0.086219, -0.57805]`; disabling the
  stock bind override restores the known-good compensated sample
  `[-39.406, 0.00000642005, -0.00000352317]` with identity quat.
- Built a matched diagnostic pair: no-stock
  `--control-root-pelvis-parent` model plus five representative no-stock,
  force-partial, model-parent-compensated guitar-main clips. The probe model
  has `bone_pelvis.mesh parent=Control_Root`; the clipset is ordinary GH2
  `CharClipSet`/`CharClipSamples`, `move_self=0`, and all representative pelvis
  numeric samples match the known-good `[-39.406, 0, 0]` / identity path.
- Direct native viewer precheck used local `gh2_ps2_hybrid_assets/GEN`,
  `GHOGX_ADDONS_DIR=gh2_ps2_hybrid_assets/DLC`, no ISO, and `ghogx_app` at
  `Idle` priority. Both deployed DLC files were restored afterward to canonical
  hashes: model
  `D0927316AB57C1CCD3DC0A564C03FCD52F244F4C042394B2BDA03B34EB2C8A7A`, main
  anim `F1A06A0E9507023D7F631598693D9F43C47C00A2982068737EDE67CF7452F598`.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/modelparentcomp_pinned_forcepartial_nostock_5case_visual_20260819_fast/visual_decision.json`
  and `matched_controlroot_contact_sheet.jpg`. Sequential visual inspection of
  all five captures passed the immediate bipedal reject gate. It is not final
  DLC approval: this covers one outfit and five representative main clips only,
  and arms/guitar/controller tiers still need follow-up.
- Next resume should stop treating source bridge or global MILO conversion as
  the primary suspect. The remaining work is rollout/iteration: automate the
  matched no-stock/Control_Root path into the full pipeline, then decide whether
  upper limbs/guitar need a scoped stock bind compromise
  (`--stock-bind-scope upper-limbs-guitar`) while preserving the no-stock pelvis
  body fix.

### 2026-08-19 pipeline automation follow-up

- The matched diagnostic route is now exposed in the canonical pipeline. Use:
  `python tools/gh3_midori_build_pipeline.py --source-pose-bridge-manifest <forcepartial manifest> --rotation-policy matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-targetlength-bakepos-bind --mesh-target-scope all-common --no-gh2-animation-rig --control-root-pelvis-parent --model-parent-compensate-acp ...`
- Relevant implementation changes:
  `tools/gh3_midori_build_pipeline.py` has `--no-gh2-animation-rig`,
  `--control-root-pelvis-parent`, `--model-parent-compensate-acp`, raw ACP
  staging via `analysis/gh3_midori_acp_stage_raw`, post-stage compensation into
  `analysis/gh3_midori_acp_stage`, no-stock/Control_Root bridge gate forwarding,
  and clipset generation with `--move-self 0 --control-root-pelvis-parent`.
  `tools/gh3_midori_acp_stage.py`, `tools/gh3_midori_pose_report.py`,
  `tools/gh3_midori_bridge_policy_multiclip_report.py`, and
  `tools/gh3_midori_acp_disk_bridge_gate.py` now accept/forward explicit
  `--no-gh2-stock-rig` where needed.
- Verified this continuation with `py_compile`, help output for the new flags,
  and targeted unit tests:
  `MidoriPipelineTest.test_production_pipeline_uses_generic_retarget_contract`
  and
  `MidoriPipelineTest.test_glblocalraw_localz180_thighbasis_policies_get_pelvis_correction`.
  No ISO was used. No full rebuild or visual approval was run in this
  continuation.

### 2026-08-19 structural full-set candidate

- Important correction to the automation: when `--model-parent-compensate-acp`
  is enabled, raw ACP staging must stay no-stock but not Control_Root-parented.
  Control_Root parenting is applied afterward in model bundling and
  `build-clipset-from-acp --control-root-pelvis-parent`, with the ACP pelvis
  stream compensated between those steps. Passing Control_Root into raw ACP
  staging caused a rotated pelvis sample and was fixed.
- A canonical analysis-only structural run completed at `Idle` priority with:
  `--skip-bridge-gate --source-pose-bridge-manifest .codex/current-evidence/midori-review-source-bridges-pinned-forcepartial-5case-20260819/review_source_bridge_batch_manifest.json --rotation-policy matrix-torso-targetgraph-rootworld-glblocalraw-localz180-thighbasis-parentcomp-targetsolveskel-sourcepos-targetlength-bakepos-bind --mesh-target-scope all-common --no-gh2-animation-rig --control-root-pelvis-parent --model-parent-compensate-acp --skip-runtime-proof --skip-runtime-deploy --skip-dlc-package --skip-hybrid-dlc-deploy --skip-anim-runtime-sanity`.
- Result: full animation-set candidate MILOs were generated in `analysis` only,
  not deployed. `analysis/gh3_midori_gh2_anim_milo_manifest.json` reports 331
  total clips across 4 animation packages (`guitar-main=266`, `guitar-ui=6`,
  `guitar-fret=36`, `guitar-strum=23`). `analysis/gh3_midori_gh2_model_milo_manifest.json`
  reports 2 structurally valid outfit model MILOs. Main candidate:
  `analysis/gh3_midori_gh2_milos/gh3_midori_main.milo_ps2`, SHA256
  `003B0C61512388F9F6DBFB02099EECF938360A67DE3EE51B3A7A572680637981`,
  `28,310,744` bytes. Outfit 1 model candidate:
  `analysis/gh3_midori_gh2_models/gh3_midori_1.milo_ps2`, SHA256
  `F08B6A02D5C172517FA5BD737C481A5DBA9EE5221FD7193CBC78727089A34F50`,
  `283,646` bytes.
- Structural proof:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pipeline_controlroot_nostock_structural_orderfix_20260819.json`.
  It records 331 compensated clips, zero compensation replay error, GH3 ISO SHA
  skipped, no runtime/viewer launch, no package/deploy, and exact attack frame
  30 pelvis parity with the previous passing five-case probe.
- Loose deployed DLC remained unchanged after the run:
  `gh2_ps2_hybrid_assets/DLC/community.gh3.midori/content/char/gh3_midori_1/og/gen/gh3_midori_1.milo_ps2`
  SHA256 `D0927316AB57C1CCD3DC0A564C03FCD52F244F4C042394B2BDA03B34EB2C8A7A`;
  `gh2_ps2_hybrid_assets/DLC/community.gh3.midori/content/char/gh3_midori/anims/gen/gh3_midori_main.milo_ps2`
  SHA256 `F1A06A0E9507023D7F631598693D9F43C47C00A2982068737EDE67CF7452F598`.
- Scratch raw/staged ACP, model bundle, and pipeline log dirs from this run were
  cleaned. Candidate analysis MILOs and compact manifests remain for the next
  direct visual/native candidate swap. Next step is to perform a no-ISO, Idle
  priority native-viewer visual gate using the analysis-only candidate model
  and animation files, then reject immediately if any capture is non-bipedal.

### 2026-08-19 visual branch results before pause

- No-stock full-set candidate: bipedal/framing gate improved, but final visual
  rejected due upper-body/guitar/hand defects. The no-stock route removed the
  non-bipedal pelvis/body failure but exposed hand/finger instability.
- Scoped stock upper-limbs/guitar candidate: current best branch. Built with
  `--stock-bind-scope upper-limbs-guitar` while preserving the Control_Root /
  model-parent-compensated pelvis path. Native-viewer capture used only local
  `gh2_ps2_hybrid_assets/GEN` plus loose DLC, no ISO, low-priority viewer.
  Proof:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_full_pose_review_20260819/pose_review_proof_manifest.json`
  status `native_viewer_representative_pose_framing_review_passed`, 9 proofs,
  0 automated failures. Sequential visual inspection passed biped/framing but
  still rejected final approval because guitar/hand contact is wrong.
- Scoped stock plus `--suppress-stock-guitar-main-anchor`: diagnostic branch
  only. It kept the exact known-good pelvis sample and changed
  `bone_pos_guitar.mesh` from the forced stock transform, but all 9 inspected
  native-viewer frames still failed performer coherence: guitar through/behind
  torso and hands detached from fret/strum contact. Proof:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_suppressguitar_full_pose_review_20260819/`;
  decision file `visual_decision.json`.
- Next resume: do not restart at pelvis-only matrix-local / Control_Root. Treat
  body/pelvis as solved to the bipedal gate. Continue at guitar/hand binding
  diagnosis and/or automated GLB-to-MILO source route using pinned ihatecompvir
  NXTools evidence plus `milo_convert_tool` output. The project is still
  experimental, not final DLC approval.

### 2026-08-19 preserve-local guitar attach diagnostic

- Added opt-in `--preserve-guitar-attach-local` to
  `tools/gh3_midori_model_bundle.py` and `tools/gh3_midori_build_pipeline.py`.
  It prevents scoped stock bind import from overwriting the already generated
  `bone_pos_guitar(.mesh)` local attach with a stock-world transplant.
- Analysis-only candidate built successfully with the same Control_Root /
  model-parent-compensated pelvis path and 331 animation clips. Candidate:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_preservelocal_full_candidate_20260819/`.
  `gh3_midori_main.milo_ps2` SHA256
  `A91CEE8FA7E9B794AE8E665699559025C51029E52DC761EB084BAC7868FB801E`;
  outfit models changed to SHA256
  `A16181C2E31DB13601058D61A1DA7E1E317CAB2487B7B532C5CCCE784A2E4CAE`
  and
  `837DC23E234E68DAA9F0A509CCF741A19A0CAA53A8D84618F0429D97596757BE`.
- Visual capture from local `gh2_ps2_hybrid_assets/GEN` plus loose DLC, no ISO,
  low priority, rejected the branch. The body stayed bipedal and the pelvis
  sample stayed exact, but the guitar moved behind/right of the body and clipped
  above the head in the accessory frame. Hands remained detached from fret/strum
  contact. Decision:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_preservelocal_full_pose_review_20260819/visual_decision.json`.
- Interpretation: model-side `bone_pos_guitar(.mesh)` stock-world versus
  stock-local alone is not enough. Next branch should target the combined
  contract between main `bone_pos_guitar.mesh` animation and the independent
  `bone_fret_hand.mesh` / `bone_strum_hand.mesh` IK clipsets.

### 2026-08-19 helper compensation diagnostic

- Added opt-in `--compensate-guitar-helper-for-main-anchor` to
  `tools/gh3_midori_model_bundle.py` and pipeline passthrough in
  `tools/gh3_midori_build_pipeline.py`, with matching
  `--main-guitar-pos-offset`. This computes the `bone_gh3_c00e3395` local as a
  child of the forced main `bone_pos_guitar.mesh` anchor instead of only the
  static model bind parent.
- Built a model-only candidate against the already-preserved current-best
  stockupper animation MILOs:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_helpercomp_full_candidate_20260819/`.
  Model hashes:
  `gh3_midori_1.milo_ps2`
  `37303F66124556E05734E97A5237B59793D5A63193BC92EFF29B7006F51ED834`;
  `gh3_midori_2.milo_ps2`
  `3FA6533165ACA421BB2B5AD3B77D1ABD7BD9A145CDD8D1458A16F3FA9E2A9379`.
  Reused current-best animation hashes, including main
  `A91CEE8FA7E9B794AE8E665699559025C51029E52DC761EB084BAC7868FB801E`.
- Native viewer capture used local `gh2_ps2_hybrid_assets/GEN` plus loose DLC,
  no ISO, low priority. Automated framing passed (9 proofs, 0 failures), but
  sequential visual inspection rejected the branch: the guitar remains
  behind/right of the body and hands remain detached from fret/strum contact
  across both outfits. Decision:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_helpercomp_full_pose_review_20260819/visual_decision.json`.
- Interpretation: changing GH3 helper local alone does not move the visible
  instrument into the hands. Next branch should directly diagnose/adjust the
  relationship between main `bone_pos_guitar.mesh` animation and the separate
  `bone_fret_hand.mesh` / `bone_strum_hand.mesh` IK target clipsets.

### 2026-08-19 MIDI fret-target sweep diagnostic

- Added `tools/gh3_midori_guitar_ik_contract_report.py` before this sweep. It
  reports the current-best stockupper candidate has large visible-hand versus
  IK-target disagreement even with viewer xplorer prop overrides: LH-to-fret up
  to `37.630`, RH-to-strum up to `34.868`, and impossible single-anchor deltas
  up to `147.164`.
- Engine inspection confirmed the standalone viewer does call
  `apply_character_pose_controller_frame`, the same high-level controller frame
  used by gameplay. The deeper controller debug envs are
  `GHOGX_AUDIT_CHARACTER_GRAPH` and `GHOGX_DEBUG_IK`; `GHOGX_CONTROLLER_AUDIT`
  only covered viewer-side flag logging.
- Built only `ghogx_app` at Idle priority with local Ninja
  `_community_re/Guitar-Hero-II-Deluxe-Unified/dependencies/windows/ninja.exe`
  and VS `vcvars64`, then ran a no-ISO hand-overlay sweep from local
  `gh2_ps2_hybrid_assets/GEN` plus loose DLC. The retained current-best
  candidate was swapped into loose DLC only for capture and then restored.
- Swept explicit `--midi-fret-target` values: none,
  `spot_neck_fret03.mesh`, `spot_neck_fret07.mesh`,
  `spot_neck_fret11.mesh`, and `spot_neck_fret15.mesh`. All five captures are
  visually equivalent for approval purposes and rejected: Midori is bipedal,
  but the guitar remains beside/behind the performer and the visible hands stay
  detached from fret/strum contact. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_miditarget_sweep_20260819/visual_decision.json`.
- Next resume: continue at controller graph/IK target publication or a
  diagnostic visible-hand solve/bake. Do not spend the next branch merely
  trying more fret target names, and do not go back to pelvis/Control_Root.

### 2026-08-19 deep IK trace and left-reach offset rejection

- Deep viewer trace with `GHOGX_AUDIT_CHARACTER_GRAPH`, `GHOGX_DEBUG_IK`, and
  `GHOGX_DEBUG_ARM_CONTRACT` proves the controller path is live. The viewer
  logs `2 ikHand`, `1 ikMidi`, `left.weight=1`, `right.weight=1`, and both
  `CharIKHand` controllers solving. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_deep_ik_trace_20260819/`.
- Important diagnosis: `right_hand.ik` is nearly exact to
  `bone_strum_hand.mesh` (`preFinalError=0.0362`), but `left_hand.ik` is aimed
  at `bone_fret_hand.mesh` about `33.75..39.06` units from the left shoulder
  while measured left arm reach is about `19.15`. This is a target-space
  problem, not a missing-IK problem.
- Updated `tools/gh3_midori_guitar_ik_contract_report.py` to emit shoulder
  reach, overreach, and suggested whole-guitar offsets. Current output:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_ik_contract_report_reach_20260819.json`.
  The left-fret average suggested `main_guitar_pos_offset=6.0,-2.84,2.91`.
- Built that real branch as
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_leftreach_offset_candidate_20260819/`
  and captured a no-ISO low-priority pose review. Automated proof failed one
  frame (`midori_1_accessory_acc01_f030:framing_has_margins`), and direct
  visual inspection rejects the branch: the guitar remains beside/behind the
  performer, hand-overlay is tangled, and the offset worsens overall
  coherence. Decision:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_leftreach_offset_pose_review_20260819/visual_decision.json`.
- Next resume: do not run more plain `bone_pos_guitar.mesh` translation-only
  offsets. Continue with target publication/rotation, source guitar orientation
  mapping, or a diagnostic visible-hand/guitar solve/bake.

### 2026-08-19 target-publication probes after left-reach rejection

- Timing probe: captured the current-best hand-overlay with explicit
  `--midi-fret-target spot_neck_fret11.mesh` at screenshot frames 2, 8, and 14.
  `CharIKMidi` reaches full fraction/weight by frame 8, but the screenshots are
  visually equivalent for approval purposes and still rejected. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_midi_timing_probe_20260819/visual_decision.json`.
- Hand-output probe: ran the current-best hand-overlay with
  `GHOGX_DISABLE_HAND_OUTPUT_LAYER=1`. This keeps the prop-local
  `bone_fret_hand.mesh` target (`[-6.27,-0.45,-4.32]`) instead of the hand clip
  output-local target (`[-5.32,1.65,1.05]`), proving the clip output layer does
  override the prop target. Visual result is still rejected, and the left target
  remains too far from the arm. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_disable_hand_output_probe_20260819/visual_decision.json`.
- Next resume: timing is ruled out, and disabling hand output alone is ruled
  out. Work the broader guitar/target orientation-publication contract or bake
  a diagnostic visible-hand/guitar solve; do not return to pelvis/Control_Root,
  plain fret-name sweeps, or plain `bone_pos_guitar.mesh` translation offsets.

### 2026-08-19 hand-target-root world-publication no-op

- Tested a narrower runtime publication hypothesis on the current-best
  stockupper candidate: publish only `bone_fret_hand` / `bone_strum_hand`
  output worlds through `current_chain * inverse(bind_chain) * stored_world`,
  while leaving finger output rows hand-local. Capture path used local
  `gh2_ps2_hybrid_assets/GEN`, loose DLC, no ISO, and low-priority viewer
  execution; loose DLC was restored afterward.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_handtarget_root_world_probe_20260819/visual_decision.json`.
  The no-bridge control and hand-target-root bridge screenshots are
  byte-identical SHA256
  `0413779D974CB4B0C2545FA198F4D34DC1B46FC0BDBD87F7CDA0D8AF63A29FB0` and
  visually still rejected.
- Temporary diagnostic code was removed from the engine trees and the
  build-target `ghogx_app` was rebuilt back to source parity. Next resume:
  do not run more runtime root-publication-only variants. Move to authored
  guitar/target relationship changes, source guitar orientation mapping, or a
  diagnostic visible-hand/guitar solve/bake.

### 2026-08-19 authored hand-root prop-local branch

- Added automated staging support for
  `--hand-root-position-source {reference,prop-local}` in
  `tools/gh3_midori_acp_stage.py`, plus build-pipeline pass-through. The
  diagnostic keeps the current-best stockupper body/main/model files and
  rebuilds only fret/strum hand packages with `bone_fret_hand` /
  `bone_strum_hand` position rows sourced from the xplorer prop-local targets
  instead of the GH2 hand-reference proxy.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_handroot_proplocal_pose_review_20260819/visual_decision.json`.
  Local `GEN` + loose DLC only, no ISO, low-priority viewer, and loose DLC
  restored afterward. The screenshot differs from stockupper, so ACP-level hand
  root rows do affect the visible solve, but direct visual inspection rejects
  the branch: guitar is still beside/behind the performer and strum/fret contact
  is incoherent.
- Next resume: do not promote raw prop-local hand roots or move hand-root rows
  alone. Work a coupled guitar-body plus fret/strum target relationship solve,
  source guitar orientation mapping, or a diagnostic visible-hand/guitar
  bake.

### 2026-08-19 main-guitar z180-post rotation rejection

- Added automated staging support for
  `--main-guitar-rotation-correction
  {none,x180-pre,y180-pre,z180-pre,x180-post,y180-post,z180-post}` in
  `tools/gh3_midori_acp_stage.py`, plus build-pipeline pass-through. This lets
  future runs test source guitar orientation corrections without runtime hacks.
- Built the `z180-post` diagnostic from the current-best stockupper branch as a
  main-MILO-only candidate. The candidate main hash was
  `F3A804C40576D7DF88D64C4FA4555ED96BEA9985736C2C9F2CE84C3233BEC2F5`.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_mainrot_z180post_pose_review_20260819/visual_decision.json`.
  Capture used local `gh2_ps2_hybrid_assets/GEN` plus loose DLC, no ISO, and
  low-priority execution; loose DLC was restored afterward. Direct visual
  inspection rejects the branch because the body/prop framing is unusable and
  guitar/hand contact does not improve.
- Next resume: do not promote `z180-post` or revisit pelvis/Control_Root.
  Continue with bounded orientation/target contract probes, ihatecompvir
  GLB-to-MILO source tooling if available locally, or a diagnostic
  visible-hand/guitar bake.

### 2026-08-19 source GLB hand/guitar helper branch rejection

- Refreshed the local ihatecompvir bridge audit:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/ihatecompvir_bridge_audit_20260819.json`.
  Verdict remains: public `glTFMilo` is useful as reference/bridge source, but
  is not a drop-in GH2 PS2 final converter. MiloLib/GH2 notes remain useful for
  source-shaped writer/validator work.
- Regenerated a richer Blender/NXTools source bridge for
  `gh3_guit_mido_a_attackl` with torso, arms, palms, `bone_guitar_body`, and
  both GH3 guitar IK hand helper bones. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/source_bridge_attack_arm_guitar_20260819/`.
- Added `tools/gh3_midori_source_guitar_contract_report.py`. Its report shows
  stock GH2 hand target locals differ sharply from GH3 source guitar-helper
  locals in the GH3 guitar-body frame: left/fret delta length about `7.73`,
  right/strum delta length about `7.46`.
- Added automated ACP staging mode
  `--hand-root-position-source source-ik-helper` plus
  `--source-guitar-contract-report`, with build-pipeline pass-through. Built a
  full fret/strum source-helper branch and captured one hand-overlay pose using
  local `gh2_ps2_hybrid_assets/GEN` plus loose DLC only, no ISO, low priority;
  loose DLC was restored afterward.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_sourceik_helper_pose_review_20260819/visual_decision.json`.
  Direct visual inspection rejects the branch. It is bipedal, but the guitar is
  still behind/beside the performer and both hands remain detached from
  coherent fret/strum contact.
- Next resume: do not promote raw, unscaled GH3 source IK helper locals as
  static fret/strum hand target roots. Use the GLB source report as proof of
  target-space mismatch, then solve guitar body and fret/strum target
  orientation/scale together. Do not revisit pelvis/Control_Root.

### 2026-08-19 source IK helper GH2-scale overlay rejection

- Added automated staging mode
  `--hand-root-position-source source-ik-helper-gh2scale`. It uses the source
  guitar contract report, maps GH3 helper locals through the established Midori
  basis `(-x, z, y)` and `GH3_PS2_SKELETON_TO_GH2_SCALE`, then writes those
  values as fret/strum hand target position rows.
- Built only the two hand-overlay clips into minimal diagnostic fret/strum
  MILOs rather than rebuilding all animation banks. Capture used local
  `gh2_ps2_hybrid_assets/GEN` plus loose DLC only, no ISO, low-priority
  execution, and loose DLC was restored afterward.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_sourceik_gh2scale_overlay_pose_review_20260819/visual_decision.json`.
  Direct visual inspection rejects the branch. It remains bipedal, but the
  mapped targets only shift the detached hand/guitar mass; the guitar stays
  behind/right of the performer and fret/strum contact is still incoherent.
- Next resume: do not promote raw or GH2-scale-mapped GH3 source IK helper
  locals as static hand target roots. Hand-target-only branches are exhausted
  for this line. The remaining path is to move visible guitar body and
  fret/strum targets together in one shared target frame, or bake a diagnostic
  visible hand/guitar pose. Do not revisit pelvis/Control_Root.

### 2026-08-19 coupled-center main-anchor overlay rejection

- Ran `tools/gh3_midori_guitar_ik_contract_report.py` on the current-best
  stockupper candidate and extracted the hand-overlay
  target-center-to-hand-center delta `[15.012511, -12.346049, 15.937957]`.
  This was a least-squares-style diagnostic translation for the whole
  guitar/target cluster, not a final solve.
- Built a minimal one-clip main MILO with that value as
  `--main-guitar-pos-offset`, reused the current-best full fret/strum/model/UI
  files, and captured one hand-overlay pose through local
  `gh2_ps2_hybrid_assets/GEN` plus loose DLC only. No ISO was used, execution
  was low priority, and loose DLC was restored afterward.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_coupled_center_overlay_pose_review_20260819/visual_decision.json`.
  Direct visual inspection hard-rejects the branch: moving the whole
  guitar/target cluster by that large delta destroys the body into a sideways,
  non-bipedal silhouette.
- Next resume: do not generalize overlay center-delta `main_guitar_pos_offset`
  branches. The remaining path is not a large main-anchor translation. Solve
  the shared guitar/target orientation frame without corrupting the accepted
  body solve, or bake a diagnostic visible hand/guitar pose while preserving
  the body. Do not revisit pelvis/Control_Root.

### 2026-08-19 visible-hand bake overlay rejection

- Added `tools/gh3_midori_visible_hand_bake_probe.py`. It reconstructs the
  current-best stockupper overlay world pose, solves only the visible hand mesh
  locals (`bone_L-hand.mesh` and `bone_R-hand.mesh`) so they land on the
  existing fret/strum target worlds, and emits minimal ACPs for the two
  hand-overlay clips. This is a diagnostic bake, not a final animation-set
  conversion.
- Converted the generated ACPs to minimal fret/strum MILOs, reused the
  current-best body/main/model/UI files, and captured one hand-overlay pose
  through local `gh2_ps2_hybrid_assets/GEN` plus loose DLC only. No ISO was
  used, execution was low priority, and loose DLC was restored afterward.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_visible_hand_bake_overlay_pose_review_20260819/visual_decision.json`.
  Direct visual inspection rejects the branch. The useful result is that visible
  hand rows can be baked without corrupting the accepted upright/bipedal body;
  the failure is that the visible guitar frame is still behind/through the torso
  and the arms/hands remain detached/stacked.
- Next resume: do not promote hand-only visible bake overlays and do not revisit
  pelvis/Control_Root. The next diagnostic should move or reorient the visible
  guitar frame in the same target frame as the hand solve while preserving the
  current-best body solve.

### 2026-08-19 minimal visible-guitar frame plus hand-bake rejection

- Added `tools/gh3_midori_guitar_frame_hand_bake_probe.py`. It emits minimal
  overlay ACPs for `bone_pos_guitar.pos` and optional `bone_pos_guitar.quat` in
  the main bank, plus visible `bone_L-hand.pos` and `bone_R-hand.pos` in the
  fret/strum banks. The point is to move the visible prop branch and hand bake
  together while leaving the accepted body solve untouched.
- Captured three single hand-overlay proofs through local
  `gh2_ps2_hybrid_assets/GEN` plus loose DLC only, no ISO, low priority; loose
  DLC was restored to canonical hashes afterward:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_overlay_pose_review_20260819/visual_decision.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_y90post_pose_review_20260819/visual_decision.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_y90post_rotfirst_pose_review_20260819/visual_decision.json`.
- Results: translate-only preserves the bipedal body and moves the guitar beside
  Midori, but the Xplorer is edge-on/vertical. `y-90-post` reveals the guitar
  face, but the translate-before-rotate order drops the guitar/hands toward the
  feet. The corrected rotation-first `y-90-post` branch keeps the body and
  guitar face visible, but the frame is still too low/right and the hands pile
  near the legs.
- Next resume: keep the minimal `bone_pos_guitar` plus visible-hand overlay
  path, because it isolates the prop without corrupting the body. Stop using
  current broken overlay hand center as the placement target. Derive guitar
  placement from source/body-relative GLB bridge rows or a stock GH2 guitarist
  guitar pose, then solve rotation, translation, and visible hands in that
  shared frame. Do not revisit pelvis/Control_Root.

### 2026-08-19 source/body-relative visible-guitar placement rejection

- Extended `tools/gh3_midori_guitar_frame_hand_bake_probe.py` with
  `--placement-mode source-bridge-pelvis-delta`, `--source-frame`,
  `--source-basis {direct,anim,helper}`, and comma-separated rotation
  corrections. This keeps the minimal `bone_pos_guitar` plus visible-hand
  overlay path while replacing the rejected current-hand-center placement with
  a GLB source pelvis-to-guitar delta.
- Captured three single hand-overlay proofs through local
  `gh2_ps2_hybrid_assets/GEN` plus loose DLC only, no ISO, low priority; loose
  DLC was restored afterward:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcdirect_y90post_pose_review_20260819/visual_decision.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_y90post_pose_review_20260819/visual_decision.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_y90x180_pose_review_20260819/visual_decision.json`.
- Results: `srcdirect_y90post` is better centered than hand-center placement
  but still behind/low. `srcanim_y90post` is the best placement clue so far
  because it lifts the guitar into the torso/waist band while preserving the
  body, but the guitar-frame orientation and hand contact remain wrong.
  `srcanim_y90x180` is worse, flipping the prop into a broad sideways slab.
- Next resume: do not continue blind axis-flip sweeps. Keep the minimal
  prop/hand overlay path and derive the real `bone_pos_guitar` rotation basis
  from source GLB matrices or a stock GH2 guitarist pose. Do not revisit
  pelvis/Control_Root.

### 2026-08-19 source-relative visible-guitar rotation rejection

- Extended `tools/gh3_midori_guitar_frame_hand_bake_probe.py` with
  `--rotation-mode source-bridge-pelvis-relative` and
  `--rotation-source-basis {direct,anim,helper}`. This uses the GLB source
  `bone_pelvis` to `bone_guitar_body` relative rotation instead of manual axis
  flips.
- Captured two single hand-overlay proofs through local
  `gh2_ps2_hybrid_assets/GEN` plus loose DLC only, no ISO, low priority; loose
  DLC was restored afterward:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_rotanim_pose_review_20260819/visual_decision.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_rotdirect_pose_review_20260819/visual_decision.json`.
- Results: both source-relative rotation mappings preserve the accepted bipedal
  body and the better source-anim placement, but the Xplorer is vertical/edge-on
  beside the torso and hand contact remains incoherent. This rejects the current
  interpretation of the GLB source matrix as the final `bone_pos_guitar` local
  basis.
- Next resume: stop source-matrix convention guessing. Compare the emitted
  overlay basis against a stock/runtime GH2 `bone_pos_guitar` local basis and
  the Xplorer prop authored frame. Keep the minimal prop/hand overlay path; do
  not revisit pelvis/Control_Root.

### 2026-08-19 stock runtime attach-world guitar-frame rejection

- Found a diagnostic bug in
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`: non-default
  `--rotation-mode` branches computed a guitar rotation but only emitted
  `bone_pos_guitar.quat` when `--rotation-correction` was non-`none`. Earlier
  source-relative/stock-local rotation-mode captures are therefore mostly
  placement evidence, not true emitted-rotation evidence. This is now fixed by
  emitting the quat whenever `--rotation-mode` is not `correction`.
- Captured stock glam1 with Xplorer through local packed
  `gh2_ps2_hybrid_assets/GEN`, no ISO, using `GHOGX_DEBUG_PROP=1`. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/stock_glam1_xplorer_prop_debug_20260819/stock_glam1_prop_debug.log`.
  The useful runtime attach-world row begins:
  `r0=(0.420651 0.862351 0.281787)`,
  `r1=(-0.445973 0.467040 -0.763533)`,
  `r2=(-0.790039 0.195511 0.581046)`,
  `pos=(7.492639 7.913158 34.870792)`.
- Added `--rotation-mode stock-prop-debug-attach-world` and
  `--placement-mode stock-prop-debug-attach-world`. Captured two single
  hand-overlay proofs through local `GEN` plus loose DLC only, low priority,
  then restored loose DLC:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_stockattach_emitrot_pose_review_20260819/visual_decision.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_stockattach_frame_pose_review_20260819/visual_decision.json`.
- Results: both are visual rejects, but the stock runtime attach-world rotation
  is now the best guitar-frame clue. It gives the Xplorer a normal diagonal
  pose and keeps Midori bipedal. Remaining failure is placement/contact: the
  guitar is behind/right/low and the baked hands do not meet the fret/strum
  areas coherently.
- Next resume: keep the fixed minimal prop/hand overlay path and the stock
  runtime attach-world rotation. Solve a Midori/body-relative placement offset
  and hand targets inside that frame. Do not revisit pelvis/Control_Root.

### 2026-08-19 hand-center placement under stock attach rotation rejection

- With the rotation-emission bug fixed, captured two single hand-overlay proofs
  through local `GEN` plus loose DLC only, low priority, then restored loose DLC:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_handcenter_stockattach_pose_review_20260819/visual_decision.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_handcenter50_stockattach_pose_review_20260819/visual_decision.json`.
- Results: full hand-center placement under the stock runtime attach-world
  rotation preserves the body and a diagonal guitar, but over-corrects downward
  so the guitar and baked hands land in the leg/foot area. A 50% blend is less
  extreme but still visually incoherent.
- Next resume: current visible-hand center is exhausted as a placement target.
  Keep stock attach-world rotation, but derive placement from source/stock
  fret/strum contact geometry or shoulder/arm reach. Do not revisit
  pelvis/Control_Root.

### 2026-08-19 reach/source-local hand target rejection

- Extended `tools/gh3_midori_guitar_frame_hand_bake_probe.py` with
  `--placement-mode arm-reach-offset`, `--reach-scale`, and
  `--hand-target-mode {current-proxies,source-palm-locals,source-ik-helper-locals}`.
  This tested whether the failure was whole-frame placement or the local hand
  target contract inside the guitar frame.
- Captured four single hand-overlay proofs through local
  `gh2_ps2_hybrid_assets/GEN` plus loose DLC only, low priority, then restored
  loose DLC:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_reach_stockattach_pose_review_20260819/visual_decision.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_reach50_stockattach_pose_review_20260819/visual_decision.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_stockattach_ikhands_pose_review_20260819/visual_decision.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_stockattach_palmhands_pose_review_20260819/visual_decision.json`.
- Results: all four are visual rejects. Arm-reach whole-guitar offsets still
  leave the guitar through the torso/legs and hands near the lower body.
  Source IK-helper and source palm locals mapped into the source-anim placement
  plus stock attach-world rotation also leave the hands in shin/foot space.
- Operational note: do not run loose-MILO swap captures in parallel. A parallel
  capture briefly left the loose anim files on diagnostic hashes; they were
  restored from the canonical source DLC package and verified.
- Next resume: stop whole-frame translation and direct source-local hand target
  guesses. Solve the visible prop anchor/mesh compensation explicitly from the
  stock runtime prop debug rows (`attach-world`, `prop-anchor-world`,
  `prop-to-attach`, `prop-rel`, `prop-mesh-world`) and then derive fret/strum
  contact targets in that same frame. Do not revisit pelvis/Control_Root.

### 2026-08-19 prop-to-char compensation and overlay endpoint rejection

- Extended `tools/gh3_midori_guitar_frame_hand_bake_probe.py` with explicit
  runtime prop compensation. Verified the stock debug relationship:
  `prop_to_char = inverse(prop-anchor-world) * attach-world`. Applying logged
  prop-local `comp` vectors through that matrix reproduces stock `char_comp`
  rows for `bone_fret_hand.mesh`, `guitar.mesh`, and `guitar_strings.mesh`.
- Added stock prop-compensated hand target modes and
  `--overlay-channel-mode {visible-hands,visible-hands-main-delta,proxy-targets}`.
  The proxy mode emits `bone_fret_hand.pos`/`bone_strum_hand.pos`; visible modes
  emit `bone_L-hand.pos`/`bone_R-hand.pos`.
- Captured four single hand-overlay proofs through local `GEN` plus loose DLC
  only, low priority, then restored loose DLC:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_stockattach_propcomp_hands_pose_review_20260819/visual_decision.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_stockattach_propcomp_strings_pose_review_20260819/visual_decision.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_stockattach_propcomp_strings_proxy_pose_review_20260819/visual_decision.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_stockattach_propcomp_strings_handdelta_pose_review_20260819/visual_decision.json`.
- Results: all four are visual rejects, but informative. The contact targets
  are now computed in the correct stock prop-to-char frame and the viewer logs
  different layer signatures for direct, proxy, and additive-delta payloads.
  Despite that, visible hands still render near shin/foot space. Therefore the
  remaining failure is not just guitar placement, prop compensation, proxy
  target choice, or absolute-vs-additive endpoint encoding.
- Next resume: keep the prop-to-char contact math. Bake or solve the full
  arm chain (`bone_L-upperArm`, `bone_L-foreArm`, `bone_L-hand` and right-side
  equivalents) for the hand-overlay proof, approximating the missing runtime IK
  in the native viewer. Do not revisit pelvis/Control_Root.

### 2026-08-19 minimal arm-chain bake rejection

- Extended `tools/gh3_midori_guitar_frame_hand_bake_probe.py` with
  `--overlay-channel-mode visible-arm-chain` and `visible-arm-chain-rotpos`.
  These modes solve a two-bone elbow/wrist chain from the clean main body pose
  toward the prop-to-char guitar-strings contact target. The rot+pos mode emits
  `upperArm.quat`, `foreArm.quat`, `foreArm.pos`, and `hand.pos`; the
  position-only mode emits `foreArm.pos` and `hand.pos`.
- Captured two single hand-overlay proofs through local `GEN` plus loose DLC
  only, low priority, then restored loose DLC:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_stockupper_guitar_frame_hand_bake_srcanim_stockattach_propcomp_strings_armchain_pose_review_20260819/visual_decision.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/armchain_rotpos_pose_review_20260819/visual_decision.json`.
- Results: both are visual rejects. The viewer logs the extra channels as
  loaded (`ch=2` for position-only, `ch=4` for rot+pos), and sampled MILOs
  contain the expected forearm/hand positions and upper/forearm quats, but the
  rendered hands still hang near the legs and do not contact the guitar.
- Operational note: the long original rot+pos proof directory exceeded what
  Windows `CopyFile2` tolerated in the temporary loose-MILO swap runner. Use
  shorter proof directory names for future diagnostic branches.
- Next resume: minimal IK payloads are exhausted. Keep the prop-to-char contact
  math, but derive a fuller native hand-layer bake from the stock hand reference
  or source hand clips, including expected hand/finger and arm channels rather
  than only endpoint or two-bone IK channels. Do not revisit pelvis/Control_Root.

### 2026-08-19 native full hand-bank comparison

- Found a channel contract mismatch: canonical staged hand clips use `.mesh`
  channel names such as `bone_fret_hand.mesh.pos` and
  `bone_L-index01.mesh.quat`, while earlier tiny probes emitted bare names.
  Added `--channel-name-mode {bare,mesh}` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`.
- Captured mesh-channel minimal arm-chain rot+pos proof:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/mesh_armchain_rotpos_pose_review_20260819/visual_decision.json`.
  It still rejects, so namespace correction alone is not enough.
- Captured full native hand-bank comparisons through local `GEN` plus loose DLC
  only, low priority, then restored loose DLC:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/meshmain_fullhands_pose_review_20260819/visual_decision.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/canonical_fullhands_baseline_pose_review_20260819/visual_decision.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/bodymain_fullhands_pose_review_20260819/visual_decision.json`.
- Results: canonical full fret/strum banks are the first branch that visibly
  engages the hands in a useful way. Canonical main plus full hands is
  non-bipedal/sideways. Accepted full body main plus full hands also falls
  sideways/off-frame in this loose-model hand-overlay path. Generated narrow
  mesh-channel guitar-main plus canonical full hands keeps the body upright and
  is materially better than endpoint-only probes, but still rejects because the
  guitar is too low/through the legs and the right hand remains near the foot.
- Next resume: keep the canonical full fret/strum hand banks. Refine only the
  generated narrow guitar-main frame/placement around that full native hand
  layer. Do not return to endpoint-only/minimal arm-chain probes, and do not
  revisit pelvis/Control_Root.

### 2026-08-19 hand target parent-space diagnosis

- Added `tools/gh3_midori_hand_target_space_report.py` and extended
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py` with
  `--overlay-channel-mode canonical-fret-target-pelvis-rebase`.
- Target-space report:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/meshmain_fullhands_target_space_report_20260819.json`.
  It shows canonical `bone_fret_hand.mesh.pos` fits `bone_pelvis.mesh` space
  better than declared `bone_fret.mesh` space (`40.978` down to `15.074`).
  Canonical strum does not improve under tested alternate parents and remains
  best under declared `bone_strum.mesh` space (`13.162`).
- Compiled the pelvis-rebased fret-target branch structurally:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/fret_pelvis_rebase_contract_report_20260819.json`.
  Result: `LH-FRET=24.589`, `RH-STRUM=13.162`, `SOLVE-DELTA=137.812`.
  This is rejected without visual capture because it is weaker than the already
  captured/rejected `canonical-fret-visible-left-arm-rot` branch
  (`LH-FRET=10.782`) and is therefore unlikely to pass the immediate bipedal
  visual gate.
- Decision record:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/hand_target_space_decision_20260819.json`.
- Next resume: keep the generated narrow mesh-channel guitar-main plus
  canonical full fret/strum hand banks. Refine the generated guitar-main
  frame/placement around that full native hand layer, with immediate rejection
  for non-bipedal/off-frame/guitar-through-legs captures. Do not return to
  endpoint-only probes, minimal arm-chain-only probes, stock target local
  overrides, or canonical/full-body main swaps in the loose hand-overlay path.

### 2026-08-19 guitar-frame pair-fit checkpoint

- Added `--case-name` to `tools/gh3_midori_pose_review.py` so single
  hand-overlay diagnostics can capture only `midori_1_hand_overlay_f010`.
- Added two-point guitar-frame modes to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`:
  `--rotation-mode hand-target-pair-fit` and
  `--placement-mode hand-target-pair-midpoint`. These rotate the
  fret-to-strum target vector toward the visible left-to-right hand vector,
  then translate the target midpoint to the hand midpoint.
- Evidence decision:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/guitar_frame_pairfit_decision_20260819.json`.
- Rejected branches:
  `fullhands_midfit` equalized the contract at `LH=26.405/RH=26.405` but
  visually kept the guitar nearly vertical through the side/legs; source-only
  guitar rotation was a structural reject (`LH=38.837/RH=21.273`); transpose
  quaternion pair-fit was a structural reject (`LH=49.136/RH=9.955`).
- Current best branch:
  `fullhands_pairfit_directquat`. Contract:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/fullhands_pairfit_directquat_contract_report_20260819.json`.
  Visual proof:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/fullhands_pairfit_directquat_pose_review_20260819/visual_decision.json`.
  Metrics are `LH=18.376/RH=9.481`, `SOLVE-DELTA=112.450`, with zero
  suggested follow-up whole-guitar offset. The screenshot is upright/bipedal
  and puts the guitar in a plausible across-body band, but it is not accepted:
  explicit user visual approval is still required and hand/arm contact remains
  rough.
- Next resume: treat direct-quat pair-fit as the current best guitar-main
  frame. Refine hand/arm contact on top of that frame. Do not go back to
  whole-translation midpoint, source-only rotation, transpose-quat pair-fit,
  endpoint-only probes, minimal arm-chain-only probes, or stock target local
  overrides.

### 2026-08-19 pair-fit left-arm merge checkpoint

- Reproduced the direct-quat pair-fit guitar frame, then merged canonical fret
  full-hand channels with visible left-arm channels on top of that exact frame.
- Evidence decision:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pairfit_leftarm_merge_decision_20260819.json`.
- Structural results:
  `pairfit_leftarm_r094` (`canonical-fret-visible-left-arm-rot`) improves the
  contract to `LH=7.507/RH=9.481`, with `SOLVE-DELTA=129.330`. Reach scales
  `0.94`, `1.20`, and `1.50` converge to the same one-frame result.
  Position-only is weaker (`LH=15.038/RH=9.481`). Rot+pos is
  `LH=15.722/RH=9.481`, `SOLVE-DELTA=111.721`.
- Visual results:
  `pairfit_leftarm_r094_pose_review_20260819/visual_decision.json` is current
  best-so-far but not accepted: upright/bipedal, plausible across-body guitar
  band, structurally better left contact, but the visible pose still reads
  rough and needs direct user approval. `pairfit_leftarm_rotpos` rejects
  because it creates an obvious displaced sleeve/arm artifact near the torso.
- Next resume: current best branch is `pairfit_leftarm_r094`. Refine the
  hand/arm silhouette/contact from that branch. Do not use rot+pos, and do not
  return to whole-translation midpoint, source-only rotation, transpose-quat
  pair-fit, endpoint-only probes, minimal arm-chain-only probes, or stock target
  local overrides.

### 2026-08-19 two-sided/source arm diagnostic rejection

- Extended `tools/gh3_midori_guitar_frame_hand_bake_probe.py` with
  `canonical-hands-visible-arm-rot`, `canonical-hands-source-arm-rot`, and
  `source_bridge_local_rotation_to_gh2`.
- Evidence decision:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pairfit_arm_source_vs_aim_decision_20260819.json`.
- Results: solving both visible arms structurally rejects because it preserves
  left contact but worsens the strum side to `LH=7.507/RH=23.300`. Mapping raw
  source arm rotations from source frame 30 also rejects: direct basis is
  `LH=17.682/RH=10.284`, anim basis is `LH=10.260/RH=19.892`, and helper
  basis is `LH=19.267/RH=20.931`.
- No visual captures were taken for these rejects. Current best remains
  `pairfit_leftarm_r094`.
- Next resume: do not continue two-sided visible-arm aim or raw source-arm
  rotation mapping. Preserve the strum-side canonical arm and look for a more
  localized silhouette/contact correction on top of `pairfit_leftarm_r094`.

### 2026-08-19 visible left-hand quat copy rejection

- Added `canonical-fret-visible-left-arm-hand-rot` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`. This mode keeps the r094
  visible left upper/forearm rotation merge and additionally writes
  `bone_L-hand.mesh.quat` from canonical `bone_fret_hand.mesh.quat`.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pairfit_leftarm_handrot_decision_20260819.json`,
  `pairfit_leftarm_handrot_contract_report_20260819.json`, and
  `pairfit_leftarm_handrot_pose_review_20260819/visual_decision.json`.
- Result: the added visible hand quat worsens structural contact to
  `LH=12.562/RH=9.481` and does not visibly improve the rough hand/arm contact.
  Reject this variant.
- Next resume: current best remains `pairfit_leftarm_r094`. Do not continue
  visible left-hand quat copying; seek a different localized silhouette/contact
  correction that preserves r094's left contact and the canonical strum side.

### 2026-08-19 left clavicle aim rejection

- Added `canonical-fret-visible-left-clavicle-arm-rot` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`. This solves
  `bone_L-clavicle.mesh` toward the fret target, then applies the r094-style
  upper/forearm rotation solve.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pairfit_leftclav_decision_20260819.json`
  and `pairfit_leftclav_contract_report_20260819.json`.
- Result: structural reject, `LH=17.670/RH=9.481`,
  `SOLVE-DELTA=136.157`. No visual capture taken.
- Next resume: current best remains `pairfit_leftarm_r094`. Do not continue
  clavicle aim solves; they disturb the shoulder chain more than they help the
  contact contract.

### 2026-08-20 visible left-hand-position-only rejection

- Added `canonical-fret-visible-left-handpos-arm-rot` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`. This keeps the r094
  upper/forearm rotation solve but adds only `bone_L-hand.mesh.pos`, avoiding
  the rejected rot+pos branch's forearm position channel.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pairfit_left_handpos_armrot_decision_20260820.json`,
  `pairfit_left_handpos_armrot_bake_manifest_20260820.json`, and
  `pairfit_left_handpos_armrot_contract_report_20260820.json`.
- Result: structural reject before capture, `LH=19.755/RH=22.861`,
  `SOLVE-DELTA=80.144`. The smaller anchor split is not useful because both
  hand contacts are much worse than r094.
- Next resume: current best remains `pairfit_leftarm_r094`. Do not continue
  visible left-hand position alone; look for a localized correction that keeps
  r094's left contact and canonical strum-side behavior.

### 2026-08-20 source-anim emitted-rotation recheck

- Re-ran the earlier source/body-relative guitar rotation idea after the
  probe's rotation-emission bug had been fixed. The branch used source-anim
  pelvis-delta placement plus source-anim pelvis-relative rotation and verified
  that `bone_pos_guitar.mesh.quat` was emitted.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/sourceanim_rotanim_emitrot_reprobe_decision_20260820.json`,
  `sourceanim_rotanim_emitrot_reprobe_bake_manifest_20260820.json`, and
  `sourceanim_rotanim_emitrot_reprobe_contract_report_20260820.json`.
- Result: structural reject before capture, `LH=37.955/RH=28.296`,
  `SOLVE-DELTA=96.912`.
- Next resume: current best remains `pairfit_leftarm_r094`. Do not continue
  source-bridge pelvis-relative guitar rotation for this pose; look for a
  different shared guitar/target-frame correction or a more direct authored
  visible-guitar/hand bake.

### 2026-08-20 r094 average shared-offset rejection

- Tested a small whole guitar/target-frame nudge on top of
  `pairfit_leftarm_r094`, using the average visible-hand-minus-target residual
  as `--guitar-world-offset -2.127,-0.808,4.955`.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pairfit_leftarm_r094_avgoffset_decision_20260820.json`,
  `pairfit_leftarm_r094_avgoffset_bake_manifest_20260820.json`, and
  `pairfit_leftarm_r094_avgoffset_contract_report_20260820.json`.
- Result: structural reject before capture, `LH=30.404/RH=26.040`,
  `SOLVE-DELTA=70.610`. The lower internal guitar-anchor split is not useful
  because both visible hand contacts are much worse than r094.
- Next resume: current best remains `pairfit_leftarm_r094`. Do not continue
  whole shared-offset nudges around the same pair-fit frame; the next useful
  branch needs a different target-frame/authored-pose correction.

### 2026-08-20 r094 reproduction base audit

- Identified that recent r094 follow-up reprobes used
  `controlroot_stockupper_full_candidate` as the base, while the retained r094
  manifest was built from an older `midori_pairfit_leftarm_mesh_candidate`
  scratch base that has since been cleaned.
- Added diagnostic-only `--suppress-guitar-rotation-output` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py` to reproduce pre-fix
  no-guitar-quat ACP shape without undoing fixed emitted-rotation behavior for
  normal probes.
- Current-source rebuilds with fixed emitted rotation, legacy no-guitar-quat
  direct, and legacy no-guitar-quat transpose do not hash-match the retained
  old mesh-armchain seed. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r094_repro_base_audit_20260820.json`.
- Next resume: current best remains the retained `pairfit_leftarm_r094`
  visual/contract evidence. Do not claim further controlroot-stockupper-base
  reprobes are apples-to-apples r094 refinements unless an explicit correct-base
  r094 candidate is retained or hash-exactly reconstructed.

- Follow-up packaging matrix: building the legacy no-guitar-quat ACP shape
  without `--control-root-pelvis-parent` and with `--move-self 0` exactly
  reproduces the old mesh-armchain seed fret/strum hashes, but not the old main
  hash. A bounded PowerShell search found 15 `gh3_midori_main.milo_ps2` files
  and none matched old mesh seed `255F01FB...` or old r094 main `23DA06BE...`.
  The missing old main remains the blocker for hash-exact r094 reconstruction.

- Resolution: the old mesh-armchain seed main also reproduces when guitar quat
  is emitted with `hmx-quat-mode transpose`, no
  `--control-root-pelvis-parent`, and `--move-self 0`. Exact r094 then
  reproduces by composing that seed main with canonical full fret/strum hand
  banks and running `canonical-fret-visible-left-arm-rot` with
  `hand-target-pair-midpoint`, `hand-target-pair-fit`,
  `rotation-source-basis anim`, `arm-chain-reach-scale 0.94`,
  `hmx-quat-mode direct`, no `--control-root-pelvis-parent`, `--move-self 0`,
  and canonical strum/ui retained.
- Retained exact r094 candidate:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/repro_exact_pairfit_leftarm_r094_candidate_20260820`.
  Main/fret/strum/ui hashes and the one-case contract match retained r094:
  `LH=7.507/RH=9.481`, `SOLVE-DELTA=129.330`.
- Next resume: use the retained exact r094 candidate for apples-to-apples
  follow-up variants. Do not use `controlroot_stockupper_full_candidate` as
  the r094 base.

### 2026-08-20 exact-base visible left-hand-position-only retest

- Reran `canonical-fret-visible-left-handpos-arm-rot` on the corrected exact
  meshmain/full-hands base instead of the wrong stockupper base.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/exactbase_left_handpos_armrot_decision_20260820.json`,
  `exactbase_left_handpos_armrot_bake_manifest_20260820.json`, and
  `exactbase_left_handpos_armrot_contract_report_20260820.json`.
- Result: structural reject before capture, `LH=11.253/RH=9.481`,
  `SOLVE-DELTA=120.815`. It is far less broken than the earlier wrong-base
  reprobe, but still worsens r094's left contact (`7.507`).
- Next resume: current best remains `pairfit_leftarm_r094`. Do not continue
  visible left-hand position alone; exact-base follow-ups need to preserve or
  improve r094's left contact before earning a visual capture.

### 2026-08-20 exact-base visible left-hand-position blend sweep

- Added diagnostic `--visible-hand-pos-blend` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py` and swept blends
  `0, 0.25, 0.5, 0.75, 1` on the hash-exact r094 base.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/exactbase_handpos_blend_sweep_decision_20260820.json`.
- Result: structural reject before capture. Blend `0` exactly reproduces r094
  (`LH=7.507/RH=9.481`, `SOLVE-DELTA=129.330`); every positive blend worsens
  left-hand contact (`7.706`, `8.491`, `9.722`, `11.253`) while right-hand
  contact stays at `9.481`.
- Next resume: current best remains `pairfit_leftarm_r094`. Do not continue
  visible left-hand position blending; the useful next branch needs to preserve
  r094's left contact and improve the visible silhouette/target relationship by
  some other exact-base guitar/pose correction.

### 2026-08-20 exact-base source target pair-fit rejection

- Added diagnostic `--pair-fit-target-mode` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py` so
  `hand-target-pair-fit` rotation and `hand-target-pair-midpoint` placement can
  use source palm or source IK guitar-local endpoints instead of the current
  GH2 proxy pair.
- Added diagnostic `--emit-hand-target-proxies` so the selected
  `--hand-target-mode` proxy positions can be emitted into fret/strum clipsets
  for structural measurement.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/exactbase_source_target_pairfit_decision_20260820.json`.
- Result: structural reject before capture. Post-fit-only stock/source target
  modes all collapsed to `LH=45.953/RH=13.278` or worse. Source pair-fit +
  emitted proxy variants across direct/anim/helper bases were also worse than
  r094; best was source IK direct at `LH=24.206/RH=11.930`,
  `SOLVE-DELTA=151.290`.
- Next resume: current best remains `pairfit_leftarm_r094`. Do not continue
  source-palm/source-IK pair-fit target modes for this pose; the next useful
  branch needs a different exact-base guitar/pose correction that preserves
  r094's contact gate.

### 2026-08-20 exact-base local rotation-correction rejection

- Swept small post-local `--rotation-correction` variants on the exact r094
  pair-fit base: `x/y/z` axes at `15`, `345`, `30`, and `330` degrees. The
  sweep retained canonical strum/ui and rebuilt only the temporary main/fret
  clipsets needed for one-case structural reports.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/exactbase_rotation_correction_sweep_decision_20260820.json`.
- Result: structural reject before capture. Every tested local-axis correction
  catastrophically regressed left-hand contact; best was `y-30-post` at
  `LH=33.941/RH=13.096`, `SOLVE-DELTA=121.136`.
- Next resume: current best remains `pairfit_leftarm_r094`. Do not continue
  simple local-axis post rotation-correction sweeps around r094; a useful
  visual-roll branch would need to preserve the target endpoints, not rotate the
  local guitar frame away from the hand-contact solve.

### 2026-08-20 exact-base pair-axis roll visual rejection

- Added diagnostic `--pair-axis-roll-degrees` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`. It applies a world-space
  roll around the final fitted `bone_fret_hand.mesh` /
  `bone_strum_hand.mesh` proxy axis after r094 pair-fit placement.
- Structural sweep evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/exactbase_pair_axis_roll_decision_20260820.json`.
- Result: most angles were structural rejects. The best metric variant was
  `+90` at `LH=7.007/RH=11.038`, `SOLVE-DELTA=122.381`, so it earned a
  no-ISO local viewer capture.
- Visual evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pairaxisroll_p90_pose_review_20260820/midori_1_hand_overlay_f010.bmp`.
- Visual decision: reject. The pose is upright/bipedal, but the guitar is
  behind/through the torso and the strum-hand relationship is less coherent than
  r094.
- Next resume: current best remains `pairfit_leftarm_r094`. Do not continue
  pair-axis roll sweeps around r094; future exact-base work needs to move the
  guitar/hand relationship without hiding the guitar behind the torso.

### 2026-08-20 exact-base authored twist roll rejection

- Derived signed `--pair-axis-roll-degrees` angles by projecting source/stock
  authored guitar frame rows around r094's final fitted
  `bone_fret_hand.mesh` / `bone_strum_hand.mesh` axis.
- Sources tested: source relative guitar frame in direct/anim/helper bases,
  source world guitar frame in direct/anim/helper bases, stock local guitar
  frame, and stock attach-world frame.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/exactbase_authored_twist_roll_decision_20260820.json`.
- Result: structural reject before capture. Best summed contact was stock local
  row1 at `LH=18.753/RH=10.530`, and best right-hand-only result was source
  world helper row1 at `RH=8.928` but with `LH=39.439`.
- Next resume: current best remains `pairfit_leftarm_r094`. Do not continue
  authored source/stock twist projection rolls around r094; future exact-base
  work needs a different correction than pair-axis twist, likely a hand/guitar
  depth or proxy-frame relationship change that keeps the guitar in front of
  the torso.

### 2026-08-20 exact-base stock prop pair-fit rejection

- Extended diagnostic `--pair-fit-target-mode` with
  `stock-prop-comp-strings` and `stock-prop-comp-hand-locals`, so pair-fit
  rotation/placement can use the same stock prop-compensated targets that
  `--emit-hand-target-proxies` writes into fret/strum clipsets.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/exactbase_stock_prop_pairfit_decision_20260820.json`.
- Result: structural reject before capture. The stock strings variant measured
  `LH=50.678/RH=27.729`, `SOLVE-DELTA=122.482`; the stock hand-locals variant
  measured `LH=51.241/RH=89.460`, `SOLVE-DELTA=94.947`.
- Next resume: current best remains `pairfit_leftarm_r094`. Do not continue
  stock prop-compensated pair-fit target modes around r094; target-pair
  selection is not the remaining fix.

### 2026-08-20 exact-base strum target guitar-rebase review candidate

- Added diagnostic `--rebase-strum-target-to-guitar` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`. It interprets canonical
  `bone_strum_hand.mesh.pos` as `bone_pos_guitar.mesh` local and solves it back
  under `bone_strum.mesh`, matching the exact r094 target-space report where
  strum fit improved from `9.481` to `8.135` under guitar-space interpretation.
- Important base correction: a first r1 attempt incorrectly fed the retained
  final r094 candidate back into the probe, double-applying r094 and producing
  invalid `LH=45.953` data. Reconstructed the pre-r094 exact
  meshmain/fullhands base from the seed recipe, verified that rebuilding r094
  from it reproduced exact main/fret/strum/ui hashes and
  `LH=7.507/RH=9.481`, then reran the strum rebase from that corrected base.
- Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/exactbase_strum_guitar_rebase_decision_20260820.json`,
  `exactbase_strum_guitar_rebase_candidate_20260820`,
  `exactbase_strum_guitar_rebase_bake_manifest_20260820.json`, and
  `exactbase_strum_guitar_rebase_r2_contract_report_20260820.json`.
- Result: retained as a direct-review candidate, not accepted. Metrics:
  `LH=7.507/RH=8.135`, `SOLVE-DELTA=130.699`. Main/fret/UI hashes match r094;
  only strum changes (`733FEA4E...`).
- Visual evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/strum_guitar_rebase_pose_review_20260820/midori_1_hand_overlay_f010.bmp`.
  The capture is upright/bipedal and visually close to r094; direct user visual
  approval is still required.
- Next resume: treat `exactbase_strum_guitar_rebase` as the current review
  candidate pending user approval. Future variants must reconstruct/verify the
  pre-r094 exact base first; do not feed final r094 candidates back into the
  probe as the input base.

### 2026-08-20 Control_Root rootlocal body-animation rejection

- Rechecked the pelvis/Control_Root branch after the rootlocal output candidate
  was captured. The visual proof is rejected before user approval:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/rootlocal_output_bipedal_pose_review_20260820/visual_decision.json`.
  Most main body poses are sideways or horizontal; only the accessory fallback
  is upright, so the body animation set is not coherent enough to promote.
- Corrected report-only evidence now agrees with the visual rejection. Fresh
  `rootlocal` medium-idle frame 60 reports `max_pose_error=32.328456`,
  `right_shin_dot=-0.501932`, `left_shin_dot=-0.724448`, and
  `min_child_aim=-0.607498`. Attack-left frame 30 reports
  `max_pose_error=38.529800` and `min_child_aim=-0.497850`.
- Neighbor Control_Root policies `rootlocal-rightshingate`, `rootbasis`,
  `pelvismeshpre`, and `pelvismeshpreinv` were rerun report-only on
  medium-idle frame 60 and attack-left frame 30. None is promotable; the
  right-shin gate only improves isolated shin/ankle numbers and still leaves
  high global pose error or failed child aim.
- Re-ran the ihatecompvir source audit:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/ihatecompvir_bridge_audit_rerun_20260820.json`.
  Status remains `reference_ready_direct_writer_needed`: GLB/pose JSON is an
  acceptable automated bridge, but public `glTFMilo` is not a GH2 PS2 drop-in
  final converter. Keep final output on the local GH2
  `CharClipSet`/`CharClipSamples` writer path, using MiloLib/GH2 notes as
  reference.
- No GH2 ISO was used for the capture; runtime proof used
  `gh2_ps2_hybrid_assets/GEN` plus loose DLC only at Idle/low priority. The
  temporary candidate and backup directories were removed, and the canonical
  loose DLC anim/model hashes were restored.
- Next resume: do not promote or deploy rootlocal/rootbasis/rightshingate or
  pelvismeshpre variants. Continue at the pelvis/Control_Root basis problem
  with an orientation-aware GLB/pose-bridge-to-target solve; visual orientation
  must be a gate before hand/guitar work is treated as body-animation progress.

### 2026-08-20 Control_Root bridge-convention direction

- Added `tools/gh3_midori_source_bridge_convention_audit.py` and evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/source_bridge_convention_audit_20260820.json`
  plus
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/controlroot_bridge_convention_direction_20260820.json`.
- Result: the older visually passing `modelparentcomp_pinned_forcepartial_nostock`
  body proof and the rejected rootlocal proof use different source-pose bridge
  conventions. All five passing bridges have `force_partial_anims=true`.
  The rejected fresh/rootlocal bridge family carries large `Control_Root` world
  drift before staging: medium-idle `129.685715`, attack-left `94.270244`, fast
  jump `138.894187`, fast solo `95.243263`, and transition-out up to
  `124.123255`. Downstream common-bone pose deltas reach `217.555184` units
  and `158.302445` degrees.
- Fixed `tools/gh3_midori_visual_orientation_diagnostic.py` so explicit
  `--target-image` values replace its defaults. Re-ran orientation evidence:
  the old pinned-forcepartial/no-stock model-parent-compensated medium-idle
  image passes (`upright_score=2.328571`), while the rejected rootlocal
  medium-idle image fails (`upright_score=0.915567`).
- Next resume: do not continue the rejected fresh/world-root bridge convention
  for body animation. Build the next body candidate from pinned
  forcepartial/no-stock source bridges, keep the model-parent compensation for
  the deployed `Control_Root -> Bone_Pelvis` hierarchy, and then move that
  diagnostic matched-model/main proof toward ordinary loose-DLC packaging only
  after the orientation gate remains green.
- A concrete no-deploy pipeline recipe is recorded at
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pinned_forcepartial_modelparentcomp_next_build_recipe_20260820.json`.
  Use it as the next build step unless newer evidence supersedes it.

### 2026-08-20 pinned forcepartial/model-parent r2 ordinary-DLC rejection

- The recorded recipe's original `--stock-bind-scope none` was invalid for
  `tools/gh3_midori_model_bundle.py`; the corrected r2 no-deploy build used
  `--stock-bind-scope upper-limbs-guitar`, remained at Idle/low priority, and
  skipped packaging/deploy/runtime proof until loose-DLC capture.
- The corrected build exited 0 and produced the expected analysis MILOs.
  Model-parent ACP compensation reported `status=compensated`,
  `clip_count=331`, `copied_files=331`, parent `Control_Root`, and pelvis
  channel `bone_pelvis.mesh`.
- Ordinary loose-DLC visual capture used only `gh2_ps2_hybrid_assets/GEN` plus
  loose DLC, no ISO. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pinned_forcepartial_modelparentcomp_pose_review_20260820/pose_review_manifest.json`
  and contact sheet
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pinned_forcepartial_modelparentcomp_pose_review_20260820/contact_sheet.png`.
- Result: hard visual reject before user approval. The pose review reports
  `status=failed`, `proofs=9`, `failures=9`; direct inspection shows only the
  accessory capture is upright-ish, while the actual animation captures are
  mostly horizontal/off-floor and not coherent bipedal poses.
- The live loose DLC was restored immediately after capture and the temporary
  visual backup was removed. Restored hashes are recorded in
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pinned_forcepartial_modelparentcomp_candidate_decision_20260820.json`.
- Next resume: do not promote this matrix-local/Control_Root model-parent
  compensation candidate. Continue diagnosis one layer earlier at the
  source-to-target orientation basis; a GLB/pose JSON bridge is acceptable as
  an automated intermediate, but final MILO output still needs the local GH2
  PS2 `CharClipSet`/`CharClipSamples` writer path.

### 2026-08-20 pinned forcepartial/model-parent r3 contract fix

- Found the r2 ordinary-DLC contract bug: `--model-parent-compensate-acp`
  patched animation samples for a `Control_Root -> bone_pelvis.mesh` replay
  model, but `tools/gh3_midori_build_pipeline.py` only passed
  `--control-root-pelvis-parent` when the explicit flag was present. The r2
  model bundle manifest therefore recorded `control_root_pelvis_parent=false`
  while the ACP compensation report assumed parent `Control_Root`.
- Patched `tools/gh3_midori_build_pipeline.py` with
  `effective_control_root_pelvis_parent(args) =
  args.control_root_pelvis_parent or args.model_parent_compensate_acp`, and
  used that effective contract for bridge gates, model bundle, post-comp disk
  gate, and final clipset packing. Raw ACP staging remains pre-compensation
  when the separate model-parent compensation pass is enabled.
- Reran the corrected no-deploy r3 build at Idle priority, with no package,
  deploy, runtime proof, or ISO. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pinned_forcepartial_modelparentcomp_contractfix_r3_build_20260820.log`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pinned_forcepartial_modelparentcomp_contractfix_r3_decision_20260820.json`.
  The model bundle manifest now records `control_root_pelvis_parent=true`.
- Candidate hashes:
  main `003B0C61512388F9F6DBFB02099EECF938360A67DE3EE51B3A7A572680637981`,
  fret `D697C06758BAD5F0544EA0A249A9353001D9AC1EC4D44B9D4C591367A05EF957`,
  strum `B1FA33BC691B15FF282CBF867078B824F54BDDEE4A8B6B8547E5B3EE7D87DB8E`,
  ui `0D0AA527298F92D0EC1FDE9B5AEBCB661BD7EE84DD2DA57C4DCB5528C674FDC9`,
  model1 `F08B6A02D5C172517FA5BD737C481A5DBA9EE5221FD7193CBC78727089A34F50`,
  model2 `9BB01AAEF7CBC1C1931364A19343282B5B2B7410CAEEEDF155225A4518AC3FF5`.
- Ordinary loose-DLC capture used only `gh2_ps2_hybrid_assets/GEN` plus loose
  DLC at Idle/low priority. Live loose DLC was restored immediately afterward
  and the temporary backup was removed. Visual evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pinned_forcepartial_modelparentcomp_contractfix_r3_pose_review_20260820/contact_sheet.png`.
- Result: retain r3 as the current body-orientation review candidate, not a
  final acceptance. The initial camera-distance-150 proof had 9 proofs and 1
  framing failure, only `midori_1_accessory_acc01_f030`, due to the raised
  guitar clipping the top of the review image. A full 9-case rerun at
  camera-distance 165 passed all checks:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pinned_forcepartial_modelparentcomp_contractfix_r3_pose_review_cam165_20260820/pose_review_manifest.json`.
  Contact sheet:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/pinned_forcepartial_modelparentcomp_contractfix_r3_pose_review_cam165_20260820/contact_sheet.png`.
  Direct Codex inspection shows all nine ordinary loose-DLC captures are
  upright coherent bipedal rather than horizontal/non-bipedal. Camera-165
  orientation diagnostic passed with target scores `1.817073`, `1.651064`,
  and `1.66791`.
- Next resume: ask for/directly obtain user visual approval on the r3 body
  contact sheet before packaging/deploying this as the new body base. If the
  user accepts the body, continue hands/guitar refinement from r3; do not
  return to r2 or pelvis-only basis sweeps unless new evidence contradicts
  this contract fix.

### 2026-08-20 r3 hand/guitar refinement probes

- Built a temporary flat r3 candidate from the current analysis MILOs and ran
  the hand-overlay contract report. Baseline r3 hand/guitar is not acceptable:
  `LH-FRET=41.731`, `RH-STRUM=16.112`, `SOLVE-DELTA=103.315`, matching the
  visible detached-hand problem in the camera-165 hand-overlay frame.
- The old exactbase hand candidate was confirmed to be a tiny one-frame probe,
  not a full animation-set candidate, so its files must not be copied into the
  final DLC. It remains useful as a recipe. Reapplying that recipe to r3
  (`hand-target-pair-midpoint`, `hand-target-pair-fit`,
  `canonical-fret-visible-left-arm-rot`, mesh channels, strum target rebase)
  improves r3 to `LH=10.926/RH=9.275`, but direct capture still shows the
  guitar crossing the torso/face area and the hands/arms are awkward.
- Ran compact r3 sweeps over hand target spaces, offsets, and two-arm overlay
  modes. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_hand_guitar_probe_decision_20260820.json`.
  Non-current target modes are structurally bad on r3. The best structural
  lead is `canonical-hands-visible-arm-rot` with current-proxy pair-fit,
  strum target rebase, and guitar offset `1.192,0.085,-6.376`, scoring
  `LH=4.876/RH=4.730`.
- Visual evidence for that best structural lead:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_twoarm_down6_pose_review_20260820/midori_1_hand_overlay_f010.bmp`.
  Decision: reject as final for now. The body remains upright, but the
  guitar/right-arm silhouette is still not clean enough for user approval.
- Live loose DLC was restored after every visual probe, no ISO was used, and
  temporary candidate folders were removed. Next resume should refine the
  two-arm/down6 visual silhouette before attempting to generalize it into the
  full r3 animation pipeline.

### 2026-08-20 r3 pair-axis roll hand/guitar sweep

- Ran a bounded pair-axis roll sweep from the `twoarm_down6` hand/guitar
  recipe using current proxies, `canonical-hands-visible-arm-rot`, strum target
  rebase, and guitar offset `1.192,0.085,-6.376`. Evidence is recorded in
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_hand_guitar_probe_decision_20260820.json`.
- Numeric results did not beat the no-roll structural lead. The strongest
  visual batch evidence is
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_roll_visual_batch_20260820/contact_sheet.png`.
  Direct inspection: `roll_m15` is the only plausibly coherent bipedal pose in
  that four-capture batch, but it is still not promotable because the guitar is
  too high/across the torso and the right-hand/prop silhouette remains wrong.
  `no_roll`, `roll_p15`, and `roll_m30` are immediate visual rejects.
- Next resume should stop treating pair-axis roll as the main missing variable.
  Continue at the guitar/prop local basis or the full-pipeline hand-target
  application path, then prove any candidate through ordinary loose DLC only
  (`gh2_ps2_hybrid_assets/GEN` + `gh2_ps2_hybrid_assets/DLC`) at low priority.

### 2026-08-20 r3 rotation/model-basis hand-guitar rejection

- Ran a tiny one-frame rotation-application probe from the `roll_m15` lead.
  Evidence is in
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_rotation_application_probe_20260820`.
  Default transpose quaternion storage reproduces the roll-sweep lead
  (`LH=8.821/RH=10.798`); `--hmx-quat-mode direct` regresses the right hand
  (`RH=29.102`), and suppressing the guitar rotation channel regresses the
  left hand (`LH=43.611` for m15, `LH=45.568` for no-roll). Do not continue
  direct Hmx-quat or suppressed-guitar-rotation variants for this pose.
- Built model-only variants using `--preserve-guitar-attach-local`,
  `--compensate-guitar-helper-for-main-anchor`, and both together against the
  same `m15` animation candidate. Evidence is in
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_model_basis_probe_20260820`
  and visual contact sheet
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_model_basis_visual_batch_20260820/contact_sheet.png`.
  All rebuilt model-basis variants measured the same (`LH=9.409/RH=34.878`)
  and rendered essentially the same bad high/across-torso guitar silhouette.
  None is promotable.
- Live loose DLC was restored to canonical hashes after capture, no ISO was
  used, and the local ihatecompvir audit remains reference-only
  (`reference_ready_direct_writer_needed`, not a drop-in GLB-to-MILO writer).
  Next resume should investigate the hand-target application path or a
  source-authored guitar/hand local-frame bridge feeding the existing direct
  MILO writer.

### 2026-08-20 r3 hand-target application rejection

- Added diagnostic flag `--rebase-fret-target-to-pelvis` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`, mirroring the existing
  strum guitar-rebase path for two-hand canonical probes. The r3 base hand
  target-space report shows why this was worth testing: the fret target sample
  fits `bone_pelvis.mesh` much better than its declared `bone_fret.mesh` parent
  (`14.060` visible-hand distance versus `41.731`), while the strum target is
  still best under `bone_strum.mesh`.
- Result: structural reject. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_fret_pelvis_rebase_probe_20260820`.
  Rebased fret-pelvis variants worsen left contact; the known no-roll
  strum-rebase lead remains best (`LH=4.876/RH=4.730`).
- Added diagnostic `pair-fit-target-mode canonical-emitted-proxies` so the
  guitar can aim at the final canonical proxy worlds rather than stale current
  proxies. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_canonical_emitted_pairfit_probe_20260820`.
  Result: structural reject. Pair-fitting to final emitted canonical targets
  moves the error between hands but does not fix r3.
- Tested source-authored arm rotations with
  `canonical-hands-source-arm-rot` across direct/anim/helper bases. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_source_arm_rot_probe_20260820`.
  Result: structural reject; all are worse than the visible-arm lead.
- Next resume should not repeat emitted fret-pelvis rebase, canonical-emitted
  pair-fit, or source-arm-rot sweeps. Continue with a constrained
  source-authored guitar/hand local-frame bridge or final guitar/world offset
  solve that preserves the r3 visible-arm lead while improving the prop
  silhouette.

### 2026-08-20 r3 m15 final-offset refinement

- Ran a bounded final guitar offset sweep around the only coherent r3 roll
  visual lead (`m15`, `canonical-hands-visible-arm-rot`, current proxy
  pair-fit, strum target rebase). Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_m15_offset_visual_probe_20260820`
  and contact sheet
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_m15_offset_visual_batch_20260820/contact_sheet.png`.
  Lowering the guitar helped; `m15_lower_down10` improved to
  `LH=5.521/RH=3.380` and looked better than baseline because the guitar no
  longer cut across the face as badly.
- Ran a tighter down10 refinement. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_m15_down10_refine_probe_20260820`
  and visual contact sheet
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_m15_down10_refine_visual_batch_20260820/contact_sheet.png`.
  Best current hand/guitar visual lead is `lessx_down10`, offset
  `-0.808,0.085,-10.376`, roll `-15`, with `LH=4.156/RH=2.882`. `down11` is
  close visually.
- Decision: retain `lessx_down10` as the current refinement lead, not a final
  approval candidate. It is more coherent than the prior m15/down6 sheet, but
  the guitar/hand relationship is still awkward and too low around the
  hands/skirt line. Next resume should preserve this lowered silhouette while
  adding a constrained prop/hand local-frame or right-hand/guitar-body
  alignment correction.

### 2026-08-20 r3 lessx/down10 micro refinement

- Ran a tighter micro sweep around `lessx_down10`, varying roll and small
  final offsets while keeping the same r3 visible-arm recipe and strum target
  rebase. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_lessx_down10_micro_refine_probe_20260820`.
  Best structural metric is `roll_m10`, offset `-0.808,0.085,-10.376`, roll
  `-10`, with `LH=3.644/RH=2.015`.
- Ordinary loose-DLC visual batch:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_lessx_down10_micro_visual_batch_20260820/contact_sheet.png`.
  Direct visual decision: `roll_m10` is the current best hand/guitar refinement
  lead. It preserves the lowered `lessx_down10` silhouette and looks slightly
  less collapsed around the right hand than `lead_r15`. `roll_m12` and `back2`
  are more awkward/too low.
- Decision: retain `roll_m10` as the current refinement lead, not a final
  approval candidate. Next resume should preserve this silhouette and solve
  the remaining prop/body overlap with a constrained guitar-body/right-hand
  local-frame correction, not broad roll/parent/model/quaternion sweeps.

### 2026-08-20 r3 roll_m10 post-prop translation rejection

- Added diagnostic `--post-guitar-world-offset` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`. This applies an offset
  only to the emitted main `bone_pos_guitar` channel after the hand/target
  solve, so the visible-arm overlay is preserved while testing whether the
  remaining overlap is just final prop translation.
- Ran a bounded post-prop translation probe from the `roll_m10` lead. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_rollm10_postprop_probe_20260820`
  and loose-DLC contact sheet
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_rollm10_postprop_visual_batch_20260820/contact_sheet.png`.
  Visual decision: reject simple post-translation. `post_none` remains at least
  as good as the tested offsets; `post_xleft1` is similar, while `post_back2`
  and `post_up1` make the guitar/hand relationship more awkward.
- Current lead remains `roll_m10`, offset `-0.808,0.085,-10.376`, roll `-10`.
  Next resume should test a constrained post-guitar local rotation or
  right-hand/guitar-body frame correction while preserving `roll_m10`, not more
  broad post-translation sweeps.

### 2026-08-20 r3 roll_m10 post-rotation rejection

- Added diagnostic `--post-guitar-rotation-correction` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`. This changes only the
  emitted main `bone_pos_guitar` rotation after hand/target solving, preserving
  the `roll_m10` visible-arm overlay.
- Ran a bounded post-rotation probe from the `roll_m10` lead. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_rollm10_postrot_probe_20260820`
  and loose-DLC contact sheet
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_rollm10_postrot_visual_batch_20260820/contact_sheet.png`.
  Visual decision: reject as a fix. `y_p10` is tolerable but not better than
  baseline; `x_p10` and `z_p10` make the prop relationship worse.
- Current lead remains `roll_m10` with no post-rotation correction. Since
  simple post-translation and post-rotation both failed, next resume should
  inspect a source-authored guitar-body-to-visible-hand frame diagnostic across
  frames or start limited multi-frame generalization of the current lead to see
  whether the remaining problem is pose-specific.

### 2026-08-20 r3 roll_m10 frame-stability rejection

- Added diagnostic `--case-name` and `--case-frame` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py` so the same hand/guitar
  recipe can be measured at specific frames of the review case.
- Ran the current `roll_m10` lead (`canonical-hands-visible-arm-rot`, current
  proxy pair-fit, strum target rebased to guitar, roll `-10`, final offset
  `-0.808,0.085,-10.376`) at frames `0/10/20/30/40`. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_rollm10_frame_sweep_probe_20260820`
  and loose-DLC contact sheet
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_rollm10_frame_sweep_visual_batch_20260820/contact_sheet.png`.
- Structural metrics prove the failure is frame-specific: frame10 remains the
  useful point (`LH=3.644/RH=2.015`), but frame20 jumps to
  `LH=38.529/RH=9.791`, frame30 to `LH=54.935/RH=18.716`, and frame40 to
  `LH=57.163/RH=12.544`.
- Direct visual decision: reject `roll_m10` as an animation-set fix. frame10 is
  the only coherent capture; frame00 is marginal, and frame20/frame30 visibly
  detach/collapse the guitar-hand relationship.
- Live proof used local `gh2_ps2_hybrid_assets/GEN` plus ordinary loose DLC
  only, not the GH2 ISO, and capture/build commands were run at Idle priority.
  The loose DLC hashes/layout were verified after capture; outfit 2 correctly
  lives at `char/gh3_midori_2/og/gen/gh3_midori_2.milo_ps2`.
- Next resume should preserve `roll_m10` only as one-frame diagnostic evidence
  and move to a frame-stable source-authored guitar-body-to-visible-hand
  local-frame bridge or equivalent automated GLB/ACP-to-MILO constraint. Do
  not continue broad one-frame post-translation/post-rotation/roll sweeps.

### 2026-08-20 r3 source-local frame bridge rejection

- Exported a bounded five-frame GH3 attack-left source bridge for frames
  `0/10/20/30/40` with guitar body, IK helpers, palms, visible arm bones, and
  finger bases. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/source_bridge_attack_arm_guitar_framesweep_20260820/`.
  The Blender bridge wrote the pose JSON/GLB; batch validation failed only on
  a nonexistent requested `Bone_Hand_Index_Top_R`, not on the guitar/hand
  records used by the diagnosis.
- Source-guitar contract passes for the bridge:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/source_bridge_attack_arm_guitar_framesweep_20260820/source_guitar_contract_report.json`.
  It also exposes why the raw source-local route is weak: IK helper offsets are
  effectively static across frames.
- Added `tools/gh3_midori_source_local_frame_bridge_report.py`, which compares
  source guitar-local palm/IK offsets to r3 visible GH2 hand locals over the
  same frames and searches signed-permutation plus uniform scale/offset
  calibrations. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_source_local_frame_bridge_report_20260820.json`.
  Best both-hand fits are still poor: IK helper `rms=9.160/max=18.098`; palm
  `rms=12.415/max=16.857`. Even per-side palm fits remain too loose
  (`left rms=9.542`, `right rms=5.772`) for visual promotion.
- Built temporary structural candidates for raw source-local IK/palm modes and
  source-IK plus the old roll/offset. All are structural rejects:
  `ik_direct` left errors stay `61.661`-`76.233`, `palm_direct` left errors
  stay `41.388`-`68.574`, and `ik_direct_rollm10` left errors stay
  `52.447`-`64.410`.
- Decision: reject raw source guitar-local IK/palm offsets and simple affine
  calibration as the frame-stable bridge. Next work should inspect canonical
  hand clip target-space sampling or build a richer per-frame constraint using
  visible-hand deltas directly. Do not repeat raw source-local IK/palm mode
  sweeps.

### 2026-08-20 r3 canonical target-space rejection

- Added `tools/gh3_midori_canonical_hand_target_space_report.py` to rank
  parent interpretations for canonical `bone_fret_hand.mesh.pos` and
  `bone_strum_hand.mesh.pos` against r3 visible hands over frames
  `0/10/20/30/40`. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_canonical_hand_target_space_report_20260820.json`.
- Plausible rig parents are still too far from the visible hands. Best
  plausible left parent is `bone_pos_guitar.mesh`
  (`mean=25.055/max=31.552`); best plausible right parent is
  `bone_pelvis.mesh` (`mean=22.328/max=26.817`). The lowest raw distances came
  from treating samples as visible-hand-local (`bone_L-hand.mesh` mean `5.667`,
  `bone_R-hand.mesh` mean `4.321`), so that interpretation was tested through
  the real ACP/MILO emission path.
- Added diagnostic flags `--rebase-fret-target-to-visible-hand` and
  `--rebase-strum-target-to-visible-hand` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`. Bounded structural probe:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_visible_hand_target_rebase_probe_20260820/contract_summary.tsv`.
- Result: structural reject. `vh_emitpair` keeps left errors around
  `48.626`-`52.402` and right errors `30.543`-`35.618`; `vh_currentpair`
  keeps left errors around `72.136`-`75.386`; adding the old roll/offset makes
  the right side worse (`43.233`-`47.301`). No visual capture was warranted.
- Decision: do not repeat parent-only target rebases. The visible-hand-local
  interpretation is tempting in a static report but becomes circular after
  emission/arm solving. Next work should inspect the ordering/feedback between
  visible arm solving and target proxy emission, or solve visible-hand deltas
  directly after the arm pose is fixed.

### 2026-08-20 r3 corrected-candidate post-arm rejection

- Corrected a same-turn diagnostic mistake: the first canonical target-space
  and post-arm batches used the probe's stale default candidate for transform
  simulation while contract candidates copied current r3 model MILOs. The
  corrected reruns explicitly use `analysis/gh3_midori_gh2_milos` for both
  probe simulation and structural contract.
- Corrected target-space report:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_canonical_hand_target_space_report_candidate_20260820.json`.
  With the correct r3 candidate, canonical fret `.pos` ranks closest as
  `bone_L-foreArm.mesh` local (`mean=4.149/max=4.149`), and canonical strum
  `.pos` ranks closest as `bone_R-hand.mesh` local (`mean=4.321/max=4.321`).
- Added diagnostic `--align-target-proxies-to-post-arm-hands` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py` and ran corrected
  structural evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_postarm_proxy_align_candidate_probe_20260820/contract_summary.tsv`.
  Result: still rejected. `postarm_current` is acceptable-ish at frames 0/10
  (`LH=8.518/9.271`) but collapses at frames 20/30/40
  (`LH=34.516/35.203/38.050`). `postarm_current_rollm10` has a good frame10
  (`LH=7.607/RH=2.257`) but collapses after that.
- Added diagnostic `--rebase-fret-target-to-visible-forearm` and tested the
  corrected parent-space interpretation with right target rebased to visible
  hand:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_forearm_hand_target_rebase_probe_20260820/contract_summary.tsv`.
  Result: structural reject. The best variants trade sides instead of fixing
  both hands; frame20+ still fails.
- Decision: no visual capture warranted. Stop parent/offset/proxy alignment
  sweeps here. Next route should inspect why the visible-arm two-bone solve
  stops following the moving left target after frame10, probably by reporting
  target reachability, elbow plane, upper/forearm aim vectors, and emitted
  local rotation continuity across frames.

### 2026-08-20 r3 visible-arm rot+pos rejection

- Added `tools/gh3_midori_visible_arm_solve_report.py` to report two-bone
  reachability and compare simulated rot+pos versus rot-only emission. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_visible_arm_solve_report_20260820.json`.
  The current strum-guitar target recipe has clamped left/right targets across
  frames, so it is mostly outside practical arm reach. The forearm/right-hand
  recipe looks reachable in isolated solve math, but that result does not
  survive final emitted ACP/MILO contract measurement.
- Added `--emit-visible-arm-positions` to
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py` so canonical full hand
  clips can also emit solved visible forearm/hand `.pos` channels. Structural
  evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_emit_visible_arm_positions_probe_20260820/contract_summary.tsv`.
- Result: structural reject. `rotpos_current` improves early frames
  (`LH=6.690/RH=16.644` at frame0, `LH=4.017/RH=10.340` at frame10) but still
  collapses at frames 20/30/40 (`LH=31.453/32.867/39.820`). `rollm10` and
  corrected forearm/right-hand parent variants are worse or trade hands.
- Sanity check: the failing `rotpos_current` frame20 candidate really contains
  emitted `bone_L-foreArm.mesh.pos` and `bone_L-hand.mesh.pos` channels, but
  final contract reconstruction still places `bone_L-hand.mesh` 31.453 from
  `bone_fret_hand.mesh`.
- Decision: reject canonical visible-arm rot+pos emission as the fix. Next
  route should compare bake-side world reconstruction against final
  contract-side reconstruction for the same emitted samples, especially
  model/main/fret/strum sample ordering and prop override application. Do not
  continue arm-position emission, parent rebase, or roll/offset sweeps without
  first closing that reconstruction mismatch.

### 2026-08-20 r3 reconstruction comparison closed

- Added `tools/gh3_midori_reconstruction_compare_report.py` and bake-manifest
  final replay diagnostics. Evidence root:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_reconstruction_compare_rotpos_current_f20_20260820`.
- For the exact failing `rotpos_current` frame20 case, the probe's own final
  replay already reproduces the rebuilt-MILO contract failure:
  `LH=31.453/RH=15.784`. The rebuilt MILO sample values preserve the emitted
  visible-arm `.pos` channels, and changing sample order/viewer override order
  does not repair the result.
- Tested two immediate hypotheses:
  `--recompute-visible-arm-positions-after-arm-rot` rejects
  (`LH=38.697/RH=12.418` on frame20), and `--hmx-quat-mode direct` is worse in
  probe replay (`LH=34.980/RH=29.050`).
- Added `--solve-visible-hands-after-final-replay` as a diagnostic only. It
  proves a last-step matrix-local hand endpoint solve can make contract metrics
  pass after MILO rebuild (`LH/RH ~= 0` on frame20), but the implied limb
  lengths are non-bipedal: five-frame probe replay has
  `LForeHand=14.46/11.72/27.46/38.44/38.69` and
  `RForeHand=15.32/1.58/17.77/17.33/12.30` for frames 0/10/20/30/40.
- Decision: reconstruction mismatch is closed. Do not spend more time on GLB,
  ACP/MILO bridge, sample ordering, viewer prop ordering, or unconstrained
  visible-hand endpoint solving for this branch. Resume from the current visual
  lead (`roll_m10`, lessx/down10) and target a constrained guitar-body/right-hand
  local-frame correction or source-authored guitar/hand frame bridge.

### 2026-08-20 r3 GLB/MILO bridge audit and constrained-reach rejection

- Refreshed local ihatecompvir bridge audit:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_ihatecompvir_bridge_audit_20260820.json`
  and `analysis/gh3_midori_ihatecompvir_bridge_audit.json`.
  Result: public `glTFMilo` is not a drop-in GH2 PS2 converter
  (`xbox/ps3`, RB-era games, TransAnim path), but MiloLib/GH2 notes are
  reference-ready for GH2 PS2 CharClipSet/CharClipSamples. Use GLB/pose JSON as
  an automated intermediate only if it feeds the existing direct GH2 writer or
  a new CharClipSamples writer/validator.
- Ran `tools/gh3_midori_glb_milo_route_gate.py` after refreshing the audit.
  The route now fails only the expected visual-precheck/direct-approval gates
  for the rejected current candidate, not the ihatecompvir source-reference
  checks.
- Added `tools/gh3_midori_guitar_reach_constraint_report.py` to test whether
  both hand targets want compatible guitar/proxy translations while respecting
  arm reach. Abstract result:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_guitar_reach_constraint_report_20260820.json`.
  `current_strum_guitar` conflicts strongly (`max_conflict=12.766`), while
  `forearm_hand` looks compatible abstractly (`max_conflict=0.909`).
- Tested that tempting `forearm_hand` average-offset idea through the actual
  bake-probe replay. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_forearm_hand_avg_offset_probe_20260820/actual_bake_replay_summary.json`.
  Result: reject. Frame `0/10/20/30/40` replay errors are
  `LH=18.66/21.16/39.73/37.95/34.60` and
  `RH=19.65/24.00/11.68/15.29/15.73`.
- Decision: do not promote simple average guitar/proxy translation, even when
  the abstract reach report looks compatible. The next productive path is a
  real per-frame GLB/pose-to-GH2 CharClipSamples writer/validator or richer
  source-authored hand/guitar transform constraint inside the direct MILO
  writer.

### 2026-08-20 r3 writer-contract, torso-axis, and actual-stage source gap

- Added `tools/gh3_midori_pose_bridge_writer_contract_report.py`. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_pose_bridge_writer_contract_report_actual_stage_20260820.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_pose_bridge_writer_contract_report_rich_attack_bridge_20260820.json`.
  The current 331-clip staged ACP manifest is GH2-writer-native: all emitted
  channels are `.pos/.quat`, type-ordered for `milo_convert`, with zero
  unsupported channels. Six staged clips are pose-bridge-active:
  `gh3_guit_midori_tran_atoout` (ui/main), `gh3_guit_mido_a_attackl`,
  `gh3_guit_mido_a_fst_jump01`, `gh3_guit_mido_a_fst_solo01`, and
  `gh3_guit_mido_a_med_idle01`.
- Important correction: the actual r3 stage manifest points at
  `.codex/current-evidence/midori-review-source-bridges-pinned-forcepartial-5case-20260819/review_source_bridge_batch_manifest.json`.
  That manifest preserves the body convention but omits 11 critical source
  records needed by the emitted hand/guitar targets: `Bone_Collar_L/R`,
  `Bone_Bicep_L/R`, `Bone_Forearm_L/R`, `Bone_Palm_L/R`,
  `bone_guitar_body`, `bone_ik_hand_guitar_l`, and
  `bone_ik_hand_guitar_r`. The contrast run with the richer attack arm/guitar
  pose bridge is `writer_native_pose_bridge_ready` with zero critical gaps, so
  the needed GLB/pose source coverage exists but is not wired into the actual
  r3 staging.
- Added `tools/gh3_midori_torso_axis_contract_report.py`. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_torso_axis_live_deployed_20260820.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r3_torso_axis_analysis_contractfix_20260820.json`.
  Live deployed/restored old loose DLC fails medium idle in transform space
  (`absZ=0.207/absX=0.937`), matching the horizontal screenshot rejection.
  Analysis `pinned_forcepartial_modelparentcomp_contractfix_r3` passes all five
  representative torso-axis checks, including medium idle
  (`absZ=0.914/absX=0.321`).
- Decision: the body-orientation branch should resume from analysis r3
  (`pinned_forcepartial_modelparentcomp_contractfix_r3`) rather than the
  restored deployed old hashes. The remaining approval blocker is the
  hands/guitar silhouette on that r3 body base, specifically actual-stage
  hand/guitar source coverage and then silhouette quality. Do not resume from
  ISO-mounted assets at game time; rebuild/export from the pinned forcepartial
  bridge source and deploy ordinary loose DLC only.

### 2026-08-20 r4 source-coverage bisect

- Re-exported the pinned forcepartial source bridges with Blender 4.5 and
  pinned NXTools `C:\Users\smmel\AppData\Local\Temp\nxtools_ref`, adding
  `Bone_Collar_L/R`, `Bone_Bicep_L/R`, `Bone_Forearm_L/R`,
  `Bone_Palm_L/R`, `bone_guitar_body`, `bone_ik_hand_guitar_l`, and
  `bone_ik_hand_guitar_r`. The wrapper ran Blender at idle priority and did
  not retain extracted GH3 scratch inputs. No game-time ISO path was used.
  Evidence:
  `.codex/current-evidence/midori-review-source-bridges-pinned-forcepartial-handguitar-5case-20260820/review_source_bridge_batch_manifest.json`.
- The fresh full re-export is not promotable by itself. It closes the
  writer-contract gaps but fails the torso-axis gate in 3 of 5 cases:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r4b_torso_axis_analysis_handguitar_20260820.json`.
- Bisected generated manifests without rerunning Blender:
  old pinned body plus arms-only passes torso, and old pinned body plus
  guitar-only also passes torso. The guitar-only build reproduces the original
  r3 main hash, so the regression was the full re-export artifact shape rather
  than either source family by itself.
- Built the safe merged manifest: preserve old pinned body/extra-frame records
  and graft in all 11 new hand/guitar donor records. Evidence:
  `.codex/current-evidence/midori-review-source-bridges-pinned-forcepartial-merged-handguitar-5case-20260820/review_source_bridge_batch_manifest.json`.
  The merged analysis candidate is GH2-writer-native with zero actual-stage
  critical gaps and passes all five torso-axis cases:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r4_merged_actual_stage_pose_bridge_writer_contract_20260820.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r4_merged_torso_axis_20260820.json`.
- Hand/guitar placement is not fixed yet. The merged candidate's hand overlay
  contract remains unchanged from r3:
  `LH-FRET=41.731`, `RH-STRUM=16.112`, `SOLVE-DELTA=103.315`; evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r4_merged_hand_overlay_contract_20260820.json`.
- Resume from the merged manifest/stage as the structurally cleaner base, but
  do not promote it visually. The next work is to wire the now-available
  arm/hand/guitar source records into the hand/guitar local-frame solve or
  target-channel emission path so the final hand target channels actually move.

### 2026-08-20 r5 hand-target local-frame diagnostics

- Added `tools/gh3_midori_final_hand_target_local_solve_report.py`. It uses
  the same final GH2 graph reconstruction as
  `gh3_midori_guitar_ik_contract_report.py` and solves the local
  `bone_fret_hand.mesh` / `bone_strum_hand.mesh` `.pos` rows needed to put the
  target transforms exactly on the visible hands.
- Re-tested static helper-local hand roots on the merged r4 baseline. Raw
  `source-ik-helper` barely improves the hand-overlay metric
  (`LH=40.180`, `RH=15.751`), while `source-ik-helper-gh2scale` worsens it
  (`LH=50.463`, `RH=17.121`). Both are rejected structurally. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r5_sourceik_hand_overlay_contract_20260820.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r5_sourceik_gh2scale_hand_overlay_contract_20260820.json`.
- The final-frame solve on the restored merged baseline says the hand-overlay
  frame would need target locals of left
  `[0.840926,-1.174859,-40.127533]` and right
  `[-3.82114,-2.992369,15.026728]`; evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r5_restored_merged_final_hand_target_local_solve_20260820.json`.
  A diagnostic build using those values proves the target channel path can
  land the metric at `LH/RH=0.000`, with torso axes still passing, but it is not
  a visual fix: `SOLVE-DELTA=131.677`, and direct local-GEN capture rejects it
  because the guitar remains a black vertical mass behind/right of the
  performer and the visible pose does not materially change. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r5_final_local_solve_hand_overlay_contract_20260820.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r5_final_local_solve_hand_overlay_visual_20260820/midori_1_hand_overlay_f010.bmp`.
- Capture used local `gh2_ps2_hybrid_assets/GEN` plus loose DLC only, no
  game-time ISO. The wrapper restored the six loose DLC MILOs afterward.
  `analysis` was also restored to the merged r4 baseline:
  `gh3_midori_main.milo_ps2` SHA256
  `BD92EDCB3CE5998C3ED75694724ABAF671E002E04F8538600E95E8491405E529`.
- Resume from the merged r4 baseline, not the r5 diagnostic target-local
  candidate. Next work should solve/move the visible guitar body and
  fret/strum targets together in one final GH2 frame, then validate multiple
  frames before any new visual capture.

### 2026-08-20 r6 coupled guitar-anchor diagnostic

- Extended `tools/gh3_midori_final_hand_target_local_solve_report.py` to solve
  a rigid `bone_pos_guitar.mesh` transform from the current final GH2
  fret/strum target points to the visible hands. Added diagnostic ACP staging
  overrides in `tools/gh3_midori_acp_stage.py` and
  `tools/gh3_midori_build_pipeline.py`:
  `--main-guitar-local-position-override` and
  `--main-guitar-local-rotation-override`.
- The coupled solve for `midori_1_hand_overlay_f010` predicts a rigid-fit
  residual of `3.507` per side, local guitar position
  `[-14.957951,15.083294,9.594125]`, and local rotation rows
  `[-0.013927,-0.668297,0.743764]`,
  `[-0.786855,-0.451652,-0.420558]`,
  `[0.616981,-0.591092,-0.519563]`; evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r6_merged_coupled_guitar_anchor_solve_20260820.json`.
- The no-deploy 0-degree coupled build keeps all five torso-axis cases green
  and improves the hand-overlay contract from r4's
  `LH=41.731/RH=16.112` to `LH=7.520/RH=3.569`; evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r6_coupled_guitar_anchor_hand_overlay_contract_20260820.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r6_coupled_guitar_anchor_torso_axis_20260820.json`.
- Direct local-GEN capture is visibly improved but still rejected: the guitar
  is no longer a featureless vertical slab behind her, but it is too
  high/right and the arm/hand silhouette is not coherent. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r6_coupled_guitar_anchor_hand_overlay_visual_20260820/midori_1_hand_overlay_f010.bmp`.
- Tested twist `+90` and `-90` around the fitted target axis. Both preserve
  torso but are structural rejects (`+90 LH=50.463/RH=13.077`, `-90
  LH=47.383/RH=7.414`), likely because the current HMX quaternion emission
  does not replay the twist report exactly.
- `analysis` is currently restored to the 0-degree coupled lead:
  `gh3_midori_main.milo_ps2` SHA256
  `9127F77DE5292408529E055C8E1F37491E2BA4058D87D6FDB5757BD5FBC71DC4`.
  Resume from this coupled visible-guitar branch. Next work should refine the
  coupled guitar-anchor orientation in the emitted GH2/HMX quaternion frame or
  derive the twist in post-replay/HMX space. Do not return to target-only
  solves.

### 2026-08-20 r7 HMX replay diagnosis

- Added explicit sampled-quaternion replay mode to the structural reports:
  `tools/gh3_midori_guitar_ik_contract_report.py`,
  `tools/gh3_midori_final_hand_target_local_solve_report.py`, and
  `tools/gh3_midori_torso_axis_contract_report.py` now accept
  `--sample-quat-mode direct|hmx`. The emitted r6 guitar override matches its
  intended local matrix under HMX replay, but the previous direct-mode hand
  metric was misleading: r6 is `LH=7.520/RH=3.569` in direct replay and
  `LH=7.277/RH=21.586` in HMX replay. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r7_current_analysis_hand_overlay_contract_direct_20260820.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r7_current_analysis_hand_overlay_contract_hmx_20260820.json`.
- Re-solved the coupled guitar anchor in HMX replay. The 0-degree HMX build
  gives `LH=2.546/RH=2.547` with HMX torso axis passing `0/5` failures, but
  direct capture is still a visual reject. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r7_hmx_coupled_guitar_anchor_hand_overlay_contract_hmx_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r7_hmx_coupled_guitar_anchor_torso_axis_hmx_20260820.json`, and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r7_hmx_coupled_guitar_anchor_visual_20260820/midori_1_hand_overlay_f010.bmp`.
- Added `tools/gh3_midori_capture_with_loose_dlc_backup.py` for future visual
  captures. It temporarily copies `analysis/gh3_midori_gh2_milos` into
  ordinary loose DLC, runs `gh3_midori_pose_review.py`, and restores the six
  DLC MILOs afterward; no game-time ISO path is used.
- Tested HMX twist `+60`, chosen as the closest roll to the less-bad r6 guitar
  orientation while preserving the `2.546` contact residual. It keeps the HMX
  hand and torso gates green but is also a direct visual reject: the guitar
  still reads as a vertical black bar through the torso and the arm mesh remains
  tangled. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r7_hmx_coupled_guitar_anchor_twist60_hand_overlay_contract_hmx_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r7_hmx_coupled_guitar_anchor_twist60_torso_axis_hmx_20260820.json`, and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r7_hmx_coupled_guitar_anchor_twist60_visual_20260820/midori_1_hand_overlay_f010.bmp`.
- Live loose DLC hashes were restored after capture. `analysis` currently holds
  the r7 HMX twist60 lead: `gh3_midori_main.milo_ps2` SHA256
  `F43FD6898EF442A8C85CD3FA8AE72B437C3896C6CACC1D46288002979CA03981`.
  Resume from HMX-mode metrics, not direct-mode metrics. Next work should solve
  against the visible guitar body and arm silhouette, not only the fret/strum
  target points.

### 2026-08-20 r8 silhouette-aware guitar orientation check

- Added `tools/gh3_midori_guitar_silhouette_report.py`. It measures the largest
  dark guitar-like BMP component, screen-space major-axis angle, aspect, and
  torso-band overlap. The metric matches the visual rejections: r6 is not a
  vertical bar (`angle=22.743`, `aspect=1.479`, `overlap=0.046`), r7 0-degree
  has heavy torso overlap (`overlap=0.784`), r7 twist60 is vertical-bar-like
  (`angle=80.378`, `aspect=0.539`, `overlap=0.982`), and r8 twist30 is also
  vertical-bar-like (`angle=-71.360`, `aspect=0.557`, `overlap=0.933`).
  Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r8_guitar_silhouette_compare_final_20260820.json`.
- Extended `tools/gh3_midori_final_hand_target_local_solve_report.py` with
  `--coupled-fixed-local-rotation`, which keeps a chosen guitar local
  orientation and solves only best-fit translation. Using the less-bad r6 local
  orientation in HMX replay gives `LH=9.504/RH=9.504` with HMX torso axis still
  passing `0/5` failures. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r8_hmx_fixed_r6orient_hand_overlay_contract_hmx_20260820.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r8_hmx_fixed_r6orient_torso_axis_hmx_20260820.json`.
- Direct capture of the fixed-r6-orientation candidate is still a visual
  reject, but it proves the failure has shifted: the guitar is no longer a
  vertical black bar, yet the body is too high/right across the chest and the
  arm mesh remains tangled. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r8_hmx_fixed_r6orient_visual_20260820/midori_1_hand_overlay_f010.bmp`.
- Live loose DLC hashes were restored after capture. `analysis` currently holds
  the r8 fixed-r6-orientation lead: `gh3_midori_main.milo_ps2` SHA256
  `5DEAB638D02C2ADADE67B4813DEF98D69F7DC39138CD79AFD4C5B062F55A5F22`.
  Resume from HMX-mode plus silhouette-gated evidence. Next work should move
  beyond guitar-only fitting into arm/hand pose correction while preserving a
  non-vertical guitar body.

### 2026-08-20 r9 arm-chain probe replay/alias diagnosis

- Preserved the r8 non-vertical guitar local and tested
  `visible-arm-chain-rotpos` overlays from
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`. Fixed two diagnostic
  issues on the way: the probe now accepts `--sample-quat-mode direct|hmx`, and
  `tools/gh3_midori_guitar_ik_contract_report.py` maps bare sampled channels
  like `bone_L-hand.pos` to `.mesh` transforms when appropriate. This matters
  because runtime visibly applies those arm channels, while the old structural
  report silently ignored them. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r9_r8_current_analysis_hand_overlay_contract_aliasfix_20260820.json`.
- The preserved-guitar HMX arm-chain candidate is rejected. It reports
  `LH=33.904/RH=21.904`, `SOLVE-DELTA=117.605`, and direct capture shows a
  coherent upright body/guitar face but both arms detached and stacked on the
  right side. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r9_r8preserve_visible_arm_chain_rotpos_hmx_hand_overlay_contract_aliasfix_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r9_r8preserve_visible_arm_chain_rotpos_hmx_visual_20260820/midori_1_hand_overlay_f010.bmp`, and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r9_hmx_armchain_visual_silhouette_compare_20260820.json`.
- Re-emitting the same HMX arm-chain probe with explicit `.mesh` channel names
  gives the same bad structural result, so the failure is not merely bare vs
  mesh channel naming. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r9_r8preserve_visible_arm_chain_rotpos_hmx_mesh_hand_overlay_contract_20260820.json`.
- `analysis` remains on the r8 fixed-r6-orientation lead:
  `5DEAB638D02C2ADADE67B4813DEF98D69F7DC39138CD79AFD4C5B062F55A5F22`.
  Do not promote the r9 arm-chain diagnostics. Next work should solve the arm
  chain in the actual overlay layer parent/local convention, comparing emitted
  upperArm/foreArm/hand locals against final HMX replay and runtime-visible
  results.

### 2026-08-20 r10 full-stage final-hand emission diagnosis

- Fixed two diagnostic bugs in
  `tools/gh3_midori_guitar_frame_hand_bake_probe.py`: the final replay path now
  applies the full main clip before solving visible hand rows, and
  `--solve-visible-hands-after-final-replay` now writes the solved
  `left_values`/`right_values` into the emitted fret/strum ACP sample rows
  instead of packing stale pre-solve positions.
- Lightweight r10 candidates remain invalid for visual proof because they
  replace the full `gh3_midori_main` bank with a one-clip probe bank and drop
  the r8 main/guitar context. The proof candidate must preserve the full
  `analysis/gh3_midori_acp_stage/guitar-main` bank and swap only the two
  emitted hand-overlay ACPs.
- Built the full-stage diagnostic candidate at
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r10_fullstage_finalhands_emitfix_candidate_20260820`.
  Its HMX hand contract is exactly solved for the target frame:
  `LH-FRET=0.000`, `RH-STRUM=0.000`, `SOLVE-DELTA=81.991`; evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r10_fullstage_finalhands_emitfix_hand_overlay_contract_hmx_20260820.json`.
- Direct loose-DLC capture is still a visual reject. Midori remains upright and
  the guitar silhouette is not the vertical-bar failure (`angle=25.826`,
  `aspect=1.432`, `overlap=0.221`), but the upper-body pose collapses: hair and
  head push through the torso/shoulder area, the arm/hand mesh is tangled, and
  the guitar body still occludes through the chest. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r10_fullstage_finalhands_emitfix_visual_20260820/midori_1_hand_overlay_f010.bmp`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r10_fullstage_finalhands_emitfix_visual_20260820/pose_review_manifest.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r10_fullstage_finalhands_emitfix_silhouette_compare_20260820.json`.
- Do not promote r10. The zero hand-distance result proves the emitted rows can
  hit the computed fret/strum targets, but the computed targets are still in
  the wrong final body frame. Return to the hierarchy diagnosis at the
  matrix-local `Control_Root`/pelvis convention before further arm or guitar
  fitting.

### 2026-08-20 r12 actual-MILO Control_Root/pelvis replay check

- Added `--control-root-pelvis-parent` to
  `tools/gh3_midori_control_root_basis_sweep.py` and
  `tools/gh3_midori_root_pelvis_bridge_diagnostic.py`, then reran both against
  the current bridge batch. The parented target graph gives the same
  conclusion as r11: no promotable pelvis basis candidate, with the old
  `122.666` degree source pelvis world-vs-root-local mismatch still present in
  the bridge-space report. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r12_control_root_parented_basis_sweep_20260820.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r12_root_pelvis_parented_bridge_diagnostic_20260820.json`.
- Added `tools/gh3_midori_actual_milo_parent_replay_report.py` to inspect the
  actual built character/clipset MILOs through `milo_convert_tool`, instead of
  relying on stale rig JSON. It found that the actual r8 lead MILO has
  `bone_pelvis.mesh` parented under `Control_Root` in both the character
  transforms and clipset `CharBone` graph, while
  `analysis/gh3_midori_current_midori1_rig.json` still reports
  `bone_pelvis.mesh` as flat.
- Actual built samples for both `gh3_guit_mido_a_attackl` and
  `stand_medium_01` show `bone_pelvis.mesh` is static across checked samples,
  while `bone_spine1.mesh` is dynamic. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r12_actual_milo_parent_replay_attackl_20260820.json`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r12_actual_milo_parent_replay_stand_medium_20260820.json`.
- Updated interpretation: do not treat the current r8/r10 visual rejects as an
  active pelvis animation explosion. The actual built lead has a stable,
  parented pelvis frame. The remaining failure is above that static frame:
  spine/chest/head/arm/guitar channels are being authored or replayed in a
  mismatched upper-body frame. Future lower-tier diagnostics should use actual
  MILO inspection or regenerate the rig JSON before drawing conclusions from
  `analysis/gh3_midori_current_midori1_rig.json`.

### 2026-08-20 r13 upper-chain actual-layer replay and head/neck freeze

- Added `tools/gh3_midori_actual_layer_replay_report.py`, which composes the
  actual built character MILO graph with sampled main/fret/strum MILO layers.
  On `midori_1_hand_overlay_f010` (`stand_medium_01` frame 10 plus
  `gh3_hnd_guit_chord_mid_bar3_d` and
  `gh3_hnd_guit_strum_mido_norm_m01_d`), translation distances were plausible,
  but world-rotation deltas from bind identified the visual-collapse tier:
  `bone_neck.mesh=150.886`, `bone_head.mesh=134.410`,
  `bone_L-clavicle.mesh=121.027`, and `bone_R-clavicle.mesh=117.117`.
  Source attribution: neck/head/L-clavicle came from the main layer, while
  R-clavicle came from the strum overlay. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r13_actual_layer_replay_hand_overlay_f010_20260820.json`.
- Added `tools/gh3_midori_freeze_acp_channels_to_bind.py` and tested two
  diagnostic freezes. Freezing head/neck plus clavicles made the screenshot
  bipedal/coherent but wrecked hand reach (`LH=33.540/RH=27.957`), proving
  clavicles must stay in the arm-chain solve. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r13_bindfreeze_upper_visual_20260820/midori_1_hand_overlay_f010.bmp`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r13_bindfreeze_upper_hand_overlay_contract_hmx_20260820.json`.
- The narrower `head-neck-only` freeze is the useful r13 branch: it removes
  the head/neck rotation warning, preserves the r8 hand contract
  (`LH=9.504/RH=9.504`), and the direct capture is a clear visual improvement
  over r8/r10: head/hair are coherent and upright. It is still rejected because
  clavicles/arms remain tangled and the guitar is still high/right across the
  chest. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r13_headneck_freeze_layer_replay_f010_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r13_headneck_freeze_hand_overlay_contract_hmx_20260820.json`, and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r13_headneck_freeze_visual_20260820/midori_1_hand_overlay_f010.bmp`.
- Do not treat r13 as final or broadly promoted: the freeze currently patches
  the representative `stand_medium_01` diagnostic clip only. The next branch
  should keep the r13 head/neck local treatment and solve clavicle/upper-arm
  rotations in the actual layer replay frame, preserving the r8 hand/guitar
  contract.

### 2026-08-20 r14 fractional clavicle-to-bind blend rejection

- Extended `tools/gh3_midori_freeze_acp_channels_to_bind.py` with
  `head-neck-clavicle-blend`, which keeps the r13 head/neck bind treatment and
  blends clavicle quaternion samples toward actual character bind by a tunable
  amount. Tested 10%, 15%, 25%, and 50% on the same representative
  `midori_1_hand_overlay_f010` frame.
- Structural tradeoff:
  - r13 head/neck-only: clavicles remain rotation warnings, hand contract
    `LH=9.504/RH=9.504`.
  - 10% clavicle blend: both clavicles still warn, hand contract worsens to
    `LH=10.008/RH=11.761`.
  - 15% blend: both clavicles still warn, hand contract worsens to
    `LH=10.805/RH=12.927`.
  - 25% blend: R-clavicle warning clears but L-clavicle remains; hand contract
    worsens to `LH=13.126/RH=15.261`.
  - 50% blend clears both clavicle warnings but hand contract worsens to
    `LH=20.577/RH=20.696`.
- Direct captures for 10% and 25% are still visual rejects and are only
  marginally different from r13 head/neck-only. Silhouette changes are tiny:
  r13 overlap `0.328`, 10% overlap `0.326`, 25% overlap `0.321`; all remain
  non-vertical guitar silhouettes. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r14_clavblend10_visual_20260820/midori_1_hand_overlay_f010.bmp`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r14_clavblend25_visual_20260820/midori_1_hand_overlay_f010.bmp`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r14_clavblend_visual_silhouette_compare_20260820.json`,
  and the `r14_clavblend*_hand_overlay_contract_hmx_20260820.json` reports.
- Do not continue simple clavicle-to-bind blending. It trades away hand reach
  faster than it improves the visible arm/guitar problem. Next branch should
  solve clavicle/upper-arm as an arm-chain/guitar-frame problem, not as a
  bind-pose damping problem.

### 2026-08-20 r15/r16 all-body head-neck and guitar-frame diagnostics

- Extended `tools/gh3_midori_freeze_acp_channels_to_bind.py` with
  `head-neck-all-body`, which patches `bone_neck.mesh.quat` and
  `bone_head.mesh.quat` to actual character bind local across `guitar-main`
  and `guitar-ui` staged ACP clips. Built r15 from the local
  `analysis/gh3_midori_acp_stage` only; no ISO path was used. The representative
  hand-overlay contract is preserved (`LH=9.504/RH=9.504`) and the actual-layer
  replay leaves only clavicle rotation warnings. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r15_headneck_allbody_freeze_report_20260820.json`,
  `r15_headneck_allbody_layer_replay_f010_20260820.json`, and
  `r15_headneck_allbody_hand_overlay_contract_hmx_20260820.json`.
- Direct r15 captures prove this is a useful partial but not an approval
  candidate. Medium idle head/hair become upright and coherent, but hand-overlay
  remains visually unchanged from r13: guitar/arm mass is still wrong. Full
  representative capture still fails direct visual review despite most automated
  load/framing checks passing. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r15_headneck_allbody_visual_20260820/midori_1_hand_overlay_f010.bmp`
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r15_headneck_allbody_visual_full_capture_20260820.json`.
- Added optional `--skip-fret/--skip-strum` to
  `tools/gh3_midori_actual_layer_replay_report.py` and ran r15 main-only
  diagnostics across representative body clips. Head/neck are fixed in all
  checked main-only cases. Remaining main-only rotation warnings are
  clip-dependent clavicles: none for medium idle/transition, L-clavicle for
  attack/jump, R-clavicle for jump/solo. `bone_pos_guitar.mesh` stays at a
  constant `61.894` degree bind delta across the same cases. Evidence:
  `r15_headneck_allbody_mainonly_*_20260820.json`.
- Tested r16 `head-neck-guitar-all-body`, which additionally binds
  `bone_pos_guitar.mesh.quat` across body/UI clips. Structural reject:
  representative hand-overlay contract worsens to `LH=21.923/RH=17.106` while
  clavicle warnings remain. Evidence:
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r16_headneck_guitar_allbody_freeze_report_20260820.json`,
  `r16_headneck_guitar_allbody_layer_replay_f010_20260820.json`, and
  `r16_headneck_guitar_allbody_hand_overlay_contract_hmx_20260820.json`.
- Next branch: keep the r15 all-body head/neck treatment as a partial layer,
  but do not bind the guitar body independently. The guitar rotation, fret/strum
  targets, and clavicle/upper-arm local frames must be solved together in the
  actual layered frame.

### 2026-08-20 r17-r20 bake-probe channel merge diagnostics

- Added `tools/gh3_midori_patch_bake_probe_positions_into_stage.py` to promote
  one-frame `gh3_midori_guitar_frame_hand_bake_probe.py` manifests into a full
  staged ACP tree without discarding base clip context. The tool can insert
  missing `.pos/.quat` channels, clamp one-sample overlay clips, and optionally
  solve desired probe world positions back into the actual r15 parent-local
  frame before writing samples.
- Ran a bounded r17 structural matrix on top of the r15 head/neck layer. Most
  pair-fit/target-proxy variants remained poor. The best probe-level row was
  `visible-arm-chain + stock-prop-comp-strings`
  (`LH=2.098/RH=4.351`), but directly replacing the three ACPs dropped base
  context and packed as `LH=24.323/RH=22.958`. Evidence:
  `r17_matrix_summary_20260820.tsv`,
  `r17_matrix2_summary_20260820.tsv`, and
  `r17_armchain_stockstrings_min_contract_hmx_20260820.json`.
- r18 used channel insertion plus world-to-local position solving for the
  stock-strings arm-chain probe. Packed result improved survival but still
  missed the probe target: `LH=7.599/RH=10.860`. Direct capture remains a
  visual reject; arm shards move slightly but the guitar/arm mass is still
  wrong. Evidence:
  `r18_armchain_stockstrings_worldsolve_patch_report_20260820.json`,
  `r18_armchain_stockstrings_worldsolve_contract_hmx_20260820.json`, and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r18_armchain_stockstrings_worldsolve_visual_20260820/midori_1_hand_overlay_f010.bmp`.
- r19 used the `visible-arm-chain-rotpos` probe so upper/forearm quats were
  inserted as well as positions. It flips the failure: right hand improves
  (`RH=4.524`) but left worsens (`LH=18.883`). r20 hybridized r18's left side
  with r19's right side and is the best packed structural row in this branch
  (`LH=7.599/RH=4.524`), but direct visual capture is still rejected: the face
  stays fixed from r15, yet black guitar mass and tan arm shards remain
  incoherent. Evidence:
  `r19_armchain_rotpos_worldsolve_contract_hmx_20260820.json`,
  `r20_hybrid_leftstock_rightrotpos_contract_hmx_20260820.json`, and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r20_hybrid_leftstock_rightrotpos_visual_20260820/midori_1_hand_overlay_f010.bmp`.
- Decision: keep the channel-merge tool; it closes an automation gap between
  probe manifests and full ACP/MILO packing. Do not promote r18/r20 visually.
  The remaining blocker is not hand distance alone; clavicle rotations and the
  visible guitar frame still need a coupled rotation solve, likely including
  clavicle local quats rather than only visible forearm/hand endpoint channels.

### 2026-08-20 r21-r23 clavicle-first arm/guitar solve

- Tested the obvious but wrong order first: r20-style hybrid arm patches, then
  clavicle bind/blends. Full clavicle bind clears rotation warnings but wrecks
  hand reach (`LH=37.046/RH=24.118`); 25% blend worsens to
  `LH=15.083/RH=9.887`; 10% blend still leaves both clavicle warnings and
  worsens to `LH=10.024/RH=6.450`. Direct 10% capture remains visually rejected.
  Evidence: `r21_hybrid_clav*_contract_hmx_20260820.json` and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r21_hybrid_clavblend10_visual_20260820/midori_1_hand_overlay_f010.bmp`.
- Reversed the order for r22: start from r15, bind clavicles first, build that
  as the base frame, regenerate the stock-strings arm-chain probe in the
  clavicle-fixed frame, then merge those channels back into the full
  clavicle-fixed stage. The probe-level result is excellent
  (`LH=0.001/RH=1.727`). Packed MILO survives partially:
  `LH=5.847/RH=12.453`, with no actual-layer suspicious rotation nodes.
  Direct capture is still rejected, but it is a real visual movement: the body
  and face remain coherent and the guitar moves out of the torso; the remaining
  failure is now a too-high/right guitar plus broken strum-side arm shards.
  Evidence:
  `r22_clavbind_stockstrings_worldsolve_contract_hmx_20260820.json`,
  `r22_clavbind_stockstrings_worldsolve_layer_replay_f010_20260820.json`, and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r22_clavbind_stockstrings_worldsolve_visual_20260820/midori_1_hand_overlay_f010.bmp`.
- A small r23 guitar/target offset probe from the r22 base found low probe
  distances for `down4`, but packed visual capture is worse: the guitar/arm
  mass crosses the face. Do not promote r23 down-offset. Evidence:
  `r23_offset_probe_summary_20260820.tsv`,
  `r23_clavbind_stockstrings_down4_contract_hmx_20260820.json`, and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r23_clavbind_stockstrings_down4_visual_20260820/midori_1_hand_overlay_f010.bmp`.
- Current visual lead is r22, not r20/r23. Next branch should keep the
  clavicle-first ordering and solve the strum/right arm and guitar placement in
  that fixed-clavicle frame. The right side is the structural weak point after
  packing (`RH=12.453`), and naive whole-frame down offsets make the visual
  worse.

### 2026-08-20 r24-r27 fixed-clavicle proxy/placement diagnostics

- Added explicit target-proxy support to
  `tools/gh3_midori_patch_bake_probe_positions_into_stage.py`:
  `--include-target-proxies` solves `left_target_world_after` and
  `right_target_world_after` back into `bone_fret_hand.mesh.pos` and
  `bone_strum_hand.mesh.pos` under the actual fixed-clavicle parent frame.
  Testing this on the r22 stock-strings solve made the packed contract worse
  (`LH=16.612/RH=15.834`, `SOLVE-DELTA=139.818`), so the proxy-missing
  hypothesis is rejected for this branch. Evidence:
  `r24_clavbind_stockstrings_targets_patch_report_20260820.json` and
  `r24_clavbind_stockstrings_targets_contract_hmx_20260820.json`.
- Tested a fixed-clavicle side hybrid using stock-strings left side and
  rot+pos right side. It also worsens the packed right side
  (`LH=5.847/RH=17.331`) while keeping rotation warnings clear. Evidence:
  `r25_hybrid_rightrotpos_contract_hmx_20260820.json`.
- Promoted two bounded placement offsets from the r22 fixed-clavicle base.
  `front4_down6` keeps the same packed reach profile as r22
  (`LH=5.847/RH=12.453`) but visually pulls the arm/guitar mass across the
  face, so it is rejected. `back4_down6` avoids the face crossing and remains
  coherent, but still rejects because the guitar is high/right and the arm mass
  drapes across the chest (`LH=10.075/RH=12.453`). Evidence:
  `r26_front4_down6_contract_hmx_20260820.json`,
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r26_front4_down6_visual_20260820/midori_1_hand_overlay_f010.bmp`,
  `r27_back4_down6_contract_hmx_20260820.json`, and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r27_back4_down6_visual_20260820/midori_1_hand_overlay_f010.bmp`.
- Decision: r22/r27 are the visual leads for the fixed-clavicle family, but
  neither passes. Do not continue plain target-proxy insertion or broad
  translation offsets. Next work should keep fixed-clavicle ordering and solve a
  rotation/attachment frame for the visible guitar/strum arm, rather than moving
  the whole guitar/target cluster in world space.

### 2026-08-20 r28-r31 fixed-clavicle guitar-rotation diagnostics

- Added `bone_pos_guitar.mesh.quat` promotion to
  `tools/gh3_midori_patch_bake_probe_positions_into_stage.py`, so bake-probe
  guitar rotation modes can be merged into full staged ACPs instead of remaining
  probe-only. This works mechanically: r28/r29/r31 patched and packed guitar
  quat samples successfully.
- Ran a bounded fixed-clavicle rotation matrix around the stock-strings arm
  solve. Probe-level rows existed for stock-rig local, stock runtime attach-world,
  pair-fit, and small correction rotations. After full packing, all useful-looking
  guitar-quat rows became structural rejects:
  `stock-prop-debug-attach-world` packed as `LH=9.487/RH=14.738` and flags
  `bone_pos_guitar.mesh`; `stock-rig-local` packed as `LH=7.894/RH=15.160`
  and also flags `bone_pos_guitar.mesh`; x-axis post rotations improve the
  probe right side but packed `x30post` is `LH=10.812/RH=13.875` and flags
  `bone_pos_guitar.mesh`. Evidence:
  `r28_rotation_probe_summary_20260820.tsv`,
  `r28_attachrot_stockstrings_contract_hmx_20260820.json`,
  `r29_stockrig_stockstrings_contract_hmx_20260820.json`,
  `r31_x_rotation_probe_summary_20260820.tsv`, and
  `r31_x30post_contract_hmx_20260820.json`.
- Decision: explicit guitar-quat rotation modes are now real packed tests, and
  this fixed-clavicle batch rejects them. Do not continue stock attach-world,
  stock-rig local, pair-fit, or small x-post guitar rotations as simple
  one-frame overrides. r22/r27 remain the visual leads. The next useful branch
  should solve the strum arm/guitar attachment as a coupled local-frame problem
  that changes the visible strum arm and guitar frame together, not as an
  independent `bone_pos_guitar` quat override.

### 2026-08-20 r32-r35 fixed-clavicle target-proxy survival diagnosis

- Rebuilt the fixed-clavicle base from local
  `analysis/gh3_midori_acp_stage` and packed with ihatecompvir-derived
  `milo_convert_tool` role names (`guitar-main`, `guitar-fret`,
  `guitar-strum`, `guitar-ui`). No ISO/runtime mounted path was used.
- Reproduced r22 exactly as `r32l_r22_repro_hmx_mesh` by using
  `--sample-quat-mode hmx` plus `--channel-name-mode mesh`. It packs as
  `LH=5.850/RH=12.450`, matching r22. Node-by-node comparison against the
  probe manifest showed the visible nodes survive packing exactly:
  `bone_pos_guitar.mesh`, `bone_L-hand.mesh`, `bone_R-hand.mesh`,
  `bone_L-foreArm.mesh`, and `bone_R-foreArm.mesh` all have `Delta=0.000`.
  The only survival gap is target proxies:
  `bone_fret_hand.mesh` delta is about `6.160`, and `bone_strum_hand.mesh`
  delta is about `12.180`.
- Diagnosed the old `--include-target-proxies` reject: target locals were
  solved against pre-move `bone_fret.mesh`/`bone_strum.mesh` parent frames, but
  those parents are children of the translated `bone_pos_guitar.mesh`. Patched
  `tools/gh3_midori_patch_bake_probe_positions_into_stage.py` so target proxy
  locals under one-level guitar children use the emitted guitar translation
  delta while solving local positions.
- New structural lead: `r34_hmx_mesh_targets_guitardelta_parent` packs as
  `LH=1.337/RH=0.000`, with clean actual-layer replay and no suspicious
  rotations. Direct capture is still a visual reject: Midori remains bipedal
  and face/body coherent, but the guitar sits too high/right and behind the
  shoulder, and strum-side proxy/arm geometry tangles near the waist. Evidence:
  `r34_hmx_mesh_targets_guitardelta_parent_contract_hmx_20260820.json`,
  `r34_hmx_mesh_targets_guitardelta_parent_layer_replay_f010_20260820.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r34_hmx_mesh_targets_guitardelta_parent_visual_20260820/midori_1_hand_overlay_f010.bmp`.
- Tested corrected-target placement offsets `r35a_targets_down8`,
  `r35b_targets_back4_down8`, and `r35c_targets_front4_down8`. Packed
  contracts remain structurally plausible (`r35a LH=4.490/RH=0.000`,
  `r35b LH=4.810/RH=1.180`, `r35c LH=4.870/RH=0.000`) but all direct captures
  visually reject: down/back/front offsets do not resolve the guitar being
  behind/right of the torso or the tangled strum-side geometry.
- Decision: r34 supersedes r22 as the structural lead, but not as a visual
  pass. Do not go back to simple missing-proxy insertion or broad translation
  offsets. Keep corrected target-parent solving enabled, then solve guitar/strum
  attachment orientation or source-frame basis as the next branch.

### 2026-08-20 r36-r39 corrected target-parent rotation/hand diagnostics

- Generalized `tools/gh3_midori_patch_bake_probe_positions_into_stage.py` again:
  target proxy local solves now recompute parent worlds under the full emitted
  `bone_pos_guitar.mesh` local pos/quat. This matters for retesting guitar
  rotation rows because `bone_fret.mesh` and `bone_strum.mesh` are one level
  under the guitar frame.
- Retested a bounded fixed-clavicle rotation batch with corrected target
  solving enabled. The old structural rejects are now much better numerically:
  `r36b_attachworld_correct_targets` packs `LH=1.340/RH=2.830`,
  `r36c_stockrig_correct_targets` packs `LH=1.340/RH=0.000`, and
  `r36d_x30post_correct_targets` packs `LH=1.340/RH=0.000`. Actual-layer replay
  still flags suspicious guitar rotation for attach-world and stock-rig
  (`175.63` and `180.00` degrees). `r36d` stays structurally clean but its
  direct visual capture is effectively the same reject as r34: bipedal coherent
  body/face, guitar over/behind the right shoulder, strum-side geometry tangled.
- Tested source pelvis-relative guitar orientation and source pelvis-delta
  placement bases (`direct`, `anim`, `helper`) with corrected targets. These
  reject at probe level before packing: source rotations have large contact
  misses (`LH` roughly `27.68-40.25`, `RH` roughly `9.96-30.22`), and source
  placements also miss badly except for one right-side partial (`r37e` has
  `LH=16.55/RH=4.25`). Do not pack/capture these source-basis rows as-is.
- Added canonical `.quat` fallback plus `--channel-filter-regex` to the patch
  tool so canonical hand/finger rotations can be promoted selectively without
  also applying the canonical arm rotations that fail at probe level. Built
  `r39_handfinger_quats_on_correct_targets` by applying only hand/finger/target
  quats over the corrected-target r36a/r34-style stage. It preserves the
  structural contract (`LH≈0/RH≈0`, no suspicious rotations) but direct capture
  remains visually rejected and looks essentially like r34. Evidence:
  `r39_handfinger_quats_on_correct_targets_contract_hmx_20260820.json`,
  `r39_handfinger_quats_on_correct_targets_layer_replay_f010_20260820.json`,
  and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r39_handfinger_quats_on_correct_targets_visual_20260820/midori_1_hand_overlay_f010.bmp`.
- Decision: corrected target-parent solving is a real infrastructure fix and
  r34/r39 remain structural leads, but neither is a visual pass. The remaining
  failure is not proxy survival, broad offset, simple guitar quat override,
  source pelvis basis, or target/finger quat promotion. Next work should inspect
  actual guitar/strum attachment parent model geometry and the rendered prop
  relationship among `bone_pos_guitar`, `bone_strum`, visible right arm, and
  the guitar mesh.

### 2026-08-20 r40-r41 front-anchor recombination diagnostics

- Inspected ihatecompvir `milo_convert_tool` guitarist runtime graph: the model
  writer adds `CharIKMidi fret.ik`, `CharIKHand left_hand.ik` targeting
  `bone_fret_hand.mesh`, `CharIKHand right_hand.ik` targeting
  `bone_strum_hand.mesh`, the guitar outfit loader, and weight setters. Midori's
  current `gh3_midori_1.milo_ps2` has those controllers and the hardcoded stock
  proxy graph. The remaining failure is therefore not an absent runtime IK
  controller.
- Reopened old coupled-anchor visuals. r8 is non-bipedal in the upper body, so
  the old coupled anchor cannot be promoted directly. r10 is bipedal and puts
  the guitar in front, but arms/hands are detached and broken. This made r10's
  guitar world useful as a placement clue, not as a candidate.
- Built `r40_r34_targets_r10_frontanchor`: start from the current fixed-clavicle
  corrected-target solve, then apply the r10 front-anchor guitar world delta
  (`+9.770849,+3.242513,-12.030040`) so `bone_pos_guitar.mesh` lands near
  `-6.014,2.106,48.814`. It packs with clean replay and `LH=7.716/RH=0.000`.
  Direct visual moves the guitar in front of the torso and keeps Midori bipedal,
  but left-side reach and tan proxy/hand geometry are visibly tangled.
- Built one contract-guided correction from r40:
  `r41b_r40_full_contract_offset` lands the guitar near
  `-7.601,3.993,51.964`, packs as `LH=3.832/RH=0.000`, and has clean
  actual-layer replay. Its direct capture is the best front-guitar visual in
  this family, but it still rejects: head/body are coherent and the guitar is no
  longer over the shoulder, yet the guitar remains too high/right across the
  chest and the arm/proxy geometry tangles around the waist. Evidence:
  `r41b_r40_full_contract_offset_contract_hmx_20260820.json`,
  `r41b_r40_full_contract_offset_layer_replay_f010_20260820.json`, and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r41b_r40_full_contract_offset_visual_20260820/midori_1_hand_overlay_f010.bmp`.
- Decision: r41b supersedes r34/r39 as the front-placement visual lead, but is
  still not a pass. Do not revive r8/r10 wholesale. Next work should either
  solve visible arm positions/rotations in the r41b final packed local frame, or
  determine whether the tan proxy/hand helper geometry should be hidden/nonmesh
  like stock GH2 helpers.

### 2026-08-20 r42 targeted mesh-chunk filter diagnostic

- Resumed from r41b by reproducing the front-anchor corrected-target candidate
  under an r42 scratch label. It matched the r41b structural contract:
  `LH-FRET=3.832`, `RH-STRUM=0.000`, `SOLVE-DELTA=92.554`; scratch candidates
  and stages were deleted after proof capture.
- Added bounded diagnostics to `tools/gh3_midori_mesh_isolation_probe.py`
  (`--milos`, repeated `--mesh`, clip/frame/camera options) and isolated the
  suspicious skinned chunks `midori_1_mesh0_part16.mesh`,
  `midori_1_mesh0_part26.mesh`, and `midori_1_mesh0_part32.mesh`, plus
  reference hand/forearm chunks `part17` and `part33`. The suspects render as
  tiny tan slivers/debris, but the reference chunks also look fragmented when
  isolated, so this did not prove the main waist tangle is a helper object.
  Evidence BMPs are retained under
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r42_mesh_iso_probe_20260820/mesh_isolation/`.
- Added diagnostic `--exclude-chunk-name` to
  `tools/gh3_midori_model_bundle.py` and built one filtered model-only
  candidate excluding parts 16/26/32 from outfit 1 while keeping the r41b/r42
  animation MILOs unchanged. The full hand-overlay capture restored loose DLC
  hashes afterward and used `gh2_ps2_hybrid_assets/GEN` plus loose DLC only,
  not an ISO. Evidence:
  `r42_filtered_parts16_26_32_pose_review_20260820_b.json` and
  `.codex/current-evidence/midori-fresh-attack-targetlength-20260818/r42_filtered_parts16_26_32_pose_review_20260820_b/midori_1_hand_overlay_f010.bmp`.
- Visual result: removing those three debris chunks makes the tiny slivers less
  relevant, but the pose still rejects. Midori remains bipedal with coherent
  head/body, the guitar remains too high/right across the chest, and the tan
  hand/forearm mass is still collapsed around the waist. Therefore the main
  failure is not just those three mixed-bone mesh chunks.
- Tooling note: ihatecompvir's `MiloEditor`/`MiloLib`/`MiloUtil` is not checked
  into this workspace. A web check found `ihatecompvir/MiloEditor` describes a
  cross-platform Milo scene editor/library, with RB3/DC1 most supported and
  broader game support varying. It may be useful as a GLB/MILO bridge, but it
  has not yet been validated as a GH2 PS2 character MILO emitter. Current local
  builds still use the OGX `milo_convert_tool`.
- Decision: r41b remains the visual lead but is not a pass, and r42 falsifies
  the narrow "delete obvious debris chunks" hypothesis. Next branch should solve
  visible arm/hand positions in the final r41b packed local frame or test a real
  GLB/MiloLib bridge end-to-end before more manual offset grinding.

### 2026-08-20 r47-r49c exact-overlay bridge and visible hand positions

- r47 created clean source-side Blender/NXTools GLB bridges for the exact
  frame-10 overlay clips:
  `gh3_guit_mido_c_med_idle`,
  `gh3_hnd_guit_chord_mid_bar3_d`, and
  `gh3_hnd_guit_strum_mido_norm_m01_d`. Source hand/guitar sanity passed
  before ACP/MILO promotion.
- r48 fed those bridges into ACP/MILO without visible palm position promotion
  and reproduced the failure: actual-layer replay still rejected with
  `hand_ratio=0.074428`, `target_ratio=-0.376701`,
  `guitar_ratio=0.141367`, `delta=0.066939`; visible
  `bone_L-hand.mesh` / `bone_R-hand.mesh` had empty `sample_sources`.
  Source-side GLB/pose was therefore no longer the main suspect.
- Added diagnostic `--bridge-visible-hand-position-bones` to
  `tools/gh3_midori_acp_stage.py`. The first r49 attempt confirmed the flag was
  accepted but did not change replay because the inserted channels were created
  after channel ordering and, through the normal mapper, targeted the hidden
  guitar hand roots instead of visible hand meshes.
- r49c writes explicit source-palm bridge positions into
  `bone_L-hand.mesh.pos` and `bone_R-hand.mesh.pos` before channel ordering.
  Role-specific staging used both palms in main, left palm in fret, and right
  palm in strum. Temporary packed MILO replay now passes the no-capture overlay
  gate: `status=overlay_visual_sanity_pass`, `failures=0`,
  `hand_ratio=0.374905`, `target_ratio=-0.376701`,
  `guitar_ratio=0.141367`, `delta=-0.233538`. Replay confirms visible hand mesh
  sources `left=pos:fret` and `right=pos:strum`.
- Evidence retained:
  `r49c_bridge_visible_hand_layer_replay_f010_20260820.json` and
  `r49c_bridge_visible_hand_visual_sanity_gate_20260820.json`.
- Decision: r49c is the current structural/no-capture lead and supersedes the
  r48 source-bridge rejection. It is not direct visual approval, has not been
  promoted to loose DLC, and must be reviewed with actual visual capture from
  external/loose DLC before any success claim. Do not run from a mounted ISO at
  game time.

### 2026-08-20 r50-r51 loose-DLC visual follow-up

- r50 built a full temporary stock-all candidate from the r49c visible-hand ACP
  path and captured through
  `tools/gh3_midori_capture_with_loose_dlc_backup.py`, which restored the loose
  DLC files afterward. The packed overlay gate still passed for both exact
  `gh3_guit_mido_c_med_idle` and runtime alias `stand_medium_01`, but native
  screenshots rejected hard: idle and overlay were upside down/collapsed. This
  proves the scalar overlay gate is too narrow and r49c cannot be promoted
  outside the known Control_Root/model-parent-compensated branch.
- Added restore-safe camera/offset passthroughs to
  `tools/gh3_midori_capture_with_loose_dlc_backup.py`.
- Added `--bridge-visible-hand-position-bones` passthrough to
  `tools/gh3_midori_build_pipeline.py`, and role-filtered the stage hook in
  `tools/gh3_midori_acp_stage.py` so visible bridge palms emit as
  main=both, fret=left, strum=right. This lets full pipeline staging reproduce
  the r49c role-specific behavior without layer cross-over.
- r51 rebuilt the analysis pipeline with
  `--no-gh2-animation-rig --control-root-pelvis-parent
  --model-parent-compensate-acp --stock-bind-scope upper-limbs-guitar` plus the
  r49c visible hand flag. Direct loose-DLC capture for idle plus hand overlay
  passed automated load/framing (`failures=0`, min margin 40), but visual
  inspection still rejects: the body is upright again, yet head/upper torso are
  bent backward and the guitar/hands are tangled/off-contact. Packed replay
  also rejects the hand-target contract (`fret_hand_to_left_hand=19.102769`,
  `strum_hand_to_right_hand=21.540285`).
- Evidence retained:
  `r50_r49c_full_candidate_pose_review_z40_20260820.json`,
  `r51_controlroot_stockupper_r49c_pose_review_20260820.json`,
  `r51_controlroot_stockupper_r49c_visual_20260820/midori_1_medium_idle_f060.bmp`,
  `r51_controlroot_stockupper_r49c_visual_20260820/midori_1_hand_overlay_f010.bmp`,
  and `r51_controlroot_stockupper_r49c_visual_decision_20260820.json`.
- Decision: r51 supersedes r50 as the current bipedal branch, but it is still
  visually rejected and not shippable. Next work should solve the combined
  Control_Root/model-parent-compensated hand/guitar target space; do not rely on
  the overlay center-ratio gate alone, and do not return to stock-all r50.

### 2026-08-20 r52 visible hands to current runtime targets

- Added `tools/gh3_midori_patch_visible_hands_to_runtime_targets.py`. It copies
  a staged ACP tree, replays a flat six-MILO candidate at the reviewed frame,
  solves the desired world positions back into the visible hand parent spaces,
  and patches only `bone_L-hand.mesh.pos` and `bone_R-hand.mesh.pos`.
- Applied it to the r51 Control_Root/model-parent-compensated stage at frame 10
  so left visible hand targets `bone_fret_hand.mesh` and right visible hand
  targets `bone_strum_hand.mesh`. Rebuilt only fret/strum temp MILOs.
- Packed replay proves the local solve works:
  `fret_hand_to_left_hand=0.000034` and
  `strum_hand_to_right_hand=0.000025`. The remaining replay warnings are still
  `neck_to_spine3` and `guitar_to_head`.
- Direct loose-DLC capture passes automated load/framing, but visual inspection
  still rejects. The pose remains upright but tangled; the current guitar/target
  cluster is bad even when the visible hands are forced onto those targets.
- Evidence retained:
  `r52_visiblehands_to_targets_patch_report_20260820.json`,
  `r52_visiblehands_to_targets_layer_replay_f010_20260820.json`,
  `r52_visiblehands_to_targets_pose_review_20260820.json`,
  `r52_visiblehands_to_targets_visual_20260820/midori_1_hand_overlay_f010.bmp`,
  and `r52_visiblehands_to_targets_visual_decision_20260820.json`.
- Decision: visible hand channel survival and parent-local solving are no
  longer the leading suspect. Next branch should solve the guitar plus
  fret/strum target frame under the Control_Root/model-parent-compensated branch.

### 2026-08-20 r53 upper-chain layer-driver diagnosis

- Treat the lower body as solved. The pelvis-only matrix-local / Control_Root
  work is now a regression gate, not the active failure.
- Added `tools/gh3_midori_synthesize_extreme_layer_driver_probe.py` and
  extended `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py` with
  `--target-layer fret-strum|main`.
- Extreme fret/strum upper-chain channels apply in actual-layer replay, but all
  five approval screenshots are unchanged. This means fret/strum arm rotations
  are not the visible driver for the broken upper pose.
- Extreme main-layer upper-chain channels apply in replay and visibly alter the
  screenshots. A real main-layer GLB-basis probe also changes the screenshots,
  but direct review rejects all five frames: pelvis/legs remain coherent while
  the arms/guitar collapse into an unusable near-vertical pose.
- Evidence retained:
  `analysis/gh3_midori_layer_driver_visual_triage.json`,
  `analysis/gh3_midori_extreme_layer_driver_visual_probe.json`,
  `analysis/gh3_midori_extreme_main_layer_driver_visual_probe.json`,
  `analysis/gh3_midori_visible_arm_main_glbbasis_visual_probe.json`, and
  `analysis/gh3_midori_visible_arm_main_glbbasis_visual_probe_proofs/*.bmp`.
- Decision: do not reopen lower body. The next branch should author the
  corrective upper-chain/guitar posture into the visible-driving main animation
  layer, either as a full main clip patch or an automated GLB-to-main-to-MILO
  bridge. Fret/strum layers should only be used for channels proven visible or
  gameplay-relevant.

### 2026-08-20 r54-r58 main guitar-frame follow-up

- r54 tested main-layer target-only visible arm IK against the promoted loose
  DLC. It keeps Midori bipedal, but direct five-frame capture rejects it because
  the arms are following the bad vertical guitar/proxy frame below/behind the
  body.
- r55 reproduced the prior pair-fit guitar-frame family against the current
  promoted loose baseline. The guitar moves into a plausible across-body band,
  proving the prop/proxy frame can be materialized in ordinary MILO clips, but
  emitting sampled torso/head base-frame channels stretches the neck/head and is
  rejected.
- r56 removed base-frame channel emission. Body/head stay normal and the guitar
  remains across the torso, but hands are still disconnected.
- r57 added final-replay visible hand snapping. It is only a minor improvement
  and remains rejected.
- r58 switched to the visible-arm-chain no-base recipe on the same pair-fit
  guitar frame. This is the current one-frame lead: normal lower body, normal
  body/head, guitar in an across-body playable band, and both hands closer to
  the guitar region. It is still not approved because contact/arm pose remains
  rough.
- Evidence retained:
  `analysis/gh3_midori_r54_r58_main_guitar_frame_visual_triage.json`,
  `analysis/gh3_midori_r58_pairfit_armchain_nobase_bake_manifest.json`, and
  `analysis/gh3_midori_r58_pairfit_armchain_nobase_visual_probe_proofs/midori_1_hand_overlay_f010.bmp`.
- Decision: lower body remains solved. Next work should generalize the r58
  no-base pair-fit guitar-frame recipe into the main-layer approval cases, then
  refine hand contact. Do not emit sampled torso/head base-frame channels until
  their convention is fixed.

### 2026-08-20 r59 multi-case main merge

- Patched `tools/gh3_midori_guitar_frame_hand_bake_probe.py` so main-only
  cases can supply fallback fret/strum overlay clips and a separate fallback
  overlay sample frame.
- Added `tools/gh3_midori_merge_main_pairfit_arm_acp.py`. It samples a
  pair-fit main clipset and a main-layer arm clipset, merges `bone_pos_guitar`
  channels from pair-fit with arm channels, and emits ordinary main ACP clips.
- r59 generated five approval-style main clips from the r58 no-base recipe and
  captured them from loose DLC. All five were visually rejected, but the branch
  is a useful mechanical generalization: Midori remains bipedal and the old
  vertical pole-guitar failure is no longer universal.
- Visual review:
  attack/outfit2 attack move the guitar away from the pole failure but too low;
  jump/solo/transition still swing toward a side-vertical guitar pose; hand
  contact is not believable.
- Evidence retained:
  `analysis/gh3_midori_r59_multicase_pairfit_main_arm_visual_triage.json`,
  `analysis/gh3_midori_r59_merged_main_pairfit_arm_acp_report.json`,
  `analysis/gh3_midori_r59_pairfit_case_bake_manifests/`, and
  `analysis/gh3_midori_r59_merged_main_pairfit_arm_visual_probe_proofs/*.bmp`.
- Decision: do not promote r59. Keep the no-base main-layer merge
  infrastructure, but replace the universal fallback overlay target frame with
  per-main-clip source or GLB hand/guitar target frames before the next
  five-frame capture.

### 2026-08-20 r60-r61 source target pair-fit comparison

- r60 reused the r59 no-base main-layer merge infrastructure but replaced the
  universal fallback overlay target frame with per-main-clip
  `source-ik-helper-locals` from the pinned source pose bridge manifest.
- r61 repeated the same comparison with `source-palm-locals`.
- Both branches preserve the solved bipedal lower body, but both are visually
  rejected. r60 often returns to vertical/behind-body guitar placement or
  places the guitar too far left/high. r61 is worse: hands/guitar explode
  outward, hide behind the torso/legs, or become too high/vertical.
- Evidence retained:
  `analysis/gh3_midori_r60_r61_source_target_pairfit_visual_triage.json`,
  `analysis/gh3_midori_r60_sourceik_pairfit_case_bake_manifests/`,
  `analysis/gh3_midori_r61_sourcepalm_pairfit_case_bake_manifests/`,
  `analysis/gh3_midori_r60_sourceik_merged_main_pairfit_arm_visual_probe_proofs/*.bmp`,
  and
  `analysis/gh3_midori_r61_sourcepalm_merged_main_pairfit_arm_visual_probe_proofs/*.bmp`.
- Decision: do not use raw source IK helper or raw source palm pair-fit targets
  directly. Keep the r58/r59 no-base merge infrastructure, but calibrate
  source/GLB target frames into the canonical-emitted proxy frame or solve a
  per-pose guitar frame from r58-style across-body constraints before another
  five-frame capture.

### 2026-08-20 r62 fixed-r58 guitar frame check

- Added `tools/gh3_midori_fixed_main_guitar_acp.py` to emit a fixed main-layer
  `bone_pos_guitar` transform across selected approval cases.
- r62 fixed the guitar to the r58 emitted local transform
  `pos=[13.311804,-8.638128,-1.333621]` and
  `quat_xyzw=[-0.144327,-0.563363,-0.576009,0.574461]`, then merged those two
  guitar channels with the existing main-layer visible-arm overlay channels.
- Five-frame loose-DLC capture confirms the lower body is still solved:
  pelvis/legs remain bipedal on all r62 proof images. Treat
  pelvis/Control_Root as a regression gate only.
- r62 is still visually rejected. The guitar is generally stable in a plausible
  across-body band, but the arms/hands read too idle/default and do not achieve
  believable fret/strum contact. The transition-out frame is the best r62
  placement lead, not an approval pass.
- Evidence retained:
  `analysis/gh3_midori_r62_fixedr58_visual_triage.json`,
  `analysis/gh3_midori_r62_fixed_guitar_acp_report.json`,
  `analysis/gh3_midori_r62_fixedr58_merged_main_pairfit_arm_acp_report.json`,
  and
  `analysis/gh3_midori_r62_fixedr58_merged_main_pairfit_arm_visual_probe_proofs/*.bmp`.
- Decision: do not reopen lower-body diagnosis. Continue with upper-body/contact
  work: use a calibrated GLB/source-to-MILO animation bridge or solve per-pose
  guitar frames from r58-style constraints, with real arm animation/contact
  driving the hands instead of weak target-only overlays.

### 2026-08-20 r63-r64 source-calibrated visible arm check

- Patched `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py` so the
  r68 visible-hand-axis calibration can be applied after
  `source-guitar-local` orientation, not only in
  `canonical-visible-mesh-axis` mode.
- r63 merged the fixed r58 main `bone_pos_guitar` transform with main-layer
  source-guitar-local orientation applied to upper arm, forearm, and hand, plus
  calibrated visible hand axes. Direct five-frame loose-DLC capture rejects:
  the lower body and guitar frame remain stable, but source upper-arm rotations
  fold the arms behind the shoulders/torso.
- r64 kept the same fixed guitar frame and calibrated hand axis path, but
  applied source orientation only to forearm and hand while leaving upper arms
  on the target IK solve. Direct capture rejects again: it avoids much of the
  behind-back fold, but arms spread into T-pose-like or across-torso poses
  instead of playable fret/strum contact.
- Evidence retained:
  `analysis/gh3_midori_r63_r64_sourcecal_visual_triage.json`,
  `analysis/gh3_midori_r63_sourcecal_main_arm_acp_report.json`,
  `analysis/gh3_midori_r63_sourcecal_merged_main_acp_report.json`,
  `analysis/gh3_midori_r63_sourcecal_visual_probe_proofs/*.bmp`,
  `analysis/gh3_midori_r64_forehand_sourcecal_main_arm_acp_report.json`,
  `analysis/gh3_midori_r64_forehand_sourcecal_merged_main_acp_report.json`,
  and
  `analysis/gh3_midori_r64_forehand_sourcecal_visual_probe_proofs/*.bmp`.
- Decision: this is a useful negative. Source GLB/pose bridge data now visibly
  reaches ordinary main-layer MILO channels, but raw source rotation basis is
  wrong for the visible arm performance channels. Next branch should derive a
  visible-arm-space correction or stop copying raw source rotations: solve
  upper/forearm rotations from source shoulder/elbow/hand positions with an
  explicit elbow plane and fixed r58 guitar/contact targets, then emit the
  solved main-layer channels.

### 2026-08-20 r65 source forearm elbow-hint check

- Patched `tools/gh3_midori_guitar_frame_hand_bake_probe.py` so the visible
  two-bone position solver accepts an optional `elbow_hint_world`, preserving
  current-elbow behavior when omitted.
- Patched `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py` with
  `--source-elbow-hint source-forearm-position`. The source forearm position is
  runtime-fitted through the source hand-IK-to-current hand-target map, then
  used only as the elbow bend hint for the MILO visible arm solve.
- r65 kept the fixed r58 main guitar frame and r68 visible hand-axis
  calibration, avoided raw source upper/forearm rotations, and captured five
  loose-DLC proof frames.
- Visual result: rejected safe negative. Lower body and guitar frame remain
  stable/bipedal in all five frames, and r65 avoids the r63/r64 explosive arm
  folds. However, the arms regress toward r62 low/default shapes and still do
  not read as playable fret/strum contact.
- Evidence retained:
  `analysis/gh3_midori_r65_elbowhint_visual_triage.json`,
  `analysis/gh3_midori_r65_elbowhint_main_arm_acp_report.json`,
  `analysis/gh3_midori_r65_elbowhint_merged_main_acp_report.json`, and
  `analysis/gh3_midori_r65_elbowhint_visual_probe_proofs/*.bmp`.
- Decision: do not retry elbow hint alone. Next branch needs to solve hand
  target/contact positions and elbow plane together from source
  shoulder/elbow/hand geometry, or construct per-pose visible hand targets
  nearer the fixed guitar neck/body before solving the arm chain.

### 2026-08-20 r66 fixed-guitar target-coupled arm solve

- Patched `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py` with
  `--target-main-guitar-pos` and `--target-main-guitar-quat-xyzw`. These apply
  the fixed r58 main `bone_pos_guitar` local transform before reading
  `bone_fret_hand` / `bone_strum_hand` targets, so the visible arm solve is
  coupled to the final guitar frame instead of the pre-fixed baseline target
  frame.
- r66 used the fixed r58 guitar transform, full arm reach scale `1.0`, the r65
  runtime-fitted source forearm elbow hint, and r68 visible hand-axis
  calibration.
- Direct five-frame loose-DLC capture rejects, but it is a real mechanical
  lead: lower body remains solved, the guitar frame stays stable, and hands are
  more consistently in the fixed guitar band than r65. However, the arms still
  read low/default and do not form a believable performance pose or clear
  fret/strum contact.
- Evidence retained:
  `analysis/gh3_midori_r66_fixedtarget_visual_triage.json`,
  `analysis/gh3_midori_r66_fixedtarget_main_arm_acp_report.json`,
  `analysis/gh3_midori_r66_fixedtarget_merged_main_acp_report.json`, and
  `analysis/gh3_midori_r66_fixedtarget_visual_probe_proofs/*.bmp`.
- Decision: keep target-frame coupling enabled in future branches. The next
  missing piece is not pelvis, not raw guitar placement, and not pre/post target
  mismatch; it is per-pose visible hand targets and arm-pose priors around the
  fixed guitar neck/body.

### 2026-08-20 r67-r68 raised hand-target prior check

- Patched `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py` with
  `--left-target-world-offset` and `--right-target-world-offset`, allowing
  explicit visible hand target priors on top of the fixed-guitar fret/strum
  proxy targets.
- r67 kept fixed-guitar target coupling and raised both hand targets by
  `0,0,7`. Numeric reports improve sharply: the first two cases move target Z
  from about `36/34` to about `43/41`, and the solver reaches the raised targets
  with near-zero final hand distance. Direct capture still rejects; the visible
  arms remain low/default and do not show the expected raised performance
  silhouette.
- r68 repeated r67 as rotations-only, with no visible arm position channels.
  This also rejects and remains visually close to r67/r66. Therefore the failure
  is not just that emitted position channels are masking the lift.
- Evidence retained:
  `analysis/gh3_midori_r67_r68_targetoffset_visual_triage.json`,
  `analysis/gh3_midori_r67_targetoffset_main_arm_acp_report.json`,
  `analysis/gh3_midori_r67_targetoffset_merged_main_acp_report.json`,
  `analysis/gh3_midori_r67_targetoffset_visual_probe_proofs/*.bmp`,
  `analysis/gh3_midori_r68_targetoffset_rotonly_main_arm_acp_report.json`,
  `analysis/gh3_midori_r68_targetoffset_rotonly_merged_main_acp_report.json`,
  and
  `analysis/gh3_midori_r68_targetoffset_rotonly_visual_probe_proofs/*.bmp`.
- Decision: keep fixed-guitar target coupling, but stop expecting hand target
  offsets alone to fix the silhouette. The active missing piece is a visible
  clavicle/upper-arm/forearm pose prior around the guitar before forearm/hand
  contact is solved.

### 2026-08-21 r69 y-axis visible arm driver check

- r69 merged the fixed r58 main `bone_pos_guitar` pairfit channels with a
  main-layer y-axis visible arm driver on clavicle, upperArm, foreArm, and hand
  quats for both sides.
- Direct five-frame loose-DLC capture rejects. Lower body is still solved and
  the fixed guitar frame stays coherent, but the arms do not form playable
  fret/strum contact. The common `y +60` driver is too strong and not
  side-specific: it lifts/crosses one arm into the chest while the other side
  remains wrong or hanging.
- Evidence retained:
  `analysis/gh3_midori_r69_yaxis_driver_visual_triage.json`,
  `analysis/gh3_midori_r69_yaxis_driver_acp_report.json`,
  `analysis/gh3_midori_r69_fixed_guitar_acp_report.json`,
  `analysis/gh3_midori_r69_yaxis_driver_merged_main_acp_report.json`, and
  `analysis/gh3_midori_r69_yaxis_driver_visual_probe_proofs/*.bmp`.
- Decision: r69 proves the visible clavicle/upper-arm/forearm/hand rotation
  channels are effective and that y-axis upper-chain rotation is a real lead.
  Do not promote it. Next branch should keep pelvis/lower body and fixed guitar
  locked, then test smaller side-specific or opposed y-axis upper-chain priors
  before blending back into the fixed-guitar hand/contact solve.

### 2026-08-21 r70 side-specific upper-chain probe support

- Patched `tools/gh3_midori_synthesize_extreme_layer_driver_probe.py` with
  optional `--left-degrees` and `--right-degrees`. Existing `--degrees` remains
  the default for both sides, so older r69/extreme-driver commands stay
  reproducible.
- Added unit coverage in `tools/gh3_midori_pipeline_test.py` proving the probe
  can emit different left/right quats for the same axis.
- Intended next visual probe: fixed r58 guitar plus smaller/opposed y-axis
  visible upper-chain priors, for example `--axis y --left-degrees 24
  --right-degrees -18 --target-layer main`, with the same direct five-frame
  bipedal/contact rejection gate.

### 2026-08-21 r70 side-specific y-axis visual probe

- Built a loose-DLC-only r70 candidate from the current live support MILOs and
  a rebuilt main animation MILO. The main MILO merges the fixed r58 guitar
  frame with side-specific y-axis visible arm priors:
  `--axis y --left-degrees 24 --right-degrees -18 --target-layer main`.
- Direct five-frame capture rejects. All five frames remain bipedal and the
  guitar frame stays coherent, so lower body and fixed guitar did not regress.
  However, the upper body reads frozen/default-like across different clips. The
  hands still do not make believable fret/strum contact, and the alternate
  outfit exposes a hanging/warped arm shape.
- Evidence retained:
  `analysis/gh3_midori_r70_side_yaxis_visual_triage.json`,
  `analysis/gh3_midori_r70_side_yaxis_visual_probe.json`,
  `analysis/gh3_midori_r70_side_yaxis_arm_acp_report.json`,
  `analysis/gh3_midori_r70_fixed_guitar_acp_report.json`,
  `analysis/gh3_midori_r70_side_yaxis_merged_main_acp_report.json`, and
  `analysis/gh3_midori_r70_side_yaxis_visual_probe_proofs/*.bmp`.
- Decision: do not promote r70. A static side-specific upper-chain prior is a
  safe containment strategy but not a contact solve. Next branch should blend a
  mild side-specific clavicle/upper-arm prior with the fixed-guitar
  target-coupled visible hand/contact solve, so per-pose source motion can move
  the hands while the prior keeps the shoulders in a playable band.

### 2026-08-21 r71 clavicle-prior plus contact solve

- Patched `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py` with
  optional clavicle local-axis priors:
  `--left-clavicle-prior-axis`,
  `--left-clavicle-prior-degrees`,
  `--right-clavicle-prior-axis`,
  `--right-clavicle-prior-degrees`, and
  `--clavicle-prior-compose-order`.
- Added unit coverage in `tools/gh3_midori_pipeline_test.py`; the full focused
  suite passed with 111 tests before capture.
- r71 reused the r66 fixed-guitar target-coupled visible hand/contact solve,
  then post-composed the r70 side-specific y prior on clavicles only:
  left `y +24`, right `y -18`.
- Direct five-frame loose-DLC capture rejects. The lower body remains bipedal
  and the fixed r58 guitar frame stays coherent, but the shoulder/arm band is
  visibly wrong. In fast jump/solo frames, an upper limb flares or detaches
  backward from the shoulder/sleeve area; the hands still do not read as
  playable fret/strum contact.
- Evidence retained:
  `analysis/gh3_midori_r71_clavprior_contact_visual_triage.json`,
  `analysis/gh3_midori_r71_clavprior_contact_visual_probe.json`,
  `analysis/gh3_midori_r71_clavprior_contact_main_arm_acp_report.json`,
  `analysis/gh3_midori_r71_fixed_guitar_acp_report.json`,
  `analysis/gh3_midori_r71_clavprior_contact_merged_main_acp_report.json`, and
  `analysis/gh3_midori_r71_clavprior_contact_visual_probe_proofs/*.bmp`.
- Decision: do not promote r71. The contact solve is active, but the r70
  post-composed clavicle signs/magnitudes are not calibrated. Next branch
  should isolate clavicle calibration with smaller magnitudes, pre-compose, or
  opposite signs before blending it into the full contact solve again. Reject
  immediately on shoulder flare or non-bipedal silhouette.

### 2026-08-21 r72 smaller pre-composed clavicle/contact calibration

- r72 reused the r66 fixed-guitar target-coupled contact solve and changed only
  the clavicle prior calibration from r71:
  `--clavicle-prior-compose-order pre`,
  left `y -12`, right `y +9`.
- Direct five-frame loose-DLC capture rejects. Lower body remains bipedal and
  the fixed r58 guitar frame stays coherent. The r71 shoulder explosion is
  reduced, but not eliminated; fast jump/solo/transition frames still show a
  rear shoulder/upper-arm/sleeve flare behind the body/head.
- Hand/contact placement remains wrong. The fret-side hand stays biased near
  the guitar body/bridge instead of the neck, and the right arm still reads
  low/default on the alternate outfit.
- Evidence retained:
  `analysis/gh3_midori_r72_clavpre_contact_visual_triage.json`,
  `analysis/gh3_midori_r72_clavpre_contact_visual_probe.json`,
  `analysis/gh3_midori_r72_clavpre_contact_main_arm_acp_report.json`,
  `analysis/gh3_midori_r72_fixed_guitar_acp_report.json`,
  `analysis/gh3_midori_r72_clavpre_contact_merged_main_acp_report.json`, and
  `analysis/gh3_midori_r72_clavpre_contact_visual_probe_proofs/*.bmp`.
- Decision: do not promote r72. Keep the safer lesson that smaller
  pre-composed/opposite-sign clavicle priors reduce flare, but stop tuning
  clavicles alone. Next branch should inspect/correct fixed-guitar fret/strum
  target placement or side assignment, because the solver is steering the fret
  hand toward the body/bridge and leaving the right arm low.

### 2026-08-21 r73 swapped visible hand targets

- Patched `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py` with
  `--swap-visible-hand-targets`. Default behavior is unchanged; when enabled,
  the visible left arm solves against `bone_strum_hand.mesh` and the visible
  right arm solves against `bone_fret_hand.mesh`.
- r73 reused the r66 fixed-guitar target-coupled contact solve, enabled the
  target swap, and disabled clavicle priors to isolate side assignment.
- Direct five-frame loose-DLC capture rejects. Lower body remains bipedal and
  the fixed r58 guitar frame stays coherent, but the arms tuck behind/inside
  the body more than before and still do not make playable neck/body contact.
  The alternate outfit confirms both hands remain around the body/bridge zone
  rather than forming a convincing fret-on-neck plus strum-at-body split.
- Evidence retained:
  `analysis/gh3_midori_r73_swaptarget_contact_visual_triage.json`,
  `analysis/gh3_midori_r73_swaptarget_contact_visual_probe.json`,
  `analysis/gh3_midori_r73_swaptarget_contact_main_arm_acp_report.json`,
  `analysis/gh3_midori_r73_fixed_guitar_acp_report.json`,
  `analysis/gh3_midori_r73_swaptarget_contact_merged_main_acp_report.json`, and
  `analysis/gh3_midori_r73_swaptarget_contact_visual_probe_proofs/*.bmp`.
- Decision: do not promote r73. A simple left/right proxy target swap is not
  the missing fix. Next branch should author explicit fixed-guitar-local
  visible hand target positions: place the fret target along the neck and the
  strum target near the body, then solve the visible arm chain against those
  authored targets instead of consuming the proxy nodes verbatim.

### 2026-08-21 r74 authored fixed-guitar-local targets

- Patched `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py` with
  `--left-target-guitar-local` and `--right-target-guitar-local`, allowing
  explicit visible hand targets in the sampled fixed guitar frame. Added unit
  coverage in `tools/gh3_midori_pipeline_test.py`.
- Corrected target-local measurement through the same `main_pose_transforms`
  path used by the synth:
  `bone_fret_hand.mesh` is approximately `[-3.102, -0.159, 22.416]` in fixed
  guitar local space, and `bone_strum_hand.mesh` is approximately
  `[5.442, -6.533, 1.627]`.
- r74 kept normal side assignment and authored the fret target farther along
  the apparent neck axis: left/fret `[-3.10, -0.16, 34.0]`, right/strum
  `[5.44, -6.53, 1.63]`. Clavicle priors were disabled.
- Direct five-frame loose-DLC capture rejects. Lower body remains bipedal and
  the fixed r58 guitar frame stays coherent, but the authored target does not
  visibly pull the skinned left hand onto the neck. The arms remain low,
  tucked, or hidden around the guitar body across both outfits.
- Evidence retained:
  `analysis/gh3_midori_r74_authoredtarget_visual_triage.json`,
  `analysis/gh3_midori_r74_authoredtarget_visual_probe.json`,
  `analysis/gh3_midori_r74_authoredtarget_main_arm_acp_report.json`,
  `analysis/gh3_midori_r74_fixed_guitar_acp_report.json`,
  `analysis/gh3_midori_r74_authoredtarget_merged_main_acp_report.json`, and
  `analysis/gh3_midori_r74_authoredtarget_visual_probe_proofs/*.bmp`.
- Decision: do not promote r74. The target override path works structurally,
  but moving the target numerically along the guitar does not create visible
  neck contact. Next branch should audit whether emitted visible foreArm/hand
  `.pos` channels actually move the rendered skinned hand as expected, and
  inspect upper-chain basis/reach before more target-coordinate tuning.

### 2026-08-21 r75 visible position-channel driver probe

- Added `tools/gh3_midori_synthesize_visible_pos_driver_probe.py`, a small
  main-layer diagnostic generator that emits only visible arm `.pos` channels:
  `bone_L/R-foreArm.mesh.pos` and `bone_L/R-hand.mesh.pos`.
- r75 merged the fixed r58 guitar frame with deliberately extreme position-only
  channels: both visible hand local positions set to `[0, 0, 80]`, forearms to
  `[0, 0, 0]`.
- Direct five-frame loose-DLC capture rejects as expected, but the diagnostic
  result is positive: all five frames visibly deform/move the rendered
  forearms/hands. The alternate outfit confirms the same behavior.
- Evidence retained:
  `analysis/gh3_midori_r75_posdriver_visual_triage.json`,
  `analysis/gh3_midori_r75_posdriver_visual_probe.json`,
  `analysis/gh3_midori_r75_posdriver_arm_acp_report.json`,
  `analysis/gh3_midori_r75_fixed_guitar_acp_report.json`,
  `analysis/gh3_midori_r75_posdriver_merged_main_acp_report.json`, and
  `analysis/gh3_midori_r75_posdriver_visual_probe_proofs/*.bmp`.
- Decision: r75 is not a candidate, but it rules out the hypothesis that r74
  failed because visible foreArm/hand `.pos` channels are ignored. Position
  channels are live. Next branch should inspect solve basis/reach and the local
  direction/magnitude relationship between emitted hand positions and rendered
  movement; further guitar-target-coordinate tuning alone is unlikely to fix
  the pose.

### 2026-08-21 r76 visible position-axis sweep

- Extended `tools/gh3_midori_synthesize_visible_pos_driver_probe.py` with
  `--pattern axis-sweep` and unit coverage. The diagnostic keeps foreArm
  `.pos` channels at `[0, 0, 0]` and assigns per-case visible hand positions:
  `+X`, `+Y`, `+Z`, `-X`, `-Y` at magnitude `80`.
- Built the r76 loose-DLC diagnostic candidate through
  `milo_convert_tool build-clipset-from-acp ... --move-self 0
  --control-root-pelvis-parent`, then merged fixed r58 `bone_pos_guitar`
  channels with the axis-sweep visible hand `.pos` channels. No ISO path was
  used at game/capture time.
- Direct five-frame loose-DLC capture rejects as expected. Lower body remains
  coherent and the fixed guitar frame remains stable, but all five poses are
  obvious upper-body/arm rejects rather than playable guitar poses.
- Axis observations from visual inspection:
  `+X` and `-X` keep hands lower around the body/guitar area with severe arm
  stretch; `+Y` moves endpoints up toward shoulder/chest; `+Z` also deforms
  around upper torso/shoulder; `-Y` on the alternate outfit confirms the same
  deformation pattern. None create fret-on-neck plus strum-at-body contact.
- Evidence retained:
  `analysis/gh3_midori_r76_posaxis_visual_triage.json`,
  `analysis/gh3_midori_r76_posaxis_visual_probe.json`,
  `analysis/gh3_midori_r76_posaxis_arm_acp_report.json`,
  `analysis/gh3_midori_r76_fixed_guitar_acp_report.json`,
  `analysis/gh3_midori_r76_posaxis_merged_main_acp_report.json`, and
  `analysis/gh3_midori_r76_posaxis_visual_probe_proofs/*.bmp`.
- Decision: r76 is not a candidate. It confirms local hand `.pos` axes are
  live/directional, but hand-only position driving stretches the visible chain
  instead of solving elbow/upper-arm contact. Next branch should derive and
  emit a coherent upper/forearm/hand local chain from the visible skeleton,
  using r76 only as the basis/direction diagnostic.

### 2026-08-21 r77-r78 source arm-direction elbow plane and reachable target

- Added `--source-elbow-hint source-arm-direction` to
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py`. This uses the
  source upper-to-forearm direction only, normalized and scaled to the current
  GH2 visible upper-arm length, as the two-bone solver elbow-plane hint. Added
  unit coverage in `tools/gh3_midori_pipeline_test.py`.
- r77 held the r74 authored target setup constant:
  left/fret `[-3.10, -0.16, 34.0]`, right/strum `[5.44, -6.53, 1.63]`,
  fixed r58 guitar, source hand orientation, and visible mesh-axis calibration.
  The only intended change was the source-arm-direction elbow hint. Direct
  loose-DLC capture rejects. The model stays bipedal and the guitar remains
  stable, but the arms read low/default and the fret hand stays around the
  body/bridge instead of the neck.
- r78 kept the same r77 setup but moved the left/fret target to
  `[-3.10, -0.16, 26.0]` to reduce left-arm overreach. Numeric overreach
  improved, especially attack frames, but the direct loose-DLC proofs still
  reject with the same visual failure: stable/default-ish arms and no playable
  fret contact.
- Evidence retained:
  `analysis/gh3_midori_r77_sourcearmdir_visual_triage.json`,
  `analysis/gh3_midori_r77_sourcearmdir_visual_probe.json`,
  `analysis/gh3_midori_r77_sourcearmdir_main_arm_acp_report.json`,
  `analysis/gh3_midori_r77_fixed_guitar_acp_report.json`,
  `analysis/gh3_midori_r77_sourcearmdir_merged_main_acp_report.json`,
  `analysis/gh3_midori_r77_glb_source_guitar_basis_report.json`,
  `analysis/gh3_midori_r77_sourcearmdir_visual_probe_proofs/*.bmp`,
  `analysis/gh3_midori_r78_reachabletarget_visual_triage.json`,
  `analysis/gh3_midori_r78_reachabletarget_visual_probe.json`,
  `analysis/gh3_midori_r78_reachabletarget_main_arm_acp_report.json`,
  `analysis/gh3_midori_r78_reachabletarget_merged_main_acp_report.json`, and
  `analysis/gh3_midori_r78_reachabletarget_visual_probe_proofs/*.bmp`.
- Decision: do not promote r77 or r78. Normalized source elbow direction avoids
  extreme mangling but does not create a performance pose, and reducing left
  target overreach alone does not put the fret hand on the neck. Next branch
  needs a stronger performance-pose source for the visible upper chain, not
  more single-parameter elbow/target tuning.

### 2026-08-21 r115 source orientation apply-blend checkpoint

- Lower body / pelvis / `Control_Root` should be considered solved and locked
  to regression coverage only. The active problem is upper-chain silhouette:
  clavicle/upper arm/forearm/hand coherence while preserving fret/strum contact.
- Finished wiring `source_orientation_apply_blend` through the visible-arm
  synthesis tool, synth-vs-built equivalence replay, and direct `solve_side`
  unit tests. Also fixed the source-load guard so GLB/source rows load whenever
  `--source-orientation-apply-to != none`, even when
  `--hand-orientation-mode reference-relative`.
- Verification: `python -m py_compile` passed for the edited synthesis and
  equivalence tools; `tools/gh3_midori_pipeline_test.py` passed 126 tests.
- Low-priority structural sweep only, no game capture: using the five-case GLB
  source bridge with `source_orientation_sides=left`,
  `source_orientation_space=glb-target-basis`,
  `emit_visible_arm_positions=false`, and `emit_hand_target_rotation=true`.
  Source rows loaded for all five cases, but any nonzero post-solve source
  orientation blend broke left/fret contact. `forearm-and-hand` blend `0.05`
  produced left max/avg contact error `1.408984/1.255321`; `0.10` produced
  `2.812145/2.507008`. `upper-forearm-hand` blend `0.05` was worse at
  `4.174171/3.664875`.
- Evidence: `analysis/gh3_midori_r115_source_applyblend_structural_triage.json`.
  Decision: reject this post-solve source-rotation-copy path without visual
  capture. Next route should apply source-pose influence inside a constrained
  solve that preserves endpoint contact, rather than after the contact solve.

### 2026-08-21 r116 post-source contact recovery rejection

- Added default-off post-source endpoint recovery options to
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py`:
  `--source-orientation-contact-refine-iterations` and
  `--source-orientation-contact-refine-strength`. These rerun the existing
  length-preserving CCD arm refinement after source orientation application and
  before final hand-target orientation. Wired the same fields through synth
  reports, CLI, synth-vs-built equivalence replay, and direct `solve_side`
  tests.
- Verification: `py_compile` passed for the edited synth/equivalence tools;
  `tools/gh3_midori_pipeline_test.py` passed 126 tests.
- Structural result: this fixes the r115 contact blow-up. With
  `source_orientation_apply_to=forearm-and-hand`,
  `source_orientation_apply_blend=0.10`, and 20 contact-recovery passes, source
  rows loaded for all five cases, left/fret contact stayed
  `max/avg=0.052434/0.051664`, and generated arm quats changed up to
  `11.498885` degrees versus zero blend. `upper-forearm-hand` was worse and
  should not be pursued from this branch.
- Build/capture correction: a first sparse-overlay capture was invalid because
  it replaced main with a four-clip overlay-only clipset. The valid r116 visual
  capture rebuilt the full 266-clip stockattach main stage, merged r116 overlay
  channels into the four review clips, built a full main MILO, and captured
  from local `gh2_ps2_hybrid_assets/GEN` plus loose
  `gh2_ps2_hybrid_assets/DLC`. No ISO was used, and live DLC hashes were
  restored.
- Built replay equivalence was close enough for visual diagnosis:
  `max_hand_delta=0.012660`, `max_distance_gap=0.012658`. Direct visual still
  rejects all five proofs: body/lower/guitar are stable, but the fretting arm
  remains hidden/mechanically pinned and the cases are too samey to read as
  natural stock-quality performance.
- Evidence: `analysis/gh3_midori_r116_contactrefine_visual_triage.json`,
  `analysis/gh3_midori_r116_full_contactrefine_visual_probe.json`, and
  `analysis/gh3_midori_r116_full_contactrefine_visual_probe_proofs/*.bmp`.
  Decision: reject r116. Next branch should change the actual visible
  elbow/forearm path, likely from constrained source positions or mesh
  attribution, instead of blending source rotations and recovering endpoint
  contact afterward.

### 2026-08-21 r117 source-position elbow-path rejection

- Swept existing `--source-elbow-hint` modes with source orientation copying
  disabled. Baseline no-hint left contact max/avg was `0.019064/0.018886`.
  The captured variant used `source-forearm-position`,
  `source_position_space=fit-stock-hand-targets`, and
  `source_orientation_space=glb-target-basis`. It loaded source rows for all
  five cases, changed generated quats up to `15.561263` degrees versus no hint,
  and kept left contact bounded at `0.563408/0.206280`.
- Built the candidate correctly as a full 266-clip stockattach main MILO with
  r117 overlay channels merged into only the four review clips. Capture used
  local `gh2_ps2_hybrid_assets/GEN` plus loose
  `gh2_ps2_hybrid_assets/DLC`; no ISO was used, and live DLC hashes were
  restored afterward.
- Direct visual still rejects all five proofs. Lower body and guitar remain
  stable, but the fretting arm is still hidden/mechanically pinned and the
  cases remain too samey. Built replay is bounded but less tight than r116
  (`max_hand_delta=0.581660`, `max_distance_gap=0.576327`).
- Ran one focused runtime pose-mesh dump before cleanup. Summary shows the
  rendered left forearm/hand parts are present near guitar/face height:
  `midori_1_mesh0_part13.mesh` is dominated by `bone_L-foreArm.mesh`
  (`94.6875`) and `bone_L-upperArm.mesh` (`20.8125`) with positive-y
  face-band vertices; left hand/finger parts are also present. This means
  source elbow hints move generated bones but still do not move the rendered
  parts into a readable silhouette.
- Evidence: `analysis/gh3_midori_r117_sourceforearm_visual_triage.json`,
  `analysis/gh3_midori_r117_meshdump_focus_report.json`,
  `analysis/gh3_midori_r117_sourceforearm_visual_probe.json`, and
  `analysis/gh3_midori_r117_sourceforearm_visual_probe_proofs/*.bmp`.
  Decision: reject r117. Next branch should use runtime mesh-dump part bounds,
  especially `midori_1_mesh0_part13.mesh` and left hand/finger parts, as the
  objective for elbow/upper-arm tuning rather than endpoint contact/source
  elbow hints alone.

### 2026-08-21 r118 explicit elbow-vector mesh-bound rejection

- Lower body/root was treated as solved and regression-only; no pelvis/Control_Root
  branch was reopened. Capture used loose `gh2_ps2_hybrid_assets/GEN` plus
  `gh2_ps2_hybrid_assets/DLC`, not an ISO, and the capture child was launched
  at low priority.
- Added `tools/gh3_midori_pose_mesh_part_report.py` to summarize runtime
  `--char-pose-mesh-dump` JSONL by selected rendered mesh parts and bones.
- Tested rotation-only explicit left elbow hints for the attack frame:
  no hint, `0,12,0`, `-10,12,-4`, and `0,8,-10`. All four generated identical
  attack ACP hashes (`4DA99D02D06DB4F0EC7AD057AC53D326C3977578381CF2926517C9355079B271`).
  Captured `outup` and `forwardup`; both failed `visible_difference_from_idle`
  and produced identical rendered bounds. `midori_1_mesh0_part13.mesh` center was
  `[-7.90928, 3.24124, 53.5188]`, essentially unchanged from r117
  (`[-7.842, 3.442, 53.557]`).
- Diagnosis: the left hand target is `14.788225` units from final hand and
  `14.773075` units beyond the rotation-only chain reach, so the two-bone solver
  collapses to the same fully extended solution and the elbow hint has no visual
  leverage. Do not continue sweeping explicit elbow vectors while the ACP hash is
  unchanged.
- Evidence: `analysis/gh3_midori_r118_elbow_mesh_probe_report.json` and latest
  proof image `analysis/gh3_midori_r118_latest_elbow_mesh_probe.bmp`.
  Live DLC hashes after capture matched the promoted main/fret/strum hashes.
  Next branch should make the left target reachable, or move to a direct
  rendered-part/GLB-to-MILO retarget route rather than endpoint-only elbow hints.

### 2026-08-21 r119 source-position bridge guard fixes and rejection

- Lower body/root remains solved and regression-only. Runtime captures used loose
  `gh2_ps2_hybrid_assets/GEN` plus `gh2_ps2_hybrid_assets/DLC`, no ISO, and
  child work was launched at low priority. Live DLC hashes matched promoted
  main/fret/strum after capture.
- Fixed two source-position bridge bugs in
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py`:
  source rows now load when `--source-position-apply-to != none` even if source
  orientation copying and elbow hints are disabled, and
  `fit-runtime-hand-targets` now applies whenever that position space is
  requested instead of only with `hand_orientation_mode=source-guitar-local`.
  Also extended the synth report to serialize source-position world targets and
  emitted forearm/hand local positions. `py_compile` passed and
  `tools/gh3_midori_pipeline_test.py` passed 126 tests.
- One-case attack probes:
  `posbase` changed the ACP versus r118 but failed visual; part13 moved to
  `[-7.01847, 1.38752, 56.3348]`, a folded upper-arm slab.
  After the guard fix, direct runtime source-position fit loaded source rows but
  exploded (`fit_scale=76.347234`, left source hand around
  `[39.019, 40.396, 93.65]`) and was rejected before capture.
  Stock-fit source positions were bounded (`fit_scale=5.148025`) but still
  failed visual; part13 was `[-7.08176, 1.63781, 56.1226]` and most left-hand
  parts remained pinned.
- Evidence: `analysis/gh3_midori_r119_source_position_bridge_report.json` and
  `analysis/gh3_midori_r119_source_position_probe.bmp`. Decision: reject
  unconstrained source-position fits. Next branch should use the fixed reporting
  to build a constrained fit: source arm shape/direction from
  `Bone_Bicep_L`/`Bone_Forearm_L`/`Bone_Palm_L`, palm anchored to the runtime
  fret target, and clamped source scale/forearm local translation before capture.

### 2026-08-21 r120 constrained anchored source-position rejection

- Lower body/root remains solved and regression-only. Runtime captures used loose
  `gh2_ps2_hybrid_assets/GEN` plus `gh2_ps2_hybrid_assets/DLC`, no ISO, and
  child work was launched at low priority. Live DLC hashes matched promoted
  main/fret/strum after capture.
- Added `anchored-runtime-hand-targets` to
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py`. This fit anchors
  each palm to the runtime fret/strum target and clamps the source arm-shape
  scale before applying source-relative forearm/upper offsets. Added
  `--source-position-max-scale`, report serialization for that scale, and a
  focused pipeline test proving the fit clamps raw helper scale and anchors both
  hands exactly. `py_compile` passed and `tools/gh3_midori_pipeline_test.py`
  passed 127 tests.
- One-case attack probes:
  `anchored5` and `anchored2` both achieved exact left/right hand contact after
  clamping the raw source scale `76.347234` to `5.0` or `2.0`, but visually
  failed by creating a long dangling source-forearm strand behind the guitar.
  `anchored2` part13 center was `[-10.4532, 9.40646, 48.3732]` with only `7`
  positive-y face-band vertices. `anchoredhand` applied only the palm anchor;
  it removed the dangling strand but threw the forearm across the face/head
  area, with part13 center `[2.54073, 3.79444, 56.4967]`.
- Evidence: `analysis/gh3_midori_r120_constrained_anchor_report.json` and
  `analysis/gh3_midori_r120_constrained_anchor_probe.bmp`. Decision: reject r120
  visually, but keep the constrained fit as a useful primitive because it proves
  exact palm contact without exploding scale. Next branch should constrain the
  left forearm/upper-arm path against rendered part13 bounds directly: keep palm
  anchored, then steer part13 away from face/head and guitar occlusion using a
  target face-band/positive-y criterion or an explicit upper/forearm silhouette
  direction.

### 2026-08-21 r121 explicit forearm override rejection

- Lower body/root remains solved and regression-only. Runtime captures used
  loose `gh2_ps2_hybrid_assets/GEN` plus `gh2_ps2_hybrid_assets/DLC`, no ISO,
  and child work was kept at low priority.
- Added explicit `--left-source-forearm-guitar-local` and
  `--right-source-forearm-guitar-local` overrides to
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py`, with report
  serialization for the selected forearm target. This tested whether the r120
  anchored-palm bridge only needed a direct forearm target rather than another
  source-scale/elbow-hint sweep.
- Five one-case attack variants produced distinct ACP hashes while preserving
  exact hand contact: `fore_low`, `fore_lowback`, `fore_mid`, `fore_out`, and
  `fore_outlow`. Captured `fore_mid` and `fore_lowback`; both failed
  `visible_difference_from_idle` and are direct visual rejects because the lower
  body/guitar are stable but the fretting arm/sleeve remains non-bipedal.
  `fore_mid` part13 center was `[-10.2383, 9.81599, 49.2079]`; `fore_lowback`
  part13 center was `[-9.47367, 9.85317, 51.0849]`; both had only `7`
  positive-y face-band vertices.
- `py_compile` passed for the modified tools and
  `tools/gh3_midori_pipeline_test.py` passed 127 tests.
- Evidence: `analysis/gh3_midori_r121_forearm_override_report.json` and latest
  proof image `analysis/gh3_midori_r121_forearm_override_probe.bmp`. Decision:
  reject r121. The solved-lower-body assumption still stands; the next useful
  branch should constrain/solve the actual rendered `midori_1_mesh0_part13.mesh`
  and upper-arm path, or bridge GLB/source hierarchy into MILO with explicit
  bone-name/hierarchy validation before capture.

### 2026-08-21 r122 source-position unit-scale diagnosis and rejection

- Lower body/root remains solved and regression-only. Runtime capture used loose
  `gh2_ps2_hybrid_assets/GEN` plus `gh2_ps2_hybrid_assets/DLC`, no ISO, and
  child work was kept at low priority.
- Added `tools/gh3_midori_visible_chain_bridge_report.py` to compare retained
  source GLB/pose arm chains against the GH2 visible `.mesh` arm chains. The
  attack-frame left source-to-visible segment ratios averaged `0.025224`, which
  matches the reciprocal (`0.02535`) of the known GH3 skeleton-to-GH2 scale
  `39.44783306581059`. This explains why r119-r121 source-position branches had
  tiny source offsets, huge runtime helper fit (`76.347234`), and unstable
  forearm behavior.
- Added `--source-position-unit-scale` to
  `tools/gh3_midori_synthesize_visible_arm_contact_overlays.py`. The scale is
  applied to mapped source `.mesh` positions and source hand IK helper positions
  before `fit-runtime-hand-targets` or `anchored-runtime-hand-targets`. Added a
  pipeline test proving both paths scale. `py_compile` passed and
  `tools/gh3_midori_pipeline_test.py` passed 128 tests.
- Synth result with scale `39.44783306581059` and
  `anchored-runtime-hand-targets`: raw/used fit scale became `1.935397` for the
  attack frame instead of the old `76.347234`, and all five synth cases had
  exact left/right hand contact. Built a scratch candidate from the merged r122
  ACP overlay and captured only the attack frame.
- Direct visual still rejects. The unit-scale fix moves the visible arm with
  correct magnitude, but the fretting arm now travels up through the head/face
  instead of dangling behind the guitar. Runtime part13 center was
  `[6.25721, 4.3473, 54.3231]`, selected face-band weight `110.5`, and
  positive-y face-band vertices `103`.
- Evidence: `analysis/gh3_midori_r122_visible_chain_bridge_report.json`,
  `analysis/gh3_midori_r122_scaled_source_position_report.json`, and latest
  proof image `analysis/gh3_midori_r122_scaled_source_position_probe.bmp`.
  Decision: keep the unit-scale fix and reject the visual. Next branch should
  start from scaled anchored source positioning and add an explicit visible-chain
  bend plane or head/face exclusion constraint for `midori_1_mesh0_part13.mesh`;
  do not reopen pelvis/Control_Root and do not go back to unscaled source
  positions.

### 2026-08-21 r123 directed bend-plane probe

- Lower body/root remains solved and regression-only. Runtime captures used
  loose `gh2_ps2_hybrid_assets/GEN` plus `gh2_ps2_hybrid_assets/DLC`, no ISO,
  and child work was kept at low priority.
- No source edits were made in r123. Used the existing r122 controls:
  `anchored-runtime-hand-targets`, `--source-position-unit-scale
  39.44783306581059`, and explicit `--left-source-forearm-guitar-local`
  overrides. The r122 base left forearm target
  `[1.9907, -16.0899, 17.7256]` mapped to world `[1.5018, 5.5807, 58.0163]`,
  causing the head/face collision.
- Report-only override variants all preserved exact hand contact:
  `blend_mid`, `chest_low`, `neck_back`, and `lowback_old`. Captured
  `chest_low` and `lowback_old`. `chest_low` (`[-3.5, -6.0, 18.7]`) removed
  the part13 face-band problem (`part13` center `[-1.30219, 6.02509, 48.5576]`,
  positive-y face vertices `0`) but the sleeve/upper-arm silhouette still read
  poorly. `lowback_old` (`[-5.1, -2.2, 18.4]`) was the best r123 visual
  silhouette and is retained as latest proof, but still rejects because the
  fretting hand/wrist is visibly detached/curled at the neck.
- Evidence: `analysis/gh3_midori_r123_bend_plane_report.json` and
  `analysis/gh3_midori_r123_bend_plane_probe.bmp`. Decision: reject r123, but
  keep the r122 unit scale and low/back forearm plane. Next branch should
  target visible `bone_L-hand.mesh` orientation/finger pose at the neck while
  preserving the low/back forearm bend plane; do not repeat broad forearm-plane
  sweeps without a wrist/hand orientation change.

### 2026-08-21 r124 pelvis-through-face promotion

- Priority order reset per user direction: pelvis/root through head/eyes/mouth
  first; arms, hands, fingers, and guitar attachment remain deferred until the
  pelvis-to-face chain is accepted. No ISO was mounted or used; captures used
  loose `gh2_ps2_hybrid_assets/gen` plus `gh2_ps2_hybrid_assets/DLC`, and
  capture/build work was run at low/idle priority.
- Diagnosed the face-layer issue: broad checksum fallback filtering made
  expression clips load, but source-space absolute face rotations tore the face.
  Added face-expression delta staging for the seven hashed Midori face targets,
  then added a `milo_convert_tool replace-clipset-clips` path so only those
  expression `CharClipSamples` are replaced in the already-solved promoted main
  MILO. This preserves the 338-entry / 64-output-bone body contract instead of
  rebuilding the body from drifted ACP stage data.
- Promoted the surgical main MILO to `analysis/gh3_midori_gh2_milos` and both
  package mirrors. Current main hash is
  `4F70FBFE6147FCDEE84AD5F1B7BFE7A8C2849772B5AB2271E19ACFF7B79E182F`.
  Running `gh3_midori_dlc_package.py` also refreshed fret/strum from the
  generated-source tree; the current package hashes are
  `384A2FD66EAA4C232F060A20483627BC398A0CF5B260A44015AFE4B24E46BA02` for fret
  and `89BB5233BEEA2CC29022E421478EE64938943027BBB5FCD457D5B85E31A9A532` for
  strum. The prior loose-DLC fret/strum payloads
  (`329E306F261646C709691313D50098DC243E1DC01E7F3A3F9F9A0CDE88692090` and
  `C6F837D5779C8BCA70E0EACFB3672D5976C393589FDB5F4B9BBD4D01DD1E2C16`) were not
  found elsewhere in the workspace after the refresh; arms/hands/guitar remain
  unapproved/deferred despite this package normalization.
- Live current-DLC proof passed baseline plus all seven face expressions
  (`default`, `happy`, `idle`, `kiss`, `pout`, `satisfied`, `yeah`) with zero
  automated failures. Logs show the body layer retained at 30 channels / 64
  output bones and each face layer filtered to seven output face channels.
  Evidence retained at
  `analysis/gh3_midori_pelvis_face_runtime_proofs_current_dlc/manifest.json`
  and
  `analysis/gh3_midori_pelvis_face_runtime_proofs_current_dlc/current_dlc_all_face_head_crops.png`.
- Visual read: pelvis/root through head remains coherent and bipedal; eyes stay
  seated and mouth expression deltas stay attached with no facial shards. This
  is the current accepted-by-agent pelvis-through-face candidate pending direct
  user approval. Do not start arms/guitar work until the user accepts this slice.

### 2026-08-21 r125 current-DLC pelvis-through-face reinforcement

- Re-ran proof from the live loose DLC package after promotion/package refresh,
  again using `gh2_ps2_hybrid_assets/gen` plus `gh2_ps2_hybrid_assets/DLC`, no
  ISO, and low/idle priority capture work.
- Five representative body/head cases passed with zero automated failures:
  `midori_1_medium_idle_f060`, `midori_1_attack_left_f030`,
  `midori_1_fast_jump_f040`, `midori_1_fast_solo_f090`, and
  `midori_1_transition_out_f050`. Evidence:
  `analysis/gh3_midori_pelvis_head_pose_review_proofs_current_dlc_r125/manifest.json`
  and
  `analysis/gh3_midori_pelvis_head_pose_review_proofs_current_dlc_r125/current_dlc_pelvis_head_pose_contact_sheet.png`.
- Added stable current-DLC face cases at
  `analysis/gh3_midori_pelvis_face_runtime_cases_current_dlc.json`. Re-ran
  baseline plus all seven expression overlays; all eight captures passed with
  zero automated failures. Evidence:
  `analysis/gh3_midori_face_expression_runtime_proofs_current_dlc_r125/manifest.json`
  and
  `analysis/gh3_midori_face_expression_runtime_proofs_current_dlc_r125/current_dlc_r125_all_face_head_crops.png`.
- Updated `tools/gh3_midori_pelvis_face_coverage_report.py` so face-expression
  coverage requires the seven hashed face `.quat` channels and reports `.pos`
  channels as optional diagnostics. Current structural report now shows
  `live_body_status=covered` and `live_face_expression_status=covered`.
- Current package hashes remain main
  `4F70FBFE6147FCDEE84AD5F1B7BFE7A8C2849772B5AB2271E19ACFF7B79E182F`, fret
  `384A2FD66EAA4C232F060A20483627BC398A0CF5B260A44015AFE4B24E46BA02`, and
  strum `89BB5233BEEA2CC29022E421478EE64938943027BBB5FCD457D5B85E31A9A532`.
  Agent visual read remains: pelvis/root through head/eyes/mouth is coherent
  and ready for user approval; do not move to arms/hands/fingers/guitar
  attachment until that approval is given.
- Added approval packet
  `analysis/gh3_midori_pelvis_face_approval_packet_current_dlc.json`. It binds
  the current package hashes, body/head proof, face-expression proof, and
  coverage report into one `awaiting_user_visual_approval` artifact and
  explicitly lists arms/hands/fingers/guitar attachment as deferred.
- Added reusable validator
  `tools/gh3_midori_validate_pelvis_face_approval.py`. Current command
  `python -B tools/gh3_midori_validate_pelvis_face_approval.py` reports
  `status=passed ... errors=0`. Use this to recheck hashes, proof paths,
  zero-failure manifests, covered/covered structural status, no-ISO evidence,
  and the deferred arms/hands/fingers/guitar boundary before acting on approval.
- Added combined visual approval sheet
  `analysis/gh3_midori_pelvis_face_current_dlc_combined_approval.png` and
  linked it from both the approval packet and
  `analysis/gh3_midori_pelvis_face_visual_approval_note.md`. The validator now
  checks packet `image` fields as proof paths.

### 2026-08-21 r126 user rejection follow-up

- User rejected r125: some head/neck frames were visually wrong, and the face
  expressions were visually inert. The r125 approval packet/validator should be
  treated as superseded, not as approval evidence.
- Diagnosed the head issue in the live main MILO: rejected jump/solo/transition
  frames had extreme `bone_head.mesh.quat` values while neck/spine samples were
  otherwise coherent. Added a staging guard in
  `tools/gh3_midori_acp_stage.py` so guitar-main `Bone_Head`/`bone_head(.mesh)`
  rotations are not staged, matching the existing head translation suppression.
- Added `milo_convert_tool strip-clip-channels` and used it surgically on the
  trusted live main to remove only `bone_head.mesh.quat`/`bone_head.quat` from
  five rejected body clips:
  `gh3_guit_mido_a_med_idle01`, `gh3_guit_mido_a_attackl`,
  `gh3_guit_mido_a_fst_jump01`, `gh3_guit_mido_a_fst_solo01`, and
  `gh3_guit_midori_tran_atoout`. A full restaged five-clip donor was explicitly
  rejected because its neck/spine samples differed from the trusted live main.
- Diagnosed the face issue: model transforms and mesh weights for the seven
  `bone_gh3_*` face targets exist, but r125 face clips emitted only identity or
  near-identity quats. Added a face-only relative rotation gain
  `MIDORI_FACE_EXPRESSION_ROTATION_GAIN = 16.0` after frame-0-relative
  expression delta calculation, then surgically replaced only the seven Midori
  face expression clips.
- Promoted the r126 candidate main only; fret/strum/UI/model MILOs were left
  unchanged. Current main hash is
  `DBC6A5CF71090D336B51D314E995F8C699422BDDCB11D9DF60A51A7B6656AF55`,
  size `20243935`, in both `analysis/gh3_midori_gh2_milos` and loose DLC.
- Evidence retained: body sheet
  `analysis/gh3_midori_headstrip_body_contact_sheet_r126.png`; face sheets
  `analysis/gh3_midori_headstrip_facegain_face_contact_sheet_r126.png` and
  `analysis/gh3_midori_headstrip_facegain_face_close_contact_sheet_r126.png`;
  runtime face report
  `analysis/gh3_midori_headstrip_facegain_runtime_report_r126.json` passed with
  `failures=0`. The body proof JSON
  `analysis/gh3_midori_headstrip_pose_review_r126.png` has one framing-margin
  failure on the second-outfit attack only; the five user-rejected first-outfit
  head/body frames visually show the head seated after stripping head rotation.
- Still awaiting direct user visual approval. Do not proceed to
  arms/hands/fingers/guitar attachment until pelvis through face is accepted.

### 2026-08-21 r127 live pelvis-through-face candidate

- Fresh live-DLC recapture of r126 showed the previous head detachments were
  gone, but jump/solo/transition still read with an over-strong downward head
  pitch. A temporary r127 probe stripped `bone_neck.mesh.quat` only from
  `gh3_guit_mido_a_fst_jump01`, `gh3_guit_mido_a_fst_solo01`, and
  `gh3_guit_midori_tran_atoout` while keeping the earlier
  `bone_head.mesh.quat` strip on the five reviewed body clips. This visually
  improved the rejected frames and passed the five-case body review.
- Promoted r127 main to all three mirrors:
  `analysis/gh3_midori_gh2_milos/gh3_midori_main.milo_ps2`,
  `gh2_ps2_hybrid_assets/DLC/community.gh3.midori/.../gh3_midori_main.milo_ps2`,
  and
  `GuitarHeroOGX-main-ui-engine/DLC/community.gh3.midori/.../gh3_midori_main.milo_ps2`.
  Current main hash is
  `077AE6973D3DE99B8CC3009C6825755B19F399DD92AFD52F303EA8F6E5B5697C`, size
  `20236607`.
- Direct live body proof after promotion passed with five proofs, zero
  failures, min margin `16`. Direct live face proof passed with eight proofs,
  zero failures, min margin `26`. Retained compact evidence:
  `analysis/gh3_midori_r127_live_body_contact_sheet.png`,
  `analysis/gh3_midori_r127_live_face_close_contact_sheet.png`,
  `analysis/gh3_midori_r127_live_pelvis_face_combined_approval.png`,
  `analysis/gh3_midori_r127_body_report.json`, and
  `analysis/gh3_midori_r127_live_face_report.json`.
- Updated `analysis/gh3_midori_pelvis_face_approval_packet_current_dlc.json`
  and `analysis/gh3_midori_pelvis_face_visual_approval_note.md` to r127.
  Updated `tools/gh3_midori_validate_pelvis_face_approval.py` so stale r125
  proof paths are rejected and the intentionally partial body coverage is
  accepted only when the packet records the head/neck stabilization strip.
  Current validator command
  `python -B tools/gh3_midori_validate_pelvis_face_approval.py --json` passes.
- Tried an r128 face-position gain probe (`32x` relative face translation) to
  make expression motion more obvious. It passed automation but looked nearly
  the same in the actual renderer, so it was not promoted and the probe-only
  source edit was removed. The remaining face-expression limitation is modest
  visible motion, not missing face channels. Direct user visual approval is
  still required before arms/hands/fingers/guitar attachment.

### 2026-08-21 face-control matrix pivot

- User clarified that GH2 does not have the full Neversoft facial-control
  surface; the goal is controllable face output through a GH2-style translation
  matrix, not exaggerated GH3/WorldTour expression parity.
- Added matrix artifact
  `analysis/gh3_midori_gh2_face_control_matrix.json`. It maps the current GH2
  bridge calls (`Neutral`, `expressionBad1..3`, `expressionGood1..5`,
  `EyesClosed`, `ref/open`, `singer_face_open/close`) and GH2 viseme controls
  onto Midori's preserved hashed controls.
- Key finding: r127 expression clips do drive the seven hashed Midori face
  deformers (`bone_gh3_e8e9bb36`, `bone_gh3_12e68655`,
  `bone_gh3_2a8d0c00`, `bone_gh3_7c73f6cf`, `bone_gh3_867ccbac`,
  `bone_gh3_5378af83`, `bone_gh3_a97792e0`) when sampled directly from the
  live main MILO. The default/reset sample also touches separate hashed
  jaw/eye controls (`bone_gh3_b8ca856b`, `bone_gh3_c8a071e4`,
  `bone_gh3_a6bc7033`), but those are not yet wired as expression targets.
- The next useful face step is matrix-driven wiring/validation, especially
  `bone_jaw.quat` / singer open-ref behavior and the `EyesClosed` gap. Do not
  return to broad visual grinding or guitar attachment until pelvis through
  head/face control is accepted.

### 2026-08-21 executable face-control matrix gate

- Added `tools/gh3_midori_validate_face_control_matrix.py`, which loads
  `analysis/gh3_midori_gh2_face_control_matrix.json`, verifies the r127 live
  main MILO hash/size, checks every non-null `call_to_clip_matrix` target exists
  in the live clipset, samples the mapped clips at bounded representative frames,
  and confirms the expected Midori hashed controls are active. The validator
  runs child `milo_convert_tool` processes at below-normal priority.
- Generated retained compact report
  `analysis/gh3_midori_face_control_matrix_validation_report.json`; latest run
  passed with seven live clips sampled and zero errors/warnings.
- Registered the validator in `tools/gh3_midori_build_pipeline.py` as a source
  anchor and as a pipeline gate after `glb_milo_route_gate`. Added the report to
  the printed summary list.
- Updated `tools/gh3_midori_completion_audit.py` so completion now requires the
  GH2-call face-control matrix and its passing validation report. The matrix
  audit item is proven; overall completion still remains failed/pending because
  old visual precheck/promoted triage/user visual approval gates are not accepted.

### 2026-08-21 r129 face-call alias candidate

- Added `milo_convert_tool alias-clipset-clips`, which duplicates existing
  `CharClipSamples` entries under new object-table names and appends matching
  `CharClipSet` root summaries. It round-trip verifies the payload/container.
- Built unpromoted candidate
  `analysis/gh3_midori_gh2_milos/gh3_midori_main_facealiases_r129.milo_ps2`
  from r127 with 13 non-gap GH2 face-call aliases:
  `gh2_face_call_Neutral`, `gh2_face_call_expressionBad1..3`,
  `gh2_face_call_expressionGood1..5`, `gh2_face_call_ref`,
  `gh2_face_call_open`, `gh2_face_call_singer_face_open`, and
  `gh2_face_call_singer_face_close`. Candidate hash
  `0C3EC9796C1309B433067B56654C3A82D9F07FC0775A894BF4C9B4D94FCDC539`, size
  `20361021`.
- Added alias manifest
  `analysis/gh3_midori_face_call_alias_candidate_r129.json` and validator
  `tools/gh3_midori_validate_face_call_aliases.py`. Latest run passed with
  13 aliases, one explicit `EyesClosed` gap, and zero errors. The validator
  compares source and alias active face-control signatures by sampling the
  candidate MILO.
- Updated `analysis/gh3_midori_gh2_face_control_matrix.json` to point at the
  r129 alias candidate while keeping live r127 hashes authoritative. Do not
  promote r129 over live DLC until it has an alias-only recapture/approval path
  or the user accepts the alias-only change. The next face work item remains
  isolating real jaw/open-close and blink/lid control rather than hiding
  `EyesClosed` as a no-op.
- Follow-up: `tools/gh3_midori_validate_face_control_matrix.py` now validates
  the linked r129 alias candidate metadata/report/hash in addition to live r127
  face clips. `tools/gh3_midori_build_pipeline.py` now runs both
  `face_control_matrix_validator` and `face_call_alias_validator`, and includes
  the alias validation report in its summary list. Latest targeted runs:
  matrix validator passed with alias candidate status `candidate_not_promoted`,
  alias validator passed with 13 aliases and one explicit gap, pelvis/face
  approval validator passed. Completion audit remains failed/pending only for
  the existing visual precheck/promoted triage/user approval gates.

### 2026-08-21 jaw/open-close diagnosis

- Added `tools/gh3_midori_face_jaw_open_diagnosis.py` and retained
  `analysis/gh3_midori_face_jaw_open_diagnosis_r129.json`. It consumes the
  r129 alias validation report and asks one narrow question: do GH2 open/ref
  calls reach the Midori jaw hash `bone_gh3_b8ca856b`?
- Latest diagnosis status is `jaw_open_gap_confirmed`: `Neutral`, `ref`, and
  `singer_face_close` all hit the jaw/reset controls, but `open` and
  `singer_face_open` only hit the seven mouth deformers and do not hit
  `bone_gh3_b8ca856b`. `EyesClosed` remains an explicit gap.
- Updated `analysis/gh3_midori_gh2_face_control_matrix.json` so the r129 alias
  candidate records `jaw_open_status: jaw_open_gap_confirmed`. Updated
  `tools/gh3_midori_validate_face_control_matrix.py` so the matrix gate checks
  the linked jaw/open diagnosis report as well as the alias report/hash.
- Corrected `tools/gh3_midori_build_pipeline.py` face-gate order to refresh
  alias validation, then jaw/open diagnosis, then matrix validation. Updated
  `tools/gh3_midori_completion_audit.py` detail/evidence so completion reports
  the open-call jaw gap instead of implying the face matrix is final.
- Built unpromoted r130 jaw-carrier candidate
  `analysis/gh3_midori_gh2_milos/gh3_midori_main_facealiases_jawcarrier_r130.milo_ps2`
  from r129 by generating a tiny automated ACP donor and replacing only
  `gh2_face_call_open` and `gh2_face_call_singer_face_open`. Candidate hash
  `91F9EEFB9CD62950AA85A972E9C7A18443A0CC768AB586F7F34912C6DD653B56`, size
  `20369689`.
- Added `tools/gh3_midori_face_open_jaw_candidate.py` and
  `tools/gh3_midori_validate_face_open_jaw_candidate.py`; retained compact
  manifest/report `analysis/gh3_midori_face_open_jaw_candidate_r130.json` and
  `analysis/gh3_midori_face_open_jaw_validation_report_r130.json`. Latest r130
  validation passed with two open calls, zero errors/warnings. Both open calls
  preserve the seven Midori mouth deformers and now expose
  `bone_gh3_b8ca856b`.
- Important limitation: r130 jaw motion status is
  `jaw_control_carrier_present_reset_only`. This proves GH2 open-call plumbing
  can address the Midori jaw hash; it is not proof of a visible jaw-open solve.
  r129 diagnosis remains `jaw_open_gap_confirmed`, and `EyesClosed` remains an
  explicit unsupported gap.
- Updated the face matrix, matrix validator, build pipeline, and completion
  audit so r130 is tracked separately from the live r127 candidate and the r129
  alias baseline. Latest targeted runs passed: r129 alias validator, r129
  jaw/open diagnosis, r130 jaw-carrier validator, face-control matrix validator,
  and pelvis/face approval validator. Completion audit still fails only at the
  existing visual precheck/promoted triage/user approval layer.
- Follow-up source audit found only `gh3_guitarist_midori_acc01`,
  `gh3_guitarist_midori_acc02`, and `gh3_guitarist_midori_default` carry the
  jaw/eye hashes in the staged ACPs. `default` is reset-only; `acc02` has the
  larger non-reset jaw quaternion delta and was selected for a source-derived
  probe.
- Built unpromoted r131 jaw-delta candidate
  `analysis/gh3_midori_gh2_milos/gh3_midori_main_facealiases_jawdelta_r131.milo_ps2`
  from r129 by generating an automated ACP donor that combines `yeah` mouth
  deformer motion with time-stretched `acc02` jaw quaternion motion for
  `gh2_face_call_open` and `gh2_face_call_singer_face_open`. Candidate hash
  `DC93FC5593A246100BF44EBF80BE775E99BC07E9D94357ACB16C34A4E57D62F6`, size
  `20378639`.
- Added `tools/gh3_midori_face_open_jaw_delta_candidate.py` and
  `tools/gh3_midori_validate_face_open_jaw_delta_candidate.py`; retained
  `analysis/gh3_midori_face_open_jaw_delta_candidate_r131.json` and
  `analysis/gh3_midori_face_open_jaw_delta_validation_report_r131.json`.
  Latest validation passed with two open calls, zero errors/warnings,
  source-derived jaw status, and sampled max jaw angle `7.179979` degrees from
  reset (`8.446331` degrees in the generated source sweep).
- Updated the face matrix, matrix validator, build pipeline, and completion
  audit so r131 is tracked separately from r129 and r130. Latest targeted runs
  passed: r131 jaw-delta validator, face-control matrix validator, and
  pelvis/face approval validator. Completion audit still fails only at the
  existing visual precheck/promoted triage/user approval layer.
- Captured r131 through the native viewer by temporarily deploying the r131 main
  MILO into the loose DLC tree with backup/restore. The wrapper restored both
  loose-DLC mirrors to live r127 afterward; verified main hashes:
  `077AE6973D3DE99B8CC3009C6825755B19F399DD92AFD52F303EA8F6E5B5697C`.
  The first r131 visual sheet advanced the main idle frame and the face frame
  together, so it was useful for bipedal/coherence but not isolated face-only
  proof; that superseded first packet was deleted during cleanup.
- Added paired same-body-frame comparison cases and retained
  `analysis/gh3_midori_r131_face_open_compare_report.json`,
  `analysis/gh3_midori_r131_face_open_compare_contact_sheet.jpg`,
  `analysis/gh3_midori_r131_face_open_compare_face_crop_sheet.jpg`, and
  `analysis/gh3_midori_r131_face_open_compare_diff_report.json`. The paired
  capture passed native viewer checks with nine proofs, zero failures, min
  margin `23`; default/open same-frame pixel deltas were measurable but small
  (`0.00085178`, `0.00084744`, `0.00085612`, `0.00099935` for frames
  60/120/180/240).
- Added `tools/gh3_midori_face_crop_sheet.py`. Visual read from the paired crop
  sheet: r131 remains bipedal/coherent and the big head orientation changes are
  from the shared main body frame, not the face layer. The face/jaw motion is
  subtle and still requires user/direct visual approval before promotion.
- Next implementation target: use the r131 paired face crop sheet for user
  review or build a stronger-but-bounded jaw delta probe if the subtle motion is
  rejected. `EyesClosed` remains an explicit unsupported gap.
