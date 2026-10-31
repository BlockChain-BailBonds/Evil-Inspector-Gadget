#pragma once
// M3rMa1d LCD diagnostic for a 4.0" ST7796S SPI 480x320 capacitive module.
// Uses only the Arduino ESP32 SPI core (no TFT_eSPI dependency).
// Physical pin-map on ESP32-S3 N16R8 CAM boards is NOT standardized.
// LCD is explicitly initialized by LCD INIT / LCD TEST from USB; never at boot.
#include <Arduino.h>

#ifndef M3R_LCD_ENABLED
#define M3R_LCD_ENABLED 0
#endif
#ifndef CAMERA_PROFILE
#define CAMERA_PROFILE 0
#endif
#if M3R_LCD_ENABLED
#if CAMERA_PROFILE != 0
#error "LCD diagnostic must not be built with an unverified active camera GPIO profile"
#endif
#ifndef M3R_LCD_PIN_CS
#error "Must specify M3R_LCD_PIN_CS"
#endif
#ifndef M3R_LCD_PIN_RST
#error "Must specify M3R_LCD_PIN_RST"
#endif
#ifndef M3R_LCD_PIN_DC
#error "Must specify M3R_LCD_PIN_DC"
#endif
#ifndef M3R_LCD_PIN_MOSI
#error "Must specify M3R_LCD_PIN_MOSI"
#endif
#ifndef M3R_LCD_PIN_SCK
#error "Must specify M3R_LCD_PIN_SCK"
#endif
#define LCD_UNSAFE_PIN(p) ((p)==0 || (p)==3 || (p)==19 || (p)==20 || (p)==35 || (p)==36 || (p)==37 || (p)==43 || (p)==44 || (p)==45 || (p)==46)
#if LCD_UNSAFE_PIN(M3R_LCD_PIN_CS) || LCD_UNSAFE_PIN(M3R_LCD_PIN_RST) || LCD_UNSAFE_PIN(M3R_LCD_PIN_DC) || LCD_UNSAFE_PIN(M3R_LCD_PIN_MOSI) || LCD_UNSAFE_PIN(M3R_LCD_PIN_SCK)
#error "A configured LCD GPIO is reserved for USB/UART/PSRAM/strapping"
#endif
#if (M3R_LCD_PIN_CS == M3R_LCD_PIN_RST) || (M3R_LCD_PIN_CS == M3R_LCD_PIN_DC) || (M3R_LCD_PIN_CS == M3R_LCD_PIN_MOSI) || (M3R_LCD_PIN_CS == M3R_LCD_PIN_SCK) || (M3R_LCD_PIN_RST == M3R_LCD_PIN_DC) || (M3R_LCD_PIN_RST == M3R_LCD_PIN_MOSI) || (M3R_LCD_PIN_RST == M3R_LCD_PIN_SCK) || (M3R_LCD_PIN_DC == M3R_LCD_PIN_MOSI) || (M3R_LCD_PIN_DC == M3R_LCD_PIN_SCK) || (M3R_LCD_PIN_MOSI == M3R_LCD_PIN_SCK)
#error "LCD GPIOs must be distinct"
#endif
#include <SPI.h>
#endif

