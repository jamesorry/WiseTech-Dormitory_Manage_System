#include "Arduino.h"
#include <SoftwareSerial.h>
#include "HMI_Command.h"
#include "cppQueue.h"
#include "MainProcess.h"
#include <Adafruit_MCP23017.h>
#include "rfid.h"
#include "hmi.h"
extern "C" {
	#include <string.h>
	#include <stdlib.h>
}

#define HMI_CMD_DEBUG		1
extern DigitalIO digitalio;
extern Adafruit_MCP23017 extio[EXTIO_NUM];
extern HardwareSerial *cmd_port;
extern MainDataStruct maindata;
extern RuntimeStatus runtimedata;
uint8_t resopnsebuf[32], resopnselen = 0;
extern RFIDData rfiddata;

HMI_Command::HMI_Command(HardwareSerial* port, const uint32_t baudrate, const uint8_t reqid, const uint8_t resid, const bool skipid)
{
	hmicmd_port = port;
	((HardwareSerial*)hmicmd_port)->begin(baudrate);

	ControllerTagID = reqid;
	ResponseTagID = resid;
	skipHMI_ID = skipid;

#if HMI_CMD_DEBUG	
	cmd_port->println("HMI_Command().");
#endif
	Update = false;
	reclen = 0;
	DataBuffLen = 0;

	cmdQueue = new Queue(sizeof(HMICmdRec), 10, FIFO);
#if HMI_CMD_DEBUG	
		cmd_port->println("init Q cnt: " + String(cmdQueue->getCount()));
#endif
	cmdRec.datalen = 0;
	ProcessIndex = 0;
	ProcessTimeCnt = 0;
}

int qlen = 0xff;
void HMI_Command::SendCommandQ(void)
{
#if 0//HMI_CMD_QUEUE_DEBUG
	if(qlen != cmdQueue->getCount())
	{
		qlen = cmdQueue->getCount();
		cmd_port->println("Q Cnt:" + String(qlen, DEC) + " is empty: " + cmdQueue->isEmpty());
		if(qlen > 100)
			cmdQueue->clean();
	}
#endif

	if((millis() - SendCmdTimeCnt) >= TIME_HMI_CMD_INTERVAL)
	{
		if(cmdQueue->isEmpty() == false)
		{
			if(!cmdQueue->pop(&cmdRec))
			{
				cmdRec.datalen = 0;
			}
		}
		if(cmdRec.datalen > 0)
		{
			bool available = true;
			if(!SWSerial)
				available = ((HardwareSerial *)hmicmd_port)->availableForWrite();
			if(available)
			{
				hmicmd_port->write(cmdRec.data, cmdRec.datalen);
#if HMI_CMD_DEBUG
			cmd_port->println("cmdRec.datalen: " + String(cmdRec.datalen));
			cmd_port->print("HMI_Command::SendCommandQ: ");
			for(int i=0; i<cmdRec.datalen; i++)
				cmd_port->print(String(cmdRec.data[i], HEX)+ ",");
			cmd_port->println();
#endif
				if(cmdRec.datatype == QUEUE_DATA_TYPE_RESPONSE)
					cmdRec.datalen = 0;
				else
				{
					if(cmdRec.retrycnt < HMI_CMD_RETRY_MAX)
						cmdRec.retrycnt ++;
					else
						cmdRec.datalen = 0;
				}
				SendCmdTimeCnt = millis();
			}
		}
	}

}

