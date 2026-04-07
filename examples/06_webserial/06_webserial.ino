/*!
 * @file 06_webserial.ino
 *
 * Web Serial demo for the Adafruit TMF8828 8x8 ToF sensor.
 *
 * Outputs structured data for the WebSerial visualization page.
 * Accepts serial commands to change mode, ranging period, and
 * kilo-iterations.
 *
 * Output format (one sub-capture per block):
 *   FRAME_START
 *   MODE:8x8
 *   SUB:0
 *   TEMP:22
 *   D:123,456,789,...  (comma-separated distance_mm values)
 *   C:12,34,56,...     (comma-separated confidence values)
 *   FRAME_END
 *
 * Commands:
 *   MODE:8X8 or MODE:LEGACY — switch modes
 *   PERIOD:100 through PERIOD:1000 — set ranging period in ms
 *   KITER:50 through KITER:1000 — set kilo-iterations
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code
 */

#include <Adafruit_TMF8828.h>

#ifdef ESP32
#define EN_PIN 27
#else
#define EN_PIN A0
#endif

Adafruit_TMF8828 tmf(EN_PIN);

tmf8828_result_t result;

String inputBuffer = "";

bool currentMode8x8 = true;
uint16_t currentPeriod = 132;
uint16_t currentKiter = 250;
const uint8_t legacySpadMap = 6;
const uint8_t mode8x8SpadMap = 15;

static void halt(const __FlashStringHelper* msg) {
  Serial.println(msg);
  while (1) {
    delay(10);
  }
}

static bool applyConfig(void) {
  if (!tmf.stopRanging()) {
    return false;
  }

  bool ok = true;
  if (currentMode8x8) {
    ok &= tmf.setMode8x8();
    ok &= tmf.configure(currentPeriod, currentKiter, mode8x8SpadMap);
  } else {
    ok &= tmf.setModeLegacy();
    ok &= tmf.configure(currentPeriod, currentKiter, legacySpadMap);
  }

  ok &= tmf.startRanging();
  return ok;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("TMF8828 WebSerial Demo"));
  Serial.println(F("======================="));
  Serial.println(F("Initializing sensor..."));

  if (!tmf.begin(0x41)) {
    halt(F("ERROR: Failed to initialize TMF8828 sensor!"));
  }

  if (!applyConfig()) {
    halt(F("ERROR: Failed to configure sensor!"));
  }

  Serial.print(F("Mode: "));
  Serial.println(currentMode8x8 ? F("8x8") : F("LEGACY"));
  Serial.print(F("Period: "));
  Serial.print(currentPeriod);
  Serial.println(F(" ms"));
  Serial.print(F("Kiter: "));
  Serial.println(currentKiter);
  Serial.println(F("READY"));
}

void loop() {
  processSerialInput();

  if (tmf.dataReady()) {
    if (tmf.getRangingData(&result)) {
      outputFrame();
    }
  }

  delay(5);
}

/**************************************************************************/
/*! @brief  Process incoming serial commands */
/**************************************************************************/
void processSerialInput(void) {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        handleCommand(inputBuffer);
        inputBuffer = "";
      }
    } else {
      inputBuffer += c;
    }
  }
}

/**************************************************************************/
/*! @brief  Handle a parsed command string */
/**************************************************************************/
void handleCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();

  if (cmd.startsWith(F("MODE:"))) {
    String mode = cmd.substring(5);
    mode.trim();
    if (mode == F("8X8")) {
      currentMode8x8 = true;
      if (applyConfig()) {
        Serial.println(F("OK MODE:8X8"));
      } else {
        Serial.println(F("ERROR: Failed to set mode"));
      }
    } else if (mode == F("LEGACY")) {
      currentMode8x8 = false;
      if (applyConfig()) {
        Serial.println(F("OK MODE:LEGACY"));
      } else {
        Serial.println(F("ERROR: Failed to set mode"));
      }
    } else {
      Serial.println(F("ERROR: Mode must be 8X8 or LEGACY"));
    }
  } else if (cmd.startsWith(F("PERIOD:"))) {
    uint16_t period = (uint16_t)cmd.substring(7).toInt();
    if (period >= 100 && period <= 1000) {
      currentPeriod = period;
      if (applyConfig()) {
        Serial.print(F("OK PERIOD:"));
        Serial.println(currentPeriod);
      } else {
        Serial.println(F("ERROR: Failed to set period"));
      }
    } else {
      Serial.println(F("ERROR: Period must be 100-1000"));
    }
  } else if (cmd.startsWith(F("KITER:"))) {
    uint16_t kiter = (uint16_t)cmd.substring(6).toInt();
    if (kiter >= 50 && kiter <= 1000) {
      currentKiter = kiter;
      if (applyConfig()) {
        Serial.print(F("OK KITER:"));
        Serial.println(currentKiter);
      } else {
        Serial.println(F("ERROR: Failed to set kiter"));
      }
    } else {
      Serial.println(F("ERROR: Kiter must be 50-1000"));
    }
  } else {
    Serial.print(F("ERROR: Unknown command: "));
    Serial.println(cmd);
  }
}

/**************************************************************************/
/*! @brief  Output a single sub-capture frame */
/**************************************************************************/
void outputFrame(void) {
  uint8_t subcapture = result.resultNumber & 0x03;

  Serial.println(F("FRAME_START"));
  Serial.print(F("MODE:"));
  Serial.println(currentMode8x8 ? F("8x8") : F("LEGACY"));
  Serial.print(F("SUB:"));
  Serial.println(subcapture);
  Serial.print(F("TEMP:"));
  Serial.println(result.temperature);

  Serial.print(F("D:"));
  for (uint8_t i = 0; i < 36; i++) {
    if (i) {
      Serial.print(F(","));
    }
    Serial.print(result.results[i].distance);
  }
  Serial.println();

  Serial.print(F("C:"));
  for (uint8_t i = 0; i < 36; i++) {
    if (i) {
      Serial.print(F(","));
    }
    Serial.print(result.results[i].confidence);
  }
  Serial.println();

  Serial.println(F("FRAME_END"));
}
