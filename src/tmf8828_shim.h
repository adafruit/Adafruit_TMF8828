/*!
 * @file tmf8828_shim.h
 *
 * Arduino shim layer for the ams-OSRAM TMF8828 driver using Adafruit BusIO.
 *
 * Copyright 2026 Adafruit Industries (Limor 'ladyada' Fried with assistance
 * from Claude Code)
 *
 * MIT License
 */

#ifndef TMF8828_SHIM_H
#define TMF8828_SHIM_H

#include <Arduino.h>
#include <stdint.h>

#if defined(__AVR__)
#include <avr/pgmspace.h>
#else
#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const uint8_t*)(addr))
#endif
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#define ARDUINO_MAX_I2C_TRANSFER 32

// for clock correction insert here the number in relation to your host
#define HOST_TICKS_PER_US 1
#define TMF8828_TICKS_PER_US 5

/** @brief macros to cast a pointer to an address - adapt for your machine-word
 * size
 */
#define PTR_TO_UINT(ptr) ((intptr_t)(ptr))

/** @brief macros to replace the platform specific printing
 */
#define PRINT_CHAR(c) printChar(c)
#define PRINT_INT(i) printInt(i)
#define PRINT_UINT(i) printUint(i)
#define PRINT_UINT_HEX(i) printUintHex(i)
#define PRINT_STR(str) printStr(str)
#define PRINT_CONST_STR(str) printConstStr((const char*)str)
#define PRINT_LN() printLn()

/** Which character to use to seperate the entries in printing */
#define SEPARATOR ','

/** forward declaration of driver structure to avoid cyclic dependancies */
typedef struct _tmf8828Driver tmf8828Driver;

void delayInMicroseconds(uint32_t wait);
uint32_t getSysTick();
uint8_t readProgramMemoryByte(const uint8_t* ptr);
void enablePinHigh(void* dptr);
void enablePinLow(void* dptr);
void configurePins(void* dptr);
void i2cOpen(void* dptr, uint32_t i2cClockSpeedInHz);
void i2cClose(void* dptr);
void printChar(char c);
void printInt(int32_t i);
void printUint(uint32_t i);
void printUintHex(uint32_t i);
void printStr(char* str);
void printLn(void);

#define I2C_SUCCESS 0
#define I2C_ERR_DATA_TOO_LONG -1
#define I2C_ERR_SLAVE_ADDR_NAK -2
#define I2C_ERR_DATA_NAK -3
#define I2C_ERR_OTHER -4
#define I2C_ERR_TIMEOUT -5

int8_t i2cTxReg(void* dptr, uint8_t slaveAddr, uint8_t regAddr, uint16_t toTx,
                const uint8_t* txData);
int8_t i2cRxReg(void* dptr, uint8_t slaveAddr, uint8_t regAddr, uint16_t toRx,
                uint8_t* rxData);
int8_t i2cTxRx(void* dptr, uint8_t slaveAddr, uint16_t toTx,
               const uint8_t* txData, uint16_t toRx, uint8_t* rxData);

void inputOpen(uint32_t baudrate);
void inputClose();
int8_t inputGetKey(char* c);
void printConstStr(const char* str);
void pinOutput(uint8_t pin);
void pinInput(uint8_t pin);
void setInterruptHandler(void (*handler)(void));
void clrInterruptHandler(void);
void disableInterrupts(void);
void enableInterrupts(void);
void printResults(void* dptr, uint8_t* data, uint8_t len);
void printHistogram(void* dptr, uint8_t* data, uint8_t len);

#if defined(__cplusplus)
}
#endif

#if defined(__cplusplus)
class Adafruit_I2CDevice;
extern Adafruit_I2CDevice* tmf8828_i2c_device;
extern int8_t tmf8828_enable_pin;

struct tmf8828_result_t;
extern struct tmf8828_result_t tmf8828_last_result;
extern volatile bool tmf8828_last_result_valid;
#endif

#endif
