#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int printf(const char* __restrict format, ...);
int putchar(int c);
int puts(const char* str);

#ifdef __cplusplus
}
#endif