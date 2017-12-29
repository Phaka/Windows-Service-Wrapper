// Copyright (c) Werner Strydom. All rights reserved.
// Licensed under the MIT license. See LICENSE in the project root for license information.

#pragma once
#include "wrapper-error.h"
#include "service_config.h"
#include "wrapper-command.h"

extern wrapper_command_t commands[];

int do_help(wrapper_config_t* config, wrapper_error_t** error);
