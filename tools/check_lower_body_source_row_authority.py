#!/usr/bin/env python3
"""Check the source-backed authority for the native lower-body output bridge."""

from __future__ import annotations

import argparse
from pathlib import Path
import sys


SOURCE_CHAR_DIR = Path("third_party/ihatecompvir-extra/rb3-latest/src/system/char")
NATIVE_CHAR_CLIP = Path("engine/src/character/char_clip.cpp")
DOC_PATH = Path("engine/src/character/IHATECOMPVIR_CHARACTER_MODEL_SOURCE.md")


def compact(text: str) -> str:
    return "".join(text.split())


def read(path: Path) -> str:
    if not path.is_file():
        raise RuntimeError(f"missing file: {path}")
    return path.read_text(encoding="utf-8", errors="replace")


def extract_between(text: str, start: str, end: str, label: str) -> str:
    start_at = text.find(start)
    if start_at < 0:
        raise RuntimeError(f"{label}: missing start marker {start}")
    end_at = text.find(end, start_at + len(start))
    if end_at < 0:
        raise RuntimeError(f"{label}: missing end marker {end}")
    return text[start_at:end_at]


def require_contains(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise RuntimeError(f"missing {label}: {needle}")


def require_contains_compact(text: str, needle: str, label: str) -> None:
    if compact(needle) not in compact(text):
        raise RuntimeError(f"missing {label}: {needle}")


def require_absent(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise RuntimeError(f"forbidden {label}: {needle}")


def check_source_charbone(source_dir: Path) -> None:
    char_bone = compact(read(source_dir / "CharBone.cpp"))
    stuff = extract_between(
        char_bone,
        "voidCharBone::StuffBones",
        "voidCharBone::ClearContext",
        "ihatecompvir CharBone::StuffBones",
    )
    require_contains(
        stuff,
        "if(mPositionContext&i){bonelist.push_back(CharBones::Bone("
        "CharBones::ChannelName(Name(),CharBones::TYPE_POS),GetWeight(i)));}",
        "position-context output row",
    )
    require_contains(
        stuff,
        "if(mScaleContext&i){bonelist.push_back(CharBones::Bone("
        "CharBones::ChannelName(Name(),CharBones::TYPE_SCALE),GetWeight(i)));}",
        "scale-context output row",
    )
    require_contains(
        stuff,
        "if(mRotation!=CharBones::TYPE_END&&mRotationContext&i){"
        "bonelist.push_back(CharBones::Bone(CharBones::ChannelName(Name(),mRotation),"
        "GetWeight(i)));}",
        "rotation-context output row",
    )


def check_source_pose_meshes(source_dir: Path) -> None:
    char_clip = compact(read(source_dir / "CharClip.cpp"))
    pose_meshes = extract_between(
        char_clip,
        "voidCharClip::PoseMeshes",
        "voidCharClip::SetPlayFlags",
        "ihatecompvir CharClip::PoseMeshes",
    )
    for needle, label in (
        ("CharBonesMeshesmeshes;", "temporary CharBonesMeshes"),
        ("meshes.SetName(\"tmp_viseme_bones\",dir);", "temporary bones name"),
        ("StuffBones(meshes);", "authored CharBone rows stuffed into meshes"),
        ("ScaleDown(meshes,0.0f);", "source ScaleDown step"),
        ("ScaleAdd(meshes,1.0f,f,0.0f);", "source ScaleAdd step"),
        ("meshes.PoseMeshes();", "source PoseMeshes handoff"),
    ):
        require_contains(pose_meshes, needle, label)


def check_source_sample_split(source_dir: Path) -> None:
    samples = compact(read(source_dir / "CharBonesSamples.cpp"))
    scale_add_sample = extract_between(
        samples,
        "voidCharBonesSamples::ScaleAddSample",
        "voidCharBonesSamples::Print",
        "ihatecompvir CharBonesSamples::ScaleAddSample",
    )
    for needle, label in (
        ("mStart=&mRawData[mTotalSize*i];", "first sample mStart"),
        ("CharBones::ScaleAdd(bones,(1.0f-f2)*f1);", "first sample weight"),
        ("if(f2>0.0f){", "adjacent sample guard"),
        ("mStart=&mRawData[mTotalSize*(i+1)];", "next sample mStart"),
        ("CharBones::ScaleAdd(bones,f2*f1);", "next sample weight"),
    ):
        require_contains(scale_add_sample, needle, label)


def check_native_bridge(native_path: Path) -> None:
    native = compact(read(native_path))
    stuff = extract_between(
        native,
        "voidsource_char_bone_stuff_bones",
        "SourceCharBoneDirDefaultState",
        "native source_char_bone_stuff_bones",
    )
    for needle, label in (
        ("bone.position_context&context_mask", "native position context gate"),
        ("source_char_bones_channel_name(bone.name,kSourceCharBonesTypePos)",
         "native position channel name"),
        ("bone.scale_context&context_mask", "native scale context gate"),
        ("source_char_bones_channel_name(bone.name,kSourceCharBonesTypeScale)",
         "native scale channel name"),
        ("bone.rotation_type!=kSourceCharBonesTypeEnd", "native rotation type gate"),
        ("bone.rotation_context&context_mask", "native rotation context gate"),
        ("source_char_bone_get_weight(bone,context_mask)", "native context weight"),
    ):
        require_contains(stuff, needle, label)

    dir_list = extract_between(
        native,
        "voidsource_char_bone_dir_list_bones",
        "std::vector<std::string>source_char_bone_dir_get_clip_types",
        "native source_char_bone_dir_list_bones",
    )
    require_contains(
        dir_list,
        "for(constCharClip::OutputBone&output_bone:output_bones){"
        "source_char_bone_stuff_bones(output_bone,context_mask,bones);}",
        "native CharBoneDir list delegates every output bone to StuffBones",
    )

    bridge = extract_between(
        native,
        "staticvoidapply_lower_body_output_layer",
        "staticvoidapply_hand_driver_output_layers",
        "native apply_lower_body_output_layer",
    )
    for needle, label in (
        ("conststd::vector<CharClip::OutputBone>&source_output_bones",
         "source output parameter"),
        ("conststd::stringkey=strip_transform_suffix(out.name);",
         "source output key"),
        ("if(!is_lower_body_pose_channel_name(key))continue;",
         "lower-body subset filter"),
        ("if(!lower_keys.insert(key).second)continue;",
         "deduplicated source output keys"),
        ("if(lower_keys.find(strip_transform_suffix(ch.bone_name))==lower_keys.end())",
         "frame channel must match source output key"),
        ("apply_clip_pose_output_layer(lower_channels,weight,character,relative,"
         "lower_output_bones,true)",
         "forced selected output publisher"),
    ):
        require_contains(bridge, needle, label)
    for needle in (
        "character.name",
        "foot_offset",
        "ankle_offset",
        "toe_offset",
        "fixup",
    ):
        require_absent(bridge, needle, "fabricated lower-body shortcut")


def check_doc(doc_path: Path) -> None:
    doc = read(doc_path)
    for needle, label in (
        ("2026-07-15 lower-body source row authority:",
         "source row authority section"),
        ("`CharBone::StuffBones` is the source-visible row authority",
         "CharBone::StuffBones authority statement"),
        ("`CharClip::PoseMeshes` builds `tmp_viseme_bones`",
         "CharClip::PoseMeshes authority statement"),
        ("the native lower-body bridge is allowed to publish only the active "
         "clip's decoded lower-body `OutputBone` subset",
         "bounded native lower-body publication statement"),
    ):
        require_contains_compact(doc, needle, label)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Verify that the lower-body output bridge is grounded in "
            "ihatecompvir CharBone/CharClip source row ownership."
        )
    )
    parser.add_argument("--source-dir", type=Path, default=SOURCE_CHAR_DIR)
    parser.add_argument("--native", type=Path, default=NATIVE_CHAR_CLIP)
    parser.add_argument("--doc", type=Path, default=DOC_PATH)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        check_source_charbone(args.source_dir)
        check_source_pose_meshes(args.source_dir)
        check_source_sample_split(args.source_dir)
        check_native_bridge(args.native)
        check_doc(args.doc)
    except RuntimeError as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        return 1
    print(
        "PASS lower_body_source_row_authority "
        "charbone_stuffbones=true "
        "charclip_posemeshes=true "
        "sample_split=true "
        "native_output_subset=true "
        "no_shortcut_fix=true"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
