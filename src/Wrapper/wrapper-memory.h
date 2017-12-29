// Copyright (c) Werner Strydom. All rights reserved.
// Licensed under the MIT license. See LICENSE in the project root for license information.

#pragma once

void* wrapper_allocate(size_t size);
void wrapper_free(void* p);
TCHAR* wrapper_allocate_string(size_t size);
