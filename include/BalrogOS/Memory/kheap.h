#pragma once
#include <stddef.h>

typedef struct block_info_t
{
    struct block_info_t* previous_chunk; // previous block
    struct
    {
        uint32_t _size : 29;        // block size
        uint32_t _non_arena : 1;    // for threading
        uint32_t _is_mmapped : 1;   // if the block is allocated
        uint32_t _present : 1;      // previous block is free
    } __attribute__((packed));
    struct block_info_t* next_free; // next block free
}__attribute__((packed)) block_info;

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