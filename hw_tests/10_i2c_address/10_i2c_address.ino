/*!
 * @file 10_i2c_address.ino
 *
 * Hardware test for TMF8828 I2C address change.
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

  Serial.println(F("TMF8828 I2C Address Test"));

  bool overall = true;
  uint32_t defaultSerial = 0;
  uint32_t newSerial = 0;

  Serial.println(F(""));
  Serial.println(F("Step 1: begin(0x41)"));
  // Args: I2C address, Wire bus, I2C speed (Hz)
  if (!tmf.begin(0x41, &Wire, 400000)) {
    halt(F("begin FAILED"));
  }
  Serial.println(F("begin PASS"));

  delay(50);

  Serial.println(F(""));
  Serial.println(F("Step 2: readDeviceInfo at 0x41"));
  bool step2Ok = tmf.readDeviceInfo();
  if (step2Ok) {
    defaultSerial = tmf.getSerialNumber();
    Serial.print(F("Serial: "));
    Serial.println(defaultSerial);
  } else {
    Serial.println(F("readDeviceInfo FAILED"));
  }
  Serial.println(step2Ok ? F("PASS") : F("FAIL"));
  if (!step2Ok) {
    overall = false;
  }

  delay(50);

  Serial.println(F(""));
  Serial.println(F("Step 3: changeI2CAddress(0x42)"));
  bool step3Ok = tmf.changeI2CAddress(0x42);
  Serial.println(step3Ok ? F("PASS") : F("FAIL"));
  if (!step3Ok) {
    overall = false;
  }

  delay(50);

  Serial.println(F(""));
  Serial.println(F("Step 4: readDeviceInfo at 0x42"));
  bool step4Ok = tmf.readDeviceInfo();
  if (step4Ok) {
    newSerial = tmf.getSerialNumber();
    Serial.print(F("Serial: "));
    Serial.println(newSerial);
    Serial.print(F("Serial match: "));
    Serial.println(newSerial == defaultSerial ? F("YES") : F("NO"));
    step4Ok = (newSerial == defaultSerial);
  } else {
    Serial.println(F("readDeviceInfo FAILED"));
  }
  Serial.println(step4Ok ? F("PASS") : F("FAIL"));
  if (!step4Ok) {
    overall = false;
  }

  delay(50);

  Serial.println(F(""));
  Serial.println(F("Step 5: verify old address (0x41) NACKs"));
  Wire.beginTransmission(0x41);
  uint8_t scanResult = Wire.endTransmission();
  Serial.print(F("Wire.endTransmission() = "));
  Serial.println(scanResult);
  bool step5Ok = (scanResult != 0);
  Serial.println(step5Ok ? F("PASS") : F("FAIL"));
  if (!step5Ok) {
    overall = false;
  }

  delay(50);

  Serial.println(F(""));
  Serial.println(F("Step 6: ranging at 0x42"));
  bool step6Ok = true;
  if (!tmf.setMode8x8()) {
    Serial.println(F("setMode8x8 FAILED"));
    step6Ok = false;
  } else {
    Serial.println(F("setMode8x8 PASS"));
  }
  if (step6Ok && !tmf.configure(132, 250, 15)) {
    Serial.println(F("configure FAILED"));
    step6Ok = false;
  } else if (step6Ok) {
    Serial.println(F("configure PASS"));
  }
  if (step6Ok && !tmf.startRanging()) {
    Serial.println(F("startRanging FAILED"));
    step6Ok = false;
  } else if (step6Ok) {
    Serial.println(F("startRanging PASS"));
  }

  if (step6Ok) {
    if (waitForResult(5000)) {
      Serial.print(F("ResultNumber="));
      Serial.print(result.resultNumber);
      Serial.print(F(" ValidResults="));
      Serial.print(result.validResults);
      Serial.print(F(" Temp="));
      Serial.print(result.temperature);
      Serial.print(F(" SysTick="));
      Serial.println(result.sysTick);
      Serial.println(F("dataReady PASS"));
    } else {
      Serial.println(F("Timeout waiting for data"));
      step6Ok = false;
    }
  }

  if (step6Ok) {
    tmf.stopRanging();
    Serial.println(F("stopRanging PASS"));
  } else {
    tmf.stopRanging();
  }

  Serial.println(step6Ok ? F("PASS") : F("FAIL"));
  if (!step6Ok) {
    overall = false;
  }

  delay(50);

  Serial.println(F(""));
  Serial.println(F("Step 7: changeI2CAddress(0x41)"));
  bool step7Ok = tmf.changeI2CAddress(0x41);
  if (!step7Ok) {
    Serial.println(F("changeI2CAddress FAILED"));
  } else if (!tmf.readDeviceInfo()) {
    Serial.println(F("readDeviceInfo FAILED"));
    step7Ok = false;
  } else {
    Serial.print(F("Serial: "));
    Serial.println(tmf.getSerialNumber());
  }
  Serial.println(step7Ok ? F("PASS") : F("FAIL"));
  if (!step7Ok) {
    overall = false;
  }

  delay(50);

  Serial.println(F(""));
  Serial.println(F("Step 8: begin(0x41) reset test"));
  tmf.disable();
  delay(100);
  tmf.enable();
  delay(100);
  bool step8Ok = tmf.begin(0x41, &Wire, 400000);
  if (!step8Ok) {
    Serial.println(F("begin FAILED"));
  } else if (!tmf.readDeviceInfo()) {
    Serial.println(F("readDeviceInfo FAILED"));
    step8Ok = false;
  } else {
    Serial.print(F("Serial: "));
    Serial.println(tmf.getSerialNumber());
  }
  Serial.println(step8Ok ? F("PASS") : F("FAIL"));
  if (!step8Ok) {
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
