////////////////////  버튼 핀 번호

#define pinButton     A3 // 버턴 핀 번호

////////////////////  AGV 리프터(Lifter) 서보 모터 제어

#include <Servo.h> 

#define pinServo1   9   // 좌상/우하 서보 모터 (리프터1)
#define pinServo2   10  // 좌하/우상 서보 모터 (리프터2)

Servo servo1;   // 리프터1용 서보 모터 인스턴스 선언
Servo servo2;   // 리프터2용 서보 모터 인스턴스 선언

void  LifterDown()
{
  servo1.attach( pinServo1 ); // 서보1 연결
  servo2.attach( pinServo2 ); // 서보2 연결
  delay( 10 );
  
  servo1.write( 180 ); // 리프터1 아래로 내림
  servo2.write( 0 );   // 리프터2 아래로 내림
  delay( 300 );

  servo1.detach(); // 서보1 연결 해제
  servo2.detach(); // 서보2 연결 해제
  delay( 10 );  
}

//////////////  바닥의 라인트레이스 IR센서

#define pinLT1  A6 // 1번(왼쪽) IR센서 연결 핀
#define pinLT2  A7 // 2번(오른쪽) IR센서 연결 핀

// 색상 판단: White=[0..410] .. 560 .. [710..1023]=Black
#define LT_AJDUST       60  // 현재 젤리비의 센서 측정 조정값
#define LT_MAX_WHITE   410 + LT_AJDUST // 흰색으로 판단하는 최대값
#define LT_MID_VALUE   560 + LT_AJDUST // 흑백 판단 경계값(중간값)
#define LT_MIN_BLACK   710 + LT_AJDUST // 검은색으로 판단하는 최소값

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

  if(dir1 == FORWARD)  // 1번(왼쪽)모터 방향
    dirHighLow1 = HIGH;
  else // BACKWARD
    dirHighLow1 = LOW;
  
  if(dir2 == FORWARD)  // 2번(오른쪽)모터
    dirHighLow2 = LOW;
  else // BACKWARD
    dirHighLow2 = HIGH;

  digitalWrite(pinDIR1, dirHighLow1);
  analogWrite(pinPWM1, power1);

  digitalWrite(pinDIR2, dirHighLow2);
  analogWrite(pinPWM2, power2);
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

void  setup()
{
  // 모터 제어 핀들을 모두 출력용으로 설정
  pinMode( pinDIR1, OUTPUT ); // 1번(왼쪽)모터 방향 핀
  pinMode( pinPWM1, OUTPUT ); // 1번(왼쪽)모터 속력 핀
  pinMode( pinDIR2, OUTPUT ); // 2번(오른쪽)모터 방향 핀
  pinMode( pinPWM2, OUTPUT ); // 2번(오른쪽)모터 속력 핀

  LifterDown();  // 리프터를 아래로 내림

  while( digitalRead(pinButton) ) // 버튼이 안눌려졌으며
    delay( 10 );  // 버튼이 눌려질때까지 계속 대기 반복
}


int Power = 80;  // 기본 속력 (약간 느리게)

void  loop()
{
  int v1 = analogRead( pinLT1 );
  int v2 = analogRead( pinLT2 );

  if( (LT_MIN_BLACK < v1) && (LT_MIN_BLACK < v2) )
  {
    Stop(); // 양쪽 IR센서 모두 검정색 (= 정지선)
    
    while( digitalRead(pinButton) ) // 버튼이 안눌려졌으며
      delay( 10 );  // 버튼이 눌려질때까지 계속 대기 반복
      
    Forward( Power ); // 직진 출발
    delay( 300 ); // 0.3초 동안은 무조건 직진 (정지선 통과)
  }
  else if( v1 > LT_MIN_BLACK )  // 왼쪽(IR1)만 검정
  {
    TurnLeft( Power ); // 좌회전
  }
  else if( v2 > LT_MIN_BLACK )  // 오른쪽(IR2)만 검정
  {
    TurnRight( Power ); // 우회전
  }
  else  // 양쪽 모두 흰색
  {
    Forward( Power ); // 전진
  }
}
