#pragma once
#include <stdint.h>

/**
 * @brief initialize the scheduler.
 * 
 */
void init_scheduler();

/**
 * @brief add a new process to a scheduling queue
 * 
 * @param name name of the process
 * @param func starting function (address) of the process
 * @param mode KERNEL MODE = 0 USER MODE = 3
 * @return uintptr_t return process PID
 */
uintptr_t push_process(char* name, uintptr_t func, uint8_t mode);