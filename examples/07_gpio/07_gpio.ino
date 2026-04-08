/*!
 * @file 07_gpio.ino
 *
 * Simple GPIO demo for Adafruit TMF8828 Time-of-Flight sensor.
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code
 * Copyright 2026 Adafruit Industries
 *
 * MIT license, all text above must be included in any redistribution.
 */

#include <Adafruit_TMF8828.h>

#define TMF8828_EN_PIN -1 // GPIO pin connected to TMF8828 EN, or -1 to skip
#define GPIO0_PIN 4
#define GPIO1_PIN 5

Adafruit_TMF8828 tmf(TMF8828_EN_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("Adafruit TMF8828 GPIO Demo"));

  pinMode(GPIO0_PIN, INPUT);
  pinMode(GPIO1_PIN, INPUT);

  if (!tmf.begin(0x41, &Wire, 400000)) {
    halt(F("TMF8828 not found!"));
  }

  if (!tmf.setMode8x8()) {
    halt(F("Failed to set 8x8 mode"));
  }

  if (!tmf.configure(132, 250, TMF8828_SPAD_8X8)) {
    halt(F("Failed to configure sensor"));
  }

  Serial.println(F(""));
  Serial.println(F("Demo 1: Toggle GPIO0"));
  for (uint8_t i = 0; i < 5; i++) {
    tmf.setGPIO0(TMF8828_GPIO_OUTPUT_HIGH);
    delay(500);
    Serial.print(F("GPIO0 set HIGH, read="));
    Serial.println(digitalRead(GPIO0_PIN) == HIGH ? F("HIGH") : F("LOW"));

    tmf.setGPIO0(TMF8828_GPIO_OUTPUT_LOW);
    delay(500);
    Serial.print(F("GPIO0 set LOW, read="));
    Serial.println(digitalRead(GPIO0_PIN) == HIGH ? F("HIGH") : F("LOW"));
  }

  Serial.println(F(""));
  Serial.println(F("Demo 2: GPIO1 VCSEL sync"));
  if (!tmf.setGPIO1(TMF8828_GPIO_VCSEL_HIGH)) {
    halt(F("Failed to set GPIO1 VCSEL_HIGH"));
  }
  if (!tmf.startRanging()) {
    halt(F("Failed to start ranging"));
  }

  uint32_t start = millis();
  while ((millis() - start) < 3000) {
    Serial.print(F("GPIO1 read="));
    Serial.println(digitalRead(GPIO1_PIN) == HIGH ? F("HIGH") : F("LOW"));
    delay(100);
  }
  tmf.stopRanging();

  Serial.println(F(""));
  Serial.println(F("Demo 3: Getter readback"));
  tmf.setGPIO0(TMF8828_GPIO_OUTPUT_HIGH);
  tmf8828_gpio_mode_t mode = tmf.getGPIO0();
  Serial.print(F("getGPIO0="));
  Serial.println((int)mode);
}

void loop() {}

void halt(const __FlashStringHelper* msg) {
  Serial.println(msg);
  while (1) {
    delay(10);
  }
}
