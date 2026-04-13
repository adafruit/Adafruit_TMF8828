/*!
 * @file 08_interrupt_driven.ino
 *
 * Interrupt-driven ranging for Adafruit TMF8828 Time-of-Flight sensor.
 * Uses the INT pin to avoid busy-waiting — the MCU sleeps until a result
 * interrupt fires, then reads and displays the 8x8 distance grid.
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code
 * Copyright 2026 Adafruit Industries
 *
 * MIT license, all text above must be included in any redistribution.
 */

#include <Adafruit_TMF8828.h>

#define TMF8828_EN_PIN -1 // GPIO pin connected to TMF8828 EN, or -1 to skip
#define TMF8828_INT_PIN 2 // Must be an interrupt-capable pin (D2 = INT0)

Adafruit_TMF8828 tmf(TMF8828_EN_PIN);

static tmf8828_frame_t frame;
volatile bool intFired = false;

static void onInterrupt() {
  intFired = true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("Adafruit TMF8828 Interrupt-Driven Example"));

  pinMode(TMF8828_INT_PIN, INPUT_PULLUP);

  if (!tmf.begin(0x41, &Wire, 400000)) {
    halt(F("TMF8828 not found!"));
  }

  if (!tmf.setMode8x8()) {
    halt(F("Failed to set 8x8 mode"));
  }

  if (!tmf.configure(132, 250, TMF8828_SPAD_8X8)) {
    halt(F("Failed to configure sensor"));
  }

  // Enable result interrupts on the sensor
  tmf.clearAndEnableInterrupts(TMF8828_APP_I2C_RESULT_IRQ_MASK);

  // Attach MCU interrupt — INT pin goes LOW on new result
  attachInterrupt(digitalPinToInterrupt(TMF8828_INT_PIN), onInterrupt, FALLING);

  if (!tmf.startRanging()) {
    halt(F("Failed to start ranging"));
  }
}

void loop() {
  // Sleep until the ISR sets the flag — no busy polling
  if (!intFired) {
    return;
  }
  intFired = false;

  // readFrame() checks dataReady() internally and clears the result interrupt.
  // It returns true only when all 4 subcaptures are collected into one frame.
  if (!tmf.readFrame(&frame)) {
    return;
  }

  // ANSI: cursor home + clear screen for stable display
  Serial.print(F("\033[H\033[2J"));

  Serial.print(F("Temp="));
  Serial.print(frame.temperature);
  Serial.println(F("C"));

  // Print grid rotated 90 CCW to match physical sensor orientation
  Serial.println(F("Distance (mm):                          Confidence:"));
  for (uint8_t col = 7; col < 8; col--) {
    Serial.print(F("  "));
    for (uint8_t row = 0; row < 8; row++) {
      printPadded(frame.distances[row][col], 5);
    }
    Serial.print(F("   "));
    for (uint8_t row = 0; row < 8; row++) {
      printPadded(frame.confidences[row][col], 4);
    }
    Serial.println();
  }
}

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
  while (1) {
    delay(10);
  }
}
