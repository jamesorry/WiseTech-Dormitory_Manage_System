#include "rfid.h"
#include <SPI.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>

#define BUZZ 48
#define needReadRfid 1

struct RFIDTag {    // 定義結構
   byte uid[4];
   char *name;
};
struct RFIDTag tags[] = {  // 初始化結構資料
   {{0x2B,0xED,0x9B,0xD}, ""},   
};
 
byte totalTags = sizeof(tags) / sizeof(RFIDTag);
 
MFRC522 mfrc522(SS_PIN, RST_PIN);  // 建立MFRC522物件
String CardID = "";
String Str_Str = "";
char str1[25];

void RFID_Init(void)
{
  SPI.begin();
  mfrc522.PCD_Init();
}
//20211201 可以印出RFID card 的 hex string 
void doRfidProcess(void)
{
	// 確認是否有新卡片
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      byte *id = mfrc522.uid.uidByte;   // 取得卡片的UID
      byte idSize = mfrc522.uid.size;   // 取得UID的長度
      bool foundTag = false;            // 是否找到紀錄中的標籤，預設為「否」。
      CardID = "";
      Str_Str = "";
      buzzerPlay(200);
#if needReadRfid
	  for (byte i = 0; i < idSize; i++)
      {  // 逐一顯示UID碼
        sprintf(str1, "%02X", id[i]);
        Str_Str = Str_Str + String(str1);// 以16進位顯示UID
	  }Serial.println(Str_Str);
#endif

#if 0
      for (byte i=0; i<totalTags; i++) {
        if (memcmp(tags[i].uid, id, idSize) == 0) {
          Serial.println(tags[i].name);  // 顯示標籤的名稱
          foundTag = true;  // 設定成「找到標籤了！」
          buzzerPlay(200);
          break;            // 退出for迴圈
        }
      }
 
      if (!foundTag) {    // 若掃描到紀錄之外的標籤，則顯示"Wrong card!"。
        Serial.println("Wrong card!");
      }
#endif
    
      mfrc522.PICC_HaltA();  // 讓卡片進入停止模式
      delay(500);
    } 
}

void buzzerPlay(int playMS){
  pinMode(BUZZ, OUTPUT);
  digitalWrite(BUZZ, HIGH);
  delay(playMS);
  digitalWrite(BUZZ, LOW);
}



