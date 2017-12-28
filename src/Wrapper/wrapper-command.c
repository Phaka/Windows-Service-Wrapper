// Copyright (c) Werner Strydom. All rights reserved.
// Licensed under the MIT license. See LICENSE in the project root for license information.

#include "stdafx.h"
#include "wrapper-command.h"

int wrapper_command_execute(wrapper_command_t* commands, const TCHAR* command_text, wrapper_config_t* config,
                            wrapper_error_t** error)
{
	for (size_t i = 0; commands[i].name != NULL; i++)
	{
		if (lstrcmpi(command_text, commands[i].name) == 0)
		{
			return commands[i].func(config, error);
		}
	}
	return -1;
}
