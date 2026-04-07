/*!
 * @file Adafruit_TMF8828.cpp
 *
 * Arduino library for the ams-OSRAM TMF8828 8x8 multizone dToF sensor.
 *
 * Copyright 2026 Adafruit Industries (Limor 'ladyada' Fried with assistance
 * from Claude Code)
 *
 * MIT License
 */

#include "Adafruit_TMF8828.h"

#include <string.h>

#include "tmf8828_image.h"
#include "tmf8828_shim.h"
#include "tmf882x_image.h"

Adafruit_TMF8828::Adafruit_TMF8828(int8_t enPin)
    : _i2c(nullptr),
      _wire(&Wire),
      _enPin(enPin),
      _is8x8(true),
      _periodMs(132),
      _kiloIterations(250),
      _spadMapId(15),
      _lowThreshold(0),
      _highThreshold(0),
      _persistence(0),
      _intMask(0),
      _dumpHistogram(0) {}

bool Adafruit_TMF8828::begin(uint8_t addr, TwoWire* wire, uint32_t i2cSpeed) {
  _wire = wire ? wire : &Wire;

  if (_i2c) {
    delete _i2c;
    _i2c = nullptr;
  }

  _i2c = new Adafruit_I2CDevice(addr, _wire);
  if (!_i2c->begin()) {
    delete _i2c;
    _i2c = nullptr;
    return false;
  }
  _i2c->setSpeed(i2cSpeed);

  tmf8828_i2c_device = _i2c;
  tmf8828_enable_pin = _enPin;
  configurePins(nullptr);

  tmf8828Initialise(&driver);
  tmf8828SetLogLevel(&driver, TMF8828_LOG_LEVEL_ERROR);

  // Force the sensor back to ROM bootloader even if it is already running
  // application firmware (e.g. after an MCU reset without power-cycling the
  // sensor). Write ENABLE register with cpu_reset=1 and PON=1.
  forceReset();

  tmf8828Enable(&driver);
  delayInMicroseconds(ENABLE_TIME_MS * 1000UL);
  tmf8828ClkCorrection(&driver, 1);
  tmf8828Wakeup(&driver);
  if (!tmf8828IsCpuReady(&driver, CPU_READY_TIME_MS)) {
    return false;
  }

  if (!downloadFirmware(tmf8828_image, (uint32_t)tmf8828_image_start,
                        (int32_t)tmf8828_image_length)) {
    return false;
  }

  _is8x8 = true;
  _subcaptureMask = 0;
  tmf8828_last_result_valid = false;
  return true;
}

void Adafruit_TMF8828::forceReset() {
  // Write ENABLE register (0xE0) with cpu_reset (bit 7) and PON (bit 0) set.
  // This forces the CPU back to ROM bootloader regardless of current state.
  // Clear powerup_select bits so the chip boots into boot monitor, not RAM app.
  uint8_t buf[2] = {0xE0, 0x81}; // reg addr, value: cpu_reset | PON
  _i2c->write(buf, 2);
  delay(5); // wait for reset to take effect
  // Clear cpu_reset, keep PON
  buf[1] = 0x01; // PON only
  _i2c->write(buf, 2);
  delay(ENABLE_TIME_MS);
}

bool Adafruit_TMF8828::downloadFirmware(const uint8_t* image, uint32_t start,
                                        int32_t length) {
  return tmf8828DownloadFirmware(&driver, start, image, length) ==
         BL_SUCCESS_OK;
}

bool Adafruit_TMF8828::setMode8x8() {
  if (tmf8828SwitchTo8x8Mode(&driver) != APP_SUCCESS_OK) {
    return false;
  }
  _is8x8 = true;
  return true;
}

bool Adafruit_TMF8828::setModeLegacy() {
  if (tmf8828SwitchToLegacyMode(&driver) != APP_SUCCESS_OK) {
    return false;
  }
  _is8x8 = false;
  return true;
}

bool Adafruit_TMF8828::isMode8x8() {
  return _is8x8;
}

bool Adafruit_TMF8828::configure(uint16_t periodMs, uint16_t kiloIterations,
                                 uint8_t spadMapId) {
  _periodMs = periodMs;
  _kiloIterations = kiloIterations;
  _spadMapId = spadMapId;

  return tmf8828Configure(&driver, _periodMs, _kiloIterations, _spadMapId,
                          _lowThreshold, _highThreshold, _persistence, _intMask,
                          _dumpHistogram) == APP_SUCCESS_OK;
}

bool Adafruit_TMF8828::setThresholds(uint16_t low, uint16_t high,
                                     uint8_t persistence) {
  _lowThreshold = low;
  _highThreshold = high;
  _persistence = persistence;

  return tmf8828Configure(&driver, _periodMs, _kiloIterations, _spadMapId,
                          _lowThreshold, _highThreshold, _persistence, _intMask,
                          _dumpHistogram) == APP_SUCCESS_OK;
}

bool Adafruit_TMF8828::setInterruptMask(uint32_t mask) {
  _intMask = mask;
  return tmf8828Configure(&driver, _periodMs, _kiloIterations, _spadMapId,
                          _lowThreshold, _highThreshold, _persistence, _intMask,
                          _dumpHistogram) == APP_SUCCESS_OK;
}

bool Adafruit_TMF8828::startRanging() {
  return tmf8828StartMeasurement(&driver) == APP_SUCCESS_OK;
}

bool Adafruit_TMF8828::stopRanging() {
  return tmf8828StopMeasurement(&driver) == APP_SUCCESS_OK;
}

