#!/usr/bin/env python3
"""Read small, explicitly named PCSX2 EE-memory ranges without side effects."""

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
MAX_RANGE_SIZE = 0x1000
MAX_TOTAL_SIZE = 0x10000

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


def parse_range(text: str) -> tuple[str, int, int]:
    parts = text.split(":")
    if len(parts) != 3 or not parts[0]:
        raise argparse.ArgumentTypeError("range must be NAME:ADDRESS:SIZE")
    address = parse_int(parts[1])
    size = parse_int(parts[2])
    if size <= 0 or size > MAX_RANGE_SIZE:
        raise argparse.ArgumentTypeError(
            f"range size must be 1..0x{MAX_RANGE_SIZE:x}"
        )
    return parts[0], address, size


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
        raise OSError(
            ctypes.get_last_error(),
            f"ReadProcessMemory failed at 0x{address:x}",
        )
    if read.value != size:
        raise OSError(f"short read at 0x{address:x}: {read.value}/{size}")
    return buffer.raw


def decode_words(data: bytes, guest_address: int) -> list[dict[str, object]]:
    rows = []
    for offset in range(0, len(data) - 3, 4):
        word = data[offset : offset + 4]
        rows.append(
            {
                "guest_address": f"0x{guest_address + offset:08x}",
                "hex": word.hex(),
                "u32": f"0x{struct.unpack('<I', word)[0]:08x}",
                "f32": struct.unpack("<f", word)[0],
            }
        )
    return rows


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--pid", type=int, required=True)
    parser.add_argument("--ee-host-base", type=parse_int, required=True)
    parser.add_argument("--range", action="append", type=parse_range, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    if sum(size for _, _, size in args.range) > MAX_TOTAL_SIZE:
        parser.error(f"total requested bytes exceed 0x{MAX_TOTAL_SIZE:x}")

    access = PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION
    process = kernel32.OpenProcess(access, False, args.pid)
    if not process:
        raise OSError(ctypes.get_last_error(), "OpenProcess failed")
    rows = []
    try:
        for name, guest_address, size in args.range:
            data = read_exact(process, args.ee_host_base + guest_address, size)
            rows.append(
                {
                    "name": name,
                    "guest_address": f"0x{guest_address:08x}",
                    "host_address": (
                        f"0x{args.ee_host_base + guest_address:016x}"
                    ),
                    "size": size,
                    "bytes_hex": data.hex(),
                    "words": decode_words(data, guest_address),
                }
            )
    finally:
        kernel32.CloseHandle(process)

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
        "ranges": rows,
    }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(
        json.dumps(
            {
                "ranges": len(rows),
                "bytes": sum(row["size"] for row in rows),
                "out": str(args.out),
            }
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
