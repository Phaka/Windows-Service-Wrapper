#pragma once
#include "wrapper-error.h"
#include "service_config.h"
#include "wrapper-command.h"

extern wrapper_command_t commands[];

int wrapper_help(wrapper_config_t* config, wrapper_error_t** error);
