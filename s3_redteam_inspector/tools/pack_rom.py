#!/usr/bin/env python3
"""Package a verified 16 MB ESP32-S3 full-flash image. Does not flash hardware.

S3 default image map: bootloader 0x000000, partitions 0x008000,
factory application 0x010000. Not compatible with ESP32/ESP32E firmware.
"""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent.parent
SIZE = 16 * 1024 * 1024
PARTS = (
    ("bootloader.bin", 0x000000, 0x8000),
    ("partitions.bin", 0x008000, 0x1000),
    ("firmware.bin", 0x010000, 0x400000),
)
PROFILES = {"s3_usb_eye", "s3_uart_eye", "s3_usb_safe", "s3_uart_safe"}


def package(binary_dir: Path, output: Path):
    payloads = []
    for name, offset, maximum in PARTS:
        source = binary_dir / name
        if not source.is_file():
            raise FileNotFoundError(f"Missing {source}; run tools/build.sh first")
        data = source.read_bytes()
        if not data or len(data) > maximum:
            raise ValueError(f"Invalid {name} size: {len(data)} (maximum {maximum})")
        payloads.append((name, offset, data))
    if output.exists():
        raise FileExistsError(f"Refusing to overwrite existing image: {output}")
    output.parent.mkdir(parents=True, exist_ok=True)
    image = bytearray(b"\xFF" * SIZE)
    for name, offset, data in payloads:
        image[offset:offset + len(data)] = data
    output.write_bytes(image)
    for name, offset, data in payloads:
        assert output.read_bytes()[offset:offset + len(data)] == data, name
        print(f"{name}: offset 0x{offset:06X}, {len(data):,} bytes")
    print(f"Image: {output} ({SIZE:,} bytes)")
    print(f"SHA256: {hashlib.sha256(image).hexdigest()}")
    print("Packaging only; no device was flashed")
    return bytes(image)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("environment", choices=sorted(PROFILES))
    parser.add_argument("--output", type=Path, help="Custom output filename; must not already exist")
    args = parser.parse_args(argv)
    binary_dir = ROOT / ".pio" / "build" / args.environment
    output = args.output or ROOT / "dist" / f"M3rMa1d-S3-CAM-{args.environment}-full-flash.bin"
    try:
        package(binary_dir, output)
    except (OSError, ValueError, AssertionError) as exc:
        parser.error(str(exc))
    return 0

if __name__ == "__main__":
    sys.exit(main())
