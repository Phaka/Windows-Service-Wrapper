#include "stdafx.h"
#include "wrapper.h"

HRESULT PrintLogo()
{
	LPCTSTR pszVersion = _T("1.0.0.0");
	_ftprintf(stdout, _T("Phaka Windows Service Wrapper version %s\n"), pszVersion);
	_ftprintf(stdout, _T("Copyright (C) Werner Strydom. All rights reserved.\n"));
	_ftprintf(stdout, _T("Licensed under the MIT license. See LICENSE for license information.\n"));
	_ftprintf(stdout, _T("\n"));

	return S_OK;
}