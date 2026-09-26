#pragma once
#include <stdint.h>
#include "klib/data_structure/queue.h"

/**
 * @brief initialize the scheduler.
 * 
 */
int init_scheduler();

/**
 * @brief add a new process to a scheduling queue
 * 
 * @param name name of the process
 * @param func starting function (address) of the process
 * @param mode KERNEL MODE = 0 USER MODE = 3
 * @return uintptr_t return process PID
 */
uintptr_t push_process(char* name, uintptr_t func, uint8_t mode);

static inline __attribute__((always_inline)) queue_t* sched_get_kstack_queue()
{
    extern queue_t kstack_to_clean;
    return &kstack_to_clean;
}
