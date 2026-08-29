#pragma once
#include <stddef.h>
#include <stdint.h>

// ull -> unsigned long ! 1u => 32bits ^^ so 4GiB not the right size.
#define HEAP_MAX_BLOCK_SIZE ((1ull << 60) - 1)

typedef struct block_info_t
{
    struct block_info_t* previous_chunk; // previous block
    struct
    {
        uint64_t _size : 60;        // block size
        uint64_t _non_arena : 1;    // for threading
        uint64_t _is_mmapped : 1;   // if the block is allocated
        uint64_t _present : 1;      // previous block is free
        uint64_t _full : 1;      // the block was not split, _size is larger than asked
    } __attribute__((packed));
    struct block_info_t* next_free; // next block free
}__attribute__((packed)) block_info;

/**
 * @brief allocate a block inside a free block of any heap.
 *
 * @param size size in byte, header not included
 * @param current_block the free block we try to allocate into
 * @param prev_block the previous free block of the free list
 * @param current_top top of the heap
 * @param first_free address of the first free block of the heap
 * @param first_block 1 if current_block is the first block of the free list
 * @return void* the address of the newly allocated block, 0 if it doesn't fit
 */
void* heap_alloc(size_t size, block_info* current_block, block_info* prev_block, block_info* current_top, uintptr_t* first_free, uint8_t first_block);

/**
 * @brief free a block of any heap and coalesce it with its free neighbours.
 *
 * @param block the block to free
 * @param next_block the block right after the one we free
 * @param current_top top of the heap
 * @param first_free address of the first free block of the heap
 * @param block_max_size a coalesced block can't get larger than this
 */
void heap_free(block_info* block, block_info* next_block, block_info* current_top, uintptr_t* first_free, uint64_t block_max_size);
