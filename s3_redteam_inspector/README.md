# 918 Technologies — RedTeam Dragon S3 v1.0.8

**CURRENT FIRMWARE SOURCE**: Read [README-DRAGON.md](README-DRAGON.md) for the branded 4-inch dashboard build, guarded application flash, diagnostics, and v1.0.7 rollback.

This source is derived from the original v1.0.7 touchscreen diagnostic. The hardware-verified v1.0.7 original is not modified.

---

## Preserved baseline documentation

# M3rMa1d S3-CAM v1.0.7 — Touch diagnostic branch

**New:** Read `README-TOUCH-TEST.md` for FT6336U I²C wiring, commands, preservation, and guarded installation. The working v1.0.6 SPI LCD source remains unchanged in its own folder.

# LCD diagnostic v1.0.6 derived from original v1.0.5. See README-LCD-TEST.md first.

# IMPORTANT: Backup-first v1.0.1 safety update

Read `README-BACKUP-FIRST.md` first. This derived version fixes the old `tpio run` behavior: `tools/build.sh` now compiles without flashing.

# M3rMa1d S3-CAM Red-Team v1.0

**Target:** ESP32-S3-WROOM-1-N16R8 (16 MB quad flash / 8 MB octal PSRAM) with an **optional OV2640 camera** and one Samsung Android USB connection. **This is a DIFFERENT device from the LCDWiki E32R40T ESP32E CYD.** Do not flash the CYD with these S3 images.

**Status:** Host-source checks and parser tests are included. This release is source code, **not a proven hardware-tested binary**. Compile and test on the actual ESP32-S3 board before treating it as a production firmware. The ZIP contains no prebuilt firmware image.

## Features

- USB-only command interpreter at 115200 baud (native USB CDC or UART0 USB bridge profiles).
- Passive 2.4 GHz Wi-Fi surveys; collect SSID, BSSID, RSSI, channel, and advertised security mode **without connecting**.
- Passive BLE advertising inventory; MAC, optional advertised name, RSSI. No active scan requests or BLE connections.
- Configuration finding rules for open Wi-Fi, WEP, WPA1, WPA1/WPA2 mixed, and same SSID with different advertised security. **No exploitability claim** from advertisements.
- OV2640 one-shot JPEG still captures **only with explicit `CAM CAPTURE` over USB**. QVGA/VGA. Frames are Base64 streamed over USB with length and CRC32 for loss detection.
- Optional SPIFFS evidence log, off by default and never auto-formatted. No remote service, Wi-Fi AP, network joins, packet injection, or authentication attempts.
- The onboard N16R8 memory format is `qio_opi`; the camera pinout must be checked separately.

## First build — Samsung Termux

Download this project's ZIP to Samsung Downloads, then execute:

```sh
pkg install -y python clang unzip
cd ~
unzip -o ~/storage/downloads/M3rMa1d-S3-CAM-RedTeam-v1.0-source.zip -d ~
cd ~/M3rMa1d-S3-CAM-RedTeam-v1.0
sh tools/test.sh
sh tools/build.sh s3_usb_safe
```

`tools/build.sh` uses `tpio` if installed, else `pio`. It only executes `run -e <env>`; **NO flash/upload command**. It prints the generated firmware SHA-256 on a successful build. A failed build leaves all device flash untouched.

**Camera build only after confirming the camera GPIO:**

```sh
sh tools/build.sh s3_usb_eye
```

After a successful compile, you may build a **16 MB combined ROM file without writing to the device**:

```sh
python3 tools/pack_rom.py s3_usb_safe
sha256sum dist/M3rMa1d-S3-CAM-s3_usb_safe-full-flash.bin
```

This creates an image for later **S3-specific** flashing, not a flash-ready guarantee for an unverified physical board. It will refuse to overwrite a previous ROM file.

The `s3_usb_eye` profile maps the OV2640 D0-D7 to GPIO 11,9,8,10,12,18,17,16; XCLK 15; PCLK 13; VSYNC 6; HREF 7; SIOD 4; SIOC 5. This profile comes from Espressif's ESP32-S3-EYE mapping and is common on generic ESP32-S3-WROOM CAM boards. It is **NOT guaranteed by N16R8**. Another model might connect the camera differently. When in doubt use the `safe` build that does not touch camera GPIO.

