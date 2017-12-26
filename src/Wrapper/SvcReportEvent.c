#include "stdafx.h"
#include "wrapper.h"

DWORD SVC_ERROR = 0;

VOID SvcReportEvent(LPTSTR szFunction)
{
	HANDLE hEventSource;
	LPCTSTR lpszStrings[2];
	TCHAR Buffer[80];

	hEventSource = RegisterEventSource(NULL, SVCNAME);

	if (NULL != hEventSource)
	{
		StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());

		lpszStrings[0] = SVCNAME;
		lpszStrings[1] = Buffer;

		ReportEvent(hEventSource, // event log handle
			EVENTLOG_ERROR_TYPE, // event type
			0, // event category
			SVC_ERROR, // event identifier
			NULL, // no security identifier
			2, // size of lpszStrings array
			0, // no binary data
			lpszStrings, // array of strings
			NULL); // no binary data

		DeregisterEventSource(hEventSource);
	}
}