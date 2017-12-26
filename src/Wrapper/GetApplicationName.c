#include "stdafx.h"
#include "wrapper.h"

HRESULT GetApplicationName(
	_Out_	     	     	     	     	      LPTSTR pszDest,
	_In_	     	     	     	     	      size_t cchDest,
	_In_opt_	     	     	     	     	      LPCTSTR pszConfigurationPath)
{
	HRESULT hr = S_OK;
	DWORD dwLastError = ERROR_SUCCESS;
	size_t length = 0;

	if (!pszDest)
	{
		return E_INVALIDARG;
	}

	if (cchDest <= 0)
	{
		return E_INVALIDARG;
	}

	if (pszConfigurationPath)
	{
		if (SUCCEEDED(hr))
		{
			if (!GetPrivateProfileString(_T("Service"), _T("DisplayName"), _T(""), pszDest, cchDest, pszConfigurationPath))
			{
				dwLastError = GetLastError();
				if (dwLastError != ERROR_FILE_NOT_FOUND)
				{
					hr = HRESULT_FROM_WIN32(dwLastError);
				}
			}

			if (SUCCEEDED(hr))
			{
				hr = StringCbLength(pszDest, cchDest, &length);
			}
		}

		if (SUCCEEDED(hr))
		{
			if (length <= 0)
			{
				if (!GetPrivateProfileString(_T("Service"), _T("DisplayName"), _T(""), pszDest, cchDest, pszConfigurationPath))
				{
					dwLastError = GetLastError();
					if (dwLastError != ERROR_FILE_NOT_FOUND)
					{
						hr = HRESULT_FROM_WIN32(dwLastError);
					}
				}

				if (SUCCEEDED(hr))
				{
					hr = StringCbLength(pszDest, cchDest, &length);
				}
			}
		}
	}

	if (SUCCEEDED(hr))
	{
		if (length <= 0)
		{
			if (!GetModuleFileName(NULL, pszDest, cchDest))
			{
				hr = HRESULT_FROM_WIN32(GetLastError());
			}

			if (SUCCEEDED(hr))
			{
				PathStripPath(pszDest);
				PathRemoveExtension(pszDest);
			}
		}
	}
	return hr;
}
