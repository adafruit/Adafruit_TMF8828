/*!
 * @file 06_thresholds.ino
 *
 * Hardware test for TMF8828 setThresholds().
 *
 * Copyright 2026 Adafruit Industries (Limor 'ladyada' Fried with assistance
 * from Claude Code)
 *
 * MIT License
 */

#include <Adafruit_TMF8828.h>

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

static void printFrameData(uint8_t frame) {
  Serial.print(F("Frame "));
  Serial.print(frame);
  Serial.print(F(" validResults="));
  Serial.println(result.validResults);

  for (uint8_t row = 0; row < 6; row++) {
    for (uint8_t col = 0; col < 6; col++) {
      uint8_t idx = (row * 6) + col;
      Serial.print(result.results[idx].distance);
      Serial.print(F("/"));
      Serial.print(result.results[idx].confidence);
      if (col < 5) {
        Serial.print(F("\t"));
      }
    }
    Serial.println();
  }
}

static bool runThresholdAcceptanceTest() {
  Serial.println(F(""));
  Serial.println(F("Threshold configuration acceptance test"));

  bool ok = true;
  bool resultOk = false;

  resultOk = tmf.setThresholds(100, 2000, 0);
  Serial.print(F("setThresholds(100, 2000, 0) -> "));
  Serial.println(resultOk ? F("true") : F("false"));
  if (!resultOk) {
    ok = false;
  }

  resultOk = tmf.setThresholds(0, 5000, 3);
  Serial.print(F("setThresholds(0, 5000, 3) -> "));
  Serial.println(resultOk ? F("true") : F("false"));
  if (!resultOk) {
    ok = false;
  }

  resultOk = tmf.setThresholds(500, 1000, 10);
  Serial.print(F("setThresholds(500, 1000, 10) -> "));
  Serial.println(resultOk ? F("true") : F("false"));
  if (!resultOk) {
    ok = false;
  }

  if (ok) {
    Serial.println(F("Threshold configuration PASS"));
  } else {
    Serial.println(F("Threshold configuration FAIL"));
  }

  return ok;
}

static bool runThresholdWindowTest(const __FlashStringHelper* label,
                                   uint16_t low, uint16_t high,
                                   uint8_t persistence) {
  Serial.println(F(""));
  Serial.println(label);

  bool ok = tmf.setThresholds(low, high, persistence);
  Serial.print(F("setThresholds("));
  Serial.print(low);
  Serial.print(F(", "));
  Serial.print(high);
  Serial.print(F(", "));
  Serial.print(persistence);
  Serial.print(F(") -> "));
  Serial.println(ok ? F("true") : F("false"));
  if (!ok) {
    return false;
  }

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

  for (uint8_t frame = 0; frame < 4; frame++) {
    if (!waitForResult(5000)) {
      Serial.println(F("Timeout waiting for data"));
      tmf.stopRanging();
      Serial.println(F("stopRanging PASS"));
      return false;
    }
    printFrameData(frame);
  }

  tmf.stopRanging();
  Serial.println(F("stopRanging PASS"));
  return true;
}

static bool runPersistenceAcceptanceTest() {
  Serial.println(F(""));
  Serial.println(F("Threshold persistence acceptance test"));

  bool ok = true;
  bool resultOk = false;

  resultOk = tmf.setThresholds(100, 2000, 0);
  Serial.print(F("setThresholds(100, 2000, 0) -> "));
  Serial.println(resultOk ? F("true") : F("false"));
  if (!resultOk) {
    ok = false;
  }

  resultOk = tmf.setThresholds(100, 2000, 5);
  Serial.print(F("setThresholds(100, 2000, 5) -> "));
  Serial.println(resultOk ? F("true") : F("false"));
  if (!resultOk) {
    ok = false;
  }

  if (ok) {
    Serial.println(F("Threshold persistence PASS"));
  } else {
    Serial.println(F("Threshold persistence FAIL"));
  }

  return ok;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("TMF8828 setThresholds() test"));

  bool overall = true;

  if (!tmf.begin(0x41, &Wire, 400000)) {
    Serial.println(F("begin FAILED"));
    Serial.println(F("FAIL"));
    while (1) {
      delay(10);
    }
  }
  Serial.println(F("begin PASS"));

  if (!tmf.setMode8x8()) {
    Serial.println(F("setMode8x8 FAILED"));
    Serial.println(F("FAIL"));
    while (1) {
      delay(10);
    }
  }
  Serial.println(F("setMode8x8 PASS"));

  if (!runThresholdAcceptanceTest()) {
    overall = false;
  }

  if (!runThresholdWindowTest(F("Threshold behavior test - wide open"), 0, 0,
                              0)) {
    overall = false;
  }

  Serial.println(F(""));
  Serial.println(F("Note: thresholds primarily control interrupts; data may"));
  Serial.println(F("still include all results."));

  if (!runThresholdWindowTest(F("Threshold behavior test - narrow window"), 20,
                              60, 0)) {
    overall = false;
  }

  if (!runPersistenceAcceptanceTest()) {
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
