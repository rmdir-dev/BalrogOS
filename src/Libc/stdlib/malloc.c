#include <stdlib.h>
#include <stdint.h>
#include <balrog/memory/heap.h>

void* const heap_start = 0x000055c0603d3000;
void* heap_end = heap_start + 0x1000;
void* first_free = heap_start;

void* malloc(size_t memory_size)
{
    block_info* current_block = heap_start;
    block_info* prev_block = current_block;
    uint8_t first_block = 1;

    while(1)
    {
        if(current_block < (block_info*)heap_end)
        {
        }
    }
    return 0;
}

void free(void* pointer)
{

}