#!/usr/bin/env python3
"""Verify the current-build in-game lower-body screenshot proof batch."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import sys


FORBIDDEN_LOG_MARKERS = (
    "ARK error",
    "not supported",
    "startup checkerboard",
)

COMMON_REQUIRED_LOG_MARKERS = (
    "[ghogx] diagnostic venue override: small2",
    "[ghogx] diagnostic guitar override: xplorer",
    "[ghogx] diagnostic camera shot: flr_near_rt01",
    "[world] diagnostic venue override: small2 -> small2",
    "[world] diagnostic camera shot selected: flr_near_rt01",
    "[diagnostic-highway] hidden mode=over_scene",
    "[diagnostic-hud] GHOGX_HIDE_HUD active; skipping HUD draw",
    "source_publisher=fenced",
    "missing=CharBones::ScaleAdd|CharBonesSamples::EvaluateChannel|"
    "CharBonesMeshes::PoseMeshes",
    "layers_used=0:stand_fast_03",
    "[ghogx] screenshot saved:",
)


@dataclass(frozen=True)
class ProofCase:
    character: str
    log: str
    png: str
    source_clip: str


PROOF_CASES = (
    ProofCase(
        character="rockabill2",
        log=(
            "rockabill2/"
            "ingame_rockabill2_t060_flr_near_rt01_leg_f8_commit005c86f.log"
        ),
        png=(
            "rockabill2/"
            "ingame_rockabill2_t060_flr_near_rt01_leg_f8_commit005c86f.png"
        ),
        source_clip="char/rockabill1/anims/gen/rockabill1_main.milo_ps2",
    ),
    ProofCase(
        character="rock2",
        log="rock2/ingame_rock2_t060_flr_near_rt01_leg_f8_commit005c86f.log",
        png="rock2/ingame_rock2_t060_flr_near_rt01_leg_f8_commit005c86f.png",
        source_clip="char/rock1/anims/gen/rock1_main.milo_ps2",
    ),
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def check_case(root: Path, case: ProofCase) -> None:
    log_path = root / case.log
    png_path = root / case.png
    require(log_path.is_file(), f"{case.character}: missing log {log_path}")
    require(png_path.is_file(), f"{case.character}: missing PNG {png_path}")
    require(png_path.stat().st_size > 10_000, f"{case.character}: PNG too small")

    text = log_path.read_text(encoding="utf-8", errors="replace")
    for marker in FORBIDDEN_LOG_MARKERS:
        require(marker not in text, f"{case.character}: forbidden log marker {marker}")
    for marker in COMMON_REQUIRED_LOG_MARKERS:
        require(marker in text, f"{case.character}: missing log marker {marker}")

    require(
        f"[ghogx] diagnostic character override: {case.character}" in text,
        f"{case.character}: missing app character override",
    )
    require(
        f"[world] diagnostic character override: glam1 -> {case.character}" in text,
        f"{case.character}: missing world character override",
    )
    require(
        f"char={case.character} group=normal index=2 clip=stand_fast_03" in text,
        f"{case.character}: missing source-selected stand_fast_03 row",
    )
    require(
        f"[clip] 'stand_fast_03' from {case.source_clip}" in text,
        f"{case.character}: missing source clip path {case.source_clip}",
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Verify the commit-005c86f in-game lower-body proof screenshots "
            "and their source-backed capture logs."
        )
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=Path("engine/out/visual_proofs/lower_body_current_commit_20260715"),
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        for case in PROOF_CASES:
            check_case(args.root, case)
    except RuntimeError as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        return 1
    print(
        "PASS lower_body_current_commit_proofs "
        f"ingame_cases={len(PROOF_CASES)} "
        "hud_hidden=true highway_hidden=true source_publisher_fenced=true"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
