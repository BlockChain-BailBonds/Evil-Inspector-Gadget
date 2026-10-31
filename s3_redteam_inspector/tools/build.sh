#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
TARGET="${1:-s3_uart_safe}"
case "$TARGET" in
  s3_usb_eye|s3_uart_eye|s3_usb_safe|s3_uart_safe|s3_uart_lcd_test|s3_uart_touch_test|s3_uart_dragon|s3_uart_inspector) ;;
  *) echo "Invalid build target: $TARGET" >&2; exit 2 ;;
esac
if command -v tpio >/dev/null 2>&1; then
  echo "BUILD ONLY using tpio --build-only: $TARGET"
  tpio run --build-only -e "$TARGET"
elif command -v pio >/dev/null 2>&1; then
  echo "BUILD ONLY using pio: $TARGET"
  pio run -e "$TARGET"
else
  echo "Missing tpio/pio" >&2
  exit 127
fi
for item in bootloader.bin partitions.bin firmware.bin; do
  test -s ".pio/build/$TARGET/$item" || {
    echo "Missing build image: $item" >&2; exit 1;
  }
done
sha256sum ".pio/build/$TARGET/bootloader.bin" ".pio/build/$TARGET/partitions.bin" ".pio/build/$TARGET/firmware.bin"
echo "Compilation complete. No hardware has been flashed by this script."
