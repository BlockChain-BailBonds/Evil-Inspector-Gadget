#!/usr/bin/env python3
"""Host-side flash partition checks; never talks to hardware."""
from pathlib import Path
import csv
import sys

ROOT = Path(__file__).resolve().parent.parent
FLASH = 16 * 1024 * 1024
ROW_ALIGNMENT = {"app": 0x10000, "data": 0x1000}

def check_layout(path=ROOT / "partitions.csv"):
    rows = []
    for raw in csv.reader(path.read_text().splitlines()):
        if not raw or raw[0].strip().startswith("#"):
            continue
        if len(raw) < 5:
            raise ValueError(f"Invalid partition row: {raw}")
        name, kind, subtype, offset, size = (value.strip() for value in raw[:5])
        a, b = int(offset, 0), int(size, 0)
        if a % ROW_ALIGNMENT[kind] or b % 0x1000:
            raise ValueError(f"Partition {name} misaligned")
        if b == 0 or a < 0x9000 or a + b > FLASH:
            raise ValueError(f"Partition {name} out of range")
        rows.append((a, a + b, name))
    ordered = sorted(rows)
    for left, right in zip(ordered, ordered[1:]):
        if left[1] > right[0]:
            raise ValueError(f"Overlapping partitions {left[2]} and {right[2]}")
    names = {n for _, _, n in ordered}
    if not {"nvs", "factory", "spiffs"}.issubset(names):
        raise ValueError("Missing required partitions")
    return ordered

if __name__ == "__main__":
    try:
        for begin, end, name in check_layout():
            print(f"{name:8} 0x{begin:06X}-0x{end - 1:06X} ({end - begin:,} bytes)")
        print("PASS: 16MB partition layout valid")
    except Exception as exc:
        print("FAIL:", exc, file=sys.stderr)
        sys.exit(1)
