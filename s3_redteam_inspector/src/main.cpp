// M3rMa1d S3-CAM Red-Team v1.0: local USB-controlled defensive reconnaissance.
// No wifi association, network listener, active BLE probing, authentication attempts,
// packet injection, wireless disruption, or exploit delivery.
// Camera is triggered only by an explicit USB command; no background recording.
#include <Arduino.h>
#include <HardwareSerial.h>
#include <cstring>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <SPIFFS.h>
#include <esp_camera.h>
#include "risk_rules.h"
#include "camera_pins.h"
#include "display_st7796.h"
#include "touch_ft6336.h"
#if defined(M3R_DRAGON_ENABLED) && M3R_DRAGON_ENABLED
#include "inspector_ui.h"
#endif

namespace {
constexpr char VERSION[] = "1.2.0-evil-inspector-gadget-uart1-full-duplex";
constexpr char LOG_FILE[] = "/evidence.jsonl";
constexpr size_t WIFI_LIMIT = 48;
constexpr size_t BLE_LIMIT = 48;
constexpr size_t LINE_LIMIT = 112;
constexpr size_t LOG_MAX = 512 * 1024;
constexpr size_t JPEG_CHUNK = 768;
constexpr size_t JPEG_MAX = 500000;

struct WifiItem {
  String ssid;
  String bssid;
  int32_t rssi = -127;
  uint8_t channel = 0;
  uint8_t auth = 255;
  mermaid::Security security = mermaid::Security::Unknown;
};
struct BleItem {
  String mac;
  String name;
  int32_t rssi = -127;
};
WifiItem wifiList[WIFI_LIMIT];
BleItem bleList[BLE_LIMIT];
uint8_t wifiSaved = 0;
uint8_t bleSaved = 0;
int wifiSeen = 0;
int bleSeen = 0;
uint32_t wifiAt = 0;
uint32_t bleAt = 0;
uint8_t cameraSize = 0;
bool cameraReady = false;
bool bleReady = false;
bool fsReady = false;
bool loggingEnabled = false;
char inputLine[LINE_LIMIT];
size_t lineLength = 0;
// Operator-verified UART1 physical pins. GPIO3 remains a boot strap input.
constexpr int FLIPPER_RX_PIN = 3;
constexpr int FLIPPER_TX_PIN = 7;
constexpr uint32_t FLIPPER_BAUD = 115200;
HardwareSerial flipperSerial(1);
char flipperInput[64] = {};
size_t flipperLength = 0;
bool flipperDiscardLine = false;
uint32_t flipperCommandsReceived = 0;
uint32_t flipperCommandsRejected = 0;

String jsonString(const String &value) {
  static const char HEX_DIGITS[] = "0123456789ABCDEF";
  String result;
  result.reserve(value.length() + 12);
  result += '"';
  for (size_t i = 0; i < value.length(); ++i) {
    const uint8_t c = static_cast<uint8_t>(value[i]);
    if (c == '"' || c == '\\') {
      result += '\\';
      result += static_cast<char>(c);
    } else if (c < 32 || c >= 127) {
      result += "\\u00";
      result += HEX_DIGITS[c >> 4];
      result += HEX_DIGITS[c & 15];
    } else {
      result += static_cast<char>(c);
    }
  }
  result += '"';
  return result;
}

const char *authText(uint8_t auth) {
  switch (auth) {
    case WIFI_AUTH_OPEN: return "OPEN";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA1";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA1/2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-E";
    default: return "OTHER";
  }
}
mermaid::Security classifyAuth(uint8_t auth) {
  switch (auth) {
    case WIFI_AUTH_OPEN: return mermaid::Security::Open;
    case WIFI_AUTH_WEP: return mermaid::Security::Wep;
    case WIFI_AUTH_WPA_PSK: return mermaid::Security::Wpa1;
    case WIFI_AUTH_WPA_WPA2_PSK: return mermaid::Security::MixedWpa;
    case WIFI_AUTH_WPA2_PSK: return mermaid::Security::Wpa2;
    case WIFI_AUTH_WPA2_ENTERPRISE: return mermaid::Security::Wpa2Enterprise;
    default: return mermaid::Security::Unknown;
  }
}

void emitWifi(Print &out, const WifiItem &w) {
  out.print("{\"type\":\"wifi\",\"uptime_ms\":"); out.print(wifiAt);
  out.print(",\"ssid\":"); out.print(jsonString(w.ssid));
  out.print(",\"bssid\":"); out.print(jsonString(w.bssid));
  out.print(",\"rssi_dbm\":"); out.print(w.rssi);
  out.print(",\"channel\":"); out.print(w.channel);
  out.print(",\"auth\":"); out.print(jsonString(authText(w.auth)));
  out.print(",\"severity\":"); out.print(jsonString(mermaid::severityName(mermaid::severity(w.security))));
  out.print(",\"indicator\":"); out.print(jsonString(mermaid::indicator(w.security)));
  out.print(",\"description\":"); out.print(jsonString(mermaid::explanation(w.security)));
  out.println(",\"exploitable_confirmed\":false}");
}
void emitBle(Print &out, const BleItem &b) {
  out.print("{\"type\":\"ble\",\"uptime_ms\":"); out.print(bleAt);
  out.print(",\"address\":"); out.print(jsonString(b.mac));
  out.print(",\"name\":"); out.print(jsonString(b.name));
  out.print(",\"rssi_dbm\":"); out.print(b.rssi);
  out.println(",\"vulnerability_status\":\"UNDETERMINED\"}");
}
void writeEvidence() {
  if (!loggingEnabled || !fsReady) return;
  File file = SPIFFS.open(LOG_FILE, FILE_APPEND);
  if (!file) { Serial.println("LOG ERROR OPEN"); return; }
  if (file.size() > LOG_MAX) {
    Serial.println("LOG FULL (disable logging or clear after exporting)");
    file.close();
    return;
  }
  file.print("{\"type\":\"scan\",\"uptime_ms\":"); file.print(millis());
  file.print(",\"wifi_seen\":"); file.print(wifiSeen);
  file.print(",\"ble_seen\":"); file.print(bleSeen);
  file.println(",\"source\":\"passive_radio\"}");
  for (size_t i = 0; i < wifiSaved; ++i) emitWifi(file, wifiList[i]);
  for (size_t i = 0; i < bleSaved; ++i) emitBle(file, bleList[i]);
  file.flush();
  file.close();
}
void status() {
  Serial.printf("M3rMa1d S3-CAM %s USB-only\n", VERSION);
  Serial.printf("STATUS chip=%s flash=%u psram=%u free_heap=%u uptime_ms=%lu\n",
                ESP.getChipModel(), (unsigned) ESP.getFlashChipSize(),
                (unsigned) ESP.getPsramSize(), (unsigned) ESP.getFreeHeap(),
                (unsigned long) millis());
  Serial.printf("STATUS camera_profile=%d camera_ready=%s camera_size=%s fs=%s log=%s\n",
                CAMERA_PROFILE, cameraReady ? "YES" : "NO", cameraSize ? "VGA" : "QVGA",
                fsReady ? "MOUNTED" : "UNAVAILABLE", loggingEnabled ? "ON" : "OFF");
  Serial.printf("STATUS wifi_seen=%d wifi_saved=%u ble_seen=%d ble_saved=%u\n",
                wifiSeen, wifiSaved, bleSeen, bleSaved);
  Serial.println("STATUS ASSOCIATION=DISABLED PASSIVE_RADIO=YES REMOTE_LISTENERS=NO EXPLOITS=NO");
  mermaid_lcd::status(Serial);
  mermaid_touch::status(Serial);
}
void scanWifi() {
  Serial.println("SCAN WIFI START passive=true no_association=true");
  // Never call WiFi.begin. A passive scan listens for beacon frames only.
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false, false);
  const int16_t n = WiFi.scanNetworks(false, true, true, 180, 0);
  wifiSaved = 0;
  wifiSeen = n > 0 ? n : 0;
  wifiAt = millis();
  if (n >= 0) {
    const int count = n < (int) WIFI_LIMIT ? n : (int) WIFI_LIMIT;
    for (int i = 0; i < count; ++i) {
      WifiItem &w = wifiList[wifiSaved++];
      w.ssid = WiFi.SSID(i);
      w.bssid = WiFi.BSSIDstr(i);
      w.rssi = WiFi.RSSI(i);
      w.channel = static_cast<uint8_t>(WiFi.channel(i));
      w.auth = static_cast<uint8_t>(WiFi.encryptionType(i));
      w.security = classifyAuth(w.auth);
    }
  }
  WiFi.scanDelete();
  if (n < 0) {
    Serial.printf("SCAN WIFI ERROR code=%d\n", n);
  } else {
    Serial.printf("SCAN WIFI DONE seen=%d saved=%u\n", wifiSeen, wifiSaved);
  }
  for (uint8_t i = 0; i < wifiSaved; ++i) emitWifi(Serial, wifiList[i]);
  writeEvidence();
}
void scanBle() {
  Serial.println("SCAN BLE START passive=true active_requests=false");
  if (!bleReady) {
    BLEDevice::init("");
    bleReady = true;
  }
  BLEScan *scanner = BLEDevice::getScan();
  scanner->setActiveScan(false);
  scanner->setInterval(160);
  scanner->setWindow(80);
  BLEScanResults results = scanner->start(5, false);
  bleSeen = results.getCount();
  bleSaved = 0;
  bleAt = millis();
  const int count = bleSeen < (int) BLE_LIMIT ? bleSeen : (int) BLE_LIMIT;
  for (int i = 0; i < count; ++i) {
    BLEAdvertisedDevice device = results.getDevice(i);
    BleItem &b = bleList[bleSaved++];
    b.mac = String(device.getAddress().toString().c_str());
    b.name = device.haveName() ? String(device.getName().c_str()) : String("");
    b.rssi = device.getRSSI();
  }
  scanner->clearResults();
  Serial.printf("SCAN BLE DONE seen=%d saved=%u\n", bleSeen, bleSaved);
  for (uint8_t i = 0; i < bleSaved; ++i) emitBle(Serial, bleList[i]);
  Serial.println("NOTICE BLE advertisements cannot confirm device CVEs or exploitability");
  writeEvidence();
}
void findings() {
  int count = 0;
  for (uint8_t i = 0; i < wifiSaved; ++i) {
    const WifiItem &w = wifiList[i];
    if (mermaid::severity(w.security) != mermaid::Severity::None) {
      emitWifi(Serial, w);
      ++count;
    }
  }
  // Same SSID under different auth can be legitimate; report as review only.
  for (uint8_t i = 0; i < wifiSaved; ++i) {
    if (wifiList[i].ssid.isEmpty()) continue;
    for (uint8_t j = i + 1; j < wifiSaved; ++j) {
      if (wifiList[i].ssid != wifiList[j].ssid || wifiList[i].auth == wifiList[j].auth) continue;
      Serial.print("{\"type\":\"review\",\"indicator\":\"SAME_SSID_DIFFERENT_AUTH\",\"ssid\":");
      Serial.print(jsonString(wifiList[i].ssid));
      Serial.print(",\"bssid_a\":"); Serial.print(jsonString(wifiList[i].bssid));
      Serial.print(",\"bssid_b\":"); Serial.print(jsonString(wifiList[j].bssid));
      Serial.println(",\"severity\":\"REVIEW\",\"evil_twin_confirmed\":false}");
      ++count;
    }
  }
  Serial.printf("FINDINGS DONE count=%d (indicators only, no exploit confirmation)\n", count);
}
void exportAll() {
  Serial.print("{\"type\":\"inventory\",\"version\":"); Serial.print(jsonString(VERSION));
  Serial.printf(",\"wifi_seen\":%d,\"ble_seen\":%d,\"uptime_ms\":%lu}\n",
                wifiSeen, bleSeen, (unsigned long) millis());
  for (uint8_t i = 0; i < wifiSaved; ++i) emitWifi(Serial, wifiList[i]);
  for (uint8_t i = 0; i < bleSaved; ++i) emitBle(Serial, bleList[i]);
  findings();
  Serial.println("EXPORT END");
}