namespace mermaid_lcd {
#if M3R_LCD_ENABLED
constexpr uint16_t WIDTH = 480;
constexpr uint16_t HEIGHT = 320;
constexpr uint32_t SPI_HZ = 10000000UL;  // conservative speed for Dupont wiring
static bool initialized = false;

inline void beginTransaction() {
  SPI.beginTransaction(SPISettings(SPI_HZ, MSBFIRST, SPI_MODE0));
  digitalWrite(M3R_LCD_PIN_CS, LOW);
}
inline void endTransaction() {
  digitalWrite(M3R_LCD_PIN_CS, HIGH);
  SPI.endTransaction();
}
inline void writeCommandData(uint8_t command, const uint8_t *args, size_t argCount) {
  beginTransaction();
  digitalWrite(M3R_LCD_PIN_DC, LOW);
  SPI.transfer(command);
  if (argCount != 0) {
    digitalWrite(M3R_LCD_PIN_DC, HIGH);
    for (size_t i=0; i<argCount; ++i) SPI.transfer(args[i]);
  }
  endTransaction();
}
inline void command(uint8_t value) {
  writeCommandData(value, nullptr, 0);
}
inline void one(uint8_t address, uint8_t value) {
  writeCommandData(address, &value, 1);
}
inline void sequence(uint8_t address, const uint8_t *data, size_t count) {
  writeCommandData(address, data, count);
}
inline void initialize() {
  if (initialized) return;
  pinMode(M3R_LCD_PIN_CS, OUTPUT);
  pinMode(M3R_LCD_PIN_RST, OUTPUT);
  pinMode(M3R_LCD_PIN_DC, OUTPUT);
  digitalWrite(M3R_LCD_PIN_CS, HIGH);
  digitalWrite(M3R_LCD_PIN_DC, HIGH);
  digitalWrite(M3R_LCD_PIN_RST, HIGH);
  SPI.begin(M3R_LCD_PIN_SCK, -1, M3R_LCD_PIN_MOSI, M3R_LCD_PIN_CS);
  delay(20);
  digitalWrite(M3R_LCD_PIN_RST, LOW); delay(25);
  digitalWrite(M3R_LCD_PIN_RST, HIGH); delay(150);
  command(0x01); delay(150); // SWRESET
  command(0x11); delay(150); // SLPOUT
  one(0xF0, 0xC3);  // manufacturer register enable
  one(0xF0, 0x96);
  one(0x36, 0x28);  // landscape: MV=1, BGR=1 (480x320)
  one(0x3A, 0x55);  // 16-bit RGB565 pixels
  one(0xB4, 0x01);
  const uint8_t functionControl[] = {0x80, 0x02, 0x3B};
  sequence(0xB6, functionControl, sizeof(functionControl));
  const uint8_t adjustment[] = {0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33};
  sequence(0xE8, adjustment, sizeof(adjustment));
  one(0xC1, 0x06);
  one(0xC2, 0xA7);
  one(0xC5, 0x18);
  const uint8_t gammaPlus[] = {0xF0, 0x09, 0x0B, 0x06, 0x04, 0x15, 0x2F, 0x54, 0x42, 0x3C, 0x17, 0x14, 0x18, 0x1B};
  const uint8_t gammaMinus[] = {0xE0, 0x09, 0x0B, 0x06, 0x04, 0x03, 0x2B, 0x43, 0x42, 0x3B, 0x16, 0x14, 0x17, 0x1B};
  sequence(0xE0, gammaPlus, sizeof(gammaPlus));
  sequence(0xE1, gammaMinus, sizeof(gammaMinus));
  one(0xF0, 0x69);
  one(0xF0, 0x3C);
  command(0x13); // normal display mode
  command(0x29); // DISPON
  delay(120);
  initialized = true; // successful SPI transmission, not proof panel responded
}
inline void window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  const uint8_t column[] = {uint8_t(x0 >> 8), uint8_t(x0), uint8_t(x1 >> 8), uint8_t(x1)};
  const uint8_t row[] = {uint8_t(y0 >> 8), uint8_t(y0), uint8_t(y1 >> 8), uint8_t(y1)};
  sequence(0x2A, column, 4);
  sequence(0x2B, row, 4);
}
inline void rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rgb565) {
  if (!initialized || x >= WIDTH || y >= HEIGHT || w == 0 || h == 0) return;
  if (w > WIDTH - x) w = WIDTH - x;
  if (h > HEIGHT - y) h = HEIGHT - y;
  window(x, y, uint16_t(x+w-1), uint16_t(y+h-1));
  uint8_t chunk[128];
  for (size_t i=0;i<sizeof(chunk);i+=2) {
    chunk[i] = uint8_t(rgb565 >> 8);
    chunk[i+1] = uint8_t(rgb565);
  }
  beginTransaction();
  digitalWrite(M3R_LCD_PIN_DC, LOW);
  SPI.transfer(0x2C); // RAMWR
  digitalWrite(M3R_LCD_PIN_DC, HIGH);
  size_t bytes = size_t(w) * h * 2;
  while (bytes) {
    size_t count = bytes < sizeof(chunk) ? bytes : sizeof(chunk);
    SPI.writeBytes(chunk, count);
    bytes -= count;
  }
  endTransaction();
}
inline void bars() {
  static const uint16_t colors[8] = {0xF800, 0x07E0, 0x001F, 0xFFFF, 0x0000, 0xFFE0, 0x07FF, 0xF81F};
  for (uint8_t i=0;i<8;++i) {
    rect(uint16_t(i * 60), 0, 60, HEIGHT, colors[i]);
    yield();
  }
}
inline void status(Print &out) {
  out.printf("LCD STATUS enabled=YES spi=ST7796S size=480x320 init_sent=%s cs=%d rst=%d dc=%d mosi=%d sck=%d hz=%lu touch=OFF camera=OFF\n",
             initialized ? "YES" : "NO", M3R_LCD_PIN_CS, M3R_LCD_PIN_RST, M3R_LCD_PIN_DC, M3R_LCD_PIN_MOSI, M3R_LCD_PIN_SCK, (unsigned long)SPI_HZ);
}
inline void handle(const String &cmd, Print &out) {
  if (cmd == "LCD STATUS") {status(out); return;}
  if (cmd == "LCD OFF") {
    if (initialized) command(0x28);
    out.println("LCD OFF command_sent=YES (no LCD readback)");
    return;
  }
  if (cmd == "LCD INIT") {
    initialize();
    command(0x29);
    out.println("LCD INIT commands_sent=YES (visual confirmation required)");
    return;
  }
  uint16_t color=0;
  if (cmd == "LCD RED") color=0xF800;
  else if (cmd == "LCD GREEN") color=0x07E0;
  else if (cmd == "LCD BLUE") color=0x001F;
  else if (cmd == "LCD WHITE") color=0xFFFF;
  else if (cmd == "LCD BLACK") color=0x0000;
  else if (cmd != "LCD TEST") {out.println("LCD ERROR UNKNOWN; use HELP"); return;}
  initialize();
  command(0x29);
  if (cmd == "LCD TEST") bars();
  else rect(0, 0, WIDTH, HEIGHT, color);
  out.print("LCD DRAW command_sent=YES mode="); out.println(cmd);
  out.println("LCD VERIFY: confirm colors on glass; SPI write alone cannot prove display operation");
}
#else
inline void status(Print &out) {
  out.println("LCD STATUS enabled=NO (use s3_uart_lcd_test after verifying display GPIO wiring)");
}
inline void handle(const String &cmd, Print &out) {
  if (cmd == "LCD STATUS") status(out);
  else out.println("LCD ERROR DISABLED; existing s3_uart_safe firmware is deliberately display-free");
}
#endif
}
