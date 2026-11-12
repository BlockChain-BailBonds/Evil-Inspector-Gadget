# Evil Inspector Gadget v1.2.0

**918 Technologies — hardware-validated release baseline, November 12, 2026**

## Hardware-accepted capabilities

- GOOUUU ESP32-S3 N16R8 boots into the ST7796S dashboard; USB/UART0 diagnostic access remains separate.
- FT6336U touch on SDA GPIO1 / SCL GPIO2 / RESET GPIO40 responds at 0x38. Controller IDs, four corners, center and UI activation were accepted on the connected device.
- Flipper Zero GPIO FAP communicates bidirectionally over UART1 (S3 RX GPIO3 from Flipper pin 13 TX; S3 TX GPIO7 to Flipper pin 14 RX; common ground; 115200 8N1).
- **1,000 sequential PING/ACK hardware round trips** were recorded. One discarded stale framing line during initialization was separately tracked and was not counted as a PING failure.
- Bounded PING, STATUS, HOME and explicitly initiated passive Wi-Fi/BLE inventory operations are available.
- Recorded final host checks include 62 passing Python tests, C protocol test, 16 MB partition validation and passing S3 and Flipper builds.

## Provenance

- [Touch, UART, backup and 1,000-round hardware acceptance — PR #2](https://github.com/BlockChain-BailBonds/Evil-Inspector-Gadget/pull/2)
- [Integrated implementation and automated checks — PR #1](https://github.com/BlockChain-BailBonds/Evil-Inspector-Gadget/pull/1)
- Canonical installed FAP reported at acceptance: SHA-256 `42be121e790dcd247af7a58fc64a2928a45ebfa0e835d1794abdc339917f4baa`.
- The recorded full 16 MiB pre-change S3 backup was independently hashed and retained; **no original device backup is distributed in the release**.
- Release binary assets, where included, are newly generated CI build outputs with their own SHA-256 manifest. Their hashes may differ from the physically installed acceptance image and do not themselves constitute a fresh hardware run.

## Exclusions and precautions

- Camera, on-board SDMMC and unvalidated SPIFFS persistence are **not** part of the accepted release profile.
- The Flipper's native Sub-GHz, NFC, RFID, infrared and iButton apps remain under direct Flipper control. The UART protocol does not remotely run native RF features or execute arbitrary commands.
- Do not use obsolete GPIO48 TX, GPIO38/39 touch mapping, or S3 UART0/CH340 for Flipper TX.
- The LCD backlight is externally powered: GPIO27 must not be driven in this N16R8 configuration.
- Use 3.3 V UART logic, independent USB supplies and common ground. Keep the original verified 16 MiB backup and known-good app binary before any flash.
- The firmware artifact is an S3 application image for the configured partition; it is **not** a full-chip backup or a generic image for other ESP32 boards.
- Before installing a newly compiled Flipper FAP, verify compatibility with the actual Flipper firmware SDK/API.

## Documentation

- [Main v1.2.0 README](README.md)
- [Operator-traced GPIO and recovery profile](RECOVERY-OPERATOR-GPIO.md)
- [Current UART1 protocol and pin map](README-FLIPPER-DUPLEX.md)
- [Guarded flashing/rollback guidance](s3_redteam_inspector/docs/FLASHING-SAFETY.md)
