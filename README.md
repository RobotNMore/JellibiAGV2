# JellibiAGV 2
**Jellibi** **A**utomated **G**uided **V**ehicle

### 개요

이 저장소의 코드들은 격자형 맵위에서 빠레뜨를 원하는 위치로 옮기는 물류이송로봇의 기능들이 구현되어 있습니다. **2020 WCRC 1차 본선 종목 중 물류로봇 종목**의 규정을 참고하였습니다.

JellibiAGV2 는 아두이노 나노 호환보드를 중심으로 RFID TAG 를 인식하기 위하여 연결된 RFID 리더, 빠레뜨를 들어올리는 한개의 서보모터와 라인트레이서 구현에 필요한 두개의 IR 센서, 바퀴를 굴리기 위한 두개의 DC 모터로 구성됩니다. 

코드는 src 디렉토리 아래에 총 다섯개의 샘플 프로젝트로 구성되어 있습니다.  
```00.UnitTest``` 폴더에는 H/W를 확인하고 조정하는데 도움을 주는 IOCheck 프로젝트가 있으며,   
```01.Application``` 폴더에는 아두이노 입출력 제어와 물류로봇 제어 응용 실습 프로젝트가 들어 있습니다.  
파일 구조는 아래와 같이 되어있습니다.

<ul>
	<li style="list-style:none">
		src
		<ul style="list-style:none">
			<li>ㄴ00.UnitTest
				<ul style="list-style:none">
					<li>ㄴIOCheck</li>
				</ul>			
			</li>
			<li>ㄴ01.Applications
				<ul style="list-style:none">
					<li>ㄴJellibi-AGV-01-01-Servo-Basic</li>
					<li>ㄴJellibi-AGV-01-02-Servo2-Basic</li>
					<li>ㄴJellibi-AGV-02-01-Lifter-Basic</li>
					<li>ㄴ...</li>
				</ul>
			</li>
		</ul>
	</li>
</ul>  


이 프로젝트들을 빌드하여 AGV2 에서 테스트 하기 위해서는 기본적인 아두이노 개발환경을 설치하고 USB 포트를 통하여 보드와 연결하고 코드를 다운로드 할 준비가 되어 있어야 합니다. 
다음 단락에서는 코드를 이 저장소에서 내려받고 아두이노 개발툴에서 샘플을 AGV2 로 다운로드 하는 방법을 설명합니다. 

아두이노 툴을 다운로드하고 설치하는 방법은 인터넷의 많은 가이드들을 참고하기 바랍니다. 

### 코드 다운로드 

모든 코드는 Git 툴을 이용하거나 이 페이지에서 Zip 파일로 다운로드 할수 있습니다. 
파일목록 바로 위의 버튼들 중 "Clone or download" 을 선택하여 "Download ZIP" 메뉴를 선택하면 이 코드저장소의 모든 파일을 다운로드 받을 수 있습니다. 
데스크탑에서 다운받은 ZIP 파일을 압축 풀어 둡니다. 

### 외부라이브러리 설치 

경기의 시작을 인식하거나 팔레트를 어느 위치로 배송하여야 하는지 구분하기 위하여 RFID 리더기를 사용합니다. 
AGV 는 경기장에서 RFID 를 계속해서 탐지 하고 "시작 RFID 태그" 를 인식하게 되면 우리가 코딩을 통해 정한 룰대로 움직입니다. 또, 팔레트 아래면에는 RFID 태그를 붙여두어 AGV 가 팔레트 아래로 이동하면 팔레트의 RFID 를 읽을 수 있도록 준비되어 있습니다. 

이 RFID 리더기를 제어하기 위하여 사용하는 라이브러리는 아래 GitHub 주소에서 다운받습니다. 
다운받은 라이브러리를 아두이노 툴에서 사용할 수 있도록 설치합니다.  

RFID 리더 제어 코드 저장소 : https://github.com/miguelbalboa/rfid

라이브러리 다운받기 링크  : https://github.com/miguelbalboa/rfid/archive/master.zip

샘플 프로그램 중 **IOCheck** 는 측면의 사용자버튼의 동작을 감지하기 위하여 JellibiButton 라이브러리를 사용합니다. 
아래 JellibiButton 라이브러리 저장소에서 코드를 다운로드 하고 아두이노 툴에 적용합니다. 

JellibiButton  코드 저장소 :https://github.com/RobotNMore/JellibiButton

라이브러리 다운받기 링크 : https://github.com/RobotNMore/JellibiButton/archive/master.zip 


### 빌드 및 다운로드 

JellibiAGV 2 오른쪽에는 코딩을 하는 PC 와 USB 로 연결할 수 있는 USB 포트가 하나 있으며 왼쪽 측면에는 코딩에서 사용할 수 있는 버튼과 전원을 켜고 끌 수 있는 스위치가 있습니다. 
코딩한 결과물을 보드에 업로드 할 때에는 전원버튼을 앞쪽으로 밀어 전원을 켜둔 상태여야 합니다. 우선 JellibiAGV 2 는 전원을 켜두고 USB 케이블로 PC 와 JellibiAGV 2 를 연결합니다. 

1. 아두이노 툴에서 JellibiAGV 2 의 연결을 설정합니다. 
	보드 : Arduino Nano 
	프로세서 : ATmega328P
	포트 : JellibiAGV 2 와 연결된 시리얼 포트 선택 
2.  업로드를 선택하여 코드를 빌드하고 JellibiAGV 2 로 코드를 업로드합니다. 
