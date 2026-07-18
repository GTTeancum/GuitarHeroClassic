#!/usr/bin/env python3
"""Verify the Rockabill2 lower-body root-cause proof pair."""

from __future__ import annotations

import argparse
from pathlib import Path
import sys

from compare_charbone_output_map import parse_output_rows, read_log_text


PROXIMAL_BONES = (
    "bone_pelvis",
    "bone_L-thigh",
    "bone_L-knee",
    "bone_R-thigh",
    "bone_R-knee",
)

DISTAL_BONES = (
    "bone_L-ankle",
    "bone_L-toe",
    "bone_R-ankle",
    "bone_R-toe",
)

LOWER_BODY_BONES = PROXIMAL_BONES + DISTAL_BONES

BAD_FORWARD_Y_MIN = {
    "bone_L-ankle": 6.0,
    "bone_L-toe": 7.0,
    "bone_R-ankle": 6.0,
    "bone_R-toe": 8.0,
}

BAD_TOE_Z_MIN = {
    "bone_L-toe": 3.0,
    "bone_R-toe": 2.0,
}


def visible_output_gap(
    log_path: Path,
    *,
    character: str,
    tag: str,
    require_live: bool,
) -> tuple[dict[str, tuple[float, float, float]], float]:
    rows, visible_rows, _ = parse_output_rows(log_path, True)
    gaps: dict[str, tuple[float, float, float]] = {}
    max_abs_xyz = 0.0
    for bone in LOWER_BODY_BONES:
        row = rows.get(bone)
        if row is None:
            raise RuntimeError(f"{log_path}: missing output row {bone}")
        if row.live != require_live:
            raise RuntimeError(
                f"{log_path}: {bone} live={int(row.live)} expected={int(require_live)}"
            )
        if "outPoseW" not in row.vectors:
            raise RuntimeError(f"{log_path}: {bone} missing outPoseW")
        visible = visible_rows.get((character, tag, bone))
        if visible is None:
            raise RuntimeError(f"{log_path}: missing visible row c={character} t={tag} b={bone}")
        gap = (
            visible.world[0] - row.vectors["outPoseW"][0],
            visible.world[1] - row.vectors["outPoseW"][1],
            visible.world[2] - row.vectors["outPoseW"][2],
        )
        gaps[bone] = gap
        max_abs_xyz = max(max_abs_xyz, *(abs(component) for component in gap))
    return gaps, max_abs_xyz


def check_root_cause(args: argparse.Namespace) -> None:
    bad_text = read_log_text(args.bad_log)
    fixed_text = read_log_text(args.fixed_log)
    if "source_publisher=fenced" not in bad_text:
        raise RuntimeError(f"{args.bad_log}: missing fenced publisher marker")
    if "source_publisher=fenced" not in fixed_text:
        raise RuntimeError(f"{args.fixed_log}: missing fenced publisher marker")

    bad_gaps, bad_max = visible_output_gap(
        args.bad_log,
        character=args.character,
        tag=args.bad_tag,
        require_live=False,
    )
    fixed_gaps, fixed_max = visible_output_gap(
        args.fixed_log,
        character=args.character,
        tag=args.fixed_tag,
        require_live=True,
    )

    bad_proximal_max = max(
        abs(component)
        for bone in PROXIMAL_BONES
        for component in bad_gaps[bone]
    )
    bad_distal_max = max(
        abs(component)
        for bone in DISTAL_BONES
        for component in bad_gaps[bone]
    )
    if bad_proximal_max > args.max_bad_proximal_gap:
        raise RuntimeError(
            f"{args.bad_log}: bad proximal gap {bad_proximal_max:.6f} > "
            f"{args.max_bad_proximal_gap:.6f}"
        )
    if bad_distal_max < args.min_bad_distal_max:
        raise RuntimeError(
            f"{args.bad_log}: bad distal gap {bad_distal_max:.3f} < "
            f"{args.min_bad_distal_max:.3f}"
        )
    for bone, required in BAD_FORWARD_Y_MIN.items():
        got = bad_gaps[bone][1]
        if got < required:
            raise RuntimeError(
                f"{args.bad_log}: {bone} forward/Y gap {got:.3f} < {required:.3f}"
            )
    for bone, required in BAD_TOE_Z_MIN.items():
        got = bad_gaps[bone][2]
        if got < required:
            raise RuntimeError(
                f"{args.bad_log}: {bone} high/Z gap {got:.3f} < {required:.3f}"
            )
    if bad_max < args.min_bad_max:
        raise RuntimeError(
            f"{args.bad_log}: bad max gap {bad_max:.3f} < {args.min_bad_max:.3f}"
        )
    if fixed_max > args.max_fixed_gap:
        raise RuntimeError(
            f"{args.fixed_log}: fixed max gap {fixed_max:.6f} > {args.max_fixed_gap:.6f}"
        )
    print(
        "PASS lower_body_root_cause "
        f"bad_max_abs_xyz={bad_max:.3f} "
        f"bad_proximal_max_abs_xyz={bad_proximal_max:.4f} "
        f"bad_distal_max_abs_xyz={bad_distal_max:.3f} "
        f"fixed_max_abs_xyz={fixed_max:.3f} "
        "proximal_rows=aligned distal_rows=drifted "
        "bad_rows=driven_live0 fixed_rows=driven_live1"
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Check that the old Rockabill2 direct clip leg proof had distal "
            "forward/high drift and the source-authored lower-output bridge "
            "proof collapses that gap."
        )
    )
    parser.add_argument(
        "--bad-log",
        type=Path,
        default=Path(
            "engine/out/visual_proofs/lower_body_legw_20260715/"
            "rockabill2_stand_fast03_f070_side_legw.log"
        ),
    )
    parser.add_argument(
        "--fixed-log",
        type=Path,
        default=Path(
            "engine/out/visual_proofs/lower_body_output_bridge_20260715/"
            "rockabill2_stand_fast03_f070_side_output_bridge.log"
        ),
    )
    parser.add_argument("--character", default="rockabill2")
    parser.add_argument("--bad-tag", default="clip")
    parser.add_argument("--fixed-tag", default="lower-output")
    parser.add_argument("--min-bad-max", type=float, default=8.0)
    parser.add_argument("--max-bad-proximal-gap", type=float, default=0.001)
    parser.add_argument("--min-bad-distal-max", type=float, default=8.0)
    parser.add_argument("--max-fixed-gap", type=float, default=0.001)
    return parser.parse_args()


def main() -> int:
    try:
        check_root_cause(parse_args())
    except RuntimeError as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
