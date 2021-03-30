#pragma once
#include <stddef.h>
#include "balrog/memory/heap.h"

/**
 * @brief initialize the virtual kernel heap
 * 
 */
void init_vmheap();

/**
 * @brief alloc a number of byte in the virtual heap.
 * 
 * @param size size in byte of the 
 * @return void* address of the newly allocated space.
 */
void* vmalloc(size_t size);

/**
 * @brief free an address in the virtual heap (if it was allocated)
 * 
 * @param ptr address
 */
void vmfree(void* ptr);

/**
 * @brief initialize the logical kernel heap
 * 
 */
void init_kheap();

/**
 * @brief allocate a block of memory in the logical heap
 * 
 * @param size size in byte
 * @return void* the address of the newly allocated block
 */
void* kmalloc(size_t size);

/**
 * @brief free a block of memory in the logical heap.
 * 
 * @param ptr address
 */
void kfree(void* ptr);