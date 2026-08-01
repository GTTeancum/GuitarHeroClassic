#!/usr/bin/env python3
"""Verify the app's 2P select diagnostic placers match the source manifest."""

from __future__ import annotations

import json
from pathlib import Path
import re
import sys


TOLERANCE = 0.00025

EXPECTED_BY_PLAYER = {
    0: "char_multi0.placer",
    1: "char_multi1.placer",
}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def root_dir() -> Path:
    return Path(__file__).resolve().parent.parent


def parse_float_list(text: str) -> list[float]:
    tokens = re.findall(r"[-+]?(?:\d+\.\d*|\d+|\.\d+)(?:[eE][-+]?\d+)?f?", text)
    return [float(token.rstrip("f")) for token in tokens]


def compact_matrix16_to_source_matrix12(matrix16: list[float]) -> list[float]:
    require(len(matrix16) == 16, f"expected 16 app matrix floats, got {len(matrix16)}")
    require(
        max(abs(matrix16[i]) for i in (3, 7, 11)) <= TOLERANCE,
        "app placer matrix must have zero perspective column",
    )
    require(abs(matrix16[15] - 1.0) <= TOLERANCE, "app placer matrix must keep w=1")
    return [
        matrix16[0], matrix16[1], matrix16[2],
        matrix16[4], matrix16[5], matrix16[6],
        matrix16[8], matrix16[9], matrix16[10],
        matrix16[12], matrix16[13], matrix16[14],
    ]


def parse_app_placers(app_main: str) -> dict[int, tuple[str, list[float]]]:
    pattern = re.compile(
        r"if\s*\(player\s*==\s*(?P<player>\d+)\)\s*\{\s*"
        r"return\s+CharSelectPlacer\s*\{\s*"
        r"(?P<returned_player>\d+)\s*,\s*"
        r"\"(?P<name>[^\"]+)\"\s*,\s*"
        r"\{(?P<matrix>.*?)\}\s*\}\s*;",
        re.DOTALL,
    )
    placers: dict[int, tuple[str, list[float]]] = {}
    for match in pattern.finditer(app_main):
        player = int(match.group("player"))
        returned_player = int(match.group("returned_player"))
        name = match.group("name")
        matrix16 = parse_float_list(match.group("matrix"))
        require(player == returned_player, f"{name}: branch/return player mismatch")
        require(player not in placers, f"duplicate app placer branch for player {player}")
        placers[player] = (name, compact_matrix16_to_source_matrix12(matrix16))
    return placers


def max_delta(a: list[float], b: list[float]) -> float:
    require(len(a) == len(b), "matrix lengths differ")
    return max(abs(x - y) for x, y in zip(a, b))


def main() -> int:
    root = root_dir()
    try:
        app_main = (root / "engine/src/app/app_main.cpp").read_text(encoding="utf-8")
        manifest = json.loads(
            (root / "tools/lower_body_2p_select_source_manifest.json").read_text(
                encoding="utf-8"
            )
        )
        app_placers = parse_app_placers(app_main)
        require(set(app_placers) == set(EXPECTED_BY_PLAYER), "app placer branches drifted")
        worst = 0.0
        for player, expected_name in EXPECTED_BY_PLAYER.items():
            name, app_matrix = app_placers[player]
            require(name == expected_name, f"player {player}: expected {expected_name}, got {name}")
            expected_matrix = manifest["placers"][name]["matrix0"]
            delta = max_delta(app_matrix, expected_matrix)
            require(delta <= TOLERANCE, f"{name}: app matrix0 delta {delta:.6f}")
            worst = max(worst, delta)
        require("--char-2p-select-placer" in app_main, "missing diagnostic CLI option")
        require("--char-2p-select-event" in app_main, "missing diagnostic event option")
        require("applied_placer=%s" in app_main, "missing applied placer log")
    except RuntimeError as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        return 1

    print(
        "PASS lower_body_2p_select_app_placer "
        "players=0,1 "
        "app_placers=char_multi0.placer,char_multi1.placer "
        "manifest_matrix=matrix0 "
        f"max_manifest_delta={worst:.6f}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
