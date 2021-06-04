#include <stdio.h>
#include <fstream>
#include <iostream>
#include <process.h>
#include <Windows.h> 
#include <time.h>
#include <string>
#include <queue>
//#include <vector>

#pragma warning(disable:4996)

using namespace std;

#include "CLockPoint.h"
#include "CLogger.h"

CLogger::CLogger()
{
}

CLogger::~CLogger()
{
}

int CLogger::main()
{
	// 메모장을 open
	fstream _writeLog;
	// write을 deny함: 메모장 수정불가
	_writeLog.open("LOG.txt", ios_base::out, _SH_DENYWR); 

	while (true)
	{
		/// signal을 기다린다
		WaitForSingleObject(m_hEvent, INFINITE);

		// 큐의 앞부분이 있으면 그냥 넘어감
		if (m_MyChatQueue.empty() == true) continue;

		// 마지막이 엔터면 함수를 끝냄
		if (m_MyChatQueue.front() == "") break;

		/// 임계지역에 진입
		CLockPoint _lock(&m_CriticalSection);
		//EnterCriticalSection(&m_CriticalSection);

		// string으로 뒤에 이어서 로그에 쓴다 
		_writeLog << LogTimeInformation() + m_MyChatQueue.front() << endl;

		m_MyChatQueue.pop();

		/// 지역스코프에서 소멸 되면서 자동으로 처리
		//LeaveCriticalSection(&m_CriticalSection);
	}

	// 열었던 메모장을 닫음
	_writeLog.close();

	return 0;
}

void CLogger::InputLog()
{
	while (true)
	{
		// 채팅 입력 반복
		string _myChat;
		cout << "Type my chat: "; // 채팅 input
		getline(cin, _myChat); // 띄어쓰기도 받음

		m_MyChatQueue.push(_myChat); // queue에 넣음-쓰레드가 처리

		EventSet(); // 쓰레드 이벤트 셋: signal로 전환: 이제 일해라!
		
		if (_myChat == "") break; // 빈 것을 치면 로그 종료
	}
}

string CLogger::LogTimeInformation()
{
	// 로그에 쓰일 시간
	time_t curTime = time(NULL);
	struct tm timeInfo;
	localtime_s(&timeInfo, &curTime);

	// 어떻게 해야 두자리수로 표현할수있을까?
	// string과 char형에서 왔다갔다한다...
	// string으로 + 하는게 편해서 char형으로 두자리수 만들어주고 string으로 전환
	string month = "00";
	sprintf((char*)month.c_str(), "%02d", 1 + timeInfo.tm_mon);
	string day = "00";
	sprintf((char*)day.c_str(), "%02d", timeInfo.tm_mday);
	string hour = "00";
	sprintf((char*)hour.c_str(), "%02d", timeInfo.tm_hour);
	string min = "00";
	sprintf((char*)min.c_str(), "%02d", timeInfo.tm_min);
	string sec = "00";
	sprintf((char*)sec.c_str(), "%02d", timeInfo.tm_sec);

	// 최종 시간날짜 정보 string
	string strCurTimeInfo =
		to_string(1900 + timeInfo.tm_year) +
		"-" + month +
		"-" + day +
		"-" + hour +
		":" + min +
		":" + sec;

	return ('[' + strCurTimeInfo + "] ");
}
