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

/**
 * @brief
 *
 * @param str
 * @return int
 */
int atoi(const char* str);

/**
 * @brief Execute a command.
 *
 * @param command
 */
void system(const char* command);

#ifdef __cplusplus
}
#endif