#pragma once

#include <stdint.h>
#include "BalrogOS/Memory/vmm.h"
#include "BalrogOS/FileSystem/filesystem.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"

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
    int pid;
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
    uint8_t wait_size;
    int waiting[5];
    uint8_t child;
    uint8_t fd_size;
    fs_fd fd_table[10];
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

int clean_process(process* proc);

/**
 * @brief 
 * 
 * @param proc 
 * @return int 
 */
int fork_process(process* proc, interrupt_regs* regs);

/**
 * @brief 
 * 
 * @param name 
 * @param argv 
 * @param kill 
 * @return int 
 */
int exec_process(const char* name, char** argv, uint8_t kill);

/**
 * @brief 
 * 
 * @param pid_to_wait 
 * @return int 
 */
int wait_process(int pid_to_wait);