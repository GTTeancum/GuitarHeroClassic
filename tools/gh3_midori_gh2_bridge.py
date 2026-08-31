#!/usr/bin/env python3
"""Build the GH2-facing bridge manifest for the decoded GH3 Midori IR."""

from __future__ import annotations

import argparse
import gzip
import json
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any

from gh3_midori_bone_names import checksum_name_map, resolved_bone_name


GH3_PS2_MESH_TO_GH2_SCALE = 72.0 / 467.25
# NXTools names this TRANS_DIVISOR_GH3PLAT. PS2 skin positions and SKE/SKA
# translations differ by this fixed-point unit bridge.
GH3_PS2_SKELETON_TO_MESH_UNITS = 256.0
GH3_PS2_SKELETON_TO_GH2_SCALE = (
    GH3_PS2_MESH_TO_GH2_SCALE * GH3_PS2_SKELETON_TO_MESH_UNITS
)


TARGET_PACKAGES = {
    "guitar-main": "char/gh3_midori/anims/gen/gh3_midori_main.milo_ps2",
    "guitar-ui": "char/gh3_midori/anims/gen/gh3_midori_ui.milo_ps2",
    "guitar-strum": "char/gh3_midori/anims/gen/gh3_midori_strum.milo_ps2",
    "guitar-fret": "char/gh3_midori/anims/gen/gh3_midori_fret.milo_ps2",
}

GH3_TO_GH2_BONES = {
    "Bone_Pelvis": "bone_pelvis",
    "Bone_Stomach_Lower": "bone_spine1",
    "Bone_Stomach_Upper": "bone_spine2",
    "Bone_Chest": "bone_spine3",
    "Bone_Neck": "bone_neck",
    "Bone_Head": "bone_head",
    "Bone_Collar_L": "bone_L-clavicle",
    "Bone_Bicep_L": "bone_L-upperArm",
    "Bone_Forearm_L": "bone_L-foreArm",
    "Bone_Palm_L": "bone_L-hand",
    "Bone_Collar_R": "bone_R-clavicle",
    "Bone_Bicep_R": "bone_R-upperArm",
    "Bone_Forearm_R": "bone_R-foreArm",
    "Bone_Palm_R": "bone_R-hand",
    "Bone_Thigh_L": "bone_L-thigh",
    "Bone_Knee_L": "bone_L-knee",
    "Bone_Ankle_L": "bone_L-ankle",
    "Bone_Toe_L": "bone_L-toe",
    "Bone_Thigh_R": "bone_R-thigh",
    "Bone_Knee_R": "bone_R-knee",
    "Bone_Ankle_R": "bone_R-ankle",
    "Bone_Toe_R": "bone_R-toe",
    "Bone_Hand_Index_Base_L": "bone_L-index01",
    "Bone_Hand_Index_Mid_L": "bone_L-index02",
    "Bone_Hand_Index_Top_L": "bone_L-index03",
    "Bone_Hand_Index_Base_R": "bone_R-index01",
    "Bone_Hand_Index_Mid_R": "bone_R-index02",
    "Bone_Hand_Index_Top_R": "bone_R-index03",
    "Bone_Hand_Middle_Base_L": "bone_L-middlefinger01",
    "Bone_Hand_Middle_Mid_L": "bone_L-middlefinger02",
    "Bone_Hand_Middle_Top_L": "bone_L-middlefinger03",
    "Bone_Hand_Middle_Base_R": "bone_R-middlefinger01",
    "Bone_Hand_Middle_Mid_R": "bone_R-middlefinger02",
    "Bone_Hand_Middle_Top_R": "bone_R-middlefinger03",
    "Bone_Hand_Ring_Base_L": "bone_L-ringfinger01",
    "Bone_Hand_Ring_Mid_L": "bone_L-ringfinger02",
    "Bone_Hand_Ring_Top_L": "bone_L-ringfinger03",
    "Bone_Hand_Ring_Base_R": "bone_R-ringfinger01",
    "Bone_Hand_Ring_Mid_R": "bone_R-ringfinger02",
    "Bone_Hand_Ring_Top_R": "bone_R-ringfinger03",
    "Bone_Hand_Pinkey_Base_L": "bone_L-pinky01",
    "Bone_Hand_Pinkey_Mid_L": "bone_L-pinky02",
    "Bone_Hand_Pinkey_Top_L": "bone_L-pinky03",
    "Bone_Hand_Pinkey_Base_R": "bone_R-pinky01",
    "Bone_Hand_Pinkey_Mid_R": "bone_R-pinky02",
    "Bone_Hand_Pinkey_Top_R": "bone_R-pinky03",
    "Bone_Hand_Thumb_Base_L": "bone_L-thumb01",
    "Bone_Hand_Thumb_Mid_L": "bone_L-thumb02",
    "Bone_Hand_Thumb_Top_L": "bone_L-thumb03",
    "Bone_Hand_Thumb_Base_R": "bone_R-thumb01",
    "Bone_Hand_Thumb_Mid_R": "bone_R-thumb02",
    "Bone_Hand_Thumb_Top_R": "bone_R-thumb03",
    "Bone_Jaw": "bone_head",
    "Bone_Eye_L": "bone_head",
    "Bone_Eye_R": "bone_head",
    "Bone_Eyelid_Upper_L": "bone_head",
    "Bone_Eyelid_Upper_R": "bone_head",
    "Bone_Mouth_L": "bone_head",
    "Bone_Mouth_R": "bone_head",
    "Bone_Lip_Lower_Mid": "bone_head",
    "Bone_Lip_Upper_Mid": "bone_head",
    "bone_guitar_body": "bone_pos_guitar",
    "bone_ik_hand_guitar_l": "bone_fret_hand",
    "bone_ik_hand_guitar_r": "bone_strum_hand",
}

