#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void abort();
void free(void* pointer);
void* malloc(size_t memory_size);

#define NULL    0

#ifdef __cplusplus
}
#endif