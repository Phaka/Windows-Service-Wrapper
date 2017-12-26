#include "stdafx.h"
#include "wrapper.h"

HRESULT PhCreateDirectory(_In_ LPCTSTR szPathName)
{
	TCHAR* pszDirectoryName;
	TCHAR* end = NULL;
	HRESULT hr = S_OK;
	size_t cbToCopy;
	DWORD error = ERROR_SUCCESS;

	if (PathFileExists(szPathName))
	{
		return S_OK;
	}

	pszDirectoryName = LocalAlloc(LPTR, sizeof(TCHAR) * MAX_PATH);
	if (!pszDirectoryName)
	{
		return E_OUTOFMEMORY;
	}

	end = _tcschr(szPathName, _T('\\')); // this would S:
	if (end)
	{
		end = _tcschr(++end, _T('\\'));
	}

	while (SUCCEEDED(hr) && end != NULL)
	{
		if (SUCCEEDED(hr))
		{
			cbToCopy = sizeof(TCHAR) * (end - szPathName);
			hr = StringCbCopyN(pszDirectoryName, _MAX_PATH, szPathName, cbToCopy);
		}

		if (SUCCEEDED(hr))
		{
			if (!CreateDirectory(pszDirectoryName, NULL))
			{
				error = GetLastError();
				if (error != ERROR_ALREADY_EXISTS)
				{
					hr = HRESULT_FROM_WIN32(error);
				}
			}
		}

		end = _tcschr(++end, _T('\\'));
	}

	if (!CreateDirectory(szPathName, NULL))
	{
		error = GetLastError();
		if (error != ERROR_ALREADY_EXISTS)
		{
			hr = HRESULT_FROM_WIN32(error);
		}
	}

	LocalFree(pszDirectoryName);
	return hr;
}