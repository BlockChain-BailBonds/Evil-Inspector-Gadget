# 918 Technologies Evil Inspector Gadget — S3 + Flipper GPIO FAP (v1.1.0)

**Device roles:** The ESP32-S3 N16R8 runs the 320×480 Evil Inspector Gadget dashboard on the proven ST7796S + FT6336U. The Flipper Zero runs a separate EXTERNAL FAP, displaying a monochrome image asset and live UART telemetry from the S3. The S3 camera remains disabled. Stock Flipper firmware need not be replaced.

**Hardware verified previously:** S3 boot/USB, passive Wi-Fi, ST7796S colors, FT6336U touch and v1.0.8 Dragon dashboard on the physical screen. **This v1.1.0 inspector firmware and companion FAP have not been tested on either device.**

## Critical UART0 wiring constraint

CH340 on the S3 is electrically connected to ESP32-S3 RX0/TX0; this is also the Samsung USB console at 115200 baud. The Flipper must **not drive S3 RX0** while the CH340 is connected: two push-pull transmitters can conflict electrically even if no USB commands are being sent. Software cannot eliminate this shared-wire risk.

**Safe functional link (one-way):** Power both boards OFF, disconnect the jumper from **Flipper pin 13 TX → S3 RX0**, and leave **S3 TX0 → Flipper pin 14 RX** and **S3 GND → Flipper pin 8 GND**. Do NOT connect device power pins. S3 powered via existing USB, Flipper independently powered. 3.3V logic at 115200 8N1. The FAP explicitly disables its TX direction while holding the port, but the physical disconnect is still required as other Flipper apps may reconfigure the pin.

This provides a real GPIO/FAP *telemetry* connection: S3 emits `EIG918|1|STAT|up=<seconds>|wifi=<count>|ble=<count>` every 2.5s. Flipper validates each frame, shows `S3 LIVE W# B#` on the menu and prints the last three lines in its serial monitor (hold OK). The Flipper launches native Sub-GHz, NFC, RFID, Infrared, iButton and Bluetooth screens locally after user confirmation. **No Flipper-to-S3 commands** are sent; Flipper cannot remotely trigger S3 scans in the RX-only mode. For two-way remote control, reroute the link to dedicated S3 UART1 pins confirmed free and rewire both boards; do not attempt it on the shared CH340 UART0 line.

## S3 installation (Samsung Termux)

1. Keep a SHA-256-preserved copy of the physically working v1.0.8 S3 `firmware.bin`, and your verified full 16MB S3 flash backup.
2. Download and unzip this project. From the `s3_redteam_inspector` folder:

```sh
sha256sum -c SOURCE_SHA256SUMS.txt
sh tools/test.sh
sh tools/build.sh s3_uart_inspector
```

**Build only**, no flashing. After `[SUCCESS]`, and only after the archive safety requirements are met:

```sh
sh tools/flash_inspector_after_backup.sh "$HOME/m3rmaid-s3-backups/20261031-022519-original-16m.bin"
```

The guarded script checks the full backup manifest, SHA-256, chip ID, image magic, partition bounds and requires explicit `FLASH-INSPECTOR-S3` confirmation. It writes only the application partition at `0x10000`, preserving bootloader and partitions. This original backup may predate the last working S3 firmware, so keep an independently saved v1.0.8 application binary as rollback. Avoid `tpio run` without `--build-only`.

Open the USB monitor at 115200 baud. Test `STATUS`, `INSPECTOR STATUS`, `INSPECTOR HOME`, `LCD TEST`, `TOUCH STATUS`, `LINK STATUS`, `SCAN WIFI`. The home image is flash-resident RGB565, not loaded from SD, and its buttons are touchscreen hit regions.

## Flipper FAP installation

From `flipper_fap/inspector_gadget` on a Linux PC or Termux host with the matching Flipper SDK/ufbt:

```sh
ufbt
find dist -type f -name '*.fap'
```

Copy the generated `.fap` to `/ext/apps/GPIO/` on the Flipper SD card, then open Apps → GPIO → Evil Inspector 918. Depending on firmware API version, rebuild against that same SDK version. Source and host tests are provided; **no claimed compiled FAP until `ufbt` runs successfully**.

The Flipper is a monochrome 128×64 screen, so it uses a 1-bit derived Inspector image (`flipper_fap/inspector_gadget/inspector_assets.h`), not the S3's color artwork. Hold OK in the FAP to inspect UART0 lines. **Only receive/monitor and native app launching are enabled.**

## Other files

- `s3_redteam_inspector/assets/approved_inspector_theme_poster.png`: your approved original image.
- `s3_redteam_inspector/assets/inspector_dashboard_320x480.png`: processed color screen artwork.
- `s3_redteam_inspector/src/inspector_asset.h`: actual embedded RGB565 pixel array used by the renderer.
- `flipper_fap/inspector_gadget/preview_128x64.png`: visible monochrome FAP art.
- `flipper_firmware_overlay/`: optional asset-only Flipper source-tree overlay; **not necessary for S3/FAP link** and does not replace Flipper radio services.

## Testing limits

Host tests verify image size/contents, serial telemetry format, parser edge cases, UI wiring, and no UART transmission from FAP. Real Flipper SDK/FAP compilation and physical link tests require the corresponding devices/toolchain. The new Inspector app changes no S3 LCD/touch GPIO assignments and leaves camera profile 0.
