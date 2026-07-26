#!/usr/bin/env python3
"""Capture bounded, read-only EE-memory traces from PCSX2's PINE server.

This client intentionally exposes no PINE write opcode and performs no window
or input operations.  It is suitable for unattended boots and for attaching
after a user has navigated PCSX2 to an interesting state.
"""

from __future__ import annotations

import argparse
import json
import socket
import struct
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Iterable


READ_OPCODES = {
    "u8": (0x00, "<B"),
    "u16": (0x01, "<H"),
    "u32": (0x02, "<I"),
    "u64": (0x03, "<Q"),
    "f32": (0x02, "<f"),
}
TITLE_OPCODE = 0x0B
SUCCESS_STATUSES = {0x00, 0x0F}
MAX_READS = 4096
MAX_TITLE_BYTES = 4096
MAX_SCAN_BYTES = 32 * 1024 * 1024
BATCH_READ_COUNT = 8192


@dataclass(frozen=True)
class ReadSpec:
    address: int
    kind: str
    label: str


class PineClient:
    def __init__(self, host: str, port: int, timeout: float) -> None:
        self._socket = socket.create_connection((host, port), timeout=timeout)
        self._socket.settimeout(timeout)

    def __enter__(self) -> "PineClient":
        return self

    def __exit__(self, *_: object) -> None:
        self._socket.close()

    def _recv_exact(self, size: int) -> bytes:
        chunks: list[bytes] = []
        remaining = size
        while remaining:
            chunk = self._socket.recv(remaining)
            if not chunk:
                raise ConnectionError("PINE connection closed during response")
            chunks.append(chunk)
            remaining -= len(chunk)
        return b"".join(chunks)

    def _request(self, command: bytes) -> tuple[int, bytes]:
        request = struct.pack("<I", 4 + len(command)) + command
        self._socket.sendall(request)
        response_length = struct.unpack("<I", self._recv_exact(4))[0]
        if response_length < 5 or response_length > 1_048_576:
            raise RuntimeError(f"invalid PINE response length {response_length}")
        response = self._recv_exact(response_length - 4)
        status = response[0]
        if status not in SUCCESS_STATUSES:
            raise RuntimeError(f"PINE request failed with status 0x{status:02x}")
        return status, response[1:]

    def title(self) -> tuple[int, str]:
        status, payload = self._request(bytes([TITLE_OPCODE]))
        if len(payload) > MAX_TITLE_BYTES:
            raise RuntimeError("PINE title response exceeds safety bound")
        if len(payload) >= 4:
            declared_size = struct.unpack("<I", payload[:4])[0]
            if 0 < declared_size <= len(payload) - 4:
                payload = payload[4 : 4 + declared_size]
        return status, payload.rstrip(b"\0").decode("utf-8", errors="replace")

    def read(self, spec: ReadSpec) -> tuple[int, int | float]:
        opcode, value_format = READ_OPCODES[spec.kind]
        status, payload = self._request(
            bytes([opcode]) + struct.pack("<I", spec.address)
        )
        value_size = struct.calcsize(value_format)
        if len(payload) != value_size:
            raise RuntimeError(
                f"short {spec.kind} response at 0x{spec.address:08x}: "
                f"{len(payload)} bytes"
            )
        return status, struct.unpack(value_format, payload)[0]

    def read_many_u32(self, addresses: list[int]) -> tuple[int, list[int]]:
        if not addresses or len(addresses) > BATCH_READ_COUNT:
            raise ValueError("u32 batch size is outside the safety bound")
        command = b"".join(
            bytes([READ_OPCODES["u32"][0]]) + struct.pack("<I", address)
            for address in addresses
        )
        status, payload = self._request(command)
        expected_size = 4 * len(addresses)
        if len(payload) != expected_size:
            raise RuntimeError(
                f"short batched u32 response: {len(payload)} of {expected_size} bytes"
            )
        return status, list(struct.unpack(f"<{len(addresses)}I", payload))

    def read_many(self, specs: list[ReadSpec]) -> tuple[int, list[int | float]]:
        if not specs or len(specs) > BATCH_READ_COUNT:
            raise ValueError("read batch size is outside the safety bound")
        command = b"".join(
            bytes([READ_OPCODES[spec.kind][0]]) + struct.pack("<I", spec.address)
            for spec in specs
        )
        status, payload = self._request(command)
        values: list[int | float] = []
        offset = 0
        for spec in specs:
            value_format = READ_OPCODES[spec.kind][1]
            value_size = struct.calcsize(value_format)
            if offset + value_size > len(payload):
                raise RuntimeError(
                    f"short batched response at 0x{spec.address:08x}"
                )
            values.append(struct.unpack_from(value_format, payload, offset)[0])
            offset += value_size
        if offset != len(payload):
            raise RuntimeError(f"batched response has {len(payload) - offset} extra bytes")
        return status, values


