// Copyright (c) Werner Strydom. All rights reserved.
// Licensed under the MIT license. See LICENSE in the project root for license information.

#include "stdafx.h"
#include "wrapper-string.h"
#include "wrapper-memory.h"
#include "wrapper-error.h"

void wrapper_string_trim_right(TCHAR* chars)
{
	for (size_t i = _tcslen(chars); i > 0; i--)
	{
		TCHAR c = chars[i-1];
		if (!_istspace(c))
		{
			return;
		}
		chars[i-1] = 0;
	}
}

void wrapper_string_copy(TCHAR* destination, const size_t destination_max_size, TCHAR* source)
{
	StringCbCopy(destination, destination_max_size * sizeof(TCHAR), source);
}

int wrapper_string_duplicate(TCHAR** result, TCHAR* source, wrapper_error_t** error)
{
	const size_t length = _tcslen(source) + 1;
	*result = wrapper_allocate_string(length);
	if (!*result)
	{
		if (error)
		{
			*error = wrapper_error_from_hresult(E_OUTOFMEMORY, _T(""));
		}
	}
	else
	{
		wrapper_string_copy(*result, length, source);
	}
	return *result != NULL;
}