void initCamera() {
#if CAMERA_PROFILE == 0
  Serial.println("CAMERA DISABLED: safe GPIO profile (requires board-specific pin map)");
#else
  if (!psramFound()) {
    Serial.println("CAMERA ERROR PSRAM_NOT_FOUND (USB and radio commands remain available)");
    return;
  }
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = mermaid_cam::d0;
  config.pin_d1 = mermaid_cam::d1;
  config.pin_d2 = mermaid_cam::d2;
  config.pin_d3 = mermaid_cam::d3;
  config.pin_d4 = mermaid_cam::d4;
  config.pin_d5 = mermaid_cam::d5;
  config.pin_d6 = mermaid_cam::d6;
  config.pin_d7 = mermaid_cam::d7;
  config.pin_xclk = mermaid_cam::xclk;
  config.pin_pclk = mermaid_cam::pclk;
  config.pin_vsync = mermaid_cam::vsync;
  config.pin_href = mermaid_cam::href;
  config.pin_sscb_sda = mermaid_cam::sda;
  config.pin_sscb_scl = mermaid_cam::scl;
  config.pin_pwdn = mermaid_cam::pwdn;
  config.pin_reset = mermaid_cam::reset;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 14;
  config.fb_count = 1;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  const esp_err_t error = esp_camera_init(&config);
  cameraReady = error == ESP_OK;
  Serial.printf("CAMERA INIT %s esp_err=0x%X profile=%d\n", cameraReady ? "OK" : "FAIL",
                (unsigned) error, CAMERA_PROFILE);
#endif
}
uint32_t crc32(const uint8_t *buffer, size_t size) {
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < size; ++i) {
    crc ^= buffer[i];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      const uint32_t mask = 0u - (crc & 1u);
      crc = (crc >> 1) ^ (0xEDB88320u & mask);
    }
  }
  return crc ^ 0xFFFFFFFFu;
}
String encodeBase64(const uint8_t *source, size_t length) {
  static constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  String out;
  out.reserve(((length + 2) / 3) * 4 + 1);
  for (size_t i = 0; i < length; i += 3) {
    const bool second = i + 1 < length;
    const bool third = i + 2 < length;
    uint32_t triple = uint32_t(source[i]) << 16;
    if (second) triple |= uint32_t(source[i + 1]) << 8;
    if (third) triple |= uint32_t(source[i + 2]);
    out += alphabet[(triple >> 18) & 63];
    out += alphabet[(triple >> 12) & 63];
    out += second ? alphabet[(triple >> 6) & 63] : '=';
    out += third ? alphabet[triple & 63] : '=';
  }
  return out;
}
void capture() {
  if (!cameraReady) { Serial.println("CAM ERROR NOT_INITIALIZED"); return; }
  camera_fb_t *frame = esp_camera_fb_get();
  if (!frame) { Serial.println("CAM ERROR CAPTURE_FAILED"); return; }
  if (frame->format != PIXFORMAT_JPEG || frame->len < 4 || frame->len > JPEG_MAX) {
    Serial.println("CAM ERROR BAD_FRAME_OR_OVERSIZE");
    esp_camera_fb_return(frame);
    return;
  }
  const uint32_t checksum = crc32(frame->buf, frame->len);
  Serial.printf("CAM_BEGIN bytes=%u crc32=%08lX format=jpeg\n",
                (unsigned) frame->len, (unsigned long) checksum);
  for (size_t i = 0; i < frame->len; i += JPEG_CHUNK) {
    const size_t left = frame->len - i;
    const size_t length = left < JPEG_CHUNK ? left : JPEG_CHUNK;
    Serial.print("CAM_DATA ");
    Serial.println(encodeBase64(frame->buf + i, length));
    Serial.flush();
    delay(1); // UART pacing; protects low-baud Samsung USB bridge sessions.
  }
  Serial.printf("CAM_END bytes=%u crc32=%08lX\n", (unsigned) frame->len, (unsigned long) checksum);
  Serial.flush();
  esp_camera_fb_return(frame);
}
void setFrameSize(uint8_t target) {
  if (!cameraReady) { Serial.println("CAM ERROR NOT_INITIALIZED"); return; }
  sensor_t *sensor = esp_camera_sensor_get();
  if (!sensor) { Serial.println("CAM ERROR SENSOR_UNAVAILABLE"); return; }
  const framesize_t size = target ? FRAMESIZE_VGA : FRAMESIZE_QVGA;
  if (sensor->set_framesize(sensor, size) != 0) {
    Serial.println("CAM ERROR SET_SIZE");
    return;
  }
  cameraSize = target;
  Serial.printf("CAM SIZE %s\n", target ? "VGA" : "QVGA");
}
void printHelp() {
  Serial.println("HELP | STATUS | SCAN WIFI | SCAN BLE | SCAN ALL | FINDINGS | EXPORT | LINK STATUS");
#if defined(M3R_DRAGON_ENABLED) && M3R_DRAGON_ENABLED
  Serial.println("INSPECTOR STATUS | INSPECTOR START | INSPECTOR HOME | INSPECTOR REDRAW | INSPECTOR OFF");
  Serial.println("INSPECTOR WIFI | INSPECTOR BLE | INSPECTOR CAMERA | INSPECTOR LOGS | INSPECTOR SETTINGS | INSPECTOR SYSTEM | INSPECTOR EXPORT | INSPECTOR TOUCH FLIP");
#endif
  Serial.println("CAM STATUS | CAM CAPTURE | CAM SIZE QVGA | CAM SIZE VGA");
  Serial.println("LCD STATUS | LCD INIT | LCD TEST | LCD RED | LCD GREEN | LCD BLUE | LCD WHITE | LCD BLACK | LCD OFF");
  Serial.println("TOUCH STATUS | TOUCH DIAG | TOUCH ALT PROBE | TOUCH LEGACY RESET40 CONFIRM | TOUCH PROBE | TOUCH INIT | TOUCH READ | TOUCH TEST | TOUCH STOP | TOUCH ROTATE 0/1/2/3");
  Serial.println("LOG STATUS | LOG ON | LOG OFF | LOG DUMP | LOG CLEAR CONFIRM | LOG FORMAT CONFIRM");
  Serial.println("All cameras require explicit USB command. No Wi-Fi join, server, injection, or exploit payload.");
}
void logCommand(const String &command) {
  if (command == "LOG STATUS") {
    Serial.printf("LOG STATUS mounted=%s enabled=%s max_bytes=%u\n", fsReady ? "YES" : "NO",
                  loggingEnabled ? "YES" : "NO", (unsigned) LOG_MAX);
  } else if (command == "LOG ON") {
    if (!fsReady) { Serial.println("LOG ERROR FS_NOT_MOUNTED; optional LOG FORMAT CONFIRM is destructive"); return; }
    loggingEnabled = true;
    Serial.println("LOG ON");
  } else if (command == "LOG OFF") {
    loggingEnabled = false;
    Serial.println("LOG OFF");
  } else if (command == "LOG DUMP") {
    if (!fsReady) { Serial.println("LOG ERROR FS_NOT_MOUNTED"); return; }
    File file = SPIFFS.open(LOG_FILE, FILE_READ);
    if (!file) { Serial.println("LOG EMPTY"); return; }
    Serial.println("LOG_BEGIN");
    while (file.available()) {
      const int data = file.read();
      if (data >= 0) Serial.write(static_cast<uint8_t>(data));
    }
    if (file.size() && file.peek() != '\n') Serial.println();
    file.close();
    Serial.println("LOG_END");
  } else if (command == "LOG CLEAR CONFIRM") {
    if (!fsReady) { Serial.println("LOG ERROR FS_NOT_MOUNTED"); return; }
    const bool deleted = SPIFFS.exists(LOG_FILE) ? SPIFFS.remove(LOG_FILE) : true;
    Serial.println(deleted ? "LOG CLEARED" : "LOG ERROR CLEAR_FAILED");
  } else if (command == "LOG FORMAT CONFIRM") {
    loggingEnabled = false;
    SPIFFS.end();
    const bool formatted = SPIFFS.format();
    fsReady = formatted && SPIFFS.begin(false);
    Serial.println(fsReady ? "LOG FS FORMATTED (prior stored evidence deleted)" : "LOG ERROR FORMAT_FAILED");
  } else {
    Serial.println("ERROR UNKNOWN LOG COMMAND");
  }
}
void handle(String command) {
  command.trim();
  command.toUpperCase();
  if (command.isEmpty()) return;
  if (command == "HELP") printHelp();
  else if (command == "STATUS") status();
  else if (command == "SCAN WIFI") scanWifi();
  else if (command == "SCAN BLE") scanBle();
  else if (command == "SCAN ALL") { scanWifi(); scanBle(); }
  else if (command == "FINDINGS") findings();
  else if (command == "EXPORT") exportAll();
  else if (command == "LINK STATUS") Serial.printf("LINK STATUS UART1_FULL_DUPLEX=YES S3_RX=%d S3_TX=%d BAUD=%lu CH340_UART0=UNTOUCHED RX_COMMANDS=%lu REJECTED=%lu GPIO3_STRAP=OPERATOR_VERIFIED GPIO7_WIRED=YES\n",
        FLIPPER_RX_PIN, FLIPPER_TX_PIN, (unsigned long)FLIPPER_BAUD,
        (unsigned long)flipperCommandsReceived, (unsigned long)flipperCommandsRejected);
  else if (command == "CAM STATUS") Serial.printf("CAM STATUS ready=%s profile=%d size=%s\n", cameraReady ? "YES" : "NO", CAMERA_PROFILE, cameraSize ? "VGA" : "QVGA");
  else if (command == "CAM CAPTURE") capture();
  else if (command == "CAM SIZE QVGA") setFrameSize(0);
  else if (command == "CAM SIZE VGA") setFrameSize(1);
#if defined(M3R_DRAGON_ENABLED) && M3R_DRAGON_ENABLED
  else if (command.startsWith("INSPECTOR ")) redteam_dragon::handle(command, Serial);
#endif
  else if (command.startsWith("LCD ")) mermaid_lcd::handle(command, Serial);
  else if (command.startsWith("TOUCH ")) mermaid_touch::handle(command, Serial);
  else if (command.startsWith("LOG ")) logCommand(command);
  else Serial.println("ERROR UNKNOWN COMMAND; send HELP");
}

