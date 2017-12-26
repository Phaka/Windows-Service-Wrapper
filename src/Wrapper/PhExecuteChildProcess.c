#include "stdafx.h"
#include "wrapper.h"

HRESULT PhExecuteChildProcess(
	_In_ LPTSTR pszCommandLine,
	_In_ LPCTSTR pszCurrentDirectory,
	_In_ HANDLE hStdOutput)
{
	HRESULT hr = S_OK;
	PROCESS_INFORMATION* lpProcessInformation = NULL;
	STARTUPINFO* lpStartUpInfo = NULL;
	size_t length = 0;

	lpProcessInformation = LocalAlloc(LPTR, sizeof(PROCESS_INFORMATION));
	if (!lpProcessInformation)
	{
		hr = E_OUTOFMEMORY;
	}

	lpStartUpInfo = LocalAlloc(LPTR, sizeof(STARTUPINFO));
	if (!lpStartUpInfo)
	{
		hr = E_OUTOFMEMORY;
	}

	if (SUCCEEDED(hr))
	{
		hr = StringCbLength(pszCurrentDirectory, _MAX_PATH, &length);
		if (FAILED(hr))
		{
			return hr;
		}
	}

	if (SUCCEEDED(hr))
	{
		if (length == 0)
		{
			// Create process will fail if the current directory is empty (that is "\0"), so
			// we'll just make it NULL.
			pszCurrentDirectory = NULL;
		}

		lpStartUpInfo->cb = sizeof(STARTUPINFO);
		lpStartUpInfo->hStdError = hStdOutput;
		lpStartUpInfo->hStdOutput = hStdOutput;
		lpStartUpInfo->dwFlags |= STARTF_USESTDHANDLES;

#if UNICODE
		int flags = CREATE_UNICODE_ENVIRONMENT;
#else
		int flags = 0;
#endif

		if (!CreateProcess(NULL,
			pszCommandLine,
			NULL,
			NULL,
			TRUE,
			flags,
			NULL,
			pszCurrentDirectory,
			lpStartUpInfo,
			lpProcessInformation))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}

	if (SUCCEEDED(hr))
	{
		if (WAIT_FAILED == WaitForSingleObject(lpProcessInformation->hProcess, INFINITE))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}

	if (lpProcessInformation)
	{
		if (lpProcessInformation->hProcess)
		{
			CloseHandle(lpProcessInformation->hProcess);
		}
		if (lpProcessInformation->hThread)
		{
			CloseHandle(lpProcessInformation->hThread);
		}
	}

	LocalFree(lpProcessInformation);
	LocalFree(lpStartUpInfo);

	return hr;
}