/*!
 * @file 02_legacy_mode.ino
 *
 * Legacy 3x3 ranging example for Adafruit TMF8828.
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code
 * Copyright 2026 Adafruit Industries
 *
 * MIT license, all text above must be included in any redistribution.
 */

#include <Adafruit_TMF8828.h>

#define TMF8828_EN_PIN 3 // GPIO pin connected to TMF8828 EN, or -1 to skip

Adafruit_TMF8828 tmf(TMF8828_EN_PIN);

tmf8828_result_t result;

// Legacy SPAD map IDs (1-13):
// 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13
// Use configure(periodMs, kiloIterations, spadMapId) to select.

static bool waitForResult(uint32_t timeoutMs) {
  uint32_t start = millis();
  while ((millis() - start) < timeoutMs) {
    if (tmf.dataReady()) {
      if (tmf.getRangingData(&result)) {
        return true;
      }
    }
    delay(5);
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("Adafruit TMF8828 Legacy 3x3 Test"));

  if (!tmf.begin(0x41, &Wire, 400000)) {
    halt(F("TMF8828 not found!"));
  }

  if (!tmf.setModeLegacy()) {
    halt(F("Failed to set legacy mode"));
  }

  if (!tmf.configure(132, 250, 6)) {
    halt(F("Failed to configure legacy mode"));
  }

  if (!tmf.startRanging()) {
    halt(F("Failed to start ranging"));
  }
}

void loop() {
  if (!waitForResult(5000)) {
    Serial.println(F("Timeout waiting for data"));
    return;
  }

  // ANSI: cursor home + clear screen for stable display
  Serial.print(F("\033[H\033[2J"));

  Serial.print(F("Temp="));
  Serial.print(result.temperature);
  Serial.println(F("C"));

  Serial.println(F("Distance (mm):   Confidence:"));
  for (uint8_t row = 0; row < 3; row++) {
    Serial.print(F("  "));
    for (uint8_t col = 0; col < 3; col++) {
      uint8_t idx = row * 3 + col;
      printPadded(result.results[idx].distance, 5);
    }
    Serial.print(F("   "));
    for (uint8_t col = 0; col < 3; col++) {
      uint8_t idx = row * 3 + col;
      printPadded(result.results[idx].confidence, 4);
    }
    Serial.println();
  }
}

// Print right-justified number padded to width
void printPadded(uint16_t val, uint8_t width) {
  uint16_t tmp = val;
  uint8_t digits = 1;
  while (tmp >= 10) {
    tmp /= 10;
    digits++;
  }
  while (digits < width) {
    Serial.print(F(" "));
    digits++;
  }
  Serial.print(val);
}

void halt(const __FlashStringHelper* msg) {
  Serial.println(msg);
  Serial.println(F("FAIL"));
  while (1) {
    delay(10);
  }
}
