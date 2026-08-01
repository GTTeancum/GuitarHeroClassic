#!/usr/bin/env python3
"""Disassemble ranges emitted by trace_pcsx2_ee_ranges_read_only.py."""

from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path

import rabbitizer


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("manifest", type=Path)
    parser.add_argument("--instructions", type=int, default=12)
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    for row in manifest["ranges"]:
        print(f"[{row['name']}]")
        data = bytes.fromhex(row["bytes_hex"])
        address = int(row["guest_address"], 16)
        count = min(args.instructions, len(data) // 4)
        for index in range(count):
            offset = index * 4
            word = struct.unpack_from("<I", data, offset)[0]
            instruction = rabbitizer.Instruction(word)
            print(f"{address + offset:08x}: {instruction.disassemble()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
