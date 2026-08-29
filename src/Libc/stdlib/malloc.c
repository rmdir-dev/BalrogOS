#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <balrog/memory/heap.h>
#include <balrog/memory/proc_mem.h>

void* const heap_start = (void*) PROCESS_HEAP_START;
void* heap_top = (void*) PROCESS_HEAP_START + 0x1000;
void* first_free = 0;

void* malloc(size_t size)
{
    block_info* current_block = first_free;
    size += sizeof(block_info) * 3; // add 60 bytes to the size to protect against heap corruption

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
        /*  if the next block is smaller than the current top
            then try to allocate the new block
        */
        if(current_block < (block_info*) heap_top && current_block >= (block_info*) heap_start)
        {
            void* ret = heap_alloc(size, current_block, prev_block, heap_top, (uintptr_t*)&first_free, first_block);
            if(ret != 0)
            {
                return ret;
            }
            // current block = next block
            prev_block = current_block;
            current_block = current_block->next_free;
        } else
        {
            /*  if the current heap top is equal to PROCESS_HEAP_END
                then we don't have any space left in memory.
            */
            if(current_block >= (block_info*) PROCESS_HEAP_END)
            {
                // Heap is full
                return 0;
            }

            // get a new page
            printf("heap full brk\n");
            brk(heap_top + 0x1000);

            // set the first block address to heap_top
            block_info* first_block = (void*) heap_top;

            // set the new first block data.
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

            // set the new heap top.
            heap_top += 0x1000;
        }
        // we're not on the first block anymore.
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
        printf("double free() 0%p", block);
        exit(-1);
    }

    heap_free(block, next_block, heap_top, (uintptr_t*)&first_free, heap_top - heap_start);
}
