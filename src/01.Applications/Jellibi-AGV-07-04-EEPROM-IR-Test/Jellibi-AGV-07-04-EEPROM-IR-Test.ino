#define TEST_LT_IR            1 // 라인트레이싱용 IR센서 값 보기

////////////////////  EEPROM 데이터 쓰기/읽기

#include <EEPROM.h>

#define EEPROM_BASE           0
#define EEPROM_DATA_VER0      (EEPROM_BASE + 0)  // 'V'
#define EEPROM_DATA_VER1      (EEPROM_BASE + 1)  // '1'
#define EEPROM_DATA_VER2      (EEPROM_BASE + 2)  // '0'
#define EEPROM_DATA_VER3      (EEPROM_BASE + 3)  // '0'

#define EEPROM_PRODUCT_SN     (EEPROM_BASE + 4)  // Reserved
#define EEPROM_POWER_RATIOF   (EEPROM_BASE + 8)  // 1.00, 1.00
#define EEPROM_POWER_RATIOR   (EEPROM_BASE + 16) // Reserved
#define EEPROM_POWER_ADJUST   (EEPROM_BASE + 24) // Reserved

// 라인트레이싱용 IR 센서 보정값
#define EEPROM_LT_THRESHOLD   (EEPROM_BASE + 28) // LeftIR, RightIR

boolean IsEepromDataValid = false;

boolean CheckEepromDataHeader()
{
  if( (EEPROM.read( EEPROM_DATA_VER0 ) == 'V') &&
      (EEPROM.read( EEPROM_DATA_VER1 ) == '1') &&
      (EEPROM.read( EEPROM_DATA_VER2 ) == '0') &&
      (EEPROM.read( EEPROM_DATA_VER3 ) == '0') )
      return  true;

  return  false;
}

/////////////  좌/우 바퀴모터 속력 보정비율 (속력 = 파워 * 보정비율)

// 전진 속력 보정비율 [0 .. 1], 대부분 [0.94 ~ 1.00] 사이로 설정함
// 성능이 나쁜쪽을 1.0으로, 좋은쪽 비율을 균형에 맞도록 낮게 설정합니다.
float Power1RatioF = 1.00;  // 왼쪽 모터 (기본= 1.0)
float Power2RatioF = 1.00;  // 오른쪽 모터 (기본= 1.0)

void  ReadPowerRatio()  // EEPROM에 저장된 모터 성능비율 읽기
{
  EEPROM.get( EEPROM_POWER_RATIOF, Power1RatioF );
  EEPROM.get( EEPROM_POWER_RATIOF + 4, Power2RatioF );
}

//////////  라인트레이싱용 IR 흑백 판정 기준 : 백=[0 ~ 1023]=흑

int LineTrace1Threshold = 560;  // 왼쪽 LT 흑백 초기 중간값
int LineTrace2Threshold = 560;  // 오른쪽 LT 흑백 초기 중간값

// 측정값이 (중간값 + 150) 이상일 때 확실한 검은색이라고 판단함
#define LT_THRESHOLD_SAFEZONE   150

int LineTrace1MinBlack = LineTrace1Threshold + LT_THRESHOLD_SAFEZONE;
int LineTrace2MinBlack = LineTrace2Threshold + LT_THRESHOLD_SAFEZONE;

// 측정값이 (중간값 - 150) 이하일 때 확실한 흰색이라고 판단함
int LineTrace1MaxWhite = LineTrace1Threshold - LT_THRESHOLD_SAFEZONE;
int LineTrace2MaxWhite = LineTrace2Threshold - LT_THRESHOLD_SAFEZONE;

void  ReadLtIrThreshold() // (흰색) [0 ~ 1023] (검은색)
{
  EEPROM.get( EEPROM_LT_THRESHOLD, LineTrace1Threshold );
  EEPROM.get( EEPROM_LT_THRESHOLD + 2, LineTrace2Threshold );
}

//////////////  바닥의 라인트레이스 IR센서

#define pinLT1  A6 // 1번(왼쪽) IR센서 연결 핀
#define pinLT2  A7 // 2번(오른쪽) IR센서 연결 핀

