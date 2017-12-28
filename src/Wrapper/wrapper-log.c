#include "stdafx.h"
#include "wrapper-log.h"
#include "wrapper-error.h"

static wrapper_log_func_t func = wrapper_log_console_handler;
static void *data;

void wrapper_log_set_handler(wrapper_log_func_t log_func, void *user_data)
{
	func = log_func;
	data = user_data;
}

void _wrapper_log_get_handler(wrapper_log_func_t *log_func, void **user_data)
{
	*log_func = func;
	*user_data = data;
}

void wrapper_log(wrapper_log_level_t log_level,
	const TCHAR *log_domain,
	const TCHAR *format,
	...)
{
	va_list args;
	const size_t message_size = 1024;
	TCHAR *message;

	if (!func) {
		return;
	}

	message = LocalAlloc(LPTR, message_size * sizeof(TCHAR));
	va_start(args, format);
	_vsntprintf_s(message, message_size, _TRUNCATE, format, args);
	va_end(args);

	func(log_level, log_domain, message, data);
	
	LocalFree(message);
}


const TCHAR *wrapper_log_level_str(wrapper_log_level_t log_level)
{
	switch (log_level) {
	case WRAPPER_LOG_LEVEL_ERROR:
		return _T("ERROR");
	case WRAPPER_LOG_LEVEL_CRITICAL:
		return _T("CRITICAL");
	case WRAPPER_LOG_LEVEL_WARNING:
		return _T("WARNING");
	case WRAPPER_LOG_LEVEL_MESSAGE:
		return _T("MESSAGE");
	case WRAPPER_LOG_LEVEL_INFO:
		return _T("INFO");
	case WRAPPER_LOG_LEVEL_DEBUG:
		return _T("DEBUG");
	case WRAPPER_LOG_LEVEL_TRACE:
		return _T("TRACE");
	default:
		return _T("UNKNOWN");
	}
}


#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64

void wrapper_log_console_handler(wrapper_log_level_t log_level,
	const TCHAR *log_domain,
	const TCHAR *message,
	void *user_data)
{
	FILE *stream;
	switch (log_level) {
	case WRAPPER_LOG_LEVEL_ERROR:
	case WRAPPER_LOG_LEVEL_CRITICAL:
	case WRAPPER_LOG_LEVEL_WARNING:
		stream = stderr;
		break;
	case WRAPPER_LOG_LEVEL_MESSAGE:
	case WRAPPER_LOG_LEVEL_INFO:
	case WRAPPER_LOG_LEVEL_DEBUG:
	case WRAPPER_LOG_LEVEL_TRACE:
	default:
		stream = stdout;
	}

	if (log_level != WRAPPER_LOG_LEVEL_INFO)
	{
		_ftprintf(stream, _T("%s: %s\n"), wrapper_log_level_str(log_level), message);
	}
	else 
	{
		_ftprintf(stream, _T("%s\n"), message);
	}
}


void wrapper_log_file_handler(wrapper_log_level_t log_level,
	const TCHAR *log_domain,
	const TCHAR *message,
	void *user_data)
{
	FILETIME ft;
	struct timeval tv;
	struct tm tt;
	time_t t;
	FILE *stream;
	TCHAR nowstr[32];
	int pid;
	TCHAR buf[128] = { 0 };
	TCHAR duration[128] = { 0 };
	TCHAR time[128] = { 0 };

	SYSTEMTIME lt = { 0 };

	GetSystemTimeAsFileTime(&ft);
	unsigned __int64 tmpres = 0;
	tmpres |= ft.dwHighDateTime;
	tmpres <<= 32;
	tmpres |= ft.dwLowDateTime;
	/*converting file time to unix epoch*/
	tmpres -= DELTA_EPOCH_IN_MICROSECS;
	tmpres /= 10;  /*convert into microseconds*/
	tv.tv_sec = (long)(tmpres / 1000000UL);
	tv.tv_usec = (long)(tmpres % 1000000UL);
	t = tv.tv_sec;

	GetSystemTime(&lt);
	int r = GetDateFormatEx(
		LOCALE_NAME_USER_DEFAULT, 
		0,
		&lt, 
		_T("yyyy/MM/dd"), 
		buf, 
		sizeof(buf) / sizeof(buf[0]), 
		NULL);
	if (r == 0)
	{
		DWORD last_error = GetLastError();
		wrapper_error_t* error = wrapper_error_from_system(last_error, "");
		if (error)
		{
			wrapper_error_free(error);
		}
	}

	int s = GetTimeFormatEx(
		LOCALE_NAME_USER_DEFAULT,
		0,
		&lt,
		_T("HH:mm:ss"),
		time,
		sizeof(time) / sizeof(time[0]));
	if (s == 0)
	{
		DWORD last_error = GetLastError();
		wrapper_error_t* error = wrapper_error_from_system(last_error, "");
		if (error)
		{
			wrapper_error_free(error);
		}
	}

	TCHAR* path = (TCHAR*)user_data;

	_tfopen_s(&stream, path, _T("at+"));
	if (stream)
	{
		pid = (int)GetProcessId(NULL);
		_ftprintf(stream,
			_T("%s %s: [%5d]: %8s: %12s: %s\n"),
			buf,
			time,
			pid,
			wrapper_log_level_str(log_level),
			log_domain,
			message);

		fclose(stream);
	}
}
