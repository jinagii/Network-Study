#include <windows.h>
#include "CLockPoint.h"

CLockPoint::CLockPoint(CRITICAL_SECTION* CS)
{	
	m_CS = CS;
	EnterCriticalSection(m_CS);
}

CLockPoint::~CLockPoint()
{
	// return, break, continueµî¿¡¼­ ÀÚµ¿À¸·Î Ã³¸®
	LeaveCriticalSection(m_CS);
}
