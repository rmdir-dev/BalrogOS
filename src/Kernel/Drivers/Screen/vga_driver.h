#pragma once

#include <stddef.h>

void vga_init();

void vga_write(const char* data, size_t size);