#!/usr/bin/env python3
"""Compare filtered arm-pose proof rows from gameplay and the character viewer."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
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
    parser.add_argument(
        "--manifest",
        type=Path,
        help="JSON manifest of proof pairs to compare as one batch.",
    )
    parser.add_argument("--ingame-log", type=Path)
    parser.add_argument("--viewer-log", type=Path)
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


@dataclass(frozen=True)
class CompareRequest:
    label: str
    ingame_log: Path
    viewer_log: Path
    character: str
    tag: str
    bones: tuple[str, ...]
    row_types: tuple[str, ...]
    tolerance: float
    expect: str
    proof_role: str
    known_control_reason: str
    require_screenshot_marker: bool
    require_ingame_contains: tuple[str, ...]
    require_viewer_contains: tuple[str, ...]


def tuple_from_manifest(value: object, default: tuple[str, ...], field: str) -> tuple[str, ...]:
    if value is None:
        return default
    if not isinstance(value, list) or not all(isinstance(item, str) for item in value):
        raise RuntimeError(f"manifest field '{field}' must be a list of strings")
    return tuple(value)


def compact_for_contains(value: str) -> str:
    return "".join(value.split())


def path_from_manifest(base_dir: Path, value: object, field: str) -> Path:
    if not isinstance(value, str) or not value:
        raise RuntimeError(f"manifest field '{field}' must be a non-empty string")
    path = Path(value)
    if path.is_absolute():
        return path
    return base_dir / path


def requests_from_manifest(path: Path) -> list[CompareRequest]:
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise RuntimeError(f"{path}: invalid JSON: {exc}") from exc
    if not isinstance(payload, dict):
        raise RuntimeError(f"{path}: manifest root must be an object")
    cases = payload.get("cases")
    if not isinstance(cases, list) or not cases:
        raise RuntimeError(f"{path}: manifest must contain a non-empty cases list")

    default_character = str(payload.get("character", "rockabill2"))
    default_tag = str(payload.get("tag", "post"))
    default_tolerance = float(payload.get("tolerance", 0.0006))
    base_dir = path.parent

    requests: list[CompareRequest] = []
    for index, case in enumerate(cases, 1):
        if not isinstance(case, dict):
            raise RuntimeError(f"{path}: case {index} must be an object")
        expect = str(case.get("expect", "match"))
        if expect not in ("match", "mismatch"):
            raise RuntimeError(f"{path}: case {index} has invalid expect '{expect}'")
        proof_role = str(case.get("proof_role", "current"))
        if proof_role not in ("current", "diagnostic_control"):
            raise RuntimeError(
                f"{path}: case {index} has invalid proof_role '{proof_role}'"
            )
        known_control_reason = str(case.get("known_control_reason", ""))
        if expect == "mismatch":
            if proof_role != "diagnostic_control":
                raise RuntimeError(
                    f"{path}: case {index} expected mismatch must be proof_role "
                    "diagnostic_control"
                )
            if not known_control_reason:
                raise RuntimeError(
                    f"{path}: case {index} diagnostic control must include "
                    "known_control_reason"
                )
        elif proof_role != "current" or known_control_reason:
            raise RuntimeError(
                f"{path}: case {index} matching proof must be proof_role current "
                "with no known_control_reason"
            )
        label = str(case.get("label", f"case{index}"))
        requests.append(
            CompareRequest(
                label=label,
                ingame_log=path_from_manifest(base_dir, case.get("ingame_log"), "ingame_log"),
                viewer_log=path_from_manifest(base_dir, case.get("viewer_log"), "viewer_log"),
                character=str(case.get("character", default_character)),
                tag=str(case.get("tag", default_tag)),
                bones=tuple_from_manifest(case.get("bones"), DEFAULT_BONES, "bones"),
                row_types=tuple_from_manifest(case.get("rows"), DEFAULT_ROWS, "rows"),
                tolerance=float(case.get("tolerance", default_tolerance)),
                expect=expect,
                proof_role=proof_role,
                known_control_reason=known_control_reason,
                require_screenshot_marker=not bool(
                    case.get("allow_no_screenshot_marker", False)
                ),
                require_ingame_contains=tuple_from_manifest(
                    case.get("require_ingame_contains"),
                    (),
                    "require_ingame_contains",
                ),
                require_viewer_contains=tuple_from_manifest(
                    case.get("require_viewer_contains"),
                    (),
                    "require_viewer_contains",
                ),
            )
        )
    return requests


def request_from_args(args: argparse.Namespace) -> CompareRequest:
    if args.ingame_log is None or args.viewer_log is None:
        raise RuntimeError("--ingame-log and --viewer-log are required without --manifest")
    return CompareRequest(
        label="cli",
        ingame_log=args.ingame_log,
        viewer_log=args.viewer_log,
        character=args.character,
        tag=args.tag,
        bones=tuple(args.bones) if args.bones else DEFAULT_BONES,
        row_types=tuple(args.rows) if args.rows else DEFAULT_ROWS,
        tolerance=args.tolerance,
        expect=args.expect,
        proof_role="current" if args.expect == "match" else "diagnostic_control",
        known_control_reason=(
            "explicit command-line expected mismatch"
            if args.expect == "mismatch"
            else ""
        ),
        require_screenshot_marker=not args.allow_no_screenshot_marker,
        require_ingame_contains=(),
        require_viewer_contains=(),
    )


def read_log_text(path: Path) -> str:
    return path.read_text(encoding=detect_text_encoding(path), errors="replace")


def missing_required_fragments(path: Path, required: tuple[str, ...]) -> list[str]:
    if not required:
        return []
    text = compact_for_contains(read_log_text(path))
    return [fragment for fragment in required if compact_for_contains(fragment) not in text]


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


def run_request(request: CompareRequest) -> int:
    try:
        missing_context: list[str] = []
        missing_context += [
            f"ingame missing required fragment: {fragment}"
            for fragment in missing_required_fragments(
                request.ingame_log, request.require_ingame_contains
            )
        ]
        missing_context += [
            f"viewer missing required fragment: {fragment}"
            for fragment in missing_required_fragments(
                request.viewer_log, request.require_viewer_contains
            )
        ]
        if missing_context:
            print(f"FAIL-CONTEXT label={request.label}")
            for message in missing_context[:40]:
                print(message)
            if len(missing_context) > 40:
                print(f"... {len(missing_context) - 40} more missing fragments")
            return 1

        ingame_rows, ingame_shot = read_pose_rows(
            request.ingame_log,
            character=request.character,
            tag=request.tag,
            require_screenshot_marker=request.require_screenshot_marker,
        )
        viewer_rows, viewer_shot = read_pose_rows(
            request.viewer_log,
            character=request.character,
            tag=request.tag,
            require_screenshot_marker=request.require_screenshot_marker,
        )
    except OSError as exc:
        print(f"ERROR {request.label}: {exc}", file=sys.stderr)
        return 2
    except RuntimeError as exc:
        print(f"ERROR {request.label}: {exc}", file=sys.stderr)
        return 2

    passed, messages, max_delta, worst_key = compare_rows(
        ingame_rows,
        viewer_rows,
        bones=request.bones,
        row_types=request.row_types,
        tolerance=request.tolerance,
    )
    compared = len(request.bones) * len(request.row_types)
    marker_text = (
        f"ingame_screenshot_line={ingame_shot} viewer_screenshot_line={viewer_shot}"
    )
    if request.expect == "mismatch":
        if not passed:
            print(
                f"EXPECTED-MISMATCH label={request.label} compared={compared} "
                f"character={request.character} tag={request.tag} "
                f"max_delta={max_delta:.6f} tolerance={request.tolerance:.6f} "
                f"proof_role={request.proof_role} "
                f"known_control_reason={request.known_control_reason!r} "
                f"{marker_text}"
            )
            for message in messages[:20]:
                print(message)
            if len(messages) > 20:
                print(f"... {len(messages) - 20} more differences")
            return 0

        print(
            f"UNEXPECTED-MATCH label={request.label} compared={compared} "
            f"character={request.character} tag={request.tag} "
            f"max_delta={max_delta:.6f} {marker_text}"
        )
        return 1

    if passed:
        worst = f"{worst_key.row_type}:{worst_key.bone}" if worst_key else "<none>"
        print(
            f"PASS label={request.label} compared={compared} "
            f"character={request.character} tag={request.tag} "
            f"max_delta={max_delta:.6f} worst={worst} {marker_text}"
        )
        return 0

    print(
        f"FAIL label={request.label} compared={compared} "
        f"character={request.character} tag={request.tag} "
        f"max_delta={max_delta:.6f} tolerance={request.tolerance:.6f} "
        f"{marker_text}"
    )
    for message in messages[:40]:
        print(message)
    if len(messages) > 40:
        print(f"... {len(messages) - 40} more differences")
    return 1


def main() -> int:
    args = parse_args()
    try:
        requests = requests_from_manifest(args.manifest) if args.manifest else [
            request_from_args(args)
        ]
    except RuntimeError as exc:
        print(f"ERROR {exc}", file=sys.stderr)
        return 2

    worst_status = 0
    for request in requests:
        status = run_request(request)
        if status != 0 and worst_status == 0:
            worst_status = status
    if args.manifest and worst_status == 0:
        print(f"BATCH-PASS cases={len(requests)} manifest={args.manifest}")
    return worst_status


if __name__ == "__main__":
    raise SystemExit(main())
