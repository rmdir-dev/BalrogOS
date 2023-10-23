#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <balrog/memory/heap.h>
#include <balrog/memory/proc_mem.h>

void* const heap_start = (void*) 0x000055c0603d3000;
void* heap_top = heap_start + 0x1000;
void* first_free = 0;

void* malloc(size_t size)
{
    block_info* current_block = first_free;
    size += sizeof(block_info) * 3; // add 40 bytes to the size to protect against heap corruption

    if(first_free == 0)
    {
        current_block = heap_start;
        current_block->next_free = heap_top;
        current_block->previous_chunk = 0;
        current_block->_is_mmapped = 0;
        current_block->_present = 0;
        current_block->_size = 0x1000 - sizeof(block_info);
        first_free = heap_start;
    }

    block_info* prev_block = current_block;
    uint8_t first_block = 1;

    while(1)
    {
        if(current_block < heap_top)
        {
            if(current_block->_size >= size)
            {
                // if the previous block is free of not
                uint8_t present = 1;
                uint8_t full = 0;
                
                // the current block is by default the block to be allocated.
                // we use a uint8_t to be able to shift the pointer byte by byte
                void* block = (void*)current_block;

                // check if the current block will be able to contain a new free block
                if(current_block->_size <= size + sizeof(block_info))
                {
                    // if no the new first free block will be the next one. if first block != 0
                    if(first_block && current_block == first_free)
                    {
                        first_free = (uintptr_t*) current_block->next_free;
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
                    full = 1;
                } else
                {
                    /*  if the block can contain the new block then
                        change the current block size.
                    */
                    uint32_t new_block_size = size + sizeof(block_info);
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
                new_block->_full = full;
                new_block->_is_mmapped = 1;

                // set current block to the next block.
                current_block = (void*)(block + sizeof(block_info) + size);

                // check if the current top is not the heap top
                if(current_block < heap_top)
                {
                    // if not then set the previous block to block and the present bit to 0
                    current_block->previous_chunk = block;
                    current_block->_present = 0;
                }

                // return the address of the newly allocated block
                return block + sizeof(block_info);
            }
            prev_block = current_block;
            current_block = current_block->next_free;
        } else 
        {
            if(current_block < PROCESS_HEAP_END)
            {
                printf("heap full brk\n");
                brk(heap_top + 0x1000);

                block_info* first_block = (void*) heap_top;
                first_block->previous_chunk = prev_block;
                first_block->_is_mmapped = 0;
                first_block->_non_arena = 0;
                first_block->_present = 1;
                first_block->_size = 0x1000 - sizeof(block_info);
                first_block->next_free = heap_top + 0x1000;

                if(prev_block->next_free != heap_top)
                {
                    prev_block->next_free = first_block;
                }

                current_block = first_block;
                if(first_free == heap_top || first_free > heap_top)
                {
                    first_free = first_block;
                }

                heap_top += 0x1000;
            } else
            {
                // Heap is full
                return 0;
            }
        }
        first_block = 0;
    }
    return 0;
}

void free(void* ptr)
{
    block_info* block = ptr - sizeof(block_info);
    block_info* next_block =  ptr + block->_size;

    if(!block->_is_mmapped)
    {
        printf("double vmfree() 0%p", block);
        exit(-1);
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
        if(next_block < heap_top)
        {
            // if the next block is not mapped
            if(!next_block->_is_mmapped)
            {
                // if block is smaller than 4096 bytes && contiguous
                // then coalesce the two blocks
                if(block->_size + sizeof(block_info) < (0x1000 - sizeof(block_info)) && contiguous)
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
            next_block = (void*)(((uint8_t*)next_block) + (next_block->_size + sizeof(block_info)));
        } else
        {
            // the next free block is the virtual kernel heap top.
            block_info* free_block = block;
            free_block->next_free = heap_top;
            next_block = free_block;
            break;
        }
    }

    // if the freed block is smaller than fist_free then 
    // set first free to block.
    if(first_free > block)
    {
        block->next_free = first_free;
        first_free = block;
    } else 
    {
        block_info* first = first_free;
        
        while(first->next_free < block)
        {
            first = first->next_free;
        }

        if(first != block)
        {
            first->next_free = block;
        }
    }
    block->_full = 0;
}