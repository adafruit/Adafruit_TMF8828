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
 * @brief Parsed TMF8828 measurement results for a single subcapture (36 zones).
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

/**
 * @brief Complete 8x8 frame accumulated from 4 subcaptures.
 *
 * In 8x8 mode the sensor time-multiplexes 4 subcaptures, each with up to
 * 36 zone results. This struct holds all 4 subcaptures once a complete
 * frame has been collected.
 */
typedef struct tmf8828_frame_t {
  uint8_t temperature;        ///< Temperature from last subcapture
  uint16_t distances[4][36];  ///< Distance (mm) per subcapture/zone
  uint8_t confidences[4][36]; ///< Confidence per subcapture/zone
} tmf8828_frame_t;

/** SPAD map IDs — select the zone layout for ranging. */
typedef enum {
  TMF8828_SPAD_3X3_NORMAL = 1,       ///< 3x3, 14x6, 29x29 deg
  TMF8828_SPAD_3X3_MACRO_A = 2,      ///< 3x3, 14x9, 29x43.5 deg
  TMF8828_SPAD_3X3_MACRO_B = 3,      ///< 3x3, 14x9, 29x43.5 deg
  TMF8828_SPAD_4X4_MACRO_A = 4,      ///< 4x4 time-mux, 14x9, 29x43.5 deg
  TMF8828_SPAD_4X4_MACRO_B = 5,      ///< 4x4 time-mux, 14x9, 29x43.5 deg
  TMF8828_SPAD_3X3_WIDE = 6,         ///< 3x3, 18x10, 44x48 deg
  TMF8828_SPAD_4X4_WIDE = 7,         ///< 4x4 time-mux, 18x10, 44x48 deg
  TMF8828_SPAD_9ZONE_MACRO_A = 8,    ///< 9 zones, 14x9, 29x43.5 deg
  TMF8828_SPAD_9ZONE_MACRO_B = 9,    ///< 9 zones, 14x9, 29x43.5 deg
  TMF8828_SPAD_3X6_TIMEMUX = 10,     ///< 3x6 time-mux, 18x12, 29x57 deg
  TMF8828_SPAD_3X3_CHECKER = 11,     ///< 3x3 checkerboard, 14x6, 29x29 deg
  TMF8828_SPAD_3X3_CHECKER_REV = 12, ///< 3x3 reverse checker, 14x6, 29x29 deg
  TMF8828_SPAD_4X4_NARROW = 13,      ///< 4x4 time-mux, 18x8, 29x39 deg
  TMF8828_SPAD_8X8 = 15,             ///< 8x8 mode (TMF8828 only)
} tmf8828_spad_map_t;

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
                 uint8_t spadMapId = TMF8828_SPAD_8X8);
  bool setThresholds(uint16_t low, uint16_t high, uint8_t persistence = 0);
  bool setInterruptMask(uint32_t mask);

  // Ranging
  bool startRanging();
  bool stopRanging();
  bool dataReady();
  bool getRangingData(tmf8828_result_t* result);

  // 8x8 frame accumulation — collects 4 subcaptures into one frame
  bool readFrame(tmf8828_frame_t* frame);

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
  void forceReset();
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

  // 8x8 frame accumulation state
  tmf8828_frame_t _frame;
  uint8_t _subcaptureMask; // bits 0-3 track which subcaptures received
};

#endif
