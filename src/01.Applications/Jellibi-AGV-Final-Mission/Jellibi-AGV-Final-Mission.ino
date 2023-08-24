#include <MFRC522.h>
#include <MFRC522Extended.h>
#include <require_cpp11.h>
#include <SPI.h>

#define RFID_SS_PIN     2 // SPI 통신용 SS(Slave Select)
#define RFID_RST_PIN    4 // RESET 핀

MFRC522 mfrc522( RFID_SS_PIN, RFID_RST_PIN );

////////////////////  부저와 버튼 핀 번호

#define pinBuzzer     3  // 부저 핀 번호
#define pinButton     A3 // 버턴 핀 번호

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

//////////////  바닥의 라인트레이스 IR센서

#define pinLT1  A6 // 1번(왼쪽) IR센서 연결 핀
#define pinLT2  A7 // 2번(오른쪽) IR센서 연결 핀

// 색상 판단: White=[0..410] .. 560 .. [710..1023]=Black
#define LT_AJDUST       60  // 현재 젤리비의 센서 측정 조정값
#define LT_MAX_WHITE   410 + LT_AJDUST // 흰색으로 판단하는 최대값
#define LT_MID_VALUE   560 + LT_AJDUST // 흑백 판단 경계값(중간값)
#define LT_MIN_BLACK   710 + LT_AJDUST // 검은색으로 판단하는 최소값

#define pinLT3  A0  // 전면 왼쪽 IR센서
#define pinLT4  A1  // 전면 중안 IR센서
#define pinLT5  A2  // 전면 오른쪽 IR센서

#define IR4_OBSTACLE_TEST   0 // 장애물까지의 거리 측정 테스트용

// 만약 전방 장애물 인식이 잘 되지 않는 경우는
// IR4_OBSTACLE_TEST 값을 1로 설정하여 전방 장애물 까지의 거리를
// 측정하여 장애물을 감지할 범위의 최대값을 설정하여 사용해야 합니다.
// (대부분의 경우 적당한 IR4 감지 거리는 990~1005 사이값 입니다.)
int ObstacleDistance = 990;  // 장애물 존재 여부 판단 최대 거리값, 1005

