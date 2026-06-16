#!/usr/bin/env python3
"""Instrument selected GH2 PS2 EE functions and keep recent argument records."""

from __future__ import annotations

import argparse
import ctypes
from ctypes import wintypes
import json
import struct
import subprocess
import time
from pathlib import Path

from extract_ps2_symbol_xrefs import parse_elf_load, word_at
from probe_pcsx2_ee_memory import (
    capture_window,
    find_ee_base,
    parse_logged_ee_base,
    read_process,
)
from trace_pcsx2_animation_calls import (
    open_process,
    patch_recompiler_setting,
    write_process_unprotect,
)
from trace_pcsx2_animation_vtables import post_key, process_window, u32, write_process


kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
kernel32.CloseHandle.restype = wintypes.BOOL

REG = {
    "zero": 0,
    "a0": 4,
    "a1": 5,
    "a2": 6,
    "a3": 7,
    "s0": 16,
    "s1": 17,
    "sp": 29,
    "ra": 31,
    "k0": 26,
    "k1": 27,
}


def ins_j(addr: int) -> int:
    return 0x08000000 | ((addr >> 2) & 0x03FFFFFF)


def ins_lui(rt: int, imm: int) -> int:
    return 0x3C000000 | (rt << 16) | (imm & 0xFFFF)


def ins_lw(rt: int, base: int, off: int) -> int:
    return 0x8C000000 | (base << 21) | (rt << 16) | (off & 0xFFFF)


def ins_sw(rt: int, base: int, off: int) -> int:
    return 0xAC000000 | (base << 21) | (rt << 16) | (off & 0xFFFF)


def ins_addiu(rt: int, rs: int, imm: int) -> int:
    return 0x24000000 | (rs << 21) | (rt << 16) | (imm & 0xFFFF)


def ins_andi(rt: int, rs: int, imm: int) -> int:
    return 0x30000000 | (rs << 21) | (rt << 16) | (imm & 0xFFFF)


def ins_sll(rd: int, rt: int, shamt: int) -> int:
    return (rt << 16) | (rd << 11) | (shamt << 6)


def ins_addu(rd: int, rs: int, rt: int) -> int:
    return (rs << 21) | (rt << 16) | (rd << 11) | 0x21


def parse_target(spec: str) -> tuple[str, int]:
    name, addr_s = spec.split("=", 1)
    return name, int(addr_s, 0)


def parse_sample_target(spec: str) -> tuple[str, int, int]:
    name, rest = spec.split("=", 1)
    addr_s, size_s = rest.split(":", 1)
    return name, int(addr_s, 0), int(size_s, 0)


def record_words_for_mode(extended_regs: bool, hair_point_state: bool) -> int:
    if hair_point_state:
        return 64
    return 8 if extended_regs else 4


def record_shift_for_words(record_words: int) -> int:
    record_bytes = record_words * 4
    if record_bytes & (record_bytes - 1):
        raise RuntimeError("record byte count must be a power of two")
    return record_bytes.bit_length() - 1


def hair_point_state_labels() -> list[str]:
    labels = ["a0", "a1", "a2", "a3", "s0", "s1", "sp", "ra"]
    labels += [f"s0_{off:02x}" for off in range(0x00, 0x64, 4)]
    labels += [f"sp_{off:02x}" for off in range(0x20, 0x90, 4)]
    while len(labels) < 64:
        labels.append(f"pad_{len(labels):02d}")
    return labels


def make_ring_stub(
    func: int, data: int, orig1: int, orig2: int, ring_size: int,
    extended_regs: bool, hair_point_state: bool,
) -> bytes:
    base = data + 0x100
    record_words = record_words_for_mode(extended_regs, hair_point_state)
    record_bytes = record_words * 4
    words = [
        orig1,
        orig2,
        ins_lui(REG["k0"], (data >> 16) & 0xFFFF),
        ins_lw(REG["k1"], REG["k0"], data & 0xFFFF),
        ins_addiu(REG["k1"], REG["k1"], 1),
        ins_sw(REG["k1"], REG["k0"], data & 0xFFFF),
        ins_addiu(REG["k1"], REG["k1"], -1),
        ins_andi(REG["k1"], REG["k1"], ring_size - 1),
        ins_sll(REG["k1"], REG["k1"], record_shift_for_words(record_words)),
        ins_lui(REG["k0"], (base >> 16) & 0xFFFF),
        ins_addiu(REG["k0"], REG["k0"], base & 0xFFFF),
        ins_addu(REG["k1"], REG["k1"], REG["k0"]),
        ins_sw(REG["a0"], REG["k1"], 0),
        ins_sw(REG["a1"], REG["k1"], 4),
        ins_sw(REG["a2"], REG["k1"], 8),
        ins_sw(REG["a3"], REG["k1"], 12),
    ]
    if extended_regs:
        words += [
            ins_sw(REG["s0"], REG["k1"], 16),
            ins_sw(REG["s1"], REG["k1"], 20),
            ins_sw(REG["sp"], REG["k1"], 24),
            ins_sw(REG["ra"], REG["k1"], 28),
        ]
    if hair_point_state:
        out_off = 32
        for in_off in range(0x00, 0x64, 4):
            words += [ins_lw(REG["k0"], REG["s0"], in_off),
                      ins_sw(REG["k0"], REG["k1"], out_off)]
            out_off += 4
        for in_off in range(0x20, 0x90, 4):
            words += [ins_lw(REG["k0"], REG["sp"], in_off),
                      ins_sw(REG["k0"], REG["k1"], out_off)]
            out_off += 4
    words += [
        ins_j(func + 8),
        0,
    ]
    return b"".join(u32(word) for word in words)


