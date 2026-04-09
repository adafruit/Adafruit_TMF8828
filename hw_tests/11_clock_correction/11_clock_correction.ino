/*!
 * @file 11_clock_correction.ino
 *
 * Hardware test for TMF8828 clock correction toggle.
 *
 * Copyright 2026 Adafruit Industries (Limor 'ladyada' Fried with assistance
 * from Claude Code)
 *
 * MIT License
 */

#include <Adafruit_TMF8828.h>
#include <Wire.h>

// Set to a GPIO pin to hardware-reset the sensor before init, or -1 to skip
#define TMF8828_EN_PIN 3

Adafruit_TMF8828 tmf(TMF8828_EN_PIN);

static tmf8828_result_t result;

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

static bool runRangingTest(const __FlashStringHelper* label,
                           bool clockCorrection, float& avgDistance,
                           bool& validNonZero) {
  Serial.println(F(""));
  Serial.print(F("Ranging test: "));
  Serial.println(label);

  tmf.setClockCorrection(clockCorrection);
  Serial.print(F("setClockCorrection("));
  Serial.print(clockCorrection ? F("true") : F("false"));
  Serial.println(F(")"));

  if (!tmf.configure(132, 250, 15)) {
    Serial.println(F("configure FAILED"));
    return false;
  }
  Serial.println(F("configure PASS"));

  if (!tmf.startRanging()) {
    Serial.println(F("startRanging FAILED"));
    return false;
  }
  Serial.println(F("startRanging PASS"));

  validNonZero = true;
  float totalSum = 0.0f;
  uint32_t totalCount = 0;

  for (uint8_t frame = 0; frame < 8; frame++) {
    if (!waitForResult(5000)) {
      Serial.println(F("Timeout waiting for data"));
      tmf.stopRanging();
      return false;
    }

    Serial.println(F(""));
    Serial.print(F("Frame "));
    Serial.println(frame);
    Serial.print(F("ValidResults="));
    Serial.print(result.validResults);
    Serial.print(F(" Temp="));
    Serial.print(result.temperature);
    Serial.print(F(" SysTick="));
    Serial.println(result.sysTick);

    if (result.validResults == 0) {
      validNonZero = false;
    }

    float frameSum = 0.0f;
    uint32_t frameCount = 0;
    Serial.println(F("Good zones (confidence > 50):"));
    for (uint8_t i = 0; i < 36; i++) {
      if (result.results[i].confidence > 50 && result.results[i].distance > 0) {
        Serial.print(F("  Z"));
        Serial.print(i);
        Serial.print(F(" dist="));
        Serial.print(result.results[i].distance);
        Serial.print(F(" conf="));
        Serial.println(result.results[i].confidence);
        frameSum += result.results[i].distance;
        frameCount++;
      }
    }

    if (frameCount == 0) {
      Serial.println(F("  (none)"));
    } else {
      float frameAvg = frameSum / frameCount;
      Serial.print(F("Frame average distance="));
      Serial.println(frameAvg, 2);
    }

    totalSum += frameSum;
    totalCount += frameCount;
  }

  tmf.stopRanging();
  Serial.println(F("stopRanging PASS"));

  if (totalCount > 0) {
    avgDistance = totalSum / totalCount;
  } else {
    avgDistance = 0.0f;
  }

  Serial.print(F("Overall average distance (good zones)="));
  Serial.println(avgDistance, 2);

  if (validNonZero) {
    Serial.println(F("validResults nonzero PASS"));
  } else {
    Serial.println(F("validResults nonzero FAIL"));
  }

  return validNonZero;
}

static bool quickRange(const __FlashStringHelper* label) {
  Serial.println(F(""));
  Serial.print(F("Quick range: "));
  Serial.println(label);

  if (!tmf.configure(132, 250, 15)) {
    Serial.println(F("configure FAILED"));
    return false;
  }
  if (!tmf.startRanging()) {
    Serial.println(F("startRanging FAILED"));
    return false;
  }
  if (!waitForResult(5000)) {
    Serial.println(F("Timeout waiting for data"));
    tmf.stopRanging();
    return false;
  }
  tmf.stopRanging();
  Serial.print(F("ValidResults="));
  Serial.println(result.validResults);
  Serial.println(F("quick range PASS"));
  return true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("TMF8828 Clock Correction Test"));

  bool overall = true;

  Serial.println(F(""));
  Serial.println(F("Step 1: begin + setMode8x8"));
  // Args: I2C address, Wire bus, I2C speed (Hz)
  if (!tmf.begin(0x41, &Wire, 400000)) {
    halt(F("begin FAILED"));
  }
  if (!tmf.setMode8x8()) {
    halt(F("setMode8x8 FAILED"));
  }
  Serial.println(F("begin PASS"));
  Serial.println(F("setMode8x8 PASS"));

  float avgOn = 0.0f;
  float avgOff = 0.0f;
  bool validOn = false;
  bool validOff = false;

  Serial.println(F(""));
  Serial.println(F("Step 2: clock correction ON"));
  bool step2Ok = runRangingTest(F("Clock correction ON"), true, avgOn, validOn);
  Serial.println(step2Ok ? F("PASS") : F("FAIL"));
  if (!step2Ok) {
    overall = false;
  }

  delay(100);

  Serial.println(F(""));
  Serial.println(F("Step 3: clock correction OFF"));
  bool step3Ok =
      runRangingTest(F("Clock correction OFF"), false, avgOff, validOff);
  Serial.println(step3Ok ? F("PASS") : F("FAIL"));
  if (!step3Ok) {
    overall = false;
  }

  Serial.println(F(""));
  Serial.println(F("Step 4: compare averages (informational)"));
  Serial.print(F("Average ON = "));
  Serial.println(avgOn, 2);
  Serial.print(F("Average OFF = "));
  Serial.println(avgOff, 2);
  Serial.println(F("Comparison is informational only"));

  if (validOn && validOff) {
    Serial.println(F("validResults nonzero in both modes PASS"));
  } else {
    Serial.println(F("validResults nonzero in both modes FAIL"));
    overall = false;
  }

  Serial.println(F(""));
  Serial.println(F("Step 5: toggle test"));
  bool toggleOk = true;

  tmf.setClockCorrection(true);
  toggleOk &= quickRange(F("toggle to ON"));

  tmf.setClockCorrection(false);
  toggleOk &= quickRange(F("toggle to OFF"));

  tmf.setClockCorrection(true);
  toggleOk &= quickRange(F("toggle back to ON"));

  Serial.println(toggleOk ? F("PASS") : F("FAIL"));
  if (!toggleOk) {
    overall = false;
  }

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
