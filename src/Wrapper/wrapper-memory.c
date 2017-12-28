// Copyright (c) Werner Strydom. All rights reserved.
// Licensed under the MIT license. See LICENSE in the project root for license information.

#include "stdafx.h"
#include "wrapper-memory.h"


void* wrapper_allocate(size_t size)
{
	return LocalAlloc(LPTR, size);
}

void wrapper_free(void* p)
{
	LocalFree(p);
}

TCHAR* wrapper_allocate_string(size_t size)
{
	return (TCHAR*)wrapper_allocate(size * sizeof(TCHAR));
}
