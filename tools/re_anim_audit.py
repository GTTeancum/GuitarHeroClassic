from __future__ import annotations

import argparse
import json
import struct
import sys
import zlib
from dataclasses import dataclass
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))
from parse_v3_hdr import parse_hdr  # noqa: E402


MARKER = b"\xad\xde\xad\xde"


def u8(b: bytes, o: int) -> int:
    return b[o]


def u32(b: bytes, o: int) -> int:
    return struct.unpack_from("<I", b, o)[0]


def i32(b: bytes, o: int) -> int:
    return struct.unpack_from("<i", b, o)[0]


def f32(b: bytes, o: int) -> float:
    return struct.unpack_from("<f", b, o)[0]


def rdstr(b: bytes, o: int) -> tuple[str, int]:
    n = u32(b, o)
    o += 4
    if n > len(b) - o or n > 0x100000:
        raise ValueError(f"bad string length {n} at {o - 4:#x}")
    return b[o : o + n].decode("latin1"), o + n


@dataclass
class ArkEntry:
    full_path: str
    offset: int
    size: int


@dataclass
class MiloEntry:
    typ: str
    name: str
    offset: int
    size: int


def ark_entries(hdr: Path) -> list[ArkEntry]:
    info = parse_hdr(hdr)
    out: list[ArkEntry] = []
    for off, ni, fi, sz, _inf in info["entries"]:
        folder = info["string_at"](fi)
        name = info["string_at"](ni)
        full = f"{folder}/{name}" if folder else name
        out.append(ArkEntry(full, off, sz))
    return out


def read_ark_entry(ark: Path, e: ArkEntry) -> bytes:
    with ark.open("rb") as f:
        f.seek(e.offset)
        return f.read(e.size)


def inflate_milo(raw: bytes) -> bytes:
    structure = u32(raw, 0)
    first = u32(raw, 4)
    count = u32(raw, 8)
    if structure == 0:
        return raw[first:]
    if structure != 0xCBBEDEAF:
        raise ValueError(f"unsupported milo structure {structure:#x}")
    sizes = [u32(raw, 16 + 4 * i) & 0xFFFFFF for i in range(count)]
    pos = first
    out = bytearray()
    for sz in sizes:
        out += zlib.decompress(raw[pos : pos + sz], -15)
        pos += sz
    return bytes(out)


def parse_dir(payload: bytes) -> tuple[int, str, str, list[MiloEntry]]:
    p = 0
    ver = i32(payload, p)
    p += 4
    typ = name = ""
    if ver >= 24:
        typ, p = rdstr(payload, p)
        name, p = rdstr(payload, p)
        p += 8
    count = i32(payload, p)
    p += 4
    entries: list[list[Any]] = []
    for _ in range(count):
        et, p = rdstr(payload, p)
        en, p = rdstr(payload, p)
        entries.append([et, en])
    cursor = payload.find(MARKER, p)
    if cursor < 0:
        return ver, typ, name, []
    cursor += 4
    out: list[MiloEntry] = []
    for et, en in entries:
        end = payload.find(MARKER, cursor)
        if end < 0:
            end = len(payload)
        out.append(MiloEntry(et, en, cursor, end - cursor))
        cursor = end + 4
        if cursor >= len(payload):
            break
    return ver, typ, name, out


def resync_classic(payload: bytes, entries: list[MiloEntry]) -> None:
    start = next(
        (
            i
            for i, e in enumerate(entries)
            if e.typ == "CharDriver" and e.name == "main.drv"
        ),
        -1,
    )
    if start < 0:
        return
    first_marker = entries[start].offset - 4
    markers: list[int] = []
    pos = payload.find(MARKER, first_marker + 1)
    while pos >= 0:
        markers.append(pos)
        pos = payload.find(MARKER, pos + 1)
    if len(markers) < len(entries) - start:
        return
    for i in range(start, len(entries)):
        m = markers[i - start]
        next_m = markers[i - start + 1] if i - start + 1 < len(markers) else len(payload)
        entries[i].offset = m + 4
        entries[i].size = next_m - entries[i].offset


class Reader:
    def __init__(self, b: bytes):
        self.b = b
        self.p = 0

    def u8(self) -> int:
        v = u8(self.b, self.p)
        self.p += 1
        return v

    def i32(self) -> int:
        v = i32(self.b, self.p)
        self.p += 4
        return v

    def u32(self) -> int:
        v = u32(self.b, self.p)
        self.p += 4
        return v

    def f32(self) -> float:
        v = f32(self.b, self.p)
        self.p += 4
        return v

    def s(self) -> str:
        v, self.p = rdstr(self.b, self.p)
        return v

    def object_base(self) -> dict[str, Any]:
        rev = self.u32()
        sym = self.s()
        if self.p < len(self.b):
            nul = self.u8()
        else:
            nul = None
        return {"revision": rev, "symbol": sym, "terminator": nul}


