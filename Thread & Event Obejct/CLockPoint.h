#pragma once

/// 수업시간 교수님이 말하셨던 소멸자에 leaveCS 팁 적용해보기
class CLockPoint
{
public:
	CLockPoint(CRITICAL_SECTION* CS);
	~CLockPoint(); // 이부분에 leaveCS 넣어줌-지역스코프에서 소멸될때 자동으로 처리

private:
	CRITICAL_SECTION* m_CS;

public:
	// 삽질 했다...
	//void InitiateCS();	//
	//void EnterCS();		// 지역스코프에 init과 enter를 넣어준다
	//void DeleteCS();	// 프로그램 종료시

};

