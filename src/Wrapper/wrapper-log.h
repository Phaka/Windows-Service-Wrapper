#pragma once

#ifndef WRAPPER_LOG_DOMAIN
#define WRAPPER_LOG_DOMAIN _T("wrapper")
#endif


#define WRAPPER_ERROR(...) \
   wrapper_log (WRAPPER_LOG_LEVEL_ERROR, WRAPPER_LOG_DOMAIN, __VA_ARGS__)
#define WRAPPER_CRITICAL(...) \
   wrapper_log (WRAPPER_LOG_LEVEL_CRITICAL, WRAPPER_LOG_DOMAIN, __VA_ARGS__)
#define WRAPPER_WARNING(...) \
   wrapper_log (WRAPPER_LOG_LEVEL_WARNING, WRAPPER_LOG_DOMAIN, __VA_ARGS__)
#define WRAPPER_MESSAGE(...) \
   wrapper_log (WRAPPER_LOG_LEVEL_MESSAGE, WRAPPER_LOG_DOMAIN, __VA_ARGS__)
#define WRAPPER_INFO(...) \
   wrapper_log (WRAPPER_LOG_LEVEL_INFO, WRAPPER_LOG_DOMAIN, __VA_ARGS__)
#define WRAPPER_DEBUG(...) \
   wrapper_log (WRAPPER_LOG_LEVEL_DEBUG, WRAPPER_LOG_DOMAIN, __VA_ARGS__)


typedef enum {
	WRAPPER_LOG_LEVEL_ERROR,
	WRAPPER_LOG_LEVEL_CRITICAL,
	WRAPPER_LOG_LEVEL_WARNING,
	WRAPPER_LOG_LEVEL_MESSAGE,
	WRAPPER_LOG_LEVEL_INFO,
	WRAPPER_LOG_LEVEL_DEBUG,
	WRAPPER_LOG_LEVEL_TRACE,
} wrapper_log_level_t;

typedef void(*wrapper_log_func_t) (wrapper_log_level_t log_level,
	const TCHAR *log_domain,
	const TCHAR *message,
	void *user_data);


void wrapper_log_set_handler(wrapper_log_func_t log_func, void *user_data);

void wrapper_log(wrapper_log_level_t log_level,
	const TCHAR *log_domain,
	const TCHAR *format,
	...);

void wrapper_log_console_handler(wrapper_log_level_t log_level,
	const TCHAR *log_domain,
	const TCHAR *message,
	void *user_data);

void wrapper_log_file_handler(wrapper_log_level_t log_level,
	const TCHAR *log_domain,
	const TCHAR *message,
	void *user_data);

const TCHAR *wrapper_log_level_str(wrapper_log_level_t log_level);
