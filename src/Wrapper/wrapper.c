// Copyright (c) Werner Strydom. All rights reserved.
// Licensed under the MIT license. See LICENSE in the project root for license information.

#include "stdafx.h"
#include "wrapper.h"

SERVICE_STATUS gSvcStatus;
SERVICE_STATUS_HANDLE gSvcStatusHandle;
HANDLE ghSvcStopEvent = NULL;
SERVICE_STATUS_HANDLE gSvcStatusHandle = NULL;

typedef struct phaka_config_t {
	LPTSTR log;
	LPTSTR name;
	LPTSTR title;
	LPTSTR description;
	LPTSTR cmdline;
} phaka_config_t;

void* phaka_alloc(size_t size)
{
	return LocalAlloc(LPTR, size);
}

void phaka_free(void* memory)
{
	LocalFree(memory);
}

void phaka_config_free(phaka_config_t* config)
{
	if (config)
	{
		phaka_free(config->name);
		phaka_free(config->description);
		phaka_free(config->cmdline);
		phaka_free(config->log);
		phaka_free(config->title);
		phaka_free(config);
	}
}

phaka_config_t* phaka_config_alloc(void)
{
	phaka_config_t* config = phaka_alloc(sizeof(phaka_config_t));
	if (config)
	{
		config->name = phaka_alloc(sizeof(TCHAR)*MAX_PATH);
		config->description = phaka_alloc(sizeof(TCHAR)*MAX_PATH);
		config->cmdline = phaka_alloc(sizeof(TCHAR) * 4096);
		config->log = phaka_alloc(sizeof(TCHAR)*MAX_PATH);
		config->title = phaka_alloc(sizeof(TCHAR)*MAX_PATH);

		if (!config->name || !config->description || !config->cmdline || !config->log || !config->title)
		{
			phaka_config_free(config);
			return NULL;
		}
	}
	return config;
}

phaka_error_t* phaka_error_alloc(void)
{
	return phaka_alloc(sizeof(phaka_error_t));
}

void phaka_error_free(phaka_error_t* error)
{
	if (error)
	{
		phaka_free(error);
	}
}

phaka_error_t* phaka_error_from_win32(const unsigned long code, LPCTSTR message)
{
	phaka_error_t* error = phaka_error_alloc();
	if (error)
	{
		error->code = HRESULT_FROM_WIN32(code);
	}
	return error;
}

phaka_error_t* phaka_error_from_hresult(const long code, LPCTSTR message)
{
	phaka_error_t* error = phaka_error_alloc();
	if (error)
	{
		error->code = code;
	}
	return error;
}

int phaka_config_get_path(LPTSTR destination, size_t size, phaka_error_t** error)
{
	HRESULT hr = S_OK;
	LPTSTR module_path = phaka_alloc(sizeof(TCHAR)*_MAX_PATH);
	
	if (!GetModuleFileName(NULL, module_path, _MAX_PATH))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
	}

	if (SUCCEEDED(hr))
	{
		hr = StringCchCopy(destination, _MAX_PATH, module_path);
	}

	if (SUCCEEDED(hr))
	{
		hr = PathCchRenameExtension(destination, _MAX_PATH, _T(".cfg"));
	}

	if (error)
	{
		*error = phaka_error_from_hresult(hr, _T("Failed to determine the path of the configuration file."));
	}

	phaka_free(module_path);
	return hr;
}

int phaka_config_read(LPCTSTR path, phaka_config_t* config, phaka_error_t** error)
{
	if (!config)
	{
		if (error)
		{
			*error = phaka_error_from_hresult(E_INVALIDARG, _T("The config argument is null"));
		}
		return 0;
	}

	if(!GetPrivateProfileString(_T("General"), _T("CommandLine"), _T(""), config->cmdline, 4096, path))
	{
		if (error)
		{
			*error = phaka_error_from_hresult(E_INVALIDARG, _T("Failed to read the command line from the configuration file. Verify that there is a 'CommandLine' variable in the 'General' section and that it has a value."));
		}
		return 0;
	}

	if (lstrlen(config->cmdline))
	{
		if (error)
		{
			*error = phaka_error_from_hresult(E_INVALIDARG, _T("The command line in the configuration file is empty."));
		}
		return 0;
	}

	if (!GetPrivateProfileString(_T("General"), _T("Name"), _T(""), config->name, _MAX_PATH, path))
	{
		if (error)
		{
			*error = phaka_error_from_hresult(E_INVALIDARG, _T("Failed to read the name from the configuration file. Verify that there is a 'Name' variable in the 'General' section and that it has a value."));
		}
		return 0;
	}

	if (lstrlen(config->name))
	{
		if (error)
		{
			*error = phaka_error_from_hresult(E_INVALIDARG, _T("The name of the application in the the configuration file is empty."));
		}
		return 0;
	}

	if (!GetPrivateProfileString(_T("General"), _T("Description"), _T(""), config->description, _MAX_PATH, path))
	{
	}

	if (!GetPrivateProfileString(_T("General"), _T("Title"), _T(""), config->title, _MAX_PATH, path))
	{
		// Copy name to title
	}

}



