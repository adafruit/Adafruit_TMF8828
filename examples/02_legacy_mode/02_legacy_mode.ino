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

#ifdef ESP32
#define EN_PIN 27
#define INT_PIN 14
#else
#define EN_PIN A0
#define INT_PIN A1
#endif

Adafruit_TMF8828 tmf(EN_PIN, INT_PIN);

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

static void printGrid3x3(const tmf8828_result_t& data) {
  for (uint8_t row = 0; row < 3; row++) {
    for (uint8_t col = 0; col < 3; col++) {
      uint8_t idx = row * 3 + col;
      Serial.print(data.results[idx].distance);
      if (col < 2) {
        Serial.print(F("\t"));
      }
    }
    Serial.println();
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("Adafruit TMF8828 Legacy 3x3 Test"));

  if (!tmf.begin(0x41)) {
    Serial.println(F("TMF8828 not found!"));
    while (1) {
      delay(10);
    }
  }

  if (!tmf.setModeLegacy()) {
    Serial.println(F("Failed to set legacy mode"));
    while (1) {
      delay(10);
    }
  }

  if (!tmf.configure(132, 250, 6)) {
    Serial.println(F("Failed to configure legacy mode"));
    while (1) {
      delay(10);
    }
  }

  if (!tmf.startRanging()) {
    Serial.println(F("Failed to start ranging"));
    while (1) {
      delay(10);
    }
  }
}

void loop() {
  if (!waitForResult(5000)) {
    Serial.println(F("Timeout waiting for data"));
    return;
  }

  Serial.println();
  Serial.print(F("ResultNumber="));
  Serial.print(result.resultNumber);
  Serial.print(F(" Temp="));
  Serial.println(result.temperature);

  printGrid3x3(result);
}
