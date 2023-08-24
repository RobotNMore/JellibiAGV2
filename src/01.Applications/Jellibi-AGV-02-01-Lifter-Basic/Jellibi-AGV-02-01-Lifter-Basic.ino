////////////////////  AGV 리프터(Lifter) 서보 모터 제어

#include <Servo.h> 

#define SERVO_DOWN  90-10   // Down 위치 (10 정도 더 아래)
#define SERVO_UP    180-10  // Up 위치 (떨림 방지)
#define SERVO_DEF   SERVO_DOWN   // 기본 위치

#define pinServo   9   // 리프터(서보)모터 핀 번호

Servo servo;   // 리프터용 서보 모터 인스턴스 선언

void  LifterUp()
{
  servo.attach( pinServo ); // 서보 연결
  delay( 10 );

  servo.write( SERVO_UP );  // 리프터 위로 올림
  delay( 300 );

  servo.detach(); // 서보 연결 해제
  delay( 10 );  
}

void  LifterDown()
{
  servo.attach( pinServo ); // 서보 연결
  delay( 10 );
  
  servo.write( SERVO_DOWN ); // 리프터 아래로 내림
  delay( 300 );

  servo.detach(); // 서보 연결 해제
  delay( 10 );  
}

////////////////////  메인 프로그램 (setup & loop)

void setup()
{
  LifterUp();     // 리프터를 위로 올림
  delay( 3000 );

  LifterDown();   // 리프터를 아래로 내림
}

void loop()
{
}
