// Copyright (c) Werner Strydom. All rights reserved.
// Licensed under the MIT license. See LICENSE in the project root for license information.

#include "stdafx.h"
#include "wrapper-error.h"
#include "wrapper-memory.h"
#include "wrapper-log.h"
#include "service_config.h"
#include "wrapper-string.h"
#include "wrapper-utils.h"

VOID WINAPI wrapper_service_main(DWORD dwArgc, LPTSTR* lpszArgv);
VOID WINAPI wrapper_service_control_handler(DWORD dwCtrl);

int wrapper_service_init(wrapper_config_t* config, wrapper_error_t** error);
int wrapper_service_report_status(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint,
                                  wrapper_config_t* config, wrapper_error_t** error);


SERVICE_STATUS_HANDLE status_handle; // TODO: Move to methods and pass around like variables
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
	UNUSED(dwArgc);
	UNUSED(lpszArgv);

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
				*error = wrapper_error_from_hresult(hr, _T("Failed to copy the command line from the configuration file to a local buffer"));
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

	wrapper_service_report_status(SERVICE_START_PENDING, NO_ERROR, 3000, config, error);

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
	UNUSED(config);

	static DWORD dwCheckPoint = 1;

	SERVICE_STATUS service_status = {0};

	service_status.dwServiceSpecificExitCode = 0;
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
			wrapper_error_t* error = NULL;
			HANDLE stop_event = OpenEvent(EVENT_ALL_ACCESS, TRUE, stop_event_name);
			if (stop_event)
			{
				WRAPPER_INFO(_T("Received stop request from the service manager."));
				if (SetEvent(stop_event))
				{
					WRAPPER_INFO(_T("Succesfully set the event '%s'."), stop_event_name);
				}
				else
				{
					error = wrapper_error_from_system(GetLastError(), _T("Failed to set the event '%s'. The service may not stop."),
					                                  stop_event_name);
				}
				CloseHandle(stop_event);
			}
			else
			{
				error = wrapper_error_from_system(GetLastError(), _T("Failed to set the event '%s'."), stop_event_name);
			}

			if (error)
			{
				wrapper_error_log(error);
				wrapper_error_free(error);
			}
		}
		break;

	case SERVICE_CONTROL_INTERROGATE:
		break;

	default:
		break;
	}
}

int wrapper_log_get_path(TCHAR* destination, const size_t size, wrapper_config_t* config, wrapper_error_t** error)
{
	UNUSED(config);

	HRESULT hr = S_OK;
	if (!GetModuleFileName(NULL, destination, (DWORD)size))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		if (error)
		{
			*error = wrapper_error_from_hresult(hr, _T("Failed to get the path of the current process."));
		}
	}

	if (SUCCEEDED(hr))
	{
		PathCchRenameExtension(destination, size, _T(".log"));
	}

	if (FAILED(hr))
	{
		return 0;
	}

	return 1;
}

int do_run(wrapper_config_t* config, wrapper_error_t** error)
{
	HRESULT hr = S_OK;
	TCHAR* log_path = NULL;

	if (SUCCEEDED(hr))
	{
		log_path = wrapper_allocate_string(MAX_PATH);
		if (!log_path)
		{
			hr = E_OUTOFMEMORY;
		}
	}

	if (SUCCEEDED(hr))
	{
		if (!wrapper_log_get_path(log_path, _MAX_PATH, config, error))
		{
			hr = E_FAIL;
		}
	}

	if (SUCCEEDED(hr))
	{
		wrapper_log_set_handler(wrapper_log_file_handler, log_path);
	}

	if (SUCCEEDED(hr))
	{
		WRAPPER_INFO(_T("Starting Service"));
		
#pragma warning( push )
#pragma warning( disable: 4204 ) // nonstandard extension used: non-constant aggregate initializer
		SERVICE_TABLE_ENTRY DispatchTable[] =
		{
			{
				.lpServiceName = config->name,
				.lpServiceProc = (LPSERVICE_MAIN_FUNCTION)wrapper_service_main
			},
			{
				.lpServiceName = NULL,
				.lpServiceProc = NULL
			}
		};
#pragma warning( pop )

		if (!StartServiceCtrlDispatcher(DispatchTable))
		{
			hr = E_FAIL;
			if (error)
			{
				*error = wrapper_error_from_hresult(hr, _T("Failed to start the service '%s'."), config->name);
			}
		}
		else
		{
			WRAPPER_INFO(_T("Done"));
		}
	}

	if (FAILED(hr))
	{
		return 0;
	}
	return 1;
}

