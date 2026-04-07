/*!
 * @file 05_power_modes.ino
 *
 * Demonstrates TMF8828 standby and wakeup power modes.
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code
 * Copyright 2026 Adafruit Industries
 *
 * MIT license, all text above must be included in any redistribution.
 */

#include <Adafruit_TMF8828.h>

#define TMF8828_EN_PIN 3 // GPIO pin connected to TMF8828 EN, or -1 to skip

Adafruit_TMF8828 tmf(TMF8828_EN_PIN);

tmf8828_result_t result;

static bool waitForResult(uint32_t timeoutMs) {
  uint32_t start = millis();
  while ((millis() - start) < timeoutMs) {
    if (tmf.dataReady()) {
      if (tmf.getRangingData(&result)) {
        return true;
      }
    }
    delay(5);
  }
  return false;
}

static bool doRangingCycle(uint32_t durationMs) {
  if (!tmf.configure(132, 250, 15)) {
    Serial.println(F("configure FAILED"));
    return false;
  }

  if (!tmf.startRanging()) {
    Serial.println(F("startRanging FAILED"));
    return false;
  }

  uint32_t start = millis();
  while ((millis() - start) < durationMs) {
    if (waitForResult(1000)) {
      Serial.print(F("ValidResults="));
      Serial.println(result.validResults);
    }
  }

  tmf.stopRanging();
  Serial.println(F("stopRanging OK"));
  return true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("Adafruit TMF8828 Power Modes Demo"));
  Serial.println(F("Ranging draws tens of mA; standby is low microamps."));

  // Args: I2C address, Wire bus, I2C speed (Hz)
  if (!tmf.begin(0x41, &Wire, 400000)) {
    halt(F("TMF8828 not found!"));
  }

  if (!tmf.setMode8x8()) {
    halt(F("Failed to set 8x8 mode"));
  }

  Serial.println(F("Ranging for a few seconds..."));
  if (!doRangingCycle(3000)) {
    halt(F("Ranging cycle FAILED"));
  }

  Serial.println(F("Entering standby (expect very low current)..."));
  if (!tmf.standby()) {
    halt(F("standby FAILED"));
  }

  delay(2000);

  Serial.println(F("Waking up (expect ranging current again)..."));
  if (!tmf.wakeup()) {
    halt(F("wakeup FAILED"));
  }

  Serial.println(F("Ranging after wakeup..."));
  if (!doRangingCycle(3000)) {
    halt(F("Ranging cycle FAILED"));
  }

  Serial.println(F("Done with power mode demo."));
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
