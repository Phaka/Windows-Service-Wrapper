#include "stdafx.h"
#include "wrapper.h"

// 
// For very long running process that produce a lot of output, we may need to investigate the following:
//
//   (1) Rolling Files based on date and size
//   (2) Creating date based directories, because IO gets slower the more files there are in the directory
//   (3) Whether the log files should be in the AppData folder of the current user; this may actually be more
//       secure that storing the log files in the same location as the executable, which could be readable
//       by any process
//   (4) Expand Variables that are provided in the szPathName, e.g. %APPDATA%\Selenium\Logs
//
HRESULT PhGetLogPathSetting(
	_Out_	      LPTSTR pszDest,
	_In_	      size_t cchDest,
	_In_	      LPCTSTR pszConfigurationPath)
{
	HRESULT hr = S_OK;
	DWORD dwLastError = ERROR_SUCCESS;
	const int size = MAX_PATH;
	TCHAR* pszSetting = NULL;
	TCHAR* pszBasePath = NULL;

	pszSetting = LocalAlloc(LPTR, sizeof(TCHAR) * size);
	if (!pszSetting)
	{
		hr = E_OUTOFMEMORY;
	}

	if (SUCCEEDED(hr))
	{
		pszBasePath = LocalAlloc(LPTR, sizeof(TCHAR) * size);
		if (!pszBasePath)
		{
			hr = E_OUTOFMEMORY;
		}
	}

	if (SUCCEEDED(hr))
	{
		if (!GetPrivateProfileString(_T("Logging"), _T("Path"), _T(""), pszSetting, size, pszConfigurationPath))
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
		hr = StringCchLength(pszSetting, size, &length);
	}

	// If no log szPathName was specified, we're just going to write it in the location where the
	// executable is at. This is generally a bad idea, since the szPathName of the executable is
	// readable by almost anyone. If logs have any kind of need to know information, we may
	// be causing more trouble that we should. We'll revisit this later. Maybe we should store
	// this in %APPDATA%\%ServiceName%\logs?
	if (SUCCEEDED(hr))
	{
		if (0 >= length)
		{
			hr = PhPathChangeExtension(pszDest, cchDest, pszConfigurationPath, _T(".log"), 4);
		}
		else if (PathIsRelative(pszSetting))
		{
			if (SUCCEEDED(hr))
			{
				hr = StringCchCopy(pszDest, cchDest, pszConfigurationPath);
			}

			if (SUCCEEDED(hr))
			{
				hr = PathCchRemoveFileSpec(pszDest, cchDest);
			}
			if (SUCCEEDED(hr))
			{
				hr = PathCchAppend(pszDest, cchDest, pszSetting);
			}

			if (SUCCEEDED(hr))
			{
				hr = StringCchCopy(pszBasePath, size, pszDest);
			}

			if (SUCCEEDED(hr))
			{
				hr = PathCchRemoveFileSpec(pszBasePath, size);
			}

			if (SUCCEEDED(hr))
			{
				hr = PhCreateDirectory(pszBasePath);
			}
		}
		else
		{
			hr = StringCchCopy(pszDest, cchDest, pszSetting);
		}
	}

	LocalFree(pszSetting);
	LocalFree(pszBasePath);

	return hr;
}