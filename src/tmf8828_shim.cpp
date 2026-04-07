/*!
 * @file tmf8828_shim.cpp
 *
 * Arduino shim layer for the ams-OSRAM TMF8828 driver using Adafruit BusIO.
 *
 * Copyright 2026 Adafruit Industries (Limor 'ladyada' Fried with assistance
 * from Claude Code)
 *
 * MIT License
 */

#include "tmf8828_shim.h"

#include <Adafruit_I2CDevice.h>

#include "Adafruit_TMF8828.h"
#include "tmf8828.h"

Adafruit_I2CDevice* tmf8828_i2c_device = nullptr;
int8_t tmf8828_enable_pin = -1;
int8_t tmf8828_interrupt_pin = -1;

struct tmf8828_result_t tmf8828_last_result = {};
volatile bool tmf8828_last_result_valid = false;

void delayInMicroseconds(uint32_t wait) {
  delayMicroseconds(wait);
}

uint32_t getSysTick() {
  return micros();
}

uint8_t readProgramMemoryByte(const uint8_t* ptr) {
#if defined(__AVR__)
  return pgm_read_byte(ptr);
#else
  return *ptr;
#endif
}

void enablePinHigh(void* dptr) {
  (void)dptr;
  if (tmf8828_enable_pin >= 0) {
    digitalWrite(tmf8828_enable_pin, HIGH);
  }
}

void enablePinLow(void* dptr) {
  (void)dptr;
  if (tmf8828_enable_pin >= 0) {
    digitalWrite(tmf8828_enable_pin, LOW);
  }
}

void configurePins(void* dptr) {
  (void)dptr;
  if (tmf8828_enable_pin >= 0) {
    pinOutput((uint8_t)tmf8828_enable_pin);
  }
  if (tmf8828_interrupt_pin >= 0) {
    pinInput((uint8_t)tmf8828_interrupt_pin);
  }
}

void i2cOpen(void* dptr, uint32_t i2cClockSpeedInHz) {
  (void)dptr;
  if (tmf8828_i2c_device) {
    tmf8828_i2c_device->begin();
    tmf8828_i2c_device->setSpeed(i2cClockSpeedInHz);
  }
}

void i2cClose(void* dptr) {
  (void)dptr;
  if (tmf8828_i2c_device) {
    tmf8828_i2c_device->end();
  }
}

void printChar(char c) {
  Serial.print(c);
}

void printInt(int32_t i) {
  Serial.print(i, DEC);
}

void printUint(uint32_t i) {
  Serial.print(i, DEC);
}

void printUintHex(uint32_t i) {
  Serial.print(i, HEX);
}

void printStr(char* str) {
  Serial.print(str);
}

void printLn(void) {
  Serial.print('\n');
}

void inputOpen(uint32_t baudrate) {
  (void)baudrate;
}

void inputClose() {}

int8_t inputGetKey(char* c) {
  if (c) {
    *c = 0;
  }
  return 0;
}

void printConstStr(const char* str) {
#if defined(__AVR__)
  Serial.print(reinterpret_cast<const __FlashStringHelper*>(str));
#else
  Serial.print(str);
#endif
}

void pinOutput(uint8_t pin) {
  pinMode(pin, OUTPUT);
}

void pinInput(uint8_t pin) {
  pinMode(pin, INPUT);
}

void setInterruptHandler(void (*handler)(void)) {
  if (tmf8828_interrupt_pin >= 0) {
    attachInterrupt(digitalPinToInterrupt(tmf8828_interrupt_pin), handler,
                    FALLING);
  }
}

void clrInterruptHandler(void) {
  if (tmf8828_interrupt_pin >= 0) {
    detachInterrupt(digitalPinToInterrupt(tmf8828_interrupt_pin));
  }
}

void disableInterrupts(void) {
  noInterrupts();
}

void enableInterrupts(void) {
  interrupts();
}

static bool i2cAddressMatches(uint8_t slaveAddr) {
  if (!tmf8828_i2c_device) {
    return false;
  }
  if (tmf8828_i2c_device->address() == slaveAddr) {
    return true;
  }

  delete tmf8828_i2c_device;
  tmf8828_i2c_device = new Adafruit_I2CDevice(slaveAddr, &Wire);
  if (!tmf8828_i2c_device) {
    return false;
  }
  tmf8828_i2c_device->begin();
  return true;
}

