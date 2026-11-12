#pragma once
// M3rMa1d FT6336U capacitive touch diagnostic for LCDWiki/Hosyond MSP4031-style panels.
// The board manufacturer specifies FT6336U, I2C 7-bit address 0x38.
// Hardware I2C pins are disabled until a command arrives; camera stays OFF.
#include <Arduino.h>
#include "display_st7796.h"

#ifndef M3R_TOUCH_ENABLED
#define M3R_TOUCH_ENABLED 0
#endif
#ifndef CAMERA_PROFILE
#define CAMERA_PROFILE 0
#endif

#if M3R_TOUCH_ENABLED
#if CAMERA_PROFILE != 0
#error "FT6336U touch diagnostic may not be combined with active unverified camera pins"
#endif
#if !M3R_LCD_ENABLED
#error "Touch diagnostic requires LCD-enabled environment"
#endif
#ifndef M3R_TOUCH_PIN_SDA
#error "Touch SDA GPIO is required"
#endif
#ifndef M3R_TOUCH_PIN_SCL
#error "Touch SCL GPIO is required"
#endif
#ifndef M3R_TOUCH_PIN_RST
#error "Touch RST GPIO is required"
#endif
#define M3R_BAD_TOUCH_GPIO(p) ((p)==0 || (p)==3 || (p)==19 || (p)==20 || (p)==35 || (p)==36 || (p)==37 || (p)==43 || (p)==44 || (p)==45 || (p)==46)
#if M3R_BAD_TOUCH_GPIO(M3R_TOUCH_PIN_SDA) || M3R_BAD_TOUCH_GPIO(M3R_TOUCH_PIN_SCL) || M3R_BAD_TOUCH_GPIO(M3R_TOUCH_PIN_RST)
#error "Unsafe touch pin selection (strapping, USB, UART, PSRAM or flash)"
#endif
#if (M3R_TOUCH_PIN_SDA == M3R_TOUCH_PIN_SCL) || (M3R_TOUCH_PIN_SDA == M3R_TOUCH_PIN_RST) || (M3R_TOUCH_PIN_SCL == M3R_TOUCH_PIN_RST)
#error "Touch pins must be distinct"
#endif
#if (M3R_TOUCH_PIN_SDA == M3R_LCD_PIN_CS) || (M3R_TOUCH_PIN_SDA == M3R_LCD_PIN_RST) || (M3R_TOUCH_PIN_SDA == M3R_LCD_PIN_DC) || (M3R_TOUCH_PIN_SDA == M3R_LCD_PIN_MOSI) || (M3R_TOUCH_PIN_SDA == M3R_LCD_PIN_SCK) || (M3R_TOUCH_PIN_SCL == M3R_LCD_PIN_CS) || (M3R_TOUCH_PIN_SCL == M3R_LCD_PIN_RST) || (M3R_TOUCH_PIN_SCL == M3R_LCD_PIN_DC) || (M3R_TOUCH_PIN_SCL == M3R_LCD_PIN_MOSI) || (M3R_TOUCH_PIN_SCL == M3R_LCD_PIN_SCK) || (M3R_TOUCH_PIN_RST == M3R_LCD_PIN_CS) || (M3R_TOUCH_PIN_RST == M3R_LCD_PIN_RST) || (M3R_TOUCH_PIN_RST == M3R_LCD_PIN_DC) || (M3R_TOUCH_PIN_RST == M3R_LCD_PIN_MOSI) || (M3R_TOUCH_PIN_RST == M3R_LCD_PIN_SCK)
#error "Touch GPIOs must not overlap the proven working LCD SPI GPIOs"
#endif
#include <Wire.h>
#endif

