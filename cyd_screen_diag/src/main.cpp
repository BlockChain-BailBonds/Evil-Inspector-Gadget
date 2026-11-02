#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>

static constexpr int BacklightPin = 21;
static TFT_eSPI display;
static const uint16_t colors[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_WHITE, TFT_BLACK, TFT_YELLOW, TFT_CYAN, TFT_MAGENTA};
static const char *labels[] = {"RED", "GREEN", "BLUE", "WHITE", "BLACK", "YELLOW", "CYAN", "MAGENTA"};
static uint8_t phase = 0;
static uint32_t nextChange = 0;

static void showPhase() {
  const uint8_t i = phase % (sizeof(colors) / sizeof(colors[0]));
  display.fillScreen(colors[i]);
  display.setTextDatum(MC_DATUM);
  display.setTextColor((i == 3 || i == 5 || i == 6) ? TFT_BLACK : TFT_WHITE, colors[i]);
  display.drawString("918 CYD DISPLAY TEST", display.width() / 2, display.height() / 2 - 15, 2);
  display.drawString(labels[i], display.width() / 2, display.height() / 2 + 15, 4);
  Serial.printf("DIAG SCREEN phase=%u color=%s width=%d height=%d backlight=HIGH millis=%lu\n",
                i, labels[i], display.width(), display.height(), static_cast<unsigned long>(millis()));
  phase++;
}

void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println("DIAG START target=classic-esp32 panel=ILI9341 expected=240x320");
  pinMode(BacklightPin, OUTPUT);
  digitalWrite(BacklightPin, HIGH);
  Serial.println("DIAG BACKLIGHT pin=21 level=HIGH");
  display.init();
  Serial.println("DIAG TFT_INIT returned");
  display.setRotation(1);
  Serial.printf("DIAG TFT_GEOMETRY width=%d height=%d\n", display.width(), display.height());
  display.setSwapBytes(true);
  showPhase();
  nextChange = millis() + 2000;
}

void loop() {
  if (static_cast<int32_t>(millis() - nextChange) >= 0) {
    nextChange = millis() + 2000;
    showPhase();
  }
  delay(10);
}
