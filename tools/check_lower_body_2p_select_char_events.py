#!/usr/bin/env python3
"""Verify 2P character-select UI events directly from stock GH2 PS2 DTB rows."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys


MULTIPLAYER_DTB = "ui/gen/multiplayer.dtb"
CHAR_OBJECTS_DTB = "char/gen/char_objects.dtb"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def root_dir() -> Path:
    return Path(__file__).resolve().parent.parent


def tool_path(root: Path, name: str) -> Path:
    candidates = {
        "ark_tool": root / "engine/out/build/win-amd64-debug/_tools_ark/ark_tool.exe",
        "dtb_tool": root / "engine/out/build/win-amd64-debug/_tools_dtb/dtb_tool.exe",
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


def extract_stock_dtbs(root: Path, ark_dir: Path) -> tuple[Path, Path]:
    out_root = root / "engine/out/2p_select_probe/char_event_check"
    if out_root.exists():
        shutil.rmtree(out_root)
    out_root.mkdir(parents=True, exist_ok=True)
    ark_tool = tool_path(root, "ark_tool")
    multiplayer = out_root / "multiplayer.dtb"
    char_objects = out_root / "char_objects.dtb"
    for source, target in (
        (MULTIPLAYER_DTB, multiplayer),
        (CHAR_OBJECTS_DTB, char_objects),
    ):
        run(
            [
                str(ark_tool),
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
    return multiplayer, char_objects


def dump_compact(root: Path, path: Path) -> str:
    dtb_tool = tool_path(root, "dtb_tool")
    text = run([str(dtb_tool), "dump", str(path), "--lines"], root)
    text = re.sub(r";line\s+\d+", "", text)
    return re.sub(r"\s+", "", text)


def check_rows(multiplayer: str, char_objects: str) -> None:
    require(
        "newMultiCharSelPanelmulti_sel_character_panel(filemulti_sel_character.milo)" in multiplayer,
        "multiplayer.dtb missing 2P character screen panel",
    )
    require(
        "newCharsysPanelchar_multi(filechar_multi.milo)(num_placers2)(load_orderLOAD_CHARACTERS)" in multiplayer,
        "multiplayer.dtb missing char_multi CharsysPanel",
    )
    require(
        "char_multichar_event$playerNumanimate" in multiplayer,
        "multiplayer.dtb missing char_multi animate event",
    )
    require(
        "char_multichar_event[player_num]select" in multiplayer,
        "multiplayer.dtb missing char_multi select event",
    )
    require(
        "char_multiset_placer0{multi_sel_character_panelfindchar_multi0.placer}" in multiplayer,
        "multiplayer.dtb missing player 0 placer binding",
    )
    require(
        "char_multiset_placer1{multi_sel_character_panelfindchar_multi1.placer}" in multiplayer,
        "multiplayer.dtb missing player 1 placer binding",
    )
    require(
        "if_else{gamemultiplayer}{do{reset_hair$this}{$thisplay_clipui_loop{|kPlayLastkPlayGraphLoop}}" in char_objects,
        "char_objects.dtb missing 2P animate ui_loop branch",
    )
    require(
        "{$thisplay_clipui_enterkPlayNoBlend}" in char_objects,
        "char_objects.dtb missing non-multiplayer ui_enter branch",
    )
    require(
        "select{$thisplay_clipui_loop{|kPlayLastkPlayGraphLoop}}" in char_objects,
        "char_objects.dtb missing select ui_loop branch",
    )
    require(
        "store{reset_hair$this}{$thisplay_clipui_loop{|kPlayNoBlendkPlayGraphLoop}}" in char_objects,
        "char_objects.dtb missing separate store branch",
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Decode stock 2P character-select event rows from multiplayer.dtb and char_objects.dtb."
    )
    parser.add_argument("--ark-dir", type=Path, default=None)
    args = parser.parse_args()
    root = root_dir()
    try:
        ark_dir = args.ark_dir.resolve() if args.ark_dir else default_ark_dir(root)
        multiplayer_path, char_objects_path = extract_stock_dtbs(root, ark_dir)
        multiplayer = dump_compact(root, multiplayer_path)
        char_objects = dump_compact(root, char_objects_path)
        check_rows(multiplayer, char_objects)
    except RuntimeError as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        return 1

    print(
        "PASS lower_body_2p_select_char_events "
        "source=stock_GH2_PS2 "
        f"screen_script={MULTIPLAYER_DTB} "
        f"char_objects={CHAR_OBJECTS_DTB} "
        "events=animate,select "
        "animate_multiplayer=reset_hair+ui_loop "
        "select=ui_loop "
        "single_player=ui_enter+ui_loop "
        "placers=char_multi0.placer,char_multi1.placer"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