// Independent, bounded UART1 command protocol (never use CH340 UART0 for Flipper RX).
// Accepted frames: EIG918|1|CMD|PING, STATUS, HOME, WIFI, BLE.
// Wireless scans are passive; no Flipper RF/IR action is proxied.
void flipperSendStatus() {
  flipperSerial.printf("EIG918|1|STAT|up=%lu|wifi=%u|ble=%u\n",
    (unsigned long)(millis() / 1000U), (unsigned)wifiSaved, (unsigned)bleSaved);
}
void flipperAck(const char* operation, const char* result) {
  flipperSerial.printf("EIG918|1|ACK|op=%s|result=%s\n", operation, result);
}
void flipperHandleLine(const char* line) {
  constexpr char prefix[] = "EIG918|1|CMD|";
  if (strncmp(line, prefix, sizeof(prefix) - 1) != 0) {
    ++flipperCommandsRejected;
    flipperSerial.println("EIG918|1|ERR|INVALID_FRAME");
    return;
  }
  const char* operation = line + sizeof(prefix) - 1;
  if (strcmp(operation, "PING") == 0) {
    ++flipperCommandsReceived;
    flipperAck("PING", "OK");
  } else if (strcmp(operation, "STATUS") == 0) {
    ++flipperCommandsReceived;
    flipperAck("STATUS", "OK");
    flipperSendStatus();
  } else if (strcmp(operation, "HOME") == 0) {
    ++flipperCommandsReceived;
#if defined(M3R_DRAGON_ENABLED) && M3R_DRAGON_ENABLED
    redteam_dragon::start();
    flipperAck("HOME", "OK");
#else
    flipperAck("HOME", "UNAVAILABLE");
#endif
  } else if (strcmp(operation, "WIFI") == 0) {
    ++flipperCommandsReceived;
    flipperAck("WIFI", "STARTED");
    scanWifi();
    flipperAck("WIFI", "DONE");
    flipperSendStatus();
  } else if (strcmp(operation, "BLE") == 0) {
    ++flipperCommandsReceived;
    flipperAck("BLE", "STARTED");
    scanBle();
    flipperAck("BLE", "DONE");
    flipperSendStatus();
  } else {
    ++flipperCommandsRejected;
    flipperSerial.println("EIG918|1|ERR|UNKNOWN_COMMAND");
  }
}
void serviceFlipperLink() {
  // Bounded work each tick, oversize or invalid lines rejected until newline.
  for (unsigned handled = 0; handled < 64 && flipperSerial.available(); ++handled) {
    const int value = flipperSerial.read();
    if (value < 0) break;
    const char c = static_cast<char>(value);
    if (c == '\r' || c == '\n') {
      if (!flipperDiscardLine && flipperLength != 0) {
        flipperInput[flipperLength] = '\0';
        flipperHandleLine(flipperInput);
      }
      flipperLength = 0;
      flipperDiscardLine = false;
      continue;
    }
    if (flipperDiscardLine) continue;
    if (c < 32 || c > 126 || flipperLength + 1 >= sizeof(flipperInput)) {
      flipperDiscardLine = true;
      flipperLength = 0;
      ++flipperCommandsRejected;
      continue;
    }
    flipperInput[flipperLength++] = c;
  }
}

}