int wrapper_get_current_process_filename(TCHAR* buffer, size_t size, wrapper_config_t* config, wrapper_error_t** error)
{
	UNUSED(config);

	if (!GetModuleFileName(NULL, buffer, (DWORD)size))
	{
		if (error)
		{
			*error = wrapper_error_from_system(GetLastError(), _T("Unable to get the filename of the current process"));
		}
		return 0;
	}
	return 1;
}

int wrapper_service_open_manager(SC_HANDLE* manager, wrapper_error_t** error)
{
	int rc = 1;
	*manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == *manager)
	{
		rc = 0;
		if (error)
		{
			*error = wrapper_error_from_system(GetLastError(), _T("Unable to get the filename of the current process"));
		}
	}
	return rc;
}

int wrapper_service_create(SC_HANDLE* service, const SC_HANDLE manager, wrapper_config_t* config,
                           wrapper_error_t** error)
{
	int rc = 1;
	TCHAR* path = NULL;
	TCHAR* service_name = NULL;
	TCHAR* display_name = NULL;

	if (rc)
	{
		rc = wrapper_string_duplicate(&service_name, config->name, error);
	}

	if (rc)
	{
		if (0 == _tcslen(config->title))
		{
			rc = wrapper_string_duplicate(&display_name, config->name, error);
		}
		else
		{
			rc = wrapper_string_duplicate(&display_name, config->title, error);
		}
	}


	if (rc)
	{
		path = wrapper_allocate_string(_MAX_PATH);
		if (!path)
		{
			rc = 0;
			if (error)
			{
				*error = wrapper_error_from_system(ERROR_OUTOFMEMORY, _T("Unable to get the filename of the current process"));
			}
		}
	}

	if (rc)
	{
		rc = wrapper_get_current_process_filename(path, _MAX_PATH, config, error);
	}

	if (rc)
	{
		void* load_order_group = NULL;
		void* tag_id = NULL;
		void* dependencies = NULL;
		void* username = NULL;
		void* password = NULL;
		const long desired_access = SERVICE_ALL_ACCESS;
		const int service_type = SERVICE_WIN32_OWN_PROCESS;
		const int start_type = SERVICE_DEMAND_START;
		const int error_control = SERVICE_ERROR_NORMAL;

		*service = CreateService(
			manager,
			service_name,
			display_name,
			desired_access,
			service_type,
			start_type,
			error_control,
			path,
			load_order_group,
			tag_id,
			dependencies,
			username,
			password);

		if (*service == NULL)
		{
			if (error)
			{
				*error = wrapper_error_from_system(GetLastError(), _T("Failed to create service '%s' (%s)"), service_name,
				                                   display_name);
			}
			rc = 0;
		}
	}

	wrapper_free(service_name);
	wrapper_free(display_name);
	wrapper_free(path);

	return rc;
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
int do_install(wrapper_config_t* config, wrapper_error_t** error)
{
	int rc = 1;
	SC_HANDLE service = NULL;
	SC_HANDLE manager = NULL;

	if (rc)
	{
		rc = wrapper_service_open_manager(&manager, error);
	}

	if (rc)
	{
		rc = wrapper_service_create(&service, manager, config, error);
	}

	if (rc)
	{
		WRAPPER_INFO(_T("The service '%s' was successfully installed"), config->name);
	}

	if (service)
	{
		CloseServiceHandle(service);
	}

	if (manager)
	{
		CloseServiceHandle(manager);
	}

	return rc;
}

int wrapper_service_open(SC_HANDLE* service, const int desired_access, const SC_HANDLE manager,
                         wrapper_config_t* config, wrapper_error_t** error)
{
	int rc = 1;
	*service = OpenService(manager, config->name, desired_access);
	if (*service == NULL)
	{
		if (error)
		{
			*error = wrapper_error_from_system(GetLastError(), _T("Failed to open service '%s'"), config->name);
		}
		rc = 0;
	}
	return rc;
}

int wrapper_service_get_config(LPQUERY_SERVICE_CONFIG* service_config,
                               const SC_HANDLE service,
                               wrapper_config_t* config,
                               wrapper_error_t** error)
{
	int rc = 1;
	DWORD bytes_needed = 0;
	DWORD buffer_size = 0;
	DWORD last_error = 0;
	*service_config = NULL;

	if (rc)
	{
		if (!QueryServiceConfig(
			service,
			NULL,
			0,
			&bytes_needed))
		{
			last_error = GetLastError();
			if (ERROR_INSUFFICIENT_BUFFER == last_error)
			{
				buffer_size = bytes_needed;
				*service_config = (LPQUERY_SERVICE_CONFIG)LocalAlloc(LMEM_FIXED, buffer_size);
			}
			else
			{
				if (error)
				{
					*error = wrapper_error_from_system(last_error, _T("Failed to query the configuration of service '%s'"),
					                                   config->name);
				}
				rc = 0;
			}
		}
	}

	if (rc)
	{
		if (!QueryServiceConfig(
			service,
			*service_config,
			buffer_size,
			&bytes_needed))
		{
			if (error)
			{
				*error = wrapper_error_from_system(last_error, _T("Failed to query the configuration of service '%s'"),
				                                   config->name);
			}
			rc = 0;
		}
	}

	if (!rc)
	{
		LocalFree(*service_config);
		*service_config = NULL;
	}

	return rc;
}

int wrapper_service_get_description(LPSERVICE_DESCRIPTION* service_description, SC_HANDLE service,
                                    wrapper_config_t* config,
                                    wrapper_error_t** error)
{
	int rc = 1;
	DWORD bytes_needed = 0;
	DWORD buffer_size = 0;
	DWORD last_error = 0;
	*service_description = NULL;

	if (rc)
	{
#pragma warning( push )
#pragma warning( disable: 28020 )
		if (!QueryServiceConfig2(service, SERVICE_CONFIG_DESCRIPTION, NULL, 0, &bytes_needed))
#pragma warning( pop )
		{
			last_error = GetLastError();
			if (ERROR_INSUFFICIENT_BUFFER != last_error)
			{
				if (error)
				{
					*error = wrapper_error_from_system(last_error, _T("Failed to query the description of service '%s'"),
					                                   config->name);
				}
				rc = 0;
			}
		}
	}

	if (rc)
	{
		buffer_size = bytes_needed;
		*service_description = (LPSERVICE_DESCRIPTION)LocalAlloc(LMEM_FIXED, buffer_size);
	}

	if (rc)
	{
		if (!QueryServiceConfig2(service, SERVICE_CONFIG_DESCRIPTION, (LPBYTE)*service_description, buffer_size,
		                         &bytes_needed))
		{
			if (error)
			{
				*error = wrapper_error_from_system(last_error, _T("Failed to query the description of service '%s'"), config->name);
			}
			rc = 0;
		}
	}

	if (!rc)
	{
		LocalFree(*service_description);
		*service_description = NULL;
	}

	return rc;
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
int do_status(wrapper_config_t* config, wrapper_error_t** error)
{
	SC_HANDLE manager = NULL;
	SC_HANDLE service = NULL;
	LPQUERY_SERVICE_CONFIG service_config = NULL;
	LPSERVICE_DESCRIPTION service_description = NULL;

	int rc = 1;

	if (rc)
	{
		rc = wrapper_service_open_manager(&manager, error);
	}

	if (rc)
	{
		rc = wrapper_service_open(&service, SERVICE_QUERY_CONFIG, manager, config, error);
	}

	if (rc)
	{
		rc = wrapper_service_get_config(&service_config, service, config, error);
	}

	if (rc)
	{
		rc = wrapper_service_get_description(&service_description, service, config, error);
	}

	if (rc)
	{
		WRAPPER_INFO(TEXT("Configuration:"));
		WRAPPER_INFO(TEXT("  %-20s: %s"), _T("Name"), config->name);
		WRAPPER_INFO(TEXT("  %-20s: %s"), _T("Description"), service_description->lpDescription ? service_description->
			lpDescription : _T(""));
		WRAPPER_INFO(TEXT("  %-20s: %-2d (0x%08x)"), _T("Service Type"), service_config->dwServiceType, service_config->
			dwServiceType);
		WRAPPER_INFO(TEXT("  %-20s: %-2d (0x%08x)"), _T("Start Type"), service_config->dwStartType, service_config->
			dwStartType);
		WRAPPER_INFO(TEXT("  %-20s: %-2d (0x%08x)"), _T("Error Control"), service_config->dwErrorControl, service_config->
			dwErrorControl);
		WRAPPER_INFO(TEXT("  %-20s: %s"), _T("Path"), service_config->lpBinaryPathName);
		WRAPPER_INFO(TEXT("  %-20s: %s"), _T("Account"), service_config->lpServiceStartName);
		WRAPPER_INFO(TEXT("  %-20s: %s"), _T("Load Order Group"), service_config->lpLoadOrderGroup);
		WRAPPER_INFO(TEXT("  %-20s: %-2d (0x%08x)"), _T("Tag ID"), service_config->dwTagId, service_config->dwTagId);
		WRAPPER_INFO(TEXT("  %-20s: %s"), _T("Dependencies"), service_config->lpDependencies);
	}

	LocalFree(service_config);
	LocalFree(service_description);

	if (service)
		CloseServiceHandle(service);

	if (manager)
		CloseServiceHandle(manager);

	return rc;
}

int wrapper_service_set_start_type(const SC_HANDLE service, int start_type, wrapper_config_t* config,
                                   wrapper_error_t** error)
{
	int rc = 1;
	if (rc)
	{
		const unsigned service_type = SERVICE_NO_CHANGE;
		unsigned error_control = SERVICE_NO_CHANGE;
		void* binary_path_name = NULL;
		void* load_order_group = NULL;
		void* tag_id = NULL;
		void* dependencies = NULL;
		void* username = NULL;
		void* password = NULL;
		void* display_name = NULL;

		if (!ChangeServiceConfig(
			service,
			service_type,
			start_type,
			error_control,
			binary_path_name,
			load_order_group,
			tag_id,
			dependencies,
			username,
			password,
			display_name))
		{
			if (error)
			{
				*error = wrapper_error_from_system(GetLastError(), _T("Failed to disable service '%s'"), config->name);
			}
			rc = 0;
		}
	}
	return rc;
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
int do_disable(wrapper_config_t* config, wrapper_error_t** error)
{
	SC_HANDLE manager = NULL;
	SC_HANDLE service = NULL;

	int rc = 1;
	if (rc)
	{
		rc = wrapper_service_open_manager(&manager, error);
	}

	if (rc)
	{
		rc = wrapper_service_open(&service, SERVICE_CHANGE_CONFIG, manager, config, error);
	}

	if (rc)
	{
		rc = wrapper_service_set_start_type(service, SERVICE_DISABLED, config, error);
	}

	if (rc)
	{
		WRAPPER_INFO(_T("Successfully disabled service '%s'"), config->name);
	}

	if (service)
	{
		CloseServiceHandle(service);
	}

	if (manager)
	{
		CloseServiceHandle(manager);
	}
	return rc;
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
int do_enable(wrapper_config_t* config, wrapper_error_t** error)
{
	SC_HANDLE manager = NULL;
	SC_HANDLE service = NULL;

	int rc = 1;
	if (rc)
	{
		rc = wrapper_service_open_manager(&manager, error);
	}

	if (rc)
	{
		rc = wrapper_service_open(&service, SERVICE_CHANGE_CONFIG, manager, config, error);
	}

	if (rc)
	{
		rc = wrapper_service_set_start_type(service, SERVICE_DEMAND_START, config, error);
	}

	if (rc)
	{
		WRAPPER_INFO(_T("Successfully enabled service '%s'"), config->name);
	}

	if (service)
	{
		CloseServiceHandle(service);
	}

	if (manager)
	{
		CloseServiceHandle(manager);
	}
	return rc;
}

int wrapper_service_set_description(SC_HANDLE service, wrapper_config_t* config, wrapper_error_t** error)
{
	int rc = 1;
	SERVICE_DESCRIPTION service_description = {0};
	service_description.lpDescription = config->description;
	if (!ChangeServiceConfig2(service, SERVICE_CONFIG_DESCRIPTION, &service_description))
	{
		if (error)
		{
			*error = wrapper_error_from_system(GetLastError(), _T("Failed to set the description of service '%s'"),
			                                   config->name);
		}
		rc = 0;
	}
	return rc;
}

int do_update(wrapper_config_t* config, wrapper_error_t** error)
{
	SC_HANDLE manager = NULL;
	SC_HANDLE service = NULL;

	int rc = 1;
	if (rc)
	{
		rc = wrapper_service_open_manager(&manager, error);
	}

	if (rc)
	{
		rc = wrapper_service_open(&service, SERVICE_CHANGE_CONFIG, manager, config, error);
	}

	if (rc)
	{
		rc = wrapper_service_set_description(service, config, error);
	}

	if (rc)
	{
		WRAPPER_INFO(_T("Successfully changed the description of service '%s'"), config->name);
	}

	if (service)
	{
		CloseServiceHandle(service);
	}

	if (manager)
	{
		CloseServiceHandle(manager);
	}
	return rc;
}

int wrapper_service_delete2(SC_HANDLE service, wrapper_config_t* config, wrapper_error_t** error)
{
	int rc = 1;
	if (!DeleteService(service))
	{
		if (error)
		{
			*error = wrapper_error_from_system(GetLastError(), _T("Failed to delete service '%s'"), config->name);
		}
		rc = 0;
	}
	return rc;
}

int do_delete(wrapper_config_t* config, wrapper_error_t** error)
{
	SC_HANDLE manager = NULL;
	SC_HANDLE service = NULL;

	int rc = 1;
	if (rc)
	{
		rc = wrapper_service_open_manager(&manager, error);
	}

	if (rc)
	{
		rc = wrapper_service_open(&service, SERVICE_ALL_ACCESS, manager, config, error);
	}

	if (rc)
	{
		rc = wrapper_service_delete2(service, config, error);
	}

	if (rc)
	{
		WRAPPER_INFO(_T("Successfully deleted service '%s'"), config->name);
	}

	if (service)
	{
		CloseServiceHandle(service);
	}

	if (manager)
	{
		CloseServiceHandle(manager);
	}

	return rc;
}


BOOL wrapper_service_stop_dependent_services(SC_HANDLE schSCManager, SC_HANDLE schService);

int wrapper_service_get_status(SC_HANDLE service,
                               SERVICE_STATUS_PROCESS* status,
                               const wrapper_config_t* config,
                               wrapper_error_t** error)
{
	int rc = 1;
	DWORD bytes_needed;
	if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)status, sizeof(SERVICE_STATUS_PROCESS), &bytes_needed))
	{
		if (error)
		{
			*error = wrapper_error_from_system(GetLastError(), _T("Failed to query the status of the service '%s'"),
			                                   config->name);
		}
		rc = 0;
	}
	else
	{
		WRAPPER_DEBUG(_T("Status of service '%s':"), config->name);
		WRAPPER_DEBUG(TEXT("  %-20s: 0x%08x (%d)"), _T("Check Point"), status->dwCheckPoint, status->dwCheckPoint);
		WRAPPER_DEBUG(TEXT("  %-20s: 0x%08x (%d)"), _T("Controls Accepted"), status->dwControlsAccepted, status->dwControlsAccepted);
		WRAPPER_DEBUG(TEXT("  %-20s: 0x%08x (%d)"), _T("Current State"), status->dwCurrentState, status->dwCurrentState);
		WRAPPER_DEBUG(TEXT("  %-20s: 0x%08x (%d)"), _T("Process Id"), status->dwProcessId, status->dwProcessId);
		WRAPPER_DEBUG(TEXT("  %-20s: 0x%08x (%d)"), _T("Service Flags"), status->dwServiceFlags, status->dwServiceFlags);
		WRAPPER_DEBUG(TEXT("  %-20s: 0x%08x (%d)"), _T("Service Type"), status->dwServiceType, status->dwServiceType);
		WRAPPER_DEBUG(TEXT("  %-20s: 0x%08x (%d)"), _T("Service Exit Code"), status->dwServiceSpecificExitCode, status->dwServiceSpecificExitCode);
		WRAPPER_DEBUG(TEXT("  %-20s: 0x%08x (%d)"), _T("Wait Hint"), status->dwWaitHint, status->dwWaitHint);
		WRAPPER_DEBUG(TEXT("  %-20s: 0x%08x (%d)"), _T("Win32 Exit Code"), status->dwWin32ExitCode, status->dwWin32ExitCode);
	}
	return rc;
}

int wrapper_service_is_not_stopped(DWORD status)
{
	return status != SERVICE_STOPPED && status != SERVICE_STOP_PENDING;
}

DWORD wrapper_service_get_recommended_wait_time(const DWORD wait_hint)
{
	// Do not wait longer than the wait hint. A good interval is 
	// one-tenth the wait hint, but no less than 1 second and no 
	// more than 10 seconds. 
	const DWORD wait_time = wait_hint / 10;
	if (wait_time < 1000) 
	{
		return 1000;
	}

	if (wait_time > 10000) 
	{
		return 10000;
	}
	return wait_time;
}

int wrapper_service_wait_to_stop(SC_HANDLE service, wrapper_config_t* config, wrapper_error_t** error)
{
	int rc = 1;

	SERVICE_STATUS_PROCESS status = {0};
	if (rc)
	{
		rc = wrapper_service_get_status(service, &status, config, error);
	}

	if (rc)
	{
		if (wrapper_service_is_not_stopped(status.dwCurrentState))
		{
			WRAPPER_INFO(_T("The service '%s' is running."), config->name);
			return 1;
		}
	}

	WRAPPER_INFO(_T("Waiting for the service '%s' to stop."), config->name);
	ULONGLONG start_tick_count = GetTickCount64();
	DWORD old_check_point = status.dwCheckPoint;
	while (rc && (status.dwCurrentState == SERVICE_STOP_PENDING))
	{
		const DWORD milliseconds = wrapper_service_get_recommended_wait_time(status.dwWaitHint);
		Sleep(milliseconds);
		if (rc)
		{
			rc = wrapper_service_get_status(service, &status, config, error);
		}

		if (rc)
		{
			if (status.dwCurrentState != SERVICE_STOP_PENDING)
			{
				break;
			}
		}

		if (status.dwCheckPoint > old_check_point)
		{
			start_tick_count = GetTickCount64();
			old_check_point = status.dwCheckPoint;
			continue;
		}
		
		if (GetTickCount64() - start_tick_count > status.dwWaitHint)
		{
			if (error)
			{
				*error = wrapper_error_from_system(ERROR_TIMEOUT, _T("An timeout occurred while waiting for the service '%s' to stop."), config->name);
			}
			rc = 0;
		}
	}
	return 1;
}

int wrapper_service_start(const SC_HANDLE service, const wrapper_config_t* config, wrapper_error_t** error)
{
	if (!StartService(service, 0, NULL)) 
	{
		if (error)
		{
			*error = wrapper_error_from_system(GetLastError(), _T("Failed to start the service '%s'"), config->name);
		}
		return 0;
	}
	return 1;
}

int wrapper_service_wait_to_start(const SC_HANDLE service, const wrapper_config_t* config, wrapper_error_t** error)
{
	int rc = 1;
	SERVICE_STATUS_PROCESS status = {0};

	if (rc)
	{
		rc = wrapper_service_get_status(service, &status, config, error);
	}

	ULONGLONG start_tick_count = GetTickCount64();
	DWORD old_check_point = status.dwCheckPoint;
	WRAPPER_INFO(_T("Waiting for service '%s' to start."), config->name);

	while (rc && (status.dwCurrentState == SERVICE_START_PENDING))
	{
		const DWORD milliseconds = wrapper_service_get_recommended_wait_time(status.dwWaitHint);
		Sleep(milliseconds);

		if (rc)
		{
			rc = wrapper_service_get_status(service, &status, config, error);
		}

		if (rc)
		{
			if(status.dwCurrentState != SERVICE_START_PENDING)
			{
				break;
			}
		}

		if (status.dwCheckPoint > old_check_point)
		{
			// Continue to wait and check.
			start_tick_count = GetTickCount64();
			old_check_point = status.dwCheckPoint;
		}
		else if (GetTickCount64() - start_tick_count > status.dwWaitHint)
		{
			if (error)
			{
				*error = wrapper_error_from_system(ERROR_TIMEOUT, _T("An timeout occurred while waiting for the service '%s' to start."), config->name);
			}
			rc = 0;
			break;
		}
	}
	return rc;
}

//
// Purpose: 
//   Starts the service if possible.
//
// Parameters:
//   None
// 
// Return value:
//   None
//
int do_start(wrapper_config_t* config, wrapper_error_t** error)
{
	SC_HANDLE manager = NULL;
	SC_HANDLE service = NULL;

	int rc = 1;
	if (rc)
	{
		rc = wrapper_service_open_manager(&manager, error);
	}

	if (rc)
	{
		rc = wrapper_service_open(&service, SERVICE_ALL_ACCESS, manager, config, error);
	}

	if (rc)
	{
		rc = wrapper_service_wait_to_stop(service, config, error);
	}

	if (rc)
	{
		rc = wrapper_service_start(service, config, error);
	}

	if (rc)
	{
		rc = wrapper_service_wait_to_start(service, config, error);
	}

	SERVICE_STATUS_PROCESS status = {0};
	if (rc)
	{
		rc = wrapper_service_get_status(service, &status, config, error);
	}

	if (rc)
	{
		if (status.dwCurrentState == SERVICE_RUNNING)
		{
			WRAPPER_INFO(_T("Service '%s' started successfully."), config->name);
		}
	}

	if (service)
	{
		CloseServiceHandle(service);
	}

	if (manager)
	{
		CloseServiceHandle(manager);
	}

	return rc;
}

int wrapper_service_stop(SC_HANDLE service, wrapper_config_t* config, wrapper_error_t** error)
{
	SERVICE_STATUS_PROCESS status = {0};
	if (!ControlService(service, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&status))
	{
		if (error)
		{
			*error = wrapper_error_from_system(GetLastError(), _T("Failed to stop the service '%s'"), config->name);
		}
		return 0;
	}
	return 1;
}

int wrapper_service_wait_to_stop2(SC_HANDLE service, wrapper_config_t* config, wrapper_error_t** error)
{
	int rc = 1;
	SERVICE_STATUS_PROCESS status = { 0 };

	if (rc)
	{
		rc = wrapper_service_get_status(service, &status, config, error);
	}

	ULONGLONG start_tick_count = GetTickCount64();
	DWORD old_check_point = status.dwCheckPoint;
	while (rc && (status.dwCurrentState != SERVICE_STOPPED))
	{
		const DWORD milliseconds = wrapper_service_get_recommended_wait_time(status.dwWaitHint);
		WRAPPER_INFO(_T("Waiting %dms for service '%s' to stop."), milliseconds, config->name);
		Sleep(milliseconds);

		if (rc)
		{
			rc = wrapper_service_get_status(service, &status, config, error);
		}

		if (rc)
		{
			if (status.dwCurrentState == SERVICE_STOPPED)
			{
				break;
			}
		}

		if (status.dwCheckPoint > old_check_point)
		{
			// Continue to wait and check.
			start_tick_count = GetTickCount64();
			old_check_point = status.dwCheckPoint;
		}
		else
		{
			if (GetTickCount64() - start_tick_count > status.dwWaitHint)
			{
				if (error)
				{
					*error = wrapper_error_from_system(ERROR_TIMEOUT, _T("An timeout occurred while waiting for the service '%s' to stop."), config->name);
				}
				rc = 0;
				break;
			}
		}
	}

	return rc;
}

int do_stop(wrapper_config_t* config, wrapper_error_t** error)
{
	SC_HANDLE manager = NULL;
	SC_HANDLE service = NULL;

	int rc = 1;
	if (rc)
	{
		rc = wrapper_service_open_manager(&manager, error);
	}

	if (rc)
	{
		const int desired_access = SERVICE_STOP |
			SERVICE_QUERY_STATUS |
			SERVICE_ENUMERATE_DEPENDENTS;

		rc = wrapper_service_open(&service, desired_access, manager, config, error);
	}

	SERVICE_STATUS_PROCESS status = { 0 };
	if (rc)
	{
		rc = wrapper_service_get_status(service, &status, config, error);
	}

	if (rc)
	{
		if (status.dwCurrentState != SERVICE_STOPPED)
		{
			rc = wrapper_service_wait_to_stop(service, config, error);

			if (rc)
			{
				rc = wrapper_service_stop(service, config, error);
			}

			if (rc)
			{
				rc = wrapper_service_wait_to_stop2(service, config, error);
			}

			WRAPPER_INFO(_T("Service '%s' stopped successfully"), config->name);
		}
		else
		{
			WRAPPER_INFO(_T("Service '%s' is already stopped."), config->name);
		}
	}

	if (service)
	{
		CloseServiceHandle(service);
	}
	if (manager)
	{
		CloseServiceHandle(manager);
	}
	return rc;
}