bool HMI_Command::SplitRecvice(void)
{
	int i, starti=-1, endi=-1;
	bool result = false;
	if(DataBuffLen >= 129)	
		DataBuffLen = 0;
    
	if(DataBuffLen >= HMI_CMD_LEN_BASE)
	{
		cmd_port->println("DataBuffLen: " + String(DataBuffLen));
		for(i=0; i<=DataBuffLen-HMI_CMD_LEN_BASE; i++)
			if((DataBuff[i] == ControllerTagID) || (DataBuff[i] == ResponseTagID) || skipHMI_ID)
			{
                cmd_port->println("DataBuffLen i: " + String(i));
				if(((DataBuff[i+1] < 32) && (DataBuff[i+1] >= HMI_CMD_LEN_BASE)))
				{
					bool match = true;
                    cmd_port->println("check match.");
					if(match)
					{
					    cmd_port->println("match.");
						starti = i;
						break;
					}
				}
			}
		if(starti > -1)
		{
			endi = DataBuff[HMI_CMD_BYTE_LENGTH+starti] + starti;
			if(DataBuffLen >= endi)
			{
#if HMI_CMD_DEBUG	
					cmd_port->println("SplitRecvice Datlen: " + String(DataBuffLen) +", Starti: " + String(starti));
					cmd_port->println("Len: " + String(DataBuff[HMI_CMD_BYTE_LENGTH]) + ", Endi: " + String(endi));
#endif
				memcpy(recdata, &DataBuff[starti], DataBuff[HMI_CMD_BYTE_LENGTH+starti]);
				reclen = DataBuff[HMI_CMD_BYTE_LENGTH+starti];
#if HMI_CMD_DEBUG	
                    cmd_port->println("reclen: " + String(reclen));
					cmd_port->println("SplitRecvice: ");
					for(i=0; i<reclen; i++)
						cmd_port->print(String(recdata[i], HEX) + " ");
					cmd_port->println();
                    cmd_port->println("DataBuffLen-(reclen+starti): " + String(DataBuffLen-(reclen+starti)));
#endif
				for(i=0; i<DataBuffLen-(reclen+starti); i++)
					DataBuff[i] = DataBuff[endi+i];
				DataBuffLen -= (reclen+starti);
				result = true;
#if HMI_CMD_DEBUG	
					cmd_port->println("SplitRecvice result: " + String(result));
					cmd_port->print("HIM Data Buff (" + String(DataBuffLen) + String("): ") );
					for(i=0; i<DataBuffLen; i++)
						cmd_port->print(String(DataBuff[i], HEX)+ String(","));
					cmd_port->println();
#endif
			}
		}
	}

	if(DataBuffLen > 0)
		if(!result && ((millis() - ReciveTime) > TIME_RECIVE_DATA_OVERDUE))
		{
#if HMI_CMD_DEBUG	
				cmd_port->println("millis: " + String(millis()) + ", ReciveTime: " + String(ReciveTime));
				cmd_port->println("Clear DataBuff(" + String(DataBuffLen) + ")");
#endif
			DataBuffLen = 0;
		}
	return result;
}

void HMI_Command::Process(void)
{
	static uint8_t preProcessIndex = 0xff;
	int ret,i,inputCount=0;
	int retry=0;
#if HMI_CMD_DEBUG
	if(preProcessIndex != ProcessIndex)
	{
		preProcessIndex = ProcessIndex;
		cmd_port->println("ProcessIndex: " + String(ProcessIndex, DEC));
	}
#endif
	switch(ProcessIndex)
	{
		case 0:
		{
			if(hmicmd_port->available())
			{
#if HMI_CMD_DEBUG	
				cmd_port->println("hmicmd_port->available()");
#endif
				ProcessTimeCnt = millis();
				cmd_port->println("ProcessTimeCnt: " + String(ProcessTimeCnt));
				DataBuffLen = 0;
                reclen = 0;
				ProcessIndex ++;
			}
			break;
		}
		case 1:
		{
			if((millis() - ProcessTimeCnt) > TIME_RECIVE_DATA_DELAY)
			{
#if HMI_CMD_DEBUG	
				cmd_port->println("delay " + String(TIME_RECIVE_DATA_DELAY) +" ms");
#endif
				ProcessIndex ++;
			}
			break;
		}
		case 2:
		{
			while(hmicmd_port->available() && (DataBuffLen < HMI_CMD_DATA_MAX_LEN))
			{
				ret = hmicmd_port->read();
				DataBuff[DataBuffLen++] = (char)ret;
//				recdata[reclen++] = (char)ret;
			}
			MsgUpdate = true;
#if HMI_CMD_DEBUG	
				cmd_port->print("HMI CMD recive (" + String(DataBuffLen) + String("): ") );
				for(i=0; i<DataBuffLen; i++)
					cmd_port->print(String(DataBuff[i], HEX)+ String(","));
				cmd_port->println();
#endif
			ReciveTime = millis();
			ProcessIndex = 0;
			break;
		}
	}
	while(SplitRecvice())
		CheckReciveData();
	SendCommandQ();
}

