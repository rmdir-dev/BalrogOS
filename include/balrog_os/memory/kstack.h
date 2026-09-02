#pragma once

#include <stdint.h>

/*
    KSTACK Range
    0xffffffe000000000 -> 0xffffffffffffffff    128GiB

    kernel stack size (per process) : 4 * 4096 = 16KiB

    TODO later : dynamicly allocate the stack using page fault.

    Use the kernel PML4T
    512PDPT * 512PDT * 512PT = 512GiB
    kernel process stack start at :
    PDPT index = 384
    use one PT/process (2MiB max)
    128 * 512 = 65536 process max
*/

/**
 * @brief Allocate a 2MiB kernel stack
 * Use this to create a new process kernel stack
 * Allocate one Page Table as stack per process
 * So the max size of the kernel stack is 2MiB
 * Currently allocate only 24KiB.
 * Later we will have a dynamically allocated stack
 * So this will allocate one page and then we use page faults
 * to add more pages of memory.
 * 
 * @return void* the stack top address
 */
void* kstack_alloc();

/**
 * @brief free the process kernel stack
 * 
 * @param addr kernel stack address.
 */
void kstack_free(uintptr_t* addr);