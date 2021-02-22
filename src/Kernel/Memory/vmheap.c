#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Memory/memory.h"
#include "BalrogOS/Memory/pmm.h"
#include "BalrogOS/Memory/vmm.h"
#include "BalrogOS/Debug/debug_output.h"

/*
Kernel heap start at KERNEL_OFFSET + 0x2000000000 (128GiB)
Total kernel heap = 512 GiB - 128GiB = 384GiB
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
    struct block_info_t* next_free;
}__attribute__((packed)) block_info;

void init_vmheap()
{
    vmheap_start = KERNEL_VIRTUAL_START;
    first_free = vmheap_start;
    vmheap_current_top = vmheap_start + 0x1000;
    uintptr_t alloc = pmm_calloc();
    vmm_set_page(0, vmheap_start, alloc, PAGE_PRESENT | PAGE_WRITE);

    block_info block;
    block.previous_chunk = 0;
    block._is_mmapped = 0;
    block._non_arena = 0;
    block._present = 0;
    block._size = 0x1000 - sizeof(block_info);
    block.next_free = vmheap_current_top;

    block_info* first_block = vmheap_start;
    *first_block = block;
}

void* vmalloc(size_t size)
{
    block_info* current_block = first_free;
    uint8_t first_block = 1;
    while(1)
    {
        /*  if the next block is smaller than the current top
            then try to allocate the new block
        */
        if(current_block < (block_info*) vmheap_current_top)
        {
            /*  check if the block is large enough 
                if yes allocate the new block of memory here.
            */
            if(current_block->_size >= size)
            {
                // if the previous block is free of not
                uint8_t present = 1;
                
                // the current block is by default the block to be allocated.
                // we use a uint8_t to be able to shift the pointer byte by byte
                uint8_t* block = current_block;

                // check if the current block will be able to contain a new free block
                if(current_block->_size < size + sizeof(block_info))
                {
                    // if no the new first free block will be the next one. if first block != 0
                    if(first_block)
                    {
                        first_free = current_block->next_free;
                    }
                    /*  the size is equal to the size of the current block.
                        else we would lose memory
                    */
                    size = current_block->_size;
                    // the previous block is not free.
                    present = 0;    
                } else
                {
                    /*  if the block can contain the new block then
                        change the current block size.
                    */
                    size_t new_block_size = size + sizeof(block_info);
                    current_block->_size -= new_block_size;
                    //move the block pointer to the new block we want to allocate
                    block += current_block->_size + sizeof(block_info);
                }

                // get the new block into structure at block
                block_info* new_block = block;

                // set the new block structure
                new_block->previous_chunk = current_block;
                new_block->_present = present;
                new_block->_non_arena = 0;
                new_block->_size = size;
                new_block->_is_mmapped = 1;

                // set current block to the next block.
                current_block = block + sizeof(block_info) + size;

                // check if the current top is not the virtual heap top
                if(current_block < (block_info*) vmheap_current_top)
                {
                    // if not then set the previous block to block and the present bit to 0
                    current_block->previous_chunk = block;
                    current_block->_present = 0;
                }

                // return the address of the newly allocated block
                return block + sizeof(block_info);
            }
            // current block = next block
            current_block = current_block->next_free;
        } else 
        {
            // if the current virtual heap top is equal to KERNEL_VIRTUAL_TOP
            // then we don't have any space left in memory.
            // We also don't allow to allocate a block larger than 4086 bytes atm.
            if(vmheap_current_top == KERNEL_VIRTUAL_TOP || size > 4086)
            {
                //Kernel heap full
                return 0;
            }

            // get a new page
            uintptr_t alloc = pmm_calloc();
            // set the new page to the vmheap_current_top address
            vmm_set_page(0, vmheap_current_top, alloc, PAGE_PRESENT | PAGE_WRITE);            

            // set the first block address to vmheap_current_top
            block_info* first_block = vmheap_current_top;

            // set the new first block data.
            first_block->previous_chunk = current_block;
            first_block->_is_mmapped = 0;
            first_block->_non_arena = 0;
            first_block->_present = 0; // Base address of a page can't be coalesce
            first_block->_size = 0x1000 - sizeof(block_info);
            first_block->next_free = vmheap_current_top + 0x1000;

            // set the new virtual heap top.
            vmheap_current_top += 0x1000;
        }
        // we're not on the first block anymore.
        first_block = 0;
    }

    return 0;
}

void vmfree(void* ptr)
{
    block_info* block = ptr - sizeof(block_info);
    block_info* next_block =  ptr + block->_size;

    if(!block->_is_mmapped)
    {
        printf("double free()");
        while (1)
        {
            /* code */
        }
    }
    block->_is_mmapped = 0;

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
        if(next_block < (block_info*) vmheap_current_top)
        {
            // if the next block is not mapped
            if(!next_block->_is_mmapped)
            {
                // if block is smaller than 4096 bytes && contiguous
                // then coalesce the two blocks
                if(block->_size + sizeof(block_info) < 0x1000 && contiguous)
                {
                    // set the new free block
                    block_info* new_free_block = block;

                    new_free_block->previous_chunk = block->previous_chunk;
                    new_free_block->_non_arena = block->_non_arena;
                    new_free_block->_is_mmapped = block->_is_mmapped;
                    new_free_block->_present = block->_present;
                    new_free_block->_size += next_block->_size + sizeof(block_info);

                    new_free_block->next_free = next_block->next_free;

                } else 
                {
                    // set the next free block.
                    block_info* free_block = block;
                    free_block->next_free = next_block;
                    break;
                }
            }
            // set contiguous to 0 and shift to the next block
            contiguous = 0;
            next_block = ((uint8_t*)next_block) + (next_block->_size + sizeof(block_info));
        } else
        {
            // the next free block is the virtual kernel heap top.
            block_info* free_block = block;
            free_block->next_free = vmheap_current_top;
            next_block = free_block;
            break;
        }
    }

    // if the freed block is smaller than fist_free then 
    // set first free to block.
    if((block_info*) first_free > block)
    {
        first_free = block;
    }
}