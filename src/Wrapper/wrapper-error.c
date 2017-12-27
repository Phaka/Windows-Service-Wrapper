#include "stdafx.h"
#include "wrapper-error.h"
#include "wrapper-log.h"

#define ERRMSGBUFFERSIZE 256

HLOCAL wrapper_error_format_message(DWORD dwErrorMsgId)
{
	DWORD rc = 0;
	HINSTANCE instance = NULL;
	HLOCAL buffer = NULL;

	if (HRESULT_FACILITY(dwErrorMsgId) == FACILITY_MSMQ)
	{
		instance = LoadLibrary(TEXT("MQUTIL.DLL"));
		if (instance != 0)
		{
			rc = FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_HMODULE |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				instance,
				dwErrorMsgId,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR)&buffer,
				ERRMSGBUFFERSIZE,
				NULL
			);
		}
	}
	else if (dwErrorMsgId >= NERR_BASE && dwErrorMsgId <= MAX_NERR)
	{
		instance = LoadLibrary(TEXT("NETMSG.DLL"));
		if (instance != 0)
		{
			rc = FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_HMODULE |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				instance,
				dwErrorMsgId,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR)&buffer,
				ERRMSGBUFFERSIZE,
				NULL
			);
		}
	}
	else
	{
		rc = FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dwErrorMsgId,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&buffer,
			ERRMSGBUFFERSIZE,
			NULL
		);
	}
	return buffer;
}


wrapper_error_t *wrapper_error_from_system(DWORD code, TCHAR* format, ...)
{
	va_list args;
	wrapper_error_t* error = NULL;
	error = LocalAlloc(LPTR, sizeof(wrapper_error_t));
	if (error)
	{
		error->code = HRESULT_FROM_WIN32(code);
		error->message = wrapper_error_format_message(code);
		error->user_message = LocalAlloc(LPTR, 4096*sizeof(TCHAR));
		va_start(args, format);
		_vsntprintf_s(error->user_message, 4096, _TRUNCATE, format, args);
		va_end(args);
	}
	return error;
}

wrapper_error_t *wrapper_error_from_hresult(long code, TCHAR* format, ...)
{
	va_list args;
	wrapper_error_t* error = NULL;
	error = LocalAlloc(LPTR, sizeof(wrapper_error_t));
	if (error)
	{
		error->code = code;
		error->message = wrapper_error_format_message(code);
		error->user_message = LocalAlloc(LPTR, 4096);
		va_start(args, format);
		_vsntprintf_s(error->user_message, 4096, _TRUNCATE, format, args);
		va_end(args);
	}
	return error;
}

void wrapper_error_free(wrapper_error_t * error)
{
	if (error)
	{
		LocalFree(error->message);
		LocalFree(error->user_message);
		LocalFree(error);
	}
}

void wrapper_error_log(wrapper_error_t * error)
{
	if (error)
	{
		WRAPPER_ERROR(_T("%s [0x%08x %s]"), error->user_message, error->code, error->message);
	}
}