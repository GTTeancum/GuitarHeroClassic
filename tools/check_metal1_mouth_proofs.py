#!/usr/bin/env python3
"""Validate the focused Metal1 mouth proof slice.

This checker intentionally proves evidence boundaries rather than visual
correctness.  GH2 PS2 stock character MILOs expose FaceFxLipSyncServo rows; the
available ihatecompvir source exposes CharFaceServo, not the GH2-specific
FaceFxLipSyncServo runtime bridge.
"""

from __future__ import annotations

import argparse
from pathlib import Path


REQUIRED_IMAGES = (
    "metal1_mouth_current_front_face.png",
    "metal1_mouth_current_three_quarter_face.png",
    "metal1_mouth_faceclip_none_front.png",
    "metal1_mouth_reference_base_front.png",
)


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--proof-dir",
        default=(
            "engine/out/visual_proofs/metal1_mouth_20260715"
        ),
        help="Directory containing Metal1 mouth screenshots and logs.",
    )
    args = parser.parse_args()

    proof_dir = Path(args.proof_dir)
    require(proof_dir.is_dir(), f"missing proof dir: {proof_dir}")

    for name in REQUIRED_IMAGES:
        image = proof_dir / name
        require(image.is_file(), f"missing proof image: {image}")
        require(
            image.stat().st_size > 1_000_000,
            f"proof image too small to be inspection-grade: {image}",
        )

    current = read_text(proof_dir / "metal1_mouth_current_front_face.log")
    face_none = read_text(proof_dir / "metal1_mouth_faceclip_none_front.log")
    reference = read_text(proof_dir / "metal1_mouth_reference_base_front.log")

    require(
        "[facefx-servo] stock FaceFxLipSyncServo rows=1" in current,
        "current proof does not log the stock FaceFxLipSyncServo row count",
    )
    require(
        "boundary=not-CharFaceServo" in current,
        "current proof does not preserve the ihatecompvir source boundary",
    )
    require(
        "viseme_milo=../../anims/metal1_viseme.milo" in current,
        "current proof does not log Metal1's authored viseme MILO reference",
    )
    require(
        "[clip] 'neutral' from char/metal1/anims/gen/metal1_viseme.milo_ps2"
        in current,
        "current proof does not load the authored neutral viseme clip",
    )
    require(
        "[char] face-filtered 'neutral': kept 15/17 channels" in current,
        "current proof does not show the face-channel filter for neutral",
    )
    require(
        "[clip] 'neutral' from" not in face_none,
        "--face-clip none proof still loaded neutral face clip",
    )
    require(
        "[out-map] bone_jaw" in face_none and "driven=0" in face_none,
        "--face-clip none proof does not show undriven jaw output rows",
    )
    require(
        "[facefx-servo] stock FaceFxLipSyncServo rows=1" in reference,
        "reference proof does not log stock FaceFxLipSyncServo row count",
    )

    print(
        "metal1 mouth proof ok: images=4 source_boundary=FaceFxLipSyncServo "
        "neutral_face_clip_logged=true face_clip_none_jaw_driven=false"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
