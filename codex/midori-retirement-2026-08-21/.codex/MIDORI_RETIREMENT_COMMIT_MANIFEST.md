# Midori Retirement Commit Manifest

The workspace root `C:\Programming\GitHub\Guitar Hero II` could not be
committed on 2026-08-21 because `.git` is an empty directory with no `HEAD` or
`config`. `git status --short` at the root fails with:

```text
fatal: not a git repository (or any of the parent directories): .git
```

When repository metadata or a remote is restored, commit/push the retirement
findings and tooling changes below.

## State / handoff

- `.codex/MIDORI_HIERARCHY_HANDOFF.md`
- `.codex/CURRENT_STATE.md`
- `.codex/MIDORI_RETIREMENT_COMMIT_MANIFEST.md`

## Tooling changes

- `tools/gh3_midori_pose_review.py`
- `tools/gh3_midori_capture_with_loose_dlc_backup.py`
- `tools/gh3_midori_build_fullclip_coupled_contact_candidate.py`
- `tools/gh3_midori_build_fullclip_candidate_from_seed_acp.py`

## r180-r182 findings

- `analysis/gh3_midori_r180_staticface_arm_tooling_decision.json`
- `analysis/gh3_midori_r181_outfit1_attack_cases.json`
- `analysis/gh3_midori_r181_outfit1_attack_pose_review_manifest.json`
- `analysis/gh3_midori_r181_outfit1_staticface_attack_candidate_report.json`
- `analysis/gh3_midori_r181_outfit1_staticface_attack_visual_decision.json`
- `analysis/gh3_midori_r182_outfit1_staticface_armonly_attack_candidate_report.json`
- `analysis/gh3_midori_r182_outfit1_staticface_armonly_attack_status.json`

## Retained visual / binary evidence

- `analysis/gh3_midori_gh2_milos/gh3_midori_1_main_staticface_attack_r181.milo_ps2`
- `analysis/gh3_midori_gh2_milos/gh3_midori_1_main_staticface_armonly_attack_r182.milo_ps2`
- `analysis/gh3_midori_r181_outfit1_attack_proofs/midori_1_attack_left_r181_f030.bmp`
- `analysis/gh3_midori_r181_outfit1_attack_proofs/midori_1_attack_left_r181_f030.log`
- `analysis/gh3_midori_r181_outfit1_attack_mesh_dump/midori_1_attack_left_r181_f030.jsonl`

Do not promote either r181 or r182. r181 is visually rejected. r182 is built
but not captured or approved.
