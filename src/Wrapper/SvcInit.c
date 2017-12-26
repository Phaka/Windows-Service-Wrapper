#include "stdafx.h"
#include "wrapper.h"

VOID SvcInit(DWORD dwArgc, LPTSTR* lpszArgv)
{
	// TO_DO: Declare and set any required variables.
	//   Be sure to periodically call ReportSvcStatus() with 
	//   SERVICE_START_PENDING. If initialization fails, call
	//   ReportSvcStatus with SERVICE_STOPPED.

	// Create an event. The control handler function, SvcCtrlHandler,
	// signals this event when it receives the stop control code.

	ghSvcStopEvent = CreateEvent(
		NULL, // default security attributes
		TRUE, // manual reset event
		FALSE, // not signaled
		NULL); // no name

	if (ghSvcStopEvent == NULL)
	{
		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}

	// Report running status when initialization is complete.

	ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

	// TO_DO: Perform work until service stops.

	// Check whether to stop the service.
	WaitForSingleObject(ghSvcStopEvent, INFINITE);

	ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
}
