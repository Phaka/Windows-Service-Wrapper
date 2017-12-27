#pragma once


typedef struct wrapper_error_t 
{
	long code;
	TCHAR* message;
	TCHAR* user_message;
} wrapper_error_t;

wrapper_error_t *wrapper_error_from_system(DWORD code, TCHAR* format, ...);
wrapper_error_t *wrapper_error_from_hresult(long code, TCHAR* format, ...);
void wrapper_error_free(wrapper_error_t * error);

void wrapper_error_log(wrapper_error_t * error);

void wrapper_error_reset(wrapper_error_t** error);