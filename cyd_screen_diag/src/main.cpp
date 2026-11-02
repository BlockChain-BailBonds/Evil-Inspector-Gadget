#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>

static constexpr int BacklightPin = 27;
static TFT_eSPI display;
static const uint16_t colors[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_WHITE};
static const char *labels[] = {"RED", "GREEN", "BLUE", "WHITE"};
static uint8_t phase = 0;
static uint32_t nextChange = 0;

static void showPhase() {
  const uint8_t i = phase % 4;
  const bool backlightHigh = (phase % 2) == 0;
  digitalWrite(BacklightPin, backlightHigh ? HIGH : LOW);
  display.fillScreen(colors[i]);
  display.setTextDatum(MC_DATUM);
  display.setTextColor(i == 3 ? TFT_BLACK : TFT_WHITE, colors[i]);
  display.drawString("918 BACKLIGHT TEST", display.width() / 2, display.height() / 2 - 15, 2);
  display.drawString(labels[i], display.width() / 2, display.height() / 2 + 15, 4);
  Serial.printf("DIAG SCREEN phase=%u color=%s width=%d height=%d backlight_pin=27 level=%s millis=%lu\n",
                phase, labels[i], display.width(), display.height(), backlightHigh ? "HIGH" : "LOW",
                static_cast<unsigned long>(millis()));
  phase++;
}

void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println("DIAG START target=classic-esp32 panel=ST7796S expected=320x480");
  pinMode(BacklightPin, OUTPUT);
  digitalWrite(BacklightPin, HIGH);
  Serial.println("DIAG BACKLIGHT pin=27 HIGH/LOW cycle enabled");
  display.init();
  Serial.println("DIAG TFT_INIT returned");
  display.setRotation(1);
  Serial.printf("DIAG TFT_GEOMETRY width=%d height=%d\n", display.width(), display.height());
  showPhase();
  nextChange = millis() + 5000;
}

void loop() {
  if (static_cast<int32_t>(millis() - nextChange) >= 0) {
    nextChange = millis() + 5000;
    showPhase();
  }
  delay(10);
}
