#pragma once

class CThread
{
public:
	CThread();
	~CThread();

protected:
	/// �� ��ü���� �� �κ��� �Ӱ������� �����ؾ���-static������ ���� 
	static CRITICAL_SECTION m_CriticalSection;
	static HANDLE m_hEvent;

	// �ٸ� �ΰŰ� �ִٸ� �ٸ� ������ ���� ������ static������ �Ⱦ�
	HANDLE m_hThread;
	
	//vector<HANDLE> vThreads;

public:
	// ������ ����
	void Create();		// �����带 �����ϰ� ThreadFunction�Լ� �����͸� �޾Ƽ� main���� ó��
	void Resume();		// �簳
	void Suspend();		// ���
	void Destroty();	// ����

	// CriticalSection ����
	static void InitializeCS();
	static void EndCS(); // �̺�Ʈ�� CS�� ���� ������
	/// enter�� leave�� Ŭ������ ���� ó��

	// �̺�Ʈ ���� 
	static void EventCreate();
	static void EventSet();
	static void EventReset();
	static void EventClose();

	// �����尡 �������� ó���� �����Լ�
	virtual int main();

	// �����带 main �Լ��� ����
	static unsigned int __stdcall ThreadFunction(void* arg);
};

