#include "stdafx.h"
#include "messages.h"

HRESULT GetServiceName(_Out_ LPTSTR  pszDest, _In_  size_t  cbDest)
{
	//TODO: Read from Configuration File
	return StringCbCopy(pszDest, cbDest, _T("Sample2"));
}