int phaka_service_execute(phaka_error_t** error)
{
	SERVICE_TABLE_ENTRY DispatchTable[] =
	{
		{SVCNAME, (LPSERVICE_MAIN_FUNCTION)SvcMain},
		{NULL, NULL}
	};

	if (!StartServiceCtrlDispatcher(DispatchTable))
	{
		SvcReportEvent(TEXT("StartServiceCtrlDispatcher"));
	}

	return 0;
}

int phaka_service_install(phaka_error_t** error)
{
	_ftprintf(stdout, _T("%s\n"), _T("installing"));
	return 0;
}

int phaka_service_delete(phaka_error_t** error)
{
	_ftprintf(stdout, _T("%s\n"), _T("deleting"));
	return 0;
}

int phaka_run(phaka_error_t** error)
{
	_ftprintf(stdout, _T("%s\n"), _T("running"));
	return 0;
}

int phaka_help(phaka_error_t** error);

typedef int(*phaka_command_func_t)(phaka_error_t** error);

typedef struct phaka_command_t
{
	TCHAR* name;
	TCHAR* description;
	phaka_command_func_t func;
} phaka_command_t;

phaka_command_t commands[] = {
	{
		.name = _T("help"),
		.description = _T("Display this usage message."),
		.func = phaka_help
	},
	{
		.name = _T("install"),
		.description = _T("Installs the Windows Service."),
		.func = phaka_service_install
	},
	{
		.name = _T("delete"),
		.description = _T("Deletes the Windows Service."),
		.func = phaka_service_delete
	},
	{
		.name = _T("run"),
		.description = _T("Runs the application."),
		.func = phaka_run
	},
	{
		.name = _T("version"),
		.description = _T("Display version information only."),
		.func = phaka_run
	},
};

int phaka_help(phaka_error_t** error)
{
	TCHAR path[_MAX_PATH];
	if(GetModuleFileName(NULL, path, _MAX_PATH))
	{
		PathRemoveExtension(path);
		PathStripPath(path);
		_tcslwr_s(path, _MAX_PATH);
	}

	_ftprintf(stdout, _T("Usage: %s [options] [command]\n"), path);
	_ftprintf(stdout, _T("\n"));
	_ftprintf(stdout, _T("Commands:\n"));
	for (int i = 0; i < sizeof(commands)/sizeof(commands[0]); ++i)
	{
		_ftprintf(stdout, _T("  %-15s%s\n"), commands[i].name, commands[i].description);
	}
	_ftprintf(stdout, _T("\n"));
	_ftprintf(stdout, _T("Options:\n"));
	_ftprintf(stdout, _T("  %-15s%s\n"), _T("-?|--help"), _T("Display this usage message."));
	_ftprintf(stdout, _T("  %-15s%s\n"), _T("--nologo"), _T("Do not display the startup banner and copyright message."));
	_ftprintf(stdout, _T("  %-15s%s\n"), _T("-v|--verbose"), _T("Display detailed information."));
	_ftprintf(stdout, _T("\n"));

	return 0;
}

void phaka_show_logo()
{
	_ftprintf(stdout, _T("Phaka Windows Service Wrapper\n"));
	_ftprintf(stdout, _T("Copyright (c) Werner Strydom. All rights reserved.\n"));
	_ftprintf(stdout, _T("Licensed under the MIT license. See the LICENSE for license information.\n\n"));
}

int _tmain(int argc, TCHAR* argv[])
{
	if (argc == 1)
	{
		return phaka_service_execute(NULL);
	}

	int nologo = 0;
	int verbosity = 1;
	phaka_command_t *command = NULL;
	for (int i = 1; i < argc; i++)
	{
		if (lstrcmpi(_T("-v"), argv[i]) == 0 || lstrcmpi(_T("--verbosity"), argv[i]) == 0)
		{
			verbosity = 2;
		}
		else if (lstrcmpi(_T("-?"), argv[i]) == 0 || lstrcmpi(_T("--help"), argv[i]) == 0)
		{
			// the first command should always be help
			command = &commands[0];
		}
		else if (lstrcmpi(_T("--nologo"), argv[i]) == 0)
		{
			nologo = 1;
		}
		else
		{
			if (_tcsnccmp(argv[i], _T("-"), _tcslen(_T("-"))) == 0)
			{
				_ftprintf(stdout, _T("fatal error: don't know anything about '%s'\n"), argv[i]);
				return 1;
			}

			if (command)
			{
				
				_ftprintf(stdout, _T("fatal error: can either do '%s' or '%s'\n"), argv[i], command->name);
				return 2;
			}

			for (int j = 0; j < sizeof(commands)/sizeof(commands[0]); ++j)
			{
				if (lstrcmpi(commands[j].name, argv[i]) == 0)
				{
					command = &commands[j];
					break;
				}
			}

			if (!command)
			{
				_ftprintf(stdout, _T("fatal error: don't know how to '%s'.\n"), argv[i]);
				return 3;
			}
		}
	}

	if (!command)
	{
		_ftprintf(stdout, _T("fatal error: no command was provided.\n"));
		return 3;
	}

	if (!nologo)
	{
		phaka_show_logo();
	}

	return command->func(NULL);
}