static int8_t i2cWriteReg(uint8_t slaveAddr, uint8_t regAddr,
                          const uint8_t* txData, uint16_t toTx) {
  if (!tmf8828_i2c_device) {
    return I2C_ERR_OTHER;
  }
  if (!i2cAddressMatches(slaveAddr)) {
    return I2C_ERR_SLAVE_ADDR_NAK;
  }

  size_t max_len = tmf8828_i2c_device->maxBufferSize();
  if (max_len < 2) {
    return I2C_ERR_DATA_TOO_LONG;
  }

  uint16_t remaining = toTx;
  const uint8_t* ptr = txData;
  uint8_t reg = regAddr;
  if (remaining == 0) {
    if (!tmf8828_i2c_device->write(nullptr, 0, true, &reg, 1)) {
      return I2C_ERR_OTHER;
    }
    return I2C_SUCCESS;
  }

  while (remaining > 0) {
    size_t chunk = remaining;
    if (chunk > (max_len - 1)) {
      chunk = max_len - 1;
    }
    if (!tmf8828_i2c_device->write(ptr, chunk, true, &reg, 1)) {
      return I2C_ERR_OTHER;
    }
    remaining -= chunk;
    ptr += chunk;
    reg = (uint8_t)(reg + chunk);
  }
  return I2C_SUCCESS;
}

static int8_t i2cReadReg(uint8_t slaveAddr, uint8_t regAddr, uint8_t* rxData,
                         uint16_t toRx) {
  if (!tmf8828_i2c_device) {
    return I2C_ERR_OTHER;
  }
  if (!i2cAddressMatches(slaveAddr)) {
    return I2C_ERR_SLAVE_ADDR_NAK;
  }

  size_t max_len = tmf8828_i2c_device->maxBufferSize();
  if (max_len == 0) {
    return I2C_ERR_DATA_TOO_LONG;
  }

  uint16_t remaining = toRx;
  uint8_t* ptr = rxData;
  uint8_t reg = regAddr;
  while (remaining > 0) {
    size_t chunk = remaining;
    if (chunk > max_len) {
      chunk = max_len;
    }
    if (!tmf8828_i2c_device->write_then_read(&reg, 1, ptr, chunk, true)) {
      return I2C_ERR_OTHER;
    }
    remaining -= chunk;
    ptr += chunk;
    reg = (uint8_t)(reg + chunk);
  }
  return I2C_SUCCESS;
}

int8_t i2cTxReg(void* dptr, uint8_t slaveAddr, uint8_t regAddr, uint16_t toTx,
                const uint8_t* txData) {
  (void)dptr;
  return i2cWriteReg(slaveAddr, regAddr, txData, toTx);
}

int8_t i2cRxReg(void* dptr, uint8_t slaveAddr, uint8_t regAddr, uint16_t toRx,
                uint8_t* rxData) {
  (void)dptr;
  return i2cReadReg(slaveAddr, regAddr, rxData, toRx);
}

int8_t i2cTxRx(void* dptr, uint8_t slaveAddr, uint16_t toTx,
               const uint8_t* txData, uint16_t toRx, uint8_t* rxData) {
  (void)dptr;
  if (!tmf8828_i2c_device) {
    return I2C_ERR_OTHER;
  }
  if (!i2cAddressMatches(slaveAddr)) {
    return I2C_ERR_SLAVE_ADDR_NAK;
  }

  if (toTx == 0) {
    if (!tmf8828_i2c_device->read(rxData, toRx, true)) {
      return I2C_ERR_OTHER;
    }
    return I2C_SUCCESS;
  }

  if (toRx == 0) {
    return i2cWriteReg(slaveAddr, txData[0], txData + 1, toTx - 1);
  }

  if (toTx == 1) {
    return i2cReadReg(slaveAddr, txData[0], rxData, toRx);
  }

  size_t max_len = tmf8828_i2c_device->maxBufferSize();
  if (toRx > max_len) {
    return I2C_ERR_DATA_TOO_LONG;
  }

  if (!tmf8828_i2c_device->write_then_read(txData, toTx, rxData, toRx, false)) {
    return I2C_ERR_OTHER;
  }
  return I2C_SUCCESS;
}

void printResults(void* dptr, uint8_t* data, uint8_t len) {
  tmf8828Driver* driver = (tmf8828Driver*)dptr;
  if (len < TMF8828_COM_CONFIG_RESULT__measurement_result_size) {
    return;
  }

  tmf8828_last_result.resultNumber = data[RESULT_REG(RESULT_NUMBER)];
  tmf8828_last_result.temperature = data[RESULT_REG(TEMPERATURE)];
  tmf8828_last_result.validResults = data[RESULT_REG(NUMBER_VALID_RESULTS)];
  tmf8828_last_result.sysTick = tmf8828GetUint32(data + RESULT_REG(SYS_TICK_0));

  uint8_t* ptr = data + RESULT_REG(RES_CONFIDENCE_0);
  for (int8_t i = 0; i < PRINT_NUMBER_RESULTS; i++) {
    uint8_t confidence = ptr[0];
    uint16_t distance = (uint16_t)ptr[1] | ((uint16_t)ptr[2] << 8);
    distance = tmf8828CorrectDistance(driver, distance);
    tmf8828_last_result.results[i].confidence = confidence;
    tmf8828_last_result.results[i].distance = distance;
    ptr += 3;
  }

  tmf8828_last_result_valid = true;
}

void printHistogram(void* dptr, uint8_t* data, uint8_t len) {
  (void)dptr;
  (void)data;
  (void)len;
}