bool  CheckObstacle()
{
  bool  bObstacle = false;
  
  if( analogRead( pinLT4 ) < ObstacleDistance ) // 장애물 발견
  {
    delay( 2 );
    
    if( analogRead( pinLT4 ) < ObstacleDistance ) // 장애물 발견 재확인
      bObstacle = true;
  }

  return  bObstacle;
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

////////////////////  메인 프로그램 (setup & loop)

void  setup()
{  
  pinMode( pinDIR1, OUTPUT ); // 1번(왼쪽)모터 방향 핀
  pinMode( pinPWM1, OUTPUT ); // 1번(왼쪽)모터 속력 핀
  pinMode( pinDIR2, OUTPUT ); // 2번(오른쪽)모터 방향 핀
  pinMode( pinPWM2, OUTPUT ); // 2번(오른쪽)모터 속력 핀

  pinMode( pinBuzzer, OUTPUT ); // 출력 핀으로 설정  
  noTone( pinBuzzer );  // 스피커 끄기(음소거)
  
  Serial.begin( 9600 ); // 시리얼 통신 (bps)

  LifterDown(); // 리프터를 아래로 내림

  SPI.begin(); // RFID 카드 사용을 위해 SPI 통신 초기화
  mfrc522.PCD_Init(); // MFRC522 리더 모듈 초기화

#if IR4_OBSTACLE_TEST  // 장애물까지의 최대 거리 센서값 확인 
  while( 1 )
  {
    ObstacleDistance = analogRead( pinLT4 ); // 980~1010
    
    Serial.print( "ObstacleDistance= " ); // 로봇의 센서값을
    Serial.println( ObstacleDistance );   // 테스트해서 설정
    delay( 500 );
  }
#endif  
}


#define TURN_POWER  100 // 회전 속력 (주행 속력과는 별개로 고정)

int Power = 80;  // 기본 주행(라인트레이싱) 속력
int SkipLineTime = 200; // 직진하여 정지선/교차로 통과할 시간

int RunState = 0; // 주행 상태 (처음에는 정지, 버튼으로 출발)
int SelectedPath; // 경유로 번호 (1=Left, 2=Center, 3=Right)


void  TurnLeft90()
{
  Stop(); // 정지
  delay( 50 ); // (달려가던 관성 때문에 약간 앞으로 밀림)
  drive(BACKWARD, 80, BACKWARD, 80); // 아주 살짝 후진
  delay( 20 );
  Stop(); // 다시 정지
  delay( 50 );

  // 이하, 왼쪽 LT 센서만 사용하여 왼쪽으로 90도 회전함
  
  TurnLeft( TURN_POWER ); // 1차 좌회전 진행 (현재 라인 벗어나기)
  while( analogRead(pinLT1) < LT_MAX_WHITE ) // 흰색인 동안
    delay( 1 );
  delay( 40 );
  
  TurnLeft( TURN_POWER ); // 2차 좌회전 진행 (반대쪽 흰바탕 도착)
  while( analogRead(pinLT1) > LT_MIN_BLACK ) // 검은색인 동안
    delay( 1 );
  delay( 40 );

  Forward( TURN_POWER );
  delay( 90 );
  
  TurnLeft( TURN_POWER ); // 좌회전 끝 마무리
  delay( 250 ); // 속도나 무게에 따라 조정

  Forward( Power ); // 좌회전 이후의 직진 시작
}

void  TurnRight90()
{
  Stop(); // 정지
  delay( 50 ); // (달려가던 관성 때문에 약간 앞으로 밀림)
  drive(BACKWARD, 80, BACKWARD, 80); // 아주 살짝 후진
  delay( 20 );
  Stop(); // 다시 정지
  delay( 50 );

  // 이하, 오른쪽 LT 센서만 사용하여 오른쪽으로 90도 회전함
  
  TurnRight( TURN_POWER ); // 1차 우회전 진행 (현재 라인 벗어나기)
  while( analogRead(pinLT2) < LT_MAX_WHITE ) // 흰색인 동안
    delay( 1 );
  delay( 40 );
  
  TurnRight( TURN_POWER ); // 2차 우회전 진행 (반대쪽 흰바탕 도착)
  while( analogRead(pinLT2) > LT_MIN_BLACK ) // 검은색인 동안
    delay( 1 );
  delay( 40 );

  Forward( TURN_POWER );
  delay( 90 );
  
  TurnRight( TURN_POWER ); // 우회전 끝 마무리
  delay( 250 ); // 속도나 무게에 따라 조정

  Forward( Power ); // 우회전 이후의 직진 시작
}

void  TurnLeft180( bool bBack )
{
  if( bBack ) // (정지선에서) 살짝 후진하고 회전 시작
  {
    drive(BACKWARD, 70, BACKWARD, 70); // 뒤로 후진
    while( (analogRead(pinLT1) > LT_MIN_BLACK) ||
            (analogRead(pinLT2) > LT_MIN_BLACK) )
      delay( 1 ); // 정지선을 벗어날때까지 계속 후진
    delay( 130 ); // 속도나 무게에 따라 조정
  }
  else // 라인 위에서 직진하다가 바로 180도 회전 할 때
  {
    drive(BACKWARD, 90, BACKWARD, 90); // (위치 조정용) 약간 후진
    delay( 150 );
    drive(FORWARD, 0, FORWARD, 90); // LT1을 라인 좌측 밖으로
    delay( 300 );
  }

  drive(BACKWARD, TURN_POWER, FORWARD, TURN_POWER);
  while( analogRead(pinLT1) < LT_MIN_BLACK)
    delay( 1 ); // 왼쪽이 검은색이 아니면 계속 좌회전
  delay( 30 );

  drive(BACKWARD, 90, FORWARD, 90);
  while( analogRead(pinLT1) > LT_MAX_WHITE )
    delay( 1 ); // 왼쪽이 흰색이 아닌 동안 계속 좌회전
  
  Forward( Power ); // (180도 회전 이후의) 직진 시작
}

//////////////////////////////

// 180도 회전 직후에 (로봇의 라인 정렬을 위해)
// 멈추기 전에 약간의 라인트레이싱 시간을 주기 위해 사용함
unsigned long   TickStart;

void  DoLineTracing()
{
  int v1 = analogRead( pinLT1 );
  int v2 = analogRead( pinLT2 );

  if( (LT_MIN_BLACK < v1) && (LT_MIN_BLACK < v2) )
  { // 양쪽 IR센서 모두 검정색을 감지한 경우, (ex) 교차선
    
    switch( RunState )  // 주행 상태에 따른 정지 동작
    {
    case 1:  // 중앙(2번 경유로) 장애물 검사
      Stop(); // 정지 상태에서 거리를 측정
      delay( 100 );

      if( CheckObstacle() ) // 장애물 감지
      {
        RunState = 11; // 좌측(1번) 경유로 장애물 체크하러 좌회전 전진
        TurnLeft90();
      }
      else // 앞에 장애물이 없으면
      {
        SelectedPath = 2; // 중앙(2번) 경유로 선택
        RunState = 102; // 가운데 길로 계속 전진
        Forward( Power );
        delay( SkipLineTime );
      }
      break;

    case 11:  // 좌측(1번 경유로) 장애물 검사하기 위해 우회전
      TurnRight90();  // 우회전 후에 정면을 바라보도록
      RunState = 12;  // 약간 전진(시간차 정지 후 거리 측정)
      TickStart = millis(); // POWER가 켜진 후 경과 시간(ms)
      break;

    case 13: // 우측 경유로로 가기 위해 뒤돌아 가다 정지선
      TurnLeft90();
      RunState = 14;  // 우측(3번) 경유로로 이동시작
      break;

    case 14:  // 우측(3번) 경유로 장애물 검사하러 중앙선 통과
      RunState = 15; // 우측 장애물 검사하러 전진
      Forward( Power );
      delay( SkipLineTime );
      break;

    case 15:  // 우측 장애물 검사는 생략 (마지막 남은 경유로)
      TurnLeft90();
      RunState = 103; // 우측 경유로 진입하여 전진
      break;

    case 101: // 좌측 경유로 통과
      TurnRight90();
      RunState = 111; // 우회전하여 목표지점으로 전진
      break;

    case 102: // 중앙 경유로 통과
      Forward( Power );
      delay( SkipLineTime );
      RunState = 201; // 목표지점으로 전진
      break;

    case 103: // 우측 경유로 통과
      TurnLeft90();
      RunState = 113; // 좌회전하여 목표지점으로 전진
      break;

    case 111: // 좌측 경유 목표지점 진입 교차로 도달
      TurnLeft90();
      RunState = 201; // 좌회전하여 목표지점으로 전진
      break;

    case 113: // 우측 경유 목표지점 진입 교차로 도달
      TurnRight90();
      RunState = 201; // 우회전하여 목표지점으로 전진
      break;
     
    case 201: // 중간 목표지점 도착 전진
      Stop();

      LifterUp(); // 리프터를 위로 올림

      tone( pinBuzzer, 262 );  // 도(4옥타브 C)
      delay( 100 );
      tone( pinBuzzer, 330 );  // 미(4옥타브 E)
      delay( 100 );
      tone( pinBuzzer, 392 );  // 솔(4옥타브 G)
      delay( 250 );
      noTone( pinBuzzer );     // 음소거(mute)
      
      delay( 2000 );  // 팔레트(화물) 싣는 시간

      RunState = 203;  // 뒤돌아 홈으로 출발
      
      TurnLeft180( true ); // (정지선에서) 살짝 후진하고 회전
      break;

    case 203: // 올때 선택한 경로에 따라서 되돌아갈 방향 선택
      if( SelectedPath == 1 ) // 좌측 경유로
      {
        RunState = 301;  // 좌측 경유로 따라서 귀환
        TurnRight90();
      }
      else if( SelectedPath == 3 ) // 우측 경유로
      {
        RunState = 401;  // 우측 경유로 따라서 귀환
        TurnLeft90();
      }
      else // SelectedPath == 2, 중앙 경유로로 직진
      {
        RunState = 204;  // 교차로를 지나 중앙으로 귀환
        Forward( Power );
        delay( SkipLineTime );
      }
      break;
      
    case 204: // 귀환 중 중앙 경유로 통과
      RunState = 501;  // 귀환 진입
      Forward( Power );
      delay( SkipLineTime );
      break;

    case 301: // 귀환 중 좌측 경유로 진입
      RunState = 302;  // 좌측 경유
      TurnLeft90();
      break;

    case 302: // 귀환 중 좌측 경유로 통과
      RunState = 303;  // 좌측 경유
      TurnLeft90();
      break;

    case 303: // 귀환 중 (좌측 경유로 통과 후) 좌측 진입
      RunState = 501;  // 귀환 진입
      TurnRight90();
      break;

    case 401: // 귀환 중 우측 경유로 진입
      RunState = 402;  // 우측 경유
      TurnRight90();
      break;

    case 402: // 귀환 중 우측 경유로 통과
      RunState = 403;  // 우측 경유
      TurnRight90();
      break;

    case 403: // 귀환 중 (우측 경유로 통과 후) 우측 진입
      RunState = 501;  // 귀환 진입
      TurnLeft90();
      break;

    case 501: // A지점 정지선 도착 후 화물 내리고 180도 회전
      Stop();

      LifterDown(); // 리프터를 아래로 내림 xxx

      tone( pinBuzzer, 392 );  // 솔(4옥타브 G)
      delay( 150 );
      tone( pinBuzzer, 330 );  // 미(4옥타브 E)
      delay( 150 );
      tone( pinBuzzer, 262 );  // 도(4옥타브 C)
      delay( 250 );
      noTone( pinBuzzer );     // 음소거(mute)
      
      delay( 2000 ); // 팔레트(화물) 내리는 시간

      RunState = 999;  // 다시 출발 대기 상태로 위치
      
      TurnLeft180( true ); // (정지선에서) 살짝 후진하고 회전
      break;

    case 999: // (최종 180도 회전 후) 약간 후진 후 정지
      Stop(); // 정지
      drive(BACKWARD, 70, BACKWARD, 70); // 약간 후진
      delay( 160 );
      Stop(); // 완료 정지

      tone( pinBuzzer, 330 );  // 미(4옥타브 E)
      delay( 100 );
      tone( pinBuzzer, 262 );  // 도(4옥타브 C)
      delay( 250 );
      noTone( pinBuzzer );     // 음소거(mute)
  
      RunState = 0;  // 출발 대기
      break;
    }
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


void  loop()
{
  switch( RunState )  // 주행 상태에 따른 주행 동작
  {
  case 0:  // 처음 출발전 RFID 카드 태깅 대기
    if( mfrc522.PICC_IsNewCardPresent() ) // 카드 태깅(접근)
    {
      if( mfrc522.PICC_ReadCardSerial() ) // UID 읽기 성공
      { 
        RunState = 1; // 최초 출발
        
        tone( pinBuzzer, 262 );  // 도(4옥타브 C)
        delay( 100 );
        tone( pinBuzzer, 330 );  // 미(4옥타브 E)
        delay( 250 );
        noTone( pinBuzzer );     // 음소거(mute)
      }
    }
    delay( 100 );  // 0.1초 대기
    break;

  case 12:  // 좌측(1번) 경유로에서 거리 측정
    if( millis() - TickStart > 160 )
    {
      Stop(); // 정지 상태에서 거리를 측정
      delay( 100 );

      if( CheckObstacle() ) // 장애물 발견 시
      {
        SelectedPath = 3; // 우측(3번) 경유로 선택
        RunState = 13;  // 우측 경유로로 가기 위해 일단 뒤돌기

        TurnLeft180( false ); // 전진 중 바로 180도 회전
      }
      else // 앞에 장애물 미발견 시
      {
        SelectedPath = 1; // 좌측(1번) 경유로 선택
        RunState = 101; // 좌측 길로 계속 전진
        
        Forward( Power );
      }
    }
    break;
    
  default:  // 대부분의 동작들이 정지선을 만났을 때 일어남
    DoLineTracing();  // 라인트레이싱 진행
    break;
  }
}