def parse_lookat(body: bytes) -> dict[str, Any]:
    r = Reader(body)
    out: dict[str, Any] = {"version": r.i32(), "object": r.object_base()}
    out.update(
        flags=r.i32(),
        weight=r.f32(),
        source=r.s(),
        target=r.s(),
        driven=r.s(),
        unknown=r.i32(),
        rate=r.f32(),
        min_x=r.f32(),
        max_x=r.f32(),
        min_z=r.f32(),
        max_z=r.f32(),
        offset_x=r.f32(),
        offset_z=r.f32(),
        max_radius=r.f32(),
    )
    out["tail_hex"] = body[r.p :].hex()
    out["consumed"] = r.p == len(body)
    return out


def parse_eyes(body: bytes) -> dict[str, Any]:
    r = Reader(body)
    out: dict[str, Any] = {"version": r.i32(), "object": r.object_base()}
    n = r.u32()
    out["lookats"] = [r.s() for _ in range(n)]
    if r.p + 4 <= len(body):
        out["upperlid_or_blink_bone"] = r.s()
    out["tail_hex"] = body[r.p :].hex()
    out["consumed"] = r.p == len(body)
    return out


def parse_driver_after_version(r: Reader, ver: int) -> dict[str, Any]:
    out: dict[str, Any] = {"version": ver, "object": r.object_base()}
    out["weightable_version"] = r.i32()
    out["weight"] = r.f32()
    out["weight_prop"] = r.s()
    if ver < 3:
        out["legacy_unknown"] = r.i32()
    out["servo"] = r.s()
    out["clip_milo"] = r.s()
    if ver > 1 and r.p < len(r.b):
        out["enabled"] = r.u8()
    if ver > 4 and r.p < len(r.b):
        out["flag_v5"] = r.u8()
    return out


def parse_driver(body: bytes, midi: bool) -> dict[str, Any]:
    r = Reader(body)
    out: dict[str, Any] = {}
    if midi:
        out["midi_version"] = r.i32()
    out.update(parse_driver_after_version(r, r.i32()))
    if midi and r.p + 4 <= len(body):
        out["midi_reserved"] = r.u32()
    out["tail_hex"] = body[r.p :].hex()
    out["consumed"] = r.p == len(body)
    return out


def parse_lip(body: bytes) -> dict[str, Any]:
    r = Reader(body)
    ver = r.i32()
    out: dict[str, Any] = {"version": ver}
    out["object"] = r.object_base()
    if ver > 4:
        out["weightable_version"] = r.i32()
        out["weight"] = r.f32()
        out["weight_prop"] = r.s()
    elif ver > 1:
        out["driver_ref"] = r.s()
    out["facefx_path"] = r.s()
    out["viseme_milo"] = r.s()
    targets = []
    for _ in range(r.u32()):
        targets.append({"object": r.s(), "prop_type": r.i32(), "property": r.s()})
    out["targets"] = targets
    out["tail_hex"] = body[r.p :].hex()
    out["consumed"] = r.p == len(body)
    return out


def parse_simple_strings(body: bytes, float_first: bool = False) -> dict[str, Any]:
    r = Reader(body)
    out: dict[str, Any] = {"version": r.i32(), "object": r.object_base()}
    if float_first:
        out["value"] = r.f32()
    vals = []
    while r.p < len(body):
        vals.append(r.s())
    out["strings"] = vals
    out["tail_hex"] = body[r.p :].hex()
    out["consumed"] = r.p == len(body)
    return out


def parse_weight_setter(body: bytes) -> dict[str, Any]:
    r = Reader(body)
    out: dict[str, Any] = {"version": r.i32(), "object": r.object_base()}
    out["weightable_version"] = r.i32()
    out["weight"] = r.f32()
    out["weight_prop"] = r.s()
    out["driver"] = r.s()
    out["flags_or_mask"] = r.u32() if r.p + 4 <= len(body) else None
    out["tail_hex"] = body[r.p :].hex()
    out["consumed"] = r.p == len(body)
    return out


def parse_ik_hand_fields(
    r: Reader, body_len: int, consume_optional_unknown: bool
) -> dict[str, Any]:
    out: dict[str, Any] = {"version": r.i32(), "object": r.object_base()}
    out["weightable_version"] = r.i32()
    out["weight"] = r.f32()
    out["weight_prop"] = r.s()
    out["hand"] = r.s()
    out["target"] = r.s()
    out["enable_pos"] = r.u8()
    out["enable_rot"] = r.u8()
    if consume_optional_unknown and r.p < body_len:
        out["unknown_flag"] = r.u8()
    return out


