#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 
 * 
 * @param format 
 * @param ... 
 * @return int 
 */
int kprint(const char* __restrict format, ...);

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