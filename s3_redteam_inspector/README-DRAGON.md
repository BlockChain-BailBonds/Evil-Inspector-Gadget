# 918 Technologies — RedTeam Dragon S3 Firmware v1.0.8

**Device:** ESP32-S3 N16R8, bare 4-inch ST7796S SPI 480×320 + FT6336U touch.
**Based on:** the physically verified v1.0.7 touchscreen diagnostic. Original source remains unchanged.
**Visual:** `assets/approved_redteam_dragon_mockup.png` is the user-approved full image; `assets/dragon_dashboard_320x480.png` is the rasterized native on-screen artwork with false demo statistics removed; `src/dragon_asset.h` is embedded RGB565 flash pixels.

This firmware **does not depend on SPIFFS/SD to show the UI**. UI allocates ~300 KiB in PSRAM and streams a rotated framebuffer through the previously verified SPI driver. Camera remains disabled while its GPIO wiring is unverified. The USB scanner commands are retained. No Wi-Fi association, radio injection or active attack features.

## Install source in Termux and build without flashing

```sh
cd ~
unzip -o ~/storage/downloads/918-Technologies-RedTeam-Dragon-S3-v1.0.8-source.zip -d ~
cd ~/918-Technologies-RedTeam-Dragon-S3-v1.0.8
sha256sum -c SOURCE_SHA256SUMS.txt
sh tools/test.sh
sh tools/build.sh s3_uart_dragon
```

**No compilation has been performed on physical hardware for this version yet.** The generated source is intended for the same working Termux/PlatformIO setup used to build v1.0.7. Check final `[SUCCESS]` before flashing.

## Preserve the verified v1.0.7 binary

```sh
mkdir -p ~/m3rmaid-known-good
cp -n ~/M3rMa1d-S3-CAM-Touch-Diagnostic-v1.0.7/.pio/build/s3_uart_touch_test/firmware.bin ~/m3rmaid-known-good/s3_touch_v107.bin
echo "71d8864559ac6b710b9e34c8c616e2a76bf45e0b14b5ab307c7235c9c1134bf5  $HOME/m3rmaid-known-good/s3_touch_v107.bin" | sha256sum -c -
```

The verified 16 MiB original backup and the saved v1.0.7 image are different artifacts. Preserve both. No automatic filesystem formatting or erase is allowed.

## Flash ONLY after a successful new build

```sh
sh tools/flash_dragon_app_after_backup.sh "$HOME/m3rmaid-s3-backups/20261031-022519-original-16m.bin"
```

Validates backup manifest, 16 MiB size, SHA-256, security settings and ROM S3 identity before explicit `FLASH-DRAGON-S3` prompt. Writes application image only at `0x10000`. **No bootloader or partition table updates.**

## Operation

Open the existing Samsung USB serial monitor at 115200 on the S3 CH340/USB-UART port:

```sh
cd ~/Termux-Serial-Monitor
python3 serial_monitor.py --baud 115200
```

The Dragon splash/home appears at boot. Six large tiles: Wi-Fi, BLE, Camera, Logs, Settings, System. Camera page explicitly says disabled. Touch coordinates are derived from the working v1.0.7 mapping; if the interface taps are inverted, `DRAGON TOUCH FLIP` flips them 180 degrees without rewiring. Existing diagnostic `LCD TEST`, `TOUCH TEST` and passive scanner commands remain available.

USB console commands:

```text
DRAGON STATUS
DRAGON HOME
DRAGON REDRAW
DRAGON WIFI
DRAGON BLE
DRAGON CAMERA
DRAGON LOGS
DRAGON SETTINGS
DRAGON SYSTEM
DRAGON EXPORT
DRAGON TOUCH FLIP
DRAGON OFF
DRAGON START
STATUS
LCD TEST
TOUCH TEST
SCAN WIFI
SCAN BLE
FINDINGS
EXPORT
```

On Wi-Fi/BLE pages the saved scan results appear in a vertically paged list. Tapping inside the list advances by seven results. On Logs page, tapping Export prints data to the Samsung USB console. Touch actions run locally and do not join networks.

### Rollback

Retain `~/m3rmaid-known-good/s3_touch_v107.bin` and SHA-256. To restore, use the included separate guarded rollback script:

```sh
sh tools/rollback_to_v107_after_backup.sh "$HOME/m3rmaid-s3-backups/20261031-022519-original-16m.bin"
```

It validates the saved known-good application SHA-256, physical backup manifest, chip identity and app bounds before asking for `RESTORE-S3-V107`. It restores **only** the app at `0x10000`; no erase, bootloader or partition table updates.

### Verification limits

The bundled host tests validate source contracts and output artifacts. Real ESP32-S3 compilation, physical LCD rotation, touch hitboxes, PSRAM availability, scanning and post-flash boot remain to be verified on the user hardware. These should not be labeled completed before device logs and screen evidence.
