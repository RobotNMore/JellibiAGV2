////////////////////  버튼 핀 번호

#define pinButton     A3 // 버턴 핀 번호

////////////////////  메인 프로그램 (setup & loop)

void  setup()
{
  pinMode( pinButton, INPUT ); // 버튼을 입력 모드로
  Serial.begin( 9600 ); // 시리얼 통신 속도 설정
}

void  loop()
{
  int value = digitalRead( pinButton );
  // 버튼 눌림 값 읽기 : 눌림= 0, 올림= 1
  
  Serial.print( "V= " );   // 따옴표 안의 내용 출력
  Serial.println( value ); // 읽은 값 출력 (줄넘김)
  
  delay( 100 );  // 0.1초 후에 다시 버튼 눌림값 체크
}
