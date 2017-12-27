// Copyright (c) Werner Strydom. All rights reserved.
// Licensed under the MIT license. See LICENSE in the project root for license information.

#include "stdafx.h"
#include "wrapper-error.h"
#include "wrapper-memory.h"
#include "wrapper-log.h"
#include "service_config.h"

VOID WINAPI wrapper_service_main(DWORD dwArgc, LPTSTR* lpszArgv);
VOID WINAPI wrapper_service_control_handler(DWORD dwCtrl);

int wrapper_service_init(wrapper_config_t* config, wrapper_error_t** error);
int wrapper_service_report_status(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint,
                                  wrapper_config_t* config, wrapper_error_t** error);

SERVICE_STATUS service_status;
SERVICE_STATUS_HANDLE status_handle;
TCHAR* stop_event_name = _T("PHAKA_WINDOWS_SERVICE_STOP_EVENT");

const TCHAR* wrapper_service_get_status_text(const unsigned long status)
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
VOID WINAPI wrapper_service_main(DWORD dwArgc, LPTSTR* lpszArgv)
{
	HRESULT hr = S_OK;
	wrapper_error_t* error = NULL;
	wrapper_config_t* config = NULL;
	TCHAR* configuration_path = wrapper_allocate_string(_MAX_PATH);
	if (NULL == configuration_path)
	{
		hr = E_OUTOFMEMORY;
	}

	if (SUCCEEDED(hr))
	{
		if (!wrapper_config_get_path(configuration_path, _MAX_PATH, &error))
		{
			wrapper_error_log(error);
			hr = E_FAIL;
		}
	}

	if (SUCCEEDED(hr))
	{
		config = wrapper_config_alloc();
		if (NULL == config)
		{
			hr = E_OUTOFMEMORY;
		}
	}

	TCHAR* service_name = NULL;
	if (SUCCEEDED(hr))
	{
		WRAPPER_INFO(_T("Reading configuration '%s'"), configuration_path);
		if (!wrapper_config_read(configuration_path, config, &error))
		{
			wrapper_error_log(error);
			hr = E_FAIL;
		}
		else
		{
			WRAPPER_INFO(_T("Configuration Settings:"));
			WRAPPER_INFO(_T("  %-20s: %s"), _T("Name"), config->name);
			WRAPPER_INFO(_T("  %-20s: %s"), _T("Title"), config->title);
			WRAPPER_INFO(_T("  %-20s: %s"), _T("Description"), config->description);
			WRAPPER_INFO(_T("  %-20s: %s"), _T("Working Directory"), config->working_directory);
			WRAPPER_INFO(_T("  %-20s: %s"), _T("Command Line"), config->command_line);
			WRAPPER_INFO(_T(""));
			service_name = config->name;
		}
	}

	if (SUCCEEDED(hr))
	{
		WRAPPER_INFO(_T("Register a service control handler for service '%s'"), service_name);
		status_handle = RegisterServiceCtrlHandler(service_name, wrapper_service_control_handler);
		if (!status_handle)
		{
			DWORD last_error = GetLastError();
			error = wrapper_error_from_system(last_error, _T("Failed to register the control handler for service '%s'"),
			                                  service_name);
			wrapper_error_log(error);
			hr = HRESULT_FROM_WIN32(last_error);
		}
		else
		{
			WRAPPER_INFO(_T("Succesfully created a service control handler for service '%s'"), service_name);
		}
	}

	if (SUCCEEDED(hr))
	{
		wrapper_service_report_status(SERVICE_START_PENDING, NO_ERROR, 3000, config, &error);
		wrapper_service_init(config, &error);
	}
	else
	{
		wrapper_error_log(error);
		wrapper_service_report_status(SERVICE_STOPPED, NO_ERROR, 0, config, &error);
	}

	wrapper_free(configuration_path);
	wrapper_error_free(error);
	wrapper_config_free(config);
}

