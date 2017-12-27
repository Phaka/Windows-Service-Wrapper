#pragma once
#include "wrapper-error.h"
#include "service_config.h"

typedef int(*wrapper_command_func)(wrapper_config_t* config, wrapper_error_t** error);

typedef struct wrapper_command_t
{
	TCHAR* name;
	TCHAR* description;
	wrapper_command_func func;
} wrapper_command_t;

int wrapper_command_execute(wrapper_command_t* commands, const TCHAR* command_text, wrapper_config_t* config, wrapper_error_t** error);
