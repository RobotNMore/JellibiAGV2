////////////////////  부저와 버튼 핀 번호

#define pinBuzzer     3  // 부저 핀 번호
#define pinButton     A3 // 버턴 핀 번호

////////////////////  메인 프로그램 (setup & loop)

void  setup()
{
  pinMode( pinButton, INPUT );  // 버튼은 입력 모드로
  pinMode( pinBuzzer, OUTPUT ); // 부저는 출력 모드로  
  
  noTone( pinBuzzer );  // 스피커 끄기(음소거)
}

void  loop()
{
  if( digitalRead(pinButton) == 0 ) // 버튼 눌림?
  {
    tone( pinBuzzer, 261 ); // "도" 음 연주
    delay( 500 );           // 0.5초 유지  
    noTone( pinBuzzer );    // 음소거(mute)
  }

  delay( 10 );  // 10 msec 후에 다시 눌림 체크
}
