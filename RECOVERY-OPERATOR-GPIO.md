# 918 Technologies operator-verified GPIO recovery profile

This recovery branch is derived from PR #1 at
`f314f0788cefdefb7b8cf4960b1ba7cad3f845f7`. It does not replace or merge the
draft PR. It changes only the `s3_uart_inspector` hardware profile to match the
operator-confirmed physical wiring reported during the November 11, 2026
recovery session.

## Active wiring

| Function | ESP32-S3 GPIO |
|---|---:|
| LCD SCLK | 42 |
| LCD MOSI | 41 |
| LCD CS | 47 |
| LCD DC | 21 |
| LCD reset | 14 |
| LCD backlight | externally powered/unmanaged; never drive GPIO27 |
| FT6336U SDA | 38 |
| FT6336U SCL | 39 |
| FT6336U IRQ | 40 (not driven by firmware) |
| FT6336U reset | externally managed; firmware uses `-1` |
| Flipper TX to S3 UART1 RX | 3 |
| S3 UART1 TX to Flipper RX | 7 |

The camera and SDMMC remain disabled. GPIO38/39/40 cannot simultaneously be
used for SDMMC in this profile. GPIO7 cannot simultaneously be used for the
alternate camera HREF signal. UART0 remains dedicated to the CH340 USB console.
GPIO27 is reserved by the N16R8 flash/PSRAM memory interface and must not be
configured as a backlight output; doing so caused a reproducible double
exception followed by `TG1WDT_SYS_RST` during `LCD INIT`.

## Recovery-safe startup

The display and touch buses are not initialized automatically at boot. This
keeps USB diagnostics and UART1 available if a peripheral bus is disconnected
or held. Validate `LCD INIT` and `TOUCH INIT` separately, then issue
`DRAGON START`.

The dashboard remains available through USB and Flipper commands when the
touch controller is absent. A failed FT6336U probe is reported as a warning and
does not suppress screen rendering.

## Validation performed

- 62 Python regression tests passed.
- `s3_uart_inspector` compiled with PlatformIO 6.2.0, Espressif platform 6.5.0,
  and Arduino ESP32 2.0.14.
- Application SHA-256:
  `b14e466cef2ac299c01030726f354e35774008679c87f90e1028269f0af927c6`.
- Flipper FAP compiled against target 7 / API 88.2.
- FAP SHA-256:
  `323113e55d3c5f19ea572dd679aea5b25eb7768601cb539c4fa9ec65767669dd`.

Compilation is not physical acceptance. Preserve and verify the current 16 MiB
flash backup before installing the application at offset `0x10000`. After
installation, validate USB recovery, LCD colors, FT6336U address `0x38`, touch
corners, UART telemetry, and bounded PING/ACK before enabling automation.

Do not connect device 5 V rails together. Use common ground and 3.3 V logic.
