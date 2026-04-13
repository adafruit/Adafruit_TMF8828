/*!
 * @file 05_configure.ino
 *
 * Hardware test for TMF8828 configure() parameters.
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

static bool runPeriodTest(uint16_t periodMs) {
  const uint8_t frames = 12;
  bool ok = true;

  Serial.println(F(""));
  Serial.print(F("Period test "));
  Serial.print(periodMs);
  Serial.println(F(" ms"));

  if (!tmf.configure(periodMs, 250, 15)) {
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

  uint32_t prev = 0;
  uint32_t sumSub = 0;
  uint8_t subCount = 0;
  uint32_t prevCycle = 0;
  uint32_t sumCycle = 0;
  uint8_t cycleCount = 0;

  for (uint8_t frame = 0; frame < frames; frame++) {
    if (!waitForResult(5000)) {
      Serial.println(F("Timeout waiting for data"));
      ok = false;
      break;
    }

    uint32_t now = millis();

    if (frame > 0) {
      uint32_t delta = now - prev;
      Serial.print(F("Frame "));
      Serial.print(frame);
      Serial.print(F(" delta="));
      Serial.print(delta);
      Serial.println(F(" ms"));
      sumSub += delta;
      subCount++;
    }

    if ((frame % 4) == 0) {
      if (frame > 0) {
        uint32_t cycleDelta = now - prevCycle;
        Serial.print(F("Cycle delta="));
        Serial.print(cycleDelta);
        Serial.println(F(" ms"));
        sumCycle += cycleDelta;
        cycleCount++;
      }
      prevCycle = now;
    }

    prev = now;
  }

  tmf.stopRanging();
  Serial.println(F("stopRanging PASS"));

  if (subCount == 0 || cycleCount == 0) {
    ok = false;
  } else {
    uint32_t avgSub = (sumSub + (subCount / 2)) / subCount;
    uint32_t avgCycle = (sumCycle + (cycleCount / 2)) / cycleCount;
    uint32_t low = periodMs / 2;
    uint32_t high = (periodMs * 3) / 2;

    Serial.print(F("Avg subcapture="));
    Serial.print(avgSub);
    Serial.println(F(" ms"));

    Serial.print(F("Avg full-cycle="));
    Serial.print(avgCycle);
    Serial.print(F(" ms (target "));
    Serial.print(periodMs);
    Serial.println(F(" ms)"));

    if (avgCycle < low || avgCycle > high) {
      ok = false;
    }
  }

  if (ok) {
    Serial.print(F("Period test "));
    Serial.print(periodMs);
    Serial.println(F(" PASS"));
  } else {
    Serial.print(F("Period test "));
    Serial.print(periodMs);
    Serial.println(F(" FAIL"));
  }

  return ok;
}

static bool runIterationsTest(uint16_t kiloIterations, uint8_t frames,
                              uint8_t* avgValidOut, uint8_t* avgConfOut) {
  bool ok = true;
  uint16_t sumValid = 0;
  uint16_t sumConf = 0;
  uint16_t confCount = 0;

  Serial.println(F(""));
  Serial.print(F("Kilo-iterations test "));
  Serial.println(kiloIterations);

  if (!tmf.configure(132, kiloIterations, 15)) {
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

  for (uint8_t frame = 0; frame < frames; frame++) {
    if (!waitForResult(5000)) {
      Serial.println(F("Timeout waiting for data"));
      ok = false;
      break;
    }

    Serial.print(F("Frame "));
    Serial.print(frame);
    Serial.print(F(" ValidResults="));
    Serial.println(result.validResults);

    sumValid += result.validResults;
    for (uint8_t i = 0; i < 36; i++) {
      if (result.results[i].confidence > 0) {
        sumConf += result.results[i].confidence;
        confCount++;
      }
    }
  }

  tmf.stopRanging();
  Serial.println(F("stopRanging PASS"));

  if (!ok) {
    return false;
  }

  uint8_t avgValid = (sumValid + (frames / 2)) / frames;
  uint8_t avgConf = 0;
  if (confCount > 0) {
    avgConf = (sumConf + (confCount / 2)) / confCount;
  }

  *avgValidOut = avgValid;
  *avgConfOut = avgConf;

  Serial.print(F("Avg validResults="));
  Serial.print(avgValid);
  Serial.print(F(" Avg confidence="));
  Serial.println(avgConf);

  if (sumValid == 0) {
    ok = false;
  }

  if (ok) {
    Serial.print(F("Kilo-iterations "));
    Serial.print(kiloIterations);
    Serial.println(F(" PASS"));
  } else {
    Serial.print(F("Kilo-iterations "));
    Serial.print(kiloIterations);
    Serial.println(F(" FAIL"));
  }

  return ok;
}

static bool runSpadMapTest(uint8_t mapId) {
  Serial.println(F(""));
  Serial.print(F("SPAD map ID test "));
  Serial.println(mapId);

  bool ok = tmf.configure(132, 250, mapId);
  if (ok) {
    Serial.println(F("configure returned true"));
    Serial.print(F("SPAD map ID "));
    Serial.print(mapId);
    Serial.println(F(" PASS"));
    return true;
  }

  Serial.println(F("configure returned false"));
  Serial.print(F("SPAD map ID "));
  Serial.print(mapId);
  Serial.println(F(" FAIL"));
  return false;
}

static bool runLegacySpadMapTest(uint8_t mapId) {
  Serial.println(F(""));
  Serial.print(F("Legacy SPAD map ID test "));
  Serial.println(mapId);

  if (!tmf.setModeLegacy()) {
    Serial.println(F("setModeLegacy FAILED"));
    return false;
  }
  Serial.println(F("setModeLegacy PASS"));

  bool ok = tmf.configure(132, 250, mapId);
  if (ok) {
    Serial.println(F("configure returned true"));
  } else {
    Serial.println(F("configure returned false"));
  }

  if (!tmf.setMode8x8()) {
    Serial.println(F("setMode8x8 FAILED"));
    return false;
  }
  Serial.println(F("setMode8x8 PASS"));

  if (ok) {
    Serial.print(F("Legacy SPAD map ID "));
    Serial.print(mapId);
    Serial.println(F(" PASS"));
    return true;
  }

  Serial.print(F("Legacy SPAD map ID "));
  Serial.print(mapId);
  Serial.println(F(" FAIL"));
  return false;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("TMF8828 configure() parameter test"));

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

  if (!runPeriodTest(100)) {
    overall = false;
  }

  if (!runPeriodTest(500)) {
    overall = false;
  }

  uint8_t avgValidLow = 0;
  uint8_t avgConfLow = 0;
  uint8_t avgValidHigh = 0;
  uint8_t avgConfHigh = 0;

  if (!runIterationsTest(50, 4, &avgValidLow, &avgConfLow)) {
    overall = false;
  }

  if (!runIterationsTest(550, 4, &avgValidHigh, &avgConfHigh)) {
    overall = false;
  }

  Serial.println(F(""));
  Serial.println(F("Kilo-iterations comparison"));
  Serial.print(F("Low (50) avgValid="));
  Serial.print(avgValidLow);
  Serial.print(F(" avgConf="));
  Serial.println(avgConfLow);
  Serial.print(F("High (550) avgValid="));
  Serial.print(avgValidHigh);
  Serial.print(F(" avgConf="));
  Serial.println(avgConfHigh);

  if (!runSpadMapTest(15)) {
    overall = false;
  }
  if (!runLegacySpadMapTest(6)) {
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
