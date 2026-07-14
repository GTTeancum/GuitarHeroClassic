#!/usr/bin/env python3
"""Compare filtered arm-pose proof rows from gameplay and the character viewer."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re
import sys


SCREENSHOT_MARKER = "screenshot saved"

DEFAULT_BONES = (
    "bone_pelvis",
    "bone_spine1",
    "bone_spine2",
    "bone_spine3",
    "bone_neck",
    "bone_head",
    "bone_L-clavicle",
    "bone_L-upperArm",
    "bone_L-upperTwist1",
    "bone_L-upperTwist2",
    "bone_L-foreArm",
    "bone_L-foreTwist1",
    "bone_L-foreTwist2",
    "bone_L-hand",
    "bone_R-clavicle",
    "bone_R-upperArm",
    "bone_R-upperTwist1",
    "bone_R-upperTwist2",
    "bone_R-foreArm",
    "bone_R-foreTwist1",
    "bone_R-foreTwist2",
    "bone_R-hand",
)

DEFAULT_ROWS = ("armw", "armr0", "armr1", "armr2")

ARM_ROW_RE = re.compile(
    r"^\[(armw|armr[0-2])\]\s+"
    r"c=(\S+)\s+t=(\S+)\s+b=(\S+)\s+[wv]="
    r"([-+0-9.eE]+),([-+0-9.eE]+),([-+0-9.eE]+)"
)


def detect_text_encoding(path: Path) -> str:
    with path.open("rb") as in_file:
        marker = in_file.read(4)
    if marker.startswith(b"\xff\xfe") or marker.startswith(b"\xfe\xff"):
        return "utf-16"
    if marker.startswith(b"\xef\xbb\xbf"):
        return "utf-8-sig"
    return "utf-8"


@dataclass(frozen=True)
class RowKey:
    row_type: str
    bone: str


@dataclass(frozen=True)
class PoseRow:
    values: tuple[float, float, float]
    line_number: int


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Compare the last complete [armw]/[armr*] pose rows before the "
            "screenshot marker in gameplay and viewer proof logs."
        )
    )
    parser.add_argument("--ingame-log", required=True, type=Path)
    parser.add_argument("--viewer-log", required=True, type=Path)
    parser.add_argument("--character", default="rockabill2")
    parser.add_argument("--tag", default="post")
    parser.add_argument(
        "--bone",
        action="append",
        dest="bones",
        help="Bone to compare. May be repeated. Defaults to the core torso/arm chain.",
    )
    parser.add_argument(
        "--row",
        action="append",
        choices=DEFAULT_ROWS,
        dest="rows",
        help="Row type to compare. May be repeated. Defaults to armw and armr0-2.",
    )
    parser.add_argument(
        "--tolerance",
        type=float,
        default=0.0006,
        help="Maximum absolute component delta allowed after log rounding.",
    )
    parser.add_argument(
        "--expect",
        choices=("match", "mismatch"),
        default="match",
        help="Expected outcome. Use mismatch for stale-viewer/control proofs.",
    )
    parser.add_argument(
        "--allow-no-screenshot-marker",
        action="store_true",
        help="Use the final matching rows if a log has no screenshot marker.",
    )
    return parser.parse_args()


def read_pose_rows(
    path: Path,
    *,
    character: str,
    tag: str,
    require_screenshot_marker: bool,
) -> tuple[dict[RowKey, PoseRow], int | None]:
    rows: dict[RowKey, PoseRow] = {}
    screenshot_line: int | None = None
    with path.open("r", encoding=detect_text_encoding(path), errors="replace") as in_file:
        for line_number, line in enumerate(in_file, 1):
            if SCREENSHOT_MARKER in line:
                screenshot_line = line_number
                break
            match = ARM_ROW_RE.match(line.rstrip())
            if match is None:
                continue
            row_type, row_character, row_tag, bone = match.group(1, 2, 3, 4)
            if row_character != character or row_tag != tag:
                continue
            values = tuple(float(match.group(i)) for i in (5, 6, 7))
            rows[RowKey(row_type, bone)] = PoseRow(values=values, line_number=line_number)
    if screenshot_line is None and require_screenshot_marker:
        raise RuntimeError(f"{path}: no '{SCREENSHOT_MARKER}' marker found")
    return rows, screenshot_line


def compare_rows(
    ingame: dict[RowKey, PoseRow],
    viewer: dict[RowKey, PoseRow],
    *,
    bones: tuple[str, ...],
    row_types: tuple[str, ...],
    tolerance: float,
) -> tuple[bool, list[str], float, RowKey | None]:
    messages: list[str] = []
    passed = True
    max_delta = 0.0
    worst_key: RowKey | None = None

    for bone in bones:
        for row_type in row_types:
            key = RowKey(row_type, bone)
            left = ingame.get(key)
            right = viewer.get(key)
            if left is None or right is None:
                passed = False
                missing = []
                if left is None:
                    missing.append("ingame")
                if right is None:
                    missing.append("viewer")
                messages.append(f"MISSING {row_type} {bone}: {','.join(missing)}")
                continue

            deltas = tuple(abs(a - b) for a, b in zip(left.values, right.values))
            row_delta = max(deltas)
            if row_delta > max_delta:
                max_delta = row_delta
                worst_key = key
            if row_delta > tolerance:
                passed = False
                messages.append(
                    f"DIFF {row_type} {bone}: max={row_delta:.6f} "
                    f"ingame={left.values} viewer={right.values}"
                )

    return passed, messages, max_delta, worst_key


def main() -> int:
    args = parse_args()
    bones = tuple(args.bones) if args.bones else DEFAULT_BONES
    row_types = tuple(args.rows) if args.rows else DEFAULT_ROWS
    require_marker = not args.allow_no_screenshot_marker

    try:
        ingame_rows, ingame_shot = read_pose_rows(
            args.ingame_log,
            character=args.character,
            tag=args.tag,
            require_screenshot_marker=require_marker,
        )
        viewer_rows, viewer_shot = read_pose_rows(
            args.viewer_log,
            character=args.character,
            tag=args.tag,
            require_screenshot_marker=require_marker,
        )
    except OSError as exc:
        print(f"ERROR {exc}", file=sys.stderr)
        return 2
    except RuntimeError as exc:
        print(f"ERROR {exc}", file=sys.stderr)
        return 2

    passed, messages, max_delta, worst_key = compare_rows(
        ingame_rows,
        viewer_rows,
        bones=bones,
        row_types=row_types,
        tolerance=args.tolerance,
    )
    compared = len(bones) * len(row_types)
    marker_text = (
        f"ingame_screenshot_line={ingame_shot} viewer_screenshot_line={viewer_shot}"
    )
    if args.expect == "mismatch":
        if not passed:
            print(
                f"EXPECTED-MISMATCH compared={compared} character={args.character} "
                f"tag={args.tag} max_delta={max_delta:.6f} "
                f"tolerance={args.tolerance:.6f} {marker_text}"
            )
            for message in messages[:20]:
                print(message)
            if len(messages) > 20:
                print(f"... {len(messages) - 20} more differences")
            return 0

        print(
            f"UNEXPECTED-MATCH compared={compared} character={args.character} "
            f"tag={args.tag} max_delta={max_delta:.6f} {marker_text}"
        )
        return 1

    if passed:
        worst = f"{worst_key.row_type}:{worst_key.bone}" if worst_key else "<none>"
        print(
            f"PASS compared={compared} character={args.character} tag={args.tag} "
            f"max_delta={max_delta:.6f} worst={worst} {marker_text}"
        )
        return 0

    print(
        f"FAIL compared={compared} character={args.character} tag={args.tag} "
        f"max_delta={max_delta:.6f} tolerance={args.tolerance:.6f} {marker_text}"
    )
    for message in messages[:40]:
        print(message)
    if len(messages) > 40:
        print(f"... {len(messages) - 40} more differences")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
