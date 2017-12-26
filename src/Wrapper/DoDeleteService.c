#include "stdafx.h"
#include "wrapper.h"

HRESULT DoDeleteService(_In_ LPTSTR pszConfigurationPath)
{
	SC_HANDLE schSCManager = NULL;
	SC_HANDLE schService = NULL;
	LPTSTR pszModulePath = NULL;
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
		if (!GetPrivateProfileString(_T("Service"), _T("Name"), _T(""), pszName, _MAX_PATH, pszConfigurationPath))
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
		schSCManager = OpenSCManager(
			NULL,
			NULL,
			SC_MANAGER_ALL_ACCESS);

		if (NULL == schSCManager)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}

	if (SUCCEEDED(hr))
	{
		schService = OpenService(
			schSCManager,
			pszName,
			DELETE);

		if (schService == NULL)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}

	if (SUCCEEDED(hr))
	{
		if (!DeleteService(schService))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}

	if (SUCCEEDED(hr))
	{
		_ftprintf(stdout, _T("Service %s deleted successfully\n"), pszName);
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
	LocalFree(pszName);

	return hr;
}