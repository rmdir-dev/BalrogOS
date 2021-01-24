#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void abort();
void free(void* pointer);
void* malloc(size_t memory_size);

#ifdef __cplusplus
}
#endif