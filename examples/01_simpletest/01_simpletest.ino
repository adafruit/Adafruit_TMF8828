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

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("Adafruit TMF8828 Simple Test"));

  // Args: I2C address, Wire bus, I2C speed (Hz)
  if (!tmf.begin(0x41, &Wire, 400000)) {
    halt(F("TMF8828 not found!"));
  }

  if (!tmf.setMode8x8()) {
    halt(F("Failed to set 8x8 mode"));
  }

  // Args: period (ms), kilo-iterations (integration time), SPAD map (zone
  // layout)
  if (!tmf.configure(132, 250, TMF8828_SPAD_8X8)) {
    halt(F("Failed to configure sensor"));
  }

  if (!tmf.startRanging()) {
    halt(F("Failed to start ranging"));
  }
}

void loop() {
  // Wait for a ranging result to arrive
  if (!tmf.dataReady()) {
    return;
  }
  if (!tmf.getRangingData(&result)) {
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

// Print a right-justified number padded to width
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

// Print a 6x6 distance/confidence grid
void printGrid(const tmf8828_result_t& data) {
  for (uint8_t row = 0; row < 6; row++) {
    for (uint8_t col = 0; col < 6; col++) {
      uint8_t idx = row * 6 + col;
      printPadded(data.results[idx].distance, 4);
      Serial.print(F("/"));
      printPadded(data.results[idx].confidence, 2);
      if (col < 5)
        Serial.print(F(" "));
    }
    Serial.println();
  }
}

void halt(const __FlashStringHelper* msg) {
  Serial.println(msg);
  Serial.println(F("FAIL"));
  while (1) {
    delay(10);
  }
}
