#!/usr/bin/env python3
"""Verify the ihatecompvir source boundary for the GH2 character pose publisher.

This intentionally checks source evidence, not native behavior. It should pass
when the currently available ihatecompvir sources still expose call flow and
format shape but not the complete runtime body-pose publisher.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
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


def resolve_manifest_path(manifest_path: Path, raw_path: str) -> Path:
    path = Path(raw_path)
    if path.is_absolute():
        return path
    return (manifest_path.parent / path).resolve()


def load_json(path: Path) -> dict:
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise SystemExit(f"failed to read source-gap manifest {path}: {exc}") from exc
    if not isinstance(payload, dict):
        raise SystemExit(f"{path}: source-gap manifest root must be an object")
    return payload


def check_gap_manifest(
    results: list[tuple[str, bool, str]], manifest_path: Path
) -> None:
    manifest = load_json(manifest_path)
    expected = {
        "CharBones::ScaleAdd(CharBones&,float)",
        "CharBonesSamples::EvaluateChannel",
        "CharBonesMeshes::PoseMeshes",
        "CharClipSamples::ScaleAdd",
        "CharClipDriver::Evaluate",
    }
    still_fenced = set(manifest.get("still_fenced", []))
    record(
        results,
        "source-gap-manifest-five-body-fence",
        still_fenced == expected,
        "manifest pins the five still-fenced publisher bodies",
    )
    boundaries = manifest.get("rb2_dump_boundaries")
    record(
        results,
        "source-gap-manifest-rb2-boundary-count",
        isinstance(boundaries, list) and len(boundaries) == 7,
        "manifest records seven RB2 dump range/local boundaries",
    )
    if not isinstance(boundaries, list):
        return
    for item in boundaries:
        if not isinstance(item, dict):
            record(results, "source-gap-manifest-entry-object", False, "entry is not an object")
            continue
        label = str(item.get("label", "unnamed"))
        file_value = item.get("file")
        text = ""
        file_ok = isinstance(file_value, str) and bool(file_value)
        if file_ok:
            path = resolve_manifest_path(manifest_path, file_value)
            text = read_text(path)
        record(results, f"source-gap-manifest-{label}-file", file_ok, str(file_value))
        range_text = f"// Range: {item.get('range', '')}"
        signature = str(item.get("signature", ""))
        fragments = item.get("required_fragments", [])
        record(
            results,
            f"source-gap-manifest-{label}-range",
            bool(text and range_text in text),
            str(item.get("range", "")),
        )
        record(
            results,
            f"source-gap-manifest-{label}-signature",
            bool(text and signature and signature in text),
            signature,
        )
        record(
            results,
            f"source-gap-manifest-{label}-fenced",
            item.get("portable_statement_body") is False,
            "portable_statement_body=false",
        )
        if isinstance(fragments, list):
            missing = [
                fragment
                for fragment in fragments
                if not isinstance(fragment, str) or fragment not in text
            ]
            record(
                results,
                f"source-gap-manifest-{label}-fragments",
                not missing,
                "required locals present" if not missing else f"missing {missing[0]}",
            )
        else:
            record(
                results,
                f"source-gap-manifest-{label}-fragments",
                False,
                "required_fragments must be a list",
            )


def default_rb3_root(cwd: Path) -> Path:
    live = cwd / "third_party" / "ihatecompvir-live" / "rb3"
    if live.exists():
        return live
    return cwd / "third_party" / "ihatecompvir-extra" / "rb3-latest"


def default_ihatecompvir_root(cwd: Path) -> Path:
    live = cwd / "third_party" / "ihatecompvir-live"
    if live.exists():
        return live
    return cwd / "third_party" / "ihatecompvir-extra"


def default_gltfmilo_root(cwd: Path) -> Path:
    live = cwd / "third_party" / "ihatecompvir-live" / "glTFMilo"
    if live.exists():
        return live
    return cwd / "third_party" / "ihatecompvir-public-milo-sources" / "glTFMilo"


def default_grim_root(cwd: Path) -> Path:
    live = cwd / "third_party" / "ihatecompvir-live" / "grim"
    if live.exists():
        return live
    return cwd / "third_party" / "ihatecompvir-extra" / "grim"


def default_re_notes_root(cwd: Path) -> Path:
    live = cwd / "third_party" / "ihatecompvir-live" / "re-notes"
    if live.exists():
        return live
    return cwd / "third_party" / "ihatecompvir-extra" / "re-notes"


def default_rb2_dump_char(cwd: Path, rb3_root: Path) -> Path:
    committed = (
        cwd
        / "third_party"
        / "ihatecompvir-extra"
        / "rb3-retail-old"
        / "doc"
        / "rb2_dump"
        / "rockband2"
        / "system"
        / "src"
        / "char"
    )
    if committed.exists():
        return committed
    dump = rb3_root / "doc" / "rb2_dump" / "rockband2" / "system" / "src" / "char"
    if dump.exists():
        return dump
    return committed


def same_path(a: Path, b: Path) -> bool:
    return str(a.resolve()).lower() == str(b.resolve()).lower()


def nested_git_toplevel(path: Path) -> Path | None:
    try:
        result = subprocess.run(
            ["git", "-C", str(path), "rev-parse", "--show-toplevel"],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            timeout=10,
        )
    except (OSError, subprocess.CalledProcessError, subprocess.TimeoutExpired):
        return None
    top = Path(result.stdout.strip())
    if not top:
        return None
    return top if same_path(top, path) else None


def git_short_head(path: Path) -> str | None:
    if nested_git_toplevel(path) is None:
        return None
    try:
        result = subprocess.run(
            ["git", "-C", str(path), "rev-parse", "--short", "HEAD"],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            timeout=10,
        )
    except (OSError, subprocess.CalledProcessError, subprocess.TimeoutExpired):
        return None
    return result.stdout.strip() or None


def git_short_ref(path: Path, ref: str) -> str | None:
    if nested_git_toplevel(path) is None:
        return None
    try:
        result = subprocess.run(
            ["git", "-C", str(path), "rev-parse", "--short", ref],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            timeout=10,
        )
    except (OSError, subprocess.CalledProcessError, subprocess.TimeoutExpired):
        return None
    return result.stdout.strip() or None


def git_remote_tip(path: Path) -> tuple[str | None, str | None]:
    for ref in ("origin/master", "origin/main"):
        commit = git_short_ref(path, ref)
        if commit:
            return ref, commit
    return None, None


SNAPSHOT_REPOS = {
    "rb3": "rb3",
    "gltfmilo": "glTFMilo",
    "grim": "grim",
    "re-notes": "re-notes",
}

LIVE_HEAD_URLS = {
    "rb3": "https://github.com/ihatecompvir/rb3",
    "gltfmilo": "https://github.com/ihatecompvir/glTFMilo.git",
    "grim": "https://github.com/ihatecompvir/grim.git",
    "re-notes": "https://github.com/ihatecompvir/re-notes.git",
}


def snapshot_commit(path: Path, label: str, cwd: Path) -> str | None:
    source_commit = path / "SOURCE_COMMIT.txt"
    if source_commit.exists():
        text = read_text(source_commit)
        match = re.search(r"Commit:\s*([0-9a-fA-F]{7,40})", text)
        if match:
            return match.group(1)[:7]

    repo_name = SNAPSHOT_REPOS[label]
    for candidate in (
        path / "README.md",
        path.parent / "README.md",
        cwd / "third_party" / "ihatecompvir-public-milo-sources" / "README.md",
    ):
        if not candidate.exists():
            continue
        text = read_text(candidate)
        patterns = [
            rf"`{re.escape(repo_name)}`:\s*`([0-9a-fA-F]{{7,40}})`",
            rf"`ihatecompvir/{re.escape(repo_name)}`\s*`([0-9a-fA-F]{{7,40}})`",
        ]
        for pattern in patterns:
            match = re.search(pattern, text)
            if match:
                return match.group(1)[:7]
    return None


def source_short_head(path: Path, label: str, cwd: Path) -> tuple[str | None, str]:
    commit = git_short_head(path)
    if commit:
        return commit, "git"
    commit = snapshot_commit(path, label, cwd)
    if commit:
        return commit, "snapshot"
    return None, "unknown"


FRESHNESS_LABELS = {
    "rb3": "rb3-mirror-fresh",
    "gltfmilo": "gltfmilo-mirror-fresh",
    "grim": "grim-mirror-fresh",
    "re-notes": "re-notes-mirror-fresh",
}

LIVE_HEAD_LABELS = {
    "rb3": "rb3-live-head-fresh",
    "gltfmilo": "gltfmilo-live-head-fresh",
    "grim": "grim-live-head-fresh",
    "re-notes": "re-notes-live-head-fresh",
}


def git_origin_url(path: Path) -> str | None:
    try:
        result = subprocess.run(
            ["git", "-C", str(path), "remote", "get-url", "origin"],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            timeout=10,
        )
    except (OSError, subprocess.CalledProcessError, subprocess.TimeoutExpired):
        return None
    return result.stdout.strip() or None


def git_ls_remote_head(url: str) -> str | None:
    try:
        result = subprocess.run(
            ["git", "ls-remote", url, "HEAD"],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            timeout=30,
        )
    except (OSError, subprocess.CalledProcessError, subprocess.TimeoutExpired):
        return None
    fields = result.stdout.split()
    if not fields:
        return None
    return fields[0][:7]


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
        "--ihatecompvir-root",
        type=Path,
        default=None,
        help="Path containing glTFMilo/grim/re-notes mirrors. Defaults to live mirror if present.",
    )
    parser.add_argument(
        "--gltfmilo-root",
        type=Path,
        default=None,
        help="Path to ihatecompvir/glTFMilo. Overrides --ihatecompvir-root for glTFMilo.",
    )
    parser.add_argument(
        "--grim-root",
        type=Path,
        default=None,
        help="Path to ihatecompvir/grim. Overrides --ihatecompvir-root for grim.",
    )
    parser.add_argument(
        "--re-notes-root",
        type=Path,
        default=None,
        help="Path to ihatecompvir/re-notes. Overrides --ihatecompvir-root for re-notes.",
    )
    parser.add_argument(
        "--require-rb2-dump",
        action="store_true",
        help="Fail if rb3/doc/rb2_dump character files are unavailable.",
    )
    parser.add_argument(
        "--require-fresh-remotes",
        action="store_true",
        help="Fail if a checked mirror HEAD differs from origin/master or origin/main.",
    )
    parser.add_argument(
        "--require-live-heads",
        action="store_true",
        help="Fail if a checked mirror HEAD differs from the live GitHub default-branch HEAD.",
    )
    parser.add_argument(
        "--gap-manifest",
        type=Path,
        default=Path("tools/pose_publisher_source_gap_manifest.json"),
        help="Manifest pinning RB2 dump range/local evidence for still-fenced bodies.",
    )
    args = parser.parse_args(argv)

    cwd = Path.cwd()
    rb3_root = (args.rb3_root or default_rb3_root(cwd)).resolve()
    ihate_root = (
        args.ihatecompvir_root.resolve() if args.ihatecompvir_root else None
    )
    src_char = rb3_root / "src" / "system" / "char"
    dump_char = default_rb2_dump_char(cwd, rb3_root)
    gltf_root = (
        args.gltfmilo_root.resolve()
        if args.gltfmilo_root
        else (ihate_root / "glTFMilo" if ihate_root else default_gltfmilo_root(cwd))
    )
    grim_root = (
        args.grim_root.resolve()
        if args.grim_root
        else (ihate_root / "grim" if ihate_root else default_grim_root(cwd))
    )
    re_notes_root = (
        args.re_notes_root.resolve()
        if args.re_notes_root
        else (ihate_root / "re-notes" if ihate_root else default_re_notes_root(cwd))
    )

    char_clip = read_text(src_char / "CharClip.cpp")
    char_bones = read_text(src_char / "CharBones.cpp")
    char_bones_h = read_text(src_char / "CharBones.h")
    char_bones_samples = read_text(src_char / "CharBonesSamples.cpp")
    char_bones_samples_h = read_text(src_char / "CharBonesSamples.h")
    char_bones_meshes = read_text(src_char / "CharBonesMeshes.cpp")
    char_clip_driver = read_text(src_char / "CharClipDriver.cpp")

    c_char_clip = compact(char_clip)
    c_char_bones = compact(char_bones)
    c_char_bones_h = compact(char_bones_h)
    c_char_bones_samples = compact(char_bones_samples)
    c_char_bones_samples_h = compact(char_bones_samples_h)
    c_char_clip_driver = compact(char_clip_driver)
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
    record(
        results,
        "charclipdriver-evaluate-body-missing",
        "CharClipDriver::Evaluate(" not in c_char_clip_driver,
        "CharClipDriver::Evaluate is not implemented in visible rb3 source",
    )

    gltf_node_processor = read_text(
        gltf_root / "Source" / "glTFMilo" / "Core" / "NodeProcessor.cs"
    )
    gltf_options = read_text(gltf_root / "Source" / "Options.cs")
    grim_char_bones_samples = read_text(
        grim_root / "core" / "grim" / "src" / "scene" / "char_bones_samples" / "io.rs"
    )
    grim_char_clip_samples = read_text(
        grim_root / "core" / "grim" / "src" / "scene" / "char_clip_samples" / "io.rs"
    )
    re_notes_char_bones_samples = read_text(
        re_notes_root / "templates" / "milo" / "char_bones_samples.bt"
    )
    re_notes_char_clip_samples = read_text(
        re_notes_root / "templates" / "milo" / "char_clip_samples.bt"
    )
    non_rb3_text = "\n".join(
        [
            gltf_node_processor,
            gltf_options,
            grim_char_bones_samples,
            grim_char_clip_samples,
            re_notes_char_bones_samples,
            re_notes_char_clip_samples,
        ]
    )
    c_non_rb3_text = compact(non_rb3_text)
    record(
        results,
        "gltfmilo-hair-branch-source-present",
        "CollectHairChainsSplitAtBranches" in gltf_node_processor
        and "disable-splitting" in gltf_options,
        "glTFMilo exposes hair branch splitting/diagnostic source, not pose publisher bodies",
    )
    record(
        results,
        "grim-sample-format-source-present",
        "load_char_bones_samples_header" in grim_char_bones_samples
        and "load_char_bones_samples_data" in grim_char_bones_samples
        and "load_char_bones_samples(&mut self.full" in grim_char_clip_samples,
        "grim exposes CharBonesSamples/CharClipSamples format loaders",
    )
    record(
        results,
        "re-notes-sample-template-source-present",
        "CharBonesSamples char_bones_samples" in re_notes_char_bones_samples
        and '#include "char_bones_samples.bt"' in re_notes_char_clip_samples,
        "re-notes exposes sample templates as format maps",
    )
    record(
        results,
        "non-rb3-publisher-bodies-absent",
        "CharBones::ScaleAdd(CharBones&" not in c_non_rb3_text
        and "CharBonesSamples::EvaluateChannel(" not in c_non_rb3_text
        and "CharBonesMeshes::PoseMeshes(" not in c_non_rb3_text
        and "CharClipSamples::ScaleAdd(" not in c_non_rb3_text
        and "CharClipDriver::Evaluate(" not in c_non_rb3_text,
        "glTFMilo/grim/re-notes do not provide the five missing C++ runtime publisher bodies",
    )
    check_gap_manifest(results, args.gap_manifest)

    for label, path in (
        ("rb3", rb3_root),
        ("gltfmilo", gltf_root),
        ("grim", grim_root),
        ("re-notes", re_notes_root),
    ):
        commit, commit_source = source_short_head(path, label, cwd)
        remote_ref, remote_commit = git_remote_tip(path)
        if args.require_fresh_remotes:
            record(
                results,
                FRESHNESS_LABELS[label],
                bool(commit and (commit_source == "snapshot" or commit == remote_commit)),
                f"{commit_source}={commit or 'unknown'} remote={remote_ref or 'snapshot'}:{remote_commit or commit or 'unknown'}",
            )
        if args.require_live_heads:
            url = git_origin_url(path) or LIVE_HEAD_URLS[label]
            live_head = git_ls_remote_head(url) if url else None
            record(
                results,
                LIVE_HEAD_LABELS[label],
                bool(commit and live_head and commit == live_head),
                f"{commit_source}={commit or 'unknown'} live=HEAD:{live_head or 'unknown'} url={url or 'none'}",
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
        dump_bones = read_text(dump_char / "CharBones.cpp")
        dump_samples = read_text(dump_char / "CharBonesSamples.cpp")
        dump_clip_samples = read_text(dump_char / "CharClipSamples.cpp")
        dump_clip_driver = read_text(dump_char / "CharClipDriver.cpp")
        c_dump_meshes = compact(dump_meshes)
        c_dump_bones = compact(dump_bones)
        c_dump_samples = compact(dump_samples)
        c_dump_clip_samples = compact(dump_clip_samples)
        c_dump_clip_driver = compact(dump_clip_driver)
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
            "rb2-charbones-scaleadd-range-local-map",
            "voidCharBones::ScaleAdd()" in c_dump_bones
            and "structShortVector3*sp;" in c_dump_bones
            and "conststructByteQuat*bq;" in c_dump_bones
            and "conststructShortQuat*qs;" in c_dump_bones
            and "floataweight;" in c_dump_bones,
            "RB2 dump names CharBones ScaleAdd compression/local buffers but not a portable statement body",
        )
        record(
            results,
            "rb2-charbones-scaleadd-delegate-stub-empty",
            "voidCharBones::ScaleAdd(classCharBones*constthis/*r0*/){}" in c_dump_bones,
            "RB2 dump maps the CharBones delegate overload as an empty/bodyless row",
        )
        record(
            results,
            "rb2-charclipsamples-scaleadd-bodyless",
            "voidCharClipSamples::ScaleAdd(classCharClipSamples*constthis" in c_dump_clip_samples
            and "intlastSample" in c_dump_clip_samples
            and "floatlastFrac" in c_dump_clip_samples,
            "RB2 dump maps CharClipSamples ScaleAdd overloads but leaves the sample writer body empty/bodyless",
        )
        record(
            results,
            "rb2-charclipsamples-scaleadd-sample-writer-empty",
            "voidCharClipSamples::ScaleAdd(classCharClipSamples*constthis/*r28*/,classCharBones&bones/*r29*/,floatweight/*f29*/,intsample/*r30*/,floatfrac/*f30*/,intlastSample/*r31*/,floatlastFrac/*f31*/){}"
            in c_dump_clip_samples,
            "RB2 dump maps the sample-index CharClipSamples ScaleAdd writer as an empty/bodyless row",
        )
        record(
            results,
            "rb2-charclipdriver-evaluate-range-local-map",
            "floatCharClipDriver::Evaluate(classCharClipDriver*constthis" in c_dump_clip_driver
            and "floatnextWeight" in c_dump_clip_driver
            and "floatoldFrame" in c_dump_clip_driver,
            "RB2 dump maps CharClipDriver Evaluate range/locals but not a portable statement body",
        )

    failed = [label for label, ok, _ in results if not ok]
    if failed:
        print(f"SUMMARY fail={len(failed)} pass={len(results) - len(failed)}")
        print("FAILED " + ",".join(failed))
        return 1

    print(
        f"SUMMARY pass={len(results)} rb3_root={rb3_root} "
        f"gltfmilo_root={gltf_root} grim_root={grim_root} "
        f"re_notes_root={re_notes_root}"
    )
    for label, path in (
        ("rb3", rb3_root),
        ("gltfMilo", gltf_root),
        ("grim", grim_root),
        ("re-notes", re_notes_root),
    ):
        source_label = "gltfmilo" if label == "gltfMilo" else label
        commit, commit_source = source_short_head(path, source_label, cwd)
        if commit:
            remote_ref, remote_commit = git_remote_tip(path)
            if remote_ref and remote_commit:
                print(
                    f"MIRROR {label} source={commit_source} commit={commit} "
                    f"remote_ref={remote_ref} remote_commit={remote_commit}"
                )
            else:
                print(f"MIRROR {label} source={commit_source} commit={commit}")
    print(
        "SOURCE-GAP still-fenced="
        "CharBones::ScaleAdd(CharBones&,float)|"
        "CharBonesSamples::EvaluateChannel|"
        "CharBonesMeshes::PoseMeshes|"
        "CharClipSamples::ScaleAdd|"
        "CharClipDriver::Evaluate"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