def parse_int(text: str) -> int:
    return int(text, 0)


def parse_read_spec(text: str) -> ReadSpec:
    parts = text.split(":", 2)
    address = parse_int(parts[0])
    kind = parts[1].lower() if len(parts) >= 2 and parts[1] else "u32"
    if kind not in READ_OPCODES:
        raise argparse.ArgumentTypeError(
            f"unsupported read type {kind!r}; choose {', '.join(READ_OPCODES)}"
        )
    if not 0 <= address <= 0xFFFFFFFF:
        raise argparse.ArgumentTypeError("EE address must fit in 32 bits")
    label = parts[2] if len(parts) == 3 and parts[2] else f"0x{address:08x}"
    return ReadSpec(address=address, kind=kind, label=label)


def read_specs_from_file(path: Path) -> list[ReadSpec]:
    specs: list[ReadSpec] = []
    for line_number, raw_line in enumerate(
        path.read_text(encoding="utf-8").splitlines(), start=1
    ):
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        try:
            specs.append(parse_read_spec(line))
        except (ValueError, argparse.ArgumentTypeError) as exc:
            raise argparse.ArgumentTypeError(
                f"{path}:{line_number}: {exc}"
            ) from exc
    return specs


def utc_timestamp() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="milliseconds")


def sample_once(client: PineClient, specs: Iterable[ReadSpec]) -> dict[str, Any]:
    spec_list = list(specs)
    values: list[dict[str, Any]] = []
    status, read_values = client.read_many(spec_list)
    for spec, value in zip(spec_list, read_values):
        values.append(
            {
                "label": spec.label,
                "address": f"0x{spec.address:08x}",
                "type": spec.kind,
                "value": value,
                "hex": (
                    f"0x{value:0{struct.calcsize(READ_OPCODES[spec.kind][1]) * 2}x}"
                    if isinstance(value, int)
                    else None
                ),
            }
        )
    return {
        "timestamp_utc": utc_timestamp(),
        "pine_statuses": [f"0x{status:02x}"],
        "values": values,
    }


def write_record(stream: Any, record: dict[str, Any]) -> None:
    stream.write(json.dumps(record, sort_keys=True) + "\n")
    stream.flush()


