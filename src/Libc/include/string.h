#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int memcmp(const void* addr_1, const void* addr_2, size_t byte_size);
void* memcpy(void* __restrict dest, const void* __restrict source, size_t size);
void* memmove(void* dest, const void* source, size_t size);
void* memset(void* pointer, int value, size_t count);
size_t strlen(const char* str);

#ifdef __cplusplus
}
#endif