/*!
 * @file 03_ranging_8x8.ino
 *
 * Hardware test for TMF8828 8x8 ranging data integrity.
 *
 * Copyright 2026 Adafruit Industries (Limor 'ladyada' Fried with assistance
 * from Claude Code)
 *
 * MIT License
 */

#include <Adafruit_TMF8828.h>

// Set to a GPIO pin to hardware-reset the sensor before init, or -1 to skip
#define TMF8828_EN_PIN 3

Adafruit_TMF8828 tmf(TMF8828_EN_PIN);

// Single result buffer — no arrays (AVR RAM is tight)
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

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("TMF8828 8x8 Ranging Test"));

  bool overall = true;

  if (!tmf.begin(0x41, &Wire, 400000)) {
    halt(F("begin FAILED"));
  }
  Serial.println(F("begin PASS"));

  if (!tmf.setMode8x8()) {
    halt(F("setMode8x8 FAILED"));
  }
  Serial.println(F("setMode8x8 PASS"));

  if (!tmf.configure(132, 250, 15)) {
    halt(F("configure FAILED"));
  }
  Serial.println(F("configure PASS"));

  tmf.clearAndEnableInterrupts(TMF8828_APP_I2C_RESULT_IRQ_MASK);

  if (!tmf.startRanging()) {
    halt(F("startRanging FAILED"));
  }
  Serial.println(F("startRanging PASS"));

  const uint8_t frames = 12;
  bool seenResult[4] = {false, false, false, false};
  bool validNonZero = true;
  bool tempRange = true;
  bool sysTickIncreasing = true;
  bool zoneHasHit = false;
  uint32_t lastSysTick = 0;

  for (uint8_t frame = 0; frame < frames; frame++) {
    if (!waitForResult(5000)) {
      Serial.println(F("Timeout waiting for data"));
      overall = false;
      break;
    }

    uint8_t subcapture = result.resultNumber & 0x03;
    seenResult[subcapture] = true;

    Serial.print(F("Frame "));
    Serial.print(frame);
    Serial.print(F(" RN="));
    Serial.print(result.resultNumber);
    Serial.print(F(" Sub="));
    Serial.print(subcapture);
    Serial.print(F(" Valid="));
    Serial.print(result.validResults);
    Serial.print(F(" T="));
    Serial.print(result.temperature);
    Serial.print(F(" Tick="));
    Serial.println(result.sysTick);

    if (result.validResults == 0) {
      validNonZero = false;
    }

    if (result.temperature < 10 || result.temperature > 80) {
      tempRange = false;
    }

    if (frame > 0 && result.sysTick <= lastSysTick) {
      sysTickIncreasing = false;
    }
    lastSysTick = result.sysTick;

    for (uint8_t i = 0; i < 36; i++) {
      if (result.results[i].confidence > 0 && result.results[i].distance > 0 &&
          result.results[i].distance < 5000) {
        zoneHasHit = true;
      }
    }
  }

  tmf.stopRanging();
  Serial.println(F("stopRanging PASS"));

  bool seenAll =
      seenResult[0] && seenResult[1] && seenResult[2] && seenResult[3];

  if (seenAll) {
    Serial.println(F("resultNumber coverage PASS"));
  } else {
    Serial.println(F("resultNumber coverage FAIL"));
    overall = false;
  }

  if (validNonZero) {
    Serial.println(F("validResults nonzero PASS"));
  } else {
    Serial.println(F("validResults nonzero FAIL"));
    overall = false;
  }

  if (tempRange) {
    Serial.println(F("temperature range PASS"));
  } else {
    Serial.println(F("temperature range FAIL"));
    overall = false;
  }

  if (sysTickIncreasing) {
    Serial.println(F("sysTick increasing PASS"));
  } else {
    Serial.println(F("sysTick increasing FAIL"));
    overall = false;
  }

  if (zoneHasHit) {
    Serial.println(F("zone distance/confidence PASS"));
  } else {
    Serial.println(F("zone distance/confidence FAIL"));
    overall = false;
  }

  if (overall) {
    Serial.println(F("\nPASS"));
  } else {
    Serial.println(F("\nFAIL"));
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
