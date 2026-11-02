# ESP32-S3 N16R8 Inspector firmware

`firmware.bin` is the application image for the `s3_uart_inspector` PlatformIO environment.

- Target: ESP32-S3 N16R8 behind a CH340 UART bridge
- Application offset: `0x10000`
- Application partition limit: `0x410000`
- LCD: ST7796S, 480 x 320 landscape, RGB565
- Touch: FT6336U at I2C address `0x38`, polled with CTP_INT disconnected
- Camera: disabled (`CAMERA_PROFILE=0`)
- USB CDC on boot: disabled; UART0 remains available through CH340

This is an application-only artifact. Do not write it at offset `0x0`, and do not treat a successful write as proof of physical display or touch operation. Use the repository's backup-first flash guard and verify the live chip as ESP32-S3 before writing.

The private 16 MiB device backup is intentionally not included.
