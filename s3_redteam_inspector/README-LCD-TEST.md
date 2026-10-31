# M3rMa1d S3-CAM 4-inch ST7796S LCD diagnostic v1.0.6

**Derived, non-destructive source revision** from `M3rMa1d-S3-CAM-Backup-First-v1.0.5` (original unchanged). This bundle does not include flashed or prebuilt binaries and does not demonstrate hardware operation. Keep the known-good running `s3_uart_safe` binary and its SHA-256.

## Purpose and gate

USB radio scanner on the ESP32-S3 N16R8 already works. The white 4.0" 480x320 capacitive touch ST7796S module currently has a lit backlight but **no display driver** in `s3_uart_safe`. This optional profile adds only a five-wire SPI LCD test (plus VCC and GND) while leaving the working Wi-Fi/BLE code and UART command handler intact. The camera remains disabled. Touch, SD card, and automatic display init on boot are disabled.

The S3 camera GPIO mapping is **board-dependent**. Never assume a number printed in an example is routed to the same header pin on your board. Do not flash the diagnostic profile before checking the exact S3 board's available GPIOs, camera and onboard SD/LED pin usage, and the actual wiring.

## Screen pin labels from the user's actual photos (14-pin header)

| Pin | LCD name | Tentative S3 assignment | Purpose |
|:--|:--|:--|:--|
| 1 | VCC | regulated 5V supply | module power (never GPIO) |
| 2 | GND | GND | shared ground |
| 3 | LCD_CS | GPIO47 | chip select |
| 4 | LCD_RST | GPIO14 | reset |
| 5 | LCD_RS | GPIO40 | command/data |
| 6 | SDI (MOSI) | GPIO41 | SPI output |
| 7 | SCK | GPIO42 | SPI clock |
| 8 | LED | leave disconnected | optional backlight drive |
| 9 | SDO (MISO) | leave disconnected | read not used |
| 10 | CTP_SCL | leave disconnected | future I2C touch |
| 11 | CTP_RST | leave disconnected | future I2C touch reset |
| 12 | CTP_SDA | leave disconnected | future I2C touch |
| 13 | CTP_INT | leave disconnected | future touch interrupt |
| 14 | SD_CS | leave disconnected | screen microSD |

Header view: in the user photo, **VCC is far right, SD_CS far left**, with the silkscreen text upright. Disconnect power before altering the Dupont wires. Screen module 5 V power input recommended by manufacturer; SPI GPIO logic must remain 3.3 V. A shared GND is required. If the power source cannot deliver enough display current, use an appropriately regulated separate supply with common GND; do not feed 5 V into any ESP32 GPIO.

The S3 `GPIO40` in this prototype may conflict with onboard microSD DAT0 on some camera boards. `GPIO47` and `GPIO14` may also be wired to camera on some S3 models. The program statically rejects GPIO pins ordinarily reserved for PSRAM (35–37), USB 19/20, UART0 43/44, or strapping 0/3/45/46. **This does not replace checking the actual board schematic and GPIO header.** If any pin overlaps a board peripheral, change the `M3R_LCD_PIN_*` flags in `platformio.ini` and rewire the LCD consistently.

## Compile-only (Termux no-root)

```sh
cd ~/M3rMa1d-S3-CAM-LCD-Diagnostic-v1.0.6
sha256sum -c SOURCE_SHA256SUMS.txt
sh tools/test.sh
sh tools/build.sh s3_uart_lcd_test
```

`tools/build.sh` uses `tpio run --build-only`; NEVER run plain `tpio run` because your wrapper can flash automatically. This package also corrects the Arduino `HEX` macro name collision in `src/main.cpp` using `HEX_DIGITS` (three-symbol rename, no logic change). No extra display library or network dependency is needed.

## Before flashing

* Preserve working build `.pio/build/s3_uart_safe/{bootloader,partitions,firmware}.bin` and checksum. A **build hash** is not a physical flash backup.
* If a full verified 16 MiB backup exists, use `tools/flash_lcd_app_after_backup.sh "<actual verified backup file>"`, which checks the chip and backup and requests `FLASH-LCD-S3` confirmation. The LCD update writes **only the application at 0x10000**; the known-good bootloader and partition table stay unchanged. **Do not bypass its guard if backup is incomplete.** If there is still no complete backup, report the readback blocker before rewriting flash.
* Only connect the bare display after physical pin mapping is verified. To test the first boot with zero GPIO conflicts, disconnect the display while checking that the USB console still operates.

## On-device test commands over Samsung CH340 at 115200 baud

```
STATUS
SCAN WIFI
LCD STATUS
LCD INIT
LCD TEST
LCD RED
LCD GREEN
LCD BLUE
LCD BLACK
LCD WHITE
LCD OFF
```

The display intentionally does not auto-initialize at boot. `LCD TEST` draws 8 vertical color bands in landscape orientation: red, green, blue, white, black, yellow, cyan, magenta. `LCD INIT commands_sent=YES` **only confirms that commands were issued**, not that glass responded: MISO is unwired and the bus is unidirectional. If the screen remains white, check shared ground and LCD CS/RST/RS/MOSI/SCK against the *physical* board labels, whether the SPI clock is active, and power/brownouts; do not infer success from console messages alone. Try `LCD BLACK`: if white remains white, SPI commands still are not reaching the LCD.

## Preservation

This file is a new branch. The v1.0.5 release and its original baseline are untouched. Camera pin map, storage partitions, Wi-Fi scanning, BLE scanning, evidence logging, and flash offsets unchanged. No hidden dependencies. Source manifest SHA-256 at root; hardware behavior **not verified**.

Sources: LCDWiki ST7796S MSP4031 docs (https://www.lcdwiki.com/4.0inch_Capacitive_SPI_Module_ST7796); ESPHome ILI9XXX ST7796 init (https://api-docs.esphome.io/ili9xxx__init_8h_source).