uint8_t HMI_CMD_ComputeCRC(uint8_t *buff)
{
	uint8_t cmp_crc = 0x00, i;
	for(i=0; i<buff[HMI_CMD_BYTE_LENGTH]-1; i++)
		cmp_crc -= buff[i];
	return (uint8_t)(cmp_crc & 0xFF);
}

bool HMI_Command::Response_Ping()
{
	uint8_t i;
	HMICmdRec rec;
	rec.datatype = QUEUE_DATA_TYPE_RESPONSE;
	rec.data[HMI_CMD_BYTE_TAGID] = ResponseTagID;
	rec.data[HMI_CMD_BYTE_LENGTH] = HMI_CMD_LEN_BASE;
	rec.data[HMI_CMD_BYTE_CMDID] = HMI_CMD_PING;
	rec.data[rec.data[HMI_CMD_BYTE_LENGTH]-1] = HMI_CMD_ComputeCRC(rec.data);
	rec.datalen = rec.data[HMI_CMD_BYTE_LENGTH];
	rec.retrycnt = 0;
	cmdQueue->push(&rec);

#if HMI_CMD_DEBUG
	cmd_port->println("Response_Ping()");
#endif
	return true;
}
//receive: FC 09 01 F1 F2 F3 F4 F5 CRC
bool HMI_Command::Response_Set_DO_State()
{
    int i;
	uint8_t bytei, result = true,num;
	uint8_t datalen = recdata[HMI_CMD_BYTE_LENGTH] - HMI_CMD_LEN_BASE;//5
    
	HMICmdRec rec;
	rec.datatype = QUEUE_DATA_TYPE_RESPONSE;
	rec.data[HMI_CMD_BYTE_TAGID] = ResponseTagID;
	rec.data[HMI_CMD_BYTE_LENGTH] = HMI_CMD_LEN_BASE + 1;
	rec.data[HMI_CMD_BYTE_CMDID] = HMI_CMD_SET_DO_STATE;
    rec.data[HMI_CMD_BYTE_DATA] = result;
	rec.data[rec.data[HMI_CMD_BYTE_LENGTH]-1] = HMI_CMD_ComputeCRC(rec.data);
	rec.datalen = rec.data[HMI_CMD_BYTE_LENGTH];
	cmdQueue->push(&rec);
    
#if HMI_CMD_DEBUG
    cmd_port->println("datalen: " + String(datalen));//5
	cmd_port->println("Set DO: ");
#endif
	//無法使用setOutput()會使板子當機，尚未找到原因
	//-->已找到問題原因==>DigitalIO buffer size
	for(bytei=0; bytei<datalen; bytei++)//0~4
	{
#if HMI_CMD_DEBUG
		cmd_port->print("Set Output(" +String(bytei) + "): " + String(recdata[HMI_CMD_BYTE_DATA + bytei],HEX) + " ");
        cmd_port->print("->");
#endif
		for(i=7; i>=0; i--)
		{
		    cmd_port->print(getbit(recdata[HMI_CMD_BYTE_DATA + bytei], i));
            cmd_port->print(",");
			setOutput(bytei*8+(7-i), getbit(recdata[HMI_CMD_BYTE_DATA + bytei], i));
        }
        cmd_port->println("");
	}
#if HMI_CMD_DEBUG
        cmd_port->println("Response_Set_DO_State()");
#endif
	return true;
}

