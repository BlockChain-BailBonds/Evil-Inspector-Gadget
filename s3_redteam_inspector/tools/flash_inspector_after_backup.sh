#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
if [ "$#" -ne 1 ]; then
  echo "Usage: sh tools/flash_inspector_after_backup.sh /absolute/path/verified-16m.bin" >&2
  exit 2
fi
BACKUP="$1"
TARGET="s3_uart_inspector"
python3 - "$BACKUP" "$TARGET" <<'PYINNER'
import hashlib, json, sys
from pathlib import Path
backup = Path(sys.argv[1]).expanduser().resolve()
target = sys.argv[2]
if not backup.is_file():
    raise SystemExit("STOP: verified backup file not found; no flashing")
manifest_path = backup.with_suffix('.json')
if not manifest_path.is_file():
    raise SystemExit("STOP: verified backup JSON manifest missing")
manifest = json.loads(manifest_path.read_text())
if manifest.get('chip') != 'esp32s3' or manifest.get('verified_backup') is not True:
    raise SystemExit("STOP: invalid device/backup marker")
if backup.stat().st_size != 16777216 or manifest.get('flash_size_bytes') != 16777216:
    raise SystemExit("STOP: backup must be exactly 16 MiB")
if manifest.get('flash_encryption_enabled') is not False or manifest.get('secure_boot_enabled') is not False:
    raise SystemExit("STOP: device security flags incompatible or unknown")
sha = hashlib.sha256()
with backup.open('rb') as stream:
    for chunk in iter(lambda: stream.read(1024 * 1024), b''):
        sha.update(chunk)
if sha.hexdigest() != manifest.get('sha256'):
    raise SystemExit("STOP: backup SHA256 mismatch")
app = Path('.pio/build') / target / 'firmware.bin'
if not app.is_file() or app.stat().st_size < 100:
    raise SystemExit("STOP: Evil Inspector Gadget dashboard firmware image missing; build target first")
if 0x10000 + app.stat().st_size > 0x410000:
    raise SystemExit("STOP: Evil Inspector Gadget dashboard firmware exceeds partition bounds")
if app.open('rb').read(1) != b'\xe9':
    raise SystemExit("STOP: not an Espressif application image")
print("PASS: verified 16MB backup", backup)
print("PASS: SHA256", sha.hexdigest())
print("PASS: Evil Inspector Gadget dashboard application", app, 'bytes', app.stat().st_size)
print("PASS: only application region 0x10000 will be replaced")
PYINNER
if ! command -v nrflash >/dev/null 2>&1; then
    echo "STOP: nrflash missing" >&2
    exit 127
fi
PROBE="${TMPDIR:-/tmp}/m3rmaid-s3-lcd-probe-$$.log"
trap 'rm -f "$PROBE"' EXIT HUP INT TERM
python3 tools/s3_chipid_probe.py 2>&1 | tee "$PROBE"
if ! grep -q 'CONFIRMED: ESP32-S3 (chip ID 9)' "$PROBE"; then
  echo "STOP: ESP32-S3 ROM chip ID 9 was not confirmed" >&2
  exit 1
fi
printf '%s\n' 'Application-only flash: bootloader and partition table will NOT be rewritten.'
printf '%s\n' 'This replaces the currently working M3rMa1d application with the UNVERIFIED Evil Inspector Gadget dashboard build.'
printf '%s' 'Type FLASH-INSPECTOR-S3 to authorize: '
IFS= read -r reply || exit 1
if [ "$reply" != 'FLASH-INSPECTOR-S3' ]; then
  echo "Cancelled: no flash write"
  exit 1
fi
nrflash write --chip esp32s3 --verify "0x10000:.pio/build/s3_uart_inspector/firmware.bin"
echo "FLASH-INSPECTOR-S3 app write/MD5 requested. Verify USB STATUS, INSPECTOR STATUS and LINK STATUS on the physical dashboard."
