#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Memory/memory.h"
#include "BalrogOS/Memory/pmm.h"
#include "BalrogOS/Debug/debug_output.h"

#define KERNEL_VIRTUAL_START    0xffff800010000000
#define KERNEL_VIRTUAL_TOP      0xfffffbffefffffff

uintptr_t* kheap_start;
uintptr_t* kheap_end;

typedef struct block_info_t
{
    struct block_info_t* previous_chunk;
    struct
    {
        uint16_t _size : 13;
        uint16_t _non_arena : 1;
        uint16_t _is_mmapped : 1;
        uint16_t _present : 1;
    } __attribute__((packed));
}__attribute__((packed)) block_info;

struct free_block_info_t;

struct free_block_t
{
    struct free_block_info_t* next_free;
    struct free_block_info_t* previous_free;
} __attribute__((packed));

typedef struct free_block_info_t
{
    struct block_info_t info;
    struct free_block_t free_blocks;
} __attribute__((packed)) free_block_info;

void init_kernel_heap()
{
    kheap_start = KERNEL_VIRTUAL_START;
    uintptr_t alloc = pmm_calloc();
    vmm_set_page(0, kheap_start, alloc, PAGE_PRESENT | PAGE_WRITE);

    free_block_info block;
    block.info.previous_chunk = 0;
    block.info._is_mmapped = 0;
    block.info._non_arena = 0;
    block.info._present = 0;
    block.info._size = 0x1000 - sizeof(block_info);
    block.free_blocks.previous_free = 0;
    block.free_blocks.next_free = KERNEL_VIRTUAL_TOP;

    free_block_info* first_block = kheap_start;
    *first_block = block;

    KERNEL_LOG_INFO("prev chunk : %p", first_block->info.previous_chunk);
    KERNEL_LOG_INFO("is mmapped : %d", first_block->info._is_mmapped);
    KERNEL_LOG_INFO("non arena : %d", first_block->info._non_arena);
    KERNEL_LOG_INFO("present : %d", first_block->info._present);
    KERNEL_LOG_INFO("size : %d", first_block->info._size);

    KERNEL_LOG_INFO("prev block : %p", first_block->free_blocks.previous_free);
    KERNEL_LOG_INFO("next block : %p", first_block->free_blocks.next_free);
}

void* vmalloc(size_t size)
{
    return 0;
}