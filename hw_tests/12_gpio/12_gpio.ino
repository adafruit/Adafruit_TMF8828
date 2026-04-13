/*!
 * @file 12_gpio.ino
 *
 * Hardware test for TMF8828 GPIO handling.
 *
 * Copyright 2026 Adafruit Industries (Limor 'ladyada' Fried with assistance
 * from Claude Code)
 *
 * MIT License
 */

#include <Adafruit_TMF8828.h>

// Set to a GPIO pin to hardware-reset the sensor before init, or -1 to skip
#define TMF8828_EN_PIN 3
#define GPIO0_PIN 4
#define GPIO1_PIN 5

Adafruit_TMF8828 tmf(TMF8828_EN_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("TMF8828 GPIO test"));

  pinMode(GPIO0_PIN, INPUT);
  pinMode(GPIO1_PIN, INPUT);

  bool overall = true;

  Serial.println(F(""));
  Serial.println(F("Step 1: begin + setMode8x8"));
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
  Serial.println(F("Step 2: GPIO0 output high/low"));
  bool step2Ok = true;
  if (!tmf.setGPIO0(TMF8828_GPIO_OUTPUT_HIGH)) {
    Serial.println(F("setGPIO0 HIGH FAILED"));
    step2Ok = false;
  }
  delay(10);
  int gpio0High = digitalRead(GPIO0_PIN);
  if (gpio0High != HIGH) {
    step2Ok = false;
  }

  if (!tmf.setGPIO0(TMF8828_GPIO_OUTPUT_LOW)) {
    Serial.println(F("setGPIO0 LOW FAILED"));
    step2Ok = false;
  }
  delay(10);
  int gpio0Low = digitalRead(GPIO0_PIN);
  if (gpio0Low != LOW) {
    step2Ok = false;
  }

  Serial.print(F("GPIO0 high="));
  Serial.print(gpio0High == HIGH ? F("HIGH") : F("LOW"));
  Serial.print(F(" low="));
  Serial.println(gpio0Low == HIGH ? F("HIGH") : F("LOW"));
  Serial.println(step2Ok ? F("PASS") : F("FAIL"));
  if (!step2Ok) {
    overall = false;
  }

  delay(50);

  Serial.println(F(""));
  Serial.println(F("Step 3: GPIO1 output high/low"));
  bool step3Ok = true;
  if (!tmf.setGPIO1(TMF8828_GPIO_OUTPUT_HIGH)) {
    Serial.println(F("setGPIO1 HIGH FAILED"));
    step3Ok = false;
  }
  delay(10);
  int gpio1High = digitalRead(GPIO1_PIN);
  if (gpio1High != HIGH) {
    step3Ok = false;
  }

  if (!tmf.setGPIO1(TMF8828_GPIO_OUTPUT_LOW)) {
    Serial.println(F("setGPIO1 LOW FAILED"));
    step3Ok = false;
  }
  delay(10);
  int gpio1Low = digitalRead(GPIO1_PIN);
  if (gpio1Low != LOW) {
    step3Ok = false;
  }

  Serial.print(F("GPIO1 high="));
  Serial.print(gpio1High == HIGH ? F("HIGH") : F("LOW"));
  Serial.print(F(" low="));
  Serial.println(gpio1Low == HIGH ? F("HIGH") : F("LOW"));
  Serial.println(step3Ok ? F("PASS") : F("FAIL"));
  if (!step3Ok) {
    overall = false;
  }

  delay(50);

  Serial.println(F(""));
  Serial.println(F("Step 4: GPIO0 tristate + pullup"));
  bool step4Ok = tmf.setGPIO0(TMF8828_GPIO_TRISTATE);
  pinMode(GPIO0_PIN, INPUT_PULLUP);
  delay(10);
  int gpio0Pullup = digitalRead(GPIO0_PIN);
  if (gpio0Pullup != HIGH) {
    step4Ok = false;
  }
  Serial.print(F("GPIO0 pullup read="));
  Serial.println(gpio0Pullup == HIGH ? F("HIGH") : F("LOW"));
  Serial.println(step4Ok ? F("PASS") : F("FAIL"));
  if (!step4Ok) {
    overall = false;
  }

  delay(50);

  Serial.println(F(""));
  Serial.println(F("Step 5: GPIO0 VCSEL_HIGH toggling"));
  bool step5Ok = true;
  if (!tmf.setGPIO0(TMF8828_GPIO_VCSEL_HIGH)) {
    Serial.println(F("setGPIO0 VCSEL_HIGH FAILED"));
    step5Ok = false;
  }

  if (!tmf.configure(132, 250, TMF8828_SPAD_8X8)) {
    Serial.println(F("configure FAILED"));
    step5Ok = false;
  }

  if (!tmf.startRanging()) {
    Serial.println(F("startRanging FAILED"));
    step5Ok = false;
  } else {
    pinMode(GPIO0_PIN, INPUT);
    int last = digitalRead(GPIO0_PIN);
    uint16_t transitions = 0;
    uint32_t start = millis();
    while ((millis() - start) < 500) {
      int state = digitalRead(GPIO0_PIN);
      if (state != last) {
        transitions++;
        last = state;
      }
    }
    tmf.stopRanging();
    Serial.print(F("Transitions="));
    Serial.println(transitions);
    if (transitions == 0) {
      step5Ok = false;
    }
  }
  Serial.println(step5Ok ? F("PASS") : F("FAIL"));
  if (!step5Ok) {
    overall = false;
  }

  delay(50);

  Serial.println(F(""));
  Serial.println(F("Step 6: VCSEL mutual exclusion"));
  bool step6Ok = true;
  if (!tmf.setGPIO0(TMF8828_GPIO_VCSEL_HIGH)) {
    Serial.println(F("setGPIO0 VCSEL_HIGH FAILED"));
    step6Ok = false;
  }
  bool gpio1Set = tmf.setGPIO1(TMF8828_GPIO_VCSEL_LOW);
  if (gpio1Set) {
    step6Ok = false;
  }
  Serial.print(F("GPIO1 VCSEL_LOW result="));
  Serial.println(gpio1Set ? F("true") : F("false"));
  Serial.println(step6Ok ? F("PASS") : F("FAIL"));
  if (!step6Ok) {
    overall = false;
  }

  delay(50);

  Serial.println(F(""));
  Serial.println(F("Step 7: getter test"));
  bool step7Ok = true;
  if (!tmf.setGPIO0(TMF8828_GPIO_OUTPUT_HIGH)) {
    Serial.println(F("setGPIO0 OUTPUT_HIGH FAILED"));
    step7Ok = false;
  }
  tmf8828_gpio_mode_t mode = tmf.getGPIO0();
  if (mode != TMF8828_GPIO_OUTPUT_HIGH) {
    step7Ok = false;
  }
  Serial.print(F("getGPIO0="));
  Serial.println((int)mode);
  Serial.println(step7Ok ? F("PASS") : F("FAIL"));
  if (!step7Ok) {
    overall = false;
  }

  Serial.println(F(""));
  Serial.println(overall ? F("PASS") : F("FAIL"));
}

void loop() {}

void halt(const __FlashStringHelper* msg) {
  Serial.println(msg);
  Serial.println(F("FAIL"));
  while (1) {
    delay(10);
  }
}