def parse_ik_hand(body: bytes) -> dict[str, Any]:
    r = Reader(body)
    out = parse_ik_hand_fields(r, len(body), True)
    out["tail_hex"] = body[r.p :].hex()
    out["consumed"] = r.p == len(body)
    return out


def parse_ik_foot(body: bytes) -> dict[str, Any]:
    r = Reader(body)
    out: dict[str, Any] = {"version": r.i32(), "object": r.object_base()}
    out["char_ik_hand"] = parse_ik_hand_fields(r, len(body), False)
    if out["version"] < 6:
        out["legacy_symbol"] = r.s()
    if out["version"] < 5:
        legacy_ints = []
        if out["version"] > 1:
            legacy_ints.append(r.i32())
        if out["version"] > 2:
            legacy_ints.append(r.i32())
        if out["version"] > 3:
            legacy_ints.append(r.i32())
        out["legacy_ints"] = legacy_ints
    else:
        out["data"] = r.s()
        out["data_index"] = r.i32()
    out["tail_hex"] = body[r.p :].hex()
    out["consumed"] = r.p == len(body)
    return out


def parse_ik_midi(body: bytes) -> dict[str, Any]:
    r = Reader(body)
    out: dict[str, Any] = {"version": r.i32(), "object": r.object_base()}
    out["target"] = r.s()
    out["tail_hex"] = body[r.p :].hex()
    out["consumed"] = r.p == len(body)
    return out


def parse_empty_object(body: bytes) -> dict[str, Any]:
    r = Reader(body)
    out: dict[str, Any] = {"version": r.i32(), "object": r.object_base()}
    out["tail_hex"] = body[r.p :].hex()
    out["consumed"] = r.p == len(body)
    return out


def parse_ik_rod(body: bytes) -> dict[str, Any]:
    r = Reader(body)
    out: dict[str, Any] = {"version": r.i32(), "object": r.object_base()}
    out["left_end"] = r.s()
    out["right_end"] = r.s()
    out["dest_pos"] = r.f32()
    out["side_axis"] = r.s()
    out["vertical"] = r.u8()
    out["dest"] = r.s()
    out["params"] = [r.f32() for _ in range(12)]
    out["tail_hex"] = body[r.p :].hex()
    out["consumed"] = r.p == len(body)
    return out


def parse_hair(body: bytes) -> dict[str, Any]:
    r = Reader(body)
    out: dict[str, Any] = {"version": r.i32(), "object": r.object_base()}
    out["globals"] = [r.f32() for _ in range(6)]
    groups = []
    for _ in range(r.u32()):
        g: dict[str, Any] = {"root_mesh": r.s(), "root_offset": r.f32(), "points": []}
        for _p in range(r.u32()):
            pt = {
                "pos": [r.f32(), r.f32(), r.f32()],
                "mesh": r.s(),
                "length": r.f32(),
                "flags_or_mode": r.u32(),
                "parent": r.s(),
                "radius": r.f32(),
            }
            if out["version"] > 1:
                pt["extra"] = r.f32()
            g["points"].append(pt)
        g["limits_or_mats"] = [r.f32() for _ in range(18)]
        groups.append(g)
    out["groups"] = groups
    if r.p < len(body):
        out["enabled"] = r.u8()
    out["tail_hex"] = body[r.p :].hex()
    out["consumed"] = r.p == len(body)
    return out


PARSERS = {
    "CharLookAt": lambda b: parse_lookat(b),
    "CharEyes": lambda b: parse_eyes(b),
    "FaceFxLipSyncServo": lambda b: parse_lip(b),
    "CharDriver": lambda b: parse_driver(b, False),
    "CharDriverMidi": lambda b: parse_driver(b, True),
    "CharUpperTwist": lambda b: parse_simple_strings(b),
    "CharForeTwist": lambda b: parse_simple_strings(b, True),
    "CharWeightSetter": parse_weight_setter,
    "CharIKHand": parse_ik_hand,
    "CharIKFoot": parse_ik_foot,
    "CharIKMidi": parse_ik_midi,
    "CharWalk": parse_empty_object,
    "CharServoBone": parse_empty_object,
    "CharIKRod": parse_ik_rod,
    "CharHair": parse_hair,
}


def parse_clip_group(body: bytes) -> dict[str, Any]:
    r = Reader(body)
    out: dict[str, Any] = {"version": r.i32(), "object": r.object_base()}
    out["clips"] = [r.s() for _ in range(r.u32())]
    if r.p + 4 <= len(body):
        out["reserved_or_sentinel"] = r.u32()
    out["tail_hex"] = body[r.p :].hex()
    out["consumed"] = r.p == len(body)
    return out


