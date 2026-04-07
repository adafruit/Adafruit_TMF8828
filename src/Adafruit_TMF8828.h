/*!
 * @file Adafruit_TMF8828.h
 *
 * Arduino library for the ams-OSRAM TMF8828 8x8 multizone dToF sensor.
 *
 * Copyright 2026 Adafruit Industries (Limor 'ladyada' Fried with assistance
 * from Claude Code)
 *
 * MIT License
 */

#ifndef ADAFRUIT_TMF8828_H
#define ADAFRUIT_TMF8828_H

#include <Adafruit_I2CDevice.h>
#include <Arduino.h>

#include "tmf8828.h"

/**
 * @brief Parsed TMF8828 measurement results.
 */
typedef struct tmf8828_result_t {
  uint8_t resultNumber;
  uint8_t temperature;
  uint8_t validResults;
  uint32_t sysTick;
  struct {
    uint8_t confidence;
    uint16_t distance; // mm
  } results[36];
} tmf8828_result_t;

class Adafruit_TMF8828 {
 public:
  Adafruit_TMF8828(int8_t enPin = -1);
  bool begin(uint8_t addr = 0x41, TwoWire* wire = &Wire,
             uint32_t i2cSpeed = 400000);

  // Mode
  bool setMode8x8();
  bool setModeLegacy();
  bool isMode8x8();

  // Configuration
  bool configure(uint16_t periodMs = 132, uint16_t kiloIterations = 250,
                 uint8_t spadMapId = 15);
  bool setThresholds(uint16_t low, uint16_t high, uint8_t persistence = 0);
  bool setInterruptMask(uint32_t mask);

  // Ranging
  bool startRanging();
  bool stopRanging();
  bool dataReady();
  bool getRangingData(tmf8828_result_t* result);

  // Calibration
  bool factoryCalibration();
  bool resetFactoryCalibration();
  bool loadFactoryCalibration(const uint8_t* calibData);

  // Power
  bool standby();
  bool wakeup();
  bool enable();
  bool disable();

  // I2C
  bool changeI2CAddress(uint8_t newAddr);

  // Info
  bool readDeviceInfo();
  uint32_t getSerialNumber();

  // Clock correction
  void setClockCorrection(bool enabled);

  // Interrupts
  uint8_t getAndClearInterrupts(uint8_t mask);
  void clearAndEnableInterrupts(uint8_t mask);
  void disableInterrupts(uint8_t mask);

  tmf8828Driver driver; // public for advanced access

 private:
  bool downloadFirmware(const uint8_t* image, uint32_t start, int32_t length);

  Adafruit_I2CDevice* _i2c;
  TwoWire* _wire;
  int8_t _enPin;
  bool _is8x8;

  uint16_t _periodMs;
  uint16_t _kiloIterations;
  uint8_t _spadMapId;
  uint16_t _lowThreshold;
  uint16_t _highThreshold;
  uint8_t _persistence;
  uint32_t _intMask;
  uint8_t _dumpHistogram;
};

#endif