// 색상 판단: White=[0..410] .. 560 .. [710..1023]=Black
#define LT_AJDUST       60  // 현재 젤리비의 센서 측정 조정값
#define LT_MAX_WHITE   410 + LT_AJDUST // 흰색으로 판단하는 최대값
#define LT_MID_VALUE   560 + LT_AJDUST // 흑백 판단 경계값(중간값)
#define LT_MIN_BLACK   710 + LT_AJDUST // 검은색으로 판단하는 최소값

////////////////////  부저와 버튼 핀 번호

#define pinBuzzer     3  // 부저 핀 번호
#define pinButton     A3 // 버턴 핀 번호

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

  if(dir1 == FORWARD)  // 1번(왼쪽)모터 방향
    dirHighLow1 = HIGH;
  else // BACKWARD
    dirHighLow1 = LOW;
  p1 = power1 * Power1RatioF;
  
  if(dir2 == FORWARD)  // 2번(오른쪽)모터
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

////////////////////  메인 프로그램 (setup & loop)

void setup() 
{
  // 모터 제어 핀들을 모두 출력용으로 설정
  
  pinMode( pinDIR1, OUTPUT ); // 1번(왼쪽)모터 방향 핀
  pinMode( pinPWM1, OUTPUT ); // 1번(왼쪽)모터 속력 핀

  pinMode( pinDIR2, OUTPUT ); // 2번(오른쪽)모터 방향 핀
  pinMode( pinPWM2, OUTPUT ); // 2번(오른쪽)모터 속력 핀
 
  pinMode(pinBuzzer, OUTPUT); // 부저 핀을 출력 핀으로 설정
  
  pinMode(pinButton, INPUT);  // 버튼 핀을 입력용 핀으로 설정

  pinMode( pinLT1, INPUT ); // IR 센서 입력 핀으로 설정
  pinMode( pinLT2, INPUT );

  Serial.begin( 9600 ); // 시리얼 통신 속도 설정
  
  Stop(); // 정지

  // EEPROM에 저장되어 있는 데이터(좌우 모터와 바닥면의 IR 조정값) 읽기
  if( IsEepromDataValid = CheckEepromDataHeader() )
  {
    // 이전에 EEPROM에 저장된 올바른 데이터가 있음
    
    ReadPowerRatio(); // 좌우 모터 속도 조정 비율값 읽기

    Serial.println( "\n====================\n" );
    Serial.print( "Power1RatioF= " );  // 따옴표 안의 내용 출력
    Serial.print( Power1RatioF );  // 읽은 값 출력
    Serial.print( ", Power2RatioF= " );
    Serial.println( Power2RatioF );  // 읽은 값 출력 (줄넘김)
    
    ReadLtIrThreshold(); // 좌우 라인트레이싱 IR 중간값 읽기

    // B/W 중간값 보다 값이 더 클 때 검은색의 라인으로 인식할 수치 결정
    LineTrace1MinBlack = LineTrace1Threshold + LT_THRESHOLD_SAFEZONE;
    LineTrace2MinBlack = LineTrace2Threshold + LT_THRESHOLD_SAFEZONE;

    // B/W 중간값 보다 더 작을 때 확실한 흰색으로 판단할 수치 결정
    LineTrace1MaxWhite = LineTrace1Threshold - LT_THRESHOLD_SAFEZONE;
    LineTrace2MaxWhite = LineTrace2Threshold - LT_THRESHOLD_SAFEZONE;
    
    Serial.print( "1Threshold= " );
    Serial.print( LineTrace1Threshold );
    Serial.print( ", 2Threshold= " );
    Serial.println( LineTrace2Threshold );
    
    Serial.print( "1MinBlack= " );
    Serial.print( LineTrace1MinBlack );
    Serial.print( ", 2MinBlack= " );
    Serial.print( LineTrace2MinBlack );
    
    Serial.print( ", 1MaxWhite= " );
    Serial.print( LineTrace1MaxWhite );
    Serial.print( ", 2MaxWhite= " );
    Serial.println( LineTrace2MaxWhite );
  }
  else
  {
    Serial.println( "Data not found from EEPROM." );
  }
  
  tone( pinBuzzer, 262 ); // 초기화가 끝났음을 "도미솔" 음 재생
  delay( 150 );
  tone( pinBuzzer, 330 );
  delay( 150 );
  tone( pinBuzzer, 392 ); 
  delay( 250 );
  noTone( pinBuzzer );  // 스피커/부저 끄기(음소거)
}

