/*!
 * @file 06_webserial.ino
 *
 * Web Serial demo for the Adafruit TMF8828 8x8 ToF sensor.
 *
 * Outputs assembled 8x8 frames for the WebSerial visualization page.
 * Accepts serial commands to change ranging period and kilo-iterations.
 *
 * Output format (one complete 8x8 frame per block):
 *   FRAME_START
 *   TEMP:22
 *   R0:123,456,789,0,0,123,456,789   (8 distances for row 0)
 *   R1:...                            (row 1, etc.)
 *   ...
 *   R7:...
 *   FRAME_END
 *
 * Commands:
 *   PERIOD:100 through PERIOD:1000 — set ranging period in ms
 *   KITER:50 through KITER:1000 — set kilo-iterations
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code
 */

#include <Adafruit_TMF8828.h>

#define TMF8828_EN_PIN 3 // GPIO pin connected to TMF8828 EN, or -1 to skip

Adafruit_TMF8828 tmf(TMF8828_EN_PIN);

tmf8828_frame_t frame;

String inputBuffer = "";

uint16_t currentPeriod = 132;
uint16_t currentKiter = 250;

static bool applyConfig(void) {
  tmf.stopRanging();
  if (!tmf.configure(currentPeriod, currentKiter, TMF8828_SPAD_8X8)) {
    return false;
  }
  tmf.clearAndEnableInterrupts(TMF8828_APP_I2C_RESULT_IRQ_MASK);
  return tmf.startRanging();
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("TMF8828 WebSerial Demo"));

  if (!tmf.begin(0x41, &Wire, 400000)) {
    halt(F("ERROR: Failed to initialize TMF8828 sensor!"));
  }

  if (!tmf.setMode8x8()) {
    halt(F("ERROR: Failed to set 8x8 mode!"));
  }

  if (!applyConfig()) {
    halt(F("ERROR: Failed to configure sensor!"));
  }

  Serial.print(F("Period: "));
  Serial.print(currentPeriod);
  Serial.println(F(" ms"));
  Serial.print(F("Kiter: "));
  Serial.println(currentKiter);
  // Flush any garbage in the RX buffer from upload/reset
  while (Serial.available()) {
    Serial.read();
  }

  Serial.println(F("READY"));
}

void loop() {
  processSerialInput();

  // readFrame() collects 4 subcaptures and assembles the 8x8 grid
  if (tmf.readFrame(&frame)) {
    outputFrame();
  }

  delay(1);
}

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

void handleCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();

  if (cmd.startsWith(F("PERIOD:"))) {
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

void outputFrame(void) {
  Serial.println(F("FRAME_START"));
  Serial.print(F("TEMP:"));
  Serial.println(frame.temperature);

  for (uint8_t row = 0; row < 8; row++) {
    Serial.print(F("R"));
    Serial.print(row);
    Serial.print(F(":"));
    for (uint8_t col = 0; col < 8; col++) {
      if (col) {
        Serial.print(F(","));
      }
      Serial.print(frame.distances[row][col]);
    }
    Serial.print(F("|"));
    for (uint8_t col = 0; col < 8; col++) {
      if (col) {
        Serial.print(F(","));
      }
      Serial.print(frame.confidences[row][col]);
    }
    Serial.println();
  }

  Serial.println(F("FRAME_END"));
}

void halt(const __FlashStringHelper* msg) {
  Serial.println(msg);
  Serial.println(F("FAIL"));
  while (1) {
    delay(10);
  }
}
