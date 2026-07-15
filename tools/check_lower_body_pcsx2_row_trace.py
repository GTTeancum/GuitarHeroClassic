#!/usr/bin/env python3
"""Verify the focused PCSX2 lower-body mesh/source row trace summary."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys
from typing import Any


COMPACT_TRANSFORM_BAND = (
    "0x4c",
    "0x50",
    "0x54",
    "0x5c",
    "0x60",
    "0x64",
    "0x6c",
    "0x70",
    "0x74",
)

STORED_TRANSFORM_BAND = (
    "0x8c",
    "0x90",
    "0x94",
    "0x98",
    "0x9c",
    "0xa0",
    "0xa4",
    "0xa8",
    "0xac",
    "0xb0",
    "0xb4",
    "0xb8",
    "0xbc",
    "0xc0",
    "0xc4",
    "0xc8",
    "0xcc",
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def resolve_manifest_path(manifest: Path, raw_path: str) -> Path:
    path = Path(raw_path)
    if path.is_absolute():
        return path
    return (manifest.parent / path).resolve()


def require_file(path: Path, label: str) -> None:
    require(path.is_file(), f"missing {label}: {path}")
    require(path.stat().st_size > 10_000, f"{label} is unexpectedly small: {path}")


def require_raw_row_label(raw_entry: dict[str, Any], bone: str, row_name: str) -> None:
    rows = raw_entry.get("initial_rows", [])
    require(isinstance(rows, list) and rows, f"{row_name}: missing initial rows")
    labeled = [row for row in rows if row.get("ascii") == bone]
    require(labeled, f"{row_name}: raw row did not contain label {bone}")
    label_addr = str(labeled[0].get("addr", "")).lower()
    sample_addr = str(raw_entry.get("addr", "")).lower()
    require(
        label_addr.startswith(sample_addr),
        f"{row_name}: label row is not at the sampled base address",
    )


def require_raw_change_detail(raw_entry: dict[str, Any], row_name: str) -> None:
    changes = raw_entry.get("changed", [])
    require(isinstance(changes, list) and changes, f"{row_name}: missing raw changed rows")
    richest = max(int(change.get("unique_count", 0)) for change in changes)
    require(richest >= 2, f"{row_name}: raw changed rows did not vary across samples")
    for change in changes[:3]:
        for key in ("addr", "first", "last", "unique_count"):
            require(key in change, f"{row_name}: changed row missing {key}")


def changed_offsets(raw_entry: dict[str, Any]) -> list[str]:
    base = int(str(raw_entry["addr"]), 16)
    return [
        hex(int(str(change["addr"]), 16) - base)
        for change in raw_entry.get("changed", [])
    ]


def max_unique_count(raw_entry: dict[str, Any]) -> int:
    return max(
        (int(change.get("unique_count", 0)) for change in raw_entry.get("changed", [])),
        default=0,
    )


def require_transform_band_motion(
    raw_entry: dict[str, Any],
    row_name: str,
    summary: dict[str, Any],
) -> tuple[int, int]:
    offsets = set(changed_offsets(raw_entry))
    compact_hits = len(offsets.intersection(COMPACT_TRANSFORM_BAND))
    stored_hits = len(offsets.intersection(STORED_TRANSFORM_BAND))
    require(
        stored_hits >= 9,
        f"{row_name}: missing source-row stored transform-band motion",
    )
    require(
        summary.get("offsets") == changed_offsets(raw_entry),
        f"{row_name}: raw summary changed offsets mismatch",
    )
    return compact_hits, stored_hits


def check_manifest(manifest_path: Path, source_json: Path | None) -> tuple[int, int, int, bool]:
    manifest = load_json(manifest_path)
    require(
        manifest.get("trace_id") == "pcsx2_rock_lower_body_mesh_rows_20260715",
        "unexpected trace id",
    )
    runtime = manifest.get("runtime", {})
    require(runtime.get("active_gameplay") is True, "trace was not active gameplay")
    require(runtime.get("focus_forced") is False, "trace must not force focus")
    require(runtime.get("sample_count") == 32, "unexpected manifest sample count")
    require(runtime.get("seconds") == 8.0, "unexpected manifest duration")
    require(
        manifest.get("source_truth")
        == "ihatecompvir_rb3_latest_CharBone_CharClip_CharBonesSamples",
        "PCSX2 trace must keep ihatecompvir as the source truth",
    )
    require_file(
        resolve_manifest_path(manifest_path, manifest["before_sample_screenshot"]),
        "before-sample PCSX2 screenshot",
    )
    require_file(
        resolve_manifest_path(manifest_path, manifest["after_sample_screenshot"]),
        "after-sample PCSX2 screenshot",
    )

    stale = manifest.get("stale_trace_rejected", {})
    require("0x00db" in stale.get("reason", ""), "stale address rejection missing")

    conclusion = manifest.get("source_backed_conclusion", {})
    require(
        conclusion.get("native_path") == "source_output_lower_body_bridge",
        "native conclusion must stay on source output bridge",
    )
    rejects = set(conclusion.get("rejects", []))
    for rejected in (
        "stale_named_mesh_wrapper_rows",
        "foot_ik",
        "camera_angle",
        "character_name_offset",
    ):
        require(rejected in rejects, f"missing rejected shortcut {rejected}")

    pairs = manifest.get("pairs", [])
    require(len(pairs) == 9, "expected pelvis plus eight lower-body pairs")
    required_moving = set(manifest.get("required_moving_desc_rows", []))
    seen_desc: dict[str, dict[str, Any]] = {}
    moving_desc = 0
    stable_mesh = 0
    source_matrix_rows = 0
    for pair in pairs:
        bone = pair.get("bone", "")
        require(bone.startswith("bone_") and bone.endswith(".mesh"), "bad bone label")
        desc = pair.get("desc", {})
        mesh = pair.get("mesh", {})
        desc_name = desc.get("name")
        mesh_name = mesh.get("name")
        require(desc_name and mesh_name, f"{bone}: missing row names")
        require(str(desc.get("addr", "")).startswith("0x00e"), f"{desc_name}: bad desc addr")
        require(str(mesh.get("addr", "")).startswith("0x00ef"), f"{mesh_name}: bad mesh addr")
        require(mesh.get("changed_count") == 0, f"{mesh_name}: wrapper row moved")
        stable_mesh += 1
        if desc.get("changed_count", 0) > 0:
            moving_desc += 1
        seen_desc[desc_name] = desc

    require(stable_mesh == 9, "not all mesh wrapper rows stayed stable")
    require(moving_desc >= 7, "not enough linked source/controller rows moved")
    missing_moving = sorted(required_moving - set(seen_desc))
    require(not missing_moving, f"missing moving desc rows {missing_moving}")
    for name in sorted(required_moving):
        require(seen_desc[name].get("changed_count", 0) > 0, f"{name} did not move")

    checked_raw = False
    if source_json is not None:
        raw = load_json(source_json)
        raw_targets = {target["name"]: target for target in raw.get("targets", [])}
        raw_change_offsets = manifest.get("raw_change_offsets", {})
        require(
            set(raw_change_offsets) == required_moving,
            "raw change-offset summary must match required moving rows",
        )
        require(raw.get("seconds") == runtime["seconds"], "raw duration mismatch")
        require(raw.get("interval") == 0.25, "raw interval mismatch")
        require_file(Path(raw["before_sample_screenshot"]), "raw before-sample screenshot")
        require_file(Path(raw["screenshot"]), "raw after-sample screenshot")
        for pair in pairs:
            bone = pair["bone"]
            for key in ("desc", "mesh"):
                entry = pair[key]
                raw_entry = raw_targets.get(entry["name"])
                require(raw_entry is not None, f"raw missing {entry['name']}")
                require(raw_entry.get("addr") == entry["addr"], f"{entry['name']}: addr mismatch")
                require(
                    raw_entry.get("changed_count") == entry["changed_count"],
                    f"{entry['name']}: changed_count mismatch",
                )
                require(
                    raw_entry.get("sample_count") == runtime["sample_count"],
                    f"{entry['name']}: sample_count mismatch",
                )
                require_raw_row_label(raw_entry, bone, entry["name"])
                if key == "mesh":
                    require(raw_entry.get("changed") == [], f"{entry['name']}: mesh wrapper changed")
                elif entry["name"] in required_moving:
                    require_raw_change_detail(raw_entry, entry["name"])
                    summary = raw_change_offsets[entry["name"]]
                    require(
                        summary.get("changed_count") == raw_entry.get("changed_count"),
                        f"{entry['name']}: raw summary changed_count mismatch",
                    )
                    require(
                        summary.get("max_unique_count") == max_unique_count(raw_entry),
                        f"{entry['name']}: raw summary max_unique_count mismatch",
                    )
                    compact_hits, stored_hits = require_transform_band_motion(
                        raw_entry, entry["name"], summary
                    )
                    if compact_hits > 0 or stored_hits > 0:
                        source_matrix_rows += 1
        checked_raw = True

    return stable_mesh, moving_desc, source_matrix_rows, checked_raw


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Check the committed PCSX2 lower-body row trace manifest."
    )
    parser.add_argument(
        "--manifest",
        type=Path,
        default=Path("tools/lower_body_pcsx2_row_trace_manifest.json"),
    )
    parser.add_argument(
        "--source-json",
        type=Path,
        help="Optional raw PCSX2 JSON to cross-check against the manifest.",
    )
    parser.add_argument(
        "--use-manifest-source-json",
        action="store_true",
        help="Cross-check the raw JSON path recorded in the manifest.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    source_json = args.source_json
    if args.use_manifest_source_json:
        manifest = load_json(args.manifest)
        source_json = resolve_manifest_path(args.manifest, manifest["source_json"])
    try:
        stable_mesh, moving_desc, source_matrix_rows, checked_raw = check_manifest(
            args.manifest, source_json
        )
    except RuntimeError as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        return 1
    print(
        "PASS lower_body_pcsx2_row_trace "
        f"stable_mesh_wrappers={stable_mesh} "
        f"moving_source_rows={moving_desc} "
        f"source_matrix_rows={source_matrix_rows} "
        f"source_json_checked={str(checked_raw).lower()} "
        "source_truth=ihatecompvir_rb3_latest_CharBone_CharClip_CharBonesSamples "
        "native_path=source_output_lower_body_bridge"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
