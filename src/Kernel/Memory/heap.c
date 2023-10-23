#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Memory/memory.h"
#include "BalrogOS/Memory/pmm.h"
#include "BalrogOS/Debug/debug_output.h"
#include "balrog/terminal/term.h"

void* alloc(size_t size, block_info* current_block, block_info* prev_block, block_info* current_top, uintptr_t* first_free, uint8_t first_block)
{
    kernel_debug_output(KDB_LVL_VERBOSE, "allocating %d bytes at 0%p", size, current_block);
    /*  check if the block is large enough 
        if yes allocate the new block of memory here.
    */
    if(current_block->_size >= size)
    {
        kernel_debug_output(KDB_LVL_VERBOSE, "block is large enough");
        // if the previous block is free of not
        uint8_t present = 1;
        uint8_t full = 0;
        
        // the current block is by default the block to be allocated.
        // we use a uint8_t to be able to shift the pointer byte by byte
        uint8_t* block = (void*)current_block;

        // check if the current block will be able to contain a new free block
        if(current_block->_size <= size + sizeof(block_info))
        {
            kernel_debug_output(KDB_LVL_VERBOSE, "block is not large enough : %d < %d", current_block->_size, size + sizeof(block_info));
//            return 0;
            // if no the new first free block will be the next one. if first block != 0
            if(first_block && current_block == *first_free)
            {
                kernel_debug_output(KDB_LVL_VERBOSE, "first free block is 0%p -> %p", *first_free, current_block->next_free);
                *first_free = (uintptr_t)current_block->next_free;
            } else
            {
                kernel_debug_output(KDB_LVL_VERBOSE, "first free block is 0%p", prev_block->next_free);
                prev_block->next_free = current_block->next_free;
            }
            /*  the size is equal to the size of the current block.
                else we would lose memory
            */
            kernel_debug_output(KDB_LVL_VERBOSE, "size = %d", current_block->_size);
            size = current_block->_size;

            // the previous block is not free.
            present = 0;
            full = 1;
        } else
        {
            /*  if the block can contain the new block then
                change the current block size.
            */
            uint32_t new_block_size = size + sizeof(block_info);
            kernel_debug_output(KDB_LVL_VERBOSE, "alloc block 0%p | size : %d -> %d", current_block, current_block->_size, current_block->_size - new_block_size);
            current_block->_size -= new_block_size;

            //move the block pointer to the new block we want to allocate
            block += current_block->_size + sizeof(block_info);
            kernel_debug_output(KDB_LVL_VERBOSE, "block = 0%p", block);
        }

        // get the new block into structure at block
        block_info* new_block = (void*)block;

        kernel_debug_output(KDB_LVL_VERBOSE, "new block = 0%p | top = 0%p", new_block, current_top);
        // set the new block structure
        new_block->previous_chunk = current_block;
        new_block->_present = present;
        new_block->_non_arena = 0;
        new_block->_size = size;
        new_block->_full = full;
        new_block->_is_mmapped = 1;

        // set current block to the next block.
        current_block = (void*)(block + sizeof(block_info) + size);
        kernel_debug_output(KDB_LVL_VERBOSE, "current block = 0%p", current_block);

        // check if the current top is not the virtual heap top
        if(current_block < current_top)
        {
            kernel_debug_output(KDB_LVL_VERBOSE, "current block is not the virtual heap top");
            // if not then set the previous block to block and the present bit to 0
            current_block->previous_chunk = (void*)block;
            current_block->_present = 0;
        }

        kernel_debug_output(KDB_LVL_VERBOSE, "returning 0%p", block + sizeof(block_info));
        // return the address of the newly allocated block
        return block + sizeof(block_info);
    }

    return 0;
}

void free(block_info* block, block_info* next_block, block_info* current_top, uintptr_t* first_free, uint64_t block_max_size)
{
    if(!block->_is_mmapped)
    {
        kernel_debug_output(KDB_LVL_ERROR, "double vmfree() 0%p", block);
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
        uint32_t size = block->_size + sizeof(block_info);
//        if(block->_full) {
//            size = block->_size;
//        }
        kernel_debug_output(KDB_LVL_VERBOSE, "free 2 block = 0%p, prev chunk 0%p", block, block->previous_chunk);
        block->previous_chunk->_size += size;
        block = block->previous_chunk;
    }
    // if next block < current heap top addr
    uint8_t contiguous = 1;

    while(1)
    {
        if(next_block < (block_info*) current_top)
        {
            // if the next block is not mapped
            if(!next_block->_is_mmapped)
            {
                // if block is smaller than block_max_size bytes && contiguous
                // then coalesce the two blocks
                if(block->_size + sizeof(block_info) < block_max_size && contiguous)
                {
                    // set the new free block
                    block_info* new_free_block = block;

                    new_free_block->previous_chunk = block->previous_chunk;
                    new_free_block->_non_arena = block->_non_arena;
                    new_free_block->_is_mmapped = block->_is_mmapped;
                    new_free_block->_present = block->_present;
//                    if(!new_free_block->_full) {
//                        new_free_block->_size += next_block->_size + sizeof(block_info);
//                    }
                    new_free_block->_size += next_block->_size + sizeof(block_info);

                    new_free_block->next_free = next_block->next_free;

                } else 
                {
                    // set the next free block.
                    block_info* free_block = block;
                    kernel_debug_output(KDB_LVL_VERBOSE, "free 1 next block = 0%p, first 0%p", block, first_free);
                    free_block->next_free = next_block;
                    break;
                }
            }
            // set contiguous to 0 and shift to the next block
            contiguous = 0;
            size_t blk_size = next_block->_size + sizeof(block_info);
//            if(block->_full) {
//                blk_size = block->_size;
//            }
            next_block = (void*)(((uint8_t*)next_block) + blk_size);
            kernel_debug_output(KDB_LVL_VERBOSE, "free 2 block = 0%p, next_block 0%p", block, next_block);
        } else
        {
            // the next free block is the virtual kernel heap top.
            block_info* free_block = block;
            kernel_debug_output(KDB_LVL_VERBOSE, "2 next block = 0%p, first 0%p", block, free_block);
            free_block->next_free = current_top;
            next_block = free_block;
            break;
        }
    }

    // if the freed block is smaller than fist_free then 
    // set first free to block.
    if((block_info*) *first_free > block)
    {
        kernel_debug_output(KDB_LVL_VERBOSE, "first free = 0%p", block);
        block->next_free = (void*)*first_free;
        *first_free = (uintptr_t)block;
    } else 
    {
        block_info* first = (block_info*)(*first_free);

        kernel_debug_output(KDB_LVL_VERBOSE, "1 first free = 0%p | first = 0%p", *first_free, first);
        while(first && first->next_free && first->next_free < block)
        {
            first = first->next_free;
            kernel_debug_output(KDB_LVL_VERBOSE, "2 first free = 0%p", first);
        }

        if(first != block)
        {
            kernel_debug_output(KDB_LVL_VERBOSE, "3 next block = 0%p, first 0%p", block, first);
            first->next_free = (void*)block;
//            if(first) {
//            } else {
//                *first_free = (uintptr_t)block;
//            }
        }
    }
    block->_full = 0;
}