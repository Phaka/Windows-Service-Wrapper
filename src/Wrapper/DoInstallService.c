#include "stdafx.h"
#include "wrapper.h"

HRESULT DoInstallService(_In_ LPTSTR pszConfigurationPath)
{
	SC_HANDLE schSCManager = NULL;
	SC_HANDLE schService = NULL;
	LPTSTR pszModulePath = NULL;
	LPTSTR pszDisplayName = NULL;
	LPTSTR pszDescription = NULL;
	LPTSTR pszName = NULL;
	HRESULT hr = S_OK;
	DWORD dwLastError = ERROR_SUCCESS;

	if (SUCCEEDED(hr))
	{
		pszModulePath = LocalAlloc(LPTR, sizeof(TCHAR) * _MAX_PATH);
		if (!pszModulePath)
		{
			hr = E_OUTOFMEMORY;
		}
	}

	if (SUCCEEDED(hr))
	{
		pszDisplayName = LocalAlloc(LPTR, sizeof(TCHAR) * _MAX_PATH);
		if (!pszDisplayName)
		{
			hr = E_OUTOFMEMORY;
		}
	}

	if (SUCCEEDED(hr))
	{
		pszDescription = LocalAlloc(LPTR, sizeof(TCHAR) * _MAX_PATH);
		if (!pszDescription)
		{
			hr = E_OUTOFMEMORY;
		}
	}

	if (SUCCEEDED(hr))
	{
		pszName = LocalAlloc(LPTR, sizeof(TCHAR) * _MAX_PATH);
		if (!pszName)
		{
			hr = E_OUTOFMEMORY;
		}
	}

	if (SUCCEEDED(hr))
	{
		if (!GetModuleFileName(NULL, pszModulePath, MAX_PATH))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}

	if (SUCCEEDED(hr))
	{
		if (!GetPrivateProfileString(_T("Service"), _T("DisplayName"), _T(""), pszDisplayName, _MAX_PATH,
			pszConfigurationPath))
		{
			dwLastError = GetLastError();
			if (dwLastError != ERROR_FILE_NOT_FOUND)
			{
				hr = HRESULT_FROM_WIN32(dwLastError);
			}
		}
	}

	if (SUCCEEDED(hr))
	{
		if (!GetPrivateProfileString(_T("Service"), _T("Name"), _T(""), pszName, _MAX_PATH, pszConfigurationPath))
		{
			dwLastError = GetLastError();
			if (dwLastError != ERROR_FILE_NOT_FOUND)
			{
				hr = HRESULT_FROM_WIN32(dwLastError);
			}
		}
	}

	if (SUCCEEDED(hr))
	{
		if (!GetPrivateProfileString(_T("Service"), _T("Description"), _T(""), pszDescription, _MAX_PATH,
			pszConfigurationPath))
		{
			dwLastError = GetLastError();
			if (dwLastError != ERROR_FILE_NOT_FOUND)
			{
				hr = HRESULT_FROM_WIN32(dwLastError);
			}
		}
	}

	size_t length = 0;
	if (SUCCEEDED(hr))
	{
		hr = StringCbLength(pszName, _MAX_PATH, &length);
	}

	if (SUCCEEDED(hr))
	{
		if (length <= 0)
		{
			hr = StringCbCopy(pszName, _MAX_PATH, pszModulePath);

			if (SUCCEEDED(hr))
			{
				PathRemoveExtension(pszName);
				PathStripPath(pszName);
			}
		}
	}

	if (SUCCEEDED(hr))
	{
		hr = StringCbLength(pszDisplayName, _MAX_PATH, &length);
	}

	if (SUCCEEDED(hr))
	{
		if (length <= 0)
		{
			hr = StringCbCopy(pszDisplayName, _MAX_PATH, pszName);
		}
	}

	if (SUCCEEDED(hr))
	{
		schSCManager = OpenSCManager(
			NULL,
			NULL,
			SC_MANAGER_ALL_ACCESS);

		if (NULL == schSCManager)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}

	LPTSTR pszUserName = NULL;
	LPTSTR pszPassword = NULL;

	if (SUCCEEDED(hr))
	{
		schService = CreateService(
			schSCManager,
			pszName,
			pszDisplayName,
			SERVICE_ALL_ACCESS,
			SERVICE_WIN32_OWN_PROCESS,
			SERVICE_DEMAND_START,
			SERVICE_ERROR_NORMAL,
			pszModulePath,
			NULL,
			NULL,
			NULL,
			pszUserName,
			pszPassword);

		if (schService == NULL)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}

	if (SUCCEEDED(hr))
	{
		_ftprintf(stdout, _T("Service %s installed successfully\n"), pszName);
	}

	if (schService)
	{
		CloseServiceHandle(schService);
	}
	if (schSCManager)
	{
		CloseServiceHandle(schSCManager);
	}


	LocalFree(pszModulePath);
	LocalFree(pszDisplayName);
	LocalFree(pszDescription);
	LocalFree(pszName);

	return hr;
}