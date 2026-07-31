#!/usr/bin/env python3
"""Scan PCSX2 EE memory for bounded ASCII strings without controlling PCSX2.

This tool opens an existing PCSX2 process with PROCESS_VM_READ and
PROCESS_QUERY_LIMITED_INFORMATION only. It does not enumerate/control windows,
send input, or write host/emulated memory.
"""

from __future__ import annotations

import argparse
import ctypes
import json
import struct
from ctypes import wintypes
from datetime import datetime, timezone
from pathlib import Path


PROCESS_VM_READ = 0x0010
PROCESS_QUERY_LIMITED_INFORMATION = 0x1000

kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
kernel32.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
kernel32.OpenProcess.restype = wintypes.HANDLE
kernel32.ReadProcessMemory.argtypes = [
    wintypes.HANDLE,
    wintypes.LPCVOID,
    wintypes.LPVOID,
    ctypes.c_size_t,
    ctypes.POINTER(ctypes.c_size_t),
]
kernel32.ReadProcessMemory.restype = wintypes.BOOL
kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
kernel32.CloseHandle.restype = wintypes.BOOL


def parse_int(text: str) -> int:
    return int(text, 0)


def parse_address_size(text: str) -> tuple[int, int]:
    parts = text.split(":")
    if len(parts) != 2:
        raise argparse.ArgumentTypeError("range must be ADDRESS:SIZE")
    start, size = (parse_int(part) for part in parts)
    if size <= 0 or size > 0x00400000:
        raise argparse.ArgumentTypeError("range size must be 1..0x400000")
    return start, size


def read_exact(process: int, address: int, size: int) -> bytes:
    buffer = ctypes.create_string_buffer(size)
    read = ctypes.c_size_t()
    if not kernel32.ReadProcessMemory(
        process,
        ctypes.c_void_p(address),
        buffer,
        size,
        ctypes.byref(read),
    ):
        error = ctypes.get_last_error()
        raise OSError(error, f"ReadProcessMemory failed at 0x{address:x}")
    return buffer.raw[: read.value]


