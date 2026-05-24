#!/usr/bin/env python3
"""
Parser for Harmonix v3 ARK headers (PS2 GH1, GH2, GH80s).

Layout (little-endian throughout):
  u32 version                (must be 3)
  u32 ??? (flag, observed 1)
  u32 ark_part_count
  u32[ark_part_count] part_sizes      -- size in bytes of each main_N.ark

Then the string blob:
  u32 string_blob_size
  u8[string_blob_size] string_blob    -- null-terminated names concatenated

Then string offset table:
  u32 string_offset_count
  u32[string_offset_count] string_offsets

Then file entry table:
  u32 entry_count
  entry[entry_count] {
    u32 ark_offset       -- offset within the concatenated ARK space
    u32 name_idx         -- index into string offset table
    u32 folder_idx       -- index into string offset table (0xFFFFFFFF if none / root)
    u32 size             -- file size
    u32 inflated_size    -- size after decompression (== size if uncompressed)
  }

Usage:
  python parse_v3_hdr.py <path_to_main.hdr> [--list] [--limit N]
"""
import argparse
import struct
import sys
from pathlib import Path


def parse_hdr(path: Path):
    data = path.read_bytes()
    p = 0

    def u32():
        nonlocal p
        v = struct.unpack_from("<I", data, p)[0]
        p += 4
        return v

    version = u32()
    flag = u32()
    part_count = u32()
    parts = [u32() for _ in range(part_count)]

    str_blob_size = u32()
    str_blob = data[p:p + str_blob_size]
    p += str_blob_size

    str_off_count = u32()
    str_offsets = [u32() for _ in range(str_off_count)]

    entry_count = u32()
    entries = []
    for _ in range(entry_count):
        ark_offset = u32()
        name_idx = u32()
        folder_idx = u32()
        size = u32()
        inflated = u32()
        entries.append((ark_offset, name_idx, folder_idx, size, inflated))

    leftover = len(data) - p

    def string_at(idx):
        if idx == 0xFFFFFFFF:
            return ""
        if idx >= len(str_offsets):
            return f"<BAD_IDX_{idx}>"
        off = str_offsets[idx]
        end = str_blob.find(b"\x00", off)
        return str_blob[off:end].decode("ascii", errors="replace")

    return {
        "version": version,
        "flag": flag,
        "parts": parts,
        "str_blob_size": str_blob_size,
        "str_offsets": str_offsets,
        "entries": entries,
        "leftover_bytes": leftover,
        "string_at": string_at,
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("hdr", type=Path)
    ap.add_argument("--list", action="store_true", help="list all file paths")
    ap.add_argument("--limit", type=int, default=30, help="when --list, print first N")
    ap.add_argument("--ext-summary", action="store_true",
                    help="count file extensions across all entries")
    args = ap.parse_args()

    if not args.hdr.exists():
        print(f"ERROR: {args.hdr} not found", file=sys.stderr)
        return 2

    info = parse_hdr(args.hdr)
    print(f"HDR: {args.hdr}")
    print(f"  version          : {info['version']}")
    print(f"  flag             : {info['flag']}")
    print(f"  ark_parts        : {len(info['parts'])}")
    for i, sz in enumerate(info['parts']):
        print(f"    main_{i}.ark size : {sz} bytes")
    print(f"  str_blob_size    : {info['str_blob_size']}")
    print(f"  str_offset_count : {len(info['str_offsets'])}")
    print(f"  entry_count      : {len(info['entries'])}")
    print(f"  leftover_bytes   : {info['leftover_bytes']} (should be 0 if layout is right)")

    if args.ext_summary:
        from collections import Counter
        exts = Counter()
        for (_, ni, _, _, _) in info["entries"]:
            name = info["string_at"](ni)
            ext = name.rsplit(".", 1)[-1] if "." in name else "<noext>"
            exts[ext.lower()] += 1
        print("\nExtension summary:")
        for ext, n in exts.most_common():
            print(f"  .{ext:<12}  {n}")

    if args.list:
        print(f"\nFirst {args.limit} entries:")
        for (off, ni, fi, sz, inf) in info["entries"][:args.limit]:
            folder = info["string_at"](fi)
            name = info["string_at"](ni)
            full = f"{folder}/{name}" if folder else name
            comp = "" if sz == inf else f" inflated={inf}"
            print(f"  off=0x{off:010x} size={sz:>10}{comp}  {full}")


if __name__ == "__main__":
    sys.exit(main() or 0)
