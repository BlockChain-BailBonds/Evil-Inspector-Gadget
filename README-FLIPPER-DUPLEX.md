# Evil Inspector Gadget v1.2.0 — validated bidirectional UART1 and hardware map

**Status (2026-11-12):** Operator-verified on the connected GOOUUU ESP32-S3 N16R8, 4-inch ST7796S/FT6336U module and Flipper Zero. This document supersedes the earlier GPIO48, GPIO38/39 touch and external-reset assumptions in the original draft. See [hardware acceptance PR #2](https://github.com/BlockChain-BailBonds/Evil-Inspector-Gadget/pull/2) and [integration PR #1](https://github.com/BlockChain-BailBonds/Evil-Inspector-Gadget/pull/1).

## Installed pin map

| Function | Display / Flipper pin | ESP32-S3 GPIO | Notes |
| --- | --- | --- | --- |
| LCD SCLK | display SCK 7 | GPIO42 | SPI |
| LCD MOSI | display MOSI 6 | GPIO41 | SPI |
| LCD CS | display CS 3 | GPIO47 | SPI |
| LCD DC | display RS 5 | GPIO21 | SPI |
| LCD reset | display RST 4 | GPIO14 | active LOW |
| LCD backlight | display LED 8 | externally powered | Never drive GPIO27 on this N16R8 |
| FT6336U I²C SDA | display CTP_SDA 12 | GPIO1 | controller address 0x38 |
| FT6336U I²C SCL | display CTP_SCL 10 | GPIO2 | verified |
| FT6336U reset | display CTP_RST 11 | GPIO40 | active LOW; physically traced |
| FT6336U IRQ | display CTP_INT 13 | not connected | poll events |
| Flipper → S3 UART1 RX | Flipper pin 13 TX | GPIO3 | 3.3 V, 115200 8N1 |
| S3 UART1 TX → Flipper | Flipper pin 14 RX | GPIO7 | 3.3 V, 115200 8N1 |
| Common ground | Flipper pin 8 GND | GND | do not join 5 V rails |

**Reserved / excluded:** S3 UART0 and CH340 remain dedicated to host diagnostics; never connect Flipper TX to S3 RX0 GPIO44. GPIO27 is not a backlight output on the N16R8 memory configuration. GPIO40 conflicts with SDMMC D0, GPIO7 can conflict with the optional camera profile; camera and SDMMC must remain disabled for this wiring. SPIFFS mounting is not an accepted feature of this build. Use independently powered devices and a common ground. Confirm connector orientation and logic voltage before any rewiring.

## Framed, bounded UART protocol

All frames are newline terminated. Allowed commands from the Flipper FAP:

```text
EIG918|1|CMD|PING
EIG918|1|CMD|STATUS
EIG918|1|CMD|HOME
EIG918|1|CMD|WIFI
EIG918|1|CMD|BLE
```

Example acknowledgement and telemetry:

```text
EIG918|1|ACK|op=PING|result=OK
EIG918|1|STAT|up=10|wifi=0|ble=0
```

The parser limits frame length and rejects invalid or unknown commands. WIFI and BLE initiate **passive** scans; longer scans may temporarily delay telemetry. The FAP enables both HAL serial TX and RX directions before asynchronous receive, with a bounded initial frame-boundary synchronization after ownership transitions. No arbitrary command shell or remote control of native Flipper RF applications is provided.

## Recorded acceptance

- Cold-booted S3 application starts the LCD dashboard without an interactive USB command.
- FT6336U returned ACK at 0x38, vendor ID 0x11 and chip ID 0x64; physical four corners, center and Home button events were captured.
- Flipper-issued framed BLE commands were individually received by the S3 and invoked passive scans.
- **1,000 sequential PING/ACK round trips** completed on real devices. The stress diagnostic sent the next PING only after the previous `EIG918|1|ACK|op=PING|result=OK` response; S3 ended with `RX_COMMANDS=1000`.
- One deliberately rejected stale boundary line (`REJECTED=1`) was recorded during Flipper expansion-service synchronization; it is not one of the 1,000 requested commands.
- Host validation reported 62/62 Python regressions, C protocol, 16 MB partition checks, and successful PlatformIO and Flipper FAP builds.
- The canonical FAP was restored after temporary stress testing. The accepted FAP SHA-256 reported in PR #2 is `42be121e790dcd247af7a58fc64a2928a45ebfa0e835d1794abdc339917f4baa`.

These are operator-recorded hardware acceptance results, not direct hardware tests run by GitHub Actions. See [PR #2 evidence](https://github.com/BlockChain-BailBonds/Evil-Inspector-Gadget/pull/2#issuecomment-6832230280).

## Reproducible source/build checks

From the repository root:

```sh
sh test_all.sh
cd s3_redteam_inspector
pio run -e s3_uart_inspector
sha256sum .pio/build/s3_uart_inspector/firmware.bin
cd ../flipper_fap/inspector_gadget
ufbt
find dist -type f -name '*.fap'
```

Builds are not flashes. Preserve the full verified 16 MiB S3 flash backup and a known-good application image. Any later S3 flash must follow `s3_redteam_inspector/docs/FLASHING-SAFETY.md` and its guarded app-only process; do not erase the bootloader, partition table or existing backups. The exact build artifact hashes can change with toolchain and source revisions and do not substitute for a device read-back.

## Historical note

The original draft referenced GPIO48 UART TX, GPIO38/39 I²C and an external touch-reset pull-up because those physical nets had not been traced. Subsequent direct hardware tracing and the documented recovery run established GPIO7 UART TX and GPIO1/2/40 touch for the **actual operator-built device**. Do not restore those superseded configurations. The pre-recovery text is retained in Git history for provenance.
