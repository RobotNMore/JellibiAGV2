#include <EEPROM.h>
#include <Servo.h>
#include "Tone_note.h"

// 전진 속력 보정비율 [0 .. 1]
float Power1RatioF = 1.00;  // 왼쪽 모터
float Power2RatioF = 1.00;  // 오른쪽 모터

#define MAX_SPEED   255

int Power = 80;  // 센서 라인트레이싱 기본 속력 (약간 느리게)

////////////////////  기본적인 핀 번호 선언

#define pinBuzzer   3  // 부저핀 번호
#define pinServo    9  // 리프터(서보)모터 핀 번호
#define pinButton   A3 // 버턴핀 번호

////////////////////  LineTrace IR Threshold : 백=[0 ~ 1023]=흑 중간값

int LineTrace1Threshold = 560;  // 왼쪽 LT 흑백 초기 중간값
int LineTrace2Threshold = 560;  // 오른쪽 LT 흑백 초기 중간값

#define LT_THRESHOLD_SAFEZONE   150 // (중간값+150) 이상일 때 검은색으로 판단

int LineTrace1MinBlack = LineTrace1Threshold + LT_THRESHOLD_SAFEZONE;
int LineTrace2MinBlack = LineTrace2Threshold + LT_THRESHOLD_SAFEZONE;


//////////  RFID Reader

// Reference : https://randomnerdtutorials.com/security-access-using-mfrc522-rfid-reader-with-arduino/
#include <MFRC522.h>
#include <MFRC522Extended.h>
#include <require_cpp11.h>
#include <SPI.h>

#define RFID_SS_PIN     2
#define RFID_RESET_PIN  4 // MFRC522::UNUSED_PIN

MFRC522 _mfrc522(RFID_SS_PIN, RFID_RESET_PIN);
String _strCardUID;


////////////////////  EEPROM 데이터 읽기/쓰기

#define EEPROM_BASE           0
#define EEPROM_DATA_VER0      (EEPROM_BASE + 0)  // 'V'
#define EEPROM_DATA_VER1      (EEPROM_BASE + 1)  // '1'
#define EEPROM_DATA_VER2      (EEPROM_BASE + 2)  // '0'
#define EEPROM_DATA_VER3      (EEPROM_BASE + 3)  // '0'

#define EEPROM_PRODUCT_SN     (EEPROM_BASE + 4)  // 1010000001
#define EEPROM_POWER_RATIOF   (EEPROM_BASE + 8)  // 1.00, 1.00
#define EEPROM_POWER_RATIOR   (EEPROM_BASE + 16) // 1.00, 1.00
#define EEPROM_POWER_ADJUST   (EEPROM_BASE + 24) // [0..50..100], [0..50..100]
#define EEPROM_LT_THRESHOLD   (EEPROM_BASE + 28) // L=[..512..], R=[..512..]
#define EEPROM_SERVO_ADJUST   (EEPROM_BASE + 32) // Pan=[..512..], Tilt=[..512..]

boolean IsEepromDataValid = false;

boolean CheckEepromDataHeader()
{
  if ( (EEPROM.read( EEPROM_DATA_VER0 ) == 'V') &&
       (EEPROM.read( EEPROM_DATA_VER1 ) == '1') &&
       (EEPROM.read( EEPROM_DATA_VER2 ) == '0') &&
       (EEPROM.read( EEPROM_DATA_VER3 ) == '0') )
    return  true;

  return  false;
}

void  ReadPowerRatio()
{
  EEPROM.get( EEPROM_POWER_RATIOF, Power1RatioF );
  EEPROM.get( EEPROM_POWER_RATIOF + 4, Power2RatioF );
}

void  ReadLtIrThreshold()
{
  EEPROM.get( EEPROM_LT_THRESHOLD, LineTrace1Threshold );
  EEPROM.get( EEPROM_LT_THRESHOLD + 2, LineTrace2Threshold );
}

