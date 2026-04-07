/*!
 * @file 01_simpletest.ino
 *
 * Simple test for Adafruit TMF8828 8x8 Time-of-Flight sensor.
 * Prints a 6x6 distance/confidence grid for each subcapture.
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code
 * Copyright 2026 Adafruit Industries
 *
 * MIT license, all text above must be included in any redistribution.
 */

#include <Adafruit_TMF8828.h>

Adafruit_TMF8828 tmf;

static tmf8828_result_t result;

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

static void printCell(uint16_t distance, uint8_t confidence) {
  if (distance < 10) {
    Serial.print(F("   "));
  } else if (distance < 100) {
    Serial.print(F("  "));
  } else if (distance < 1000) {
    Serial.print(F(" "));
  }
  Serial.print(distance);
  Serial.print(F("/"));
  if (confidence < 10) {
    Serial.print(F("0"));
  }
  Serial.print(confidence);
}

static void printGrid(const tmf8828_result_t& data) {
  for (uint8_t row = 0; row < 6; row++) {
    for (uint8_t col = 0; col < 6; col++) {
      uint8_t idx = row * 6 + col;
      printCell(data.results[idx].distance, data.results[idx].confidence);
      if (col < 5) {
        Serial.print(F(" "));
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

  Serial.println(F("Adafruit TMF8828 Simple Test"));

  if (!tmf.begin(0x41, &Wire, 400000)) {
    halt(F("TMF8828 not found!"));
  }

  if (!tmf.setMode8x8()) {
    halt(F("Failed to set 8x8 mode"));
  }

  if (!tmf.configure(132, 250, 15)) {
    halt(F("Failed to configure sensor"));
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

  Serial.println();
  Serial.print(F("ResultNumber="));
  Serial.print(result.resultNumber);
  Serial.print(F(" Temp="));
  Serial.print(result.temperature);
  Serial.print(F(" ValidResults="));
  Serial.println(result.validResults);

  uint8_t subcapture = result.resultNumber & 0x03;
  Serial.print(F("Subcapture "));
  Serial.println(subcapture);

  printGrid(result);
}
void halt(const __FlashStringHelper* msg) {
  Serial.println(msg);
  Serial.println(F("FAIL"));
  while (1) {
    delay(10);
  }
}
