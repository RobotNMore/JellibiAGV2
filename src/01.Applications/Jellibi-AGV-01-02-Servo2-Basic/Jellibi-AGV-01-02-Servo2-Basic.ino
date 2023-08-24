#include <Servo.h> 

#define SERVO_DOWN  90-10   // Down 위치 (10 정도 더 아래)
#define SERVO_UP    180-10  // Up 위치 (떨림 방지)
#define SERVO_DEF   SERVO_DOWN   // 기본 위치

#define pinServo   9   // 리프터(서보)모터 핀 번호

Servo servo;   // 리프터용 서보 모터 인스턴스 선언


void setup()
{
  servo.attach( pinServo ); // 서보1 연결
  delay( 10 );

  servo.write( SERVO_UP );  // 리프터1 위로 올림
  delay( 3000 );
  
  servo.write( SERVO_DOWN ); // 리프터1 아래로 내림
  delay( 300 );

  servo.detach(); // 서보1 연결 해제
  delay( 10 );
}

void loop()
{
}
