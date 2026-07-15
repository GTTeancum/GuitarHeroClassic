#!/usr/bin/env python3
"""Verify that stock GH2 lower-body proof manifests cover all base characters."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import struct
import sys

from compare_arm_pose_logs import RowKey, read_pose_rows


LOWER_BODY_BONES = {
    "bone_pelvis",
    "bone_L-thigh",
    "bone_L-knee",
    "bone_L-ankle",
    "bone_L-toe",
    "bone_R-thigh",
    "bone_R-knee",
    "bone_R-ankle",
    "bone_R-toe",
}

PLAYABLE_INGAME_LABELS = {
    "alterna1": "alterna1_current_lower_body_live_stack_match",
    "alterna2": "alterna2_current_lower_body_live_stack_match",
    "classic": "classic_current_lower_body_live_stack_match",
    "deathmetal1": "deathmetal1_current_lower_body_live_stack_match",
    "deathmetal2": "deathmetal2_current_lower_body_live_stack_match",
    "funk1": "funk1_current_lower_body_live_stack_match",
    "glam1": "glam1_current_lower_body_live_stack_match",
    "glam2": "glam2_current_lower_body_live_stack_match",
    "goth1": "goth1_current_lower_body_live_stack_match",
    "goth2": "goth2_current_lower_body_live_stack_match",
    "metal1": "metal1_current_lower_body_live_stack_match",
    "metal2": "metal2_current_lower_body_live_stack_match",
    "punk1": "punk1_current_lower_body_live_stack_match",
    "punk2": "punk2_current_lower_body_live_stack_match",
    "rock1": "rock1_current_lower_body_live_stack_match",
    "rock2": "rock2_current_lower_body_live_stack_match",
    "rockabill1": "rockabill1_current_lower_body_live_stack_match",
    "rockabill2": "rockabill2_current_lower_body_live_stack_match",
}

SUPPORT_VIEWER_LABELS = {
    "female_singer": "female_singer_support_lower_body_f70_live_match",
    "grim": "grim_support_lower_body_f70_live_match",
    "metal_bass": "metal_bass_support_lower_body_f70_live_match",
    "metal_drummer": "metal_drummer_support_lower_body_f70_live_match",
    "metal_keyboard": "metal_keyboard_support_lower_body_f70_live_match",
    "metal_singer": "metal_singer_support_lower_body_f70_live_match",
}

SUPPORT_OUTPUT_BONES = {
    "metal_bass": {
        "bone_L-thigh",
        "bone_L-knee",
        "bone_L-foot",
        "bone_L-toe",
        "bone_R-thigh",
        "bone_R-knee",
        "bone_R-foot",
        "bone_R-toe",
    },
    "metal_drummer": {
        "bone_L-thigh",
        "bone_L-knee",
        "bone_L-ankle",
        "bone_L-toe0",
        "bone_R-thigh",
        "bone_R-knee",
        "bone_R-ankle",
        "bone_R-toe0",
    },
}

SCREENSHOT_MARKERS = ("screenshot saved", "saved screenshot", "screenshot ->")
MIN_PROOF_WIDTH = 1280
MIN_PROOF_HEIGHT = 720
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
MAX_PLAYABLE_LOWEST_TOE_Z = 1.0
MIN_PLAYABLE_PELVIS_TO_LOWEST_TOE_Z = 30.0
MIN_PLAYABLE_CHAIN_Z_DROP = 8.0


def detect_text_encoding(path: Path) -> str:
    with path.open("rb") as in_file:
        marker = in_file.read(4)
    if marker.startswith(b"\xff\xfe") or marker.startswith(b"\xfe\xff"):
        return "utf-16"
    if marker.startswith(b"\xef\xbb\xbf"):
        return "utf-8-sig"
    return "utf-8"


def load_json(path: Path) -> dict:
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise RuntimeError(f"{path}: invalid JSON: {exc}") from exc
    if not isinstance(payload, dict):
        raise RuntimeError(f"{path}: root must be an object")
    return payload


def cases_by_label(manifest: dict, path: Path) -> dict[str, dict]:
    cases = manifest.get("cases")
    if not isinstance(cases, list):
        raise RuntimeError(f"{path}: missing cases list")
    result: dict[str, dict] = {}
    for case in cases:
        if not isinstance(case, dict):
            raise RuntimeError(f"{path}: each case must be an object")
        label = case.get("label")
        if not isinstance(label, str) or not label:
            raise RuntimeError(f"{path}: each case needs a label")
        result[label] = case
    return result


def path_from_manifest(manifest_path: Path, value: object, field: str) -> Path:
    if not isinstance(value, str) or not value:
        raise RuntimeError(f"{manifest_path}: {field} must be a non-empty string")
    path = Path(value)
    return path if path.is_absolute() else manifest_path.parent / path


def read_log(path: Path) -> str:
    try:
        return path.read_text(encoding=detect_text_encoding(path), errors="replace")
    except OSError as exc:
        raise RuntimeError(f"{path}: {exc}") from exc


def compact_for_contains(value: str) -> str:
    return "".join(value.split())


def require_fragments(path: Path, fragments: object, label: str) -> None:
    if fragments is None:
        return
    if not isinstance(fragments, list) or not all(isinstance(item, str) for item in fragments):
        raise RuntimeError(f"{label}: required fragments must be a list of strings")
    if not fragments:
        return
    text = compact_for_contains(read_log(path))
    missing = [
        fragment for fragment in fragments if compact_for_contains(fragment) not in text
    ]
    if missing:
        raise RuntimeError(f"{label}: {path} missing required fragment {missing[0]}")


def require_log_with_screenshot(path: Path) -> None:
    text = read_log(path)
    if not any(marker in text for marker in SCREENSHOT_MARKERS):
        raise RuntimeError(f"{path}: no screenshot marker found")


def png_dimensions(path: Path) -> tuple[int, int]:
    try:
        with path.open("rb") as in_file:
            header = in_file.read(24)
    except OSError as exc:
        raise RuntimeError(f"{path}: {exc}") from exc
    if not header.startswith(PNG_SIGNATURE):
        raise RuntimeError(f"{path}: not a PNG file")
    if len(header) < 24:
        raise RuntimeError(f"{path}: truncated PNG header")
    return struct.unpack(">II", header[16:24])


def require_inspectable_png(path: Path) -> None:
    if not path.is_file():
        raise RuntimeError(f"{path}: visual proof missing")
    width, height = png_dimensions(path)
    if width < MIN_PROOF_WIDTH or height < MIN_PROOF_HEIGHT:
        raise RuntimeError(f"{path}: visual proof resolution too small {width}x{height}")


def require_png_in_folder(folder: Path, prefix: str) -> None:
    if not folder.is_dir():
        raise RuntimeError(f"{folder}: proof folder missing")
    matches = [item for item in folder.glob("*.png") if item.name.startswith(prefix)]
    if not matches:
        raise RuntimeError(f"{folder}: no {prefix}*.png visual proof found")
    for match in matches:
        require_inspectable_png(match)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def playable_leg_chain_metrics(log_path: Path, character: str, tag: str) -> tuple[float, float]:
    rows, _ = read_pose_rows(
        log_path,
        character=character,
        tag=tag,
        require_screenshot_marker=True,
    )

    def world(bone: str) -> tuple[float, float, float]:
        key = RowKey("armw", bone)
        row = rows.get(key)
        require(row is not None, f"{log_path}: missing playable leg-chain row {bone}")
        return row.values

    pelvis_z = world("bone_pelvis")[2]
    toe_z_values = []
    for side in ("L", "R"):
        thigh_z = world(f"bone_{side}-thigh")[2]
        knee_z = world(f"bone_{side}-knee")[2]
        ankle_z = world(f"bone_{side}-ankle")[2]
        toe_z = world(f"bone_{side}-toe")[2]
        toe_z_values.append(toe_z)
        require(
            knee_z <= thigh_z - MIN_PLAYABLE_CHAIN_Z_DROP,
            f"{log_path}: {side} knee is not below thigh enough "
            f"({knee_z:.4f} vs {thigh_z:.4f})",
        )
        require(
            ankle_z <= knee_z - MIN_PLAYABLE_CHAIN_Z_DROP,
            f"{log_path}: {side} ankle is not below knee enough "
            f"({ankle_z:.4f} vs {knee_z:.4f})",
        )
        require(
            toe_z <= ankle_z,
            f"{log_path}: {side} toe is above ankle ({toe_z:.4f} vs {ankle_z:.4f})",
        )

    lowest_toe_z = min(toe_z_values)
    pelvis_to_lowest_toe_z = pelvis_z - lowest_toe_z
    require(
        lowest_toe_z <= MAX_PLAYABLE_LOWEST_TOE_Z,
        f"{log_path}: lowest toe z {lowest_toe_z:.4f} > {MAX_PLAYABLE_LOWEST_TOE_Z:.4f}",
    )
    require(
        pelvis_to_lowest_toe_z >= MIN_PLAYABLE_PELVIS_TO_LOWEST_TOE_Z,
        f"{log_path}: pelvis/toe drop {pelvis_to_lowest_toe_z:.4f} "
        f"< {MIN_PLAYABLE_PELVIS_TO_LOWEST_TOE_Z:.4f}",
    )
    return lowest_toe_z, pelvis_to_lowest_toe_z


def check_playable(
    arm_manifest_path: Path, arm_cases: dict[str, dict], default_character: str
) -> tuple[float, float]:
    max_lowest_toe_z = float("-inf")
    min_pelvis_to_lowest_toe_z = float("inf")
    for character, label in PLAYABLE_INGAME_LABELS.items():
        case = arm_cases.get(label)
        require(case is not None, f"missing playable lower-body case {label}")
        require(case.get("expect") == "match", f"{label}: expected match proof")
        require(case.get("rows") == ["armw"], f"{label}: must compare final world rows only")
        bones = set(case.get("bones", []))
        missing_bones = sorted(LOWER_BODY_BONES - bones)
        require(not missing_bones, f"{label}: missing lower-body bones {missing_bones}")
        ingame_log = path_from_manifest(arm_manifest_path, case.get("ingame_log"), "ingame_log")
        viewer_log = path_from_manifest(arm_manifest_path, case.get("viewer_log"), "viewer_log")
        require(ingame_log.is_file(), f"{label}: missing in-game log {ingame_log}")
        require(viewer_log.is_file(), f"{label}: missing viewer log {viewer_log}")
        require_log_with_screenshot(ingame_log)
        require_log_with_screenshot(viewer_log)
        require_fragments(
            ingame_log, case.get("require_ingame_contains"), f"{label}: in-game log"
        )
        require_fragments(
            viewer_log, case.get("require_viewer_contains"), f"{label}: viewer log"
        )
        require_png_in_folder(ingame_log.parent, "ingame_")
        require_png_in_folder(viewer_log.parent, "viewer_")
        case_character = str(case.get("character", default_character))
        if character.startswith("punk"):
            require(case_character == "punk", f"{label}: expected runtime punk row label")
        elif character == "rockabill1":
            require(case_character == "rockabill", f"{label}: expected runtime rockabill row label")
        else:
            require(case_character == character, f"{label}: wrong character row label")
        for log_path in (ingame_log, viewer_log):
            lowest_toe_z, pelvis_to_lowest_toe_z = playable_leg_chain_metrics(
                log_path, case_character, str(case.get("tag", "post"))
            )
            max_lowest_toe_z = max(max_lowest_toe_z, lowest_toe_z)
            min_pelvis_to_lowest_toe_z = min(
                min_pelvis_to_lowest_toe_z, pelvis_to_lowest_toe_z
            )
    return max_lowest_toe_z, min_pelvis_to_lowest_toe_z


def check_support(output_manifest_path: Path, output_cases: dict[str, dict]) -> None:
    for character, label in SUPPORT_VIEWER_LABELS.items():
        case = output_cases.get(label)
        require(case is not None, f"missing support lower-body case {label}")
        require(case.get("character") == character, f"{label}: wrong character")
        require(case.get("tag") == "lower-output", f"{label}: wrong visible tag")
        require(case.get("require_live") is True, f"{label}: must require live output rows")
        require(float(case.get("max_abs_xyz_gap", 1.0)) <= 0.001, f"{label}: tolerance too loose")
        expected_bones = SUPPORT_OUTPUT_BONES.get(character, LOWER_BODY_BONES - {"bone_pelvis"})
        bones = set(case.get("bones", output_cases.get("__default_bones__", [])))
        missing_bones = sorted(expected_bones - bones)
        require(not missing_bones, f"{label}: missing authored output bones {missing_bones}")
        if character == "metal_drummer":
            aliases = case.get("visible_bone_aliases")
            require(
                aliases == {
                    "bone_L-toe0": "bone_L-toe",
                    "bone_R-toe0": "bone_R-toe",
                },
                f"{label}: missing toe0 diagnostic aliases",
            )
        log_path = path_from_manifest(output_manifest_path, case.get("log"), "log")
        require(log_path.is_file(), f"{label}: missing proof log {log_path}")
        require_log_with_screenshot(log_path)
        require_fragments(log_path, case.get("require_contains"), f"{label}: proof log")
        png_path = log_path.with_suffix(".png")
        require_inspectable_png(png_path)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Check stock GH2 lower-body proof coverage across all 24 base characters."
    )
    parser.add_argument("--arm-manifest", type=Path, default=Path("tools/arm_pose_diff_manifest.json"))
    parser.add_argument(
        "--output-manifest",
        type=Path,
        default=Path("tools/charbone_output_map_manifest.json"),
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        arm_manifest = load_json(args.arm_manifest)
        output_manifest = load_json(args.output_manifest)
        arm_cases = cases_by_label(arm_manifest, args.arm_manifest)
        output_cases = cases_by_label(output_manifest, args.output_manifest)
        output_cases["__default_bones__"] = output_manifest.get("bones", [])
        max_lowest_toe_z, min_pelvis_to_lowest_toe_z = check_playable(
            args.arm_manifest, arm_cases, str(arm_manifest.get("character", "rockabill2"))
        )
        check_support(args.output_manifest, output_cases)
    except RuntimeError as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        return 1
    print(
        "PASS lower_body_stock_coverage "
        f"playable_ingame={len(PLAYABLE_INGAME_LABELS)} "
        f"support_viewer={len(SUPPORT_VIEWER_LABELS)} "
        f"stock_total={len(PLAYABLE_INGAME_LABELS) + len(SUPPORT_VIEWER_LABELS)} "
        f"proof_min_resolution={MIN_PROOF_WIDTH}x{MIN_PROOF_HEIGHT} "
        f"playable_max_lowest_toe_z={max_lowest_toe_z:.4f} "
        f"playable_min_pelvis_to_lowest_toe_z={min_pelvis_to_lowest_toe_z:.4f} "
        "playable_leg_chain_sane=true"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
