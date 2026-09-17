"""Repair generated Mat27 lighting without rebuilding accepted character rigs.

Uses native MILO entry boundaries. Only use_environment/prelit bytes change in
the decompressed model; textures, geometry, binds and animation remain intact.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import subprocess
import tempfile
import zlib
from pathlib import Path

from refresh_rb2_baked_textures import pack_payload


def inflate(raw: bytes) -> bytes:
    magic, offset, count, limit = struct.unpack_from("<4I", raw)
    if magic != 0xCBBEDEAF or not count or count > (len(raw) - 16) // 4:
        raise ValueError("expected compressed MILO_B")
    if offset < 16 + 4 * count or offset > len(raw):
        raise ValueError("invalid block table")
    result = bytearray()
    for size in struct.unpack_from(f"<{count}I", raw, 16):
        if size > len(raw) - offset:
            raise ValueError("truncated block")
        block = zlib.decompress(raw[offset:offset + size], -15)
        if len(block) > limit:
            raise ValueError("block exceeds declared limit")
        result.extend(block)
        offset += size
    if offset != len(raw):
        raise ValueError("unexpected trailing data")
    return bytes(result)


def lighting_offset(body: bytes) -> int:
    if len(body) < 13 or struct.unpack_from("<2I", body) != (27, 0):
        raise ValueError("expected Mat27 with ObjectFields revision 0")
    name_size = struct.unpack_from("<I", body, 8)[0]
    props = 12 + name_size
    offset = props + 1 + 4 + 16
    if offset + 2 > len(body) or body[props] != 0:
        raise ValueError("unsupported/truncated material properties")
    if any(value not in (0, 1) for value in body[offset:offset + 2]):
        raise ValueError("invalid material lighting booleans")
    return offset


def repair_payload(payload: bytes, materials: dict[str, bytes]):
    if not materials:
        raise ValueError("no character materials")
    result = bytearray(payload)
    report = []
    spans = []
    groups = {}
    for name, body in sorted(materials.items()):
        groups.setdefault(body, []).append(name)
    for body, names in groups.items():
        # Atlas materials can have byte-identical bodies under different names.
        # Native inventory must account for every occurrence; never patch an
        # unexplained matching sequence elsewhere in the payload.
        local_offset = lighting_offset(body)
        before = list(body[local_offset:local_offset + 2])
        after = before if before[0] else [1, 0]
        if payload.count(body) != len(names):
            raise ValueError(f"material occurrence count differs from native inventory: {names}")
        offsets = []
        cursor = 0
        for _ in names:
            start = payload.index(body, cursor)
            end = start + len(body)
            if any(start < other_end and other_start < end for other_start, other_end in spans):
                raise ValueError("overlapping material bodies")
            spans.append((start, end))
            offset = start + local_offset
            result[offset:offset + 2] = bytes(after)
            offsets.append(offset)
            cursor = end
        for name in names:
            report.append(dict(material=name, identical_body_offsets=offsets,
                               before=before, after=after))
    allowed = {offset + delta for row in report
               for offset in row['identical_body_offsets'] for delta in (0, 1)}
    changed = {i for i, (a, b) in enumerate(zip(payload, result)) if a != b}
    if not changed <= allowed:
        raise AssertionError("non-lighting payload changed")
    return bytes(result), report


def native(tool: Path, *args: str) -> str:
    run = subprocess.run([str(tool), *map(str, args)], capture_output=True, text=True)
    if run.returncode:
        raise ValueError(f"native MILO command failed: {run.stderr[-1500:]}")
    return run.stdout


def repair_model(tool: Path, model: Path, apply: bool = False) -> dict:
    raw = model.read_bytes()
    payload = inflate(raw)
    listing = native(tool, "list", model)
    if "boundaries  : exact" not in listing:
        raise ValueError("native MILO entry boundaries must be exact")
    names = re.findall(r"^\s+Mat\s+size=\d+\s+body=\d+\s+(\S+)\s*$", listing, re.M)
    with tempfile.TemporaryDirectory(prefix="ghogx-material-lighting-") as scratch:
        extracted = Path(scratch) / "native"
        native(tool, "extract", model, "--out", extracted)
        materials = {}
        for name in names:
            path = extracted / ("Mat__" + name.replace("/", "_").replace("\\", "_"))
            materials[name] = path.read_bytes()
        fixed, rows = repair_payload(payload, materials)
        output = raw if fixed == payload else pack_payload(fixed)
        candidate = Path(scratch) / "candidate.milo_ps2"
        candidate.write_bytes(output)
        native(tool, "verify", candidate)
        if inflate(output) != fixed:
            raise AssertionError("compressed payload round trip failed")
    if apply and output != raw:
        model.write_bytes(output)
    return dict(model=str(model), applied=apply, materials=rows,
                changed_bytes=sum(a != b for a, b in zip(payload, fixed)),
                only_material_lighting_changed=True,
                before_sha256=hashlib.sha256(raw).hexdigest(),
                after_sha256=hashlib.sha256(output).hexdigest())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--milo-tool", type=Path, required=True)
    parser.add_argument("--model", type=Path, required=True)
    parser.add_argument("--audit", type=Path, required=True)
    parser.add_argument("--apply", action="store_true")
    args = parser.parse_args()
    report = repair_model(args.milo_tool, args.model, args.apply)
    args.audit.write_text(json.dumps(report, indent=2) + "\n")
    print(f"{len(report['materials'])} materials; {report['changed_bytes']} lighting bytes changed; applied={args.apply}")


if __name__ == "__main__":
    main()
