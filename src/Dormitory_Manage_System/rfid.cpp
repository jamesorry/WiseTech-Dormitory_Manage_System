#include "Arduino.h"
#include "SPI.h"//必須要設定
#include "MFRC522.h"//必須要設定
#include "hmi.h"
#include "rfid.h"
#include "Timer.h"
#include "MainProcess.h"
#include "HMI_Command.h"

#define RFID_DEBUG 1

extern HardwareSerial *cmd_port;
extern MainDataStruct maindata;
extern RuntimeStatus runtimedata;
extern HMI_Command *hmicmd;
MFRC522 mfrc522(SS_PIN, RST_PIN);//必須要設定
RFIDData rfiddata;

/*
板子<====>RFID模組接線方法
SDA------->SDA
SCK------->SCK
MOSI------>MOSI
MISO------>MISO
IRQ------->不用接
GND------->GND
RST------->RST
3.3V------>3.3V
*/

void RFID_Init(void)
{
  SPI.begin();  //必須要設定
  mfrc522.PCD_Init();//必須要設定
  rfiddata.Len = 0;
  rfiddata.Update =false;
  rfiddata.retrytimecnt = 0xFF00;
  rfiddata.ProcessIndex = 0xFF;
  rfiddata.ProcessTimeCnt = 0;
  cmd_port->println("RFID_Init!");
}
void RFID_Reset(void)
{
  mfrc522.PCD_Reset();
  mfrc522.PCD_Init();
}

void RFID_Read()
{
	rfiddata.ProcessIndex = 0;
	rfiddata.Update = false;
	rfiddata.Len = 4;
	for(uint8_t i=0; i<rfiddata.Len; i++)
		rfiddata.Data[i] = 0x00;
}

void RFID_Process(void)
{
    if(rfiddata.preProcessIndex != rfiddata.ProcessIndex){
        rfiddata.preProcessIndex = rfiddata.ProcessIndex;
#if RFID_DEBUG    
        cmd_port->println("ProcessIndex: " + String(rfiddata.ProcessIndex));
#endif
    }
	switch(rfiddata.ProcessIndex)
	{
		case 0:
		{
#if RFID_DEBUG    
			cmd_port->println("Start of RFID_Process().");
#endif     
			mfrc522.PCD_SoftPowerUp();
			rfiddata.ProcessIndex ++;
			rfiddata.retrytimecnt = 0;
			break;
		}
		case 1:
		{
			if(rfiddata.retrytimecnt > runtimedata.TimeoutSecond*1000)
			{
#if RFID_DEBUG    
				cmd_port->println("Read RFID Fail.");
#endif     
				rfiddata.ProcessIndex = 0x0E;
			}
			else
			{
				if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) 
				{
					//已讀取到RFID
					digitalWrite(BUZZ, HIGH);
					rfiddata.ProcessTimeCnt = 0;
					rfiddata.ProcessIndex ++;
				}
			}
			break;
		}
		case 2:
		{
			if(rfiddata.ProcessTimeCnt > 300)
			{
				digitalWrite(BUZZ, LOW);
				rfiddata.ProcessIndex ++;
			}
			break;
		}
		case 3:
		{
		    byte *id = mfrc522.uid.uidByte;   // 取得卡片的UID
		    byte idSize = mfrc522.uid.size;   // 取得UID的長度

		    // 根據卡片回應的SAK值（mfrc522.uid.sak）判斷卡片類型
		    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
#if RFID_DEBUG    
				cmd_port->print("PICC type: ");	   // 顯示卡片類型
		    	cmd_port->println(mfrc522.PICC_GetTypeName(piccType));
#endif
		    for (byte i = 0; i < idSize; i++)
		      rfiddata.Data[i] = id[i];
			rfiddata.Len = idSize;
		    rfiddata.Update = true;
#if RFID_DEBUG
		    cmd_port->print("RFID: ");       // 顯示卡片的UID長度值
		    for (byte i = 0; i < idSize; i++) {  // 逐一顯示UID碼
		      cmd_port->print(id[i], HEX);       // 以16進位顯示UID值
		    }
		    cmd_port->println();
#endif 
		    mfrc522.PICC_HaltA();  // 讓卡片進入停止模式    
			rfiddata.ProcessIndex = 0x0E;
  			break;
		}
		case 0x0E:
		{
			mfrc522.PCD_SoftPowerDown();
			rfiddata.ProcessIndex = 0xFF;
			hmicmd->Response_Get_RFID();
#if RFID_DEBUG    
			cmd_port->println("End of RFID_Process().");
#endif     
			break;
		}
	}
}

void RFID_Timer()
{
	if(rfiddata.retrytimecnt < 0xFF00)
		rfiddata.retrytimecnt += TIMER_INTERVAL_MS;
	if(rfiddata.ProcessTimeCnt < 0xFF00)
		rfiddata.ProcessTimeCnt += TIMER_INTERVAL_MS;
}

