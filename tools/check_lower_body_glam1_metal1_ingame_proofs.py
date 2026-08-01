#!/usr/bin/env python3
"""Verify the focused Glam1/Metal1 in-game lower-body proof batch."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import struct
import sys


PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
MIN_PROOF_WIDTH = 1280
MIN_PROOF_HEIGHT = 720

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
    "ARK error",
    "not supported",
    "startup checkerboard",
)

COMMON_INGAME_MARKERS = (
    "[ghogx] diagnostic venue override: small2",
    "[ghogx] diagnostic guitar override: xplorer",
    "[ghogx] diagnostic camera shot: flr_near_rt01",
    "[ghogx] diagnostic autoplay enabled",
    "[ghogx] song='trogdor' difficulty=3",
    "[gameplay] diagnostic seek: 60.000s",
    "[world] diagnostic guitar override: lespaul -> xplorer",
    "[world] diagnostic venue override: small2 -> small2",
    "[world] diagnostic camera shot selected: flr_near_rt01",
    "[diagnostic-highway] hidden mode=over_scene",
    "[diagnostic-hud] GHOGX_HIDE_HUD active; skipping HUD draw",
    "[ghogx] screenshot saved:",
    "source_publisher=fenced",
    "missing=CharBones::ScaleAdd|CharBonesSamples::EvaluateChannel|"
    "CharBonesMeshes::PoseMeshes",
)

COMMON_VIEWER_MARKERS = (
    "[char] reference base enabled",
    "[char] midi fret target: spot_neck_fret11.mesh",
    "[ghogx] screenshot saved:",
    "source_publisher=fenced",
)

@dataclass(frozen=True)
class ProofCase:
    character: str
    ingame_log: str
    ingame_png: str
    viewer_log: str
    viewer_png: str
    source_clip: str
    prev_clip: str
    current_clip: str
    active_layer: str


PROOF_CASES = (
    ProofCase(
        character="glam1",
        ingame_log="ingame_glam1_t060_flr_near_rt01.log",
        ingame_png="ingame_glam1_t060_flr_near_rt01.png",
        viewer_log="viewer_glam1_live_stack.log",
        viewer_png="viewer_glam1_live_stack.png",
        source_clip="char/glam1/anims/gen/glam1_main.milo_ps2",
        prev_clip="stand_fast_02",
        current_clip="stand_fast_03",
        active_layer="stand_fast_03",
    ),
    ProofCase(
        character="metal1",
        ingame_log="ingame_metal1_t060_flr_near_rt01.log",
        ingame_png="ingame_metal1_t060_flr_near_rt01.png",
        viewer_log="viewer_metal1_live_stack.log",
        viewer_png="viewer_metal1_live_stack.png",
        source_clip="char/metal1/anims/gen/metal1_main.milo_ps2",
        prev_clip="stand_fast_03",
        current_clip="stand_fast_04",
        active_layer="stand_fast_04",
    ),
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def compact(text: str) -> str:
    return "".join(
        text.replace("\x00", "").replace("ghogx_app.exe : ", "").split()
    )


def png_dimensions(path: Path) -> tuple[int, int]:
    with path.open("rb") as in_file:
        header = in_file.read(24)
    require(header.startswith(PNG_SIGNATURE), f"{path}: not a PNG")
    require(len(header) >= 24, f"{path}: truncated PNG header")
    return struct.unpack(">II", header[16:24])


def require_png(path: Path, label: str) -> None:
    require(path.is_file(), f"{label}: missing PNG {path}")
    require(path.stat().st_size > 10_000, f"{label}: PNG too small")
    width, height = png_dimensions(path)
    require(
        width >= MIN_PROOF_WIDTH and height >= MIN_PROOF_HEIGHT,
        f"{label}: PNG resolution too small {width}x{height}",
    )


def require_text(text: str, fragment: str, path: Path, label: str) -> None:
    require(compact(fragment) in compact(text), f"{path}: missing {label}: {fragment}")


def require_lower_body_rows(text: str, case: ProofCase, path: Path) -> None:
    compact_text = compact(text)
    for bone in LOWER_BODY_BONES:
        require(
            compact(f"[armw] c={case.character} t=post b={bone} w=") in compact_text,
            f"{path}: missing lower-body armw post row for {bone}",
        )


def check_case(root: Path, case: ProofCase) -> None:
    ingame_log = root / case.ingame_log
    ingame_png = root / case.ingame_png
    viewer_log = root / case.viewer_log
    viewer_png = root / case.viewer_png

    require(ingame_log.is_file(), f"{case.character}: missing in-game log")
    require(viewer_log.is_file(), f"{case.character}: missing viewer log")
    require_png(ingame_png, f"{case.character} in-game")
    require_png(viewer_png, f"{case.character} viewer")

    ingame_text = read_text(ingame_log)
    viewer_text = read_text(viewer_log)
    for marker in FORBIDDEN_LOG_MARKERS:
        require(marker not in ingame_text, f"{ingame_log}: forbidden marker {marker}")
        require(marker not in viewer_text, f"{viewer_log}: forbidden marker {marker}")

    for marker in COMMON_INGAME_MARKERS:
        require_text(ingame_text, marker, ingame_log, "in-game marker")
    for marker in COMMON_VIEWER_MARKERS:
        require_text(viewer_text, marker, viewer_log, "viewer marker")

    require_text(
        ingame_text,
        f"[ghogx] diagnostic character override: {case.character}",
        ingame_log,
        "app character override",
    )
    require_text(
        ingame_text,
        f"[world] diagnostic character override: glam1 -> {case.character}",
        ingame_log,
        "world character override",
    )
    require_text(
        ingame_text,
        f"[world] quickplay rig: character={case.character} guitar=xplorer venue=small2",
        ingame_log,
        "quickplay rig",
    )
    require_text(
        ingame_text,
        f"[clip] '{case.current_clip}' from {case.source_clip}",
        ingame_log,
        "current source clip",
    )
    require_text(
        ingame_text,
        f"role=guitarist0 char={case.character} group=normal index=",
        ingame_log,
        "source clip group row",
    )
    require_text(
        ingame_text,
        f"clip={case.current_clip}",
        ingame_log,
        "active clip",
    )
    require_text(
        ingame_text,
        f"layers_used=0:{case.active_layer}",
        ingame_log,
        "active layer",
    )
    require_text(
        viewer_text,
        f"[clip] '{case.current_clip}' from {case.source_clip}",
        viewer_log,
        "viewer current clip",
    )
    require_text(
        viewer_text,
        f"[clip] '{case.prev_clip}' from {case.source_clip}",
        viewer_log,
        "viewer previous clip",
    )
    require_text(
        viewer_text,
        f"[char] viewer stack main: prev={case.prev_clip} current={case.current_clip} blend=0.250 immediate",
        viewer_log,
        "viewer transition",
    )
    require_lower_body_rows(ingame_text, case, ingame_log)
    require_lower_body_rows(viewer_text, case, viewer_log)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Check focused Glam1/Metal1 current in-game lower-body proofs."
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=Path("engine/out/visual_proofs/lower_body_glam1_metal1_ingame_20260715"),
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
        "PASS lower_body_glam1_metal1_ingame_proofs "
        f"cases={len(PROOF_CASES)} "
        "characters=glam1,metal1 "
        f"proof_min_resolution={MIN_PROOF_WIDTH}x{MIN_PROOF_HEIGHT} "
        "hud_hidden=true highway_hidden=true lower_body_rows=true "
        "source_publisher_fenced=true"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