HANDLE wrapper_create_child_process(wrapper_config_t* config, wrapper_error_t** error)
{
	HRESULT hr = S_OK;
	STARTUPINFO* startupinfo = NULL;
	PROCESS_INFORMATION* process_information = NULL;

	if (SUCCEEDED(hr))
	{
		process_information = wrapper_allocate(sizeof*process_information);
		if (!process_information)
		{
			hr = E_OUTOFMEMORY;
			if (error)
			{
				*error = wrapper_error_from_hresult(hr, _T("Failed to allocate memory for the process information"));
			}
		}
	}

	if (SUCCEEDED(hr))
	{
		startupinfo = wrapper_allocate(sizeof*startupinfo);
		if (!startupinfo)
		{
			hr = E_OUTOFMEMORY;
			if (error)
			{
				*error = wrapper_error_from_hresult(hr, _T("Failed to allocate memory for the process startup information"));
			}
		}
	}

	TCHAR* command_line = NULL;
	const int command_line_max_size = 32768;
	if (SUCCEEDED(hr))
	{
		command_line = wrapper_allocate_string(command_line_max_size);
		if (!command_line)
		{
			if (error)
			{
				*error = wrapper_error_from_hresult(E_OUTOFMEMORY, _T("Failed to allocate a buffer for the command line"));
			}
			hr = E_OUTOFMEMORY;
		}
	}

	if (SUCCEEDED(hr))
	{
		hr = StringCbCopy(command_line, command_line_max_size, config->command_line);
		if (FAILED(hr))
		{
			if (error)
			{
				*error = wrapper_error_from_hresult(
					hr, _T("Failed to copy the command line from the configuration file to a local buffer"));
			}
		}
	}

	if (SUCCEEDED(hr))
	{
		startupinfo->cb = sizeof startupinfo;
		startupinfo->dwFlags |= STARTF_USESTDHANDLES;

		WRAPPER_INFO(_T("Starting process with command line '%s'"), command_line);

		if (!CreateProcess(NULL,
		                   command_line,
		                   NULL,
		                   NULL,
		                   FALSE,
		                   0,
		                   NULL,
		                   NULL,
		                   startupinfo,
		                   process_information)
		)
		{
			DWORD last_error = GetLastError();
			if (error)
			{
				*error = wrapper_error_from_system(last_error, _T("Failed to start the process with command line '%s'"),
				                                   command_line);
			}
			hr = HRESULT_FROM_WIN32(last_error);
		}
	}

	HANDLE process = NULL;

	wrapper_free(command_line);
	wrapper_free(startupinfo);

	if (process_information)
	{
		process = process_information->hProcess;

		if (process_information->hThread)
		{
			CloseHandle(process_information->hThread);
		}
		wrapper_free(process_information);
	}

	return process;
}

