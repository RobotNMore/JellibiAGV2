#include <Servo.h> 

#define pinServo    9  // 리프터(서보)모터 핀 번호

Servo servo;  // 리프터 서보 모터 인스턴스 선언

void setup()
{
  servo.attach( pinServo ); // 서보 모터 연결

  servo.write( 180-10 );  // 리프터를 위로 올림
  delay( 3000 );
  
  servo.write( 90-10 );  // 리프터를 아래로 내림
  delay( 1000 );

  servo.detach(); // 서보 모터 연결 해제
}

void loop()
{
}
