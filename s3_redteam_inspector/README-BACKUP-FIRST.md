# M3rMa1d S3-CAM Backup-First v1.0.4

This is a derived copy of v1.0, with **read-back backup support and corrected build-only behavior**. The camera and wireless firmware code are unchanged. For Samsung no-root Termux; the device is **ESP32-S3 N16R8, not the ESP32E CYD**.

## Key safety correction

The original v1.0 `tools/build.sh` called `tpio run -e ...`, but this Termux-PlatformIO build of `tpio run` **flashes by default**. The new `tools/build.sh` uses `tpio run --build-only -e ...`. **Don't run an old v1.0 build script before backup.**

## Step 1 — read existing device firmware

Connect Samsung to the ESP32-S3's USB-UART/COM port (not the CYD), close other serial monitors, and verify USB with `termux-usb -l`. Install `pip install nrflash espbridge` if necessary. Run `python3 tools/backup_s3_flash.py` and grant the USB permission prompt. This creates an immutable 16 MiB backup in `~/m3rmaid-s3-backups/` plus `.json` metadata and `.sha256`; none of these files existed before the read.

Readback uses ESP ROM + temporary RAM stub. Every 64KiB read is verified against a device-supplied MD5. It then verifies the complete file SHA256 a second time. If read fails, it retains a `.partial` and refuses to mark the backup verified. The script **never writes the SPI flash**.

**Important**: `nrflash` itself does **not** have a `read` command. The included Python backup utility implements the documented stub `READ_FLASH` protocol on top of `nrflash`'s existing raw USB transport. It is host-test-covered but not physically tested on your board. A backup from an encrypted/secure-boot S3 may not be restorable; flashing is blocked when protection is detected.

## Step 2 — build only (no flash)

Run `sh tools/test.sh` and `sh tools/build.sh s3_uart_safe` for the board's USB-UART port. The safe firmware disables the camera GPIO until the exact camera header mapping has been checked. Use `s3_usb_safe` if connected via the native USB port and not UART. Compilation is different from flashing.

## Step 3 — flash only after backup passes

Run: `sh tools/flash_after_backup.sh "$HOME/m3rmaid-s3-backups/<actual-filename>.bin" s3_uart_safe`

This verifies the backup's full size, SHA256, the device's S3 identity, and the new firmware image headers. It refuses encrypted / secure-boot targets. It requires a deliberate `FLASH-S3` acknowledgement and writes only the three compiled images with `nrflash write --verify` using **S3 offsets** `0x0`, `0x8000`, and `0x10000`. Do not use CYD offsets or its firmware. The tool's MD5 checks are per image; also confirm the application boots and serial `STATUS` works.

## Firmware functions

`STATUS`, `CAM STATUS`, `SCAN WIFI`, `SCAN BLE`, `FINDINGS`, `EXPORT`; `CAM CAPTURE` is only available after a camera-enabled build with verified GPIO pinout. The safe first flash is intended to establish a recoverable USB baseline.

## Rollback

The backup is a 16 MiB chip-specific full-flash read. Keep it private, because it can contain stored Wi-Fi credentials and other device-specific configuration. Never blindly restore the entire image to a different ESP32-S3. If the first image fails to boot, inspect the USB boot log before any rollback operation. For an unencrypted chip, restore can be performed using a verified `nrflash write --chip esp32s3 --offset 0x0 <backup.bin> --verify`, but that **overwrites the entire device flash** and should be performed only after confirming the connected device identity and the backup checksum.

## v1.0.2 update / ESP32-S3 invalid-header recovery

* Corrected chip identification in the backup utility: validate S3 chip ID 9
  using ROM `GET_SECURITY_INFO`, not the legacy magic-register heuristic used
  by `nrflash probe` (which may misidentify this board as a classic ESP32).
* The boot loop `ESP-ROM:esp32s3-20210327`, `invalid header: 0xffffffff`
  indicates the ROM cannot read a valid second-stage bootloader at offset 0x0.
  This does **not** establish that every flash region is empty.
* Before attempting readback, use the read-only quick check:

  ```sh
  sh tools/check_flash_header.sh
  ```

  `MATCH` means that 4096 bytes at offset 0x0 have the same MD5 as 0xff
  bytes, according to the ROM MD5 protocol. `MISMATCH` means only that the
  contents differ; it does not prove the bootloader is valid.
* To attempt a complete read-only 16 MiB backup, run
  `python3 tools/backup_s3_flash.py`. Stop on errors; do not flash.
* A complete backup with an all-FF boot header is a *raw-flash dump*, not a
  recovery of bootable original firmware. The 16 MiB backup remains private.

* Updated flash script to use `tools/s3_chipid_probe.py` for ROM ID 9 instead of unreliable `nrflash probe` magic. The mandatory verified-backup gate and explicit `FLASH-S3` confirmation remain.

## v1.0.3 Termux backup reliability hotfix

The `espbridge.launch_with_fd` helper starts its log-reader thread immediately; the thread may try opening the file before the USB subprocess creates it. This release pre-creates the USB log to avoid `FileNotFoundError` in `_tail`. It also corrects the stub `READ_FLASH` acknowledgement to a raw little-endian 4-byte count, matching the Espressif reference protocol; older code incorrectly SLIP-encoded that ACK. This changes only Samsung-side backup/identity tools. It does not write firmware to the ESP32-S3. If a backup fails, any `.partial` file is **not** a valid restore image. Only trust backups with `BACKUP VERIFIED`, a complete manifest, and matching SHA-256.

From Termux, run `python3 tools/backup_s3_flash.py`; do not run any flash command unless a verified backup exists and the first S3 build is hardware-compatible.

## v1.0.4 physical S3 ROM handshake fix

The v1.0.3 physical backup timed out in step `[1/4]`, while the separate `nrflash probe` previously synchronized over the same CH340. Root cause candidate: the backup replaced `RomLoader._slip_read_packet` before the ROM sync command, unlike the working `nrflash` protocol path. In v1.0.4 the known-good `nrflash` ROM reader stays active for ROM sync, GET_SECURITY_INFO, and upload of the read-only RAM stub. The buffered SLIP reader is enabled only after the RAM stub is confirmed, to handle coalesced flash-read data frames.

The diagnostic logs now distinguish ROM sync from flash readback. This change is limited to the Samsung-side backup script. It does not erase or write the ESP32-S3 flash; hardware success remains unverified.

If ROM sync still fails: disconnect the CYD, keep only S3 on USB-UART, close other monitors, manually enter DOWNLOAD mode by holding BOOT and tapping EN, then release BOOT, and retry. A still-failing handshake needs a serial log showing the actual boot mode. Do not flash until backup integrity has been demonstrated.

## v1.0.5 — SLIP ACK protocol correction

The original readback client sent raw four-byte cumulative acknowledgments to the ESP32-S3 RAM flasher stub. Espressif's `esptool.write` SLIP-frames and escapes each acknowledgment before transmission. This update performs the same framing and adds tests for payloads containing the SLIP escape byte `0xC0`. A missing acknowledgment can stall the data stream after the first sector. The hardware backup has not yet been verified on the physical S3; do not flash before `BACKUP VERIFIED`.

Reference: https://docs.espressif.com/projects/esptool/en/release-v4/esp32/advanced-topics/serial-protocol.html
