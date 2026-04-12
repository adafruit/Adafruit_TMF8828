/*!
 * @file 05_power_modes.ino
 *
 * Demonstrates low-power periodic sensing with the TMF8828.
 * Wakes up, captures a full 8x8 frame, prints it, then enters
 * standby for several seconds. Useful for battery-powered designs.
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code
 * Copyright 2026 Adafruit Industries
 *
 * MIT license, all text above must be included in any redistribution.
 */

#include <Adafruit_TMF8828.h>

#define TMF8828_EN_PIN 3 // GPIO pin connected to TMF8828 EN, or -1 to skip
#define SLEEP_SECONDS 5  // How long to standby between measurements

Adafruit_TMF8828 tmf(TMF8828_EN_PIN);

tmf8828_frame_t frame;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("Adafruit TMF8828 Low-Power Periodic Sensing"));
  Serial.print(F("Sleep between frames: "));
  Serial.print(SLEEP_SECONDS);
  Serial.println(F(" seconds"));

  if (!tmf.begin(0x41, &Wire, 400000)) {
    halt(F("TMF8828 not found!"));
  }

  if (!tmf.setMode8x8()) {
    halt(F("Failed to set 8x8 mode"));
  }

  Serial.println(F("Ready.\n"));
}

void loop() {
  // Wake from standby (first iteration is a no-op since sensor starts awake)
  tmf.wakeup();

  // Configure and start ranging
  if (!tmf.configure(132, 250, 15)) {
    Serial.println(F("configure FAILED"));
    return;
  }
  tmf.clearAndEnableInterrupts(TMF8828_APP_I2C_RESULT_IRQ_MASK);
  if (!tmf.startRanging()) {
    Serial.println(F("startRanging FAILED"));
    return;
  }

  // Collect one full 8x8 frame (4 subcaptures)
  bool gotFrame = false;
  uint32_t start = millis();
  while ((millis() - start) < 5000) {
    if (tmf.readFrame(&frame)) {
      gotFrame = true;
      break;
    }
    delay(5);
  }
  tmf.stopRanging();

  if (gotFrame) {
    Serial.print(F("Temp: "));
    Serial.print(frame.temperature);
    Serial.println(F(" C"));

    // Print 8x8 distance grid (mm)
    for (uint8_t row = 0; row < 8; row++) {
      for (uint8_t col = 0; col < 8; col++) {
        printPadded(frame.distances[row][col], 5);
      }
      Serial.println();
    }
    Serial.println();
  } else {
    Serial.println(F("No frame received"));
  }

  // Enter standby — current drops to microamps
  tmf.standby();

  // Sleep on the MCU side too (could use a real low-power sleep here)
  for (uint8_t i = 0; i < SLEEP_SECONDS; i++) {
    delay(1000);
  }
}

void printPadded(uint16_t val, uint8_t width) {
  // Right-justify a number in the given field width
  uint16_t tmp = val;
  uint8_t digits = 1;
  while (tmp >= 10) {
    tmp /= 10;
    digits++;
  }
  for (uint8_t i = digits; i < width; i++) {
    Serial.print(' ');
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
