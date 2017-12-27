// Copyright (c) Werner Strydom. All rights reserved.
// Licensed under the MIT license. See LICENSE in the project root for license information.

#include "stdafx.h"
#include "wrapper.h"
#include "service.h"
#include "service_config.h"
#include "wrapper-log.h"

int __cdecl _tmain(int argc, TCHAR* argv[])
{
	int rc = 0;
	TCHAR service_name[_MAX_PATH];
	GetServiceName(service_name, _MAX_PATH);

	if (argc > 1)
	{
		if (lstrcmpi(argv[1], TEXT("install")) == 0)
		{
			wrapper_service_install(service_name);
		}
		else if (lstrcmpi(argv[1], TEXT("query")) == 0)
		{
			wrapper_service_query(service_name);
		}
		else if (lstrcmpi(argv[1], TEXT("describe")) == 0)
		{
			wrapper_service_update(service_name);
		}
		else if (lstrcmpi(argv[1], TEXT("disable")) == 0)
		{
			wrapper_service_disable(service_name);
		}
		else if (lstrcmpi(argv[1], TEXT("enable")) == 0)
		{
			wrapper_service_enable(service_name);
		}
		else if (lstrcmpi(argv[1], TEXT("delete")) == 0)
		{
			wrapper_service_delete(service_name);
		}
		else if (lstrcmpi(argv[1], TEXT("start")) == 0)
		{
			wrapper_service_start(service_name);
		}
		else if (lstrcmpi(argv[1], TEXT("dacl")) == 0)
		{
			wrapper_service_dacl(service_name);
		}
		else if (lstrcmpi(argv[1], TEXT("stop")) == 0)
		{
			wrapper_service_stop(service_name);
		}
		else
		{
			_ftprintf(stderr, _T("fatal error: don't know how to '%s'"), argv[1]);
			rc = 1;
		}
	}
	else
	{
		wrapper_service_run(service_name);
	}
	return rc;
}
