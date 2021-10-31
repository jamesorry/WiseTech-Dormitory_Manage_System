#include <Arduino.h>
#include "rfid.h"
#include "Adafruit_MCP23017.h"
#include "MainProcess.h"
#include "HMI.h"
#include <SPI.h>
#include "Timer.h"
#include "UserCommand.h"
#include "EEPROM_Function.h"
#include "HMI_Command.h"

extern RuntimeStatus runtimedata;
extern MainDataStruct maindata;

HMI_Command *hmicmd;
HardwareSerial *cmd_port;

void setup() {
	cmd_port = &CMD_PORT;
	cmd_port->begin(CMD_PORT_BR);
	hmicmd = new HMI_Command(&HMI_CMD_PORT, HMI_CMD_PORT_BR);
    
	READ_EEPROM();
	TimerInit(1, 10000);
	RFID_Init();
	MainProcess_Init();
	buzzerPlay(500);
}

void loop() {
	RFID_Process();
	MainProcess_Task();
	UserCommand_Task();
    hmicmd->Process();
	if(runtimedata.UpdateEEPROM)
	{
		runtimedata.UpdateEEPROM = false;
		WRITE_EEPROM();
	}
}

ISR(TIMER1_COMPA_vect)//10ms
{
	MainProcess_Timer();
    RFID_Timer();
}

