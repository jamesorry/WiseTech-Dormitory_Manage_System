#ifndef _RFID_H_
#define _RFID_H_

#include "Arduino.h"

typedef struct
{
	uint8_t 	Data[16];
	int 		Len;
	bool 		Update;

	uint16_t	retrytimecnt;

	uint8_t	ProcessIndex;
    uint8_t	preProcessIndex = 0xFF;
	uint16_t ProcessTimeCnt;
}RFIDData;

void RFID_Init(void);
void RFID_Process(void);
void RFID_Reset(void);
void RFID_Timer();
void RFID_Read();

#endif //_RFID_H_
