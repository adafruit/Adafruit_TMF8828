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

#define EN_PIN 27

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

static void printPaddedDistance(uint16_t distance) {
  if (distance < 10) {
    Serial.print(F("   "));
  } else if (distance < 100) {
    Serial.print(F("  "));
  } else if (distance < 1000) {
    Serial.print(F(" "));
  }
  Serial.print(distance);
}

static void printZones(const tmf8828_result_t& data) {
  for (uint8_t i = 0; i < 36; i++) {
    Serial.print(F("Zone "));
    Serial.print(i);
    Serial.print(F(": "));
    Serial.print(data.results[i].distance);
    Serial.print(F(" mm  Conf="));
    Serial.println(data.results[i].confidence);
  }
}

static void printGrid(const tmf8828_result_t& data) {
  for (uint8_t row = 0; row < 6; row++) {
    Serial.print(F("  "));
    for (uint8_t col = 0; col < 6; col++) {
      uint8_t idx = row * 6 + col;
      printPaddedDistance(data.results[idx].distance);
      if (col < 5) {
        Serial.print(F(" "));
      }
    }
    Serial.println();
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("TMF8828 8x8 Ranging Test"));

  bool overall = true;

  if (!tmf.begin(0x41)) {
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

  if (!tmf.configure(132, 250, 15)) {
    Serial.println(F("configure FAILED"));
    Serial.println(F("FAIL"));
    while (1) {
      delay(10);
    }
  }
  Serial.println(F("configure PASS"));

  tmf.clearAndEnableInterrupts(TMF8828_APP_I2C_RESULT_IRQ_MASK);

  if (!tmf.startRanging()) {
    Serial.println(F("startRanging FAILED"));
    Serial.println(F("FAIL"));
    while (1) {
      delay(10);
    }
  }
  Serial.println(F("startRanging PASS"));

  const uint8_t frames = 12;
  bool seenResult[4] = {false, false, false, false};
  bool validNonZero = true;
  bool tempRange = true;
  bool sysTickIncreasing = true;
  bool zoneHasHit = false;
  uint32_t lastSysTick = 0;

  tmf8828_result_t captures[4];
  bool haveCapture[4] = {false, false, false, false};

  for (uint8_t frame = 0; frame < frames; frame++) {
    if (!waitForResult(5000)) {
      Serial.println(F("Timeout waiting for data"));
      overall = false;
      break;
    }

    Serial.println(F(""));
    Serial.print(F("Frame "));
    Serial.println(frame);

    uint8_t subcapture = result.resultNumber & 0x03;

    Serial.print(F("ResultNumber="));
    Serial.print(result.resultNumber);
    Serial.print(F(" Subcapture="));
    Serial.print(subcapture);
    Serial.print(F(" ValidResults="));
    Serial.print(result.validResults);
    Serial.print(F(" Temp="));
    Serial.print(result.temperature);
    Serial.print(F(" SysTick="));
    Serial.println(result.sysTick);

    seenResult[subcapture] = true;
    if (!haveCapture[subcapture]) {
      captures[subcapture] = result;
      haveCapture[subcapture] = true;
    }

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

    printZones(result);
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

  bool haveAllCaptures =
      haveCapture[0] && haveCapture[1] && haveCapture[2] && haveCapture[3];

  if (haveAllCaptures) {
    Serial.println(F("subcapture set PASS"));
  } else {
    Serial.println(F("subcapture set FAIL"));
    overall = false;
  }

  if (haveAllCaptures) {
    Serial.println(F(""));
    Serial.println(F("Subcapture Distance Grids (6x6 each)"));
    for (uint8_t idx = 0; idx < 4; idx++) {
      Serial.print(F("Subcapture "));
      Serial.println(idx);
      printGrid(captures[idx]);
      Serial.println();
    }
  }

  if (overall) {
    Serial.println(F("PASS"));
  } else {
    Serial.println(F("FAIL"));
  }
}

void loop() {}
