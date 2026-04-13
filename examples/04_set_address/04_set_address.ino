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

#define TMF8828_EN_PIN 3 // GPIO pin connected to TMF8828 EN, or -1 to skip

#define DEFAULT_ADDR 0x41
#define ALT_ADDR 0x42

Adafruit_TMF8828 tmf(TMF8828_EN_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("Adafruit TMF8828 I2C Address Change"));
  Serial.println(F("Note: address changes are not permanent."));

  // Try default address first; if that fails, toggle EN to power-cycle the
  // sensor (a CPU reset reverts volatile address changes, but only works if
  // we can reach the sensor on I2C — EN toggle always works)
  if (!tmf.begin(DEFAULT_ADDR, &Wire, 400000)) {
    Serial.println(F("Not at 0x41 — toggling EN to power-cycle sensor..."));
    pinMode(TMF8828_EN_PIN, OUTPUT);
    digitalWrite(TMF8828_EN_PIN, LOW);
    delay(100);
    digitalWrite(TMF8828_EN_PIN, HIGH);
    delay(100);
    if (!tmf.begin(DEFAULT_ADDR, &Wire, 400000)) {
      halt(F("TMF8828 not found after EN power-cycle"));
    }
  }
  Serial.println(F("Found TMF8828 at 0x41"));

  Serial.println(F("Changing address to 0x42..."));
  if (!tmf.changeI2CAddress(ALT_ADDR)) {
    halt(F("Failed to change address"));
  }

  // Verify at new address (don't call begin — that resets the CPU which
  // reverts the volatile address change)
  if (!tmf.readDeviceInfo()) {
    halt(F("Could not verify at 0x42"));
  }
  Serial.print(F("Verified at 0x42, serial=0x"));
  Serial.println(tmf.getSerialNumber(), HEX);

  Serial.println(F("Changing address back to 0x41..."));
  if (!tmf.changeI2CAddress(DEFAULT_ADDR)) {
    halt(F("Failed to change address back"));
  }

  if (!tmf.readDeviceInfo()) {
    halt(F("Could not verify at 0x41"));
  }
  Serial.print(F("Verified at 0x41, serial=0x"));
  Serial.println(tmf.getSerialNumber(), HEX);
  Serial.println(F("Done! Power cycle also resets the address."));
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
