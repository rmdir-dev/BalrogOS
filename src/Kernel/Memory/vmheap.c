#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Memory/memory.h"
#include "BalrogOS/Memory/pmm.h"
#include "BalrogOS/Debug/debug_output.h"

/*
Kernel heap start at KERNEL_OFFSET + 0x2000000000 (128GiB)
Total kernel heap = 512 GiB - 128GiB = 384
*/

#define KERNEL_VIRTUAL_START    0xffffffa000000000
#define KERNEL_VIRTUAL_TOP      0xffffffffffffffff

uintptr_t vmheap_start;
uintptr_t vmheap_current_top;
uintptr_t first_free;
uintptr_t vmheap_end;

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

void init_vmheap()
{
    vmheap_start = KERNEL_VIRTUAL_START;
    first_free = vmheap_start;
    vmheap_current_top = vmheap_start + 0x1000;
    uintptr_t alloc = pmm_calloc();
    vmm_set_page(0, vmheap_start, alloc, PAGE_PRESENT | PAGE_WRITE);

    free_block_info block;
    block.info.previous_chunk = 0;
    block.info._is_mmapped = 0;
    block.info._non_arena = 0;
    block.info._present = 0;
    block.info._size = 0x1000 - sizeof(block_info);
    block.free_blocks.previous_free = KERNEL_VIRTUAL_START;
    block.free_blocks.next_free = vmheap_current_top;

    free_block_info* first_block = vmheap_start;
    *first_block = block;
}

void* vmalloc(size_t size)
{
    free_block_info* next_block = first_free;
    
    while(1)
    {
        if(next_block < vmheap_current_top)
        {
            if(next_block->info._size >= size)
            {
                uint8_t present = 1;
                
                uint8_t* block = next_block;

                if(next_block->info._size == size)
                {
                    first_free = next_block->free_blocks.next_free;
                    present = 0;
                } else
                {
                    size_t new_block_size = size + sizeof(block_info);
                    next_block->info._size -= new_block_size;
                    block += next_block->info._size + sizeof(block_info);
                }

                block_info* new_block = block;

                new_block->previous_chunk = next_block;
                new_block->_present = present;
                new_block->_non_arena = 0;
                new_block->_size = size;
                new_block->_is_mmapped = 1;

                return block + sizeof(block_info);
            }
            next_block = next_block->free_blocks.next_free;
        } else 
        {
            if(vmheap_current_top == KERNEL_VIRTUAL_TOP || size > 4086)
            {
                //Kernel heap full
                return 0;
            }

            uintptr_t alloc = pmm_calloc();
            vmm_set_page(0, vmheap_current_top, alloc, PAGE_PRESENT | PAGE_WRITE);

            free_block_info block;
            block.info.previous_chunk = next_block;
            block.info._is_mmapped = 0;
            block.info._non_arena = 0;
            block.info._present = 0; // Base address of a page can't be coalesce
            block.info._size = 0x1000 - sizeof(block_info);
            block.free_blocks.previous_free = next_block;
            block.free_blocks.next_free = vmheap_current_top + 0x1000;

            free_block_info* first_block = vmheap_current_top;

            *first_block = block;

            vmheap_current_top += 0x1000;
        }
    }

    return 0;
}

void vmfree(void* ptr)
{
    block_info* block = ptr - sizeof(block_info);
    free_block_info* next_block =  ptr + block->_size;

    //printf("block size : %d | nb addr : %p | vmh top : %p\n", block->_size, next_block, vmheap_current_top);
    // if previous block is present
    // then coalesce the two blocks
    if(block->_present)
    {
        block->previous_chunk->_size += block->_size + sizeof(block_info);
        block = block->previous_chunk;
    }
    // if next block < current heap top addr
    uint8_t contiguous = 1;

    while(1)
    {
        if(next_block < vmheap_current_top)
        {
            // if the next block is mapped
            if(!next_block->info._is_mmapped)
            {
                // if block is smaller than 4096 bytes && contiguous
                // then coalesce the two blocks
                if(block->_size + sizeof(block_info) < 0x1000 && contiguous)
                {
                    free_block_info free;
                    free.info = *block;
                    free.info._size += next_block->info._size + sizeof(block_info); 

                    free.free_blocks = next_block->free_blocks;
                    if(free.free_blocks.next_free < vmheap_current_top)
                    {
                        free.free_blocks.next_free->free_blocks.previous_free = block;
                    }
                    free.free_blocks.previous_free->free_blocks.next_free = block;

                    free_block_info* new_free_block = block;
                    *new_free_block = free;
                } else 
                {
                    free_block_info* free_block = block;
                    free_block->free_blocks.next_free = next_block;
                    break;
                }
            }
            contiguous = 0;
            next_block = ((uint8_t*)next_block) + (next_block->info._size + sizeof(block_info));
        } else
        {
            free_block_info* free_block = block;
            free_block->free_blocks.next_free = vmheap_current_top;
            break;
        }
    }

    if(first_free > block)
    {
        first_free = block;
    }
}