bool HMI_Command::Response_IO_Status()
{
	int i;
	uint8_t hl = 0;
	uint8_t bytei = 0;
	HMICmdRec rec;
	rec.datatype = QUEUE_DATA_TYPE_RESPONSE;
	rec.data[HMI_CMD_BYTE_TAGID] = ResponseTagID;
	rec.data[HMI_CMD_BYTE_CMDID] = HMI_CMD_IO_STATUS;
    rec.data[HMI_CMD_BYTE_LENGTH] = HMI_CMD_LEN_BASE + 1 + INPUT_8_NUMBER+EXTIO_NUM;

    if(recdata[HMI_CMD_BYTE_DATA] == 0)	//DI
	{
		rec.data[HMI_CMD_BYTE_DATA] = 0;	//0:Input
#if HMI_CMD_DEBUG
        cmd_port->print("Input: ");
#endif
		for(bytei=0; bytei<(INPUT_8_NUMBER + EXTIO_NUM); bytei++)
		{
			rec.data[HMI_CMD_BYTE_DATA+1+bytei] = 0;
            for(i=0; i<8; i++)
//            for(i=7; i>=0; i--)
			{
				hl = (getInput(bytei*8+i) & 0x01);
				//無法使用getInput()會使板子當機，尚未找到原因
				//-->已找到問題原因==>DigitalIO buffer size
				rec.data[HMI_CMD_BYTE_DATA+1+bytei] |=  (hl << (7-i));
				#if HMI_CMD_DEBUG
					cmd_port->print(String(hl) + " ");
				#endif
			}
#if HMI_CMD_DEBUG
			cmd_port->println(": " + String(rec.data[HMI_CMD_BYTE_DATA+1+bytei], HEX));
#endif
		}
	}
	else if(recdata[HMI_CMD_BYTE_DATA] == 1)	//DO
	{
		rec.data[HMI_CMD_BYTE_DATA] = 1;	//1:Output
#if HMI_CMD_DEBUG
		cmd_port->print("Output: ");
#endif
		for(bytei=0; bytei<(OUTPUT_8_NUMBER + EXTIO_NUM); bytei++)
		{
			rec.data[HMI_CMD_BYTE_DATA+1+bytei] = 0;
//			for(i=7; i>=0; i--)
			for(i=0; i<8; i++)
            {
				rec.data[HMI_CMD_BYTE_DATA+1+bytei] |= digitalio.Output[bytei*8+i] << (7-i);
				cmd_port->print(String(digitalio.Output[bytei*8+i]) + " ");
			}
#if HMI_CMD_DEBUG
			cmd_port->println(": " + String(rec.data[HMI_CMD_BYTE_DATA+1+bytei], HEX));
#endif
		}
	}
    
	rec.data[rec.data[HMI_CMD_BYTE_LENGTH]-1] = HMI_CMD_ComputeCRC(rec.data);
	rec.datalen = rec.data[HMI_CMD_BYTE_LENGTH];
	rec.retrycnt = 0;
	cmdQueue->push(&rec);
#if HMI_CMD_DEBUG
            cmd_port->println("Response_IO_Status()");
#endif
	return true;
}

bool HMI_Command::Response_Get_RFID()
{
	uint8_t i, result;
#if 0
	//獲取到timeout
    uint8_t timeout_second = recdata[HMI_CMD_BYTE_DATA];
    cmd_port->println("timeout_second: " + String(timeout_second));
    rfiddata.Len = 4;
	for(i=0; i<rfiddata.Len; i++)
		rfiddata.Data[i] = 0x00;	
	rfiddata.retrytimecnt = 0;
	RFID_Read();
#endif
	if(rfiddata.Update == true)
		result = 0x00; //pass
	else
		result = 0x01; //fail
	HMICmdRec rec;
	rec.datatype = QUEUE_DATA_TYPE_RESPONSE;
	rec.data[HMI_CMD_BYTE_TAGID] = ResponseTagID;
	rec.data[HMI_CMD_BYTE_LENGTH] = HMI_CMD_LEN_BASE + 1 + rfiddata.Len;
	rec.data[HMI_CMD_BYTE_CMDID] = HMI_CMD_GET_RFID;
	rec.data[HMI_CMD_BYTE_DATA] = result;
	for(i=0; i<rfiddata.Len; i++)
		rec.data[HMI_CMD_BYTE_DATA + 1 + i] = rfiddata.Data[i];
	rec.data[rec.data[HMI_CMD_BYTE_LENGTH]-1] = HMI_CMD_ComputeCRC(rec.data);
	rec.datalen = rec.data[HMI_CMD_BYTE_LENGTH];
	rec.retrycnt = 0;
	cmdQueue->push(&rec);

#if HMI_CMD_DEBUG
	cmd_port->println("Response_Get_RFID()");
#endif
	runtimedata.TimeoutSecond = 1;
	return true;
}

