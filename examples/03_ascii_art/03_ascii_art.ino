/*!
 * @file 03_ascii_art.ino
 *
 * ASCII art visualization for Adafruit TMF8828 8x8 ToF sensor.
 * Uses ANSI escape codes to animate in place.
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

// Paul Bourke density ramp
const char densityRamp[] =
    " .'`^\",:;Il!i><~+_-?][}{1)(|/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";
const uint8_t rampLength = sizeof(densityRamp) - 1;
const uint16_t minDist = 200;  // mm (closest)
const uint16_t maxDist = 2000; // mm (farthest)

uint16_t combinedDistance[64];
uint8_t combinedConfidence[64];
bool combinedValid[64];
bool seenSubcapture[4];

static void resetCombined() {
  for (uint8_t i = 0; i < 64; i++) {
    combinedDistance[i] = 0;
    combinedConfidence[i] = 0;
    combinedValid[i] = false;
  }
  for (uint8_t i = 0; i < 4; i++) {
    seenSubcapture[i] = false;
  }
}

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

static char distanceToChar(uint16_t distance) {
  if (distance <= minDist) {
    return densityRamp[rampLength - 1];
  }
  if (distance >= maxDist) {
    return densityRamp[0];
  }
  uint8_t idx = (uint8_t)((long)(maxDist - distance) * (rampLength - 1) /
                          (maxDist - minDist));
  return densityRamp[idx];
}

static void updateCombined(uint8_t subcapture, const tmf8828_result_t& data) {
  const uint8_t rowOffset[4] = {0, 0, 2, 2};
  const uint8_t colOffset[4] = {0, 2, 0, 2};
  uint8_t ro = rowOffset[subcapture & 0x03];
  uint8_t co = colOffset[subcapture & 0x03];

  for (uint8_t row = 0; row < 6; row++) {
    for (uint8_t col = 0; col < 6; col++) {
      uint8_t r = row + ro;
      uint8_t c = col + co;
      if (r >= 8 || c >= 8) {
        continue;
      }
      uint8_t idx6 = row * 6 + col;
      uint8_t idx8 = r * 8 + c;
      uint8_t conf = data.results[idx6].confidence;
      if (!combinedValid[idx8] || conf > combinedConfidence[idx8]) {
        combinedValid[idx8] = true;
        combinedConfidence[idx8] = conf;
        combinedDistance[idx8] = data.results[idx6].distance;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("Adafruit TMF8828 ASCII Art Demo"));
  Serial.println(F("Starting sensor..."));

  if (!tmf.begin(0x41)) {
    Serial.println(F("TMF8828 not found!"));
    while (1) {
      delay(10);
    }
  }

  if (!tmf.setMode8x8()) {
    Serial.println(F("Failed to set 8x8 mode"));
    while (1) {
      delay(10);
    }
  }

  if (!tmf.configure(132, 250, 15)) {
    Serial.println(F("Failed to configure sensor"));
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

  Serial.println(F("Starting ASCII art display..."));
  delay(200);
  Serial.print(F("\033[2J\033[?25l"));

  resetCombined();
}

void loop() {
  if (!waitForResult(5000)) {
    return;
  }

  uint8_t subcapture = result.resultNumber & 0x03;
  seenSubcapture[subcapture] = true;
  updateCombined(subcapture, result);

  bool haveAll = seenSubcapture[0] && seenSubcapture[1] && seenSubcapture[2] &&
                 seenSubcapture[3];
  if (!haveAll) {
    return;
  }

  Serial.print(F("\033[H"));
  Serial.println(F("TMF8828 8x8 ASCII Art"));
  Serial.println(F("===================="));

  for (uint8_t row = 0; row < 8; row++) {
    for (uint8_t col = 0; col < 8; col++) {
      uint8_t idx = row * 8 + col;
      char c = combinedValid[idx] ? distanceToChar(combinedDistance[idx]) : '?';
      Serial.print(c);
      if (col < 7) {
        Serial.print(F(" "));
      }
    }
    Serial.println();
  }
  Serial.println();

  resetCombined();
}