namespace mermaid_touch {
#if M3R_TOUCH_ENABLED
constexpr uint8_t ADDRESS = 0x38;
constexpr uint16_t MAX_X = 320;
constexpr uint16_t MAX_Y = 480;
constexpr uint32_t POLL_MS = 35;
constexpr uint32_t TEST_MS = 30000;
static bool busStarted = false;
static bool controllerReady = false;
static bool testActive = false;
static uint8_t rotation = 0;  // 0 clockwise, 1 counterclockwise, 2 mirrored clockwise, 3 mirrored counterclockwise
static uint32_t testStarted = 0;
static uint32_t lastPolled = 0;
static uint32_t lastReported = 0;
static int lastX = -100;
static int lastY = -100;
static bool wasPressed = false;

struct Point { bool down; uint8_t count; uint16_t rawX; uint16_t rawY; uint16_t screenX; uint16_t screenY; };

inline void startBus() {
  if (busStarted) return;
  Wire.begin(M3R_TOUCH_PIN_SDA, M3R_TOUCH_PIN_SCL, 100000);
  Wire.setTimeOut(50);
  busStarted = true;
}
inline void diagnoseBus(Print &out) {
  if (busStarted) {
    Wire.end();
    busStarted = false;
  }

  // Inspect idle levels before activating I2C. HIGH does not prove continuity.
  pinMode(M3R_TOUCH_PIN_SDA, INPUT);
  pinMode(M3R_TOUCH_PIN_SCL, INPUT);
  delay(2);
  const int sda = digitalRead(M3R_TOUCH_PIN_SDA);
  const int scl = digitalRead(M3R_TOUCH_PIN_SCL);
  out.printf("TOUCH DIAG idle_sda=%d idle_scl=%d\n", sda, scl);

  const bool started = Wire.begin(M3R_TOUCH_PIN_SDA, M3R_TOUCH_PIN_SCL, 100000);
  if (!started) {
    out.println("TOUCH DIAG ERROR I2C_BEGIN_FAILED");
    return;
  }
  Wire.setTimeOut(50);
  busStarted = true;

  Wire.beginTransmission(0x38);
  const uint8_t result38 = Wire.endTransmission(true);
  Wire.beginTransmission(0x48);
  const uint8_t result48 = Wire.endTransmission(true);
  out.printf("TOUCH DIAG addr38=%u ack38=%s addr48=%u ack48=%s rst=EXTERNAL irq=GPIO40_UNDRIVEN\n",
    result38, result38 == 0 ? "YES" : "NO", result48, result48 == 0 ? "YES" : "NO");
}
inline void diagnoseAlternatePins(Print &out) {
  constexpr int alternateSda = 1;
  constexpr int alternateScl = 2;
  Wire.end();
  busStarted = false;
  pinMode(alternateSda, INPUT_PULLUP);
  pinMode(alternateScl, INPUT_PULLUP);
  delay(2);
  out.printf("TOUCH ALT pins sda=1 scl=2 idle_sda=%s idle_scl=%s\n",
    digitalRead(alternateSda) ? "HIGH" : "LOW", digitalRead(alternateScl) ? "HIGH" : "LOW");
  Wire.begin(alternateSda, alternateScl, 100000);
  Wire.setTimeOut(50);
  Wire.beginTransmission(ADDRESS);
  const uint8_t result = Wire.endTransmission();
  out.printf("TOUCH ALT probe=0x38 result=%u ack=%s\n", result, result == 0 ? "YES" : "NO");
  Wire.end();
  Wire.begin(M3R_TOUCH_PIN_SDA, M3R_TOUCH_PIN_SCL, 100000);
  Wire.setTimeOut(50);
  busStarted = true;
}
inline void legacyReset40Probe(Print &out) {
  // Explicit recovery only: reproduces the repository's former known-good
  // CTP_SDA=GPIO1, CTP_SCL=GPIO2, CTP_RST=GPIO40 sequence. SDMMC is disabled.
  Wire.end();
  busStarted = false;
  controllerReady = false;
  pinMode(40, OUTPUT);
  digitalWrite(40, LOW);
  delay(15);
  digitalWrite(40, HIGH);
  delay(250);
  const bool started = Wire.begin(1, 2, 100000);
  if (!started) {
    pinMode(40, INPUT);
    out.println("TOUCH LEGACY ERROR I2C_BEGIN_FAILED");
    return;
  }
  Wire.setTimeOut(50);
  busStarted = true;
  Wire.beginTransmission(ADDRESS);
  const uint8_t result = Wire.endTransmission(true);
  controllerReady = result == 0;
  out.printf("TOUCH LEGACY reset=GPIO40 sda=1 scl=2 addr=0x38 result=%u ack=%s\n",
    result, controllerReady ? "YES" : "NO");
  if (!controllerReady) {
    Wire.end();
    busStarted = false;
    pinMode(40, INPUT);
    startBus();
  }
}
inline bool probe() {
  startBus();
  Wire.beginTransmission(ADDRESS);
  return Wire.endTransmission() == 0;
}
inline bool readRegisters(uint8_t address, uint8_t *result, size_t count) {
  if (!busStarted || !result || count == 0 || count > 16) return false;
  Wire.beginTransmission(ADDRESS);
  Wire.write(address);
  if (Wire.endTransmission(false) != 0) return false;
  const uint8_t received = Wire.requestFrom(ADDRESS, static_cast<uint8_t>(count));
  if (received != count) {
    while (Wire.available()) Wire.read();
    return false;
  }
  for (size_t i = 0; i < count; ++i) {
    int byteRead = Wire.read();
    if (byteRead < 0) return false;
    result[i] = static_cast<uint8_t>(byteRead);
  }
  return true;
}
inline void mapPoint(uint16_t x, uint16_t y, Point &point) {
  // FT6336U native portrait sensor coordinates (320x480), LCD initialized as landscape (480x320).
  switch (rotation) {
    case 0: point.screenX = y;             point.screenY = MAX_X - 1 - x; break;
    case 1: point.screenX = MAX_Y - 1 - y; point.screenY = x; break;
    case 2: point.screenX = y;             point.screenY = x; break;
    default:point.screenX = MAX_Y - 1 - y; point.screenY = MAX_X - 1 - x; break;
  }
}
inline bool sample(Point &point) {
  point = {};
  if (!controllerReady) return false;
  // 0x02 TD_STATUS, 0x03-0x06 first touch: XH XL YH YL.
  uint8_t registerBytes[5] = {};
  if (!readRegisters(0x02, registerBytes, sizeof(registerBytes))) return false;
  point.count = registerBytes[0] & 0x0F;
  if (point.count > 2) return false;
  if (point.count == 0) return true;
  const uint8_t event = registerBytes[1] >> 6;
  if (event == 1 || event == 3) return true; // finger up / reserved
  point.rawX = uint16_t((registerBytes[1] & 0x0F) << 8) | registerBytes[2];
  point.rawY = uint16_t((registerBytes[3] & 0x0F) << 8) | registerBytes[4];
  if (point.rawX >= MAX_X || point.rawY >= MAX_Y) return false;
  point.down = true;
  mapPoint(point.rawX,point.rawY,point);
  return true;
}
inline void status(Print &out) {
  out.printf("TOUCH STATUS driver=FT6336U i2c=0x38 sda=%d scl=%d rst=%d irq=UNWIRED bus=%s ready=%s test=%s rotation=%u camera=OFF\n",
    M3R_TOUCH_PIN_SDA, M3R_TOUCH_PIN_SCL, M3R_TOUCH_PIN_RST,
    busStarted ? "YES" : "NO",controllerReady ? "YES" : "NO",testActive ? "ON" : "OFF",rotation);
}
inline void init(Print &out) {
  testActive = false;
  startBus();
  // RST=-1 requires an external pull-up to 3.3V.
  // Never drive GPIO40 in this profile; it is SDMMC D0.
#if M3R_TOUCH_PIN_RST >= 0
  pinMode(M3R_TOUCH_PIN_RST, OUTPUT);
  digitalWrite(M3R_TOUCH_PIN_RST, LOW);
  delay(15);
  digitalWrite(M3R_TOUCH_PIN_RST, HIGH);
  delay(250);
#else
  delay(100);
#endif
  controllerReady = probe();
  if (!controllerReady) {
    out.println("TOUCH INIT FAIL no_i2c_ack=0x38 check GND/SCL/SDA/RST and board pin routing");
    return;
  }
  uint8_t vendor = 0, chip = 0;
  const bool vendorOk = readRegisters(0xA8,&vendor,1);
  const bool chipOk = readRegisters(0xA3,&chip,1);
  out.printf("TOUCH INIT ACK=YES address=0x38 vendor=%s chip=%s ",vendorOk ? "READ" : "UNREADABLE",chipOk ? "READ" : "UNREADABLE");
  if (vendorOk) out.printf("vendor_id=0x%02X ",vendor);
  if (chipOk) out.printf("chip_id=0x%02X ",chip);
  out.println("(touch events still require verification)");
}
inline void report(Print &out, const Point &point) {
  if (!point.down) {out.println("TOUCH READ pressed=NO"); return;}
  out.printf("TOUCH EVENT pressed=YES count=%u raw_x=%u raw_y=%u x=%u y=%u rotate=%u\n",
    point.count,point.rawX,point.rawY,point.screenX,point.screenY,rotation);
}
inline void handle(const String &cmd, Print &out) {
  if (cmd == "TOUCH STATUS") {status(out);return;}
  if (cmd == "TOUCH DIAG") {diagnoseBus(out);return;}
  if (cmd == "TOUCH ALT PROBE") {diagnoseAlternatePins(out);return;}
  if (cmd == "TOUCH LEGACY RESET40 CONFIRM") {legacyReset40Probe(out);return;}
  if (cmd == "TOUCH PROBE") {
    out.println(probe() ? "TOUCH PROBE found=0x38 (ack only)" : "TOUCH PROBE missing=0x38 check SDA/SCL/VCC/GND");return;
  }
  if (cmd == "TOUCH INIT") {init(out);return;}
  if (cmd == "TOUCH STOP") {testActive=false;out.println("TOUCH TEST STOPPED");return;}
  if (cmd.startsWith("TOUCH ROTATE ")) {
    if (cmd.length() != 14 || cmd[13] < '0' || cmd[13] > '3') {
      out.println("TOUCH ERROR ROTATION use TOUCH ROTATE 0/1/2/3");return;
    }
    rotation=static_cast<uint8_t>(cmd[13]-'0');
    out.printf("TOUCH ROTATE %u updated; display rotation unchanged\n",rotation);
    return;
  }
  if (cmd == "TOUCH READ") {
    Point point;
    if (!controllerReady) {out.println("TOUCH ERROR NOT_INITIALIZED; use TOUCH INIT");return;}
    if (!sample(point)) {out.println("TOUCH READ ERROR i2c_or_bad_coords");return;}
    report(out,point);
    return;
  }
  if (cmd == "TOUCH TEST") {
    if (!controllerReady) {out.println("TOUCH ERROR NOT_INITIALIZED; use TOUCH INIT");return;}
    if (!mermaid_lcd::initialized) {out.println("TOUCH ERROR LCD_NOT_INITIALIZED; use LCD INIT");return;}
    mermaid_lcd::rect(0,0,mermaid_lcd::WIDTH,mermaid_lcd::HEIGHT,0x0000);
    // Four colored corner targets, then white dot at touch position.
    mermaid_lcd::rect(4,4,22,22,0xF800);
    mermaid_lcd::rect(mermaid_lcd::WIDTH-26,4,22,22,0x07E0);
    mermaid_lcd::rect(4,mermaid_lcd::HEIGHT-26,22,22,0x001F);
    mermaid_lcd::rect(mermaid_lcd::WIDTH-26,mermaid_lcd::HEIGHT-26,22,22,0xFFE0);
    wasPressed=false; lastX=-100;lastY=-100;lastReported=0;lastPolled=0;
    testStarted=millis();testActive=true;
    out.println("TOUCH TEST START 30s. Tap display: white dots should track taps; UART prints raw+mapped coordinates. TOUCH STOP to end.");
    return;
  }
  out.println("TOUCH ERROR UNKNOWN; use TOUCH STATUS/PROBE/INIT/READ/TEST/STOP/ROTATE N");
}
inline void service(Print &out) {
  if (!testActive || !controllerReady) return;
  const uint32_t now=millis();
  if (now-testStarted>=TEST_MS) {testActive=false;out.println("TOUCH TEST DONE timeout=30s");return;}
  if (now-lastPolled<POLL_MS) return;
  lastPolled=now;
  Point point;
  if (!sample(point)) {out.println("TOUCH TEST WARN i2c_read_failed_or_invalid_data");testActive=false;return;}
  if (!point.down) {wasPressed=false;return;}
  const bool moved = abs(int(point.screenX)-lastX)>=6 || abs(int(point.screenY)-lastY)>=6;
  if (!wasPressed || moved || now-lastReported>=500) {
    report(out,point);
    mermaid_lcd::rect(point.screenX>4 ? point.screenX-4 : 0,point.screenY>4 ? point.screenY-4 : 0,9,9,0xFFFF);
    lastReported=now;lastX=point.screenX;lastY=point.screenY;
  }
  wasPressed=true;
}
#else
inline void status(Print &out) {out.println("TOUCH STATUS enabled=NO (build s3_uart_touch_test)");}
inline void handle(const String &, Print &out) {out.println("TOUCH ERROR DISABLED");}
inline void service(Print &) {}
#endif
}