uint8_t HMI_Command::CheckReciveData()
{
	uint8_t rec_crc, cmp_crc = 0x00, i;
	bool issupportcmd = false;
	if(reclen < HMI_CMD_LEN_BASE)
	{
#if HMI_CMD_DEBUG	
		cmd_port->println("CheckReciveData reclen: " + String(reclen) + " Exit.");
#endif
		return -1;
	}
#if HMI_CMD_DEBUG	
		cmd_port->println("CheckReciveData reclen: " + String(reclen));
#endif
	if(reclen == recdata[HMI_CMD_BYTE_LENGTH])
	{
#if HMI_CMD_DEBUG	
		cmd_port->println("CheckReciveData reclen: " + String(reclen));
#endif

		rec_crc = recdata[recdata[HMI_CMD_BYTE_LENGTH] - 1];
		cmp_crc = HMI_CMD_ComputeCRC(recdata);
		if(cmp_crc != rec_crc)
		{
#if HMI_CMD_DEBUG	
			cmd_port->println("CRC Fail " + String(cmp_crc, HEX));
#endif
			return -1;
		}

#if HMI_CMD_DEBUG	
	cmd_port->println("cmdRec.datatype: " + String(cmdRec.datatype, HEX) + ", cmd: " + String(recdata[HMI_CMD_BYTE_CMDID], HEX));
#endif
		if((cmdRec.datalen > 0)
			&& (cmdRec.datatype == QUEUE_DATA_TYPE_INDICATION)
			&& (cmdRec.data[HMI_CMD_BYTE_CMDID] != recdata[HMI_CMD_BYTE_CMDID])
			)
		{
			if(cmdRec.retrycnt < HMI_CMD_RETRY_MAX)
			{
				cmdRec.last_ms = millis();
				cmdQueue->push(&cmdRec);
			}
			cmdRec.datalen = 0;
		}
		else
		{
			if(cmdRec.datalen > 0)
			{
				if(cmdRec.retrycnt < HMI_CMD_RETRY_MAX)
				{
					cmdRec.last_ms = millis();
					cmdQueue->push(&cmdRec);
				}
				cmdRec.datalen = 0;
			}
#if HMI_CMD_DEBUG	
	cmd_port->println("HMI_CMD: " + String(recdata[HMI_CMD_BYTE_CMDID], HEX));
#endif
			
			resopnsebuf[HMI_CMD_BYTE_TAGID] = ResponseTagID;

			switch(recdata[HMI_CMD_BYTE_CMDID])
			{
				case HMI_CMD_PING:
					issupportcmd = Response_Ping();
					cmd_port->println("HMI_CMD_PING.");
					break;
                case HMI_CMD_SET_DO_STATE:
                    issupportcmd = Response_Set_DO_State();
                    cmd_port->println("HMI_CMD_SET_DO_STATE.");
                    break;
                case HMI_CMD_IO_STATUS:
                    issupportcmd = Response_IO_Status();
                    cmd_port->println("HMI_CMD_IO_STATUS.");
                    break;
                case HMI_CMD_GET_RFID:
                    cmd_port->println("HMI_CMD_GET_RFID.");
					runtimedata.TimeoutSecond = recdata[HMI_CMD_BYTE_DATA];
					cmd_port->println("timeout_second: " + String(runtimedata.TimeoutSecond));
					RFID_Read();
                    break;
			}
			if(issupportcmd)
			{
				cmdRec.datalen = 0;
			}	
		}
	}
	else
	{
#if HMI_CMD_DEBUG
		if(reclen != recdata[HMI_CMD_BYTE_LENGTH])
			cmd_port->println("Leng fail (" + String(reclen, DEC) + " != " + String(recdata[HMI_CMD_BYTE_LENGTH]) + String(")"));
#endif
		return -1;
	}
	
}
