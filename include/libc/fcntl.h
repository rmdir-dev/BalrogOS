#pragma once

#include <stddef.h>
#include <unistd.h>

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

#ifdef __cplusplus
}
#endif