/*!
 * @file 02_mode_switch.ino
 *
 * Hardware test for TMF8828 mode switching between 8x8 and legacy.
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

static bool doRangingTest(const __FlashStringHelper* label, bool expect8x8,
                          uint8_t* outAvgValid) {
  const uint8_t frames = 6;
  bool ok = true;
  uint16_t sumValid = 0;
  uint8_t minValid = 0xFF;
  uint8_t maxValid = 0;

  Serial.print(label);
  Serial.print(F(": "));

  uint8_t spadMap = expect8x8 ? 15 : 6;
  if (!tmf.configure(132, 250, spadMap)) {
    Serial.println(F("configure FAILED"));
    return false;
  }

  tmf.clearAndEnableInterrupts(TMF8828_APP_I2C_RESULT_IRQ_MASK);

  if (!tmf.startRanging()) {
    Serial.println(F("startRanging FAILED"));
    return false;
  }

  Serial.print(F("validResults="));
  for (uint8_t i = 0; i < frames; i++) {
    if (!waitForResult(5000)) {
      Serial.println(F("no data"));
      tmf.stopRanging();
      return false;
    }

    if (i > 0) {
      Serial.print(F(","));
    }
    Serial.print(result.validResults);

    sumValid += result.validResults;
    if (result.validResults < minValid) {
      minValid = result.validResults;
    }
    if (result.validResults > maxValid) {
      maxValid = result.validResults;
    }
  }
  Serial.println();

  tmf.stopRanging();

  uint8_t avgValid = (sumValid + (frames / 2)) / frames;
  *outAvgValid = avgValid;

  Serial.print(F("validResults min="));
  Serial.print(minValid);
  Serial.print(F(" max="));
  Serial.print(maxValid);
  Serial.print(F(" avg="));
  Serial.println(avgValid);

  if (expect8x8) {
    if (avgValid < 16) {
      ok = false;
    }
  } else {
    if (avgValid > 16) {
      ok = false;
    }
  }

  if (ok) {
    Serial.println(F("PASS"));
  } else {
    Serial.println(F("FAIL"));
  }

  return ok;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("TMF8828 Mode Switch Test"));

  bool overall = true;

  if (!tmf.begin(0x41, &Wire, 400000)) {
    halt(F("begin FAILED"));
  }
  Serial.println(F("begin PASS"));

  if (tmf.isMode8x8()) {
    Serial.println(F("mode after begin (8x8) PASS"));
  } else {
    Serial.println(F("mode after begin (8x8) FAIL"));
    overall = false;
  }

  if (tmf.setModeLegacy()) {
    Serial.println(F("setModeLegacy PASS"));
  } else {
    Serial.println(F("setModeLegacy FAIL"));
    overall = false;
  }

  if (!tmf.isMode8x8()) {
    Serial.println(F("mode legacy verify PASS"));
  } else {
    Serial.println(F("mode legacy verify FAIL"));
    overall = false;
  }

  if (tmf.setMode8x8()) {
    Serial.println(F("setMode8x8 PASS"));
  } else {
    Serial.println(F("setMode8x8 FAIL"));
    overall = false;
  }

  if (tmf.isMode8x8()) {
    Serial.println(F("mode 8x8 verify PASS"));
  } else {
    Serial.println(F("mode 8x8 verify FAIL"));
    overall = false;
  }

  uint8_t avgValid8x8 = 0;
  uint8_t avgValidLegacy = 0;

  if (!doRangingTest(F("Ranging 8x8"), true, &avgValid8x8)) {
    overall = false;
  }

  if (tmf.setModeLegacy()) {
    Serial.println(F("setModeLegacy (after ranging) PASS"));
  } else {
    Serial.println(F("setModeLegacy (after ranging) FAIL"));
    overall = false;
  }

  if (!tmf.isMode8x8()) {
    Serial.println(F("mode legacy verify 2 PASS"));
  } else {
    Serial.println(F("mode legacy verify 2 FAIL"));
    overall = false;
  }

  if (!doRangingTest(F("Ranging legacy"), false, &avgValidLegacy)) {
    overall = false;
  }

  if (avgValid8x8 <= avgValidLegacy) {
    Serial.println(F("mode structural compare FAIL"));
    overall = false;
  } else {
    Serial.println(F("mode structural compare PASS"));
  }

  if (tmf.setMode8x8()) {
    Serial.println(F("setMode8x8 (after legacy) PASS"));
  } else {
    Serial.println(F("setMode8x8 (after legacy) FAIL"));
    overall = false;
  }

  if (tmf.isMode8x8()) {
    Serial.println(F("mode 8x8 verify 2 PASS"));
  } else {
    Serial.println(F("mode 8x8 verify 2 FAIL"));
    overall = false;
  }

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
