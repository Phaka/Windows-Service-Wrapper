// Copyright (c) Werner Strydom. All rights reserved.
// Licensed under the MIT license. See LICENSE in the project root for license information.

#pragma once
#include "wrapper-error.h"

void wrapper_string_trim_right(TCHAR* chars);
int wrapper_string_duplicate(TCHAR** result, TCHAR* source, wrapper_error_t** error);
void wrapper_string_copy(TCHAR* destination, const size_t destination_max_size, TCHAR* source);
