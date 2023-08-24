////////////////////  버튼 핀 번호

#define pinButton     A3 // 버턴 핀 번호

//////////////  IR센서 (바닥면 2개 + 전면 3개)

#define pinLT1  A6  // 1번(왼쪽) IR센서 연결 핀
#define pinLT2  A7  // 2번(오른쪽) IR센서 연결 핀

#define pinLT3  A0  // 전면 왼쪽 IR센서
#define pinLT4  A1  // 전면 중앙 IR센서
#define pinLT5  A2  // 전면 오른쪽 IR센서

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

////////////////////

void  setup()
{
  // 모터 제어 핀들을 모두 출력용으로 설정
  pinMode( pinDIR1, OUTPUT ); // 1번(왼쪽)모터 방향 핀
  pinMode( pinPWM1, OUTPUT ); // 1번(왼쪽)모터 속력 핀
  pinMode( pinDIR2, OUTPUT ); // 2번(오른쪽)모터 방향 핀
  pinMode( pinPWM2, OUTPUT ); // 2번(오른쪽)모터 속력 핀

  Serial.begin( 9600 ); // 시리얼 통신 (bps)

  LifterDown();  // 리프터를 아래로 내림
}

int Power = 80;  // 기본 속력 (약간 느리게)

void  StopAndBackward() // 정지한 후 뒤로 물러나기
{
  Stop();
  delay( 30 );
  Backward( Power );
  delay( 500 );
}


// 주행 상태(이동중 또는 정지) 플래그
bool  IsDriving = false; // 처음에는 정지 (버튼으로 출발/정지)


void  loop()
{
  ////////// 먼저, 버튼 눌림 체크 (버튼이 눌리면 주행 상태 변경)
  
  if( digitalRead(pinButton) == 0 ) // 버튼이 눌려진 상태이면
  {
    if( IsDriving ) // 만약 현재 주행중 상태이면
    {
      Stop(); // 정지
      
      while( digitalRead(pinButton) == 0 ) // 버튼이 올려질 때까지 대기
        delay( 10 );
        
      IsDriving = false; // 정지 상태로 변경
    }
    else // 만약 현재 정지 상태이면
    {
      while( digitalRead(pinButton) == 0 ) // 버튼이 올려질 때까지 대기
        delay( 10 );
        
      IsDriving = true; // 주행중 상태로 변경
    }
    delay( 100 ); // 0.1초 지연
  }

  ////////// 주행 상태에 따라 주행 또는 대기
  
  if( IsDriving ) // 현재 주행중 상태이면
  {
    int value = analogRead( pinLT4 ); // 전면 중앙 IR센서 읽기

    Serial.println( value ); // 시리얼 모니터로 IR센서 값 출력
  
    if( value < 980 )  // 대략 전방 10Cm 이내에 장애물이 있음
    {
      delay( 1 );
      if( value < 950 )  // 재확인 (모터 동작시 전류 변화가 큼)
      { 
        StopAndBackward(); // 일단 살짝 뒤로 물러나서
        TurnLeft( Power ); // 좌회전
        delay( 356 - Power ); // 속력에 따라 회전시간 조절
      }
    }

    Forward( Power ); // 전진
  }
  else // 정지중 상태이면
  {
    delay( 100 );  // 0.1초 대기
  }
}
