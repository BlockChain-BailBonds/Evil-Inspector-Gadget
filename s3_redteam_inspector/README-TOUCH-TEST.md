# v1.0.7 — FT6336U I2C touch diagnostic — Samsung Termux (NO ROOT)

**Known-good:** v1.0.6 LCD SPI ST7796S drew verified colored bars on physical glass, and scanner USB/Wi-Fi commands worked. This release is a copy; it does NOT modify v1.0.6 or v1.0.5. Source-level unit tests do **not** prove firmware compilation or hardware touch.

**Panel identification:** photos show `4.0" TFT SPI 480X320 V1.0 Capacitive Touch` matching LCDWiki MSP4031. Official specification identifies touch controller **FT6336U**, I²C 7-bit address **0x38**, native 320x480 portrait. https://www.lcdwiki.com/res/MSP4030_MSP4031/MSP4030_MSP4031_Specification_EN_V1.0.pdf

## Power OFF before adding three wires

Use the newer confirmed wiring plan: power VCC/GND, LCD_CS->47, LCD_RST->14, LCD_RS->21, MOSI->41, SCK->42. The bare LCD's numbered 14-pin header is labeled right-to-left when its printed text is upright (VCC pin 1 far right; SD_CS pin 14 far left).

| TFT header pin | Touch label | ESP32-S3 candidate GPIO |
|---|---|---|
| **10** | CTP_SCL | **GPIO2** |
| **11** | CTP_RST | **GPIO40** |
| **12** | CTP_SDA | **GPIO1** |
| 13 | CTP_INT | leave unconnected initially |

This is a *candidate pin assignment*, NOT proof that your physical S3 N16R8 camera board exposes these GPIOs or does not use them for onboard peripherals. Confirm board labels/camera/SD wiring before connecting. If unavailable, change only `M3R_TOUCH_PIN_*` flags, wire accordingly, and rebuild. 3.3 V signal logic; 5 V belongs ONLY on the display module's VCC, not on any ESP32 GPIO. A common ground is essential.

## Backup (source and binary), then build ONLY

```sh
mkdir -p "$HOME/m3rmaid-known-good"
cp -n "$HOME/M3rMa1d-S3-CAM-LCD-Diagnostic-v1.0.6/.pio/build/s3_uart_lcd_test/firmware.bin" "$HOME/m3rmaid-known-good/s3_uart_lcd_test_v106.bin"
sha256sum "$HOME/m3rmaid-known-good/s3_uart_lcd_test_v106.bin"
# Known user-reported SHA256 of application: f01a6dfd95026329917a3c82b7885f6e23d3574d69140157935e480f5b4f3fcf
cd "$HOME/M3rMa1d-S3-CAM-Touch-Diagnostic-v1.0.7"
sha256sum -c SOURCE_SHA256SUMS.txt
sh tools/test.sh
sh tools/build.sh s3_uart_touch_test
```

Avoid plain `tpio run` (the local tpio wrapper may **flash**). `tools/build.sh` calls `--build-only`.

## Optional guarded application-only flash

Only after verifying hardware pin assignments and preserving the current known-good v1.0.6 binary, identify the *actual* complete 16 MiB backup file and run:

```sh
sh tools/flash_touch_app_after_backup.sh "$HOME/m3rmaid-s3-backups/20261031-022519-original-16m.bin"
```

The script validates the manifest, 16 MiB size, SHA256 and S3 ROM ID, then requires `FLASH-TOUCH-S3`. This writes ONLY application at `0x10000` (no bootloader, partition table or SPIFFS erasure). The cited backup predates the working v1.0.6, so the **known-good current application `.bin`** remains the primary rollback artifact; the earlier full backup may contain nonbooting firmware. If your verified backup file has another name, substitute the actual filename.

## USB test procedure (115200 baud CH340)

1. `HELP`, `STATUS`, `LCD TEST` (confirm color bars still work).
2. `TOUCH STATUS` (shows GPIOs, no I²C yet).
3. `TOUCH PROBE` (non-resetting probe of I²C address 0x38).
4. `TOUCH INIT` (explicitly pulses touch RST GPIO40, reads vendor/chip IDs; should print `ACK=YES`).
5. `TOUCH READ` (tap/hold the glass while sending it) or `TOUCH TEST` (nonblocking 30 s test; clears LCD to black, draws four corner markers, prints raw/mapped touch events and white dots). `TOUCH STOP` ends test.
6. `TOUCH ROTATE 0`, `TOUCH ROTATE 1`, `TOUCH ROTATE 2`, `TOUCH ROTATE 3` adjusts *touch coordinate transform only* if a dot appears mirrored or rotated. The LCD orientation is never changed by these commands.

**Outcomes:** `TOUCH PROBE missing=0x38` → wiring, power or I²C issue. Probe ACK but `TOUCH READ` no change → RST, flex, touch controller or event parse. Coordinate dots mirrored → change `TOUCH ROTATE`. No touch is initialized at boot; scanner, BLE, camera-off, logging and existing LCD commands remain. No camera preview or full UI in this diagnostic.

**Acceptance gate:** Physical I²C ACK + accurate touch dots in all four corners + successful `STATUS` and `SCAN WIFI`. Then build full M3rMa1d touchscreen UI on a fresh branch. If the touch build breaks anything, reinstall v1.0.6 app from its saved `firmware.bin` through the established guarded application flash procedure; preserve that image's SHA256.
