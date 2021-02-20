#pragma once
#include <stdint.h>

/**
 * @brief add the current process into the waiting queue
 * 
 */
void park();

/**
 * @brief wake a waiting process.
 * 
 * @param pid process ID of the process we want to wake up
 */
void unpark(uint64_t pid);