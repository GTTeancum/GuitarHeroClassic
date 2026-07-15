#!/usr/bin/env python3
"""Verify the ihatecompvir source boundary for the GH2 character pose publisher.

This intentionally checks source evidence, not native behavior. It should pass
when the currently available ihatecompvir sources still expose call flow and
format shape but not the complete runtime body-pose publisher.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


def compact(text: str) -> str:
    return re.sub(r"\s+", "", text)


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8", errors="replace")
    except OSError as exc:
        raise SystemExit(f"missing source file: {path} ({exc})") from exc


def pose_meshes_body(text: str) -> str:
    match = re.search(
        r"void\s+CharBonesMeshes::PoseMeshes\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
        text,
        flags=re.S,
    )
    return match.group("body") if match else ""


def record(results: list[tuple[str, bool, str]], label: str, ok: bool, detail: str) -> None:
    results.append((label, ok, detail))
    print(f"{'PASS' if ok else 'FAIL'} {label}: {detail}")


def default_rb3_root(cwd: Path) -> Path:
    live = cwd / "third_party" / "ihatecompvir-live" / "rb3"
    if live.exists():
        return live
    return cwd / "third_party" / "ihatecompvir-extra" / "rb3-latest"


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(
        description="Check ihatecompvir pose-publisher source coverage."
    )
    parser.add_argument(
        "--rb3-root",
        type=Path,
        default=None,
        help="Path to ihatecompvir/rb3 source root. Defaults to live mirror if present.",
    )
    parser.add_argument(
        "--require-rb2-dump",
        action="store_true",
        help="Fail if rb3/doc/rb2_dump character files are unavailable.",
    )
    args = parser.parse_args(argv)

    cwd = Path.cwd()
    rb3_root = (args.rb3_root or default_rb3_root(cwd)).resolve()
    src_char = rb3_root / "src" / "system" / "char"
    dump_char = rb3_root / "doc" / "rb2_dump" / "rockband2" / "system" / "src" / "char"

    char_clip = read_text(src_char / "CharClip.cpp")
    char_bones = read_text(src_char / "CharBones.cpp")
    char_bones_h = read_text(src_char / "CharBones.h")
    char_bones_samples = read_text(src_char / "CharBonesSamples.cpp")
    char_bones_samples_h = read_text(src_char / "CharBonesSamples.h")
    char_bones_meshes = read_text(src_char / "CharBonesMeshes.cpp")

    c_char_clip = compact(char_clip)
    c_char_bones = compact(char_bones)
    c_char_bones_h = compact(char_bones_h)
    c_char_bones_samples = compact(char_bones_samples)
    c_char_bones_samples_h = compact(char_bones_samples_h)
    meshes_pose_body = pose_meshes_body(char_bones_meshes)
    c_meshes_pose_body = compact(meshes_pose_body)

    results: list[tuple[str, bool, str]] = []

    record(
        results,
        "charclip-posemeshes-call-order",
        "ScaleDown(meshes,0.0f);" in c_char_clip
        and "ScaleAdd(meshes,1.0f,f,0.0f);" in c_char_clip
        and "meshes.PoseMeshes();" in c_char_clip,
        "CharClip::PoseMeshes builds temp meshes and delegates to PoseMeshes",
    )
    record(
        results,
        "samples-scaleaddsample-visible",
        "voidCharBonesSamples::ScaleAddSample(CharBones&bones,floatf1,inti,floatf2)" in c_char_bones_samples
        and "CharBones::ScaleAdd(bones,(1.0f-f2)*f1);" in c_char_bones_samples
        and "CharBones::ScaleAdd(bones,f2*f1);" in c_char_bones_samples,
        "CharBonesSamples::ScaleAddSample source body is visible",
    )
    record(
        results,
        "charbones-clip-delegate-visible",
        "voidCharBones::ScaleAdd(CharClip*clip,floatf1,floatf2,floatf3)" in c_char_bones
        and "clip->ScaleAdd(*this,f1,f2,f3);" in c_char_bones,
        "CharBones::ScaleAdd(CharClip*) only delegates to CharClip::ScaleAdd",
    )
    record(
        results,
        "charbones-bones-scaleadd-body-missing",
        "voidScaleAdd(CharBones&,float)const;" in c_char_bones_h
        and "CharBones::ScaleAdd(CharBones&" not in c_char_bones,
        "CharBones::ScaleAdd(CharBones&, float) is declared but not implemented in visible rb3 source",
    )
    record(
        results,
        "samples-evaluatechannel-body-missing",
        "voidEvaluateChannel(void*,int,int,float);" in c_char_bones_samples_h
        and "CharBonesSamples::EvaluateChannel(" not in c_char_bones_samples,
        "CharBonesSamples::EvaluateChannel is declared but not implemented in visible rb3 source",
    )
    record(
        results,
        "posemeshes-latest-source-stub",
        bool(meshes_pose_body)
        and "floatangle;" in c_meshes_pose_body
        and "Hmx::Matrix3m;" in c_meshes_pose_body
        and "m.RotateAboutY(angle);" in c_meshes_pose_body
        and "m.RotateAboutX(angle);" in c_meshes_pose_body
        and "SetWorldXfm" not in meshes_pose_body
        and "WorldXfm" not in meshes_pose_body,
        "Latest CharBonesMeshes::PoseMeshes body is a stub-like fragment with no transform publication",
    )

    rb2_dump_available = dump_char.exists()
    record(
        results,
        "rb2-dump-available",
        rb2_dump_available or not args.require_rb2_dump,
        f"rb2 dump path {'present' if rb2_dump_available else 'absent'}: {dump_char}",
    )
    if rb2_dump_available:
        dump_meshes = read_text(dump_char / "CharBonesMeshes.cpp")
        dump_samples = read_text(dump_char / "CharBonesSamples.cpp")
        dump_clip_samples = read_text(dump_char / "CharClipSamples.cpp")
        c_dump_meshes = compact(dump_meshes)
        c_dump_samples = compact(dump_samples)
        c_dump_clip_samples = compact(dump_clip_samples)
        record(
            results,
            "rb2-posemeshes-range-local-map",
            "voidCharBonesMeshes::PoseMeshes(classCharBonesMeshes*constthis" in c_dump_meshes
            and "classObjOwnerPtr*bone;" in c_dump_meshes
            and "classVector3blendScale;" in c_dump_meshes
            and "SetWorldXfm" not in dump_meshes,
            "RB2 dump names PoseMeshes locals/range but not a portable statement body",
        )
        record(
            results,
            "rb2-evaluatechannel-range-local-map",
            "voidCharBonesSamples::EvaluateChannel()" in c_dump_samples
            and "classQuata;" in c_dump_samples
            and "classVector3b;" in c_dump_samples,
            "RB2 dump names EvaluateChannel locals/range but not a portable statement body",
        )
        record(
            results,
            "rb2-charclipsamples-scaleadd-bodyless",
            "voidCharClipSamples::ScaleAdd(classCharClipSamples*constthis" in c_dump_clip_samples
            and "intlastSample" in c_dump_clip_samples
            and "floatlastFrac" in c_dump_clip_samples,
            "RB2 dump maps CharClipSamples ScaleAdd overloads but leaves the sample writer body empty/bodyless",
        )

    failed = [label for label, ok, _ in results if not ok]
    if failed:
        print(f"SUMMARY fail={len(failed)} pass={len(results) - len(failed)}")
        print("FAILED " + ",".join(failed))
        return 1

    print(f"SUMMARY pass={len(results)} rb3_root={rb3_root}")
    print(
        "SOURCE-GAP still-fenced="
        "CharBones::ScaleAdd(CharBones&,float)|"
        "CharBonesSamples::EvaluateChannel|"
        "CharBonesMeshes::PoseMeshes|"
        "CharClipSamples::ScaleAdd"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
