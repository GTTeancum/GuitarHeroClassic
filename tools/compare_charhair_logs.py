#!/usr/bin/env python3
"""Compare CharHair source/runtime boundary proof rows from gameplay and viewer logs."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
from pathlib import Path
import re
import sys


SCREENSHOT_MARKERS = ("screenshot saved", "screenshot ->")
SIM_COMPARE_FIELDS = (
    "runtimeWriteback",
    "resolvedPointCollides",
    "managedHookup",
    "bandCharacterHookup",
    "defaultHookupWouldReturn",
    "dirCollides",
    "legacyInlinePoints",
    "hookupOverloadBody",
    "missingHookupOverloadBody",
    "zeroTimeBodyAvailable",
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


def read_lines_before_screenshot(path: Path, require_marker: bool) -> tuple[list[str], int | None]:
    lines: list[str] = []
    screenshot_line: int | None = None
    with path.open("r", encoding=detect_text_encoding(path), errors="replace") as in_file:
        for line_number, line in enumerate(in_file, 1):
            if any(marker in line for marker in SCREENSHOT_MARKERS):
                screenshot_line = line_number
                break
            lines.append(line.rstrip("\n"))
    if screenshot_line is None and require_marker:
        raise RuntimeError(f"{path}: no screenshot marker found")
    return lines, screenshot_line


def missing_required_fragments(lines: list[str], required: tuple[str, ...]) -> list[str]:
    if not required:
        return []
    text = compact_for_contains("\n".join(lines))
    return [fragment for fragment in required if compact_for_contains(fragment) not in text]


def extract_logical_records(lines: list[str], prefix: str, hair: str) -> list[str]:
    records: list[str] = []
    i = 0
    needle = f"{prefix} "
    hair_needle = f"hair={hair}"
    while i < len(lines):
        line = lines[i]
        if needle in line and hair_needle in line:
            pieces = [line]
            i += 1
            while i < len(lines) and not lines[i].startswith("["):
                pieces.append(lines[i])
                i += 1
            records.append(" ".join(pieces))
            continue
        i += 1
    return records


def parse_key_values(record: str) -> dict[str, str]:
    return dict(re.findall(r"([A-Za-z][A-Za-z0-9_]*)=([^\s]+)", record))


@dataclass(frozen=True)
class HairRequest:
    label: str
    hair: str
    ingame_log: Path
    viewer_log: Path
    require_screenshot_marker: bool
    require_ingame_contains: tuple[str, ...]
    require_viewer_contains: tuple[str, ...]
    require_common_contains: tuple[str, ...]
    expected_sim_fields: dict[str, str]


def tuple_from_manifest(value: object, field: str) -> tuple[str, ...]:
    if value is None:
        return ()
    if not isinstance(value, list) or not all(isinstance(item, str) for item in value):
        raise RuntimeError(f"manifest field '{field}' must be a list of strings")
    return tuple(value)


def dict_from_manifest(value: object, field: str) -> dict[str, str]:
    if value is None:
        return {}
    if not isinstance(value, dict) or not all(
        isinstance(key, str) and isinstance(item, (str, int, float))
        for key, item in value.items()
    ):
        raise RuntimeError(f"manifest field '{field}' must be an object of scalar values")
    return {key: str(item) for key, item in value.items()}


def path_from_manifest(base_dir: Path, value: object, field: str) -> Path:
    if not isinstance(value, str) or not value:
        raise RuntimeError(f"manifest field '{field}' must be a non-empty string")
    path = Path(value)
    if path.is_absolute():
        return path
    return base_dir / path


def requests_from_manifest(path: Path) -> list[HairRequest]:
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
    requests: list[HairRequest] = []
    for index, case in enumerate(cases, 1):
        if not isinstance(case, dict):
            raise RuntimeError(f"{path}: case {index} must be an object")
        label = str(case.get("label", f"case{index}"))
        hair = str(case.get("hair", ""))
        if not hair:
            raise RuntimeError(f"{path}: case {index} needs a hair name")
        requests.append(
            HairRequest(
                label=label,
                hair=hair,
                ingame_log=path_from_manifest(base_dir, case.get("ingame_log"), "ingame_log"),
                viewer_log=path_from_manifest(base_dir, case.get("viewer_log"), "viewer_log"),
                require_screenshot_marker=not bool(case.get("allow_no_screenshot_marker", False)),
                require_ingame_contains=tuple_from_manifest(
                    case.get("require_ingame_contains"), "require_ingame_contains"
                ),
                require_viewer_contains=tuple_from_manifest(
                    case.get("require_viewer_contains"), "require_viewer_contains"
                ),
                require_common_contains=tuple_from_manifest(
                    case.get("require_common_contains"), "require_common_contains"
                ),
                expected_sim_fields=dict_from_manifest(
                    case.get("expected_sim_fields"), "expected_sim_fields"
                ),
            )
        )
    return requests


def run_request(request: HairRequest) -> int:
    try:
        ingame_lines, ingame_shot = read_lines_before_screenshot(
            request.ingame_log, request.require_screenshot_marker
        )
        viewer_lines, viewer_shot = read_lines_before_screenshot(
            request.viewer_log, request.require_screenshot_marker
        )
    except OSError as exc:
        print(f"ERROR {request.label}: {exc}", file=sys.stderr)
        return 2
    except RuntimeError as exc:
        print(f"ERROR {request.label}: {exc}", file=sys.stderr)
        return 2

    missing_context: list[str] = []
    missing_context += [
        f"ingame missing required fragment: {fragment}"
        for fragment in missing_required_fragments(
            ingame_lines, request.require_ingame_contains + request.require_common_contains
        )
    ]
    missing_context += [
        f"viewer missing required fragment: {fragment}"
        for fragment in missing_required_fragments(
            viewer_lines, request.require_viewer_contains + request.require_common_contains
        )
    ]
    if missing_context:
        print(f"FAIL-CONTEXT label={request.label} hair={request.hair}")
        for message in missing_context[:40]:
            print(message)
        if len(missing_context) > 40:
            print(f"... {len(missing_context) - 40} more missing fragments")
        return 1

    ingame_sim = extract_logical_records(
        ingame_lines, "[charhair-source-sim]", request.hair
    )
    viewer_sim = extract_logical_records(
        viewer_lines, "[charhair-source-sim]", request.hair
    )
    if not ingame_sim or not viewer_sim:
        print(f"FAIL label={request.label} hair={request.hair} missing sim records")
        return 1

    left = parse_key_values(ingame_sim[-1])
    right = parse_key_values(viewer_sim[-1])
    messages: list[str] = []
    for field in SIM_COMPARE_FIELDS:
        if left.get(field) != right.get(field):
            messages.append(
                f"DIFF {field}: ingame={left.get(field, '<missing>')} "
                f"viewer={right.get(field, '<missing>')}"
            )
    for field, expected in request.expected_sim_fields.items():
        if left.get(field) != expected or right.get(field) != expected:
            messages.append(
                f"UNEXPECTED {field}: expected={expected} "
                f"ingame={left.get(field, '<missing>')} "
                f"viewer={right.get(field, '<missing>')}"
            )

    marker_text = (
        f"ingame_screenshot_line={ingame_shot} viewer_screenshot_line={viewer_shot}"
    )
    if messages:
        print(f"FAIL label={request.label} hair={request.hair} {marker_text}")
        for message in messages[:40]:
            print(message)
        if len(messages) > 40:
            print(f"... {len(messages) - 40} more differences")
        return 1

    print(
        f"PASS label={request.label} hair={request.hair} "
        f"sim_records=ingame:{len(ingame_sim)} viewer:{len(viewer_sim)} "
        f"runtimeWriteback={left.get('runtimeWriteback', '<missing>')} "
        f"legacyInlinePoints={left.get('legacyInlinePoints', '<missing>')} "
        f"missingHookupOverloadBody={left.get('missingHookupOverloadBody', '<missing>')} "
        f"{marker_text}"
    )
    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Compare CharHair source/runtime proof rows from gameplay and viewer logs."
    )
    parser.add_argument("--manifest", type=Path, required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        requests = requests_from_manifest(args.manifest)
    except RuntimeError as exc:
        print(f"ERROR {exc}", file=sys.stderr)
        return 2

    worst_status = 0
    for request in requests:
        status = run_request(request)
        if status != 0 and worst_status == 0:
            worst_status = status
    if worst_status == 0:
        print(f"BATCH-PASS cases={len(requests)} manifest={args.manifest}")
    return worst_status


if __name__ == "__main__":
    raise SystemExit(main())
