#pragma once

#include <stdint.h>
#include "balrog_os/tasking/tasking.h"

typedef struct __cpu_state_t
{
    /* gs:0 points at the struct itself, so taking the address of a cpu state field
       is a single read instead of a msr round trip. */
    struct __cpu_state_t* this;
    process* current_running;
    uint32_t lapic_id;
} cpu_state_t;

/**
 * @brief the process running on this cpu
 *
 * @return process*
 */
static inline __attribute__((always_inline)) process* get_current_process()
{
    extern process* current_running;
    return current_running;
}

/**
 * @brief set the process running on this cpu
 *
 * @param proc
 */
static inline __attribute__((always_inline)) void set_current_process(process* proc)
{
    extern process* current_running;
    current_running = proc;
}
