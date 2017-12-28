// Copyright (c) Werner Strydom. All rights reserved.
// Licensed under the MIT license. See LICENSE in the project root for license information.

#include "stdafx.h"
#include "wrapper-error.h"
#include "wrapper-config.h"

wrapper_config_t* wrapper_config_alloc(void)
{
	wrapper_config_t* config = NULL;
	config = LocalAlloc(LPTR, sizeof(wrapper_config_t));
	if (config)
	{
		config->name = LocalAlloc(LPTR, sizeof(TCHAR) * (WRAPPER_SERVICE_NAME_MAX_LEN + 1));
		config->title = LocalAlloc(LPTR, sizeof(TCHAR) * (WRAPPER_SERVICE_TITLE_MAX_LEN + 1));
		config->description = LocalAlloc(LPTR, sizeof(TCHAR) * (WRAPPER_SERVICE_DESCRIPTION_MAX_LEN + 1));
		config->command_line = LocalAlloc(LPTR, sizeof(TCHAR) * (WRAPPER_SERVICE_CMDLINE_MAX_LEN + 1));
		config->working_directory = LocalAlloc(LPTR, sizeof(TCHAR) * (WRAPPER_SERVICE_WORKDIR_MAX_LEN + 1));

		// If any member is NULL, then we do not have sufficient memory. 
		if (!config->name || !config->title || !config->description || !config->command_line || !config->working_directory)
		{
			wrapper_config_free(config);
			config = NULL;
		}
	}
	return config;
}

void wrapper_config_free(wrapper_config_t* config)
{
	if (config)
	{
		LocalFree(config->name);
		LocalFree(config->title);
		LocalFree(config->description);
		LocalFree(config->command_line);
		LocalFree(config);
	}
}

int wrapper_config_get_path(TCHAR* path, const size_t size, wrapper_error_t** error)
{
	int rc = 1;
	if (rc)
	{
		if (!GetModuleFileName(NULL, path, size))
		{
			if (error)
			{
				*error = wrapper_error_from_system(GetLastError(), _T("Unable to get the current module name"));
			}
			rc = 0;
		}
	}

	if (rc)
	{
		HRESULT hr = PathCchRenameExtension(path, size, _T(".cfg"));
		if (FAILED(hr))
		{
			if (error)
			{
				*error = wrapper_error_from_system(hr, _T("Unable to change the extension of '%s' to '.cfg'"), path);
			}
			rc = 0;
		}
	}

	return rc;
}

int wrapper_config_read_string(
	TCHAR* buffer,
	DWORD size,
	TCHAR* section,
	TCHAR* key,
	TCHAR* default_value,
	TCHAR* path,
	wrapper_error_t** error
)
{
	if (GetPrivateProfileString(section, key, EMPTY_STRING, buffer, size, path))
	{
		return 1;
	}

	DWORD last_error = GetLastError();
	if (default_value && ERROR_FILE_NOT_FOUND == last_error)
	{
		StringCbCopy(buffer, size, default_value);
		return 1;
	}

	if (error)
	{
		*error = wrapper_error_from_system(
			last_error, _T("Unable to read the value of '%s' in section '%s' of configuration file '%s'"), key, section, path);
	}
	return 0;
}

int wrapper_config_read(TCHAR* path, wrapper_config_t* config, wrapper_error_t** error)
{
	if (!path)
	{
		return 0;
	}

	if (!config)
	{
		return 0;
	}

	TCHAR* section_name = _T("Unit");

	if (!wrapper_config_read_string(config->name, WRAPPER_SERVICE_NAME_MAX_LEN, section_name, _T("Name"), NULL, path,
	                                error))
	{
		return 0;
	}

	if (!wrapper_config_read_string(config->title, WRAPPER_SERVICE_TITLE_MAX_LEN, section_name, _T("Title"), config->name,
	                                path, error))
	{
		return 0;
	}

	if (!wrapper_config_read_string(config->description, WRAPPER_SERVICE_DESCRIPTION_MAX_LEN, section_name,
	                                _T("Description"), EMPTY_STRING, path, error))
	{
		return 0;
	}

	if (!wrapper_config_read_string(config->command_line, WRAPPER_SERVICE_CMDLINE_MAX_LEN, section_name, _T("CommandLine"),
	                                NULL, path, error))
	{
		return 0;
	}

	if (!wrapper_config_read_string(config->working_directory, WRAPPER_SERVICE_WORKDIR_MAX_LEN, section_name,
	                                _T("WorkingDirectory"), EMPTY_STRING, path, error))
	{
		return 0;
	}

	return 1;
}
