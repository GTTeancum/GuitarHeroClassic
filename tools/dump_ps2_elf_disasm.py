#!/usr/bin/env python3
"""Disassemble a bounded file-backed virtual-address range from a PS2 ELF."""

from __future__ import annotations

import argparse
from pathlib import Path

from capstone import CS_ARCH_MIPS, CS_MODE_LITTLE_ENDIAN, CS_MODE_MIPS32, Cs
from elftools.elf.elffile import ELFFile


def parse_int(text: str) -> int:
    return int(text, 0)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf", type=Path)
    parser.add_argument("--start", type=parse_int, required=True)
    parser.add_argument("--count", type=int, default=64)
    args = parser.parse_args()
    if args.start & 3:
        parser.error("--start must be four-byte aligned")
    if args.count < 1 or args.count > 4096:
        parser.error("--count must be between 1 and 4096")

    byte_count = args.count * 4
    with args.elf.open("rb") as stream:
        elf = ELFFile(stream)
        for segment in elf.iter_segments():
            if segment["p_type"] != "PT_LOAD":
                continue
            start = int(segment["p_vaddr"])
            end = start + int(segment["p_filesz"])
            if start <= args.start and args.start + byte_count <= end:
                offset = args.start - start
                data = segment.data()[offset : offset + byte_count]
                break
        else:
            raise RuntimeError("requested virtual-address range is not file-backed")

    disassembler = Cs(CS_ARCH_MIPS, CS_MODE_MIPS32 | CS_MODE_LITTLE_ENDIAN)
    disassembler.skipdata = True
    for instruction in disassembler.disasm(data, args.start):
        print(
            f"0x{instruction.address:08x}: "
            f"{instruction.mnemonic:<8} {instruction.op_str}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
