/*!
 * @file 01_simpletest.ino
 *
 * Simple test for Adafruit TMF8828 Time-of-Flight sensor.
 * Prints a complete 8x8 distance and confidence grid.
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
  // readFrame() collects all 4 subcaptures and assembles a true 8x8 grid.
  // Returns true when a complete frame is ready.
  if (!tmf.readFrame(&frame)) {
    return;
  }

  Serial.println();
  Serial.print(F("Temp="));
  Serial.print(frame.temperature);
  Serial.println(F("C"));

  Serial.println(F("Distance (mm):"));
  for (uint8_t row = 0; row < 8; row++) {
    Serial.print(F("  "));
    for (uint8_t col = 0; col < 8; col++) {
      printPadded(frame.distances[row][col], 5);
      if (col < 7)
        Serial.print(F(" "));
    }
    Serial.println();
  }

  Serial.println(F("Confidence:"));
  for (uint8_t row = 0; row < 8; row++) {
    Serial.print(F("  "));
    for (uint8_t col = 0; col < 8; col++) {
      printPadded(frame.confidences[row][col], 3);
      if (col < 7)
        Serial.print(F(" "));
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
