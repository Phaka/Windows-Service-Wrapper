// Copyright (c) Werner Strydom. All rights reserved.
// Licensed under the MIT license. See LICENSE in the project root for license information.

#include "stdafx.h"
#include "wrapper-help.h"
#include "wrapper-memory.h"

int wrapper_command_get_executable_name(TCHAR* destination, size_t size, wrapper_error_t** error)
{
	if (!GetModuleFileName(NULL, destination, size))
	{
		if (error)
		{
			*error = wrapper_error_from_system(GetLastError(), _T("Unable to get the module path"));
		}
		return 0;
	}

	PathCchRemoveExtension(destination, size);
	PathStripPath(destination);
	CharLower(destination);

	return 1;
}

int do_help(wrapper_config_t* config, wrapper_error_t** error)
{
	int rc = 1;
	TCHAR* name = wrapper_allocate_string(_MAX_PATH);
	if (!name)
	{
		if (error)
		{
			*error = wrapper_error_from_hresult(E_OUTOFMEMORY, _T("Failed to allocate memory for the executable name"));
		}
		rc = 0;
	}

	if (rc)
	{
		rc = wrapper_command_get_executable_name(name, _MAX_PATH, error);
	}

	if (rc)
	{
		_ftprintf(stdout, _T("Usage:\n"));
		_ftprintf(stdout, _T("  %s [command]\n"), name);
		_ftprintf(stdout, _T("\n"));
		_ftprintf(stdout, _T("Commands:\n"));
		for (size_t i = 0; commands[i].name != NULL; i++)
		{
			_ftprintf(stdout, _T("  %-15s %s\n"), commands[i].name, commands[i].description);
		}

		_ftprintf(stdout, _T("\n"));
		_ftprintf(stdout, _T("Options:\n"));
		_ftprintf(stdout, _T("  %-15s %s\n"), _T("--nologo"),
		          _T("Suppress the display the startup banner and copyright message."));
		_ftprintf(stdout, _T("\n"));
	}

	wrapper_free(name);
	return rc;
}
