#pragma once

#include <stddef.h>
#include <stdarg.h>

/**
 * @brief
 *
 * @param out
 * @param maxsize
 * @param format
 * @param parameters
 * @return
 */
int ksprint(char* out, size_t maxsize, const char* format, ...);

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
 * @param str 
 * @return int 
 */
int kputs(const char* str);
