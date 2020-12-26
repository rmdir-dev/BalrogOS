#pragma once
#include <stddef.h>

void terminal_writestring(const char* data);

void terminal_write(const char* data, size_t size);

void terminal_initialize(void);