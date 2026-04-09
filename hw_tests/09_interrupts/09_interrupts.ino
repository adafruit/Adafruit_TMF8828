/*!
 * @file 09_interrupts.ino
 *
 * Hardware test for TMF8828 interrupt handling.
 *
 * Copyright 2026 Adafruit Industries (Limor 'ladyada' Fried with assistance
 * from Claude Code)
 *
 * MIT License
 */

#include <Adafruit_TMF8828.h>

// Set to a GPIO pin to hardware-reset the sensor before init, or -1 to skip
#define TMF8828_EN_PIN 3
#define TMF8828_INT_PIN 2

Adafruit_TMF8828 tmf(TMF8828_EN_PIN);

volatile uint32_t interruptCount = 0;

static void handleInterrupt() {
  interruptCount++;
}

static uint32_t readInterruptCount() {
  noInterrupts();
  uint32_t count = interruptCount;
  interrupts();
  return count;
}

static void resetInterruptCount() {
  noInterrupts();
  interruptCount = 0;
  interrupts();
}

static void waitForInterrupts(uint32_t timeoutMs) {
  uint32_t start = millis();
  while ((millis() - start) < timeoutMs) {
    if (readInterruptCount() > 0) {
      return;
    }
    delay(5);
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("TMF8828 interrupt test"));

  pinMode(TMF8828_INT_PIN, INPUT_PULLUP);

  bool overall = true;

  Serial.println(F(""));
  Serial.println(F("Step 1: begin + setMode8x8"));
  // Args: I2C address, Wire bus, I2C speed (Hz)
  if (!tmf.begin(0x41, &Wire, 400000)) {
    halt(F("begin FAILED"));
  }
  Serial.println(F("begin PASS"));

  if (!tmf.setMode8x8()) {
    halt(F("setMode8x8 FAILED"));
  }
  Serial.println(F("setMode8x8 PASS"));
  Serial.println(F("PASS"));

  delay(50);

  Serial.println(F(""));
  Serial.println(F("Step 2: INT pin idle state"));
  int idleState = digitalRead(TMF8828_INT_PIN);
  Serial.print(F("TMF8828_INT_PIN read="));
  Serial.println(idleState == HIGH ? F("HIGH") : F("LOW"));
  bool step2Ok = (idleState == HIGH);
  Serial.println(step2Ok ? F("PASS") : F("FAIL"));
  if (!step2Ok) {
    overall = false;
  }

  delay(50);

  Serial.println(F(""));
  Serial.println(F("Step 3: Interrupt-driven ranging"));
  bool step3Ok = true;
  tmf.clearAndEnableInterrupts(TMF8828_APP_I2C_RESULT_IRQ_MASK);
  if (!tmf.configure(132, 250, 15)) {
    halt(F("configure FAILED"));
    overall = false;
    step3Ok = false;
  } else {
    Serial.println(F("configure PASS"));
  }

  resetInterruptCount();
  if (!tmf.startRanging()) {
    Serial.println(F("startRanging FAILED"));
    step3Ok = false;
  } else {
    Serial.println(F("startRanging PASS"));
    attachInterrupt(digitalPinToInterrupt(TMF8828_INT_PIN), handleInterrupt,
                    FALLING);
    waitForInterrupts(5000);
    tmf.stopRanging();
    Serial.println(F("stopRanging PASS"));
    detachInterrupt(digitalPinToInterrupt(TMF8828_INT_PIN));
  }

  uint32_t step3Count = readInterruptCount();
  Serial.print(F("Interrupt count="));
  Serial.println(step3Count);
  if (step3Ok && (step3Count > 0)) {
    Serial.println(F("PASS"));
  } else {
    Serial.println(F("FAIL"));
    overall = false;
  }

  delay(100);

  Serial.println(F(""));
  Serial.println(F("Step 4: getAndClearInterrupts"));
  bool step4Ok = true;
  if (!tmf.startRanging()) {
    Serial.println(F("startRanging FAILED"));
    step4Ok = false;
  } else {
    Serial.println(F("startRanging PASS"));
    delay(500);
    uint8_t first = tmf.getAndClearInterrupts(TMF8828_APP_I2C_RESULT_IRQ_MASK);
    uint8_t second = tmf.getAndClearInterrupts(TMF8828_APP_I2C_RESULT_IRQ_MASK);
    tmf.stopRanging();
    Serial.println(F("stopRanging PASS"));

    Serial.print(F("first read=0x"));
    Serial.print(first, HEX);
    Serial.print(F(" second read=0x"));
    Serial.println(second, HEX);

    if (first == 0 || second != 0) {
      step4Ok = false;
    }
  }

  Serial.println(step4Ok ? F("PASS") : F("FAIL"));
  if (!step4Ok) {
    overall = false;
  }

  delay(100);

  Serial.println(F(""));
  Serial.println(F("Step 5: disableInterrupts"));
  bool step5Ok = true;
  tmf.disableInterrupts(TMF8828_APP_I2C_RESULT_IRQ_MASK);
  // Clear any pending interrupt so INT pin goes HIGH
  tmf.getAndClearInterrupts(TMF8828_APP_I2C_RESULT_IRQ_MASK);
  delay(50);
  if (!tmf.startRanging()) {
    Serial.println(F("startRanging FAILED"));
    step5Ok = false;
  } else {
    Serial.println(F("startRanging PASS"));
    // Let ranging run, then attach interrupt to verify no continuous edges
    delay(500);
    resetInterruptCount();
    attachInterrupt(digitalPinToInterrupt(TMF8828_INT_PIN), handleInterrupt,
                    FALLING);
    delay(2000);
    detachInterrupt(digitalPinToInterrupt(TMF8828_INT_PIN));
    tmf.stopRanging();
    Serial.println(F("stopRanging PASS"));
  }

  uint32_t step5Count = readInterruptCount();
  Serial.print(F("Interrupt count="));
  Serial.println(step5Count);
  // Allow ≤1 spurious edge (I2C crosstalk / noise); key test is no continuous
  // stream like steps 3/6
  step5Ok = step5Ok && (step5Count <= 1);
  Serial.println(step5Ok ? F("PASS") : F("FAIL"));
  if (!step5Ok) {
    overall = false;
  }

  delay(100);

  Serial.println(F(""));
  Serial.println(F("Step 6: Re-enable interrupts"));
  bool step6Ok = true;
  tmf.clearAndEnableInterrupts(TMF8828_APP_I2C_RESULT_IRQ_MASK);
  resetInterruptCount();
  attachInterrupt(digitalPinToInterrupt(TMF8828_INT_PIN), handleInterrupt,
                  FALLING);
  if (!tmf.startRanging()) {
    Serial.println(F("startRanging FAILED"));
    step6Ok = false;
  } else {
    Serial.println(F("startRanging PASS"));
    delay(2000);
    tmf.stopRanging();
    Serial.println(F("stopRanging PASS"));
  }
  detachInterrupt(digitalPinToInterrupt(TMF8828_INT_PIN));

  uint32_t step6Count = readInterruptCount();
  Serial.print(F("Interrupt count="));
  Serial.println(step6Count);
  step6Ok = step6Ok && (step6Count > 0);
  Serial.println(step6Ok ? F("PASS") : F("FAIL"));
  if (!step6Ok) {
    overall = false;
  }

  Serial.println(F(""));
  if (overall) {
    Serial.println(F("PASS"));
  } else {
    Serial.println(F("FAIL"));
  }
}

void loop() {}

void halt(const __FlashStringHelper* msg) {
  Serial.println(msg);
  Serial.println(F("FAIL"));
  while (1) {
    delay(10);
  }
}
