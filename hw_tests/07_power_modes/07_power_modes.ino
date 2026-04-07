/*!
 * @file 07_power_modes.ino
 *
 * Hardware test for TMF8828 power mode transitions.
 *
 * Copyright 2026 Adafruit Industries (Limor 'ladyada' Fried with assistance
 * from Claude Code)
 *
 * MIT License
 */

#include <Adafruit_TMF8828.h>

// Set to a GPIO pin to hardware-reset the sensor before init, or -1 to skip
#define EN_PIN -1

Adafruit_TMF8828 tmf(EN_PIN);

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

static bool runSingleRanging(const __FlashStringHelper* label) {
  Serial.println(F(""));
  Serial.println(label);

  if (!tmf.configure(132, 250, 15)) {
    Serial.println(F("configure FAILED"));
    return false;
  }
  Serial.println(F("configure PASS"));

  tmf.clearAndEnableInterrupts(TMF8828_APP_I2C_RESULT_IRQ_MASK);

  if (!tmf.startRanging()) {
    Serial.println(F("startRanging FAILED"));
    return false;
  }
  Serial.println(F("startRanging PASS"));

  if (!waitForResult(5000)) {
    Serial.println(F("Timeout waiting for data"));
    tmf.stopRanging();
    Serial.println(F("stopRanging PASS"));
    return false;
  }

  Serial.print(F("validResults="));
  Serial.println(result.validResults);

  tmf.stopRanging();
  Serial.println(F("stopRanging PASS"));

  if (result.validResults > 0) {
    Serial.println(F("Ranging PASS"));
    return true;
  }

  Serial.println(F("Ranging FAIL"));
  return false;
}

static bool verifyNoDataReady(uint8_t attempts, uint16_t delayMs) {
  bool ok = true;

  Serial.print(F("Checking dataReady() for "));
  Serial.print(attempts);
  Serial.println(F(" attempts"));

  for (uint8_t i = 0; i < attempts; i++) {
    bool ready = tmf.dataReady();
    Serial.print(F("dataReady #"));
    Serial.print(i);
    Serial.print(F(" -> "));
    Serial.println(ready ? F("true") : F("false"));
    if (ready) {
      ok = false;
    }
    delay(delayMs);
  }

  if (ok) {
    Serial.println(F("No data ready as expected PASS"));
  } else {
    Serial.println(F("Unexpected data ready FAIL"));
  }

  return ok;
}

static bool runStandbyWakeupCycle() {
  Serial.println(F(""));
  Serial.println(F("Standby / Wakeup cycle"));

  if (!runSingleRanging(F("Initial ranging before standby"))) {
    return false;
  }

  if (!tmf.standby()) {
    Serial.println(F("standby FAILED"));
    return false;
  }
  Serial.println(F("standby PASS"));

  if (!verifyNoDataReady(5, 100)) {
    return false;
  }

  if (!tmf.wakeup()) {
    Serial.println(F("wakeup FAILED"));
    return false;
  }
  Serial.println(F("wakeup PASS"));

  return runSingleRanging(F("Ranging after wakeup"));
}

static bool runMultipleStandbyCycles() {
  Serial.println(F(""));
  Serial.println(F("Multiple standby / wakeup cycles"));

  bool ok = true;
  for (uint8_t i = 0; i < 3; i++) {
    Serial.print(F("Cycle "));
    Serial.println(i + 1);

    if (!tmf.standby()) {
      Serial.println(F("standby FAILED"));
      ok = false;
      break;
    }
    Serial.println(F("standby PASS"));

    delay(50);

    if (!tmf.wakeup()) {
      Serial.println(F("wakeup FAILED"));
      ok = false;
      break;
    }
    Serial.println(F("wakeup PASS"));
  }

  if (!ok) {
    return false;
  }

  return runSingleRanging(F("Ranging after 3 cycles"));
}

static bool runDisableEnableCycle() {
  Serial.println(F(""));
  Serial.println(F("Disable / Enable cycle"));

  tmf.disable();
  Serial.println(F("disable done"));
  delay(100);

  tmf.enable();
  Serial.println(F("enable done"));
  delay(10);

  Serial.println(F("Reinitializing after enable"));
  // Args: I2C address, Wire bus, I2C speed (Hz)
  if (!tmf.begin(0x41, &Wire, 400000)) {
    Serial.println(F("begin FAILED"));
    return false;
  }
  Serial.println(F("begin PASS"));

  if (!tmf.setMode8x8()) {
    Serial.println(F("setMode8x8 FAILED"));
    return false;
  }
  Serial.println(F("setMode8x8 PASS"));

  return runSingleRanging(F("Ranging after re-init"));
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("TMF8828 power mode test"));

  bool overall = true;

  // Args: I2C address, Wire bus, I2C speed (Hz)
  if (!tmf.begin(0x41, &Wire, 400000)) {
    halt(F("begin FAILED"));
  }
  Serial.println(F("begin PASS"));

  if (!tmf.setMode8x8()) {
    halt(F("setMode8x8 FAILED"));
  }
  Serial.println(F("setMode8x8 PASS"));

  Serial.println(F(""));
  Serial.println(
      F("Note: standby current should drop to ~1.5uA vs ~300mA active"));

  if (!runStandbyWakeupCycle()) {
    overall = false;
  }

  if (!runMultipleStandbyCycles()) {
    overall = false;
  }

  if (!runDisableEnableCycle()) {
    overall = false;
  }

  tmf.stopRanging();
  Serial.println(F("stopRanging PASS"));

  Serial.println(F(""));
  if (overall) {
    Serial.println(F("PASS"));
  } else {
    Serial.println(F("FAIL"));
  }
}

void loop() {}

void halt(const __FlashStringHelper* msg) {
  Serial.println(msg);
  Serial.println(F("FAIL"));
  while (1) {
    delay(10);
  }
}
