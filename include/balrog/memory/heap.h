#pragma once

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