GH3_PARENT_BONES = {
    "Control_Root": "",
    "Bone_Pelvis": "Control_Root",
    "Bone_Stomach_Lower": "Bone_Pelvis",
    "Bone_Stomach_Upper": "Bone_Stomach_Lower",
    "Bone_Chest": "Bone_Stomach_Upper",
    "Bone_Neck": "Bone_Chest",
    "Bone_Head": "Bone_Neck",
    "Bone_Collar_L": "Bone_Chest",
    "Bone_Bicep_L": "Bone_Collar_L",
    "Bone_Forearm_L": "Bone_Bicep_L",
    "Bone_Palm_L": "Bone_Forearm_L",
    "Bone_Collar_R": "Bone_Chest",
    "Bone_Bicep_R": "Bone_Collar_R",
    "Bone_Forearm_R": "Bone_Bicep_R",
    "Bone_Palm_R": "Bone_Forearm_R",
    "Bone_Thigh_L": "Bone_Pelvis",
    "Bone_Knee_L": "Bone_Thigh_L",
    "Bone_Ankle_L": "Bone_Knee_L",
    "Bone_Toe_L": "Bone_Ankle_L",
    "Bone_Thigh_R": "Bone_Pelvis",
    "Bone_Knee_R": "Bone_Thigh_R",
    "Bone_Ankle_R": "Bone_Knee_R",
    "Bone_Toe_R": "Bone_Ankle_R",
    "Bone_Twist_Bicep_Top_L": "Bone_Bicep_L",
    "Bone_Twist_Bicep_Top_R": "Bone_Bicep_R",
    "Bone_Twist_Bicep_Mid_L": "Bone_Bicep_L",
    "Bone_Twist_Bicep_Mid_R": "Bone_Bicep_R",
    "Bone_Twist_Wrist_L": "Bone_Forearm_L",
    "Bone_Twist_Wrist_R": "Bone_Forearm_R",
    "Bone_Split_Ass_L": "Bone_Pelvis",
    "Bone_Split_Ass_R": "Bone_Pelvis",
    "Bone_Twist_Thigh_L": "Bone_Thigh_L",
    "Bone_Twist_Thigh_R": "Bone_Thigh_R",
    "Bone_Jaw": "Bone_Head",
    "Bone_Eye_L": "Bone_Head",
    "Bone_Eye_R": "Bone_Head",
    "Bone_Eyelid_Upper_L": "Bone_Head",
    "Bone_Eyelid_Upper_R": "Bone_Head",
    "Bone_Mouth_L": "Bone_Head",
    "Bone_Mouth_R": "Bone_Head",
    "Bone_Lip_Lower_Mid": "Bone_Head",
    "Bone_Lip_Upper_Mid": "Bone_Head",
    "bone_guitar_body": "Bone_Chest",
}

