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

/// �ʱ� �� main�� �� ���� cpp

/// ũ��Ƽ�� ����
CRITICAL_SECTION g_CriticalSection;

string myFullchat;
queue<string> myChatQueue;
ofstream writeLog;

// ��¥ �ð� ���� ���� �ϴ� �Լ�
string GetTimeInformation()
{
	// �α׿� ���� �ð�
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
		"-" + day + // ��� �ؾ� ���ڸ����� ǥ���Ҽ�������
		"-" + hour +
		":" + min +
		":" + sec;

	return ('[' + strCurTimeInfo + "] ");
}

/// 1. �Է¹��� ���ڿ����� ���Ͽ� ���
/// 2. main���κ��� ���� ��ȣ ������ return
unsigned int __stdcall ThreadFunction(void* arg)
{
	// �� ���� ����
	writeLog.open("LOG.txt");

	while (true)
	{
		/// ť�� �պκ��� ������ �׳� �Ѿ
		if (myChatQueue.empty() == true) continue;

		EnterCriticalSection(&g_CriticalSection);
		
		/// �������� ���͸� �Լ��� ����
		if (myChatQueue.front() == "") break;

		//myFullchat += GetTimeInformation() + myChatQueue.front();

		writeLog << GetTimeInformation() + myChatQueue.front() << endl;

		myChatQueue.pop();
		//writeLog.write(myFullchat.c_str(), myFullchat.size());

		LeaveCriticalSection(&g_CriticalSection);
	}

	return 0;
}

/// 1. ����� �Է¹��� (���ڿ��� �����̽� �Էµ� �����ؾ���)
/// 2. ���� ���ڿ��� thread �ʿ� �ѱ�
/// 3. �ƹ��Էµ� ���� �ʰ� enter�� ������ ���α׷� ���� (thread�� return�� ���� ���� ������ main ����)
int main(void)
{
	/// �Ӱ����� ����
	InitializeCriticalSection(&g_CriticalSection);

	/// ������ ����
	HANDLE hThreads = nullptr;

	/// ������ ���
	unsigned int uiThreadID = 0;
	hThreads = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, ThreadFunction, 0, CREATE_SUSPENDED, &uiThreadID));

	/// ������ �ٽ� on
	ResumeThread(hThreads);

	while (true)
	{
		// ���Ͽ� �� ������ ä��
		string myChat;
		cout << "Type my chat: ";
		getline(cin, myChat); // ���⵵ ����

		myChatQueue.push(myChat); // queue�� mychat�� ���� ����

		if (myChat == "") break;
	}

	/// ������ ��ٸ�
	WaitForSingleObject(hThreads, INFINITE);

	// ������ ������ �ݾ������
	writeLog.close();

	/// �����嵵 �ݾ������
	CloseHandle(hThreads);

	/// �Ӱ������� ��������
	DeleteCriticalSection(&g_CriticalSection);

	return 0;

	/// �������� �ڵ��...?!
	/*
	// �α׿� ������ full ä��
	while (true)
	{
		// ���Ͽ� �� ������ ä��
		string myChat;
		cout << "Type my chat: ";
		getline(cin, myChat); // ���⵵ ����

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
			// �α׿� ���� �ð�
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
				"-" + day + // ��� �ؾ� ���ڸ����� ǥ���Ҽ�������
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