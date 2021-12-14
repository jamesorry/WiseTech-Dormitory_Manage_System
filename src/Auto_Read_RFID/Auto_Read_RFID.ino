#include "Arduino.h"
#include "rfid.h"

void setup() {
  Serial.begin(115200);
  RFID_Init();
  Serial.println("Init OK.");
  buzzerPlay(200);
}
 
void loop() {
  doRfidProcess();
}
