# Hardware definition and acceptance gates

The gateway target is an ESP32-S3 module advertised as N16R8: 16 MiB flash and 8 MiB PSRAM. The module identity, flash size, PSRAM mode, board variant, and available pins must be measured with an official esptool/ESP-IDF probe before a firmware write. No camera is enabled by default; camera support requires a verified module and pin map.

UART wiring is fixed: ESP32 GPIO43 TX0 to Flipper pin 14 RX, ESP32 GPIO44 RX0 from Flipper pin 13 TX, and common ground. Signaling must be 3.3 V. GPIO43 and GPIO44 are reserved. UART0 boot output and programming conflicts must be handled during acceptance testing. The Flipper GPIO supply must not be used to power the ESP32-S3 unless its current budget is independently verified.

Physical hardware status at repository creation: `NOT TESTED`.

