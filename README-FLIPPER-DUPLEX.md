# 918 Technologies — Evil Inspector Gadget 1.2 / Flipper bidirectional link

## Status and provenance

This is a source-only candidate implementing full-duplex 3.3 V UART between a GOOUUU ESP32-S3-CAM N16R8, its existing 4-inch ST7796S/FT6336U screen, and a Flipper Zero GPIO FAP. It was derived from the version 1.1 application already in this repository. The original \`main\` branch and known-good firmware must remain untouched.

**NOT hardware verified.** Neither GPIO3 strapping behavior with Flipper TX attached nor GPIO48 UART output through its onboard WS2812 load has been electrically validated. Do not connect or flash until that gate passes. App compilation and Flipper FAP compilation are separate gates.

## Signal assignments

| Function | 4-inch TFT pin | GOOUUU ESP32-S3 pin |
|---|---:|---:|
| Module supply | 1 VCC | regulated module 5V |
| Ground | 2 GND | GND |
| LCD select | 3 LCD_CS | GPIO47 |
| LCD reset | 4 LCD_RST | GPIO14 |
| LCD D/C | 5 LCD_RS | GPIO21 |
| LCD MOSI | 6 MOSI | GPIO41 |
| LCD clock | 7 SCK | GPIO42 |
| Backlight | 8 LED | module default (no ESP GPIO) |
| LCD MISO | 9 MISO | not connected |
| Capacitive SCL | 10 CTP_SCL | GPIO2 (status LED shared, no LED software updates) |
| Capacitive RESET | 11 CTP_RST | external 10k pull-up to 3.3V, **not GPIO40** |
| Capacitive SDA | 12 CTP_SDA | GPIO1 |
| Capacitive IRQ | 13 CTP_INT | not connected; polling |
| Display microSD select | 14 SD_CS | not connected |

| Flipper Zero connector | GOOUUU pin | Direction |
|---|---|---|
| GPIO pin 13, UART TX | ESP32 GPIO3, UART1 RX | Flipper sends commands |
| GPIO pin 14, UART RX | ESP32 GPIO48, UART1 TX | S3 sends ACK, status |
| GPIO pin 8, GND | ESP32 GND | common ground |

**Do not connect** Flipper pin 13 TX to GOOUUU GPIO44 (UART0 RX tied to CH340). Never connect 5V from Flipper to ESP GPIO, or connect the two devices' 5V rails together. Power each by its own supported USB port; share GND only. Use 3.3V logic; 115200 baud, 8N1.

### Electrical gates before connecting

1. Confirm the actual GOOUUU PCB header exposes GPIO3 and GPIO48 and trace pin identities from silkscreen/schematic. Never choose positions from an image without checking orientation.
2. GPIO3 is a boot-strapping pin. The Flipper TX idle-high signal can change the sampled boot strap. Verify cold boot, reset, native USB, and CH340 recovery with Flipper physically connected **before** relying on this mapping. A series resistor alone does not guarantee an unchanged strap; isolate or buffer the Flipper TX line until after boot if needed.
3. GPIO48 drives the onboard WS2812 RGB LED, which may blink or corrupt UART signaling. Disable all WS2812 code and scope the UART output with LED load attached. Use a buffer or a different interface if the output waveform fails.
4. GPIO38=SDMMC CMD, GPIO39=SDMMC CLK, GPIO40=SDMMC D0. Do not run legacy touch-reset test configurations that toggle GPIO40 while using the onboard SD slot.
5. CTP_RST must remain inactive HIGH through a suitable external pull-up (nominal 10k to 3.3V). Test FT6336U I2C ACK at 0x38, corner mappings, and repeated power cycles. There is no touch reset GPIO in this build.
6. Camera GPIO4-18 (except 14) are reserved; **camera remains disabled** by \`CAMERA_PROFILE=0\` in this build. The onboard SD pins are reserved, but the new code does not claim the SD filesystem is mounted.
7. Preserve the last physically verified LCD/touch firmware binary and a verified flash backup before any installation.

If the GPIO3/48 hardware gate fails, do not force this wiring; a dedicated SPI/I2C UART bridge or a hardware-isolated serial design is required. The code does not claim a fully conflict-free physical pinout until tested.

## Frame protocol

S3 -> Flipper, every 2.5 s:
\`\`\`
EIG918|1|STAT|up=10|wifi=0|ble=0
\`\`\`

Flipper -> S3 on Flipper monitor key presses:
\`\`\`
EIG918|1|CMD|PING
EIG918|1|CMD|STATUS
EIG918|1|CMD|HOME
EIG918|1|CMD|WIFI
EIG918|1|CMD|BLE
\`\`\`

S3 replies:
\`\`\`
EIG918|1|ACK|op=PING|result=OK
EIG918|1|ERR|UNKNOWN_COMMAND
\`\`\`

The S3 uses bounded ASCII newline-delimited input with no arbitrary command execution or shell. WIFI and BLE commands are explicit passive inventory scans; those scans block the main loop during scanning, so telemetry and responses may pause. A link timeout during a scan is not proof the physical cable failed.

Flipper FAP monitor controls: UP=PING, DOWN=STATUS, LEFT=HOME, RIGHT=passive WIFI scan, OK=passive BLE scan, BACK=exit monitor. The native Flipper applications remain separate and run under Flipper control; this UART bridge does not launch RF transmissions or remote exploits.

## Build and validation (no automatic flashing)

S3:
\`\`\`sh
cd s3_redteam_inspector
python3 -m unittest discover -s tests -v
pio run -e s3_uart_inspector
sha256sum .pio/build/s3_uart_inspector/firmware.bin
\`\`\`

Flipper FAP (match SDK to installed Flipper firmware):
\`\`\`sh
cd flipper_fap/inspector_gadget
ufbt
find dist -type f -name '*.fap'
\`\`\`

At repository root:
\`\`\`sh
sh test_all.sh
\`\`\`

Use the repository's original **guarded app-only** flash path only after a verified full backup and known-good rollback application are present. Test USB console first, then screen colors/touch, then one-way UART1 S3-to-Flipper telemetry, then Flipper TX PING/ACK and STATUS, then HOME, then explicitly requested passive scans. A passing source test is not proof of hardware performance.

## Acceptance checklist

- [ ] S3 build produces a valid application image and checksum.
- [ ] Flipper FAP builds against actual firmware SDK.
- [ ] Verified fallback binary/flash backup retained, never overwritten.
- [ ] GPIO3 strapping and USB recovery validated with Flipper connected.
- [ ] GPIO48 UART waveform checked with WS2812 onboard load.
- [ ] ST7796S RGB bars and FT6336U four-corner touches verified.
- [ ] Flipper TX -> S3 UART1 RX receives PING and S3 returns ACK.
- [ ] 1000 bidirectional PING/ACK cycles measured with no corruption.
- [ ] Camera remains disabled; onboard SDMMC GPIOs left untouched.
