# Safe flashing and recovery checklist

This package deliberately includes **no upload helper script**. The earlier CYD project used an ESP32, while this firmware targets `esp32s3`. Never flash a binary from either project onto the other chip.

1. Identify the board printing and both USB connectors (when present). N16R8 does not identify camera routing.
2. Confirm a 16MB flash chip and 8MB PSRAM; if `PSRAM=0`, do not use the camera build yet.
3. Build the `s3_usb_safe` or `s3_uart_safe` variant first. Check the firmware SHA-256.
4. Before any flash write, save the existing board firmware via your trusted ESP32-S3-capable flash tool, if possible.
5. Flash only with an ESP32-S3-specific toolchain; ESP32-S3 bootloader image offset is typically `0x0`, unlike ESP32's `0x1000`.
6. After safe build: `STATUS`; confirm the serial console and RAM. Then use the matching `eye` profile only if the camera pinout is confirmed.
7. Camera failures should not disable USB command functionality. `CAM STATUS` reports availability.
8. Avoid `LOG FORMAT CONFIRM` unless you intend to delete existing SPIFFS data.

Hardware-specific programming steps depend on whether the board exposes UART0 through CH340/CP210x or the ESP32-S3's native USB-OTG pins. If that is unknown, do not guess flashing flags from the older ESP32E.
