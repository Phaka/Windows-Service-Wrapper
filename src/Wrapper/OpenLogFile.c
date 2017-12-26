#include "stdafx.h"
#include "wrapper.h"

HRESULT OpenLogFile(_In_ LPCTSTR pszLogPath, _Out_ HANDLE* hFile)
{
	HRESULT hr = S_OK;
	SECURITY_ATTRIBUTES security_attributes;
	ZeroMemory(&security_attributes, sizeof security_attributes);
	security_attributes.nLength = sizeof security_attributes;
	security_attributes.lpSecurityDescriptor = NULL;
	security_attributes.bInheritHandle = TRUE;

	if (NULL == hFile)
	{
		return E_INVALIDARG;
	}

	*hFile = CreateFile(
		pszLogPath,
		FILE_APPEND_DATA,
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		&security_attributes,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (!*hFile)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
	}

	return hr;
}