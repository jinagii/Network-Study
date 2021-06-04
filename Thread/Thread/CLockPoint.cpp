#include <windows.h>
#include "CLockPoint.h"

CLockPoint::CLockPoint(CRITICAL_SECTION* CS)
{	
	m_CS = CS;
	EnterCriticalSection(m_CS);
}

CLockPoint::~CLockPoint()
{
	// return, break, continue��� �ڵ����� ó��
	LeaveCriticalSection(m_CS);
}
