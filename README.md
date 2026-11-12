# 918 Technologies — Evil Inspector Gadget v1.2.0

**Release baseline:** November 12, 2026. **Status: hardware-validated for the bounded Inspector features below** on the operator's GOOUUU ESP32-S3 N16R8 with 4-inch ST7796S/FT6336U display and Flipper Zero. This is the current bidirectional UART1 implementation, not the superseded one-way UART0 v1.1 configuration.

## Validated hardware and behavior

| Subsystem | Current configuration | Validation evidence |
| --- | --- | --- |
| ESP32-S3 boot and recovery | 16 MB flash / 8 MB OPI PSRAM; CH340 USB/UART0 untouched | Power-on display and USB boot logs reported |
| ST7796S LCD | SCLK GPIO42, MOSI GPIO41, CS GPIO47, DC GPIO21, RESET GPIO14; backlight externally powered | Dashboard starts after cold boot; screen rendering accepted |
| FT6336U touch | SDA GPIO1, SCL GPIO2, active-low reset GPIO40; IRQ unwired (polling); I²C address 0x38 | Controller ACK; vendor 0x11 / chip 0x64; four corners, center, Home control |
| Flipper TX → S3 RX | Flipper header pin 13 → ESP32-S3 UART1 GPIO3 | Framed passive BLE commands and 1,000 PING commands received |
| S3 TX → Flipper RX | ESP32-S3 UART1 GPIO7 → Flipper header pin 14 | 1,000 corresponding ACK frames parsed by the Flipper |
| UART electrical settings | Common GND, separate USB power; 3.3 V logic, 115200 8N1 | Bidirectional, sequential PING/ACK acceptance |
| Host regressions / builds | Python, C parser, partition validation, PlatformIO, Flipper uFBT | 62/62 Python tests and successful build workflows recorded |
| Passive inventory | Wi-Fi/BLE scans initiated by explicitly bounded commands | Operator-reported device tests |
| Camera and SDMMC | Disabled in the inspected firmware profile | Not included in v1.2.0 acceptance |
| SPIFFS persistence | Mount not validated | Not a v1.2.0 feature or acceptance claim |

