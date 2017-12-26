// Copyright (c) Werner Strydom. All rights reserved.
// Licensed under the MIT license. See LICENSE in the project root for license information.

#include "stdafx.h"
#include "wrapper.h"

SERVICE_STATUS gSvcStatus;
SERVICE_STATUS_HANDLE gSvcStatusHandle;
HANDLE ghSvcStopEvent = NULL;
SERVICE_STATUS_HANDLE gSvcStatusHandle = NULL;




int _tmain(int argc, TCHAR* argv[])
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

	if (SUCCEEDED(hr))
	{
		if (!SetConsoleTitle(pszTitle))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}

	if (SUCCEEDED(hr))
	{
		hr = PrintLogo();
	}

	if (SUCCEEDED(hr))
	{
		if (argc >= 2)
		{
			hr = StringCchCopy(pszCommand, 10, argv[1]);
		}
	}

	if (SUCCEEDED(hr))
	{
		if (argc == 1)
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
