#include <Arduino.h>
#include "rfid.h"
#include "Adafruit_MCP23017.h"
#include "MainProcess.h"
#include "hmi.h"
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
	MainProcess_Init();
    TimerInit(1, 10000);
	RFID_Init();
    cmd_port->println("Version:" + String(VERSTR));
    cmd_port->println("End of setup.");
    buzzerPlay(300);
}

void loop() {
    hmicmd->Process();
	RFID_Process();
	MainProcess_Task();
	UserCommand_Task();
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