void  WriteEepromDataHeader()
{
  EEPROM.write( EEPROM_DATA_VER0, 'V' );
  EEPROM.write( EEPROM_DATA_VER1, '1' );
  EEPROM.write( EEPROM_DATA_VER2, '0' );
  EEPROM.write( EEPROM_DATA_VER3, '0' );
}

void  ClearEepromDataHeader()
{
  EEPROM.write( EEPROM_DATA_VER0, 0 );
  EEPROM.write( EEPROM_DATA_VER1, 0 );
}

void  WritePowerRatio()
{
  EEPROM.put( EEPROM_POWER_RATIOF, Power1RatioF );
  EEPROM.put( EEPROM_POWER_RATIOF + 4, Power2RatioF );
}

void  WriteLtIrThreshold()
{
  EEPROM.put( EEPROM_LT_THRESHOLD, LineTrace1Threshold );
  EEPROM.put( EEPROM_LT_THRESHOLD + 2, LineTrace2Threshold );
}

void  WriteServoAdjust()
{
  EEPROM.put( EEPROM_SERVO_ADJUST, 512 );
  EEPROM.put( EEPROM_SERVO_ADJUST + 2, 512 );
}

//////////////  바닥의 라인트레이스 IR센서

#define pinLT1  A6 // 1번(왼쪽) IR센서 연결 핀
#define pinLT2  A7 // 2번(오른쪽) IR센서 연결 핀

#define pinLT3  A1  // 정면 Left IR
#define pinLT4  A0  // 정면 Center IR
#define pinLT5  A2  // 정면 Right IR

// 연속 회전 횟수를 4단계까지 나누어 부드럽게 회전
#define MAX_TURN_STEP1    1  // 연속 좌/우 회전 감지 카운터
#define MAX_TURN_STEP2    2  // (속력을 1/4 씩 추가로 감속)
#define MAX_TURN_STEP3    3  //
#define MAX_TURN_STEP4    4  //

int LeftTurn = 0;   // 왼쪽 연속회전 누적횟수
int RightTurn = 0;  // 오른쪽 연속회전 누적횟수

////////////////////  모터 1번(왼쪽)과 2번(오른쪽)

#define pinDIR1   7 // 1번(왼쪽)모터 방향 지정용 연결 핀
#define pinPWM1   5 // 1번(왼쪽)모터 속력 지정용 연결 핀

#define pinDIR2   8 // 2번(오른쪽)모터 방향 지정용 연결 핀
#define pinPWM2   6 // 2번(오른쪽)모터 속력 지정용 연결 핀

////////////////////  모터 회전 동작

#define FORWARD   0 // 전진 방향
#define BACKWARD  1 // 후진 방향

void  drive(int dir1, int power1, int dir2, int power2)
{
  boolean dirHighLow1, dirHighLow2;
  int     p1, p2;

  if (dir1 == FORWARD) // 1번(왼쪽)모터 방향
    dirHighLow1 = HIGH;
  else // BACKWARD
    dirHighLow1 = LOW;
  p1 = power1 * Power1RatioF;

  if (dir2 == FORWARD) // 2번(오른쪽)모터
    dirHighLow2 = LOW;
  else // BACKWARD
    dirHighLow2 = HIGH;
  p2 = power2 * Power2RatioF;

  digitalWrite(pinDIR1, dirHighLow1);
  analogWrite(pinPWM1, p1);

  digitalWrite(pinDIR2, dirHighLow2);
  analogWrite(pinPWM2, p2);
}

void  Forward( int power )  // 전진
{
  drive(FORWARD, power, FORWARD, power);
}

void  Backward( int power )  // 후진
{
  drive(BACKWARD, power, BACKWARD, power);
}

void  TurnLeft( int power )  // 좌회전
{
  drive(BACKWARD, power, FORWARD, power);
}

void  TurnRight( int power )  // 우회전
{
  drive(FORWARD, power, BACKWARD, power);
}

