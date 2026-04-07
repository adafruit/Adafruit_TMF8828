/*!
 * @file 01_init_alive.ino
 *
 * Hardware test for TMF8828 init and device info.
 *
 * Copyright 2026 Adafruit Industries (Limor 'ladyada' Fried with assistance
 * from Claude Code)
 *
 * MIT License
 */

#include <Adafruit_TMF8828.h>

#define EN_PIN -1

Adafruit_TMF8828 tmf(EN_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("TMF8828 Init Test"));

  if (!tmf.begin(0x41, &Wire, 400000)) {
    halt(F("Failed to find TMF8828"));
  }

  if (!tmf.readDeviceInfo()) {
    halt(F("Failed to read device info"));
  }

  Serial.print(F("FW: "));
  Serial.print(tmf.driver.device.appVersion[0]);
  Serial.print(F("."));
  Serial.print(tmf.driver.device.appVersion[1]);
  Serial.print(F("."));
  Serial.print(tmf.driver.device.appVersion[2]);
  Serial.print(F("."));
  Serial.println(tmf.driver.device.appVersion[3]);

  Serial.print(F("Serial: "));
  Serial.println(tmf.getSerialNumber());

  if (tmf.getSerialNumber() == 0) {
    halt(F("Serial number missing"));
  } else {
    Serial.println(F("PASS"));
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
