#pragma once
#include "wrapper-config.h"

int wrapper_service_install(wrapper_config_t* config, wrapper_error_t** error);
int wrapper_service_query(wrapper_config_t* config, wrapper_error_t** error);
int wrapper_service_update(wrapper_config_t* config, wrapper_error_t** error);
int wrapper_service_disable(wrapper_config_t* config, wrapper_error_t** error);
int wrapper_service_enable(wrapper_config_t* config, wrapper_error_t** error);
int wrapper_service_delete(wrapper_config_t* config, wrapper_error_t** error);
int wrapper_service_start(wrapper_config_t* config, wrapper_error_t** error);
int wrapper_service_dacl(wrapper_config_t* config, wrapper_error_t** error);
int wrapper_service_stop(wrapper_config_t* config, wrapper_error_t** error);
int wrapper_service_run(wrapper_config_t* config, wrapper_error_t** error);