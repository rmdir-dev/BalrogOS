#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Memory/memory.h"
#include "BalrogOS/Memory/pmm.h"
#include "BalrogOS/Debug/debug_output.h"

uintptr_t kheap_start = 0;
uintptr_t kheap_end = 0;
uintptr_t kfirst_free = 0;

extern void* alloc(size_t size, block_info* current_block, block_info* prev_block, block_info* current_top, uintptr_t* first_free, uint8_t first_block);
extern void free(block_info* block, block_info* next_block, block_info* current_top, uintptr_t* first_free);

void init_kheap()
{
    /* Declared in linker script */
    extern uintptr_t* kernel_end;
    kheap_start = &kernel_end;
    kheap_end = P2V(0x9fc00);

    block_info* first_block = kheap_start;
    first_block->_size = (kheap_end - kheap_start) - sizeof(block_info);
    first_block->next_free = kheap_end;
    first_block->previous_chunk = 0;
    first_block->_is_mmapped = 0;
    first_block->_present = 0;
    first_block->_non_arena = 0;

    kfirst_free = kheap_start;

    KERNEL_LOG_INFO("kernel logical heap size = %dKiB starts at : 0%p", BYTE_TO_KiB(first_block->_size), kheap_start);
}

void* kmalloc(size_t size)
{
    block_info* current_block = kfirst_free;
    block_info* prev_block = current_block;
    uint8_t first_block = 1;

    while(1)
    {
        if(current_block < kheap_end)
        {
            void* ret = alloc(size, current_block, prev_block, kheap_end, &kfirst_free, first_block);
            if(ret != 0)
            {
                return ret;
            }
            // current block = next block
            prev_block = current_block;
            current_block = current_block->next_free;
        } else 
        {
            return 0x00;
        }
    }
    return 0x0;
}

void kfree(void* ptr)
{
    block_info* block = ptr - sizeof(block_info);
    block_info* next_block =  ptr + block->_size;
    
    free(block, next_block, kheap_end, &kfirst_free);
}
