#include "stdafx.h"
#include "wrapper.h"

HRESULT DisplayUsage()
{
	LPTSTR pszApplicationName = NULL;
	HRESULT hr = S_OK;

	pszApplicationName = LocalAlloc(LPTR, _MAX_PATH * sizeof(TCHAR));
	if (!pszApplicationName)
	{
		return E_OUTOFMEMORY;
	}

	if (SUCCEEDED(hr))
	{
		hr = GetApplicationName(pszApplicationName, _MAX_PATH, NULL);
	}

	if (SUCCEEDED(hr))
	{
		_ftprintf(stdout, _T("Description:\n"));
		_ftprintf(stdout, _T("\tCommand-line tool that configures a service.\n\n"));
		_ftprintf(stdout, _T("Usage:\n"));
		_ftprintf(stdout, _T("\t%s [command]\n\n"), pszApplicationName);
		_ftprintf(stdout, _T("\t[command]\n"));
		_ftprintf(stdout, _T("\t  query\n"));
		_ftprintf(stdout, _T("\t  describe\n"));
		_ftprintf(stdout, _T("\t  disable\n"));
		_ftprintf(stdout, _T("\t  enable\n"));
		_ftprintf(stdout, _T("\t  delete\n"));
	}

	return S_OK;
}