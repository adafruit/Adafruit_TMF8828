# Adafruit TMF8828 Library

<!-- @cond -->
[![Arduino Library CI](https://github.com/adafruit/Adafruit_TMF8828/actions/workflows/ci.yml/badge.svg)](https://github.com/adafruit/Adafruit_TMF8828/actions/workflows/ci.yml)
<!-- @endcond -->

Arduino library for the ams-OSRAM TMF8828 8x8 multizone direct Time-of-Flight (dToF) sensor. Supports up to 5m range with 3x3, 4x4, or 8x8 zone resolution.

**Features**
- 8x8 multizone mode (64 zones via 4 time-multiplexed sub-captures)
- Legacy mode (3x3 / 4x4 / 3x6 zones, selectable SPAD maps)
- Configurable ranging period and integration time (kilo-iterations)
- Distance-based detection thresholds with persistence
- Interrupt support (INT pin, configurable mask)
- Factory calibration (run, save, load, reset)
- Power management (standby, wakeup, enable, disable)
- Dynamic I2C address change
- Host-side clock correction for improved accuracy
- Serial number readout and device info

**Hardware Requirements**

> **This sensor will NOT work on AVR boards (Arduino Uno, Mega, etc.).**

The TMF8828 requires a firmware upload (~7KB) on every power-on. While much smaller than some ToF sensors, the driver still needs more RAM than AVR provides. Use an ARM-class MCU: ESP32, SAMD51, RP2040, nRF52840, STM32, etc.

- **First `begin()` call takes a few seconds** while firmware uploads. This is normal.
- **The sensor retains firmware across soft resets** (as long as 3.3V stays up).
- **Default I2C address:** 0x41

**Dependencies**
- [Adafruit BusIO](https://github.com/adafruit/Adafruit_BusIO)

**Examples**
- `examples/01_simpletest` — Basic 8x8 ranging readout
- `examples/02_legacy_mode` — Legacy 3x3 mode ranging
- `examples/03_ascii_art` — 8x8 distance grid as ASCII art in the terminal
- `examples/04_set_address` — Change the I2C address at runtime
- `examples/05_power_modes` — Standby/wakeup power management demo

**TMF8828 Driver**

The low-level TMF8828 driver (`tmf8828.c`, `tmf8828.h`, `tmf8828_calib.*`, `tmf8828_image.*`, `tmf882x_*`) is copyright ams-OSRAM AG and distributed under their license terms. See the individual file headers for details. The Adafruit wrapper and shim layer are MIT licensed.

**License**

MIT (Adafruit wrapper, shim, and examples). See individual ams-OSRAM driver files for their license terms.