def scan_region(
    process: int,
    host_base: int,
    size: int,
    needles: list[bytes],
    chunk_size: int,
) -> dict[bytes, list[int]]:
    hits = {needle: [] for needle in needles}
    overlap = max((len(needle) for needle in needles), default=1) - 1
    carried = b""
    offset = 0
    while offset < size:
        requested = min(chunk_size, size - offset)
        chunk = read_exact(process, host_base + offset, requested)
        window = carried + chunk
        window_start = offset - len(carried)
        for needle in needles:
            cursor = 0
            while True:
                found = window.find(needle, cursor)
                if found < 0:
                    break
                guest_offset = window_start + found
                if guest_offset >= 0 and (
                    not hits[needle] or hits[needle][-1] != guest_offset
                ):
                    hits[needle].append(guest_offset)
                cursor = found + 1
        carried = window[-overlap:] if overlap else b""
        offset += len(chunk)
        if len(chunk) != requested:
            break
    return hits


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--pid", type=int, required=True)
    parser.add_argument("--ee-host-base", type=parse_int, required=True)
    parser.add_argument("--ee-size", type=parse_int, default=0x08F00000)
    parser.add_argument("--needle", action="append", default=[])
    parser.add_argument(
        "--pointer-value",
        action="append",
        type=parse_int,
        default=[],
        help="also scan for an exact little-endian guest u32 value",
    )
    parser.add_argument(
        "--hex-needle",
        action="append",
        default=[],
        help="also scan for an exact byte sequence encoded as hexadecimal",
    )
    parser.add_argument(
        "--mips-lbu-v0-a0-accessors",
        type=parse_address_size,
        help="scan a bounded EE range for tiny lbu v0, offset(a0) accessors",
    )
    parser.add_argument("--chunk-size", type=parse_int, default=0x00100000)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()

    if (
        not args.needle
        and not args.pointer_value
        and not args.hex_needle
        and not args.mips_lbu_v0_a0_accessors
    ):
        parser.error(
            "at least one --needle, --pointer-value, or --hex-needle is required"
        )
    encoded = [needle.encode("ascii") + b"\0" for needle in args.needle]
    hex_encoded = []
    for value in args.hex_needle:
        try:
            decoded = bytes.fromhex(value)
        except ValueError as exc:
            parser.error(f"invalid --hex-needle {value!r}: {exc}")
        if not decoded:
            parser.error("--hex-needle cannot be empty")
        hex_encoded.append(decoded)
    access = PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION
    process = kernel32.OpenProcess(access, False, args.pid)
    if not process:
        raise OSError(ctypes.get_last_error(), "OpenProcess failed")
    try:
        string_hits = scan_region(
            process,
            args.ee_host_base,
            args.ee_size,
            encoded,
            args.chunk_size,
        )
        pointer_needles: list[bytes] = []
        pointer_owner: dict[bytes, tuple[str, int]] = {}
        for needle, offsets in string_hits.items():
            for offset in offsets:
                packed = struct.pack("<I", offset)
                pointer_needles.append(packed)
                pointer_owner[packed] = (needle[:-1].decode("ascii"), offset)
        pointer_hits = (
            scan_region(
                process,
                args.ee_host_base,
                args.ee_size,
                pointer_needles,
                args.chunk_size,
            )
            if pointer_needles
            else {}
        )
        explicit_pointers = [struct.pack("<I", value) for value in args.pointer_value]
        explicit_pointer_hits = (
            scan_region(
                process,
                args.ee_host_base,
                args.ee_size,
                explicit_pointers,
                args.chunk_size,
            )
            if explicit_pointers
            else {}
        )
        hex_hits = (
            scan_region(
                process,
                args.ee_host_base,
                args.ee_size,
                hex_encoded,
                args.chunk_size,
            )
            if hex_encoded
            else {}
        )
        accessor_rows = []
        if args.mips_lbu_v0_a0_accessors:
            code_start, code_size = args.mips_lbu_v0_a0_accessors
            code = read_exact(process, args.ee_host_base + code_start, code_size)
            jr_ra = 0x03E00008
            for offset in range(0, len(code) - 7, 4):
                first, second = struct.unpack_from("<II", code, offset)
                for order, lbu_word in (
                    ("lbu_then_jr", first if second == jr_ra else 0),
                    ("jr_then_lbu", second if first == jr_ra else 0),
                ):
                    if lbu_word & 0xFFFF0000 != 0x90820000:
                        continue
                    accessor_rows.append(
                        {
                            "guest_address": f"0x{code_start + offset:08x}",
                            "order": order,
                            "field_offset": f"0x{lbu_word & 0xffff:04x}",
                        }
                    )
    finally:
        kernel32.CloseHandle(process)

    rows = []
    for needle in encoded:
        for offset in string_hits[needle]:
            packed = struct.pack("<I", offset)
            rows.append(
                {
                    "needle": needle[:-1].decode("ascii"),
                    "guest_address": f"0x{offset:08x}",
                    "host_address": f"0x{args.ee_host_base + offset:016x}",
                    "guest_pointer_xrefs": [
                        f"0x{xref:08x}" for xref in pointer_hits.get(packed, [])
                    ],
                }
            )
    manifest = {
        "captured_utc": datetime.now(timezone.utc).isoformat(),
        "pid": args.pid,
        "read_only": True,
        "process_access": [
            "PROCESS_VM_READ",
            "PROCESS_QUERY_LIMITED_INFORMATION",
        ],
        "focus_or_input_used": False,
        "host_or_emulated_memory_write_used": False,
        "ee_host_base": f"0x{args.ee_host_base:016x}",
        "ee_size": f"0x{args.ee_size:x}",
        "chunk_size": f"0x{args.chunk_size:x}",
        "results": rows,
        "pointer_value_results": [
            {
                "pointer_value": f"0x{value:08x}",
                "guest_xrefs": [
                    f"0x{xref:08x}"
                    for xref in explicit_pointer_hits.get(
                        struct.pack("<I", value), []
                    )
                ],
            }
            for value in args.pointer_value
        ],
        "hex_results": [
            {
                "hex_needle": needle.hex(),
                "guest_hits": [
                    f"0x{offset:08x}" for offset in hex_hits.get(needle, [])
                ],
            }
            for needle in hex_encoded
        ],
        "mips_lbu_v0_a0_accessors": accessor_rows,
    }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(
        json.dumps(
            {
                "needles": len(encoded),
                "string_hits": len(rows),
                "pointer_xrefs": sum(
                    len(row["guest_pointer_xrefs"]) for row in rows
                )
                + sum(
                    len(hits) for hits in explicit_pointer_hits.values()
                )
                + sum(len(hits) for hits in hex_hits.values())
                + len(accessor_rows),
                "out": str(args.out),
            }
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
