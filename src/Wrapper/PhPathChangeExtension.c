// Copyright (c) Werner Strydom. All rights reserved.
// Licensed under the MIT license. See LICENSE in the project root for license information.

#include "stdafx.h"
#include "wrapper.h"

HRESULT PhPathChangeExtension(
	_Out_ LPTSTR pszDest,
	_In_ size_t cchDest,
	_In_ LPCTSTR pszSrc,
	_In_ LPCTSTR pszExt,
	_In_ size_t cchExt)
{
	HRESULT hr = S_OK;
	size_t length = 0;

	hr = StringCchCopy(pszDest, cchDest, pszSrc);

	if (SUCCEEDED(hr) && cchExt > 0)
	{
		hr = StringCchLength(pszExt, cchExt + 1, &length);
	}

	if (SUCCEEDED(hr) && length > 0)
	{
		hr = PathCchRenameExtension(pszDest, cchDest, pszExt);
	}

	return hr;
}