for side, palm in (("L", "Bone_Palm_L"), ("R", "Bone_Palm_R")):
    for finger in ("Index", "Middle", "Ring", "Pinkey"):
        base = f"Bone_Hand_{finger}_Base_{side}"
        mid = f"Bone_Hand_{finger}_Mid_{side}"
        top = f"Bone_Hand_{finger}_Top_{side}"
        GH3_PARENT_BONES[base] = palm
        GH3_PARENT_BONES[mid] = base
        GH3_PARENT_BONES[top] = mid
    thumb_base = f"Bone_Hand_Thumb_Base_{side}"
    thumb_mid = f"Bone_Hand_Thumb_Mid_{side}"
    thumb_top = f"Bone_Hand_Thumb_Top_{side}"
    GH3_PARENT_BONES[thumb_base] = palm
    GH3_PARENT_BONES[thumb_mid] = thumb_base
    GH3_PARENT_BONES[thumb_top] = thumb_mid
GH3_PARENT_BONES["Bone_Hand_Middle_Top_R"] = "Bone_Hand_Middle_Mid_R"

PASSTHROUGH_BONE_REASON = (
    "GH3-only skeleton bone preserved under its source name; generated Midori "
    "models carry matching Trans objects for ark-external animation binding"
)

FRET_ALIAS_PLAN = {
    "finger_open": "gh3_hnd_guit_chord_mid_empty_d",
    "finger_chord_bar": "gh3_hnd_guit_chord_mid_bar1_d",
    "finger_powerchord_1": "gh3_hnd_guit_chord_mid_bar2_d",
    "finger_powerchord_2": "gh3_hnd_guit_chord_mid_bar3_d",
    "finger_hold_index": "gh3_hnd_guit_chord_mid_bar1_d",
    "finger_hold_middle": "gh3_hnd_guit_chord_mid_bar2_d",
    "finger_hold_ring": "gh3_hnd_guit_chord_mid_bar3_d",
    "finger_hold_pinky": "gh3_hnd_guit_chord_mid_bar4_d",
    "finger_hold_index_hi": "gh3_hnd_guit_chord_mid_bar1_d",
    "finger_hold_middle_hi": "gh3_hnd_guit_chord_mid_bar2_d",
    "finger_hold_ring_hi": "gh3_hnd_guit_chord_mid_bar3_d",
    "finger_hold_pinky_hi": "gh3_hnd_guit_chord_mid_bar5_d",
    "finger_vibrato_index": "gh3_hnd_guit_chord_mid_bar1_d",
    "finger_vibrato_middle": "gh3_hnd_guit_chord_mid_bar2_d",
    "finger_vibrato_ring": "gh3_hnd_guit_chord_mid_bar3_d",
    "finger_vibrato_pinky": "gh3_hnd_guit_chord_mid_bar4_d",
    "finger_vibrato_index_hi": "gh3_hnd_guit_chord_mid_bar1_d",
    "finger_vibrato_middle_hi": "gh3_hnd_guit_chord_mid_bar2_d",
    "finger_vibrato_ring_hi": "gh3_hnd_guit_chord_mid_bar3_d",
    "finger_vibrato_pinky_hi": "gh3_hnd_guit_chord_mid_bar5_d",
    "finger_chord_dmajor": "gh3_hnd_guit_chord_mid_bar3_d",
}

