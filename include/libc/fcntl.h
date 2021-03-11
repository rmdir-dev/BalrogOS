#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 
 * 
 * @param pathname 
 * @param flags 
 * @return int 
 */
int open(const char* pathname, int flags);

/**
 * @brief 
 * 
 * @param fd 
 * @param buf 
 * @param count 
 * @return size_t 
 */
size_t read(int fd, void* buf, size_t count);

/**
 * @brief 
 * 
 * @param fd 
 * @param buf 
 * @param count 
 * @return size_t 
 */
size_t write(int fd, void* buf, size_t count);

/**
 * @brief 
 * 
 * @param fd 
 * @return int 
 */
int close(int fd);

#ifdef __cplusplus
}
#endif