#pragma once
#include <stddef.h>

void init_kernel_heap();

void* vmalloc(size_t size);