#include "stdafx.h"
#include "wrapper.h"

HRESULT PhGetDirectoryName(
	_Out_	     	     	     	     	      LPTSTR pszDest,
	_In_	     	     	     	     	      size_t cchDest,
	_In_	     	     	     	     	      LPCTSTR pszSrc)
{
	HRESULT hr = S_OK;

	if (SUCCEEDED(hr))
	{
		hr = StringCchCopy(pszDest, cchDest, pszSrc);
	}

	if (SUCCEEDED(hr))
	{
		hr = PathCchRemoveFileSpec(pszDest, cchDest);
	}

	return hr;
}