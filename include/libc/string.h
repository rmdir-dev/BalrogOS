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

/**
 * @brief Copy the content of source to dest.
 * 
 * @param dest 
 * @param source 
 * @return char* 
 */
char* strcpy(void* __restrict dest, const void* __restrict source);

/**
 * @brief Breaks string str into a series of tokens using the delimiter delim.
 * 
 * @param str 
 * @param delim 
 * @return char* 
 */
char* strtok(char * __restrict str, const char delim);

/**
 * @brief Returns a pointer to a null-terminated byte string, 
 * which is a duplicate of the string pointed to by str1. 
 * The returned pointer must be passed to free to avoid a memory leak.
 * 
 * @param str 
 * @return char* 
 */
char* strdup(char* str1);

/**
 * @brief Compare two strings
 * 
 * @param first 
 * @param second 
 * @return int 
 */
int strcmp(const char* first, const char* second);

/**
 * @brief 
 * 
 * @param str 
 * @param delim 
 * @return uintptr_t 
 */
uintptr_t strspn(char * __restrict str, const char delim);

/**
 * @brief 
 * 
 * @param str 
 * @param delim 
 * @return uintptr_t 
 */
uintptr_t strcspn(char * __restrict str, const char delim);

#ifdef __cplusplus
}
#endif