int wrapper_wait(HANDLE process, wrapper_config_t* config, wrapper_error_t** error)
{
	DWORD last_error;
	HRESULT hr = S_OK;
	HANDLE events[2];
	HANDLE stop_event = NULL;

	if (SUCCEEDED(hr))
	{
		stop_event = OpenEvent(EVENT_ALL_ACCESS, TRUE, stop_event_name);
		if (!stop_event)
		{
			last_error = GetLastError();
			if (error)
			{
				*error = wrapper_error_from_system(
					last_error, _T("Failed to wait either for the process to terminate or for the stop event to be raised"));
			}
			hr = HRESULT_FROM_WIN32(last_error);
		}
	}

	if (SUCCEEDED(hr))
	{
		events[0] = process;
		events[1] = stop_event;

		const unsigned count = sizeof events / sizeof events[0];
		const int wait_all = FALSE;

		DWORD event = WaitForMultipleObjects(count, events, wait_all, INFINITE);
		switch (event)
		{
		case WAIT_OBJECT_0 + 0:
			// TODO: Display more information about the state of the process, like whether it was killed, crashed or terminated gracefully 
			WRAPPER_INFO(_T("The child process has ended."));
			wrapper_service_report_status(SERVICE_STOP_PENDING, NO_ERROR, 0, config, error);
			break;

		case WAIT_OBJECT_0 + 1:
			WRAPPER_INFO(_T("A request was received to stop the service. Sending a CTRL+C signal to the child process."));
			wrapper_service_report_status(SERVICE_STOP_PENDING, NO_ERROR, 0, config, error);
			DWORD pid = GetProcessId(process);
			SendConsoleCtrlEvent(pid, CTRL_C_EVENT);

			// TODO: Should we forcefully terminate the process if it doesn't respond in say 10 minutes?
			const int timeout = 5000;
			DWORD status;
			do
			{
				wrapper_service_report_status(SERVICE_STOP_PENDING, NO_ERROR, timeout, config, error);
				WRAPPER_INFO(_T("Waiting up to %dms for the child process to termimate."), timeout);
				status = WaitForSingleObject(process, timeout);
			}
			while (status == WAIT_TIMEOUT);
			WRAPPER_INFO(_T("The child process succesfully termimated."));
			break;

		case WAIT_TIMEOUT:
			WRAPPER_WARNING(_T("Wait timed out while waiting for the process to terminate or for the stop event.\n"));
			wrapper_service_report_status(SERVICE_STOP_PENDING, NO_ERROR, 0, config, error);
			hr = HRESULT_FROM_WIN32(ERROR_TIMEOUT);
			if (error)
			{
				*error = wrapper_error_from_hresult(
					hr, _T("Failed to wait either for the process to terminate or for the stop event to be raised"));
			}
			break;

		default:
			wrapper_service_report_status(SERVICE_STOP_PENDING, NO_ERROR, 0, config, error);
			last_error = GetLastError();
			if (error)
			{
				*error = wrapper_error_from_system(
					last_error, _T("Failed to wait either for the process to terminate or for the stop event to be raised"));
			}
			hr = HRESULT_FROM_WIN32(last_error);
			break;
		}
	}

	if (stop_event)
	{
		CloseHandle(stop_event);
	}

	if (FAILED(hr))
	{
		return 0;
	}

	return 1;
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
int wrapper_service_init(wrapper_config_t* config, wrapper_error_t** error)
{
	HRESULT hr = S_OK;
	DWORD last_error;
	HANDLE process = NULL;

	if (SUCCEEDED(hr))
	{
		HANDLE stop_event = CreateEvent(NULL, TRUE, FALSE, stop_event_name);
		if (stop_event == NULL)
		{
			last_error = GetLastError();
			if (error)
			{
				*error = wrapper_error_from_system(
					last_error, _T("Failed to register the event for service '%s' that would be used to say the process has stopped."),
					config->name);
			}
			hr = HRESULT_FROM_WIN32(last_error);
		}
	}


	if (SUCCEEDED(hr))
	{
		process = wrapper_create_child_process(config, error);
		if (process)
		{
			DWORD pid = GetProcessId(process);
			// TODO: Display more information that could help the user diagnose when there is a failure to execute the process 
			WRAPPER_INFO(_T("Successfully started process with command line '%s'"), config->command_line);
			WRAPPER_INFO(_T("  Process ID: %d (0x%08x)"), pid, pid);

			wrapper_service_report_status(SERVICE_RUNNING, NO_ERROR, 0, config, error);
		}
		else
		{
			if (error)
			{
				wrapper_error_log(*error);
			}
			hr = E_FAIL;
		}
	}

	if (SUCCEEDED(hr))
	{
		if (!wrapper_wait(process, config, error))
		{
			if (error)
			{
				wrapper_error_log(*error);
			}
			hr = E_FAIL;
		}
	}

	if (error && *error)
	{
		const long code = (*error)->code;
		WRAPPER_INFO(_T("The windows service stopped with errors."));
		wrapper_service_report_status(SERVICE_STOPPED, code, 0, config, error);
	}
	else
	{
		WRAPPER_INFO(_T("The windows service stopped succesfully."));
		wrapper_service_report_status(SERVICE_STOPPED, NO_ERROR, 0, config, error);
	}

	if (process)
	{
		CloseHandle(process);
	}
	return 1;
}

//
// Purpose: 
//   Sets the current service status and reports it to the SCM.
//
// Parameters:
//   state - The current state (see SERVICE_STATUS)
//   exit_code - The system error code
//   timeout - Estimated time for pending operation, 
//     in milliseconds
// 
// Return value:
//   None
//
int wrapper_service_report_status(DWORD state,
                                  DWORD exit_code,
                                  DWORD timeout,
                                  wrapper_config_t* config,
                                  wrapper_error_t** error)
{
	static DWORD dwCheckPoint = 1;
	service_status.dwCurrentState = state;
	service_status.dwWin32ExitCode = exit_code;
	service_status.dwWaitHint = timeout;
	service_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

	if (state == SERVICE_START_PENDING)
		service_status.dwControlsAccepted = 0;
	else
		service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if (state == SERVICE_RUNNING ||
		state == SERVICE_STOPPED)
		service_status.dwCheckPoint = 0;
	else
		service_status.dwCheckPoint = dwCheckPoint++;

	const TCHAR* status_text = wrapper_service_get_status_text(state);
	WRAPPER_INFO(_T("Setting the status of the service to '%s'"), status_text);
	if (SetServiceStatus(status_handle, &service_status))
	{
		WRAPPER_INFO(_T("Set the status of the service to '%s'"), status_text);
	}
	else
	{
		DWORD last_error = GetLastError();
		if (error)
		{
			*error = wrapper_error_from_system(
				last_error, _T("Failed to set the status of the service to '%s'"),
				status_text);
		}

		return 0;
	}
	return 1;
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
VOID WINAPI wrapper_service_control_handler(DWORD dwCtrl)
{
	switch (dwCtrl)
	{
	case SERVICE_CONTROL_STOP:
		{
			HANDLE stop_event = OpenEvent(EVENT_ALL_ACCESS, TRUE, stop_event_name);
			if (stop_event)
			{
				WRAPPER_INFO("Received stop request from the service manager. Setting event so that processes can stop");
				SetEvent(stop_event);
				CloseHandle(stop_event);
			}
			else
			{
				wrapper_error_t* error = wrapper_error_from_system(GetLastError(), _T("Failed to open event named '%s'"),
				                                                   stop_event_name);
				if (error)
				{
					wrapper_error_log(error);
					wrapper_error_free(error);
				}
			}
		}
		break;

	case SERVICE_CONTROL_INTERROGATE:
		break;

	default:
		break;
	}
}

int wrapper_service_run(wrapper_config_t* config, wrapper_error_t** error)
{
	TCHAR module_path[_MAX_PATH];
	GetModuleFileName(NULL, module_path, _MAX_PATH);
	PathCchRenameExtension(module_path, sizeof module_path / sizeof module_path[0], _T(".log"));
	wrapper_log_set_handler(wrapper_log_file_handler, module_path);

	WRAPPER_INFO(_T("Starting Service"));
	SERVICE_TABLE_ENTRY DispatchTable[] =
	{
		{config->name, (LPSERVICE_MAIN_FUNCTION)wrapper_service_main},
		{NULL, NULL}
	};

	if (!StartServiceCtrlDispatcher(DispatchTable))
	{
		WRAPPER_ERROR(_T("Failed to start service"));
	}
	WRAPPER_INFO(_T("Done"));

	return 1;
}

//
// Purpose: 
//   Installs a service in the SCM database
//
// Parameters:
//   None
// 
// Return value:
//   None
//
int wrapper_service_install(wrapper_config_t* config, wrapper_error_t** error)
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	TCHAR szPath[MAX_PATH];

	if (!GetModuleFileName(NULL, szPath, MAX_PATH))
	{
		printf("Cannot install service (%d)\n", GetLastError());
		return;
	}

	// Get a handle to the SCM database. 

	schSCManager = OpenSCManager(
		NULL, // local computer
		NULL, // ServicesActive database 
		SC_MANAGER_ALL_ACCESS); // full access rights 

	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	// Create the service

	schService = CreateService(
		schSCManager, // SCM database 
		config->name, // name of service 
		config->title, // service name to display 
		SERVICE_ALL_ACCESS, // desired access 
		SERVICE_WIN32_OWN_PROCESS, // service type 
		SERVICE_DEMAND_START, // start type 
		SERVICE_ERROR_NORMAL, // error control type 
		szPath, // path to service's binary 
		NULL, // no load ordering group 
		NULL, // no tag identifier 
		NULL, // no dependencies 
		NULL, // LocalSystem account 
		NULL); // no password 

	if (schService == NULL)
	{
		printf("CreateService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}
	printf("Service installed successfully\n");

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);

	return 1;
}

//
// Purpose: 
//   Retrieves and displays the current service configuration.
//
// Parameters:
//   None
// 
// Return value:
//   None
//
int wrapper_service_query(wrapper_config_t* config, wrapper_error_t** error)
{
	SC_HANDLE schSCManager = NULL;
	SC_HANDLE schService = NULL;
	LPQUERY_SERVICE_CONFIG lpsc = NULL;
	LPSERVICE_DESCRIPTION lpsd = NULL;
	DWORD dwBytesNeeded = 0, cbBufSize = 0, dwError = 0;

	// Get a handle to the SCM database. 

	schSCManager = OpenSCManager(
		NULL, // local computer
		NULL, // ServicesActive database 
		SC_MANAGER_ALL_ACCESS); // full access rights 

	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	// Get a handle to the service.

	schService = OpenService(
		schSCManager, // SCM database 
		config->name, // name of service 
		SERVICE_QUERY_CONFIG); // need query config access 

	if (schService == NULL)
	{
		printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}

	// Get the configuration information.

	if (!QueryServiceConfig(
		schService,
		NULL,
		0,
		&dwBytesNeeded))
	{
		dwError = GetLastError();
		if (ERROR_INSUFFICIENT_BUFFER == dwError)
		{
			cbBufSize = dwBytesNeeded;
			lpsc = (LPQUERY_SERVICE_CONFIG)LocalAlloc(LMEM_FIXED, cbBufSize);
		}
		else
		{
			printf("QueryServiceConfig failed (%d)", dwError);
			goto cleanup;
		}
	}

	if (!QueryServiceConfig(
		schService,
		lpsc,
		cbBufSize,
		&dwBytesNeeded))
	{
		printf("QueryServiceConfig failed (%d)", GetLastError());
		goto cleanup;
	}

	if (!QueryServiceConfig2(
		schService,
		SERVICE_CONFIG_DESCRIPTION,
		NULL,
		0,
		&dwBytesNeeded))
	{
		dwError = GetLastError();
		if (ERROR_INSUFFICIENT_BUFFER == dwError)
		{
			cbBufSize = dwBytesNeeded;
			lpsd = (LPSERVICE_DESCRIPTION)LocalAlloc(LMEM_FIXED, cbBufSize);
		}
		else
		{
			printf("QueryServiceConfig2 failed (%d)", dwError);
			goto cleanup;
		}
	}

	if (!QueryServiceConfig2(
		schService,
		SERVICE_CONFIG_DESCRIPTION,
		(LPBYTE)lpsd,
		cbBufSize,
		&dwBytesNeeded))
	{
		printf("QueryServiceConfig2 failed (%d)", GetLastError());
		goto cleanup;
	}

	// Print the configuration information.

	_tprintf(TEXT("%s configuration: \n"), config->name);
	_tprintf(TEXT("  Type: 0x%x\n"), lpsc->dwServiceType);
	_tprintf(TEXT("  Start Type: 0x%x\n"), lpsc->dwStartType);
	_tprintf(TEXT("  Error Control: 0x%x\n"), lpsc->dwErrorControl);
	_tprintf(TEXT("  Binary path: %s\n"), lpsc->lpBinaryPathName);
	_tprintf(TEXT("  Account: %s\n"), lpsc->lpServiceStartName);

	if (lpsd->lpDescription != NULL && lstrcmp(lpsd->lpDescription, TEXT("")) != 0)
		_tprintf(TEXT("  Description: %s\n"), lpsd->lpDescription);
	if (lpsc->lpLoadOrderGroup != NULL && lstrcmp(lpsc->lpLoadOrderGroup, TEXT("")) != 0)
		_tprintf(TEXT("  Load order group: %s\n"), lpsc->lpLoadOrderGroup);
	if (lpsc->dwTagId != 0)
		_tprintf(TEXT("  Tag ID: %d\n"), lpsc->dwTagId);
	if (lpsc->lpDependencies != NULL && lstrcmp(lpsc->lpDependencies, TEXT("")) != 0)
		_tprintf(TEXT("  Dependencies: %s\n"), lpsc->lpDependencies);

	LocalFree(lpsc);
	LocalFree(lpsd);

cleanup:
	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);

	return 1;
}

//
// Purpose: 
//   Disables the service.
//
// Parameters:
//   None
// 
// Return value:
//   None
//
int wrapper_service_disable(wrapper_config_t* config, wrapper_error_t** error)
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;

	// Get a handle to the SCM database. 

	schSCManager = OpenSCManager(
		NULL, // local computer
		NULL, // ServicesActive database 
		SC_MANAGER_ALL_ACCESS); // full access rights 

	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	// Get a handle to the service.

	schService = OpenService(
		schSCManager, // SCM database 
		config->name, // name of service 
		SERVICE_CHANGE_CONFIG); // need change config access 

	if (schService == NULL)
	{
		printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}

	// Change the service start type.

	if (!ChangeServiceConfig(
		schService, // handle of service 
		SERVICE_NO_CHANGE, // service type: no change 
		SERVICE_DISABLED, // service start type 
		SERVICE_NO_CHANGE, // error control: no change 
		NULL, // binary path: no change 
		NULL, // load order group: no change 
		NULL, // tag ID: no change 
		NULL, // dependencies: no change 
		NULL, // account name: no change 
		NULL, // password: no change 
		NULL)) // display name: no change
	{
		printf("ChangeServiceConfig failed (%d)\n", GetLastError());
	}
	else
		printf("Service disabled successfully.\n");

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);

	return 1;
}

