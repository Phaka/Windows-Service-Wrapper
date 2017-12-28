// Copyright (c) Werner Strydom. All rights reserved.
// Licensed under the MIT license. See LICENSE in the project root for license information.

#pragma once

#define WRAPPER_SERVICE_NAME_MAX_LEN 256
#define WRAPPER_SERVICE_TITLE_MAX_LEN 256
#define WRAPPER_SERVICE_DESCRIPTION_MAX_LEN 4096
#define WRAPPER_SERVICE_CMDLINE_MAX_LEN 4096
#define WRAPPER_SERVICE_WORKDIR_MAX_LEN 260 // _MAX_PATH

#define EMPTY_STRING _T("")

#include "wrapper-error.h"

typedef struct wrapper_config_t
{
	TCHAR* name;
	TCHAR* command_line;
	TCHAR* title;
	TCHAR* description;
	TCHAR* working_directory;
} wrapper_config_t;

wrapper_config_t* wrapper_config_alloc(void);
void wrapper_config_free(wrapper_config_t* config);

int wrapper_config_get_path(TCHAR* path, size_t size, wrapper_error_t** error);
int wrapper_config_read(TCHAR* path, wrapper_config_t* config, wrapper_error_t** error);
int wrapper_config_read_string(
	TCHAR* buffer,
	DWORD size,
	TCHAR* section,
	TCHAR* key,
	TCHAR* default_value,
	TCHAR* path,
	wrapper_error_t** error
);
