#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 
 * 
 */
void abort();

/**
 * @brief 
 * 
 * @param pointer 
 */
void free(void* pointer);

/**
 * @brief 
 * 
 * @param memory_size 
 * @return void* 
 */
void* malloc(size_t memory_size);

#define NULL ((void *)0)

#ifdef __cplusplus
}
#endif