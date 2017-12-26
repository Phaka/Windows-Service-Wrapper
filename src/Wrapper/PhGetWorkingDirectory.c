#include "stdafx.h"
#include "wrapper.h"

HRESULT PhGetWorkingDirectory(
	_Out_ LPTSTR pszDest,
	_In_ size_t cchDest,
	_In_ LPCTSTR pszConfigurationPath)
{
	HRESULT hr = S_OK;
	size_t length = 0;
	DWORD dwLastError = ERROR_SUCCESS;

	if (!GetPrivateProfileString(_T("Application"), _T("WorkingDirectory"), _T(""), pszDest, cchDest,
		pszConfigurationPath))
	{
		dwLastError = GetLastError();
		if (dwLastError != ERROR_FILE_NOT_FOUND)
		{
			hr = HRESULT_FROM_WIN32(dwLastError);
		}
	}

	if (SUCCEEDED(hr))
	{
		hr = StringCchLength(pszDest, cchDest, &length);
	}

	if (SUCCEEDED(hr))
	{
		if (0 >= length)
		{
			hr = PhGetDirectoryName(pszDest, cchDest, pszConfigurationPath);
		}
	}

	return hr;
}