void Stop()  // 정지
{
  analogWrite(pinPWM1, 0);
  analogWrite(pinPWM2, 0);
}

void  Drive2( int32_t deltaLeft, int32_t deltaRight )
{
  boolean dirHighLow1, dirHighLow2;
  int     p1, p2;

  if (deltaLeft >= 0) // 1번(왼쪽)모터 방향
    dirHighLow1 = HIGH;
  else // BACKWARD
    dirHighLow1 = LOW;

  if (deltaRight >= 0) // 2번(오른쪽)모터
    dirHighLow2 = LOW;
  else // BACKWARD
    dirHighLow2 = HIGH;

  int spd1 = abs(deltaLeft);
  int spd2 = abs(deltaRight);

  if (spd1 > MAX_SPEED)  spd1 = MAX_SPEED;
  if (spd2 > MAX_SPEED)  spd2 = MAX_SPEED;

  p1 = spd1 * Power1RatioF;
  p2 = spd2 * Power2RatioF;

  digitalWrite(pinDIR1, dirHighLow1);
  analogWrite(pinPWM1, spd1);

  digitalWrite(pinDIR2, dirHighLow2);
  analogWrite(pinPWM2, spd2);
}

//////////

#define SERVO_DOWN  90-10   // Down 위치 (10 정도 더 아래)
#define SERVO_UP    180-10  // Up 위치 (떨림 방지)
#define SERVO_DEF   SERVO_DOWN   // 기본 위치
#define SERVO_UP_LIMIT  180

#define SERVO_ANGLE_STEP    10

Servo servo;

void  ServoAttach() {
  if ( ! servo.attached() ) {
    servo.attach( pinServo );
    delay(20);
  }
}

void  ServoDetach() {
  if ( servo.attached() ) {
    servo.detach();
    delay(10);
  }
}

void LiftUpDown( int isUp ) {
  ServoAttach();

  if ( isUp )
    servo.write( SERVO_UP );
  else
    servo.write( SERVO_DOWN );

  delay(300);
  ServoDetach();
}

////////////////////////////////////////  setup()

#define MODE_STOP_READY         0
#define MODE_LINETRACE          1   // UP
#define MODE_ALL_PART_TEST      2   // DOWN
#define MODE_SET_MOTOR_PERF     3   // DOWN+LEFT
#define MODE_SET_IR_CALIB       4   // DOWN+RIGHT

int DriveTestMode = MODE_STOP_READY;

void  StopReady()
{
  Stop();
  DriveTestMode = MODE_STOP_READY;
}

void  PlayBeginMelody()
{
  tone( pinBuzzer, 262 ); // "도"
  delay( 100 );
  tone( pinBuzzer, 392 ); // "솔"
  delay( 250 );
  noTone( pinBuzzer );
}

void  PlayEndMelody()
{
  tone( pinBuzzer, 392 ); // "솔"
  delay( 100 );
  tone( pinBuzzer, 262 ); // "도"
  delay( 250 );
  noTone( pinBuzzer );
}

void  PlaySwitchWrongMelody()
{
  tone( pinBuzzer, 392 ); // "솔"
  delay( 150 );
  tone( pinBuzzer, 330 ); // "미"
  delay( 150 );
  tone( pinBuzzer, 294 ); // "레"
  delay( 250 );
  noTone( pinBuzzer );
}

bool SetupMode = 0;

