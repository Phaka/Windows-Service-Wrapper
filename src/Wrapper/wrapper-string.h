#pragma once
#include "wrapper-error.h"

void wrapper_string_trim_right(TCHAR* chars); 
int wrapper_string_duplicate(TCHAR **result, TCHAR* source, wrapper_error_t** error);
void wrapper_string_copy(TCHAR* destination, const int destination_max_size, TCHAR* source);