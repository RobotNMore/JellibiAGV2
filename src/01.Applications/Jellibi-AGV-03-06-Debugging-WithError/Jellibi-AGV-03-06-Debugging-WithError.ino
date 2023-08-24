/*
 * 이 코드는 디버깅 실습을 위한 소스 예제로,
 * 의도적으로 2군데 오류를 추가했습니다.
 * 
 * 업로드하여 실행했을 때 시리얼 모니터에
 * 거리 측정값이 잘 나오도록 수정하여보세요~
 * 
 * (Jellibi-AGV-03-05-LT-IR-Distance.ino 소스 참고)
 */
 
//////////////  IR센서 (바닥면 2개 + 전면 3개)

#define pinLT1  A6  // 1번(왼쪽) IR센서 연결 핀
#define pinLT2  A7  // 2번(오른쪽) IR센서 연결 핀

#define pinLT3  A0  // 전면 왼쪽 IR센서
#define pinLT4  A1  // 전면 중앙 IR센서
#define pinLT5  A2  // 전면 오른쪽 IR센서

////////////////////  메인 프로그램 (setup & loop)

void setup() {
  Serial.begin( 9600 ); // 시리얼 통신 (bps)
}

void loop() {
  int value = digitalRead( pinLT4 );  // IR센서 값 읽기

  Serial.print( "IR값= " ); // IR센서로 반사되는 값을 읽음
  Serial.print( value );    // 색상이나 반사면 상태에 의존

  if( value < 1000 )
    Serial.println( ", 10Cm 이내" );
  else if( value < 950 )
    Serial.println( ", 5Cm 이내" );
  else
    Serial.println( ", 10Cm 보다 멀거나 없음" );
    
  delay( 1000 );
}
