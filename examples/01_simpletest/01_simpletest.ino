/*!
 * @file 01_simpletest.ino
 *
 * Simple test for Adafruit TMF8828 8x8 Time-of-Flight sensor.
 * In 8x8 mode the sensor time-multiplexes 4 subcaptures, each returning
 * up to 36 zone results. This example uses readFrame() to accumulate all
 * 4 subcaptures and prints the distance and confidence grids.
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code
 * Copyright 2026 Adafruit Industries
 *
 * MIT license, all text above must be included in any redistribution.
 */

#include <Adafruit_TMF8828.h>

Adafruit_TMF8828 tmf;

static tmf8828_frame_t frame;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("Adafruit TMF8828 Simple Test"));

  // Set to a GPIO pin to hardware-reset the sensor before init, or -1 to skip
  if (!tmf.begin(0x41, &Wire, 400000)) {
    halt(F("TMF8828 not found!"));
  }

  if (!tmf.setMode8x8()) {
    halt(F("Failed to set 8x8 mode"));
  }

  // Args: period (ms), kilo-iterations (integration time), SPAD map
  if (!tmf.configure(132, 250, TMF8828_SPAD_8X8)) {
    halt(F("Failed to configure sensor"));
  }

  if (!tmf.startRanging()) {
    halt(F("Failed to start ranging"));
  }
}

void loop() {
  // readFrame() returns true once all 4 subcaptures are collected
  if (!tmf.readFrame(&frame)) {
    return;
  }

  Serial.println();
  Serial.print(F("Temp="));
  Serial.print(frame.temperature);
  Serial.println(F("C"));

  Serial.println(F("Distance (mm):"));
  for (uint8_t s = 0; s < 4; s++) {
    Serial.print(F("  Sub "));
    Serial.println(s);
    printGrid16(frame.distances[s]);
  }

  Serial.println(F("Confidence:"));
  for (uint8_t s = 0; s < 4; s++) {
    Serial.print(F("  Sub "));
    Serial.println(s);
    printGrid8(frame.confidences[s]);
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

// Print a 6x6 grid of uint16_t values
void printGrid16(const uint16_t* data) {
  for (uint8_t row = 0; row < 6; row++) {
    Serial.print(F("    "));
    for (uint8_t col = 0; col < 6; col++) {
      printPadded(data[row * 6 + col], 5);
      if (col < 5)
        Serial.print(F(" "));
    }
    Serial.println();
  }
}

// Print a 6x6 grid of uint8_t values
void printGrid8(const uint8_t* data) {
  for (uint8_t row = 0; row < 6; row++) {
    Serial.print(F("    "));
    for (uint8_t col = 0; col < 6; col++) {
      printPadded(data[row * 6 + col], 3);
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
