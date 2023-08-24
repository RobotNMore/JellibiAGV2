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
  int value1 = analogRead(pinLT1);  // IR1 값 읽기
  int value2 = analogRead(pinLT2);  // IR2 값 읽기

  Serial.print( value1 ); // WHITE= 300~400, BLACK= 900~950
  Serial.print( ", " );
  Serial.print( value2 );
  Serial.println();
  delay(100);
}