def read_ring(
    handle: int, ee_base: int, data: int, ring_size: int, extended_regs: bool,
    hair_point_state: bool,
) -> dict[str, object]:
    record_words = record_words_for_mode(extended_regs, hair_point_state)
    record_bytes = record_words * 4
    raw_count = read_process(handle, ee_base + data, 4)
    count = struct.unpack("<I", raw_count)[0] if len(raw_count) == 4 else 0
    raw = read_process(handle, ee_base + data + 0x100, ring_size * record_bytes)
    values = list(struct.unpack("<" + "I" * (len(raw) // 4), raw[: len(raw) & ~3])) if raw else []
    labels = hair_point_state_labels() if hair_point_state else []
    records = []
    for i in range(0, len(values), record_words):
        if values[i : i + record_words] == [0] * record_words:
            continue
        rec = {
            "index": i // record_words,
            "a0": f"0x{values[i]:08x}",
            "a1": f"0x{values[i + 1]:08x}",
            "a2": f"0x{values[i + 2]:08x}",
            "a3": f"0x{values[i + 3]:08x}",
        }
        if extended_regs:
            rec.update(
                {
                    "s0": f"0x{values[i + 4]:08x}",
                    "s1": f"0x{values[i + 5]:08x}",
                    "sp": f"0x{values[i + 6]:08x}",
                    "ra": f"0x{values[i + 7]:08x}",
                }
            )
        if hair_point_state:
            rec["words"] = {
                labels[j]: f"0x{values[i + j]:08x}"
                for j in range(min(record_words, len(labels)))
            }
        records.append(rec)
    return {"count": count, "records": records}


def classify_word(value: int) -> str:
    if value == 0:
        return "zero"
    if 0x00100000 <= value <= 0x02000000:
        return "ee_ram"
    if 0x00300000 <= value <= 0x00450000:
        return "code_rodata"
    bits = value & 0x7FFFFFFF
    if 0x3D000000 <= bits <= 0x44800000:
        return "floatish"
    return "other"


def read_word_rows(handle: int, ee_base: int, addr: int, size: int) -> list[dict[str, str]]:
    raw = read_process(handle, ee_base + addr, size)
    words = list(struct.unpack("<" + "I" * (len(raw) // 4), raw[: len(raw) & ~3])) if raw else []
    return [
        {
            "addr": f"0x{addr + i * 4:08x}",
            "value": f"0x{word:08x}",
            "kind": classify_word(word),
        }
        for i, word in enumerate(words)
    ]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--pcsx2", default=r"C:\Games\Emulators\PCSX2\pcsx2-qt.exe", type=Path)
    parser.add_argument("--ini", default=r"C:\Games\Emulators\PCSX2\inis\PCSX2.ini", type=Path)
    parser.add_argument("--iso", required=True, type=Path)
    parser.add_argument("--elf", required=True, type=Path)
    parser.add_argument("--state", type=int, default=1)
    parser.add_argument("--log", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--target", action="append", required=True)
    parser.add_argument("--ring-size", type=int, default=64)
    parser.add_argument("--seconds", type=float, default=8.0)
    parser.add_argument("--pre-retry-seconds", type=float, default=4.0)
    parser.add_argument("--post-retry-seconds", type=float, default=3.0)
    parser.add_argument("--retry-key", type=lambda x: int(x, 0), default=0x4C)
    parser.add_argument("--retry-pulses", type=int, default=2)
    parser.add_argument("--stub-base", type=lambda x: int(x, 0), default=0x01F00000)
    parser.add_argument("--data-base", type=lambda x: int(x, 0), default=0x01F10000)
    parser.add_argument("--disable-ee-recompiler", action="store_true")
    parser.add_argument("--gui", action="store_true")
    parser.add_argument("--require-screenshot", action="store_true")
    parser.add_argument(
        "--extended-regs",
        action="store_true",
        help="also record s0/s1/sp/ra for each ring entry",
    )
    parser.add_argument(
        "--hair-point-state",
        action="store_true",
        help="record s0 point fields and stack work rows at a hair write site",
    )
    parser.add_argument(
        "--sample-target",
        action="append",
        default=[],
        help="also dump fixed EE memory range name=addr:size after tracing",
    )
    args = parser.parse_args()

    if args.ring_size & (args.ring_size - 1):
        raise RuntimeError("--ring-size must be a power of two")

    args.out.parent.mkdir(parents=True, exist_ok=True)
    targets = [parse_target(spec) for spec in args.target]
    sample_targets = [parse_sample_target(spec) for spec in args.sample_target]
    record_bytes = record_words_for_mode(args.extended_regs, args.hair_point_state) * 4
    data_stride = ((0x100 + args.ring_size * record_bytes + 0xFFF) // 0x1000) * 0x1000
    elf_base, _elf_off, elf_code = parse_elf_load(args.elf)
    original_ini = None
    if args.disable_ee_recompiler:
        original_ini = patch_recompiler_setting(args.ini, enable_ee=False)

    proc = subprocess.Popen(
        [str(args.pcsx2)]
        + ([] if args.gui else ["-nogui"])
        + ["-logfile", str(args.log), "-state", str(args.state), "--", str(args.iso)],
        cwd=str(args.pcsx2.parent),
    )
    handle = None
    try:
        handle = open_process(proc.pid)
        ee_base = parse_logged_ee_base(args.log, timeout_seconds=20.0)
        if ee_base is None:
            ee_base, _elf_host = find_ee_base(handle, elf_base, elf_code)
        if ee_base is None:
            raise RuntimeError("Could not parse EE Main Memory base")
        time.sleep(args.pre_retry_seconds)
        hwnd = process_window(proc.pid)
        for _ in range(max(1, args.retry_pulses)):
            post_key(hwnd, args.retry_key, hold_seconds=0.08)
            time.sleep(0.15)
        time.sleep(args.post_retry_seconds)

        patches = []
        for i, (name, func) in enumerate(targets):
            orig1 = word_at(elf_code, elf_base, func)
            orig2 = word_at(elf_code, elf_base, func + 4)
            if orig1 is None or orig2 is None:
                raise RuntimeError(f"Could not read original words for 0x{func:08x}")
            stub = args.stub_base + i * 0x100
            data = args.data_base + i * data_stride
            write_process(
                handle,
                ee_base + data,
                b"\x00" * (0x100 + args.ring_size * record_bytes),
            )
            write_process(
                handle,
                ee_base + stub,
                make_ring_stub(
                    func, data, orig1, orig2, args.ring_size,
                    args.extended_regs, args.hair_point_state,
                ),
            )
            write_process_unprotect(handle, ee_base + func, b"".join([u32(ins_j(stub)), u32(0)]))
            patches.append(
                {
                    "name": name,
                    "func": f"0x{func:08x}",
                    "stub": f"0x{stub:08x}",
                    "data": f"0x{data:08x}",
                    "orig": [f"0x{orig1:08x}", f"0x{orig2:08x}"],
                }
            )

        time.sleep(args.seconds)
        screenshot = capture_window(hwnd, args.out.with_suffix(".window.png"))
        if args.require_screenshot and screenshot is None:
            raise RuntimeError("Required PCSX2 PrintWindow screenshot capture failed")
        samples = []
        for patch in patches:
            data = int(patch["data"], 16)
            samples.append(
                {
                    "name": patch["name"],
                    "func": patch["func"],
                    **read_ring(
                        handle, ee_base, data, args.ring_size,
                        args.extended_regs, args.hair_point_state,
                    ),
                }
            )
        object_samples = [
            {
                "name": name,
                "addr": f"0x{addr:08x}",
                "size": size,
                "rows": read_word_rows(handle, ee_base, addr, size),
            }
            for name, addr, size in sample_targets
        ]
        report = {
            "pcsx2_pid": proc.pid,
            "ee_base_host": f"0x{ee_base:016x}",
            "interpreter_for_probe": bool(args.disable_ee_recompiler),
            "window_handle": f"0x{hwnd:x}" if hwnd else None,
            "screenshot": str(screenshot) if screenshot else None,
            "seconds": args.seconds,
            "ring_size": args.ring_size,
            "extended_regs": args.extended_regs,
            "hair_point_state": args.hair_point_state,
            "data_stride": f"0x{data_stride:x}",
            "patches": patches,
            "samples": samples,
            "object_samples": object_samples,
        }
        args.out.write_text(json.dumps(report, indent=2), encoding="utf-8")
        print(json.dumps({sample["name"]: sample["count"] for sample in samples}, indent=2))
    finally:
        if handle:
            kernel32.CloseHandle(handle)
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()
        if original_ini is not None:
            args.ini.write_bytes(original_ini)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
