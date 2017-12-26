#include "stdafx.h"
#include "wrapper.h"


HRESULT GetErrorMessage(_Out_ LPTSTR* pszDest, _In_ HRESULT dwMessageId)
{
	DWORD rc = ERROR_SUCCESS;
	HINSTANCE hInstance;

	if (pszDest == NULL)
	{
		return E_INVALIDARG;
	}

	*pszDest = NULL;

	if (HRESULT_FACILITY(dwMessageId) == FACILITY_MSMQ)
	{
		hInstance = LoadLibrary(TEXT("MQUTIL.DLL"));
		if (hInstance != 0)
		{
			rc = FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_HMODULE |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				hInstance,
				dwMessageId,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR)pszDest,
				4096,
				NULL
			);
		}
	}
	else if (dwMessageId >= NERR_BASE && dwMessageId <= MAX_NERR)
	{
		hInstance = LoadLibrary(TEXT("NETMSG.DLL"));
		if (hInstance != 0)
		{
			rc = FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_HMODULE |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				hInstance,
				dwMessageId,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR)pszDest,
				4096,
				NULL
			);
		}
	}
	else
	{
		rc = FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dwMessageId,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)pszDest,
			4096,
			NULL
		);
	}

	if (!rc || *pszDest == NULL)
	{
		return HRESULT_FROM_WIN32(rc);
	}
	return S_OK;
}