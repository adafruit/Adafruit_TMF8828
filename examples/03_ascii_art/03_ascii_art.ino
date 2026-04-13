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

#define TMF8828_EN_PIN 3 // GPIO pin connected to TMF8828 EN, or -1 to skip

Adafruit_TMF8828 tmf(TMF8828_EN_PIN);

static tmf8828_frame_t frame;

// Paul Bourke density ramp
const char densityRamp[] =
    " .'`^\",:;Il!i><~+_-?][}{1)(|/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";
const uint8_t rampLength = sizeof(densityRamp) - 1;
const uint16_t minDist = 200;  // mm (closest = dense)
const uint16_t maxDist = 2000; // mm (farthest = sparse)

static char distanceToChar(uint16_t distance) {
  if (distance == 0) {
    return ' ';
  }
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

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("Adafruit TMF8828 ASCII Art Demo"));
  Serial.println(F("Starting sensor..."));

  if (!tmf.begin(0x41, &Wire, 400000)) {
    halt(F("TMF8828 not found!"));
  }

  if (!tmf.setMode8x8()) {
    halt(F("Failed to set 8x8 mode"));
  }

  if (!tmf.configure(132, 250, TMF8828_SPAD_8X8)) {
    halt(F("Failed to configure sensor"));
  }

  if (!tmf.startRanging()) {
    halt(F("Failed to start ranging"));
  }

  Serial.println(F("Starting ASCII art display..."));
  delay(200);
  // Clear screen and hide cursor
  Serial.print(F("\033[2J\033[?25l"));
}

void loop() {
  if (!tmf.readFrame(&frame)) {
    return;
  }

  // ANSI: cursor home
  Serial.print(F("\033[H"));
  Serial.print(F("TMF8828 8x8 ASCII Art  Temp="));
  Serial.print(frame.temperature);
  Serial.println(F("C"));
  Serial.println(F("=========================="));

  // Rotated 90° CCW to match physical sensor orientation
  for (uint8_t col = 7; col < 8; col--) { // unsigned wraps to 255
    for (uint8_t row = 0; row < 8; row++) {
      Serial.print(distanceToChar(frame.distances[row][col]));
      if (row < 7) {
        Serial.print(F(" "));
      }
    }
    Serial.println();
  }
  Serial.println();
}

void halt(const __FlashStringHelper* msg) {
  Serial.println(msg);
  Serial.println(F("FAIL"));
  while (1) {
    delay(10);
  }
}
