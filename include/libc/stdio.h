#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * @brief 
 * 
 * @param format 
 * @param ... 
 * @return int 
 */
int printf(const char* __restrict format, ...);

/**
 * @brief 
 * 
 * @param c 
 * @return int 
 */
int putchar(int c);

/**
 * @brief 
 * 
 * @param str 
 * @return int 
 */
int puts(const char* str);

#ifdef __cplusplus
}
#endif