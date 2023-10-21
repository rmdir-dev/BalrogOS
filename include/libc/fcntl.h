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

#pragma region open flags

#define O_ACCMODE       00000003
#define O_RDONLY        00000000
#define O_WRONLY        00000001
#define O_RDWR          00000002

#pragma endregion

#ifdef __cplusplus
}
#endif