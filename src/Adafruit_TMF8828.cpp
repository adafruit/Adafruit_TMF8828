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

bool Adafruit_TMF8828::begin(uint8_t addr, TwoWire* wire) {
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

  tmf8828_i2c_device = _i2c;
  tmf8828_enable_pin = _enPin;
  configurePins(nullptr);

  tmf8828Initialise(&driver);
  tmf8828Enable(&driver);
  delayInMicroseconds(ENABLE_TIME_MS * 1000UL);
  tmf8828ClkCorrection(&driver, 1);
  tmf8828SetLogLevel(&driver, TMF8828_LOG_LEVEL_ERROR);
  tmf8828Wakeup(&driver);
  if (!tmf8828IsCpuReady(&driver, CPU_READY_TIME_MS)) {
    return false;
  }

  if (!downloadFirmware(tmf8828_image, (uint32_t)tmf8828_image_start,
                        (int32_t)tmf8828_image_length)) {
    return false;
  }

  _is8x8 = true;
  tmf8828_last_result_valid = false;
  return true;
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
