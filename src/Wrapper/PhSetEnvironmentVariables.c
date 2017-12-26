#include "stdafx.h"
#include "wrapper.h"

HRESULT PhSetEnvironmentVariables(_In_ LPCTSTR pszConfigurationPath)
{
	//
	// If we pass environments to CreateProcess, then none of the existing variables will be visible to the
	// child process. So instead, we'll read the environment variables and manually set them, thus merging
	// environment variables.
	//
	HRESULT hr = S_OK;
	TCHAR* pszText = NULL;
	TCHAR* pszEnvironmentVariables = NULL;

	pszText = LocalAlloc(LPTR, sizeof(TCHAR) * _MAX_PATH);
	if (!pszText)
	{
		hr = E_OUTOFMEMORY;
	}

	pszEnvironmentVariables = LocalAlloc(LPTR, sizeof(TCHAR) * 32767);
	if (!pszEnvironmentVariables)
	{
		hr = E_OUTOFMEMORY;
	}

	if (SUCCEEDED(hr))
	{
		if (!GetPrivateProfileSection(_T("Environment"), pszEnvironmentVariables, 32767, pszConfigurationPath))
		{
			DWORD error = GetLastError();
			if (error != ERROR_FILE_NOT_FOUND)
			{
				hr = HRESULT_FROM_WIN32(error);
			}
		}
	}


	size_t length = 0;
	if (SUCCEEDED(hr))
	{
		hr = StringCbLength(pszEnvironmentVariables, 32767, &length);
	}

	if (SUCCEEDED(hr) && 0 < length)
	{
		for (LPTSTR pszVariable = (LPTSTR)pszEnvironmentVariables; *pszVariable; pszVariable++)
		{
			for (size_t i = 0; *pszVariable; i++)
			{
				pszText[i] = *pszVariable;
				pszVariable++;
			}

			wchar_t* pszToken = NULL;
			LPCTSTR pszName = _tcstok_s(pszText, _T("="), &pszToken);
			LPCTSTR pszValue = _tcstok_s(NULL, _T("="), &pszToken);

			if (!SetEnvironmentVariable(pszName, pszValue))
			{
				hr = HRESULT_FROM_WIN32(GetLastError());
				break;
			}
		}
	}

	LocalFree(pszText);
	LocalFree(pszEnvironmentVariables);

	return hr;
}