STRUM_ALIAS_PLAN = {
    "strum_open": "gh3_hnd_guit_strum_mido_nostrum_d",
    "strum_short_01": "gh3_hnd_guit_strum_mido_norm_s01_d",
    "strum_short_02": "gh3_hnd_guit_strum_mido_norm_s02_d",
    "strum_short_03": "gh3_hnd_guit_strum_mido_norm_s03_d",
    "strum_short_04": "gh3_hnd_guit_strum_mido_norm_s04_d",
    "strum_long_01": "gh3_hnd_guit_strum_mido_norm_l01_d",
    "strum_long_02": "gh3_hnd_guit_strum_mido_norm_l02_d",
    "strum_long_03": "gh3_hnd_guit_strum_mido_norm_l01_d",
    "strum_long_04": "gh3_hnd_guit_strum_mido_norm_l02_d",
    "strum_long": "gh3_hnd_guit_strum_mido_norm_l01_d",
    "strum_pick_01": "gh3_hnd_guit_strum_mido_norm_m01_d",
    "strum_pick_02": "gh3_hnd_guit_strum_mido_norm_m02_d",
}

UI_ALIAS_PLAN = {
    "ui_loop": "gh3_guit_midori_tran_out_idle",
    "ui_enter": "gh3_guit_midori_a_intro2",
}


def stem(path: str) -> str:
    name = Path(path).name
    return name[:-8] if name.endswith(".ska.ps2") else Path(name).stem


def classify_clip(path: str, name: str) -> str:
    if "/hands/" in path and "strum" in name:
        return "guitar-strum"
    if "/hands/" in path:
        return "guitar-fret"
    if "/frontend/" in path:
        return "guitar-ui"
    return "guitar-main"


def target_bone_name(source_name: str | None) -> str | None:
    if not source_name:
        return None
    return GH3_TO_GH2_BONES.get(source_name, source_name)


def source_parent_name(source_name: str | None) -> str | None:
    if not source_name:
        return None
    return GH3_PARENT_BONES.get(source_name)


def channel_targets(bone: dict[str, Any], name_by_checksum: dict[str, str]) -> list[str]:
    if not bone.get("partial_flag_allowed", True):
        return []
    name = resolved_bone_name(bone, name_by_checksum)
    target = target_bone_name(name)
    if not target:
        return []
    channels = []
    if bone.get("quat_key_count", 0):
        channels.append(target + ".quat")
    if bone.get("trans_key_count", 0):
        channels.append(target + ".pos")
    return channels


def load_clips(source_root: Path, source_manifest: dict[str, Any]) -> list[dict[str, Any]]:
    rel = source_manifest["animations"]["ska_ir"]["relative_path"]
    path = source_root / rel
    clips = []
    with gzip.open(path, "rt", encoding="utf-8") as handle:
        for line in handle:
            clips.append(json.loads(line))
    return clips


def build_bone_map(source_manifest: dict[str, Any]) -> list[dict[str, Any]]:
    rows = []
    skeleton_bones = source_manifest["skeleton"]["bones"]
    name_by_checksum = checksum_name_map(skeleton_bones)
    for bone in skeleton_bones:
        name = resolved_bone_name(bone, name_by_checksum)
        target = target_bone_name(name)
        reason = None
        status = "mapped" if target else "unmapped"
        if target and target == name and name not in GH3_TO_GH2_BONES:
            reason = PASSTHROUGH_BONE_REASON
        rows.append({
            "source_index": bone["index"],
            "source_name": name,
            "source_checksum": bone["name_checksum"],
            "target_base": target,
            "status": status,
            "reason": reason,
        })
    return rows


