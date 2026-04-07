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

// Set to a GPIO pin to hardware-reset the sensor before init, or -1 to skip
#define EN_PIN -1

Adafruit_TMF8828 tmf(EN_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("TMF8828 Init Test"));

  // Args: I2C address, Wire bus, I2C speed (Hz)
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
  }

  Serial.println(F("First begin() OK"));

  // Call begin() again to verify re-init works without a power cycle.
  // This simulates what happens when the MCU resets but the sensor stays
  // powered — the driver must soft-reset back to bootloader and re-upload FW.
  Serial.println(F("Re-calling begin() (soft reset test)..."));
  if (!tmf.begin(0x41, &Wire, 400000)) {
    halt(
        F("Second begin() FAILED - sensor cannot re-init without power cycle"));
  }

  if (!tmf.readDeviceInfo()) {
    halt(F("Second readDeviceInfo FAILED"));
  }

  Serial.print(F("Serial after re-init: "));
  Serial.println(tmf.getSerialNumber());

  Serial.println(F("PASS"));
}

void loop() {}

void halt(const __FlashStringHelper* msg) {
  Serial.println(msg);
  Serial.println(F("FAIL"));
  while (1) {
    delay(10);
  }
}
