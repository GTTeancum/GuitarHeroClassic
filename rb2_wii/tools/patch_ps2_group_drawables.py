#!/usr/bin/env python3
"""Append drawables to a GH2 PS2 standalone Group without rewriting it."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


STOCK_DRAWABLES = [
    "guitar.mesh",
    "guitar_fire.mesh",
    "guitar_strings.mesh",
]


def encode(drawables: list[str]) -> bytes:
    output = bytearray(struct.pack("<I", len(drawables)))
    for name in drawables:
        raw = name.encode("ascii")
        output += struct.pack("<I", len(raw))
        output += raw
    return bytes(output)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("drawables", nargs="*")
    args = parser.parse_args()

    source = args.input.read_bytes()
    needle = encode(STOCK_DRAWABLES)
    count = source.count(needle)
    if count != 1:
        raise RuntimeError(
            f"{args.input}: expected one stock drawable block, found {count}"
        )
    additions = list(dict.fromkeys(args.drawables))
    if any(name in STOCK_DRAWABLES for name in additions):
        raise RuntimeError("an appended drawable duplicates a stock drawable")
    patched = source.replace(
        needle, encode([*STOCK_DRAWABLES, *additions]), 1
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(patched)
    print(
        "PATCHED_PS2_GROUP "
        f"input={args.input} output={args.output} "
        f"drawables={len(STOCK_DRAWABLES) + len(additions)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
