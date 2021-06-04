#include <process.h>
#include <iostream>
#include <windows.h>
#include <assert.h>
#include <vector>

using namespace std;

#include "CThread.h"

// 정적변수라 cpp에서 선언을 해줘야 사용가능
CRITICAL_SECTION CThread::m_CriticalSection;
HANDLE CThread::m_hEvent;

CThread::CThread()
{
}

CThread::~CThread()
{
}

void CThread::Create()
{
	// 쓰레드 생성시 부여될 ID
	unsigned int uiThreadID = 0; 
	// 쓰레드를 생성하고 함수포인터를 받아서 역할을 부여 (대기상태로 생성)
	m_hThread = reinterpret_cast<HANDLE>(
		_beginthreadex(NULL, 0, ThreadFunction, this, CREATE_SUSPENDED, &uiThreadID));
}

void CThread::Destroty()
{
	// 쓰레드가 완료됬는지 확인하고
	WaitForSingleObject(m_hThread, INFINITE);	
	// 쓰레드를 종료
	CloseHandle(m_hThread);	
}

void CThread::Resume()
{
	ResumeThread(m_hThread);
}

void CThread::Suspend()
{
	SuspendThread(m_hThread);
}

void CThread::InitializeCS()
{
	// 쓰레드 생성과 함께 임계지점도 init
	InitializeCriticalSection(&m_CriticalSection);
}

void CThread::EndCS()
{
	// 이벤트도 종료
	CloseHandle(m_hEvent);
	// 임계 지점 종료
	DeleteCriticalSection(&m_CriticalSection);
}

void CThread::EventCreate()
{	
	// waitforsingleobject로 자동으로 처리시 2번째 인자 false
	// 3번째 인자는 초기 signal 상태 false : non-signal
	m_hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	assert(nullptr != m_hEvent);		// 디버그시 활용
}

void CThread::EventSet()
{
	SetEvent(m_hEvent); // non-signal -> signal 전환
}

/// 자동으로 이벤트 처리시에는 필요없음 
/// waitforsingleobject는 자동으로 signal -> non-signal로 바꿈
void CThread::EventReset()
{
	ResetEvent(m_hEvent); // signal -> non-signal 전환
}

void CThread::EventClose()
{
	CloseHandle(m_hEvent);	// destroy에서 처리함
}

int CThread::main()	// 쓰레드가 처리할 메인 가상함수
{
	// 할 일을 함
	return 0;	
}

unsigned int __stdcall CThread::ThreadFunction(void* arg)
{
	/// 쓰레드를 main 함수로 보냄
	CThread* nowThread = reinterpret_cast<CThread*>(arg);
	nowThread->main();

	return 0;
}

