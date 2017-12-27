// Copyright (c) Werner Strydom. All rights reserved.
// Licensed under the MIT license. See LICENSE in the project root for license information.

#include "stdafx.h"
#include "messages.h"
#include "service.h"
#include "wrapper.h"
#include "wrapper-error.h"
#include "wrapper-log.h"

SERVICE_STATUS gSvcStatus;
SERVICE_STATUS_HANDLE gSvcStatusHandle;
HANDLE ghSvcStopEvent = NULL;

const TCHAR* wrapper_service_get_status_text(DWORD status)
{
	switch (status)
	{
	case SERVICE_STOPPED:
		return _T("SERVICE_STOPPED");
	case SERVICE_START_PENDING:
		return _T("SERVICE_START_PENDING");
	case SERVICE_STOP_PENDING:
		return _T("SERVICE_STOP_PENDING");
	case SERVICE_RUNNING:
		return _T("SERVICE_RUNNING");
	case SERVICE_CONTINUE_PENDING:
		return _T("SERVICE_CONTINUE_PENDING");
	case SERVICE_PAUSE_PENDING:
		return _T("SERVICE_PAUSE_PENDING");
	case SERVICE_PAUSED:
		return _T("SERVICE_PAUSED");
	default:
		return _T("UNKNOWN");
	}
}

// Inspired from http://stackoverflow.com/a/15281070/1529139
// and http://stackoverflow.com/q/40059902/1529139
BOOL SendConsoleCtrlEvent(DWORD dwProcessId, DWORD dwCtrlEvent)
{
	BOOL success = FALSE;
	DWORD thisConsoleId = GetCurrentProcessId();
	// Leave current console if it exists
	// (otherwise AttachConsole will return ERROR_ACCESS_DENIED)
	BOOL consoleDetached = FreeConsole() != FALSE;

	if (AttachConsole(dwProcessId) != FALSE)
	{
		SetConsoleCtrlHandler(NULL, TRUE);
		success = GenerateConsoleCtrlEvent(dwCtrlEvent, 0) != FALSE;
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
VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
	// Register the handler function for the service
	TCHAR service_name[_MAX_PATH];
	GetServiceName(service_name, _MAX_PATH);

	WRAPPER_INFO(_T("Service Name: %s"), service_name);

	gSvcStatusHandle = RegisterServiceCtrlHandler(
		service_name,
		SvcCtrlHandler);

	if (!gSvcStatusHandle)
	{
		DWORD last_error = GetLastError();
		wrapper_error_t* error = wrapper_error_from_system(
			last_error, _T("Failed to register the control handler for service '%s'"), service_name);
		if (error)
		{
			wrapper_error_log(error);
			wrapper_error_free(error);
		}

		SvcReportEvent(TEXT("RegisterServiceCtrlHandler"));
		return;
	}

	WRAPPER_INFO(_T("Succesfully created a service control handler for service '%s'"), service_name);

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
VOID SvcInit(DWORD dwArgc, LPTSTR* lpszArgv)
{
	SECURITY_ATTRIBUTES saAttr = {0};
	STARTUPINFO si = {0};
	PROCESS_INFORMATION pi = {0};

	TCHAR service_name[_MAX_PATH];
	GetServiceName(service_name, _MAX_PATH);

	ZeroMemory(&si, sizeof si);
	ZeroMemory(&pi, sizeof pi);

	si.cb = sizeof si;
	si.dwFlags |= STARTF_USESTDHANDLES;

	ghSvcStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (ghSvcStopEvent == NULL)
	{
		DWORD last_error = GetLastError();
		wrapper_error_t* error = wrapper_error_from_system(
			last_error, _T("Failed to register the event for service '%s' that would be used to say the process has stopped."),
			service_name);
		if (error)
		{
			wrapper_error_log(error);
			wrapper_error_free(error);
		}

		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}

	WRAPPER_INFO(_T("Starting process '%s'"), _T("sample1.exe"));

	TCHAR szCmdline[4096];
	StringCbCopy(szCmdline, 4096, _T("sample1.exe"));

	if (!CreateProcess(NULL,
		szCmdline,
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
		DWORD last_error = GetLastError();
		wrapper_error_t* error = wrapper_error_from_system(last_error, _T("Failed to start the process '%s'"),
		                                                   _T("sample1.exe"));
		if (error)
		{
			wrapper_error_log(error);
			wrapper_error_free(error);
		}
		ReportSvcStatus(SERVICE_STOPPED, last_error, 0);
		return;
	}

	WRAPPER_INFO(_T("Succesfully started process '%s'"), _T("sample1.exe"));

	ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

	//WRAPPER_INFO(_T("Waiting for signal to stop service"));
	//WaitForSingleObject(ghSvcStopEvent, INFINITE);
	//WRAPPER_INFO(_T("Received a request to stop the windows service"));

	// TO_DO: Perform work until service stops.
	HANDLE ghEvents[2];
	ghEvents[0] = pi.hProcess;
	ghEvents[1] = ghSvcStopEvent;
	DWORD dwEvent = WaitForMultipleObjects(
		2,           // number of objects in array
		ghEvents,    // array of objects
		FALSE,       // wait for any object
		INFINITE);       // five-second wait

	switch (dwEvent)
	{
	case WAIT_OBJECT_0 + 0:
		WRAPPER_INFO(_T("The process has ended. "));
		break;

	case WAIT_OBJECT_0 + 1:
		WRAPPER_INFO(_T("A request was received to stop the Windows Service."));
		WRAPPER_INFO(_T("Sending a CTRL+C signal to the child process."));
		SendConsoleCtrlEvent(pi.dwProcessId, CTRL_C_EVENT);

		DWORD dwStatus;
		do
		{
			ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
			WRAPPER_INFO(_T("Waiting up to 5000ms for the child process to termimate."));
			dwStatus = WaitForSingleObject(pi.hProcess, 1000);
		} while (dwStatus == WAIT_TIMEOUT);
		WRAPPER_INFO(_T("The child process succesfully termimated."));
		break;

	case WAIT_TIMEOUT:
		printf("Wait timed out.\n");
		break;

	default:
		DWORD last_error = GetLastError();
		wrapper_error_t* error = wrapper_error_from_system(last_error, _T("Failed to wait either for the process to terminate or for the stop event to be raised"));
		if (error)
		{
			wrapper_error_log(error);
			wrapper_error_free(error);
		}

		break;
	}

	ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
	WRAPPER_INFO(_T("The windows service was stopped."));

	if (pi.hProcess)
		CloseHandle(pi.hProcess);

	if (pi.hThread)
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

	if (dwCurrentState == SERVICE_RUNNING ||
		dwCurrentState == SERVICE_STOPPED)
		gSvcStatus.dwCheckPoint = 0;
	else
		gSvcStatus.dwCheckPoint = dwCheckPoint++;

	const TCHAR* status_text = wrapper_service_get_status_text(dwCurrentState);
	WRAPPER_INFO(_T("Setting the status of the service to '%s'"), status_text);
	if (SetServiceStatus(gSvcStatusHandle, &gSvcStatus)) 
	{
		WRAPPER_INFO(_T("Set the status of the service to '%s'"), status_text);
	}
	else
	{
		DWORD last_error = GetLastError();
		wrapper_error_t* error = wrapper_error_from_system(
			last_error, _T("Failed to set the status of the service to '%s'"),
			status_text);
		if (error)
		{
			wrapper_error_log(error);
			wrapper_error_free(error);
		}
	}
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
