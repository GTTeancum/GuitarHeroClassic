#!/usr/bin/env python3
"""Disassemble a bounded virtual-address range from a little-endian PS2 ELF."""

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
    parser.add_argument("--end", type=parse_int, required=True)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    if args.end <= args.start:
        parser.error("--end must be greater than --start")
    if args.end - args.start > 16 * 1024 * 1024:
        parser.error("range exceeds 16 MiB safety bound")

    with args.elf.open("rb") as stream:
        elf = ELFFile(stream)
        containing = None
        for segment in elf.iter_segments():
            if segment["p_type"] != "PT_LOAD":
                continue
            start = int(segment["p_vaddr"])
            end = start + int(segment["p_filesz"])
            if start <= args.start and args.end <= end:
                containing = segment
                break
        if containing is None:
            raise RuntimeError("requested virtual-address range is not file-backed")
        file_offset = int(containing["p_offset"]) + args.start - int(
            containing["p_vaddr"]
        )
        stream.seek(file_offset)
        code = stream.read(args.end - args.start)

    disassembler = Cs(
        CS_ARCH_MIPS,
        CS_MODE_MIPS32 | CS_MODE_LITTLE_ENDIAN,
    )
    disassembler.detail = False
    # Capstone does not decode every Emotion Engine MMI instruction.  Keep the
    # surrounding scalar control flow visible instead of stopping at one.
    disassembler.skipdata = True
    lines = [
        f"# file={args.elf.resolve()}",
        f"# va=0x{args.start:08x}..0x{args.end:08x} bytes={len(code)}",
    ]
    lines.extend(
        f"0x{insn.address:08x}: {insn.mnemonic:<10} {insn.op_str}".rstrip()
        for insn in disassembler.disasm(code, args.start)
    )
    text = "\n".join(lines) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text, encoding="utf-8")
    else:
        print(text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
