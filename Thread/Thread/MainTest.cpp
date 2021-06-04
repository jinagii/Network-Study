//#include <string>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <process.h>
#include <Windows.h> 
#include <time.h>
#include <string>
#include <queue>
#include <vector>

using namespace std;
#pragma warning(disable:4996)

#include "CLogger.h"

int main()
{
	// logger 생성
	CLogger* Logger = new CLogger;

	/// 쓰레드를 생성
	Logger->Create();

	/// CS 초기화 
	Logger->InitializeCS();

	/// 이벤트 쓰레드 생성
	Logger->EventCreate();

	/// 쓰레드를 재개
	Logger->Resume();
	
	// 큐에 chat log를 집어 넣는다
	Logger->InputLog();
	/// 쓰레드에서 알아서 CLogger main으로 가서 처리한다!!!!!
	//Logger->main(); 위에것을 몰라서 main을 또 돌리고 있었다...

	/// 쓰레드를 종료한다
	Logger->Destroty();

	/// 이벤트 쓰레드 종료
	Logger->EndCS();
	//Logger->EventClose();

	return 0;
}