#if defined(M3R_DRAGON_ENABLED) && M3R_DRAGON_ENABLED
// These bridge functions are deliberately outside the anonymous namespace,
// matching the global redteam_dragon namespace declared by dragon_ui.h.
// Original scanner functions/arrays retain internal linkage above.
namespace redteam_dragon {
void refreshWifi(){scanWifi();}
void refreshBle(){scanBle();}
void sendExport(){exportAll();}
int wifiResults(){return wifiSaved;}
int bleResults(){return bleSaved;}
String wifiLabel(unsigned index){
 if(index>=wifiSaved)return String("");
 return wifiList[index].ssid.length()?wifiList[index].ssid:String("(hidden SSID)");
}
String wifiDetail(unsigned index){
 if(index>=wifiSaved)return String("");
 const WifiItem &w=wifiList[index];
 return String(w.rssi)+" dBm CH "+String(w.channel)+" "+String(mermaid::severityName(mermaid::severity(w.security)));
}
String bleLabel(unsigned index){
 if(index>=bleSaved)return String("");
 const BleItem &b=bleList[index];
 return b.name.length()?b.name:b.mac;
}
String bleDetail(unsigned index){
 if(index>=bleSaved)return String("");
 const BleItem &b=bleList[index];return b.mac+" "+String(b.rssi)+" dBm";
}
bool filesystemReady(){return fsReady;}
bool loggingIsEnabled(){return loggingEnabled;}
}
#endif