def build_assignments(
    clips: list[dict[str, Any]],
    name_by_checksum: dict[str, str],
) -> tuple[list[dict[str, Any]], dict[str, Any]]:
    assignments = []
    role_counts: Counter[str] = Counter()
    role_key_counts: dict[str, Counter[str]] = defaultdict(Counter)
    unmapped_channels: Counter[str] = Counter()
    mapped_channels: dict[str, set[str]] = defaultdict(set)

    for clip in clips:
        name = stem(clip["path"])
        role = classify_clip(clip["path"], name)
        role_counts[role] += 1
        role_key_counts[role]["quat"] += clip["quat_key_count"]
        role_key_counts[role]["trans"] += clip["trans_key_count"]

        channels = []
        unmapped_bones = []
        for bone in clip["bones"]:
            targets = channel_targets(bone, name_by_checksum)
            if targets:
                channels.extend(targets)
                mapped_channels[role].update(targets)
            else:
                unmapped_bones.append({
                    "index": bone["index"],
                    "name": resolved_bone_name(bone, name_by_checksum),
                    "quat_key_count": bone["quat_key_count"],
                    "trans_key_count": bone["trans_key_count"],
                })
                unmapped_channels[role] += int(bool(bone["quat_key_count"])) + int(bool(bone["trans_key_count"]))

        assignments.append({
            "source_path": clip["path"],
            "source_clip": name,
            "target_role": role,
            "target_package": TARGET_PACKAGES[role],
            "target_clip": name,
            "duration_seconds": clip["header"]["duration_seconds"],
            "animated_bone_count": clip["animated_bone_count"],
            "mapped_channel_count": len(set(channels)),
            "unmapped_bone_count": len(unmapped_bones),
            "unmapped_bones": unmapped_bones[:12],
        })

    summary = {
        "role_clip_counts": dict(sorted(role_counts.items())),
        "role_key_counts": {
            role: dict(counts) for role, counts in sorted(role_key_counts.items())
        },
        "role_mapped_channel_counts": {
            role: len(channels) for role, channels in sorted(mapped_channels.items())
        },
        "role_unmapped_channel_mentions": dict(sorted(unmapped_channels.items())),
    }
    return assignments, summary


def alias_rows(plan: dict[str, str], available: set[str], role: str) -> list[dict[str, Any]]:
    return [
        {
            "target_clip": target,
            "source_clip": source,
            "target_role": role,
            "target_package": TARGET_PACKAGES[role],
            "source_available": source in available,
            "status": "draft_requires_visual_validation",
        }
        for target, source in plan.items()
    ]


def build_dlc_manifest() -> dict[str, Any]:
    outfit_paths = [
        {
            "selection": "gh3_midori_1",
            "label": "Outfit 1",
            "model": "char/gh3_midori_1/og/gen/gh3_midori_1.milo_ps2",
            "ui_model": "char/gh3_midori_1/og/gen/gh3_midori_1.milo_ps2",
            "ui_anim": TARGET_PACKAGES["guitar-ui"],
            "main_anim": TARGET_PACKAGES["guitar-main"],
            "strum_anim": TARGET_PACKAGES["guitar-strum"],
            "fret_anim": TARGET_PACKAGES["guitar-fret"],
        },
        {
            "selection": "gh3_midori_2",
            "label": "Outfit 2",
            "model": "char/gh3_midori_2/og/gen/gh3_midori_2.milo_ps2",
            "ui_model": "char/gh3_midori_2/og/gen/gh3_midori_2.milo_ps2",
            "ui_anim": TARGET_PACKAGES["guitar-ui"],
            "main_anim": TARGET_PACKAGES["guitar-main"],
            "strum_anim": TARGET_PACKAGES["guitar-strum"],
            "fret_anim": TARGET_PACKAGES["guitar-fret"],
        },
    ]
    return {
        "schema_version": 1,
        "id": "community.gh3.midori",
        "name": "GH3 Midori",
        "version": "0.1.0",
        "content_root": "content",
        "characters": [{
            "id": "gh3_midori",
            "label": "Midori",
            "outfits": outfit_paths,
        }],
    }


def bridge_outfit_paths(dlc_manifest: dict[str, Any]) -> list[dict[str, Any]]:
    return [
        {
            key: value
            for key, value in outfit.items()
            if key != "label"
        }
        for outfit in dlc_manifest["characters"][0]["outfits"]
    ]


