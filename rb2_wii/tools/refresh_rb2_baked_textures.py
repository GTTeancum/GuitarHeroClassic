#!/usr/bin/env python3
"""Refresh same-layout baked textures without rebuilding an accepted rig.

Requires native-extracted base/donor directories. Only uniquely matching,
same-size Tex bodies are replaced; every other payload byte stays identical.
Native milo_tool verify and DLC index validation must precede deployment.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zlib
from pathlib import Path


def replace_textures(base: Path, donor: Path) -> tuple[bytes, list[dict]]:
    payload = (base / "_payload.bin").read_bytes()
    replacements = []
    changes = []
    for path in sorted(donor.glob("Tex__*")):
        target = base / path.name
        if not target.is_file():
            raise ValueError(f"missing existing texture: {path.name}")
        old, new = target.read_bytes(), path.read_bytes()
        if len(old) != len(new):
            raise ValueError(f"texture layout changed: {path.name}")
        if payload.count(old) != 1:
            raise ValueError(f"texture body not unique: {path.name}")
        offset = payload.index(old)
        replacements.append((offset, old, new))
        changes.append({"texture": path.name, "bytes": len(new),
                        "changed": old != new,
                        "sha256": hashlib.sha256(new).hexdigest()})
    if not changes:
        raise ValueError("donor contains no textures")
    result = bytearray(payload)
    previous_end = 0
    for offset, old, new in sorted(replacements):
        if offset < previous_end:
            raise ValueError("overlapping texture bodies")
        result[offset:offset + len(old)] = new
        previous_end = offset + len(old)
    return bytes(result), changes


def pack_payload(payload: bytes) -> bytes:
    blocks = []
    for offset in range(0, len(payload), 65536):
        compressor = zlib.compressobj(9, zlib.DEFLATED, -15)
        blocks.append(compressor.compress(payload[offset:offset + 65536])
                      + compressor.flush())
    header = struct.pack("<4I", 0xCBBEDEAF, 16 + 4 * len(blocks), len(blocks), 65536)
    return header + b"".join(struct.pack("<I", len(b)) for b in blocks) + b"".join(blocks)


def validate_rgb_only(base: Path, donor: Path) -> None:
    for path in donor.glob("Tex__*"):
        old, new = (base / path.name).read_bytes(), path.read_bytes()
        if len(old) < 25 or len(new) != len(old):
            raise ValueError("invalid same-layout Tex body")
        width, height, bpp = struct.unpack_from("<3i", old, 13)
        offset = len(old) - width * height * 4
        if struct.unpack_from("<I", old)[0] != 10 or bpp != 32 or min(width, height) < 1 or offset < 25:
            raise ValueError("RGB-only refresh requires revision-10 RGBA textures")
        if old[:offset] != new[:offset] or old[offset + 3::4] != new[offset + 3::4]:
            raise ValueError(f"texture header or alpha changed: {path.name}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base-extracted", type=Path, required=True)
    parser.add_argument("--donor-extracted", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--audit", type=Path, required=True)
    args = parser.parse_args()
    validate_rgb_only(args.base_extracted, args.donor_extracted)
    payload, changes = replace_textures(args.base_extracted, args.donor_extracted)
    args.out.write_bytes(pack_payload(payload))
    args.audit.write_text(json.dumps({"textures": changes,
        "non_texture_payload_unchanged": True,
        "texture_headers_and_alpha_unchanged": True,
        "output_sha256": hashlib.sha256(args.out.read_bytes()).hexdigest()}, indent=2) + "\n")


if __name__ == "__main__":
    main()
