#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
if [ "$#" -ne 1 ]; then
  echo "Usage: sh tools/rollback_to_v107_after_backup.sh /absolute/path/verified-16m.bin" >&2
  exit 2
fi
BACKUP="$1"
APP="$HOME/m3rmaid-known-good/s3_touch_v107.bin"
python3 - "$BACKUP" "$APP" <<'PYINNER'
import hashlib, json, sys
from pathlib import Path
backup=Path(sys.argv[1]).expanduser().resolve()
app=Path(sys.argv[2]).expanduser().resolve()
expected='71d8864559ac6b710b9e34c8c616e2a76bf45e0b14b5ab307c7235c9c1134bf5'
if not backup.is_file():raise SystemExit('STOP: verified physical flash backup missing')
manifestfile=backup.with_suffix('.json')
if not manifestfile.is_file():raise SystemExit('STOP: backup manifest missing')
manifest=json.loads(manifestfile.read_text())
if manifest.get('chip')!='esp32s3' or manifest.get('verified_backup') is not True:
    raise SystemExit('STOP: backup is not verified for ESP32-S3')
if backup.stat().st_size!=16777216 or manifest.get('flash_size_bytes')!=16777216:
    raise SystemExit('STOP: backup size incorrect')
if manifest.get('flash_encryption_enabled') is not False or manifest.get('secure_boot_enabled') is not False:
    raise SystemExit('STOP: security flags unexpected')
def sha256(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for part in iter(lambda:f.read(1024*1024),b''):h.update(part)
    return h.hexdigest()
if sha256(backup)!=manifest.get('sha256'):raise SystemExit('STOP: flash backup checksum mismatch')
if not app.is_file() or sha256(app)!=expected:raise SystemExit('STOP: v1.0.7 known-good application checksum mismatch')
if app.stat().st_size+0x10000>0x410000:raise SystemExit('STOP: known-good app exceeds partition')
if app.open('rb').read(1)!=b'\xe9':raise SystemExit('STOP: not an Espressif application')
print('PASS: complete 16 MiB flash backup, original backup manifest and checksum')
print('PASS: known-good physical LCD/touch v1.0.7 app SHA256',expected)
print('PASS: app-only rollback at 0x10000; bootloader and partition table untouched')
PYINNER
if ! command -v nrflash >/dev/null 2>&1; then
  echo 'STOP: nrflash missing' >&2
  exit 127
fi
PROBE="${TMPDIR:-/tmp}/dragon-rollback-probe-$$.log"
trap 'rm -f "$PROBE"' EXIT HUP INT TERM
python3 tools/s3_chipid_probe.py 2>&1 | tee "$PROBE"
if ! grep -q 'CONFIRMED: ESP32-S3 (chip ID 9)' "$PROBE"; then
  echo 'STOP: ESP32-S3 identity not confirmed' >&2
  exit 1
fi
printf '%s' 'Type RESTORE-S3-V107 to authorize rollback: '
IFS= read -r reply || exit 1
if [ "$reply" != 'RESTORE-S3-V107' ]; then
  echo 'Cancelled: no flash write'
  exit 1
fi
nrflash write --chip esp32s3 --verify "0x10000:$APP"
echo 'v1.0.7 app rollback write/MD5 complete; verify LCD TEST and TOUCH TEST physically.'
