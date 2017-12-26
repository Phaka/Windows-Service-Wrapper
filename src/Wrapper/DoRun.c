#include "stdafx.h"
#include "wrapper.h"

HRESULT DoRun(_In_ LPCTSTR pszConfigurationPath)
{
	const size_t size = _MAX_PATH;
	HANDLE hStdOut = NULL;
	LPTSTR pszBasePath = NULL;
	LPTSTR pszCommandLine = NULL;
	LPTSTR pszLogPath = NULL;
	LPTSTR pszWorkingDirectory = NULL;
	HRESULT hr = S_OK;

	if (!pszConfigurationPath)
	{
		return E_INVALIDARG;
	}

	pszBasePath = LocalAlloc(LPTR, sizeof(TCHAR) * size);
	if (!pszBasePath)
	{
		hr = E_OUTOFMEMORY;
	}

	pszCommandLine = LocalAlloc(LPTR, sizeof(TCHAR) * size);
	if (!pszCommandLine)
	{
		hr = E_OUTOFMEMORY;
	}

	pszLogPath = LocalAlloc(LPTR, sizeof(TCHAR) * size);
	if (!pszLogPath)
	{
		hr = E_OUTOFMEMORY;
	}

	pszWorkingDirectory = LocalAlloc(LPTR, sizeof(TCHAR) * size);
	if (!pszWorkingDirectory)
	{
		hr = E_OUTOFMEMORY;
	}

	if (SUCCEEDED(hr))
	{
		hr = PhGetDirectoryName(pszBasePath, size, pszConfigurationPath);
	}

	if (SUCCEEDED(hr))
	{
		if (!SetEnvironmentVariable(_T("PHAKA_SERVICEW_BASEPATH"), pszBasePath))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}

	if (SUCCEEDED(hr))
	{
		hr = PhGetLogPathSetting(pszLogPath, size, pszConfigurationPath);
	}

	if (SUCCEEDED(hr))
	{
		hr = GetCommandLineSetting(pszCommandLine, size, pszConfigurationPath);
	}

	if (SUCCEEDED(hr))
	{
		hr = PhSetEnvironmentVariables(pszConfigurationPath);
	}

	if (SUCCEEDED(hr))
	{
		hr = PhGetWorkingDirectory(pszBasePath, size, pszConfigurationPath);
	}

	if (SUCCEEDED(hr))
	{
		hr = OpenLogFile(pszLogPath, &hStdOut);
	}

	if (SUCCEEDED(hr))
	{
		hr = PhExecuteChildProcess(pszCommandLine, pszWorkingDirectory, hStdOut);
	}

	if (hStdOut)
	{
		CloseHandle(hStdOut);
	}

	LocalFree(pszBasePath);
	LocalFree(pszCommandLine);
	LocalFree(pszLogPath);
	LocalFree(pszWorkingDirectory);

	return hr;
}