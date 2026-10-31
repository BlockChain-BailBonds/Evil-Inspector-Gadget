#!/usr/bin/env python3
"""Recover the last complete M3rMa1d S3-CAM JPEG from a serial-monitor text log.

Standard library only. Requires verified size, CRC32, and JPEG framing.
No network access, board access or flashing.
"""
from __future__ import annotations
import argparse
import base64
import binascii
from pathlib import Path
import re
import sys
import zlib

BEGIN = re.compile(r"CAM_BEGIN bytes=(\d+) crc32=([0-9A-Fa-f]{8}) format=jpeg")
CHUNK = re.compile(r"CAM_DATA ([A-Za-z0-9+/=]+)")
END = re.compile(r"CAM_END bytes=(\d+) crc32=([0-9A-Fa-f]{8})")
MAX_IMAGE_SIZE = 500000


def recover(text: str) -> bytes:
    active = None
    results = []
    for line in text.splitlines():
        start = BEGIN.search(line)
        if start:
            declared = int(start.group(1))
            if not 4 <= declared <= MAX_IMAGE_SIZE:
                raise ValueError(f"Invalid advertised image length {declared}")
            active = {"size": declared, "crc": int(start.group(2), 16), "parts": []}
            continue
        match = CHUNK.search(line)
        if active is not None and match:
            encoded = match.group(1)
            if len(encoded) > 1500:
                raise ValueError("Invalid JPEG chunk length")
            try:
                payload = base64.b64decode(encoded, validate=True)
            except binascii.Error as exc:
                raise ValueError("Invalid Base64 frame chunk") from exc
            active["parts"].append(payload)
            if sum(len(x) for x in active["parts"]) > MAX_IMAGE_SIZE:
                raise ValueError("Excess image data")
            continue
        end = END.search(line)
        if active is not None and end:
            data = b"".join(active["parts"])
            claimed_size = int(end.group(1))
            claimed_crc = int(end.group(2), 16)
            digest = zlib.crc32(data) & 0xFFFFFFFF
            if claimed_size != active["size"] or len(data) != active["size"]:
                raise ValueError("Image size mismatch or truncated serial capture")
            if digest != active["crc"] or digest != claimed_crc:
                raise ValueError("Image CRC32 mismatch")
            if not data.startswith(b"\xFF\xD8\xFF") or not data.endswith(b"\xFF\xD9"):
                raise ValueError("Not a complete JPEG file")
            results.append(data)
            active = None
    if not results:
        raise ValueError("No complete and verified CAM_BEGIN/CAM_END JPEG sequence found")
    return results[-1]


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("logfile", type=Path, help="Termux tee log containing a CAM CAPTURE session")
    parser.add_argument("output", type=Path, help="Target JPEG file path")
    args = parser.parse_args(argv)
    if not args.logfile.is_file():
        parser.error(f"Input log not found: {args.logfile}")
    if args.output.exists():
        parser.error(f"Refusing to overwrite existing JPEG: {args.output}")
    try:
        jpg = recover(args.logfile.read_text(errors="replace"))
    except ValueError as exc:
        parser.error(str(exc))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(jpg)
    print(f"Verified JPEG saved: {args.output} ({len(jpg)} bytes; CRC32={zlib.crc32(jpg):08X})")
    return 0

if __name__ == "__main__":
    sys.exit(main())
