#include "stdafx.h"
#include "wrapper.h"

HRESULT GetCommandLineSetting(
	_Out_	     	     	     	     	      LPTSTR pszDest,
	_In_	     	     	     	     	      size_t cchDest,
	_In_	     	     	     	     	      LPCTSTR pszConfigurationPath)
{
	HRESULT hr = S_OK;
	DWORD dwLastError = ERROR_SUCCESS;
	if (!GetPrivateProfileString(_T("Application"), _T("CommandLine"), _T(""), pszDest, cchDest, pszConfigurationPath))
	{
		dwLastError = GetLastError();
		if (dwLastError != ERROR_FILE_NOT_FOUND)
		{
			hr = HRESULT_FROM_WIN32(dwLastError);
		}
	}

	return hr;
}