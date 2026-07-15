#!/usr/bin/env python3
"""Compare decoded CharBone output rows against visible bone pose rows."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
from pathlib import Path
import re
import sys


SCREENSHOT_MARKERS = ("screenshot saved", "screenshot ->")

OUT_MAP_RE = re.compile(
    r"^\[out-map\]\s+(\S+)\s+output=(\S+)\s+parent=(.*?)\s+"
    r"driven=([01])\s+live=([01])\s+(.*)$"
)

VEC_RE = re.compile(
    r"(outLocal|meshLocal|outBindW|meshBindW|outPoseW|meshPoseW|bindLocal)="
    r"\(([-+0-9.eE]+)\s+([-+0-9.eE]+)\s+([-+0-9.eE]+)\)"
)

ARM_WORLD_RE = re.compile(
    r"^\[armw\]\s+c=(\S+)\s+t=(\S+)\s+b=(\S+)\s+w="
    r"([-+0-9.eE]+),([-+0-9.eE]+),([-+0-9.eE]+)"
)

LEG_WORLD_RE = re.compile(
    r"^\[legw\]\s+c=(\S+)\s+t=(\S+)\s+b=(\S+)\s+w="
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


def compact_for_contains(value: str) -> str:
    return "".join(value.split())


@dataclass(frozen=True)
class OutputRow:
    bone: str
    output: str
    parent: str
    driven: bool
    live: bool
    vectors: dict[str, tuple[float, float, float]]
    line_number: int


@dataclass(frozen=True)
class VisibleWorldRow:
    character: str
    tag: str
    bone: str
    world: tuple[float, float, float]
    line_number: int


@dataclass(frozen=True)
class CompareRequest:
    label: str
    log: Path
    character: str
    tag: str
    bones: tuple[str, ...]
    require_driven: bool
    require_live: bool
    require_fragments: tuple[str, ...]
    visible_minus_output_z_min: dict[str, float]
    require_screenshot_marker: bool


def read_log_text(path: Path) -> str:
    return path.read_text(encoding=detect_text_encoding(path), errors="replace")


def joined_records_before_screenshot(path: Path, require_marker: bool) -> tuple[list[tuple[int, str]], int | None]:
    records: list[tuple[int, str]] = []
    current_line = 0
    current_parts: list[str] = []
    screenshot_line: int | None = None

    def flush() -> None:
        nonlocal current_line, current_parts
        if current_parts:
            records.append((current_line, " ".join(current_parts)))
            current_parts = []
            current_line = 0

    with path.open("r", encoding=detect_text_encoding(path), errors="replace") as in_file:
        for line_number, raw_line in enumerate(in_file, 1):
            line = raw_line.rstrip()
            if any(marker in line for marker in SCREENSHOT_MARKERS):
                flush()
                screenshot_line = line_number
                break
            if line.startswith("["):
                flush()
                current_line = line_number
                current_parts = [line]
            elif current_parts:
                current_parts.append(line)
        else:
            flush()

    if screenshot_line is None and require_marker:
        raise RuntimeError(f"{path}: no screenshot marker found")
    return records, screenshot_line


def parse_output_rows(
    path: Path, require_marker: bool
) -> tuple[dict[str, OutputRow], dict[tuple[str, str, str], VisibleWorldRow], int | None]:
    rows: dict[str, OutputRow] = {}
    visible_rows: dict[tuple[str, str, str], VisibleWorldRow] = {}
    records, screenshot_line = joined_records_before_screenshot(path, require_marker)
    for line_number, record in records:
        match = OUT_MAP_RE.match(record)
        if match is not None:
            bone, output, parent, driven, live, rest = match.groups()
            vectors = {
                name: (float(x), float(y), float(z))
                for name, x, y, z in VEC_RE.findall(rest)
            }
            rows[bone] = OutputRow(
                bone=bone,
                output=output,
                parent=parent.strip(),
                driven=driven == "1",
                live=live == "1",
                vectors=vectors,
                line_number=line_number,
            )
            continue
        arm_match = ARM_WORLD_RE.match(record)
        leg_match = LEG_WORLD_RE.match(record)
        world_match = arm_match if arm_match is not None else leg_match
        if world_match is None:
            continue
        character, tag, bone, x, y, z = world_match.groups()
        visible_rows[(character, tag, bone)] = VisibleWorldRow(
            character=character,
            tag=tag,
            bone=bone,
            world=(float(x), float(y), float(z)),
            line_number=line_number,
        )
    return rows, visible_rows, screenshot_line


def tuple_from_manifest(value: object, default: tuple[str, ...], field: str) -> tuple[str, ...]:
    if value is None:
        return default
    if not isinstance(value, list) or not all(isinstance(item, str) for item in value):
        raise RuntimeError(f"manifest field '{field}' must be a list of strings")
    return tuple(value)


def path_from_manifest(base_dir: Path, value: object, field: str) -> Path:
    if not isinstance(value, str) or not value:
        raise RuntimeError(f"manifest field '{field}' must be a non-empty string")
    path = Path(value)
    return path if path.is_absolute() else base_dir / path


def z_gap_map_from_manifest(value: object) -> dict[str, float]:
    if value is None:
        return {}
    if not isinstance(value, dict):
        raise RuntimeError("manifest z-gap field must be an object")
    result: dict[str, float] = {}
    for key, gap in value.items():
        if not isinstance(key, str):
            raise RuntimeError("mesh_minus_output_z_min keys must be strings")
        result[key] = float(gap)
    return result


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
    base_dir = path.parent
    default_bones = tuple_from_manifest(payload.get("bones"), (), "bones")
    default_character = str(payload.get("character", "rockabill2"))
    default_tag = str(payload.get("tag", "post"))
    requests: list[CompareRequest] = []
    for index, case in enumerate(cases, 1):
        if not isinstance(case, dict):
            raise RuntimeError(f"{path}: case {index} must be an object")
        requests.append(
            CompareRequest(
                label=str(case.get("label", f"case{index}")),
                log=path_from_manifest(base_dir, case.get("log"), "log"),
                character=str(case.get("character", default_character)),
                tag=str(case.get("tag", default_tag)),
                bones=tuple_from_manifest(case.get("bones"), default_bones, "bones"),
                require_driven=bool(case.get("require_driven", True)),
                require_live=bool(case.get("require_live", False)),
                require_fragments=tuple_from_manifest(
                    case.get("require_contains"), (), "require_contains"
                ),
                visible_minus_output_z_min=z_gap_map_from_manifest(
                    case.get(
                        "visible_minus_output_z_min",
                        case.get("mesh_minus_output_z_min"),
                    )
                ),
                require_screenshot_marker=not bool(
                    case.get("allow_no_screenshot_marker", False)
                ),
            )
        )
    return requests


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Verify CharBone output-map proof rows before screenshot markers."
    )
    parser.add_argument("--manifest", type=Path)
    parser.add_argument("--log", type=Path)
    parser.add_argument("--character", default="rockabill2")
    parser.add_argument("--tag", default="post")
    parser.add_argument("--bone", action="append", dest="bones")
    parser.add_argument("--allow-no-screenshot-marker", action="store_true")
    return parser.parse_args()


def request_from_args(args: argparse.Namespace) -> CompareRequest:
    if args.log is None:
        raise RuntimeError("--log is required without --manifest")
    return CompareRequest(
        label="cli",
        log=args.log,
        character=args.character,
        tag=args.tag,
        bones=tuple(args.bones or ()),
        require_driven=True,
        require_live=False,
        require_fragments=(),
        visible_minus_output_z_min={},
        require_screenshot_marker=not args.allow_no_screenshot_marker,
    )


def run_request(request: CompareRequest) -> int:
    try:
        text = compact_for_contains(read_log_text(request.log))
        missing_fragments = [
            fragment
            for fragment in request.require_fragments
            if compact_for_contains(fragment) not in text
        ]
        if missing_fragments:
            print(f"FAIL-CONTEXT label={request.label}")
            for fragment in missing_fragments:
                print(f"missing required fragment: {fragment}")
            return 1
        rows, visible_rows, screenshot_line = parse_output_rows(
            request.log, request.require_screenshot_marker
        )
    except OSError as exc:
        print(f"ERROR {request.label}: {exc}", file=sys.stderr)
        return 2
    except RuntimeError as exc:
        print(f"ERROR {request.label}: {exc}", file=sys.stderr)
        return 2

    messages: list[str] = []
    max_abs_z_gap = 0.0
    min_z_gap: float | None = None
    for bone in request.bones:
        row = rows.get(bone)
        if row is None:
            messages.append(f"MISSING {bone}")
            continue
        if row.driven != request.require_driven:
            messages.append(f"DRIVEN {bone}: got={int(row.driven)}")
        if row.live != request.require_live:
            messages.append(f"LIVE {bone}: got={int(row.live)}")
        visible = visible_rows.get((request.character, request.tag, bone))
        if visible is None:
            messages.append(
                f"VISIBLE {bone}: missing [legw]/[armw] c={request.character} t={request.tag}"
            )
            continue
        if "outPoseW" not in row.vectors:
            messages.append(f"VECTORS {bone}: missing outPoseW")
            continue
        z_gap = visible.world[2] - row.vectors["outPoseW"][2]
        min_z_gap = z_gap if min_z_gap is None else min(min_z_gap, z_gap)
        max_abs_z_gap = max(max_abs_z_gap, abs(z_gap))
        required_gap = request.visible_minus_output_z_min.get(bone)
        if required_gap is not None and z_gap < required_gap:
            messages.append(
                f"Z-GAP {bone}: visible_minus_output={z_gap:.3f} required={required_gap:.3f}"
            )

    if messages:
        print(
            f"FAIL label={request.label} rows={len(rows)} checked={len(request.bones)} "
            f"visible_rows={len(visible_rows)} min_z_gap={min_z_gap} "
            f"max_abs_z_gap={max_abs_z_gap:.3f} screenshot_line={screenshot_line}"
        )
        for message in messages[:40]:
            print(message)
        if len(messages) > 40:
            print(f"... {len(messages) - 40} more failures")
        return 1

    min_z_gap_text = f"{min_z_gap:.3f}" if min_z_gap is not None else "n/a"
    print(
        f"PASS label={request.label} rows={len(rows)} checked={len(request.bones)} "
        f"visible_rows={len(visible_rows)} min_z_gap={min_z_gap_text} "
        f"max_abs_z_gap={max_abs_z_gap:.3f} screenshot_line={screenshot_line}"
    )
    return 0


def main() -> int:
    args = parse_args()
    try:
        requests = requests_from_manifest(args.manifest) if args.manifest else [
            request_from_args(args)
        ]
    except RuntimeError as exc:
        print(f"ERROR {exc}", file=sys.stderr)
        return 2
    worst = 0
    for request in requests:
        status = run_request(request)
        if status != 0 and worst == 0:
            worst = status
    if args.manifest and worst == 0:
        print(f"BATCH-PASS cases={len(requests)} manifest={args.manifest}")
    return worst


if __name__ == "__main__":
    raise SystemExit(main())
