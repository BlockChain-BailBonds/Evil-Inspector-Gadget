#!/bin/sh
set -eu
command -v nrflash >/dev/null 2>&1 || { echo 'nrflash is required' >&2; exit 127; }
python3 - <<'INNER_PY_918'
from pathlib import Path
import hashlib
p=Path.home()/'s3-erased-4k-reference.bin'
expected=b'\xff'*4096
if not p.is_file() or p.read_bytes()!=expected:
    p.write_bytes(expected)
print('Created/verified local all-FF reference; no device write')
INNER_PY_918
printf '%s\n' 'Checking ESP32-S3 0x0-0xfff against erased flash (read-only)...'
nrflash verify --chip esp32s3 --offset 0x0 "$HOME/s3-erased-4k-reference.bin"