def parse_clip_filter(body: bytes) -> dict[str, Any]:
    r = Reader(body)
    out: dict[str, Any] = {"object": r.object_base()}
    out["tail_hex"] = body[r.p :].hex()
    out["consumed"] = r.p == len(body)
    return out


def parse_char_bone(body: bytes) -> dict[str, Any]:
    r = Reader(body)
    out: dict[str, Any] = {"version": r.i32(), "object": r.object_base()}
    out["trans_version"] = r.i32()
    out["local_xfm"] = [r.f32() for _ in range(12)]
    out["world_xfm"] = [r.f32() for _ in range(12)]
    out["constraint_or_flags"] = r.object_base()
    out["parent"] = r.s()
    if r.p + 10 <= len(body):
        out["unknown_flag"] = r.u8()
        out["unknown_a"] = r.u32()
        out["unknown_b"] = r.u32()
        out["unknown_tail_byte"] = r.u8()
    out["tail_hex"] = body[r.p :].hex()
    out["consumed"] = r.p == len(body)
    return out


def audit(hdr: Path, ark: Path) -> dict[str, Any]:
    result: dict[str, Any] = {"entries": 0, "objects": {}, "failures": []}
    paths = [
        e
        for e in ark_entries(hdr)
        if e.full_path.startswith("char/")
        and e.full_path.endswith(".milo_ps2")
        and "/og/gen/" in e.full_path
    ]
    result["entries"] = len(paths)
    for e in paths:
        try:
            payload = inflate_milo(read_ark_entry(ark, e))
            _dv, _dt, _dn, ents = parse_dir(payload)
            if e.full_path == "char/classic/og/gen/classic.milo_ps2":
                resync_classic(payload, ents)
        except Exception as ex:
            result["failures"].append({"path": e.full_path, "error": str(ex)})
            continue
        for me in ents:
            if me.typ not in PARSERS:
                continue
            body = payload[me.offset : me.offset + me.size]
            try:
                parsed = PARSERS[me.typ](body)
            except Exception as ex:
                result["failures"].append(
                    {"path": e.full_path, "type": me.typ, "name": me.name, "error": str(ex)}
                )
                continue
            result["objects"].setdefault(me.typ, []).append(
                {"path": e.full_path, "name": me.name, "size": me.size, **parsed}
            )
            if not parsed.get("consumed", False):
                result["failures"].append(
                    {
                        "path": e.full_path,
                        "type": me.typ,
                        "name": me.name,
                        "error": "unconsumed tail",
                        "tail_hex": parsed.get("tail_hex", ""),
                    }
                )
    return result


def audit_anims(hdr: Path, ark: Path) -> dict[str, Any]:
    parsers = {
        "CharClipGroup": parse_clip_group,
        "CharClipFilter": parse_clip_filter,
        "CharBone": parse_char_bone,
    }
    result: dict[str, Any] = {"entries": 0, "objects": {}, "failures": []}
    paths = [
        e
        for e in ark_entries(hdr)
        if (e.full_path.startswith("char/") or e.full_path.startswith("../../system/run/char/"))
        and "/anims/gen/" in e.full_path
        and e.full_path.endswith(".milo_ps2")
    ]
    result["entries"] = len(paths)
    for e in paths:
        try:
            payload = inflate_milo(read_ark_entry(ark, e))
            _dv, _dt, _dn, ents = parse_dir(payload)
        except Exception as ex:
            result["failures"].append({"path": e.full_path, "error": str(ex)})
            continue
        for me in ents:
            if me.typ not in parsers:
                continue
            body = payload[me.offset : me.offset + me.size]
            try:
                parsed = parsers[me.typ](body)
            except Exception as ex:
                result["failures"].append(
                    {"path": e.full_path, "type": me.typ, "name": me.name, "error": str(ex)}
                )
                continue
            result["objects"].setdefault(me.typ, []).append(
                {"path": e.full_path, "name": me.name, "size": me.size, **parsed}
            )
            if not parsed.get("consumed", False):
                result["failures"].append(
                    {
                        "path": e.full_path,
                        "type": me.typ,
                        "name": me.name,
                        "error": "unconsumed tail",
                        "tail_hex": parsed.get("tail_hex", ""),
                    }
                )
    return result


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--hdr", default=r"C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\GEN\MAIN.HDR")
    ap.add_argument("--ark", default=r"C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA)\GEN\MAIN_0.ARK")
    ap.add_argument("--out")
    ap.add_argument("--anims", action="store_true")
    ns = ap.parse_args()
    res = audit_anims(Path(ns.hdr), Path(ns.ark)) if ns.anims else audit(Path(ns.hdr), Path(ns.ark))
    text = json.dumps(res, indent=2)
    if ns.out:
        Path(ns.out).write_text(text, encoding="utf-8")
    else:
        print(text)


if __name__ == "__main__":
    main()
