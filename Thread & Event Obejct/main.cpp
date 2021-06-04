#include <stdio.h>
#include <fstream>
#include <iostream>
#include <process.h>
#include <Windows.h> 
#include <time.h>
#include <string>
#include <queue>

using namespace std;
#pragma warning(disable:4996)

/// 초기 한 main에 다 넣은 cpp

/// 크리티컬 섹션
CRITICAL_SECTION g_CriticalSection;

string myFullchat;
queue<string> myChatQueue;
ofstream writeLog;

// 날짜 시간 정보 생성 하는 함수
string GetTimeInformation()
{
	// 로그에 쓰일 시간
	time_t curTime = time(NULL);
	struct tm timeInfo;
	localtime_s(&timeInfo, &curTime);

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

	string strCurTimeInfo =
		to_string(1900 + timeInfo.tm_year) +
		"-" + month +
		"-" + day + // 어떻게 해야 두자리수로 표현할수있을까
		"-" + hour +
		":" + min +
		":" + sec;

	return ('[' + strCurTimeInfo + "] ");
}

/// 1. 입력받은 문자열들을 파일에 기록
/// 2. main으로부터 종료 신호 받으면 return
unsigned int __stdcall ThreadFunction(void* arg)
{
	// 쓸 파일 열기
	writeLog.open("LOG.txt");

	while (true)
	{
		/// 큐의 앞부분이 있으면 그냥 넘어감
		if (myChatQueue.empty() == true) continue;

		EnterCriticalSection(&g_CriticalSection);
		
		/// 마지막이 엔터면 함수를 끝냄
		if (myChatQueue.front() == "") break;

		//myFullchat += GetTimeInformation() + myChatQueue.front();

		writeLog << GetTimeInformation() + myChatQueue.front() << endl;

		myChatQueue.pop();
		//writeLog.write(myFullchat.c_str(), myFullchat.size());

		LeaveCriticalSection(&g_CriticalSection);
	}

	return 0;
}

/// 1. 사용자 입력받음 (문자열에 스페이스 입력도 가능해야함)
/// 2. 받은 문자열을 thread 쪽에 넘김
/// 3. 아무입력도 받지 않고 enter가 눌리면 프로그램 종료 (thread는 return을 통한 정상 종료후 main 종료)
int main(void)
{
	/// 임계지점 생성
	InitializeCriticalSection(&g_CriticalSection);

	/// 쓰레드 생성
	HANDLE hThreads = nullptr;

	/// 쓰레드 대기
	unsigned int uiThreadID = 0;
	hThreads = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, ThreadFunction, 0, CREATE_SUSPENDED, &uiThreadID));

	/// 쓰레드 다시 on
	ResumeThread(hThreads);

	while (true)
	{
		// 파일에 쓸 각각의 채팅
		string myChat;
		cout << "Type my chat: ";
		getline(cin, myChat); // 띄어쓰기도 받음

		myChatQueue.push(myChat); // queue에 mychat을 집어 넣음

		if (myChat == "") break;
	}

	/// 쓰레드 기다림
	WaitForSingleObject(hThreads, INFINITE);

	// 끝날때 파일을 닫아줘야함
	writeLog.close();

	/// 쓰레드도 닫아줘야함
	CloseHandle(hThreads);

	/// 임계지점도 지워야함
	DeleteCriticalSection(&g_CriticalSection);

	return 0;

	/// 시행착오 코드들...?!
	/*
	// 로그에 저장할 full 채팅
	while (true)
	{
		// 파일에 쓸 각각의 채팅
		string myChat;
		cout << "Type my chat: ";
		getline(cin, myChat); // 띄어쓰기도 받음

		if (myChat == "")
		{
			for (int i = 0; i < ciThreadCount; i++)
			{
				unsigned int uiThreadID = 0;
				hThreads[i] = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, ThreadFunction, 0, CREATE_SUSPENDED, &uiThreadID));
			}

			break;
		}
		else
		{
			// 로그에 쓰일 시간
			time_t curTime = time(NULL);
			struct tm timeInfo;
			localtime_s(&timeInfo, &curTime);

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

			string strCurTimeInfo =
				to_string(1900 + timeInfo.tm_year) +
				"-" + month +
				"-" + day + // 어떻게 해야 두자리수로 표현할수있을까
				"-" + hour +
				":" + min +
				":" + sec;

			myFullchat += '[' + strCurTimeInfo + "] " + myChat + '\n';

			//sprintf(&myTotalChat, "[%04d-%02d-%02d-%02d-%02d-%02d] %s", 1900 + timeInfo.tm_year, 1 + timeInfo.tm_mon, timeInfo.tm_mday, timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
			//myFullchat.to_string(myTotalChat);
		}
	}
	*/

	//for (int i = 0; i < ciThreadCount; i++)
	//{
	//	ResumeThread(hThreads[i]);
	//}

	//WaitForMultipleObjects(2, hThreads, true, INFINITE);

	//for (int i = 0; i < ciThreadCount; i++)
	//{
	//}

}