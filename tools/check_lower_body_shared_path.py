#!/usr/bin/env python3
"""Guard viewer/gameplay lower-body pose routing through the shared clip path."""

from __future__ import annotations

import argparse
from pathlib import Path
import sys


CHAR_CLIP_REQUIRED = (
    (
        "staticvoidapply_lower_body_output_layer(",
        "shared lower-body output bridge function",
    ),
    (
        "apply_clip_pose_output_layer(lower_channels,weight,character,relative,"
        "lower_output_bones,true)",
        "source-output lower-body bridge application",
    ),
    (
        'dump_leg_pose(character,"lower-output")',
        "post-bridge lower-output proof trace",
    ),
    (
        "voidapply_clip_channel_layers(",
        "shared clip-layer pose application",
    ),
    (
        "apply_lower_body_output_layer(frame,1.0f,character,relative,"
        "output_bones);",
        "shared stack fallback lower-body bridge",
    ),
    (
        "apply_lower_body_output_layer(clip.frames[(size_t)fi],1.0f,"
        "character,clip.relative,clip.output_bones);",
        "direct frame fallback lower-body bridge",
    ),
    (
        "apply_lower_body_output_layer(clip.frames[(size_t)fi],weight,"
        "character,clip.relative,clip.output_bones);",
        "weighted frame fallback lower-body bridge",
    ),
    (
        "apply_lower_body_output_layer(frame,weight,character,relative,"
        "current->output_bones);",
        "live player fallback lower-body bridge",
    ),
    (
        "voidapply_clip_layer_stack(",
        "shared pose-stack application helper",
    ),
    (
        "apply_clip_channel_layers(stack.layers,character,stack.relative);",
        "pose stack enters shared clip-layer path",
    ),
    (
        "apply_character_pose_stack_frame(",
        "public shared pose-stack frame entry point",
    ),
    (
        "apply_character_pose_controller_frame(",
        "public shared controller frame entry point",
    ),
)

APP_REQUIRED = (
    (
        "append_character_pose_frame_layers(pose_stack,frame_layers);",
        "viewer frame override enters shared frame-layer builder",
    ),
    (
        "append_character_pose_player_layers(pose_stack,player_layers);",
        "viewer realtime playback enters shared player-layer builder",
    ),
    (
        "controller_sources.pose_stack=&pose_stack;",
        "viewer controller frame receives shared pose stack",
    ),
    (
        "apply_character_pose_controller_frame(renderer.character(),"
        "controller_sources);",
        "viewer uses shared controller-frame application",
    ),
)

GAMEPLAY_REQUIRED = (
    (
        "append_character_pose_player_layers(pose_stack,pose_player_layers);",
        "gameplay performer playback enters shared player-layer builder",
    ),
    (
        "apply_character_pose_stack_frame(character,&pose_stack);",
        "gameplay applies shared pose-stack frame before controllers",
    ),
    (
        "apply_character_pose_controller_frame(character,controller_sources);",
        "gameplay uses shared controller-frame application",
    ),
)

FORBIDDEN_LOCAL_PUBLISHER = (
    "apply_lower_body_output_layer(",
    'dump_leg_pose(character,"lower-output")',
    "lower-output",
)


def compact(text: str) -> str:
    return "".join(text.split())


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def require_contains(block: str, needle: str, label: str) -> None:
    if needle not in block:
        raise RuntimeError(f"missing {label}: {needle}")


def require_absent(block: str, needle: str, label: str) -> None:
    if needle in block:
        raise RuntimeError(f"forbidden {label}: {needle}")


def require_count(block: str, needle: str, expected: int, label: str) -> None:
    got = block.count(needle)
    if got != expected:
        raise RuntimeError(f"{label}: expected {expected}, got {got}")


def check_viewer_app(text: str) -> None:
    body = compact(text)
    for needle, label in APP_REQUIRED:
        require_contains(body, needle, label)
    for needle in FORBIDDEN_LOCAL_PUBLISHER:
        require_absent(body, needle, "viewer local lower-body publisher")
    require_count(
        body,
        "is_lower_body_clip_name(",
        3,
        "viewer lower-body classifier use count",
    )
    require_contains(
        body,
        "voidkeep_hand_overlay_channels_only(",
        "viewer hand-overlay lower-body stripping boundary",
    )
    require_contains(
        body,
        "returnis_lower_body_clip_name(ch.bone_name);",
        "viewer strips lower-body channels only from hand overlays",
    )
    require_contains(
        body,
        "returnis_lower_body_clip_name(bone.name);",
        "viewer strips lower-body output bones only from hand overlays",
    )


def check_gameplay(text: str) -> None:
    body = compact(text)
    for needle, label in GAMEPLAY_REQUIRED:
        require_contains(body, needle, label)
    for needle in FORBIDDEN_LOCAL_PUBLISHER:
        require_absent(body, needle, "gameplay local lower-body publisher")
    require_count(
        body,
        "is_lower_body_clip_channel(",
        3,
        "gameplay lower-body classifier use count",
    )
    require_contains(
        body,
        "voidkeep_hand_overlay_channels(",
        "gameplay hand-overlay lower-body stripping boundary",
    )
    require_contains(
        body,
        "returnis_lower_body_clip_channel(ch.bone_name);",
        "gameplay strips lower-body channels only from hand overlays",
    )
    require_contains(
        body,
        "returnis_lower_body_clip_channel(bone.name);",
        "gameplay strips lower-body output bones only from hand overlays",
    )


def check_char_clip(text: str) -> None:
    body = compact(text)
    for needle, label in CHAR_CLIP_REQUIRED:
        require_contains(body, needle, label)
    require_count(
        body,
        "apply_lower_body_output_layer(",
        5,
        "char_clip lower-body bridge definition plus call-site count",
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Check that viewer and gameplay lower-body poses enter the same "
            "shared character clip path, and that app-local lower-body filters "
            "only strip hand-overlay rows."
        )
    )
    parser.add_argument(
        "--char-clip",
        type=Path,
        default=Path("engine/src/character/char_clip.cpp"),
    )
    parser.add_argument(
        "--viewer",
        type=Path,
        default=Path("engine/src/app/app_main.cpp"),
    )
    parser.add_argument(
        "--gameplay",
        type=Path,
        default=Path("engine/src/game/gameplay.cpp"),
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        check_char_clip(read(args.char_clip))
        check_viewer_app(read(args.viewer))
        check_gameplay(read(args.gameplay))
    except RuntimeError as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        return 1
    print(
        "PASS lower_body_shared_path "
        "viewer_shared=true gameplay_shared=true "
        "fallback_call_sites=4 local_filters=hand_overlay_only"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
