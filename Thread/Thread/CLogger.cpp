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
	// �޸����� open
	fstream _writeLog;
	// write�� deny��: �޸��� �����Ұ�
	_writeLog.open("LOG.txt", ios_base::out, _SH_DENYWR); 

	while (true)
	{
		/// signal�� ��ٸ���
		WaitForSingleObject(m_hEvent, INFINITE);

		// ť�� �պκ��� ������ �׳� �Ѿ
		if (m_MyChatQueue.empty() == true) continue;

		// �������� ���͸� �Լ��� ����
		if (m_MyChatQueue.front() == "") break;

		/// �Ӱ������� ����
		CLockPoint _lock(&m_CriticalSection);
		//EnterCriticalSection(&m_CriticalSection);

		// string���� �ڿ� �̾ �α׿� ���� 
		_writeLog << LogTimeInformation() + m_MyChatQueue.front() << endl;

		m_MyChatQueue.pop();

		/// �������������� �Ҹ� �Ǹ鼭 �ڵ����� ó��
		//LeaveCriticalSection(&m_CriticalSection);
	}

	// ������ �޸����� ����
	_writeLog.close();

	return 0;
}

void CLogger::InputLog()
{
	while (true)
	{
		// ä�� �Է� �ݺ�
		string _myChat;
		cout << "Type my chat: "; // ä�� input
		getline(cin, _myChat); // ���⵵ ����

		m_MyChatQueue.push(_myChat); // queue�� ����-�����尡 ó��

		EventSet(); // ������ �̺�Ʈ ��: signal�� ��ȯ: ���� ���ض�!
		
		if (_myChat == "") break; // �� ���� ġ�� �α� ����
	}
}

string CLogger::LogTimeInformation()
{
	// �α׿� ���� �ð�
	time_t curTime = time(NULL);
	struct tm timeInfo;
	localtime_s(&timeInfo, &curTime);

	// ��� �ؾ� ���ڸ����� ǥ���Ҽ�������?
	// string�� char������ �Դٰ����Ѵ�...
	// string���� + �ϴ°� ���ؼ� char������ ���ڸ��� ������ְ� string���� ��ȯ
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

	// ���� �ð���¥ ���� string
	string strCurTimeInfo =
		to_string(1900 + timeInfo.tm_year) +
		"-" + month +
		"-" + day +
		"-" + hour +
		":" + min +
		":" + sec;

	return ('[' + strCurTimeInfo + "] ");
}
