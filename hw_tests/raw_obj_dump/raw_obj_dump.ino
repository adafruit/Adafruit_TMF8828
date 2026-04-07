/*!
 * @file raw_obj_dump.ino
 *
 * Dumps raw #Obj CSV lines (ams format) for zone map verification.
 * Use with parse_8x8.py to display the 8x8 grid.
 */

#include <Adafruit_TMF8828.h>

Adafruit_TMF8828 tmf;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  if (!tmf.begin(0x41, &Wire, 400000)) {
    Serial.println(F("FAIL"));
    while (1) delay(10);
  }
  if (!tmf.setMode8x8()) {
    Serial.println(F("FAIL"));
    while (1) delay(10);
  }
  if (!tmf.configure(132, 250, TMF8828_SPAD_8X8)) {
    Serial.println(F("FAIL"));
    while (1) delay(10);
  }
  if (!tmf.startRanging()) {
    Serial.println(F("FAIL"));
    while (1) delay(10);
  }
}

void loop() {
  if (!tmf.dataReady()) return;

  tmf8828_result_t res;
  if (!tmf.getRangingData(&res)) return;

  // Print in ams #Obj format:
  // #Obj,addr,result_number,temp,valid_results,systick,dist0,conf0,dist1,conf1,...
  Serial.print(F("#Obj,65,"));
  Serial.print(res.resultNumber);
  Serial.print(F(","));
  Serial.print(res.temperature);
  Serial.print(F(","));
  Serial.print(res.validResults);
  Serial.print(F(",0"));
  for (uint8_t i = 0; i < 36; i++) {
    Serial.print(F(","));
    Serial.print(res.results[i].distance);
    Serial.print(F(","));
    Serial.print(res.results[i].confidence);
  }
  Serial.println();
}
