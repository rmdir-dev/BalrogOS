#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Memory/memory.h"
#include "BalrogOS/Memory/pmm.h"
#include "BalrogOS/Debug/debug_output.h"

void* alloc(size_t size, block_info* current_block, block_info* prev_block, block_info* current_top, uintptr_t* first_free, uint8_t first_block)
{
    /*  check if the block is large enough 
        if yes allocate the new block of memory here.
    */
    //kprint("current size 0%x addr: 0%p | size : %d\n", current_block->_size, current_block, size);
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
                *first_free = current_block->next_free;
            } else 
            {
                prev_block->next_free = current_block->next_free;
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
            uint32_t new_block_size = size + sizeof(block_info);
            //kprint("current block size : 0%x | 0%p\n", current_block->_size, current_block);
            current_block->_size -= new_block_size;
            //kprint("current block size : 0%x\n", new_block_size);

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
        if(current_block < current_top)
        {
            // if not then set the previous block to block and the present bit to 0
            current_block->previous_chunk = block;
            current_block->_present = 0;
        }

        // return the address of the newly allocated block
        //kprint("vh alloc : 0%p | prev 0%p\n", block + sizeof(block_info), prev_block->next_free);
        return block + sizeof(block_info);
    }

    return 0;
}

void free(block_info* block, block_info* next_block, block_info* current_top, uintptr_t* first_free, uint64_t block_max_size)
{
    //kprint("vh freeing : 0%p \n", block);
    if(!block->_is_mmapped)
    {
        kprint("double vmfree() 0%p", block);
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
        if(next_block < (block_info*) current_top)
        {
            // if the next block is not mapped
            if(!next_block->_is_mmapped)
            {
                // if block is smaller than 4096 bytes && contiguous
                // then coalesce the two blocks
                if(block->_size + sizeof(block_info) < block_max_size && contiguous)
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
            free_block->next_free = current_top;
            next_block = free_block;
            break;
        }
    }

    // if the freed block is smaller than fist_free then 
    // set first free to block.
    if((block_info*) *first_free > block)
    {
        *first_free = block;
    } else 
    {
        block_info* first = *first_free;
        
        while(first->next_free < block)
        {
            first = first->next_free;
        }

        if(first != block)
        {
            first->next_free = block;
        }
    }
}