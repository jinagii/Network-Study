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
	// logger ����
	CLogger* Logger = new CLogger;

	/// �����带 ����
	Logger->Create();

	/// CS �ʱ�ȭ 
	Logger->InitializeCS();

	/// �̺�Ʈ ������ ����
	Logger->EventCreate();

	/// �����带 �簳
	Logger->Resume();
	
	// ť�� chat log�� ���� �ִ´�
	Logger->InputLog();
	/// �����忡�� �˾Ƽ� CLogger main���� ���� ó���Ѵ�!!!!!
	//Logger->main(); �������� ���� main�� �� ������ �־���...

	/// �����带 �����Ѵ�
	Logger->Destroty();

	/// �̺�Ʈ ������ ����
	Logger->EndCS();
	//Logger->EventClose();

	return 0;
}