bool Adafruit_TMF8828::dataReady() {
  return tmf8828GetAndClrInterrupts(&driver, TMF8828_APP_I2C_RESULT_IRQ_MASK) !=
         0;
}

bool Adafruit_TMF8828::getRangingData(tmf8828_result_t* result) {
  if (!result) {
    return false;
  }
  if (tmf8828ReadResults(&driver) != APP_SUCCESS_OK) {
    return false;
  }
  if (!tmf8828_last_result_valid) {
    return false;
  }

  memcpy(result, &tmf8828_last_result, sizeof(tmf8828_result_t));
  tmf8828_last_result_valid = false;
  return true;
}

bool Adafruit_TMF8828::readFrame(tmf8828_frame_t* frame) {
  if (!frame) {
    return false;
  }

  // Poll for data
  if (!dataReady()) {
    return false;
  }

  tmf8828_result_t res;
  if (!getRangingData(&res)) {
    return false;
  }

  uint8_t sub = res.resultNumber & 0x03;

  // On first subcapture of a new frame, clear the accumulator
  if (_subcaptureMask == 0) {
    memset(&_frame, 0, sizeof(_frame));
  }

  // Each subcapture has 36 results in 4 groups of 9. The first entry of each
  // group (indices 0, 9, 18, 27) is a reference channel. Stripping those gives
  // 32 real zones: first 16 = object 0, second 16 = object 1. We use object 0.
  // The 16 zones per subcapture map to specific 8x8 positions via a
  // checkerboard-like interleave (from ams-OSRAM TMF8828 zone mapping).
  static const uint8_t PROGMEM zoneMap[4][16] = {
      {8, 9, 12, 13, 24, 25, 28, 29, 40, 41, 44, 45, 56, 57, 60, 61},
      {10, 11, 14, 15, 26, 27, 30, 31, 42, 43, 46, 47, 58, 59, 62, 63},
      {0, 1, 4, 5, 16, 17, 20, 21, 32, 33, 36, 37, 48, 49, 52, 53},
      {2, 3, 6, 7, 18, 19, 22, 23, 34, 35, 38, 39, 50, 51, 54, 55},
  };

  uint8_t zoneIdx = 0;
  for (uint8_t i = 0; i < 36; i++) {
    if ((i % 9) == 0) {
      continue; // skip reference channel
    }
    if (zoneIdx < 16) {
      uint8_t gridIdx = pgm_read_byte(&zoneMap[sub][zoneIdx]);
      _frame.distances[gridIdx / 8][gridIdx % 8] = res.results[i].distance;
      _frame.confidences[gridIdx / 8][gridIdx % 8] = res.results[i].confidence;
    }
    zoneIdx++;
  }
  _frame.temperature = res.temperature;
  _subcaptureMask |= (1 << sub);

  // Return true only when all 4 subcaptures are collected
  if (_subcaptureMask != 0x0F) {
    return false;
  }

  memcpy(frame, &_frame, sizeof(tmf8828_frame_t));
  _subcaptureMask = 0;
  return true;
}

bool Adafruit_TMF8828::factoryCalibration() {
  return tmf8828FactoryCalibration(&driver) == APP_SUCCESS_OK;
}

bool Adafruit_TMF8828::resetFactoryCalibration() {
  return tmf8828ResetFactoryCalibration(&driver) == APP_SUCCESS_OK;
}

bool Adafruit_TMF8828::loadFactoryCalibration(const uint8_t* calibData) {
  if (!calibData) {
    return false;
  }
  return tmf8828SetStoredFactoryCalibration(&driver, calibData) ==
         APP_SUCCESS_OK;
}

bool Adafruit_TMF8828::standby() {
  tmf8828Standby(&driver);
  return true;
}

bool Adafruit_TMF8828::wakeup() {
  tmf8828Wakeup(&driver);
  return tmf8828IsCpuReady(&driver, CPU_READY_TIME_MS);
}

bool Adafruit_TMF8828::enable() {
  tmf8828Enable(&driver);
  delayInMicroseconds(ENABLE_TIME_MS * 1000UL);
  return true;
}

bool Adafruit_TMF8828::disable() {
  tmf8828Disable(&driver);
  delayInMicroseconds(CAP_DISCHARGE_TIME_MS * 1000UL);
  return true;
}

bool Adafruit_TMF8828::changeI2CAddress(uint8_t newAddr) {
  if (!_i2c) {
    return false;
  }
  if (tmf8828ChangeI2CAddress(&driver, newAddr) != APP_SUCCESS_OK) {
    return false;
  }

  _i2c = tmf8828_i2c_device;
  return _i2c != nullptr;
}

bool Adafruit_TMF8828::readDeviceInfo() {
  return tmf8828ReadDeviceInfo(&driver) == APP_SUCCESS_OK;
}

uint32_t Adafruit_TMF8828::getSerialNumber() {
  return driver.device.deviceSerialNumber;
}

void Adafruit_TMF8828::setClockCorrection(bool enabled) {
  tmf8828ClkCorrection(&driver, enabled ? 1 : 0);
}

uint8_t Adafruit_TMF8828::getAndClearInterrupts(uint8_t mask) {
  return tmf8828GetAndClrInterrupts(&driver, mask);
}

void Adafruit_TMF8828::clearAndEnableInterrupts(uint8_t mask) {
  tmf8828ClrAndEnableInterrupts(&driver, mask);
}

void Adafruit_TMF8828::disableInterrupts(uint8_t mask) {
  tmf8828DisableInterrupts(&driver, mask);
}
