/*!
 * @file 04_set_address.ino
 *
 * Demonstrates changing the TMF8828 I2C address.
 * Address changes are volatile and reset on power cycle.
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

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("Adafruit TMF8828 I2C Address Change"));
  Serial.println(F("Note: address changes are not permanent."));

  if (!tmf.begin(0x41)) {
    Serial.println(F("TMF8828 not found at 0x41"));
    while (1) {
      delay(10);
    }
  }

  Serial.println(F("Changing address to 0x42..."));
  if (!tmf.changeI2CAddress(0x42)) {
    Serial.println(F("Failed to change address"));
    while (1) {
      delay(10);
    }
  }

  if (!tmf.begin(0x42)) {
    Serial.println(F("Could not talk to device at 0x42"));
    while (1) {
      delay(10);
    }
  }
  Serial.println(F("Verified address 0x42"));

  Serial.println(F("Changing address back to 0x41..."));
  if (!tmf.changeI2CAddress(0x41)) {
    Serial.println(F("Failed to change address back"));
    while (1) {
      delay(10);
    }
  }

  if (!tmf.begin(0x41)) {
    Serial.println(F("Could not talk to device at 0x41"));
    while (1) {
      delay(10);
    }
  }
  Serial.println(F("Verified address 0x41"));
  Serial.println(F("Done! Power cycle to reset the address."));
}

void loop() {
  delay(1000);
}
