#!/usr/bin/env python3
"""Guard the native lower-body bridge against fabricated shortcut fixes."""

from __future__ import annotations

import argparse
from pathlib import Path
import sys


LOWER_BODY_MARKERS = (
    "bone_facing",
    "bone_pelvis",
    "-thigh",
    "-knee",
    "-ankle",
    "-foot",
    "-toe",
)

FORBIDDEN_BRIDGE_TOKENS = (
    "rockabill",
    "rock1",
    "rock2",
    "character.name",
    "char_name",
    "foot_offset",
    "leg_offset",
    "ankle_offset",
    "toe_offset",
    "fixup",
)


def extract_between(text: str, start: str, end: str) -> str:
    start_at = text.find(start)
    if start_at < 0:
        raise RuntimeError(f"missing source block start: {start}")
    end_at = text.find(end, start_at + len(start))
    if end_at < 0:
        raise RuntimeError(f"missing source block end after {start}: {end}")
    return text[start_at:end_at]


def require_contains(block: str, needle: str, label: str) -> None:
    if needle not in block:
        raise RuntimeError(f"missing {label}: {needle}")


def require_absent(block: str, needle: str, label: str) -> None:
    if needle in block:
        raise RuntimeError(f"forbidden {label}: {needle}")


def check_boundary(path: Path) -> None:
    text = path.read_text(encoding="utf-8", errors="replace")
    classifier = extract_between(
        text,
        "static bool is_lower_body_pose_channel_name",
        "static void dump_pose_source_weight_channel",
    )
    bridge = extract_between(
        text,
        "static void apply_lower_body_output_layer",
        "static void apply_hand_driver_output_layers",
    )

    for marker in LOWER_BODY_MARKERS:
        require_contains(classifier, marker, "lower-body classifier marker")

    require_contains(
        bridge,
        "const std::vector<CharClip::OutputBone>& source_output_bones",
        "source-authored output bone parameter",
    )
    require_contains(
        bridge,
        "if (!is_lower_body_pose_channel_name(key)) continue;",
        "source output row lower-body filter",
    )
    require_contains(
        bridge,
        "if (!lower_keys.insert(key).second) continue;",
        "deduplicated source output keys",
    )
    require_contains(
        bridge,
        "if (lower_keys.find(strip_transform_suffix(ch.bone_name)) ==",
        "frame channels filtered through source output keys",
    )
    require_contains(
        bridge,
        "apply_clip_pose_output_layer(lower_channels, weight, character, relative,\n"
        "                                   lower_output_bones, true)",
        "forced live selected output graph bridge",
    )
    require_contains(bridge, 'dump_leg_pose(character, "lower-output")', "lower-output proof tag")

    for token in FORBIDDEN_BRIDGE_TOKENS:
        require_absent(bridge, token, "lower-body bridge shortcut")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Check that apply_lower_body_output_layer remains a source-authored "
            "OutputBone subset bridge, not a character/name/offset workaround."
        )
    )
    parser.add_argument(
        "--source",
        type=Path,
        default=Path("engine/src/character/char_clip.cpp"),
    )
    return parser.parse_args()


def main() -> int:
    try:
        check_boundary(parse_args().source)
    except RuntimeError as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        return 1
    print("PASS lower_body_bridge_boundary source_output_subset=true no_named_or_offset_shortcut=true")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
