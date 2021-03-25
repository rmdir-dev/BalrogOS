#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Memory/memory.h"
#include "BalrogOS/Memory/pmm.h"
#include "BalrogOS/Memory/vmm.h"
#include "BalrogOS/Debug/debug_output.h"

/*
Kernel heap start at KERNEL_OFFSET + 0x4000000000 (256GiB)
Total kernel heap = 128GiB
*/

#define KERNEL_VIRTUAL_START    0xffffffc000000000
#define KERNEL_VIRTUAL_TOP      0xffffffdfffffffff

uintptr_t vmheap_start;
uintptr_t vmheap_current_top;
uintptr_t first_free;
uintptr_t vmheap_end;

extern void* alloc(size_t size, block_info* current_block, block_info* prev_block, block_info* current_top, uintptr_t* first_free, uint8_t first_block);
extern void free(block_info* block, block_info* next_block, block_info* current_top, uintptr_t* first_free, uint64_t block_max_size);

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
    block_info* prev_block = current_block;
    uint8_t first_block = 1;
    
    while(1)
    {
        /*  if the next block is smaller than the current top
            then try to allocate the new block
        */
        if(current_block < (block_info*) vmheap_current_top)
        {
            void* ret = alloc(size, current_block, prev_block, vmheap_current_top, &first_free, first_block);
            if(ret != 0)
            {
                return ret;
            }
            // current block = next block
            prev_block = current_block;
            current_block = current_block->next_free;
        } else 
        {
            // if the current virtual heap top is equal to KERNEL_VIRTUAL_TOP
            // then we don't have any space left in memory.
            // We also don't allow to allocate a block larger than 4072 bytes atm.
            if(vmheap_current_top == KERNEL_VIRTUAL_TOP || size > 0x1000 - sizeof(block_info))
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
    
    free(block, next_block, vmheap_current_top, &first_free, 0x1000);
}