void setup()
{
  Serial.begin(9600);
  
  //Init RFIDReader
  SPI.begin();
  _mfrc522.PCD_Init();

  // 모터 제어 핀들을 모두 출력용으로 설정
  pinMode( pinDIR1, OUTPUT ); // 1번(왼쪽)모터 방향 핀
  pinMode( pinPWM1, OUTPUT ); // 1번(왼쪽)모터 속력 핀

  pinMode( pinDIR2, OUTPUT ); // 2번(오른쪽)모터 방향 핀
  pinMode( pinPWM2, OUTPUT ); // 2번(오른쪽)모터 속력 핀

  pinMode( pinLT1, INPUT ); // IR 센서 핀을 입력용으로 설정
  pinMode( pinLT2, INPUT );

  pinMode( pinLT3, INPUT );
  pinMode( pinLT4, INPUT );
  pinMode( pinLT5, INPUT );

  pinMode( pinButton, INPUT );  // 푸시버튼 핀을 입력용으로 설정

  pinMode( pinBuzzer, OUTPUT ); // 출력 핀으로 설정
  noTone( pinBuzzer );  // 스피커/부저 끄기(음소거)

  Stop(); // 정지

  if( digitalRead(pinButton) == 0 )
  {
    tone( pinBuzzer, 330 ); // "미"
    delay( 250 );
    tone( pinBuzzer, 262 ); // "도"
    delay( 250 );
    tone( pinBuzzer, 330 ); // "미"
    delay( 250 );
    tone( pinBuzzer, 262 ); // "도"
    delay( 250 );
    tone( pinBuzzer, 330 ); // "미"
    delay( 250 );
    tone( pinBuzzer, 262 ); // "도"
    delay( 500 );
    noTone( pinBuzzer );
    delay( 1000 );

    if ( digitalRead(pinButton) == 0 )
    {
      tone( pinBuzzer, 262 ); // "도"
      delay( 150 );
      tone( pinBuzzer, 330 ); // "미"
      delay( 250 );
      noTone( pinBuzzer );

      while ( digitalRead(pinButton) == 0 )
        delay( 10 );

      ClearEepromDataHeader();
    }
  }

  if ( IsEepromDataValid = CheckEepromDataHeader() )
  {
    ReadPowerRatio();

    ReadLtIrThreshold();
    LineTrace1MinBlack = LineTrace1Threshold + LT_THRESHOLD_SAFEZONE;
    LineTrace2MinBlack = LineTrace2Threshold + LT_THRESHOLD_SAFEZONE;
  }
  else
  {
    WriteEepromDataHeader();
    WritePowerRatio();
    WriteLtIrThreshold();
    WriteServoAdjust();
  }

  LiftUpDown( true );
  delay( 1000 );
  
  LiftUpDown( false );

  StopReady();

  tone( pinBuzzer, 262 ); // "도"
  delay( 150 );
  tone( pinBuzzer, 330 ); // "미"
  delay( 150 );
  tone( pinBuzzer, 392 ); // "솔"
  delay( 250 );
  noTone( pinBuzzer );
}

////////////////////////////  loop()

#define MODE_STOP_READY         0
#define MODE_LINETRACE          1   // ShortClick
#define MODE_ALL_PART_TEST      2   // LongClick + ShortClick(1)
#define MODE_SET_MOTOR_PERF     3   // LongClick + ShortClick(2)
#define MODE_SET_IR_CALIB       4   // LongClick + ShortClick(3)
#define MODE_SET_LIFTER         5   // ShortClick + FrontIR1 + FrontIR3

