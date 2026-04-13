/*!
 * @file 07_gpio.ino
 *
 * GPIO demo for Adafruit TMF8828 Time-of-Flight sensor.
 * Alternately toggles GPIO0 and GPIO1 as outputs — wire LEDs to see them
 * blink back and forth.
 *
 * Wiring:
 *   GPIO0 -> LED + resistor to GND
 *   GPIO1 -> LED + resistor to GND
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code
 * Copyright 2026 Adafruit Industries
 *
 * MIT license, all text above must be included in any redistribution.
 */

#include <Adafruit_TMF8828.h>

#define TMF8828_EN_PIN 3 // GPIO pin connected to TMF8828 EN, or -1 to skip

Adafruit_TMF8828 tmf(TMF8828_EN_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("Adafruit TMF8828 GPIO Toggle Demo"));

  if (!tmf.begin(0x41, &Wire, 400000)) {
    halt(F("TMF8828 not found!"));
  }

  if (!tmf.setMode8x8()) {
    halt(F("Failed to set 8x8 mode"));
  }

  Serial.println(F("Alternating GPIO0 and GPIO1..."));
}

void loop() {
  // GPIO0 on, GPIO1 off
  tmf.setGPIO0(TMF8828_GPIO_OUTPUT_HIGH);
  tmf.setGPIO1(TMF8828_GPIO_OUTPUT_LOW);
  Serial.println(F("GPIO0=HIGH  GPIO1=LOW"));
  delay(500);

  // GPIO0 off, GPIO1 on
  tmf.setGPIO0(TMF8828_GPIO_OUTPUT_LOW);
  tmf.setGPIO1(TMF8828_GPIO_OUTPUT_HIGH);
  Serial.println(F("GPIO0=LOW   GPIO1=HIGH"));
  delay(500);
}

void halt(const __FlashStringHelper* msg) {
  Serial.println(msg);
  while (1) {
    delay(10);
  }
}