//
// Purpose: 
//   Enables the service.
//
// Parameters:
//   None
// 
// Return value:
//   None
//
int wrapper_service_enable(wrapper_config_t* config, wrapper_error_t** error)
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;

	// Get a handle to the SCM database. 

	schSCManager = OpenSCManager(
		NULL, // local computer
		NULL, // ServicesActive database 
		SC_MANAGER_ALL_ACCESS); // full access rights 

	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	// Get a handle to the service.

	schService = OpenService(
		schSCManager, // SCM database 
		config->name, // name of service 
		SERVICE_CHANGE_CONFIG); // need change config access 

	if (schService == NULL)
	{
		printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}

	// Change the service start type.

	if (!ChangeServiceConfig(
		schService, // handle of service 
		SERVICE_NO_CHANGE, // service type: no change 
		SERVICE_DEMAND_START, // service start type 
		SERVICE_NO_CHANGE, // error control: no change 
		NULL, // binary path: no change 
		NULL, // load order group: no change 
		NULL, // tag ID: no change 
		NULL, // dependencies: no change 
		NULL, // account name: no change 
		NULL, // password: no change 
		NULL)) // display name: no change
	{
		printf("ChangeServiceConfig failed (%d)\n", GetLastError());
	}
	else
		printf("Service enabled successfully.\n");

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);

	return 1;
}