def add_connection_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=28011)
    parser.add_argument("--timeout", type=float, default=5.0)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    add_connection_arguments(parser)
    subparsers = parser.add_subparsers(dest="command", required=True)

    title_parser = subparsers.add_parser("title", help="read the emulated title")
    title_parser.add_argument("--output", type=Path)

    sample_parser = subparsers.add_parser(
        "sample", help="sample one or more EE addresses"
    )
    sample_parser.add_argument(
        "--read",
        action="append",
        type=parse_read_spec,
        metavar="ADDRESS[:TYPE[:LABEL]]",
    )
    sample_parser.add_argument(
        "--read-file",
        action="append",
        type=Path,
        metavar="PATH",
        help="read newline-delimited ADDRESS[:TYPE[:LABEL]] specs",
    )
    sample_parser.add_argument("--count", type=int, default=1)
    sample_parser.add_argument("--interval-ms", type=int, default=100)
    sample_parser.add_argument("--output", type=Path)

    find_parser = subparsers.add_parser(
        "find32", help="find an exact u32 value in a bounded EE address range"
    )
    find_parser.add_argument("--start", type=parse_int, required=True)
    find_parser.add_argument("--end", type=parse_int, required=True)
    find_parser.add_argument("--value", type=parse_int, required=True)
    find_parser.add_argument("--max-results", type=int, default=4096)
    find_parser.add_argument("--output", type=Path)

    find_string_parser = subparsers.add_parser(
        "findstr", help="find an exact ASCII string in a bounded EE address range"
    )
    find_string_parser.add_argument("--start", type=parse_int, required=True)
    find_string_parser.add_argument("--end", type=parse_int, required=True)
    find_string_parser.add_argument("--text", required=True)
    find_string_parser.add_argument("--max-results", type=int, default=4096)
    find_string_parser.add_argument("--output", type=Path)

    args = parser.parse_args()
    with PineClient(args.host, args.port, args.timeout) as client:
        if args.command == "title":
            status, title = client.title()
            record = {
                "timestamp_utc": utc_timestamp(),
                "host": args.host,
                "port": args.port,
                "operation": "title",
                "pine_status": f"0x{status:02x}",
                "title": title,
            }
            if args.output:
                args.output.parent.mkdir(parents=True, exist_ok=True)
                with args.output.open("a", encoding="utf-8") as stream:
                    write_record(stream, record)
            print(json.dumps(record, sort_keys=True))
            return 0

        if args.command == "find32":
            if (
                args.start < 0
                or args.end <= args.start
                or args.end > 0x1_0000_0000
                or args.start % 4
                or args.end % 4
            ):
                parser.error("scan range must be increasing, 32-bit, and u32-aligned")
            if args.end - args.start > MAX_SCAN_BYTES:
                parser.error(f"scan range exceeds {MAX_SCAN_BYTES} bytes")
            if not 0 <= args.value <= 0xFFFFFFFF:
                parser.error("--value must fit in a u32")
            if not 1 <= args.max_results <= MAX_READS:
                parser.error(f"--max-results must be between 1 and {MAX_READS}")

            matches: list[str] = []
            statuses: set[int] = set()
            for batch_start in range(args.start, args.end, BATCH_READ_COUNT * 4):
                addresses = list(
                    range(
                        batch_start,
                        min(args.end, batch_start + BATCH_READ_COUNT * 4),
                        4,
                    )
                )
                status, values = client.read_many_u32(addresses)
                statuses.add(status)
                matches.extend(
                    f"0x{address:08x}"
                    for address, value in zip(addresses, values)
                    if value == args.value
                )
                if len(matches) > args.max_results:
                    raise RuntimeError("scan result exceeds --max-results safety bound")
            record = {
                "timestamp_utc": utc_timestamp(),
                "host": args.host,
                "port": args.port,
                "operation": "find32",
                "read_only": True,
                "start": f"0x{args.start:08x}",
                "end": f"0x{args.end:08x}",
                "value": f"0x{args.value:08x}",
                "pine_statuses": [
                    f"0x{status:02x}" for status in sorted(statuses)
                ],
                "matches": matches,
            }
            if args.output:
                args.output.parent.mkdir(parents=True, exist_ok=True)
                with args.output.open("a", encoding="utf-8") as stream:
                    write_record(stream, record)
            print(json.dumps(record, sort_keys=True))
            return 0

        if args.command == "findstr":
            try:
                needle = args.text.encode("ascii")
            except UnicodeEncodeError as exc:
                parser.error(f"--text must be ASCII: {exc}")
            if not 4 <= len(needle) <= 256:
                parser.error("--text must contain between 4 and 256 ASCII bytes")
            if (
                args.start < 0
                or args.end <= args.start
                or args.end > 0x1_0000_0000
                or args.start % 4
                or args.end % 4
            ):
                parser.error("scan range must be increasing, 32-bit, and u32-aligned")
            if args.end - args.start > MAX_SCAN_BYTES:
                parser.error(f"scan range exceeds {MAX_SCAN_BYTES} bytes")
            if not 1 <= args.max_results <= MAX_READS:
                parser.error(f"--max-results must be between 1 and {MAX_READS}")

            first_word = struct.unpack("<I", needle[:4])[0]
            candidates: list[int] = []
            statuses: set[int] = set()
            for batch_start in range(args.start, args.end, BATCH_READ_COUNT * 4):
                addresses = list(
                    range(
                        batch_start,
                        min(args.end, batch_start + BATCH_READ_COUNT * 4),
                        4,
                    )
                )
                status, values = client.read_many_u32(addresses)
                statuses.add(status)
                candidates.extend(
                    address
                    for address, value in zip(addresses, values)
                    if value == first_word
                )

            matches: list[str] = []
            candidates_per_batch = max(1, BATCH_READ_COUNT // len(needle))
            for offset in range(0, len(candidates), candidates_per_batch):
                batch_candidates = candidates[offset : offset + candidates_per_batch]
                specs = [
                    ReadSpec(address=address + byte_index, kind="u8", label="")
                    for address in batch_candidates
                    for byte_index in range(len(needle))
                ]
                status, values = client.read_many(specs)
                statuses.add(status)
                for index, address in enumerate(batch_candidates):
                    start = index * len(needle)
                    value = bytes(values[start : start + len(needle)])
                    if value == needle:
                        matches.append(f"0x{address:08x}")
                        if len(matches) > args.max_results:
                            raise RuntimeError(
                                "scan result exceeds --max-results safety bound"
                            )

            record = {
                "timestamp_utc": utc_timestamp(),
                "host": args.host,
                "port": args.port,
                "operation": "findstr",
                "read_only": True,
                "start": f"0x{args.start:08x}",
                "end": f"0x{args.end:08x}",
                "text": args.text,
                "pine_statuses": [
                    f"0x{status:02x}" for status in sorted(statuses)
                ],
                "matches": matches,
            }
            if args.output:
                args.output.parent.mkdir(parents=True, exist_ok=True)
                with args.output.open("a", encoding="utf-8") as stream:
                    write_record(stream, record)
            print(json.dumps(record, sort_keys=True))
            return 0

        read_specs = list(args.read or [])
        for read_file in args.read_file or []:
            try:
                read_specs.extend(read_specs_from_file(read_file))
            except (OSError, argparse.ArgumentTypeError) as exc:
                parser.error(str(exc))
        if not read_specs:
            parser.error("sample requires at least one --read or --read-file entry")
        if len(read_specs) > BATCH_READ_COUNT:
            parser.error(
                f"sample accepts at most {BATCH_READ_COUNT} read specs"
            )
        if not 1 <= args.count <= MAX_READS:
            parser.error(f"--count must be between 1 and {MAX_READS}")
        if not 0 <= args.interval_ms <= 60_000:
            parser.error("--interval-ms must be between 0 and 60000")

        metadata = {
            "timestamp_utc": utc_timestamp(),
            "host": args.host,
            "port": args.port,
            "operation": "sample_start",
            "read_only": True,
            "read_count": len(read_specs),
            "sample_count": args.count,
            "interval_ms": args.interval_ms,
        }
        output_stream = None
        try:
            if args.output:
                args.output.parent.mkdir(parents=True, exist_ok=True)
                output_stream = args.output.open("a", encoding="utf-8")
                write_record(output_stream, metadata)
            else:
                print(json.dumps(metadata, sort_keys=True))

            for index in range(args.count):
                record = sample_once(client, read_specs)
                record["sample_index"] = index
                if output_stream:
                    write_record(output_stream, record)
                else:
                    print(json.dumps(record, sort_keys=True))
                if index + 1 < args.count and args.interval_ms:
                    time.sleep(args.interval_ms / 1000.0)
        finally:
            if output_stream:
                output_stream.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
