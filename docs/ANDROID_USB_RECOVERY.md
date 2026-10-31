# Android USB recovery evidence

## Device enumeration

Captured through `termux-usb` on 2026-10-31:

```text
USB node: /dev/bus/usb/001/002
VID:PID: 1A86:7523
USB version: 1.16
Device class: 0xff / subclass 0x00 / protocol 0x00
Interface 0: vendor-specific 0xff / subclass 0x01 / protocol 0x02
Endpoint 0x82: bulk IN, max packet 32
Endpoint 0x02: bulk OUT, max packet 32
Endpoint 0x81: interrupt IN, max packet 8
```

This is a CH340/CH341-family UART bridge. It is not the native ESP32-S3 USB Serial/JTAG identity (`303A:1001`). USB presence alone is therefore insufficient to identify the board.

## Transport validation

The reviewed `7wp81x/Termux-ESP-Flasher` adapter was audited and run from an isolated environment:

```text
revision: 1df332f7b83c98df9c4bf39abb548ef7f7f5b006
version: nrflash 1.2.0
license: Apache-2.0; bundled stubs carry Apache-2.0/MIT notices
```

Observed read-only probe result:

```text
CH340 opened and configured: PASS
UART reset transport: PASS
ESP ROM bootloader sync: PASS
automatic chip identification: FAIL (timeout)
explicit esp32s3 identification: FAIL (timeout)
flash identity and characteristics: NOT TESTED
```

The adapter uses the Termux USB file descriptor and raw USB endpoints; it does not require root, `chmod`, or a Linux serial node. No write or erase command was run.

## Next physical step

With the board connected, manually hold BOOT while pressing RESET or reconnecting USB, then release BOOT after enumeration. Run the read-only probe again. If identification still times out, inspect TX/RX crossover, common ground, CH340-to-S3 wiring, and power stability. Do not authorize flashing until the ROM identity reports ESP32-S3 and flash size is read successfully.