def command_build(args: argparse.Namespace) -> int:
    source_root = args.source_root
    source_manifest = json.loads((source_root / "midori_source_ir_manifest.json").read_text())
    clips = load_clips(source_root, source_manifest)
    name_by_checksum = checksum_name_map(source_manifest["skeleton"]["bones"])
    assignments, assignment_summary = build_assignments(clips, name_by_checksum)
    available = {item["source_clip"] for item in assignments}
    bone_map = build_bone_map(source_manifest)
    dlc_manifest = build_dlc_manifest()
    (source_root / "gh2_dlc_manifest.draft.json").write_text(
        json.dumps(dlc_manifest, indent=2) + "\n",
        encoding="utf-8",
    )
    source_manifest.setdefault("gh2_destination", {})
    source_manifest["gh2_destination"]["package_id"] = "community.gh3.midori"
    source_manifest["gh2_destination"]["content_root"] = "content"
    source_manifest["gh2_destination"]["outfit_paths"] = bridge_outfit_paths(dlc_manifest)
    source_manifest["gh2_bridge_manifest"] = args.output.as_posix()
    (source_root / "midori_source_ir_manifest.json").write_text(
        json.dumps(source_manifest, indent=2) + "\n",
        encoding="utf-8",
    )

    mapped_bones = sum(1 for row in bone_map if row["status"] == "mapped")
    payload = {
        "format": "gh3_midori_gh2_bridge_manifest_v1",
        "source_ir": str(source_root),
        "character": "gh3_midori",
        "destination_package": "community.gh3.midori",
        "destination_packages": TARGET_PACKAGES,
        "dlc_manifest_draft": (source_root / "gh2_dlc_manifest.draft.json").as_posix(),
        "writer_contract": {
            "milo_directory_version": 24,
            "root_type": "CharClipSet",
            "clip_type": "CharClipSamples",
            "clip_revision": 10,
            "char_clip_revision": 5,
            "channel_suffixes": [".pos", ".quat"],
            "source_reference": "tools/milo_object",
        },
        "bone_map_summary": {
            "source_bones": len(bone_map),
            "mapped_bones": mapped_bones,
            "unmapped_or_extra_bones": len(bone_map) - mapped_bones,
        },
        "bone_map": bone_map,
        "clip_assignment_summary": assignment_summary,
        "clip_assignments": assignments,
        "alias_plan": {
            "guitar-ui": alias_rows(UI_ALIAS_PLAN, available, "guitar-ui"),
            "guitar-fret": alias_rows(FRET_ALIAS_PLAN, available, "guitar-fret"),
            "guitar-strum": alias_rows(STRUM_ALIAS_PLAN, available, "guitar-strum"),
        },
        "open_writer_tasks": [
            "Convert mapped WXYZ quaternion and position channels into GH2 CharBonesSamples10 sample_bytes.",
            "Keep GH3-only face/accessory/twist/control bones as source-name Trans targets and validate all clips against both generated Midori models.",
            "Validate draft hand/UI aliases in-game and promote or replace them with better clip choices.",
            "Build GH2 model MILOs for both Midori outfits from the mesh/texture/skeleton IR.",
        ],
    }
    text = json.dumps(payload, indent=2)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(text + "\n", encoding="utf-8")
    if args.print_summary:
        print(
            "clips=%d mapped_bones=%d roles=%s"
            % (
                len(assignments),
                mapped_bones,
                ",".join(
                    f"{key}:{value}"
                    for key, value in assignment_summary["role_clip_counts"].items()
                ),
            )
        )
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source-root", type=Path, default=Path("out/midori/input/source_ir")
    )
    parser.add_argument(
        "--output", type=Path, default=Path("out/midori/gh2_bridge_manifest.json")
    )
    parser.add_argument("--print-summary", action="store_true")
    parser.set_defaults(func=command_build)
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
