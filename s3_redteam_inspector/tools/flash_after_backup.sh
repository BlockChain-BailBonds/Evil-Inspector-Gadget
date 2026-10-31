#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
  echo "Usage: sh tools/flash_after_backup.sh /absolute/path/original-16m.bin [s3_uart_safe|s3_usb_safe]" >&2
  exit 2
fi
BACKUP="$1"
TARGET="${2:-s3_uart_safe}"
case "$TARGET" in
  s3_uart_safe|s3_usb_safe) ;;
  *) echo "Only safe camera-disabled targets allowed in first flash" >&2; exit 2 ;;
esac
python3 - "$BACKUP" "$TARGET" <<'PY'
import hashlib
import json
from pathlib import Path
import sys
backup = Path(sys.argv[1]).expanduser().resolve()
target = sys.argv[2]
if not backup.is_file():
    raise SystemExit("STOP: 16MB original backup not found")
manifest_path = backup.with_suffix(".json")
if not manifest_path.is_file():
    raise SystemExit("STOP: backup manifest missing")
manifest = json.loads(manifest_path.read_text())
if manifest.get("chip") != "esp32s3" or manifest.get("verified_backup") is not True:
    raise SystemExit("STOP: unverified ESP32-S3 backup")
if manifest.get("flash_size_bytes") != 16777216 or backup.stat().st_size != 16777216:
    raise SystemExit("STOP: backup is not exactly 16 MiB")
if manifest.get("flash_encryption_enabled") is not False or manifest.get("secure_boot_enabled") is not False:
    raise SystemExit("STOP: secure boot/flash encryption is enabled or unknown; no plaintext flash")
sha = hashlib.sha256()
with backup.open("rb") as f:
    for data in iter(lambda: f.read(1024 * 1024), b""):
        sha.update(data)
if sha.hexdigest() != manifest.get("sha256"):
    raise SystemExit("STOP: original backup checksum mismatch")
base = Path('.pio/build') / target
images = (("bootloader.bin", 0x0), ("partitions.bin", 0x8000), ("firmware.bin", 0x10000))
for name, off in images:
    path = base / name
    if not path.is_file() or path.stat().st_size < 100:
        raise SystemExit(f"STOP: image missing/empty: {path}")
    if off + path.stat().st_size > 16777216:
        raise SystemExit(f"STOP: image exceeds 16MB flash: {path}")
    if name != 'partitions.bin' and path.open('rb').read(1) != b'\xe9':
        raise SystemExit(f"STOP: invalid ESP image header: {path}")
print("[+] Original device backup verified:", backup)
print("[+] Original SHA256:", sha.hexdigest())
print("[+] New S3 build images verified:", target)
PY
if ! command -v nrflash >/dev/null 2>&1; then
  echo "STOP: nrflash not installed" >&2
  exit 127
fi
PROBE="${TMPDIR:-/tmp}/m3rmaid-s3-probe-$$.log"
trap 'rm -f "$PROBE"' EXIT HUP INT TERM
printf '%s\n' "[+] Confirming ESP32-S3 ROM chip ID using GET_SECURITY_INFO..."
python3 tools/s3_chipid_probe.py 2>&1 | tee "$PROBE"
if ! grep -q 'CONFIRMED: ESP32-S3 (chip ID 9)' "$PROBE"; then
  echo "STOP: S3 ROM chip ID 9 was not verified; refusing flash" >&2
  exit 1
fi
printf '\n%s\n' "Backup validated and ESP32-S3 identified."
printf '%s\n' "First flash will replace ONLY bootloader, partition-table and app regions."
printf '%s\n' "The original 16MB backup remains private and untouched."
printf 'Type FLASH-S3 to authorize writing hardware: '
IFS= read -r reply || exit 1
if [ "$reply" != "FLASH-S3" ]; then
  echo "Cancelled. Nothing flashed."
  exit 1
fi
DIR=".pio/build/$TARGET"
nrflash write --chip esp32s3 --verify \
  "0x0:$DIR/bootloader.bin" \
  "0x8000:$DIR/partitions.bin" \
  "0x10000:$DIR/firmware.bin"
echo "[+] Firmware write completed and device MD5 verification requested. Check USB boot log."
