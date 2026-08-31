"""Checksum-backed GH3 Midori bone-name resolution.

Neversoft skeleton tables store QB-style checksums. NXTools' stock
BoneList_GHWTRocker.ghbones is useful community context, but Midori's PS2
skeleton cannot safely be named by positional index alone. Prefer direct
checksum matches and use GH2-parser-compatible checksum-stable fallback names
for unknown extras.
"""

from __future__ import annotations

import zlib
from typing import Any


SUPPLEMENTAL_GHWT_BONE_NAMES = (
    "Control_Root",
    "Bone_Pelvis",
    "Bone_Stomach_Lower",
    "Bone_Stomach_Upper",
    "Bone_Chest",
    "Bone_Neck",
    "Bone_Head",
    "Bone_Collar_L",
    "Bone_Bicep_L",
    "Bone_Forearm_L",
    "Bone_Palm_L",
    "Bone_Collar_R",
    "Bone_Bicep_R",
    "Bone_Forearm_R",
    "Bone_Palm_R",
    "Bone_Thigh_L",
    "Bone_Knee_L",
    "Bone_Ankle_L",
    "Bone_Toe_L",
    "Bone_Thigh_R",
    "Bone_Knee_R",
    "Bone_Ankle_R",
    "Bone_Toe_R",
    "Bone_Hand_Index_Base_L",
    "Bone_Hand_Index_Mid_L",
    "Bone_Hand_Index_Top_L",
    "Bone_Hand_Index_Base_R",
    "Bone_Hand_Index_Mid_R",
    "Bone_Hand_Index_Top_R",
    "Bone_Hand_Middle_Base_L",
    "Bone_Hand_Middle_Mid_L",
    "Bone_Hand_Middle_Top_L",
    "Bone_Hand_Middle_Base_R",
    "Bone_Hand_Middle_Mid_R",
    "Bone_Hand_Middle_Top_R",
    "Bone_Hand_Ring_Base_L",
    "Bone_Hand_Ring_Mid_L",
    "Bone_Hand_Ring_Top_L",
    "Bone_Hand_Ring_Base_R",
    "Bone_Hand_Ring_Mid_R",
    "Bone_Hand_Ring_Top_R",
    "Bone_Hand_Pinkey_Base_L",
    "Bone_Hand_Pinkey_Mid_L",
    "Bone_Hand_Pinkey_Top_L",
    "Bone_Hand_Pinkey_Base_R",
    "Bone_Hand_Pinkey_Mid_R",
    "Bone_Hand_Pinkey_Top_R",
    "Bone_Hand_Thumb_Base_L",
    "Bone_Hand_Thumb_Mid_L",
    "Bone_Hand_Thumb_Top_L",
    "Bone_Hand_Thumb_Base_R",
    "Bone_Hand_Thumb_Mid_R",
    "Bone_Hand_Thumb_Top_R",
    "bone_guitar_body",
    "bone_ik_hand_guitar_l",
    "bone_ik_hand_guitar_r",
)


def gh3_name_checksum(name: str) -> str:
    value = (~zlib.crc32(name.lower().encode("ascii"))) & 0xFFFFFFFF
    return f"0x{value:08x}"


def fallback_name(checksum: str | None, index: int | None = None) -> str:
    if checksum:
        return "bone_gh3_" + checksum.replace("0x", "")
    return f"bone_gh3_{index if index is not None else 'unknown'}"


def checksum_name_map(skeleton_bones: list[dict[str, Any]]) -> dict[str, str]:
    skeleton_checksums = {
        str(bone.get("name_checksum"))
        for bone in skeleton_bones
        if bone.get("name_checksum")
    }
    names = set(SUPPLEMENTAL_GHWT_BONE_NAMES)
    names.update(
        str(bone.get("stock_ghwt_name"))
        for bone in skeleton_bones
        if bone.get("stock_ghwt_name")
    )
    result: dict[str, str] = {}
    for name in names:
        try:
            checksum = gh3_name_checksum(name)
        except UnicodeEncodeError:
            continue
        if checksum in skeleton_checksums:
            result[checksum] = name
    return result


def resolved_bone_name(
    bone: dict[str, Any],
    name_by_checksum: dict[str, str],
) -> str:
    checksum = bone.get("name_checksum")
    if checksum in name_by_checksum:
        return name_by_checksum[str(checksum)]
    return fallback_name(str(checksum) if checksum else None, int(bone.get("index", -1)))
