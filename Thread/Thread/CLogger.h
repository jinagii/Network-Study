#pragma once
#include "CThread.h"

class CLockPoint;

class CLogger : public CThread
{
public:
	CLogger();
	~CLogger();

public:
	// 쓰다가 수정된 변수들
	//CThread* hThread;

private:
	//string m_MyFullChat;
	//string m_MyChat;
	queue<string> m_MyChatQueue;	

public:
	virtual int main();	
	void InputLog();
	string LogTimeInformation();
};

