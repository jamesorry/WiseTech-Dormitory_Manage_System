#ifndef _RFID_H_
#define _RFID_H_

#define RST_PIN  42     // RST-PIN
#define SS_PIN   53     // SDA-PIN

void RFID_Init(void);
void doRfidProcess(void);
void buzzerPlay(int);

#endif