void loop()
{
  if (SetupMode) {
    if ( digitalRead(pinButton) == 0 ) // '눌림' 상태이면
    {
      if ( DriveTestMode == MODE_LINETRACE )
      {
        StopReady();
        PlayEndMelody();
        return;
      }

      if ( (analogRead(pinLT3) < 600) && (analogRead(pinLT4) < 600) && (analogRead(pinLT5) < 600) )
      {
        while ( digitalRead(pinButton) == 0 )
          delay( 5 );
        delay( 10 );

        for ( int i = 0; i < 3; i++ )
        {
          tone( pinBuzzer, 262 ); // "도"
          delay( 50 );
          tone( pinBuzzer, 330 ); // "미"
          delay( 50 );
        }
        noTone( pinBuzzer );

        PlayBeginMelody();
        //DriveTestMode = MODE_SET_LIFTER;

        bool  bAdjust = true;
        while ( bAdjust )
        {
          bAdjust = false;
          
          LiftUpDown( false );
        }

        StopReady();
        //PlayEndMelody();
        return;
      }

      int nClick = 0;
      unsigned long tick = millis();
      
      while ( 1 )
      {
        Serial.println("8");
        while ( digitalRead(pinButton) == 0 )
          delay( 5 );
        delay( 10 );
        if ( digitalRead(pinButton) != 0 )
          break;
      }

      if ( millis() - tick > 2000 ) // long-press click
      {
        for ( int freq = 200; freq < 1000; freq += 10 )
        {
          tone( pinBuzzer, freq );
          delay( 5 );
        }
        noTone( pinBuzzer );

        tick = millis();
        while ( 1 )
        {
          if ( digitalRead(pinButton) == 0 )
          {
            while ( 1 )
            {
              while ( digitalRead(pinButton) == 0 )
                delay( 5 );
              delay( 10 );
              if ( digitalRead(pinButton) != 0 )
                break;
            }
            nClick++;

            tone( pinBuzzer, 220 );
            delay( 50 );
            noTone( pinBuzzer );

            tick = millis();
          }

          if ( millis() - tick > 2000 )
            break;

          delay( 10 );
        }

        if ( nClick == 1 )
        {
          //PlayBeginMelody();
          DriveTestMode = MODE_ALL_PART_TEST;
        }
        else if ( nClick == 2 )
        {
          PlayBeginMelody();
          DriveTestMode = MODE_SET_MOTOR_PERF;
        }
        else if ( nClick == 3 )
        {
          PlayBeginMelody();
          DriveTestMode = MODE_SET_IR_CALIB;
        }
        else // zero or extra many clicks ... ignore
        {
          // ignore ...
          for ( int freq = 1000; freq > 200; freq -= 10 )
          {
            tone( pinBuzzer, freq );
            delay( 5 );
          }
          noTone( pinBuzzer );
        }
      }
      else  // short-press click
      {
        PlayBeginMelody();
        DriveTestMode = MODE_LINETRACE;
      }
    }

    if ( DriveTestMode == MODE_LINETRACE )
    {
      int v1 = analogRead( pinLT1 );  // 왼쪽 IR 센서값 읽기
      int v2 = analogRead( pinLT2 );  // 오른쪽 IR 센서값 읽기

      if ( (LineTrace1MinBlack < v1) && (LineTrace2MinBlack < v2) )
      { // 양쪽 IR센서 모두 검정색을 감지한 경우, (ex) 검정 교차선
        Forward( Power ); // 계속 직진
        delay( 255 - Power + 1 );

        LeftTurn = 0;  // 왼쪽 연속회전 누적횟수 초기화
        RightTurn = 0; // 오른쪽 연속회전 누적횟수 초기화
      }
      else if ( v1 > LineTrace1MinBlack ) // 왼쪽만 검정, 좌회전
      {
        if ( LeftTurn < MAX_TURN_STEP1 ) // 왼쪽 회전 시작
          drive(FORWARD, Power * 3.0 / 4, FORWARD, Power);
        else if ( LeftTurn < MAX_TURN_STEP2 )
          drive(FORWARD, Power / 2, FORWARD, Power);
        else if ( LeftTurn < MAX_TURN_STEP3 )
          drive(FORWARD, 0, FORWARD, Power);
        else // 연속으로 좌회전이 많이 누적되는 경우
          drive(BACKWARD, Power / 2, FORWARD, Power);

        LeftTurn++;    // 왼쪽 연속회전 누적횟수 증가
        RightTurn = 0; // 오른쪽 연속회전 누적횟수 초기화
      }
      else if ( v2 > LineTrace2MinBlack ) // 오른쪽만 검정,우회전
      {
        if ( RightTurn < MAX_TURN_STEP1 ) // 오른쪽 회전 시작
          drive(FORWARD, Power, FORWARD, Power * 3.0 / 4);
        else if ( RightTurn < MAX_TURN_STEP2 )
          drive(FORWARD, Power, FORWARD, Power / 2);
        else if ( RightTurn < MAX_TURN_STEP3 )
          drive(FORWARD, Power, FORWARD, 0);
        else // 연속으로 우회전이 많이 누적되는 경우
          drive(FORWARD, Power, BACKWARD, Power / 2);

        RightTurn++;  // 왼쪽 연속회전 누적횟수 초기화
        LeftTurn = 0; // 오른쪽 연속회전 누적횟수 증가
      }
      else  // 양쪽 모두 흰색, 계속 전진
      {
        Forward( Power );
        LeftTurn = 0;  // 왼쪽 연속회전 누적횟수 초기화
        RightTurn = 0; // 오른쪽 연속회전 누적횟수 초기화
      }
    }

    else if ( DriveTestMode == MODE_ALL_PART_TEST )
    {
      // 부저와 Lifter 테스트
      LiftUpDown( true );

      tone( pinBuzzer, 262 ); // "도"
      delay( 150 );
      tone( pinBuzzer, 294 ); // "레"
      delay( 150 );
      tone( pinBuzzer, 330 ); // "미"
      delay( 150 );
      tone( pinBuzzer, 349 ); // "파"
      delay( 150 );
      tone( pinBuzzer, 392 ); // "솔"
      delay( 150 );
      tone( pinBuzzer, 440 ); // "라"
      delay( 150 );
      tone( pinBuzzer, 494 ); // "시"
      delay( 150 );
      tone( pinBuzzer, 523 ); // 높은 "도"
      delay( 250 );
      noTone( pinBuzzer ); // 음소거(mute)

      LiftUpDown( false );
      delay( 1000 );

      int vLeft, vRight, vCenter;

      while ( DriveTestMode == MODE_ALL_PART_TEST )
      {
        if ( digitalRead(pinButton) == 0 )
        {
          delay( 500 );
          break;
        }
        
        vLeft = analogRead( pinLT3 );
        vCenter = analogRead( pinLT4 );
        vRight = analogRead( pinLT5 );

        if ( vLeft < 900 )
        {
          tone( pinBuzzer, 262 ); // "도"
          delay( 150 );
          noTone( pinBuzzer );
        }

        if ( vCenter < 900 )
        {
          tone( pinBuzzer, 330 ); // "미"
          delay( 150 );
          noTone( pinBuzzer );
        }

        if ( vRight < 900 )
        {
          tone( pinBuzzer, 392 ); // "솔"
          delay( 150 );
          noTone( pinBuzzer );
        }
      }

      StopReady();
      PlayEndMelody();
    }

    else if ( DriveTestMode == MODE_SET_MOTOR_PERF )
    {
      float r10 = Power1RatioF;  // 왼쪽 모터
      float r20 = Power2RatioF;  // 오른쪽 모터
      bool  bAdjustSpeed = true;

      int vLeft, vRight;

      delay( 500 );

      // 좌우 모터 성능 테스트
      while ( bAdjustSpeed )
      {
        Forward( 60 );
        delay( 100 );
        Forward( 100 );
        delay( 1500 );
        
        Forward( 60 );
        delay( 100 );
        Stop();
        delay( 100 );

        Backward( 60 );
        delay( 100 );
        Backward( 100 );
        delay( 1500 );
        
        Backward( 60 );
        delay( 100 );
        Stop();

        bool  bButtonAdjust = true;

        while ( bButtonAdjust )
        {
          if ( digitalRead(pinButton) == 0 )
          {
            vLeft = analogRead( pinLT3 );
            vRight = analogRead( pinLT5 );

            while ( digitalRead(pinButton) == 0 )
              delay( 1 );

            if ( vLeft < 600 )
            {
              if ( Power2RatioF > 0.99 )
                Power1RatioF -= 0.01;
              else
                Power2RatioF += 0.01;

              tone( pinBuzzer, 262 ); // "도"
              delay( 250 );
              noTone( pinBuzzer );
            }
            else if ( vRight < 600 )
            {
              if ( Power1RatioF > 0.99 )
                Power2RatioF -= 0.01;
              else
                Power1RatioF += 0.01;

              tone( pinBuzzer, 330 ); // "미"
              delay( 250 );
              noTone( pinBuzzer );
            }
            else // EEPROM에 쓰고, 이동 테스트 반복
            {
              bButtonAdjust = false;
              if ( (r10 == Power1RatioF) && (r20 == Power2RatioF) )
              {
                // 비율 변화 없음, 설정 끝
                bAdjustSpeed = false;
              }
              else
              {
                WritePowerRatio();

                r10 = Power1RatioF;
                r20 = Power2RatioF;

                for ( int l = 0; l < 3; l++ )
                {
                  tone( pinBuzzer, 262 ); // "도"
                  delay( 30 );
                  tone( pinBuzzer, 294 ); // "레"
                  delay( 30 );
                  tone( pinBuzzer, 330 ); // "미"
                  delay( 50 );
                }
              }
              noTone( pinBuzzer );
              delay( 500 );
            }
          } // button pressed
        } // while( bButtonAdjust )
      }

      StopReady();
      PlayEndMelody();
    }


    else if ( DriveTestMode == MODE_SET_IR_CALIB )
    {
      LiftUpDown( true );

      int w1, w2;
      while ( 1 )
      {
        while ( digitalRead(pinButton) )
          delay( 10 );
        while ( digitalRead(pinButton) == 0 )
          delay( 10 );

        w1 = analogRead( pinLT1 );
        w2 = analogRead( pinLT2 );

        if ( (w1 < 650) && (w2 < 650) )
        {
          tone( pinBuzzer, 262 ); // "도"
          delay( 150 );
          tone( pinBuzzer, 330 ); // "미"
          delay( 250 );
          noTone( pinBuzzer );
          break;
        }

        PlaySwitchWrongMelody();
      }

      LiftUpDown( false );

      int k1, k2;
      while ( 1 )
      {
        while ( digitalRead(pinButton) )
          delay( 10 );
        while ( digitalRead(pinButton) == 0 )
          delay( 10 );

        k1 = analogRead( pinLT1 );
        k2 = analogRead( pinLT2 );

        if ( (k1 > 600) && (k2 > 600) )
          break;

        PlaySwitchWrongMelody();
      }

      LineTrace1Threshold = (k1 - w1) / 2 + w1; // 왼쪽 LT 흑백 중간값
      LineTrace2Threshold = (k2 - w2) / 2 + w2; // 오른쪽 LT 흑백 중간값

      LineTrace1MinBlack = LineTrace1Threshold + LT_THRESHOLD_SAFEZONE;
      LineTrace2MinBlack = LineTrace2Threshold + LT_THRESHOLD_SAFEZONE;

      WriteLtIrThreshold();

      for ( int freq = 2000; freq > 1000; freq -= 10 )
      {
        tone( pinBuzzer, freq );
        delay( 5 );
      }
      noTone( pinBuzzer );
      delay( 500 );

      StopReady();
      PlayEndMelody();
    }
  }
  else 
  {
    //IOCheck 실행
    if( digitalRead(pinButton) ) // 버튼이 안 눌림 (올려진 상태)
      return;

    if ( (analogRead(pinLT3) < 600) && (analogRead(pinLT4) < 600) && (analogRead(pinLT5) < 600) )
    {
        SetupMode = true;
    } else {
        TestDrive();
        delay(1000);
        
        LiftUp();
        delay(1000);
        
        PutDown();
        delay(1000);
        
        while (!ReadRFIDReader()) {
          delay(500);
        }
        PlayTone();
        
        int cnt = 0;
        bool isTone = 0;
        
        while (1)
        {
          if( digitalRead(pinButton) == 0 ) // 버튼 눌림
          {
            cnt++;
            
            while( digitalRead(pinButton) == 0 ) // 버튼이 올려질 때 까지
                delay( 10 ); // 10 ms 대기
          }
          
          if(cnt==1 && !isTone){
             tone( pinBuzzer, 262 ); // "도"
             delay( 150 );
             noTone( pinBuzzer );
             
             isTone = 1;
          }
          else if(cnt==2 && isTone){
            tone( pinBuzzer, 330 ); // "미"
            delay( 150 );
            noTone( pinBuzzer );
            
            isTone = 0;
          }
          else if (cnt == 3 && !isTone) {
            tone( pinBuzzer, 392 ); // "솔"
            delay( 150 );
            noTone( pinBuzzer );
            
            LiftUp_LIMIT();
            isTone = 1;
            break;
          }
        }
        
        while(1) { }    // HALT!!!
    }
  }
}


