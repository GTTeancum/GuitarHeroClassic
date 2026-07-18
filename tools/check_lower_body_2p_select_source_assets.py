#!/usr/bin/env python3
"""Verify 2P select BandPlacer rows directly from stock GH2 PS2 MILO bytes."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys


SCREEN_MILO = "ui/gen/multi_sel_character.milo_ps2"
ENTRY_PREFIX = "BandPlacer__"
MATRIX0_OFFSET = 46
MATRIX1_OFFSET = 94
STRING_OFFSET = 151
ENTRY_SIZE = 188
TOLERANCE = 0.00025


PLACERS = {
    "char_multi0.placer": {
        "entry": "BandPlacer__char_multi0.placer",
        "player": 0,
    },
    "char_multi1.placer": {
        "entry": "BandPlacer__char_multi1.placer",
        "player": 1,
    },
}


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


def extract_entries(root: Path, ark_dir: Path) -> Path:
    out_root = root / "engine/out/2p_select_probe/source_asset_check"
    entries = out_root / "multi_sel_entries"
    if entries.exists():
        shutil.rmtree(entries)
    entries.mkdir(parents=True, exist_ok=True)
    milo_path = out_root / "multi_sel_character.milo_ps2"
    ark_tool = tool_path(root, "ark_tool")
    milo_tool = tool_path(root, "milo_tool")
    run(
        [
            str(ark_tool),
            "extract",
            str(ark_dir / "main.hdr"),
            str(ark_dir / "main_0.ark"),
            "--path",
            SCREEN_MILO,
            "--out",
            str(milo_path),
        ],
        root,
    )
    run([str(milo_tool), "extract", str(milo_path), "--out", str(entries)], root)
    return entries


def read_matrix(data: bytes, offset: int) -> list[float]:
    require(offset + 12 * 4 <= len(data), "matrix row outside placer body")
    return [struct.unpack_from("<f", data, offset + i * 4)[0] for i in range(12)]


def read_string(data: bytes, offset: int) -> tuple[str, int]:
    require(offset + 4 <= len(data), "string length outside placer body")
    length = struct.unpack_from("<I", data, offset)[0]
    start = offset + 4
    end = start + length
    require(end <= len(data), "string payload outside placer body")
    return data[start:end].decode("ascii"), end


def parse_placer(path: Path) -> dict:
    data = path.read_bytes()
    require(len(data) == ENTRY_SIZE, f"{path.name}: unexpected body size {len(data)}")
    matrix0 = read_matrix(data, MATRIX0_OFFSET)
    matrix1 = read_matrix(data, MATRIX1_OFFSET)
    group, cursor = read_string(data, STRING_OFFSET)
    mesh, cursor = read_string(data, cursor)
    require(cursor == len(data), f"{path.name}: trailing bytes after target mesh")
    return {
        "matrix0": matrix0,
        "matrix1": matrix1,
        "target_group": group,
        "target_mesh": mesh,
    }


def max_delta(a: list[float], b: list[float]) -> float:
    require(len(a) == len(b), "matrix lengths differ")
    return max(abs(x - y) for x, y in zip(a, b))


def check_against_manifest(root: Path, entries: Path) -> float:
    manifest = json.loads((root / "tools/lower_body_2p_select_source_manifest.json").read_text())
    require(manifest["screen_milo"] == SCREEN_MILO, "manifest screen MILO drifted")
    manifest_placers = manifest["placers"]
    worst = 0.0
    for name, spec in PLACERS.items():
        entry = entries / spec["entry"]
        require(entry.is_file(), f"missing stock BandPlacer entry {entry.name}")
        parsed = parse_placer(entry)
        expected = manifest_placers[name]
        require(parsed["target_group"] == expected["target_group"], f"{name}: target group drifted")
        require(parsed["target_mesh"] == expected["target_mesh"], f"{name}: target mesh drifted")
        for matrix_name in ("matrix0", "matrix1"):
            delta = max_delta(parsed[matrix_name], expected[matrix_name])
            require(delta <= TOLERANCE, f"{name}: {matrix_name} max delta {delta:.6f}")
            worst = max(worst, delta)
        require(parsed["target_mesh"] == "spot_ui.mesh", f"{name}: expected spot_ui target")
    return worst


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Decode stock 2P select BandPlacer rows and compare them to the source manifest."
    )
    parser.add_argument("--ark-dir", type=Path, default=None)
    args = parser.parse_args()
    root = root_dir()
    try:
        ark_dir = args.ark_dir.resolve() if args.ark_dir else default_ark_dir(root)
        entries = extract_entries(root, ark_dir)
        worst = check_against_manifest(root, entries)
    except RuntimeError as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        return 1

    print(
        "PASS lower_body_2p_select_source_assets "
        "source=stock_GH2_PS2 "
        f"screen_milo={SCREEN_MILO} "
        "entries=char_multi0.placer,char_multi1.placer "
        "matrix0_offset=46 matrix1_offset=94 "
        "target_group=mgs_camerafix.grp target_mesh=spot_ui.mesh "
        f"max_manifest_delta={worst:.6f}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
