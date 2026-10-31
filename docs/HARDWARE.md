# Hardware definition and acceptance gates

The gateway target is an ESP32-S3 module advertised as N16R8: 16 MiB flash and 8 MiB PSRAM. The module identity, flash size, PSRAM mode, board variant, and available pins must be measured with an official esptool/ESP-IDF probe before a firmware write. No camera is enabled by default; camera support requires a verified module and pin map.

UART wiring is fixed: ESP32 GPIO43 TX0 to Flipper pin 14 RX, ESP32 GPIO44 RX0 from Flipper pin 13 TX, and common ground. Signaling must be 3.3 V. GPIO43 and GPIO44 are reserved. UART0 boot output and programming conflicts must be handled during acceptance testing. The Flipper GPIO supply must not be used to power the ESP32-S3 unless its current budget is independently verified.

Physical hardware status at repository creation: `NOT TESTED`.

## Verified build environment

Ubuntu 24.04 ARM64 under Termux PRoot has PlatformIO Core `6.2.0` and uFBT `0.2.6`. uFBT deployed Flipper SDK `1.5.1` and its ARM64 toolchain. These versions are pinned in `tools/requirements-build-ubuntu.txt`. No firmware build has been claimed yet because the gateway and FAP implementation sources are still being added.

## Host probe toolchain

Termux native Python 3.13 cannot install `esptool` because its `cryptography` dependency has no compatible Android ARM64 wheel and attempts an unsupported Rust target. Ubuntu 24.04 ARM64 under `proot-distro` is therefore the reproducible probe environment. It contains Python 3.12 and `esptool==5.6.0` in `/root/eig-venv`; its version command was verified. A device probe is valid only when `/dev/ttyACM0` or another explicitly identified serial node is present. No flash command is part of the probe.