int _tmain2(int argc, TCHAR* argv[])
{
	LPTSTR pszErrorMessage = NULL;
	LPTSTR pszTitle = NULL;
	LPTSTR pszModulePath = NULL;
	LPTSTR pszConfigurationPath = NULL;
	LPTSTR pszCommand = NULL;
	HRESULT hr = S_OK;
	const int size = MAX_PATH;

	if (SUCCEEDED(hr))
	{
		pszModulePath = LocalAlloc(LPTR, sizeof(TCHAR) * size);
		if (!pszModulePath)
		{
			hr = E_OUTOFMEMORY;
		}
	}

	if (SUCCEEDED(hr))
	{
		pszCommand = LocalAlloc(LPTR, sizeof(TCHAR) * size);
		if (!pszCommand)
		{
			hr = E_OUTOFMEMORY;
		}
	}

	if (SUCCEEDED(hr))
	{
		pszConfigurationPath = LocalAlloc(LPTR, sizeof(TCHAR) * size);
		if (!pszConfigurationPath)
		{
			hr = E_OUTOFMEMORY;
		}
	}

	if (SUCCEEDED(hr))
	{
		pszTitle = LocalAlloc(LPTR, sizeof(TCHAR) * _MAX_PATH);
		if (!pszTitle)
		{
			hr = E_OUTOFMEMORY;
		}
	}

	if (SUCCEEDED(hr))
	{
		if (!GetModuleFileName(NULL, pszModulePath, size))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}

	if (SUCCEEDED(hr))
	{
		hr = PhPathChangeExtension(pszConfigurationPath, size, pszModulePath, _T(".cfg"), 4);
	}

	if (SUCCEEDED(hr))
	{
		if (!PathFileExists(pszConfigurationPath))
		{
			_ftprintf(stderr, _T("The configuration file '%s' does not exist."), pszConfigurationPath);
			hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
		}
	}

	if (SUCCEEDED(hr))
	{
		hr = GetApplicationName(pszTitle, _MAX_PATH, pszConfigurationPath);
	}

	//if (SUCCEEDED(hr))
	//{
	//	if (!SetConsoleTitle(pszTitle))
	//	{
	//		hr = HRESULT_FROM_WIN32(GetLastError());
	//	}
	//}

	//if (SUCCEEDED(hr))
	//{
	//	hr = PrintLogo();
	//}

	//if (SUCCEEDED(hr))
	//{
	//	if (argc >= 2)
	//	{
	//		hr = StringCchCopy(pszCommand, 10, argv[1]);
	//	}
	//}

	if (SUCCEEDED(hr))
	{
		SERVICE_TABLE_ENTRY DispatchTable[] =
		{
			{SVCNAME, (LPSERVICE_MAIN_FUNCTION)SvcMain},
			{NULL, NULL}
		};

		if (!StartServiceCtrlDispatcher(DispatchTable))
		{
			SvcReportEvent(TEXT("StartServiceCtrlDispatcher"));
		}
	}

	if (SUCCEEDED(hr))
	{
		if (argc == 1)
		{
		}
		else if (lstrcmpi(pszCommand, TEXT("run")) == 0)
		{
			hr = DoRun(pszConfigurationPath);
		}
		else if (lstrcmpi(pszCommand, TEXT("install")) == 0)
		{
			hr = DoInstallService(pszConfigurationPath);
		}
		else if (lstrcmpi(pszCommand, TEXT("query")) == 0)
		{
			hr = DoQueryService(pszConfigurationPath);
		}
		else if (lstrcmpi(pszCommand, TEXT("describe")) == 0)
		{
			hr = DoDescribeService(pszConfigurationPath);
		}
		else if (lstrcmpi(pszCommand, TEXT("disable")) == 0)
		{
			hr = DoDisableService(pszConfigurationPath);
		}
		else if (lstrcmpi(pszCommand, TEXT("enable")) == 0)
		{
			hr = DoEnableService(pszConfigurationPath);
		}
		else if (lstrcmpi(pszCommand, TEXT("delete")) == 0)
		{
			hr = DoDeleteService(pszConfigurationPath);
		}
		else
		{
			_ftprintf(stderr, TEXT("Unknown command (%s)\n\n"), pszCommand);
		}
	}

	if (FAILED(hr))
	{
		if (SUCCEEDED(GetErrorMessage(&pszErrorMessage, hr)))
		{
			_ftprintf(stderr, _T("%s"), pszErrorMessage);
		}
		else
		{
			_ftprintf(stderr, _T("Failed to extract error"));
		}
		_ftprintf(stderr, _T("\n"));
	}

	LocalFree(pszModulePath);
	LocalFree(pszConfigurationPath);
	LocalFree(pszErrorMessage);
	LocalFree(pszTitle);
	return hr;
}
