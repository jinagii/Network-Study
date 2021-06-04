#include <process.h>
#include <iostream>
#include <windows.h>
#include <assert.h>
#include <vector>

using namespace std;

#include "CThread.h"

// ���������� cpp���� ������ ����� ��밡��
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
	// ������ ������ �ο��� ID
	unsigned int uiThreadID = 0; 
	// �����带 �����ϰ� �Լ������͸� �޾Ƽ� ������ �ο� (�����·� ����)
	m_hThread = reinterpret_cast<HANDLE>(
		_beginthreadex(NULL, 0, ThreadFunction, this, CREATE_SUSPENDED, &uiThreadID));
}

void CThread::Destroty()
{
	// �����尡 �Ϸ����� Ȯ���ϰ�
	WaitForSingleObject(m_hThread, INFINITE);	
	// �����带 ����
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
	// ������ ������ �Բ� �Ӱ������� init
	InitializeCriticalSection(&m_CriticalSection);
}

void CThread::EndCS()
{
	// �̺�Ʈ�� ����
	CloseHandle(m_hEvent);
	// �Ӱ� ���� ����
	DeleteCriticalSection(&m_CriticalSection);
}

void CThread::EventCreate()
{	
	// waitforsingleobject�� �ڵ����� ó���� 2��° ���� false
	// 3��° ���ڴ� �ʱ� signal ���� false : non-signal
	m_hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	assert(nullptr != m_hEvent);		// ����׽� Ȱ��
}

void CThread::EventSet()
{
	SetEvent(m_hEvent); // non-signal -> signal ��ȯ
}

/// �ڵ����� �̺�Ʈ ó���ÿ��� �ʿ���� 
/// waitforsingleobject�� �ڵ����� signal -> non-signal�� �ٲ�
void CThread::EventReset()
{
	ResetEvent(m_hEvent); // signal -> non-signal ��ȯ
}

void CThread::EventClose()
{
	CloseHandle(m_hEvent);	// destroy���� ó����
}

int CThread::main()	// �����尡 ó���� ���� �����Լ�
{
	// �� ���� ��
	return 0;	
}

unsigned int __stdcall CThread::ThreadFunction(void* arg)
{
	/// �����带 main �Լ��� ����
	CThread* nowThread = reinterpret_cast<CThread*>(arg);
	nowThread->main();

	return 0;
}

