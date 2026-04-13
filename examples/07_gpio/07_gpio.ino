/*!
 * @file 07_gpio.ino
 *
 * GPIO demo for Adafruit TMF8828 Time-of-Flight sensor.
 * GPIO0 reads a button (active-low with internal pull-up).
 * GPIO1 blinks an LED continuously.
 *
 * Wiring:
 *   GPIO0 -> button to GND (no external pull-up needed)
 *   GPIO1 -> LED anode (with current-limiting resistor to GND)
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code
 * Copyright 2026 Adafruit Industries
 *
 * MIT license, all text above must be included in any redistribution.
 */

#include <Adafruit_TMF8828.h>

#define TMF8828_EN_PIN 3 // GPIO pin connected to TMF8828 EN, or -1 to skip
#define GPIO0_PIN 4      // MCU pin wired to TMF8828 GPIO0 (for digitalRead)

Adafruit_TMF8828 tmf(TMF8828_EN_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("Adafruit TMF8828 GPIO Demo"));
  Serial.println(F("GPIO0 = button input (active-low)"));
  Serial.println(F("GPIO1 = LED blink output"));

  if (!tmf.begin(0x41, &Wire, 400000)) {
    halt(F("TMF8828 not found!"));
  }

  if (!tmf.setMode8x8()) {
    halt(F("Failed to set 8x8 mode"));
  }

  // GPIO0: input with pull-up (reads HIGH when open, LOW when button pressed)
  if (!tmf.setGPIO0(TMF8828_GPIO_INPUT_HIGH)) {
    halt(F("Failed to set GPIO0 input"));
  }
  pinMode(GPIO0_PIN, INPUT);

  // GPIO1: start with output high (LED on)
  if (!tmf.setGPIO1(TMF8828_GPIO_OUTPUT_HIGH)) {
    halt(F("Failed to set GPIO1 output"));
  }

  Serial.println(F("Ready! Press button on GPIO0, watch LED on GPIO1."));
}

void loop() {
  // Read button on GPIO0
  int buttonState = digitalRead(GPIO0_PIN);
  Serial.print(F("Button: "));
  Serial.print(buttonState == LOW ? F("PRESSED") : F("released"));

  // Blink LED on GPIO1
  static bool ledOn = true;
  ledOn = !ledOn;
  tmf.setGPIO1(ledOn ? TMF8828_GPIO_OUTPUT_HIGH : TMF8828_GPIO_OUTPUT_LOW);
  Serial.print(F("  LED: "));
  Serial.println(ledOn ? F("ON") : F("OFF"));

  delay(500);
}

void halt(const __FlashStringHelper* msg) {
  Serial.println(msg);
  while (1) {
    delay(10);
  }
}
