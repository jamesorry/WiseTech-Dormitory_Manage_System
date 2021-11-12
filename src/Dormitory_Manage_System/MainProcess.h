#ifndef _MAIN_PROCESS_H_
#define _MAIN_PROCESS_H_

#include "Arduino.h"

#define EXTIO_NUM   2//8個為一組
#define INPUT_8_NUMBER  1
#define OUTPUT_8_NUMBER 1

#define OUTPUT_NONE_ACTIVE  0
#define OUTPUT_ACTIVE   1

#define INPUT_NONE_ACTIVE 0
#define INPUT_ACTIVE    1

typedef struct _DigitalIO_
{
  uint8_t Input[(INPUT_8_NUMBER + EXTIO_NUM)*8];
  uint8_t Output[(OUTPUT_8_NUMBER + EXTIO_NUM)*8];
  uint8_t PreOutput[(OUTPUT_8_NUMBER + EXTIO_NUM)*8];
}DigitalIO;

typedef struct _MainDataStruct_
{
  	char Vendor[10];
}MainDataStruct;


typedef struct _RuntimeStruct_
{
  	int   Workindex;
  	int   preWorkindex;

  	uint8_t sensor[INPUT_8_NUMBER*8 + EXTIO_NUM*8];
  	uint8_t outbuf[(OUTPUT_8_NUMBER+EXTIO_NUM)*8];

  	bool    UpdateEEPROM;
    uint8_t TimeoutSecond = 1;//RFID time out(Unit of a second)
}RuntimeStatus;

void MainProcess_Timer();
void MainProcess_Task();
void MainProcess_Init();
void WriteDigitalOutput();
void ReadDigitalInput();
void setOutput(uint8_t index, uint8_t hl);
uint8_t getInput(uint8_t index);
void buzzerPlay(int playMS);


#endif  //_MAIN_PROCESS_H_