//
// Purpose: 
//   Updates the service description to "This is a test description".
//
// Parameters:
//   None
// 
// Return value:
//   None
//
int wrapper_service_update(wrapper_config_t* config, wrapper_error_t** error)
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	SERVICE_DESCRIPTION sd;
	LPTSTR szDesc = config->description;

	// Get a handle to the SCM database. 

	schSCManager = OpenSCManager(
		NULL, // local computer
		NULL, // ServicesActive database 
		SC_MANAGER_ALL_ACCESS); // full access rights 

	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	// Get a handle to the service.

	schService = OpenService(
		schSCManager, // SCM database 
		config->name, // name of service 
		SERVICE_CHANGE_CONFIG); // need change config access 

	if (schService == NULL)
	{
		printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}

	// Change the service description.

	sd.lpDescription = szDesc;

	if (!ChangeServiceConfig2(
		schService, // handle to service
		SERVICE_CONFIG_DESCRIPTION, // change: description
		&sd)) // new description
	{
		printf("ChangeServiceConfig2 failed\n");
	}
	else
		printf("Service description updated successfully.\n");

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);

	return 1;
}

//
// Purpose: 
//   Deletes a service from the SCM database
//
// Parameters:
//   None
// 
// Return value:
//   None
//
int wrapper_service_delete(wrapper_config_t* config, wrapper_error_t** error)
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	SERVICE_STATUS ssStatus;

	// Get a handle to the SCM database. 

	schSCManager = OpenSCManager(
		NULL, // local computer
		NULL, // ServicesActive database 
		SC_MANAGER_ALL_ACCESS); // full access rights 

	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	// Get a handle to the service.

	schService = OpenService(
		schSCManager, // SCM database 
		config->name, // name of service 
		DELETE); // need delete access 

	if (schService == NULL)
	{
		printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}

	// Delete the service.

	if (!DeleteService(schService))
	{
		printf("DeleteService failed (%d)\n", GetLastError());
	}
	else
		printf("Service deleted successfully\n");

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);

	return 1;
}
