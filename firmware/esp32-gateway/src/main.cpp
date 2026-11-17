#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "eig_hardware_config.h"

HardwareSerial flipper(EIG_FLIPPER_UART_INSTANCE);
WebServer server(80);

static void send_json(int status, const String& body) {
  server.send(status, "application/json", body + "\n");
}

static void handle_health() {
  String out = "{\"ok\":true,\"version\":\"" EIG_VERSION "\",\"chip\":\"";
  out += ESP.getChipModel();
  out += "\",\"flash_bytes\":" + String(ESP.getFlashChipSize());
  out += ",\"psram_bytes\":" + String(ESP.getPsramSize()) + "}";
  send_json(200, out);
}

static void handle_ping() {
  if (server.method() != HTTP_POST) return send_json(405, "{\"error\":\"method\"}");
  send_json(200, "{\"version\":1,\"ok\":true,\"result\":{\"pong\":true}}");
}

static void handle_uart() {
  if (server.method() != HTTP_POST || !server.hasArg("plain")) return send_json(400, "{\"error\":\"body\"}");
  String frame = server.arg("plain");
  frame.trim();
  flipper.println(frame);
  send_json(200, "{\"version\":1,\"ok\":true,\"result\":{\"forwarded\":true}}");
}

void setup() {
  Serial.begin(115200);
  flipper.begin(EIG_FLIPPER_BAUD, SERIAL_8N1, EIG_FLIPPER_RX_GPIO, EIG_FLIPPER_TX_GPIO);
  WiFi.mode(WIFI_AP);
  WiFi.softAP("EIG-Gateway", nullptr);
  server.on("/health", HTTP_GET, handle_health);
  server.on("/v1/ping", HTTP_POST, handle_ping);
  server.on("/v1/uart", HTTP_POST, handle_uart);
  server.begin();
  Serial.printf("EIG_READY version=%s ip=%s flash=%u psram=%u\n", EIG_VERSION,
                WiFi.softAPIP().toString().c_str(), ESP.getFlashChipSize(), ESP.getPsramSize());
}

void loop() {
  server.handleClient();
  while (flipper.available()) Serial.write(flipper.read());
  delay(1);
}

