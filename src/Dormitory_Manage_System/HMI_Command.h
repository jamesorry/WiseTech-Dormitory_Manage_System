#ifndef _HMI_CMD_BT_COMMAND_H_
#define _HMI_CMD_BT_COMMAND_H_

#include "Arduino.h"
#include "cppQueue.h"

#define HMI_CMD_DATA_MAX_LEN 128+1

#define TIME_RECIVE_DATA_DELAY		50
#define TIME_RECIVE_DATA_OVERDUE	1000

typedef struct {
	uint8_t datatype;
	uint8_t retrycnt;
	uint8_t datalen;
	uint8_t data[40];

	uint32_t last_ms;
} HMICmdRec;



class HMI_Command
{
private:
	Queue	*cmdQueue;		//!< Queue implementation: FIFO LIFO
	HMICmdRec cmdRec;

	uint8_t	ProcessIndex;
	uint16_t ProcessTimeCnt;
	
	Stream *hmicmd_port = NULL;

	bool SWSerial = false;
	bool Update;

	uint32_t SendCmdTimeCnt = 0;
	uint32_t	ReciveTime;

	bool skipHMI_ID;
	uint8_t recdata[HMI_CMD_DATA_MAX_LEN];
	uint8_t reclen;
	uint8_t DataBuff[HMI_CMD_DATA_MAX_LEN];
	uint8_t DataBuffLen;

public:
	
		uint8_t 	ControllerTagID;
		uint8_t 	ResponseTagID;
		uint8_t 	Receive_HMI_CMD;
		bool 		MsgUpdate;
        HMI_Command(HardwareSerial* port, const uint32_t baudrate=115200, const uint8_t reqid=0xFC, const uint8_t resid=0xFD, const bool skipid=false);
        ~HMI_Command();
	
	void Process(void);
	void Recive_Message();
	void Response_Message();
	void Response_CRC_Fail(uint8_t crc);
	bool SplitRecvice();
	uint8_t CheckReciveData();
	void SendCommandQ();

	bool Response_Ping();
    bool Response_Set_DO_State();
    bool Response_IO_Status();
    bool Response_Get_RFID();
};

#define HMI_CMD_LEN_BASE			4

#define QUEUE_DATA_TYPE_RESPONSE	0
#define QUEUE_DATA_TYPE_INDICATION	1

#define HMI_CMD_RETRY_MAX	        0


#define HMI_CMD_PING				    0x00
#define HMI_CMD_SET_DO_STATE            0x01
#define HMI_CMD_IO_STATUS               0x02
#define HMI_CMD_GET_RFID                0x03

#define HMI_CMD_BYTE_TAGID				0
#define HMI_CMD_BYTE_LENGTH				1
#define HMI_CMD_BYTE_CMDID				2
//#define HMI_CMD_BYTE_HMIID				3
#define HMI_CMD_BYTE_DATA				3


#define HMI_CMD_TIME_RESEND_TIMEOUT		10000
#define TIME_HMI_CMD_INTERVAL			400

uint8_t HMI_CMD_ComputeCRC(uint8_t *buff);

#endif	//_HMI_CMD_BT_COMMAND_H_