If the board has CH340/CP210x USB-to-UART0 rather than native USB-OTG, build `s3_uart_safe` or `s3_uart_eye`; the firmware USB Serial must match the board port. On a native USB S3 the ROM boot/download interface can have different USB enumeration from the Arduino USB CDC application.

## Important — flashed-address contract

This is an ESP32-S3, **not** the original ESP32. Use `--chip esp32s3` or confirm auto-detection reports `esp32s3`. For S3 builds the bootloader typically starts at **0x0**, the partition table at **0x8000**, and the application at **0x10000**. **Never reuse the CYD/ESP32 bootloader address `0x1000` or its image.** Use the exact offsets printed by the build/upload tool for the generated firmware before flashing. Do not invoke an automatic upload until you know which physical USB port is connected and have verified a backup.

Safest staged procedure: compile and hash the `s3_usb_safe` build; record the source and board photos; confirm successful native USB/serial bring-up; only then build and flash the matching camera profile. The upload action is deliberately not automated by this package.

## Firmware command reference

```
HELP
STATUS
SCAN WIFI
SCAN BLE
SCAN ALL
FINDINGS
EXPORT
CAM STATUS
CAM SIZE QVGA
CAM SIZE VGA
CAM CAPTURE
LOG STATUS
LOG ON
LOG OFF
LOG DUMP
LOG CLEAR CONFIRM
LOG FORMAT CONFIRM
```

`SCAN WIFI` performs a passive scan, and `SCAN BLE` listens for BLE advertising. Wireless identifiers are environmental observations, not authorization or proof of vulnerabilities.

### JPEG over Samsung USB

Use a USB serial monitor with a log-capture function, 115200 baud. With the monitor running, send `CAM STATUS`, then `CAM CAPTURE`. This prints `CAM_BEGIN`, a series of `CAM_DATA` Base64 lines, and `CAM_END`. The camera does not run when you don't invoke capture. Use the text log to reconstruct and validate the image:

```sh
python3 tools/extract_jpeg.py ~/m3rmaid-s3-session.log ~/storage/downloads/m3rmaid-s3-capture.jpg
```

The converter checks the declared byte count, CRC32, and JPEG markers and refuses to overwrite an existing JPEG. If text was truncated or the host monitor did not capture the entire frame, it refuses to produce a file. For an ESP32-S3 with native USB CDC, use a serial monitor supporting that device; the earlier CH340-specific ESP32E monitor may not be compatible.

### Logging caution

SPIFFS logs store wireless device identifiers locally, unencrypted. Logging is **OFF by default**. `LOG FORMAT CONFIRM` wipes the local log partition; do not use it when the partition may contain data you need. `LOG CLEAR CONFIRM` deletes the evidence log. `LOG DUMP` sends saved records over USB. The default camera snapshot is streamed only; it is not saved to flash.

## Hardware limits

This cannot scan 5 GHz/6 GHz Wi-Fi, prove CVEs by beacon observation, or serve as Kali Linux. It does not exploit or actively attack nearby devices. Camera recognition/QR decoding is not implemented; JPEG capture is available for operator review. There is no GPS positioning in this board.

## Source provenance / pin reference

- Espressif S3-EYE camera BSP: https://github.com/espressif/esp-bsp/blob/master/bsp/esp32_s3_eye/esp32_s3_eye.json
- N16R8 memory modes: https://github.com/sivar2311/ESP32-PlatformIO-Flash-and-PSRAM-configurations
- Alternate board camera example: https://github.com/jaysys/esp32s3-with-camera-ov2640-and-sdcard/blob/main/pinmap.md

## Verification

`sh tools/test.sh` runs a native C++ risk-rules harness, Python JPEG frame parser tests, and a 16MB partition overlap/alignment verifier. This does not replace compiling the firmware with PlatformIO or testing the actual camera and USB port.

**v1.0.3:** Fixed no-root USB log startup race and ESP32-S3 stub backup raw ACK framing. See README-BACKUP-FIRST.md.

**v1.0.4:** Delay buffered SLIP reader until RAM-stub streaming, retaining nrflash ROM sync and security-ID behavior. See README-BACKUP-FIRST.md.
