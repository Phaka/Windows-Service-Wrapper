// Copyright (c) Werner Strydom. All rights reserved.
// Licensed under the MIT license. See LICENSE in the project root for license information.

#pragma once
#include "wrapper-config.h"

int do_install(wrapper_config_t* config, wrapper_error_t** error);
int do_status(wrapper_config_t* config, wrapper_error_t** error);
int do_update(wrapper_config_t* config, wrapper_error_t** error);
int do_disable(wrapper_config_t* config, wrapper_error_t** error);
int do_enable(wrapper_config_t* config, wrapper_error_t** error);
int do_delete(wrapper_config_t* config, wrapper_error_t** error);
int do_start(wrapper_config_t* config, wrapper_error_t** error);
int do_stop(wrapper_config_t* config, wrapper_error_t** error);
int do_run(wrapper_config_t* config, wrapper_error_t** error);
