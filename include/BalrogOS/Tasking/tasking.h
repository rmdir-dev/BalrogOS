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
#define PROCESS_STATE_SLEEPING  5

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
    uint8_t forked_memory;
    char* cwd;
    uint32_t uid;
    uint32_t gid;
    uint32_t* error_no;
    struct process_t* next;
    struct process_t* parent;
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

int clean_process(process* proc, uint8_t clean_memory);

/**
 * @brief 
 * 
 * @param proc 
 * @return int 
 */
int fork_process(process* proc, interrupt_regs* regs);

/**
 * @brief Copy the memory of a process into its forked_memory child.
 *        This function should only be called after recieving
 *        a write/read exception from the child process.
 *
 * @param proc forked_memory process
 */
void copy_forked_process_memory(process* proc, uintptr_t copy_addrs);

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