#pragma once

#include <stdint.h>
#include "BalrogOS/Memory/vmm.h"

#define PROCESS_STATE_READY     0
#define PROCESS_STATE_RUNNING   1
#define PROCESS_STATE_WAITING   2
#define PROCESS_STATE_DEAD      3
#define PROCESS_STATE_ZOMBIE    4

#define PROCESS_USER_MODE       3
#define PROCESS_KERNEL_MODE     0

typedef struct process_t
{
    struct process_t* prev;
    char* name;
    uint64_t pid;
    uintptr_t rsp;
    uintptr_t rip;
    uintptr_t stack_top;
    uintptr_t kernel_stack_top;
    uint8_t exec;
    union 
    {
        /*
            PML4T physical address.
        */
        page_table* PML4T;
        /*
            cr3 register content
        */
        uintptr_t cr3;
    };    
    uint8_t state;
    struct process_t* next;
} process;

/**
 * @brief Create a process object
 * 
 * @param name process name
 * @param func starting function
 * @param mode User Mode = 3 | Driver Mode = 2 or 1 | Kernel Mode = 0
 * @return process* 
 */
process* create_process(char* name, uintptr_t func, uint8_t mode);