**Acceptance provenance:** [PR #2 — GPIO recovery and hardware run](https://github.com/BlockChain-BailBonds/Evil-Inspector-Gadget/pull/2) documents physical touch calibration and the completed 1,000 sequential PING/ACK test on November 12, 2026. [PR #1](https://github.com/BlockChain-BailBonds/Evil-Inspector-Gadget/pull/1) contains the integrated source changes and CI validation. These are operator-captured hardware results, not hardware tests performed by GitHub-hosted CI.

**Validated command scope:** The Flipper FAP sends `PING`, `STATUS`, `HOME`, passive `WIFI` inventory and passive `BLE` inventory to the S3. The S3 replies with framed `ACK`, status and bounded errors. Native Flipper Sub-GHz, NFC, RFID, infrared and iButton applications remain independent and require local user interaction; this bridge does not remotely run them. It is not an arbitrary command-execution interface.

**Physical wiring safety:** Do **not** attach Flipper TX to S3 UART0/CH340 RX (GPIO44), drive GPIO27 as LCD backlight, drive 5 V into an S3 GPIO, join the devices' 5 V rails, or enable SDMMC/camera pins conflicting with GPIO40/GPIO7. Keep the original full 16 MiB verified S3 backup and previously working application image for rollback. See [operator recovery wiring](RECOVERY-OPERATOR-GPIO.md) and [UART1 protocol guide](README-FLIPPER-DUPLEX.md).

## Copy-ready Codex automation prompt

Use the following prompt from the Linux host physically connected to the S3 and Flipper. “Complete automation” here means the repository's bounded UART commands, diagnostics, dashboard, passive Wi-Fi/BLE inventory, builds, backups, and evidence capture. It does **not** authorize arbitrary shell execution, unattended RF transmission, or remote control of native Flipper Sub-GHz, NFC, RFID, infrared, or iButton operations.

```text
Continue the existing 918 Technologies Evil Inspector Gadget project. Do not
start over, replace approved artwork, erase either device, or change physical
wiring without direct operator instruction.

Verified hardware baseline (2026-11-12):
- GOOUUU ESP32-S3 N16R8; 16 MB flash and 8 MB PSRAM.
- ST7796S LCD: SCLK42, MOSI41, CS47, DC21, RESET14; backlight external/unmanaged.
- FT6336U: SDA1, SCL2, active-low RESET40, address 0x38; IRQ unwired/polling.
- Flipper UART1: Flipper pin13 TX -> S3 GPIO3 RX; S3 GPIO7 TX ->
  Flipper pin14 RX; common GND; independent USB power; 115200 8N1.
- Never use S3 UART0/CH340 GPIO44 for Flipper TX. Never join 5 V rails.
- Camera and SDMMC remain disabled.

Repository and tools:
- Repository: BlockChain-BailBonds/Evil-Inspector-Gadget
- Preserve the existing checkout and all uncommitted user files.
- Preserve the uFBT cache; never run `ufbt clean` or force an SDK update.
- Use an isolated checkout/worktree for changes.

Execute in this order:

1. Record provenance before editing:
   pwd
   git status --short
   git remote -v
   git branch -a
   git rev-parse HEAD
   git log -10 --oneline --decorate

2. Locate and hash, without modifying, the verified 16 MiB S3 backup,
   known-good application images, current PlatformIO firmware, and FAP files.
   Record real paths, sizes, SHA-256 values, source commits, and flash offsets.
   Stop if no verified rollback image exists before any S3 flash operation.

3. Verify tools and devices:
   cc --version; gcc --version; g++ --version; python3 --version
   pio --version; ufbt --version
   lsusb
   ls -l /dev/ttyUSB* /dev/ttyACM*
   Expected: S3 CH340 on /dev/ttyUSB* and Flipper CDC on /dev/ttyACM*.

4. Run host validation from the repository root:
   sh test_all.sh
   Require all Python tests, the C protocol parser, backup protocol tests,
   partition checks, GPIO collision checks, and asset checks to pass. Save the
   complete log and exit code. A host pass is not a physical hardware pass.

5. Build only; do not flash automatically:
   cd s3_redteam_inspector
   pio run -e s3_uart_inspector
   sha256sum .pio/build/s3_uart_inspector/firmware.bin
   cd ../flipper_fap/inspector_gadget
   ufbt
   sha256sum dist/evil_inspector_918.fap
   Require Flipper target 7 / installed-firmware-compatible API.

6. Confirm the FAP enables both independent HAL directions before async RX:
   furi_hal_serial_enable_direction(app->serial, FuriHalSerialDirectionTx)
   furi_hal_serial_enable_direction(app->serial, FuriHalSerialDirectionRx)
   Preserve the newline plus 50 ms frame-boundary synchronization used when
   taking UART ownership from the Flipper expansion service.

7. Install only the built FAP to Flipper microSD /ext/apps/GPIO/. Do not alter
   Flipper firmware. Verify the copied file's SHA-256 by reading it back. Remove
   obsolete duplicate diagnostic FAPs after preserving their hashes.

8. Monitor S3 USB at 115200 without reopening the port during acceptance.
   Confirm these observables:
   - LCD dashboard starts after reboot.
   - TOUCH INIT ACK=YES at 0x38, vendor 0x11, chip 0x64.
   - FLIPPER LINK UART1 GPIO3=RX GPIO7=TX 115200 8N1.
   - LINK STATUS reports CH340_UART0=UNTOUCHED.

9. Validate bounded FAP controls in its serial monitor:
   UP=PING, DOWN=STATUS, LEFT=HOME, RIGHT=passive WIFI, OK=passive BLE.
   Confirm ACK and status frames return to the FAP. Do not add arbitrary UART
   command forwarding or trigger native Flipper radio transmissions.

10. Run a sequential 1,000-round PING/ACK acceptance test. Send the next PING
    only after parsing the prior exact response:
    EIG918|1|ACK|op=PING|result=OK
    Pass only if 1,000 commands and 1,000 matching ACKs complete with no timeout.
    Record S3 LINK STATUS counters and the FAP result. A deliberate initial
    frame-boundary flush may create one rejected stale line; document it
    separately and do not count it as a command round.

11. Restore the canonical non-stress FAP after testing, rebuild it, reinstall
    it, read it back, and verify SHA-256. Remove the temporary stress FAP.

12. Re-run sh test_all.sh and both builds. Use VERIFIED, UNVERIFIED, BLOCKED,
    or FAILED for every subsystem. Do not claim unsupported Flipper-native or
    wireless capabilities.

13. Commit only tracked source, tests, and documentation. Exclude .pio,
    .host-build, .vscode, dist, SDK caches, backups, serial logs containing
    nearby device addresses, and generated binaries. Publish to a feature
    branch, update the pull request with hashes and exact evidence, verify CI,
    and merge only when the expected head SHA is unchanged and all required
    checks plus physical acceptance pass.

Final report must include exact commands, exit codes, hashes, device paths,
test counts, UART counters, rollback paths, GitHub commit/PR links, and any
remaining blockers. Never erase, format, or flash without an explicit guarded
step and a verified recovery image.
```

**Hardware accepted on the operator's device (2026-11-12):** S3 boot/USB, passive Wi-Fi/BLE inventory, ST7796S rendering, FT6336U touch, dashboard startup, dedicated UART1 in both directions, and 1,000 sequential PING/ACK exchanges. The v1.1 UART0 text below is retained only as historical rollback documentation.

## Historical v1.1 UART0 design — superseded

The prior v1.1 prototype used one-way S3 UART0 telemetry because S3 RX0 was shared with the CH340 USB adapter. **Do not use the old v1.1 wiring or follow the former one-way instructions on the completed v1.2.0 build.** The correct, tested link is separate UART1 on GPIO3 (RX) and GPIO7 (TX), with USB/UART0 reserved for diagnostics. Historical documentation and code remain accessible in the Git history (pre-v1.2.0 main commit `5a2f8ee60e391f55d07f1fbdf4da94269e2c5bb8`).

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

Copy the generated `.fap` to `/ext/apps/GPIO/` on the Flipper SD card, then open Apps → GPIO → Evil Inspector 918. Depending on firmware API version, rebuild against that same SDK version. The v1.2.0 FAP has passed host/CI builds; run `ufbt` against an SDK compatible with the actual installed Flipper firmware when rebuilding it locally.

The Flipper is a monochrome 128×64 screen, so it uses a 1-bit derived Inspector image (`flipper_fap/inspector_gadget/inspector_assets.h`), not the S3's color artwork. Hold OK in the FAP to open the bounded UART1 monitor. Its fixed commands are PING, STATUS, HOME, passive WIFI inventory, and passive BLE inventory.

## Other files

- `s3_redteam_inspector/assets/approved_inspector_theme_poster.png`: your approved original image.
- `s3_redteam_inspector/assets/inspector_dashboard_320x480.png`: processed color screen artwork.
- `s3_redteam_inspector/src/inspector_asset.h`: actual embedded RGB565 pixel array used by the renderer.
- `flipper_fap/inspector_gadget/preview_128x64.png`: visible monochrome FAP art.
- `flipper_firmware_overlay/`: optional asset-only Flipper source-tree overlay; **not necessary for S3/FAP link** and does not replace Flipper radio services.

## Verification scope and release notes

The completed hardware acceptance is reported in the PR #2 operator logs: 1,000 sequential PING/ACK round trips, with the next PING transmitted only after the previous ACK was parsed. A single deliberate stale-line synchronization rejection was recorded separately and not counted as a failed PING. The Flipper receive-direction correction enables `FuriHalSerialDirectionRx` before asynchronous reception.

GitHub Actions verifies source tests and firmware/FAP compilation; it **does not** physically test an attached ESP32-S3 or Flipper. Touch, UART and startup acceptance was performed on the operator's connected hardware. The production scope excludes onboard camera, SDMMC and unvalidated SPIFFS persistence. Do not treat the older pre-correction dist manifest as an installed v1.2.0 hardware acceptance record.

See [v1.2.0 release notes](RELEASE_NOTES_v1.2.0.md) for provenance, included features, and compatibility constraints.
