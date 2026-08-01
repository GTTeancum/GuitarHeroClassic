#!/usr/bin/env python3
"""Verify the glam1/metal1 lower-body follow-up proof batch."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import struct
import sys

from compare_charbone_output_map import parse_output_rows, read_log_text


PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
MIN_PROOF_WIDTH = 1280
MIN_PROOF_HEIGHT = 720
MAX_REASONABLE_WORLD_ABS = 80.0
MAX_LOWEST_TOE_Z = 1.25
MIN_PELVIS_TO_LOWEST_TOE_Z = 24.0
MIN_CHAIN_Z_DROP = 4.0

LOWER_BODY_BONES = (
    "bone_pelvis",
    "bone_L-thigh",
    "bone_L-knee",
    "bone_L-ankle",
    "bone_L-toe",
    "bone_R-thigh",
    "bone_R-knee",
    "bone_R-ankle",
    "bone_R-toe",
)

FORBIDDEN_LOG_MARKERS = (
    "startup checkerboard",
    "ARK error",
    "source_publisher=direct",
)


@dataclass(frozen=True)
class ProofCase:
    character: str
    view: str
    log_name: str
    png_name: str
    source_clip: str


PROOF_CASES = (
    ProofCase(
        character="glam1",
        view="front",
        log_name="glam1_front.log",
        png_name="glam1_front.png",
        source_clip="char/glam1/anims/gen/glam1_main.milo_ps2",
    ),
    ProofCase(
        character="glam1",
        view="side",
        log_name="glam1_side.log",
        png_name="glam1_side.png",
        source_clip="char/glam1/anims/gen/glam1_main.milo_ps2",
    ),
    ProofCase(
        character="metal1",
        view="front",
        log_name="metal1_front.log",
        png_name="metal1_front.png",
        source_clip="char/metal1/anims/gen/metal1_main.milo_ps2",
    ),
    ProofCase(
        character="metal1",
        view="side",
        log_name="metal1_side.log",
        png_name="metal1_side.png",
        source_clip="char/metal1/anims/gen/metal1_main.milo_ps2",
    ),
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def png_dimensions(path: Path) -> tuple[int, int]:
    with path.open("rb") as in_file:
        header = in_file.read(24)
    require(header.startswith(PNG_SIGNATURE), f"{path}: not a PNG")
    require(len(header) >= 24, f"{path}: truncated PNG header")
    return struct.unpack(">II", header[16:24])


def require_png(path: Path) -> None:
    require(path.is_file(), f"{path}: missing visual proof PNG")
    width, height = png_dimensions(path)
    require(
        width >= MIN_PROOF_WIDTH and height >= MIN_PROOF_HEIGHT,
        f"{path}: visual proof resolution too small {width}x{height}",
    )
    require(path.stat().st_size > 10_000, f"{path}: visual proof PNG too small")


def require_text(text: str, fragment: str, path: Path, label: str) -> None:
    require(fragment in text, f"{path}: missing {label}: {fragment}")


@dataclass(frozen=True)
class CaseMetrics:
    max_abs_gap: float
    lowest_toe_z: float
    pelvis_to_lowest_toe_z: float


def require_lower_chain_sanity(
    log_path: Path,
    case: ProofCase,
    visible_rows: dict[tuple[str, str, str], object],
) -> tuple[float, float]:
    def world(bone: str) -> tuple[float, float, float]:
        row = visible_rows.get((case.character, "lower-output", bone))
        require(row is not None, f"{log_path}: missing visible lower-output row for {bone}")
        return row.world

    pelvis_z = world("bone_pelvis")[2]
    toe_z_values = []
    for side in ("L", "R"):
        thigh_z = world(f"bone_{side}-thigh")[2]
        knee_z = world(f"bone_{side}-knee")[2]
        ankle_z = world(f"bone_{side}-ankle")[2]
        toe_z = world(f"bone_{side}-toe")[2]
        toe_z_values.append(toe_z)
        require(
            knee_z <= thigh_z - MIN_CHAIN_Z_DROP,
            f"{log_path}: {side} knee is not below thigh enough "
            f"({knee_z:.4f} vs {thigh_z:.4f})",
        )
        require(
            ankle_z <= knee_z - MIN_CHAIN_Z_DROP,
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
        lowest_toe_z <= MAX_LOWEST_TOE_Z,
        f"{log_path}: lowest toe z {lowest_toe_z:.4f} > {MAX_LOWEST_TOE_Z:.4f}",
    )
    require(
        pelvis_to_lowest_toe_z >= MIN_PELVIS_TO_LOWEST_TOE_Z,
        f"{log_path}: pelvis/toe drop {pelvis_to_lowest_toe_z:.4f} "
        f"< {MIN_PELVIS_TO_LOWEST_TOE_Z:.4f}",
    )
    return lowest_toe_z, pelvis_to_lowest_toe_z


def check_case(root: Path, case: ProofCase) -> CaseMetrics:
    log_path = root / case.log_name
    png_path = root / case.png_name
    require(log_path.is_file(), f"{case.character} {case.view}: missing log {log_path}")
    require_png(png_path)

    text = read_log_text(log_path)
    for marker in FORBIDDEN_LOG_MARKERS:
        require(marker not in text, f"{log_path}: forbidden marker {marker}")

    require_text(text, f"[char] loaded '{case.character}'", log_path, "character load")
    require_text(
        text,
        f"[clip] 'stand_fast_03' from {case.source_clip}",
        log_path,
        "source clip",
    )
    require_text(text, "[char] reference base enabled", log_path, "reference base")
    require_text(text, "[char] clip-frame override enabled: 70", log_path, "frame")
    require_text(text, "[char3d] prop 'xplorer' attached", log_path, "prop attach")
    require_text(text, "[ghogx] screenshot saved:", log_path, "screenshot marker")

    rows, visible_rows, screenshot_line = parse_output_rows(log_path, True)
    require(screenshot_line is not None, f"{log_path}: missing screenshot line")

    max_abs_gap = 0.0
    for bone in LOWER_BODY_BONES:
        row = rows.get(bone)
        require(row is not None, f"{log_path}: missing output row {bone}")
        require(row.driven, f"{log_path}: output row {bone} is not driven")
        require(row.live, f"{log_path}: output row {bone} is not live")
        output_pose = row.vectors.get("outPoseW")
        require(output_pose is not None, f"{log_path}: output row {bone} missing outPoseW")
        visible = visible_rows.get((case.character, "lower-output", bone))
        require(
            visible is not None,
            f"{log_path}: missing visible lower-output row for {bone}",
        )
        require(
            max(abs(component) for component in visible.world) <= MAX_REASONABLE_WORLD_ABS,
            f"{log_path}: visible row {bone} is outside expected full-body bounds",
        )
        gap = tuple(visible.world[i] - output_pose[i] for i in range(3))
        max_abs_gap = max(max_abs_gap, *(abs(component) for component in gap))

    require(
        max_abs_gap <= 0.001,
        f"{log_path}: lower-output/visible gap {max_abs_gap:.6f} > 0.001",
    )
    lowest_toe_z, pelvis_to_lowest_toe_z = require_lower_chain_sanity(
        log_path, case, visible_rows
    )
    return CaseMetrics(
        max_abs_gap=max_abs_gap,
        lowest_toe_z=lowest_toe_z,
        pelvis_to_lowest_toe_z=pelvis_to_lowest_toe_z,
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Check the individual full-body glam1 and metal1 lower-body "
            "proof screenshots and their source-authored live output rows."
        )
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=Path("engine/out/visual_proofs/lower_body_glam1_metal1_20260715"),
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        metrics = [check_case(args.root, case) for case in PROOF_CASES]
    except RuntimeError as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        return 1

    max_gap = max(item.max_abs_gap for item in metrics)
    max_lowest_toe_z = max(item.lowest_toe_z for item in metrics)
    min_pelvis_to_lowest_toe_z = min(item.pelvis_to_lowest_toe_z for item in metrics)
    characters = ",".join(sorted({case.character for case in PROOF_CASES}))
    print(
        "PASS lower_body_glam1_metal1_followup_proofs "
        f"cases={len(PROOF_CASES)} "
        f"characters={characters} "
        f"max_lower_output_visible_gap={max_gap:.6f} "
        f"max_lowest_toe_z={max_lowest_toe_z:.4f} "
        f"min_pelvis_to_lowest_toe_z={min_pelvis_to_lowest_toe_z:.4f} "
        f"proof_min_resolution={MIN_PROOF_WIDTH}x{MIN_PROOF_HEIGHT} "
        "live_output_rows=true direct_source_publisher_absent=true"
        " standing_chain_sane=true"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
