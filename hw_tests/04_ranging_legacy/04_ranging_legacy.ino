/*!
 * @file 04_ranging_legacy.ino
 *
 * Hardware test for TMF8828 legacy ranging across SPAD maps.
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

static bool runMapTest(uint8_t mapId, uint8_t* avgValidOut) {
  const uint8_t frames = 4;
  bool ok = true;
  uint16_t sumValid = 0;
  uint8_t minValid = 0xFF;
  uint8_t maxValid = 0;
  uint8_t avgValid = 0;

  Serial.println(F(""));
  Serial.print(F("=== SPAD map "));
  Serial.print(mapId);
  Serial.println(F(" ==="));

  if (!tmf.configure(132, 250, mapId)) {
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
    Serial.println(frame);
    Serial.print(F("ResultNumber="));
    Serial.print(result.resultNumber);
    Serial.print(F(" ValidResults="));
    Serial.print(result.validResults);
    Serial.print(F(" Temp="));
    Serial.println(result.temperature);

    if (frame == 0) {
      printZones(result);
    }

    if (result.validResults == 0) {
      ok = false;
    }

    if (result.temperature < 10 || result.temperature > 80) {
      ok = false;
    }

    for (uint8_t i = 0; i < 36; i++) {
      if (result.results[i].confidence > 0 &&
          result.results[i].distance > 5000) {
        ok = false;
      }
    }

    sumValid += result.validResults;
    if (result.validResults < minValid) {
      minValid = result.validResults;
    }
    if (result.validResults > maxValid) {
      maxValid = result.validResults;
    }
  }

  tmf.stopRanging();
  Serial.println(F("stopRanging PASS"));

  avgValid = (sumValid + (frames / 2)) / frames;
  *avgValidOut = avgValid;

  Serial.print(F("validResults min="));
  Serial.print(minValid);
  Serial.print(F(" max="));
  Serial.print(maxValid);
  Serial.print(F(" avg="));
  Serial.println(avgValid);

  if (ok) {
    Serial.print(F("SPAD map "));
    Serial.print(mapId);
    Serial.println(F(" PASS"));
  } else {
    Serial.print(F("SPAD map "));
    Serial.print(mapId);
    Serial.println(F(" FAIL"));
  }

  return ok;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("TMF8828 Legacy Ranging SPAD Map Test"));

  bool overall = true;

  // Args: I2C address, Wire bus, I2C speed (Hz)
  if (!tmf.begin(0x41, &Wire, 400000)) {
    halt(F("begin FAILED"));
  }
  Serial.println(F("begin PASS"));

  if (!tmf.setModeLegacy()) {
    halt(F("setModeLegacy FAILED"));
  }
  Serial.println(F("setModeLegacy PASS"));

  const uint8_t mapIds[3] = {6, 7, 1};
  uint8_t avgValid[3] = {0, 0, 0};

  for (uint8_t i = 0; i < 3; i++) {
    if (!runMapTest(mapIds[i], &avgValid[i])) {
      overall = false;
    }
  }

  Serial.println(F(""));
  Serial.println(F("validResults comparison"));
  Serial.print(F("Map 6 avg="));
  Serial.print(avgValid[0]);
  Serial.print(F(" Map 7 avg="));
  Serial.print(avgValid[1]);
  Serial.print(F(" Map 1 avg="));
  Serial.println(avgValid[2]);

  bool diff = !((avgValid[0] == avgValid[1]) && (avgValid[1] == avgValid[2]));
  if (diff) {
    Serial.println(F("validResults comparison NOTE: differences observed"));
  } else {
    Serial.println(F("validResults comparison NOTE: no differences observed"));
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
