#pragma once

class CThread
{
public:
	CThread();
	~CThread();

protected:
	/// 각 객체들은 한 부분의 임계지점을 공유해야함-static변수로 지정 
	static CRITICAL_SECTION m_CriticalSection;
	static HANDLE m_hEvent;

	// 다른 로거가 있다면 다른 내용을 담을 것으로 static변수로 안씀
	HANDLE m_hThread;
	
	//vector<HANDLE> vThreads;

public:
	// 쓰레드 관련
	void Create();		// 쓰레드를 생성하고 ThreadFunction함수 포인터를 받아서 main문을 처리
	void Resume();		// 재개
	void Suspend();		// 대기
	void Destroty();	// 종료

	// CriticalSection 관련
	static void InitializeCS();
	static void EndCS(); // 이벤트와 CS를 같이 끝내자
	/// enter와 leave는 클래스로 만들어서 처리

	// 이벤트 관련 
	static void EventCreate();
	static void EventSet();
	static void EventReset();
	static void EventClose();

	// 쓰레드가 메인으로 처리할 가상함수
	virtual int main();

	// 쓰레드를 main 함수로 보냄
	static unsigned int __stdcall ThreadFunction(void* arg);
};

