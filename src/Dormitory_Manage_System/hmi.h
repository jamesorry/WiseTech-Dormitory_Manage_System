#ifndef _HMI_H_
#define _HMI_H_

#define VERSTR "2021111201"
#define VENDOR "LH"

#include "SoftwareSerial.h"
    const byte rxPin = 10;
    const byte txPin = 9;
inline SoftwareSerial Serial4(rxPin, txPin);

#define setbit(value,x) (value |=(1<<x))
#define getbit(value,x) ((value>>x)&1)
#define clrbit(value,x) (value &=~(1<<x))
#define revbit(value,x) (value ^=(1<<x))

#define RST_PIN      42        // RFID_RST 腳位 
#define MISO         50        // MISO 腳位
#define M0SO         51        // MOSO 腳位
#define SCK          52        // SCK  腳位
#define SS_PIN       53        // SS   腳位

#define FLASH_CS     47        // FLASH ROM CS PIN
#define BUZZ         48 
#define BT_PWRC      49        // BT4.2 

#define PWM_2         2        // MPU 直接產生
#define PWM_3         3        // MPU 直接產生
#define PWM_6         6        // MPU 直接產生
#define PWM_7         7        // MPU 直接產生
#define PWM_8         8        // MPU 直接產生
#define PWM_11       11        // MPU 直接產生
#define PWM_12       12        // MPU 直接產生
#define PWM_44       44        // MPU 直接產生
#define PWM_45       45        // MPU 直接產生

#define	OUTPUT_TOTAL	8
#define	INPUT_TOTAL		16


static const uint8_t OutputPin[] = {A0, A1, A2, A3, A4, A5, A6, A7,
                  A8, A9, A10, A11, A12, A13, A14, A15};
static const uint8_t InputPin[] = {22, 23, 24, 25, 26, 27, 28, 29,
                     30, 31, 32, 33, 34, 35, 36, 37};
static const uint8_t ADC_PWMPin[] = {0, 0, 0, A8, 2, A9, 3, A10, 6, A11, 7, A12,
									11, A13, 12, A14, 44, A15, 45, 0, 0};

#define CMD_PORT        Serial
#define CMD_PORT_BR     115200
#define	HMI_CMD_PORT		Serial3
#define	HMI_CMD_PORT_BR		115200
#endif //_HMI_H_
