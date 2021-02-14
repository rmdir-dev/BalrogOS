#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 
 * 
 * @param addr_1 
 * @param addr_2 
 * @param byte_size 
 * @return int 
 */
int memcmp(const void* addr_1, const void* addr_2, size_t byte_size);

/**
 * @brief 
 * 
 * @param dest 
 * @param source 
 * @param size 
 * @return void* 
 */
void* memcpy(void* __restrict dest, const void* __restrict source, size_t size);

/**
 * @brief 
 * 
 * @param dest 
 * @param source 
 * @param size 
 * @return void* 
 */
void* memmove(void* dest, const void* source, size_t size);

/**
 * @brief 
 * 
 * @param pointer 
 * @param value 
 * @param count 
 * @return void* 
 */
void* memset(void* pointer, int value, size_t count);

/**
 * @brief 
 * 
 * @param str 
 * @return size_t 
 */
size_t strlen(const char* str);

#ifdef __cplusplus
}
#endif