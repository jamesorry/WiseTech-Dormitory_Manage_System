#include "MainProcess.h"
#include <Adafruit_MCP23017.h>
#include "hmi.h"
#include "Timer.h"
#include <inttypes.h>
#include "HMI_Command.h"

uint16_t TimeCnt = 0;

DigitalIO digitalio;
Adafruit_MCP23017 extio[EXTIO_NUM];
extern HardwareSerial *cmd_port;
extern HMI_Command *hmicmd;
MainDataStruct maindata;
RuntimeStatus runtimedata;

void MainProcess_Timer()
{
    if(TimeCnt < 0xFF00)
        TimeCnt += TIMER_INTERVAL_MS;
}

void MainProcess_Init()
{
  int i,j;
  for(i=0;i<INPUT_8_NUMBER+EXTIO_NUM;i++)
    digitalio.Input[i] = 0;
  
  for(i=0;i<OUTPUT_8_NUMBER+EXTIO_NUM;i++)
  {
    if(OUTPUT_NONE_ACTIVE == 0)
      digitalio.Output[i] = 0;
    else
      digitalio.Output[i] = 0xFF;
  }
    
  for(i=0; i<INPUT_8_NUMBER*8; i++)
  {
    pinMode(InputPin[i], INPUT);
  }
  for(i=0; i<OUTPUT_8_NUMBER*8; i++)
  {
    pinMode(OutputPin[i], OUTPUT);  
  }
  
  for(j=0; j<EXTIO_NUM; j++)//0 1
  {
    extio[j].begin(j);      // Default device address 0x20+j //0x20，0x21

    for(i=0; i<8; i++)
    {
      extio[j].pinMode(i, OUTPUT);  // Toggle LED 1
      extio[j].digitalWrite(i, OUTPUT_NONE_ACTIVE);
    }
  }
  for(i=0; i<OUTPUT_8_NUMBER*8; i++)
    digitalWrite(OutputPin[i], OUTPUT_NONE_ACTIVE);

  for(j=0; j<EXTIO_NUM; j++)
    for(i=0; i<8; i++)
    {
      extio[j].pinMode(i+8,INPUT);   // Button i/p to GND
      extio[j].pullUp(i+8,HIGH);   // Puled high to ~100k
    }
}


void ReadDigitalInput()
{
  uint8_t i,bi, j, value;
  String outstr = "";
  bool inputupdate = false;
  uint8_t input8 = 1;
  
  for(i=0; i<8; i++)
  {
    runtimedata.sensor[i] = digitalRead(InputPin[i]);
    if(runtimedata.sensor[i])
      {setbit(digitalio.Input[0], i); }
    else
      {clrbit(digitalio.Input[0], i); }
  }

  if(INPUT_8_NUMBER == 2)
  {
    for(i=0; i<8; i++)
    {
      runtimedata.sensor[i+8] = digitalRead(InputPin[i+8]);
      
      if(runtimedata.sensor[i+8])
        {setbit(digitalio.Input[1], i); }
      else
        {clrbit(digitalio.Input[1], i); }
    }
    input8 += 1;
  }

  if(EXTIO_NUM > 0)
  {
    for(i=0; i<8; i++)
    {
      runtimedata.sensor[i+8] = extio[0].digitalRead(i+8);
        
      if(runtimedata.sensor[i+input8*8])
        {setbit(digitalio.Input[input8], i);  }
      else
        {clrbit(digitalio.Input[input8], i);  }
    }
    input8 += 1;
  }
  if(EXTIO_NUM > 1)
  {
    for(i=0; i<8; i++)
    {
      runtimedata.sensor[i+input8*8] = extio[1].digitalRead(i+8);
      if(runtimedata.sensor[i+input8*8])
        {setbit(digitalio.Input[input8], i);  }
      else
        {clrbit(digitalio.Input[input8], i);  }
    }
  }

}

void WriteDigitalOutput()
{
  uint8_t i,bi, j, value;

  for(i=0; i<OUTPUT_8_NUMBER+EXTIO_NUM; i++)
  {
    if(digitalio.PreOutput[i] != digitalio.Output[i])
    {
      digitalio.PreOutput[i] = digitalio.Output[i];
      switch(i)
      {
        case 0: //onboard
          for(bi=0; bi<8; bi++)
          {
            value = getbit(digitalio.Output[i], bi);
            digitalWrite(OutputPin[bi], value);
          }
          break;

        case 1: //extern board 0
          for(bi=0; bi<8; bi++)
          {
            value = getbit(digitalio.Output[i], bi);
            if(OUTPUT_8_NUMBER == 2)
              digitalWrite(OutputPin[bi+8], value);
            else
              extio[0].digitalWrite(bi, value);
          }
          break;
        case 2: //extern board 1
          for(bi=0; bi<8; bi++)
          {
            value = getbit(digitalio.Output[i], bi);
            if(OUTPUT_8_NUMBER == 2)
              extio[0].digitalWrite(bi, value);
            else
              extio[1].digitalWrite(bi, value);
          }
          break;
        case 3: //extern board 1
          for(bi=0; bi<8; bi++)
          {
            value = getbit(digitalio.Output[i], bi);
            extio[1].digitalWrite(bi, value);
          }
          break;
      } 
    }
  }
}

void OutputBuf2Byte()
{
  uint8_t i, bi, byteindex;
  for(i=0; i< (OUTPUT_8_NUMBER+EXTIO_NUM)*8; i++)
  {
    byteindex = i / 8;
    bi = i % 8;
    if(runtimedata.outbuf[i] == 1)
      setbit(digitalio.Output[byteindex], bi);
    else
      clrbit(digitalio.Output[byteindex], bi);
  }
}
void setOutput(uint8_t index, uint8_t hl)
{
	if(index < (8))
	{
		digitalWrite(OutputPin[index], hl);
	}
	else
	{
		uint8_t extindex = index-(8);
		uint8_t exi = extindex >> 3;
		uint8_t bi = extindex & 0x07;
		extio[exi].digitalWrite(bi, hl);
	}
	digitalio.Output[index] = hl;
}

uint8_t getInput(uint8_t index)
{
	uint8_t hl;
	if(index < (INPUT_8_NUMBER*8))
	{
		hl = digitalRead(InputPin[index]);
	}
	else
	{
		uint8_t extindex = index-(INPUT_8_NUMBER*8);
		uint8_t exi = extindex >> 3;
		uint8_t bi = extindex & 0x07;
		hl = extio[exi].digitalRead(bi+8);
	}

	digitalio.Input[index] = hl;
	return hl;
}

void MainProcess_Task()  
{
//    WriteDigitalOutput();
//    ReadDigitalInput();
}
void buzzerPlay(int playMS)
{
	pinMode(BUZZ, OUTPUT);
	digitalWrite(BUZZ, HIGH);
	delay(playMS);
	digitalWrite(BUZZ, LOW);
}

