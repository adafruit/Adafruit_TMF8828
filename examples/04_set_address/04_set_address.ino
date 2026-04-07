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
#else
#define EN_PIN A0
#endif

Adafruit_TMF8828 tmf(EN_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("Adafruit TMF8828 I2C Address Change"));
  Serial.println(F("Note: address changes are not permanent."));

  if (!tmf.begin(0x41, &Wire, 400000)) {
    halt(F("TMF8828 not found at 0x41"));
  }

  Serial.println(F("Changing address to 0x42..."));
  if (!tmf.changeI2CAddress(0x42)) {
    halt(F("Failed to change address"));
  }

  if (!tmf.begin(0x42, &Wire, 400000)) {
    halt(F("Could not talk to device at 0x42"));
  }
  Serial.println(F("Verified address 0x42"));

  Serial.println(F("Changing address back to 0x41..."));
  if (!tmf.changeI2CAddress(0x41)) {
    halt(F("Failed to change address back"));
  }

  if (!tmf.begin(0x41, &Wire, 400000)) {
    halt(F("Could not talk to device at 0x41"));
  }
  Serial.println(F("Verified address 0x41"));
  Serial.println(F("Done! Power cycle to reset the address."));
}

void loop() {
  delay(1000);
}
void halt(const __FlashStringHelper* msg) {
  Serial.println(msg);
  Serial.println(F("FAIL"));
  while (1) {
    delay(10);
  }
}