void LiftUp()
{
  const int STEP = 20;
  
  servo.attach(pinServo);
  Serial.println(__FUNCTION__);

  for ( int i = 0; i <= STEP; i++) {
    int angle1 = map(i, 0, STEP, SERVO_DOWN, SERVO_UP);
    servo.write(angle1);
    delay(20);
  }
}

void PutDown()
{
  const int STEP = 20;
  Serial.println(__FUNCTION__);

  for (int i = 0; i <= STEP; i++) {
    int angle1 = map(i, 0, STEP, SERVO_UP, SERVO_DOWN);
    servo.write(angle1);
    delay(20);
  }
  servo.detach();
}

void LiftUp_LIMIT()
{
  const int STEP = 20;
  servo.attach(pinServo);

  for ( int i = 0; i <= STEP; i++) {
    int angle1 = map(i, 0, STEP, SERVO_DOWN, SERVO_UP_LIMIT);
    servo.write(angle1);
    delay(20);
  }
  servo.detach();
}

bool ReadRFIDReader()
{
  Serial.println("read");
  
  if (!_mfrc522.PICC_IsNewCardPresent()) {
    Serial.println("New card false");
    return false;
  }
  
  if (!_mfrc522.PICC_ReadCardSerial()) {
    Serial.println("card Serial false");
    return false;
  }
  
  Serial.println("detect");
  _strCardUID = "";
  
  for (byte i = 0; i < _mfrc522.uid.size; i++) {
    _strCardUID.concat(String(_mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    _strCardUID.concat(String(_mfrc522.uid.uidByte[i], HEX));
  }
  
  Serial.print("CARD UID : ");
  _strCardUID.toUpperCase();
  Serial.println(_strCardUID);
  return true;
}

void TestDrive()
{
  analogWrite(pinPWM2, 0);
  analogWrite(pinPWM1, 0);

  digitalWrite(pinDIR2, LOW);
  digitalWrite(pinDIR1, HIGH);
  delay(10);
  analogWrite(pinPWM2, 60);
  analogWrite(pinPWM1, 60);
  delay(1500);
  
  analogWrite(pinPWM2, 0);
  analogWrite(pinPWM1, 0);

  digitalWrite(pinDIR2, HIGH);
  digitalWrite(pinDIR1, LOW);
  delay(100);
  analogWrite(pinPWM2, 60);
  analogWrite(pinPWM1, 60);
  delay(1500);
  
  analogWrite(pinPWM2, 0);
  analogWrite(pinPWM1, 0);
}

void PlayTone()
{
  // notes in the melody:
  int melody[] = {  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4};

  // note durations: 4 = quarter note, 8 = eighth note, etc.:
  int noteDurations[] = {4, 8, 8, 4, 4, 4, 4, 4};
  
  // iterate over the notes of the melody:
  for (int thisNote = 0; thisNote < 8; thisNote++) {
    // to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(pinBuzzer, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    
    // stop the tone playing:
    noTone(pinBuzzer);
  }
}
