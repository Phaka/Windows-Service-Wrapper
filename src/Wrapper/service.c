#include "stdafx.h"
#include "messages.h"
#include "service.h"
#include "wrapper.h"

SERVICE_STATUS          gSvcStatus;
SERVICE_STATUS_HANDLE   gSvcStatusHandle;
HANDLE                  ghSvcStopEvent = NULL;

// Inspired from http://stackoverflow.com/a/15281070/1529139
// and http://stackoverflow.com/q/40059902/1529139
BOOL SendConsoleCtrlEvent(DWORD dwProcessId, DWORD dwCtrlEvent)
{
	BOOL success = FALSE;
	DWORD thisConsoleId = GetCurrentProcessId();
	// Leave current console if it exists
	// (otherwise AttachConsole will return ERROR_ACCESS_DENIED)
	BOOL consoleDetached = (FreeConsole() != FALSE);

	if (AttachConsole(dwProcessId) != FALSE)
	{
		SetConsoleCtrlHandler(NULL, TRUE);
		success = (GenerateConsoleCtrlEvent(dwCtrlEvent, 0) != FALSE);
		FreeConsole();
	}

	if (consoleDetached)
	{
		// Create a new console if previous was deleted by OS
		if (AttachConsole(thisConsoleId) == FALSE)
		{
			int errorCode = GetLastError();
			if (errorCode == 31) // 31=ERROR_GEN_FAILURE
			{
				AllocConsole();
			}
		}
	}
	return success;
}

//
// Purpose: 
//   Entry point for the service
//
// Parameters:
//   dwArgc - Number of arguments in the lpszArgv array
//   lpszArgv - Array of strings. The first string is the name of
//     the service and subsequent strings are passed by the process
//     that called the StartService function to start the service.
// 
// Return value:
//   None.
//
VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
	// Register the handler function for the service
	TCHAR service_name[_MAX_PATH];
	GetServiceName(service_name, _MAX_PATH);

	gSvcStatusHandle = RegisterServiceCtrlHandler(
		service_name,
		SvcCtrlHandler);

	if (!gSvcStatusHandle)
	{
		SvcReportEvent(TEXT("RegisterServiceCtrlHandler"));
		return;
	}

	// These SERVICE_STATUS members remain as set here

	gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	gSvcStatus.dwServiceSpecificExitCode = 0;

	// Report initial status to the SCM

	ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

	// Perform service-specific initialization and work.

	SvcInit(dwArgc, lpszArgv);
}

//
// Purpose: 
//   The service code
//
// Parameters:
//   dwArgc - Number of arguments in the lpszArgv array
//   lpszArgv - Array of strings. The first string is the name of
//     the service and subsequent strings are passed by the process
//     that called the StartService function to start the service.
// 
// Return value:
//   None
//
VOID SvcInit(DWORD dwArgc, LPTSTR *lpszArgv)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	ghSvcStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);   

	if (ghSvcStopEvent == NULL)
	{
		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}

	if (!CreateProcess(NULL,   
		"sample1.exe",        
		NULL,           
		NULL,           
		FALSE,          
		0,              
		NULL,           
		NULL,           
		&si,            
		&pi)           
		)
	{
		ReportSvcStatus(SERVICE_STOPPED, GetLastError(), 0);
		return;
	}

	ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

	WaitForSingleObject(ghSvcStopEvent, INFINITE);

	// TO_DO: Perform work until service stops.
	//HANDLE ghEvents[2];
	//ghEvents[0] = pi.hProcess;
	//ghEvents[1] = ghSvcStopEvent;
	//DWORD dwEvent = WaitForMultipleObjects(
	//	2,           // number of objects in array
	//	ghEvents,    // array of objects
	//	FALSE,       // wait for any object
	//	INFINITE);       // five-second wait

	//switch (dwEvent)
	//{
	//case WAIT_OBJECT_0 + 0:
	//	printf("The process terminated. We're done.\n");
	//	break;

	//case WAIT_OBJECT_0 + 1:
	//	printf("Received a request to stop the service.\n");
	//	SendConsoleCtrlEvent(pi.dwProcessId, CTRL_C_EVENT);

	//	DWORD dwStatus;
	//	do
	//	{
	//		ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
	//		dwStatus = WaitForSingleObject(pi.hProcess, 1000);
	//	} while (dwStatus == WAIT_TIMEOUT);

	//	break;

	//case WAIT_TIMEOUT:
	//	printf("Wait timed out.\n");
	//	break;

	//default:
	//	printf("Wait error: %d\n", GetLastError());
	//	ExitProcess(0);
	//}
	
	ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);

	if(pi.hProcess)
	CloseHandle(pi.hProcess);
	if(pi.hThread)
	CloseHandle(pi.hThread);
}

//
// Purpose: 
//   Sets the current service status and reports it to the SCM.
//
// Parameters:
//   dwCurrentState - The current state (see SERVICE_STATUS)
//   dwWin32ExitCode - The system error code
//   dwWaitHint - Estimated time for pending operation, 
//     in milliseconds
// 
// Return value:
//   None
//
VOID ReportSvcStatus(DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;
	gSvcStatus.dwCurrentState = dwCurrentState;
	gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	gSvcStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		gSvcStatus.dwControlsAccepted = 0;
	else 
		gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if ((dwCurrentState == SERVICE_RUNNING) ||
		(dwCurrentState == SERVICE_STOPPED))
		gSvcStatus.dwCheckPoint = 0;

	else gSvcStatus.dwCheckPoint = dwCheckPoint++;

	SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}

//
// Purpose: 
//   Called by SCM whenever a control code is sent to the service
//   using the ControlService function.
//
// Parameters:
//   dwCtrl - control code
// 
// Return value:
//   None
//
VOID WINAPI SvcCtrlHandler(DWORD dwCtrl)
{
	// Handle the requested control code. 

	switch (dwCtrl)
	{
	case SERVICE_CONTROL_STOP:
		ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

		// Signal the service to stop.

		SetEvent(ghSvcStopEvent);
		ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);

		return;

	case SERVICE_CONTROL_INTERROGATE:
		break;

	default:
		break;
	}

}

//
// Purpose: 
//   Logs messages to the event log
//
// Parameters:
//   szFunction - name of function that failed
// 
// Return value:
//   None
//
// Remarks:
//   The service must have an entry in the Application event log.
//
VOID SvcReportEvent(LPTSTR szFunction)
{
	HANDLE hEventSource;
	LPCTSTR lpszStrings[2];
	TCHAR Buffer[80];

	TCHAR service_name[_MAX_PATH];
	GetServiceName(service_name, _MAX_PATH);

	hEventSource = RegisterEventSource(NULL, service_name);

	if (NULL != hEventSource)
	{
		StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());

		lpszStrings[0] = service_name;
		lpszStrings[1] = Buffer;

		ReportEvent(hEventSource,        // event log handle
			EVENTLOG_ERROR_TYPE, // event type
			0,                   // event category
			SVC_ERROR,           // event identifier
			NULL,                // no security identifier
			2,                   // size of lpszStrings array
			0,                   // no binary data
			lpszStrings,         // array of strings
			NULL);               // no binary data

		DeregisterEventSource(hEventSource);
	}
}
