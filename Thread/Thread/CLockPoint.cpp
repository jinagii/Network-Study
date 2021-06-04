#include <windows.h>
#include "CLockPoint.h"

CLockPoint::CLockPoint(CRITICAL_SECTION* CS)
{	
	m_CS = CS;
	EnterCriticalSection(m_CS);
}

CLockPoint::~CLockPoint()
{
	// return, break, continue등에서 자동으로 처리
	LeaveCriticalSection(m_CS);
}