void setup() {
  Serial.begin(115200);
  const uint32_t started = millis();
  while (!Serial && millis() - started < 1800) delay(10);
  Serial.println("918 TECHNOLOGIES EVIL INSPECTOR GADGET v1.2.0 FLIPPER UART1 FULL DUPLEX");
  flipperSerial.setRxBufferSize(256);
  flipperSerial.begin(FLIPPER_BAUD, SERIAL_8N1, FLIPPER_RX_PIN, FLIPPER_TX_PIN);
  Serial.printf("MEMORY flash=%u psram=%u\n", (unsigned) ESP.getFlashChipSize(),
                (unsigned) ESP.getPsramSize());
  fsReady = SPIFFS.begin(false);
  Serial.printf("LOG FS %s (automatic format disabled)\n", fsReady ? "MOUNTED" : "UNAVAILABLE");
  // Disable reconnection before any scan and never WiFi.begin/join.
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false, false);
  initCamera();
  status();
  printHelp();
#if defined(M3R_DRAGON_ENABLED) && M3R_DRAGON_ENABLED
  // Display SPI is hardware-verified. Touch failure is non-fatal inside start(),
  // so the dashboard is restored automatically after every power cycle.
  redteam_dragon::start();
#endif
  // Flipper UART1 is physically independent of the USB-UART0 CH340 bridge.
  flipperSerial.println("EIG918|1|HELLO|brand=918|app=INSPECTOR|mode=FULL_DUPLEX");
  Serial.println("FLIPPER LINK UART1 GPIO3=RX GPIO7=TX 115200 8N1");
}
void loop() {
  static uint32_t lastLinkFrame = 0;
  const uint32_t linkNow = millis();
  if ((uint32_t)(linkNow - lastLinkFrame) >= 2500U) {
    lastLinkFrame = linkNow;
    // UART1 fixed telemetry, no SSIDs or device addresses.
    flipperSendStatus();
  }
  serviceFlipperLink();
  while (Serial.available()) {
    const int read = Serial.read();
    if (read < 0) break;
    const char c = static_cast<char>(read);
    if (c == '\r' || c == '\n') {
      if (lineLength) {
        inputLine[lineLength] = '\0';
        handle(String(inputLine));
        lineLength = 0;
      }
    } else if (c >= 32 && c <= 126) {
      if (lineLength < LINE_LIMIT - 1) {
        inputLine[lineLength++] = c;
      } else {
        lineLength = 0;
        Serial.println("ERROR COMMAND TOO LONG");
      }
    }
  }
  mermaid_touch::service(Serial);
#if defined(M3R_DRAGON_ENABLED) && M3R_DRAGON_ENABLED
  redteam_dragon::service();
#endif
  delay(2);
}
