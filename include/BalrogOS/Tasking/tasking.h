#pragma once

#include <stdint.h>
#include "BalrogOS/Memory/vmm.h"

#define PROCESS_STATE_ALIVE     0
#define PROCESS_STATE_ZOMBIE    1
#define PROCESS_STATE_DEAD      2

typedef struct process_t
{
    struct process_t* prev;
    char* name;
    uintptr_t pid;
    uintptr_t rsp;
    uintptr_t rip;
    uintptr_t stack_top;
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

process* create_process(char* name, uintptr_t func);