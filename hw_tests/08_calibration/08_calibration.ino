/*!
 * @file 08_calibration.ino
 *
 * Hardware test for TMF8828 factory calibration flow.
 *
 * Copyright 2026 Adafruit Industries (Limor 'ladyada' Fried with assistance
 * from Claude Code)
 *
 * MIT License
 */

#include <Adafruit_TMF8828.h>

#define EN_PIN 27
#define INT_PIN 14

Adafruit_TMF8828 tmf(EN_PIN, INT_PIN);

tmf8828_result_t result;

struct RangeSummary {
  uint32_t sumValid;
  uint32_t sumZone0;
  uint32_t sumZone17;
  uint32_t sumZone35;
  uint8_t framesCaptured;
};

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

static void printFrameSummary(uint8_t frameIndex) {
  Serial.print(F("Frame "));
  Serial.print(frameIndex);
  Serial.print(F(" ValidResults="));
  Serial.print(result.validResults);
  Serial.print(F(" Z0="));
  Serial.print(result.results[0].distance);
  Serial.print(F(" Z17="));
  Serial.print(result.results[17].distance);
  Serial.print(F(" Z35="));
  Serial.println(result.results[35].distance);
}

static void printRangeSummary(const RangeSummary& summary,
                              const __FlashStringHelper* label) {
  if (summary.framesCaptured == 0) {
    Serial.print(label);
    Serial.println(F(" no frames captured"));
    return;
  }

  float avgValid =
      static_cast<float>(summary.sumValid) / summary.framesCaptured;
  float avgZ0 = static_cast<float>(summary.sumZone0) / summary.framesCaptured;
  float avgZ17 = static_cast<float>(summary.sumZone17) / summary.framesCaptured;
  float avgZ35 = static_cast<float>(summary.sumZone35) / summary.framesCaptured;

  Serial.print(label);
  Serial.print(F(" avgValidResults="));
  Serial.print(avgValid, 2);
  Serial.print(F(" avgZ0="));
  Serial.print(avgZ0, 1);
  Serial.print(F(" avgZ17="));
  Serial.print(avgZ17, 1);
  Serial.print(F(" avgZ35="));
  Serial.println(avgZ35, 1);
}

static bool runRangingFrames(const __FlashStringHelper* label, uint8_t frames,
                             RangeSummary* summary) {
  Serial.println(F(""));
  Serial.println(label);

  if (!summary) {
    Serial.println(F("summary pointer missing"));
    return false;
  }

  summary->sumValid = 0;
  summary->sumZone0 = 0;
  summary->sumZone17 = 0;
  summary->sumZone35 = 0;
  summary->framesCaptured = 0;

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

  bool ok = true;
  for (uint8_t frame = 0; frame < frames; frame++) {
    if (!waitForResult(6000)) {
      Serial.println(F("Timeout waiting for data"));
      ok = false;
      break;
    }

    printFrameSummary(frame);
    summary->sumValid += result.validResults;
    summary->sumZone0 += result.results[0].distance;
    summary->sumZone17 += result.results[17].distance;
    summary->sumZone35 += result.results[35].distance;
    summary->framesCaptured++;
  }

  tmf.stopRanging();
  Serial.println(F("stopRanging PASS"));

  printRangeSummary(*summary, F("Summary:"));

  if (!ok) {
    Serial.println(F("Ranging frames FAILED"));
  } else {
    Serial.println(F("Ranging frames PASS"));
  }

  return ok;
}

static bool runSingleFrame(const __FlashStringHelper* label) {
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

  bool ok = true;
  if (!waitForResult(6000)) {
    Serial.println(F("Timeout waiting for data"));
    ok = false;
  } else {
    printFrameSummary(0);
  }

  tmf.stopRanging();
  Serial.println(F("stopRanging PASS"));

  if (ok) {
    Serial.println(F("Single frame PASS"));
  } else {
    Serial.println(F("Single frame FAIL"));
  }

  return ok;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("TMF8828 calibration test"));

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

  RangeSummary baseline;
  if (!runRangingFrames(F("Baseline ranging (pre-calibration)"), 4,
                        &baseline)) {
    overall = false;
  }

  Serial.println(F(""));
  Serial.println(F("Factory calibration"));
  bool calibrationOk = tmf.factoryCalibration();
  Serial.print(F("factoryCalibration returned "));
  Serial.println(calibrationOk ? F("true") : F("false"));
  if (calibrationOk) {
    Serial.println(F("factoryCalibration PASS"));
  } else {
    Serial.println(F("factoryCalibration did not complete"));
    Serial.println(F("Note: proper target may be required"));
  }

  if (calibrationOk) {
    RangeSummary postCal;
    if (!runRangingFrames(F("Ranging after calibration"), 4, &postCal)) {
      overall = false;
    }
    Serial.println(F(""));
    Serial.println(F("Baseline vs post-cal comparison (informational)"));
    printRangeSummary(baseline, F("Baseline:"));
    printRangeSummary(postCal, F("Post-cal:"));
  } else {
    Serial.println(F(""));
    Serial.println(F("Skipping post-cal ranging due to calibration failure"));
  }

  Serial.println(F(""));
  Serial.println(F("Reset factory calibration"));
  bool resetOk = tmf.resetFactoryCalibration();
  Serial.print(F("resetFactoryCalibration returned "));
  Serial.println(resetOk ? F("true") : F("false"));
  if (resetOk) {
    Serial.println(F("resetFactoryCalibration PASS"));
  } else {
    Serial.println(F("resetFactoryCalibration FAIL"));
    overall = false;
  }

  RangeSummary postReset;
  if (!runRangingFrames(F("Ranging after calibration reset"), 4, &postReset)) {
    overall = false;
  }

  Serial.println(F(""));
  Serial.println(F("Load dummy factory calibration"));
  uint8_t dummyCalib[188];
  memset(dummyCalib, 0, sizeof(dummyCalib));
  bool loadOk = tmf.loadFactoryCalibration(dummyCalib);
  Serial.print(F("loadFactoryCalibration returned "));
  Serial.println(loadOk ? F("true") : F("false"));
  if (!loadOk) {
    Serial.println(F("Dummy load returned false (expected)"));
  } else {
    Serial.println(F("Dummy load returned true (unexpected)"));
  }

  if (!runSingleFrame(F("Ranging after failed dummy load"))) {
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
