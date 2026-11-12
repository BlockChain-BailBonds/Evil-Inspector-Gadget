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
| FT6336U SDA | 1 |
| FT6336U SCL | 2 |
| FT6336U reset | 40 (active LOW) |
| FT6336U IRQ | not connected; polling |
| Flipper TX to S3 UART1 RX | 3 |
| S3 UART1 TX to Flipper RX | 7 |

The camera and SDMMC remain disabled. GPIO40 cannot simultaneously be used for
SDMMC D0 in this profile. GPIO7 cannot simultaneously be used for the
alternate camera HREF signal. UART0 remains dedicated to the CH340 USB console.
GPIO27 is reserved by the N16R8 flash/PSRAM memory interface and must not be
configured as a backlight output; doing so caused a reproducible double
exception followed by `TG1WDT_SYS_RST` during `LCD INIT`.

## Startup behavior

The hardware-verified display and FT6336U touch controller start automatically
after every boot. Touch initialization failure remains non-fatal so USB and
Flipper recovery stay available.

The dashboard remains available through USB and Flipper commands when the
touch controller is absent. A failed FT6336U probe is reported as a warning and
does not suppress screen rendering.

## Touch diagnostic result

The diagnostic now always ends any prior `Wire` instance, reads idle levels,
successfully reinitializes I2C, and only then probes. Corrected hardware result
before reset recovery: `idle_sda=1 idle_scl=1 addr38=2 ack38=NO addr48=2 ack48=NO`.
This supersedes the earlier result 5 timeout, which was affected by defective
diagnostic reinitialization. Subsequent physical tracing confirmed display pin
11 CTP_RST is connected
to GPIO40. The historical reset sequence (LOW 15 ms, HIGH, wait 250 ms) with
SDA GPIO1 and SCL GPIO2 produced `result=0 ack=YES` at `0x38`. A physical touch
was then recorded at `x=80 y=230` and activated the Home-page Wi-Fi control.
After installing the permanent mapping, a power-on boot reported
`ACK=YES vendor_id=0x11 chip_id=0x64` before starting the Home screen.

## Validation performed

- 62 Python regression tests passed.
- `s3_uart_inspector` compiled with PlatformIO 6.2.0, Espressif platform 6.5.0,
  and Arduino ESP32 2.0.14.
- Application SHA-256:
  `be823217b920f39a6f53ee342bc2620177d23a3fa3a81f10758a5434e2fd940c`.
- Flipper FAP compiled against target 7 / API 88.2.
- FAP SHA-256:
  `323113e55d3c5f19ea572dd679aea5b25eb7768601cb539c4fa9ec65767669dd`.

Compilation is not physical acceptance. Preserve and verify the current 16 MiB
flash backup before installing the application at offset `0x10000`. After
installation, validate USB recovery, LCD colors, FT6336U address `0x38`, touch
corners, UART telemetry, and bounded PING/ACK before enabling automation.

Do not connect device 5 V rails together. Use common ground and 3.3 V logic.