bool  DoLineTrace = false; // 라인트레이싱(=1) 또는 정지(=0)

int   Power = 100; // 주행 속력 (80= 약간 느리게, 120= 빠르게)

void loop()
{
#if TEST_LT_IR
  if( digitalRead(pinButton) == 0 ) // 버튼이 눌림
  {
    if( DoLineTrace )  // 현재 라인트레이싱 중이면 측정 정지
      DoLineTrace = false;
    else  // 현재 라인트레이싱 중이 아니면 측정 시작
      DoLineTrace = true;
  }

  if( DoLineTrace )
  {
    int v1 = analogRead( pinLT1 );  // 왼쪽 IR 센서값 읽기
    int v2 = analogRead( pinLT2 );  // 오른쪽 IR 센서값 읽기
  
    if( (LineTrace1MinBlack < v1) && (LineTrace2MinBlack < v2) )
    { // 양쪽 IR센서 모두 검정색을 감지한 경우, (ex) 검정 교차선
      Serial.print( "[B,B] " );
      Serial.print( v1 - LineTrace1MinBlack );
      Serial.print( ", " );
      Serial.println( v2 - LineTrace2MinBlack );
    }
    else if( v1 > LineTrace1MinBlack )  // 왼쪽만 검정, 좌회전
    {
      Serial.print( "[B,W] " );
      Serial.print( v1 - LineTrace1MinBlack );
      Serial.print( ", " );
      Serial.println( LineTrace2MaxWhite - v2 );
    }
    else if( v2 > LineTrace2MinBlack )  // 오른쪽만 검정,우회전
    {
      Serial.print( "[W,B] " );
      Serial.print( LineTrace1MaxWhite - v1 );
      Serial.print( ", " );
      Serial.println( v2 - LineTrace1MinBlack );
    }
    else  // 양쪽 모두 흰색, 계속 전진
    {
      Serial.print( "[W,W] " );
      Serial.print( LineTrace1MaxWhite - v1 );
      Serial.print( ", " );
      Serial.println( LineTrace2MaxWhite - v2 );
    }
  }

  delay( 200 );
#else  
  if( digitalRead(pinButton) == 0 ) // 버튼이 눌림
  {
    if( DoLineTrace )  // 현재 라인트레이싱 중이면 정지
    {
      Stop();
      
      tone( pinBuzzer, 330 ); // "미"
      delay( 100 );
      tone( pinBuzzer, 262 ); // "도"
      delay( 250 );
      noTone( pinBuzzer );
    }
      
    while( digitalRead(pinButton) == 0 ) // 버튼이 올려질 때 까지 대기
      delay( 10 );

    DoLineTrace = ! DoLineTrace;
    
    if( DoLineTrace )  // 라인트레이싱 시작
    {
      tone( pinBuzzer, 262 ); // "도"
      delay( 100 );
      tone( pinBuzzer, 330 ); // "미"
      delay( 250 );
      noTone( pinBuzzer );
      
      Forward( Power ); // 주행 시작
    }
  }
  
  if( DoLineTrace )  // 라인트레이싱
  {
    int v1 = analogRead( pinLT1 );  // 왼쪽 IR 센서값 읽기
    int v2 = analogRead( pinLT2 );  // 오른쪽 IR 센서값 읽기
  
    if( (LineTrace1MinBlack < v1) && (LineTrace2MinBlack < v2) )
    {
      // 양쪽 IR센서 모두 검정색을 감지한 경우, (ex) 검정 교차선
      Forward( Power ); // 계속 직진
      delay( 255-Power+1 );
    }
    else if( v1 > LineTrace1MinBlack )  // 왼쪽만 검정, 좌회전
    {
      TurnLeft( Power ); // 좌회전
    }
    else if( v2 > LineTrace2MinBlack )  // 오른쪽만 검정,우회전
    {
      TurnRight( Power ); // 우회전 
    }
    else  // 양쪽 모두 흰색, 계속 전진
    {
      Forward( Power );
    }
  }
#endif  
}
