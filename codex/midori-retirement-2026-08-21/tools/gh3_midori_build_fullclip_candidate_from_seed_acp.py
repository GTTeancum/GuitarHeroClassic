#!/usr/bin/env python3
"""Build a full-clip candidate by overwriting patch channels in seed ACPs."""

from __future__ import annotations

import argparse
import ctypes
import json
import shutil
from pathlib import Path
from typing import Any

import gh3_midori_build_fullclip_coupled_contact_candidate as fullclip
import gh3_midori_guitar_ik_contract_report as contract
import gh3_midori_patch_acp_constant_channel as acp_patch


def set_low_priority() -> None:
    try:
        ctypes.windll.kernel32.SetPriorityClass(
            ctypes.windll.kernel32.GetCurrentProcess(),
            fullclip.child_creationflags(),
        )
    except Exception:
        pass


def load_json(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8-sig") as handle:
        return json.load(handle)


def rel(path: Path) -> str:
    return fullclip.rel(path)


def build(args: argparse.Namespace) -> dict[str, Any]:
    if args.acp_dir.exists():
        shutil.rmtree(args.acp_dir)
    shutil.copytree(args.seed_acp_dir, args.acp_dir)

    candidate = args.candidate.resolve()
    solve = load_json(args.solve_report)
    forearm_override_map = fullclip.load_forearm_override_map(args.visible_arm_forearm_override_map)
    solve_rows = list(solve.get("cases", []))
    if args.case_name:
        wanted = set(args.case_name)
        solve_rows = [row for row in solve_rows if row.get("name") in wanted]
        missing = sorted(wanted - {str(row.get("name")) for row in solve_rows})
        if missing:
            raise ValueError(f"solve row(s) not found: {', '.join(missing)}")
    solve_rows = [row for row in solve_rows if row.get("coupled_guitar_anchor_solve")]
    if not solve_rows:
        raise ValueError(f"{args.solve_report}: no coupled solve rows after filtering")
    rows = []
    clips_to_replace = []
    clip_patch_signatures: dict[str, dict[str, Any]] = {}
    for solve_row in solve_rows:
        patch_values = fullclip.target_proxy_locals_after_guitar(
            args.tool,
            candidate,
            solve_row,
            args.hmx_quat_mode,
            args.sample_quat_mode,
            args.viewer_prop_overrides,
            args.solve_visible_arms,
            args.arm_chain_reach_scale,
            args.solve_visible_hand_rotations,
            args.solve_visible_clavicles,
            args.visible_clavicle_blend,
            args.visible_arm_rotation_mode,
            args.visible_arm_target_mode,
            args.visible_arm_target_blend_with_current,
            set(args.visible_arm_target_swap_case),
            args.visible_arm_grip_map,
            args.visible_arm_source_coordinate_map,
            args.visible_arm_elbow_hint_mode,
            args.visible_arm_elbow_side_offset,
            args.visible_arm_elbow_down_offset,
            fullclip.case_forearm_override_arg(
                forearm_override_map,
                solve_row["name"],
                "L",
                args.visible_arm_left_forearm_guitar_local,
            ),
            fullclip.case_forearm_override_arg(
                forearm_override_map,
                solve_row["name"],
                "R",
                args.visible_arm_right_forearm_guitar_local,
            ),
            args.visible_arm_source_rotation_blend_with_aim,
            args.visible_hand_rotation_mode,
            args.visible_hand_axis_calibration,
            args.left_hand_grip_guitar_local_axis,
            args.left_hand_grip_guitar_local_degrees,
            args.right_hand_grip_guitar_local_axis,
            args.right_hand_grip_guitar_local_degrees,
            args.source_bridge,
            args.source_frame,
            args.source_basis,
            args.source_scale,
            args.per_case_source_bridge_root,
        )
        arm_solve = patch_values.pop("_arm_solve", None)
        if args.skip_contact_patches:
            for channel in fullclip.PATCH_CHANNELS:
                patch_values.pop(channel, None)
        clip_name = str(solve_row["main_clip"])
        signature = fullclip.patch_signature(patch_values)
        previous_signature = clip_patch_signatures.get(clip_name)
        if previous_signature is not None and previous_signature["signature"] != signature:
            if args.duplicate_clip_policy == "error":
                raise ValueError(
                    f"{clip_name}: duplicate clip patch conflict between "
                    f"{previous_signature['case']} and {solve_row['name']}; "
                    "use a shared clip map or pass --duplicate-clip-policy "
                    "first/last for an explicit diagnostic overwrite"
                )
            if args.duplicate_clip_policy == "first":
                rows.append(
                    {
                        "case": solve_row["name"],
                        "clip": clip_name,
                        "duplicate_clip_skipped": True,
                        "duplicate_clip_policy": args.duplicate_clip_policy,
                        "previous_case": previous_signature["case"],
                    }
                )
                continue
        else:
            clip_patch_signatures[clip_name] = {
                "case": solve_row["name"],
                "signature": signature,
            }
        acp = args.acp_dir / f"{clip_name}.acp"
        if not acp.is_file():
            raise FileNotFoundError(str(acp))
        patched_channels = []
        for channel, values in patch_values.items():
            acp_patch.patch_file(
                acp,
                channel,
                [float(value) for value in values],
                allow_add=args.allow_add_channels,
            )
            patched_channels.append(channel)
        clips_to_replace.append(clip_name)
        rows.append(
            {
                "case": solve_row["name"],
                "clip": clip_name,
                "patched_channels": sorted(patched_channels),
                "arm_solve": arm_solve,
                "patch_values": patch_values,
            }
        )

    fullclip.run_tool(
        args.tool,
        [
            "build-clipset-from-acp",
            str(args.acp_dir),
            "--name",
            args.donor_name,
            "--role",
            "guitar-main",
            "--move-self",
            "0",
            "--out",
            str(args.donor),
        ],
    )
    main_milo = contract.candidate_milo(candidate, "gh3_midori_main.milo_ps2")
    replace_args = [
        "replace-clipset-clips",
        str(main_milo),
        "--donor",
        str(args.donor),
    ]
    for clip_name in sorted(set(clips_to_replace)):
        replace_args.extend(["--clip", clip_name])
    replace_args.extend(["--out", str(args.output)])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fullclip.run_tool(args.tool, replace_args)

    report = {
        "format": "gh3_midori_fullclip_seed_acp_candidate_v1",
        "status": "candidate_built",
        "revision": args.revision,
        "candidate": rel(candidate),
        "seed_acp_dir": rel(args.seed_acp_dir),
        "acp_dir": rel(args.acp_dir),
        "solve_report": rel(args.solve_report),
        "output_main_milo": {
            "path": rel(args.output),
            "size": args.output.stat().st_size,
            "sha256": fullclip.sha256_file(args.output),
        },
        "donor_milo": rel(args.donor),
        "case_filter": args.case_name,
        "skip_contact_patches": args.skip_contact_patches,
        "duplicate_clip_policy": args.duplicate_clip_policy,
        "visible_arm_forearm_override_map": (
            rel(args.visible_arm_forearm_override_map)
            if args.visible_arm_forearm_override_map
            else None
        ),
        "rows": rows,
    }
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report


def main() -> int:
    set_low_priority()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tool", type=Path, default=fullclip.DEFAULT_TOOL)
    parser.add_argument("--candidate", type=Path, default=fullclip.DEFAULT_CANDIDATE)
    parser.add_argument("--solve-report", type=Path, required=True)
    parser.add_argument("--seed-acp-dir", type=Path, required=True)
    parser.add_argument("--acp-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--donor", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--revision", required=True)
    parser.add_argument("--donor-name", default="gh3_midori_seed_acp_donor")
    parser.add_argument("--hmx-quat-mode", choices=("direct", "transpose"), default="transpose")
    parser.add_argument("--sample-quat-mode", choices=("direct", "hmx"), default="hmx")
    parser.add_argument("--viewer-prop-overrides", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--solve-visible-arms", action="store_true")
    parser.add_argument("--arm-chain-reach-scale", type=float, default=0.94)
    parser.add_argument("--solve-visible-hand-rotations", action="store_true")
    parser.add_argument("--solve-visible-clavicles", action="store_true")
    parser.add_argument("--visible-clavicle-blend", type=float, default=1.0)
    parser.add_argument(
        "--skip-contact-patches",
        action="store_true",
        help="Leave guitar/fret/strum contact channels unchanged; useful for arm-only diagnostics.",
    )
    parser.add_argument(
        "--visible-arm-rotation-mode",
        choices=("aim", "source", "source-per-case", "source-pose-per-case"),
        default="aim",
    )
    parser.add_argument(
        "--visible-arm-target-mode",
        choices=(
            "current-proxies",
            "source-palm-locals-per-case",
            "source-ik-helper-locals-per-case",
            "swapped-current-proxies",
            "mapped-current-proxies",
            "explicit-guitar-local-grip",
            "source-palm-fit-stock-hand-targets-per-case",
        ),
        default="current-proxies",
    )
    parser.add_argument("--visible-arm-target-blend-with-current", type=float, default=1.0)
    parser.add_argument("--visible-arm-target-swap-case", action="append", default=[])
    parser.add_argument("--visible-arm-grip-map", type=Path, default=fullclip.ROOT / "analysis" / "gh3_midori_explicit_guitar_grip_map_r164.json")
    parser.add_argument(
        "--visible-arm-source-coordinate-map",
        choices=("palm-fit-negx-y-negz", "direct"),
        default="palm-fit-negx-y-negz",
    )
    parser.add_argument(
        "--visible-arm-elbow-hint-mode",
        choices=("current", "down-out", "source-pose-per-case"),
        default="current",
    )
    parser.add_argument("--visible-arm-elbow-side-offset", type=float, default=5.0)
    parser.add_argument("--visible-arm-elbow-down-offset", type=float, default=8.0)
    parser.add_argument(
        "--visible-arm-left-forearm-guitar-local",
        help="Opt-in visible left forearm/elbow target in solved guitar-local space.",
    )
    parser.add_argument(
        "--visible-arm-right-forearm-guitar-local",
        help="Opt-in visible right forearm/elbow target in solved guitar-local space.",
    )
    parser.add_argument(
        "--visible-arm-forearm-override-map",
        type=Path,
        help="Optional JSON case map overriding visible forearm guitar-local targets per diagnostic case.",
    )
    parser.add_argument("--visible-arm-source-rotation-blend-with-aim", type=float, default=1.0)
    parser.add_argument(
        "--case-name",
        action="append",
        default=[],
        help="Limit the build to one diagnostic solve row. May be repeated.",
    )
    parser.add_argument(
        "--visible-hand-rotation-mode",
        choices=("target-proxy", "visible-axis-calibration"),
        default="target-proxy",
    )
    parser.add_argument("--visible-hand-axis-calibration", type=Path, default=fullclip.DEFAULT_VISIBLE_HAND_AXIS_CALIBRATION)
    parser.add_argument("--left-hand-grip-guitar-local-axis")
    parser.add_argument("--left-hand-grip-guitar-local-degrees", type=float, default=0.0)
    parser.add_argument("--right-hand-grip-guitar-local-axis")
    parser.add_argument("--right-hand-grip-guitar-local-degrees", type=float, default=0.0)
    parser.add_argument("--source-bridge", type=Path, default=fullclip.probe.DEFAULT_SOURCE_BRIDGE)
    parser.add_argument("--per-case-source-bridge-root", type=Path, default=fullclip.DEFAULT_PER_CASE_SOURCE_BRIDGE_ROOT)
    parser.add_argument("--source-frame", type=int, default=30)
    parser.add_argument("--source-basis", choices=("direct", "anim", "helper"), default="direct")
    parser.add_argument("--source-scale", type=float, default=fullclip.probe.GH3_PS2_SKELETON_TO_GH2_SCALE)
    parser.add_argument("--allow-add-channels", action="store_true")
    parser.add_argument(
        "--duplicate-clip-policy",
        choices=("error", "first", "last"),
        default="error",
        help="How to handle multiple solve rows that patch the same main clip.",
    )
    parser.add_argument("--print-summary", action="store_true")
    args = parser.parse_args()
    report = build(args)
    if args.print_summary:
        output = report["output_main_milo"]
        print(
            "status=%s rows=%d bytes=%d sha256=%s"
            % (report["status"], len(report["rows"]), output["size"], output["sha256"])
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
