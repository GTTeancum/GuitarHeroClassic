#!/usr/bin/env python3
"""Verify the stock 2P select animation source identity from ARK/MILO rows."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys


SCREEN_MILOS = (
    "ui/gen/multi_sel_character.milo_ps2",
    "ui/gen/char_multi.milo_ps2",
)

CHARACTER_SOURCES = {
    "glam1": (
        "char/glam1/og/gen/glam1_ui.milo_ps2",
        "char/glam1/anims/gen/glam1_ui.milo_ps2",
    ),
    "metal1": (
        "char/metal1/og/gen/metal1_ui.milo_ps2",
        "char/metal1/anims/gen/metal1_ui.milo_ps2",
    ),
    "rock1": (
        "char/rock1/og/gen/rock1_ui.milo_ps2",
        "char/rock1/anims/gen/rock1_ui.milo_ps2",
    ),
    "rock2": (
        "char/rock2/og/gen/rock2_ui.milo_ps2",
        "char/rock1/anims/gen/rock1_ui.milo_ps2",
    ),
    "funk1": (
        "char/funk1/og/gen/funk1_ui.milo_ps2",
        "char/funk1/anims/gen/funk1_ui.milo_ps2",
    ),
    "deathmetal1": (
        "char/deathmetal1/og/gen/deathmetal1_ui.milo_ps2",
        "char/deathmetal1/anims/gen/deathmetal1_ui.milo_ps2",
    ),
}

ABSENT_CHARACTER_ANIMS = {
    "rock2": "char/rock2/anims/gen/rock2_ui.milo_ps2",
}

FORBIDDEN_ANIM_TOKENS = ("2p", "multi", "select")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def root_dir() -> Path:
    return Path(__file__).resolve().parent.parent


def tool_path(root: Path, name: str) -> Path:
    candidates = {
        "ark_tool": root / "engine/out/build/win-amd64-debug/_tools_ark/ark_tool.exe",
        "milo_tool": root / "engine/out/build/win-amd64-debug/_tools_milo/milo_tool.exe",
    }
    path = candidates[name]
    require(path.is_file(), f"missing built {name}: {path}")
    return path


def default_ark_dir(root: Path) -> Path:
    env = os.environ.get("GHOGX_GH2_ARK_DIR")
    candidates = []
    if env:
        candidates.append(Path(env))
    candidates.append(root.parent / "gh2_ps2_hybrid_assets/gen")
    candidates.append(root / "../gh2_ps2_hybrid_assets/gen")
    for candidate in candidates:
        if (candidate / "main.hdr").is_file() and (candidate / "main_0.ark").is_file():
            return candidate.resolve()
    raise RuntimeError("missing GH2 PS2 ARK dir; set GHOGX_GH2_ARK_DIR")


def run(command: list[str], cwd: Path) -> str:
    result = subprocess.run(
        command,
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        check=False,
    )
    require(result.returncode == 0, "command failed:\n" + " ".join(command) + "\n" + result.stdout)
    return result.stdout


def archive_paths(root: Path, ark_dir: Path) -> set[str]:
    out = run(
        [str(tool_path(root, "ark_tool")), "list", str(ark_dir / "main.hdr"), "--limit", "2000"],
        root,
    )
    paths: set[str] = set()
    for line in out.splitlines():
        match = re.search(r"\s(\S+)$", line)
        if match:
            paths.add(match.group(1).replace("\\", "/"))
    return paths


def extract_milo(root: Path, ark_dir: Path, source: str, out_dir: Path) -> Path:
    target = out_dir / source.replace("/", "__")
    target.parent.mkdir(parents=True, exist_ok=True)
    run(
        [
            str(tool_path(root, "ark_tool")),
            "extract",
            str(ark_dir / "main.hdr"),
            str(ark_dir / "main_0.ark"),
            "--path",
            source,
            "--out",
            str(target),
        ],
        root,
    )
    return target


def milo_list(root: Path, milo: Path) -> str:
    return run([str(tool_path(root, "milo_tool")), "list", str(milo)], root)


def check_screen_milo(name: str, text: str) -> None:
    require("dir_type    : PanelDir" in text, f"{name}: expected PanelDir")
    require("CharClipSamples" not in text, f"{name}: screen MILO unexpectedly contains CharClipSamples")
    require("CharClipSet" not in text, f"{name}: screen MILO unexpectedly contains CharClipSet")
    if name.endswith("multi_sel_character.milo_ps2"):
        require("BandPlacer              2" in text, f"{name}: expected two BandPlacers")
        require("char_multi0.placer" in text, f"{name}: missing char_multi0.placer")
        require("char_multi1.placer" in text, f"{name}: missing char_multi1.placer")


def check_character_anim_milo(character: str, text: str) -> None:
    require("dir_type    : CharClipSet" in text, f"{character}: UI anim is not a CharClipSet")
    dir_match = re.search(r"^dir_name\s+:\s+(\S+)\s*$", text, re.MULTILINE)
    require(dir_match is not None, f"{character}: missing CharClipSet dir name")
    require(dir_match.group(1).endswith("_ui"), f"{character}: expected *_ui CharClipSet dir name")
    require("CharClipSamples         2" in text, f"{character}: expected exactly two UI clip samples")
    require("CharClipSamples         size=" in text and "ui_enter" in text, f"{character}: missing ui_enter")
    require("CharClipSamples         size=" in text and "ui_loop" in text, f"{character}: missing ui_loop")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Verify stock GH2 PS2 2P select clip assets and absence of a separate 2P body anim."
    )
    parser.add_argument("--ark-dir", type=Path, default=None)
    args = parser.parse_args()
    root = root_dir()
    try:
        ark_dir = args.ark_dir.resolve() if args.ark_dir else default_ark_dir(root)
        out_root = root / "engine/out/2p_select_probe/clip_asset_check"
        if out_root.exists():
            shutil.rmtree(out_root)
        out_root.mkdir(parents=True, exist_ok=True)

        paths = archive_paths(root, ark_dir)
        for screen in SCREEN_MILOS:
            require(screen in paths, f"stock archive missing {screen}")
            check_screen_milo(screen, milo_list(root, extract_milo(root, ark_dir, screen, out_root)))

        for character, (model, anim) in CHARACTER_SOURCES.items():
            require(model in paths, f"stock archive missing {model}")
            require(anim in paths, f"stock archive missing {anim}")
            check_character_anim_milo(
                character,
                milo_list(root, extract_milo(root, ark_dir, anim, out_root)),
            )

            character_entries = [
                path
                for path in paths
                if path.startswith(f"char/{character}/anims/gen/")
                and path.endswith(".milo_ps2")
            ]
            forbidden = [
                path
                for path in character_entries
                if any(token in Path(path).name.lower() for token in FORBIDDEN_ANIM_TOKENS)
                and path != anim
            ]
            require(not forbidden, f"{character}: unexpected separate 2P/select anim assets: {forbidden}")

        for character, missing_anim in ABSENT_CHARACTER_ANIMS.items():
            require(missing_anim not in paths, f"{character}: expected shared UI anim source, found {missing_anim}")

    except RuntimeError as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        return 1

    characters = ",".join(CHARACTER_SOURCES)
    print(
        "PASS lower_body_2p_select_clip_assets "
        "source=stock_GH2_PS2 "
        f"characters={characters} "
        "screen_milos=multi_sel_character,char_multi "
        "screen_body_clips=absent "
        "per_character_motion=ui_clipset "
        "ui_clip_samples=ui_enter,ui_loop "
        "separate_2p_anim_assets=absent "
        "rock2_shared_ui_anim=rock1_ui "
        "stock_asset_identity=true"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
