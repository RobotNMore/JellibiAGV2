#include <deprecated.h>
#include <MFRC522.h>
#include <MFRC522Extended.h>
#include <require_cpp11.h>
#include <SPI.h>

#define RFID_SS_PIN     2 // SPI 통신용 SS(Slave Select)
#define RFID_RST_PIN    4 // RESET 핀

MFRC522 mfrc522( RFID_SS_PIN, RFID_RST_PIN ); // 객체 인스턴스 선언

////////////////////  메인 프로그램 (setup & loop)

void setup()
{
  Serial.begin( 9600 );
  
  SPI.begin(); // SPI 통신 초기화
  mfrc522.PCD_Init(); // MFRC522 초기화
  
  Serial.println("RFID Reader test: please touch with RFID card.");
}

void loop()
{
  if( ! mfrc522.PICC_IsNewCardPresent() )
  {
    Serial.print("."); // 카드가 태깅(접근)되지 않음, 대기
    delay( 500 );
    return;
  }
  
  if( ! mfrc522.PICC_ReadCardSerial() )
  {
    Serial.print("!"); // UID 읽기 실패, 다시 대기
    delay( 500 );
    return;
  }

  Serial.print("\n\nUID tag: ");  // 읽은 UID를 16진수로 출력 (ex) B00EA11B
  
  String  strRFID = "";  // RFID TAG를 저장할 문자열 변수
  
  for( byte i=0; i < mfrc522.uid.size; i++ )  // UID size는 4 byte
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     
     strRFID.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
     strRFID.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  
  Serial.print("\nMessage: ");  // (ex) [29B0007F]
  Serial.print("[");
  strRFID.toUpperCase();  // strRFID에 저장된 UID 문자열을 대문자로 변환
  Serial.print(strRFID);
  Serial.println("]");
  
  delay(3000);  // 출력하고 3초 후에 다시 카드 읽기 반복
  Serial.println("\nPlease touch with RFID card again.");
}
