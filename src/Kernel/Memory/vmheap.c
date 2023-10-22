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

void* vmheap_start;
void* vmheap_current_top;
void* first_free;
void* vmheap_end;
size_t vmheap_size = 0;
size_t vmheap_current_size = 0;
size_t alloc_count = 0;
size_t free_count = 0;

extern void* alloc(size_t size, block_info* current_block, block_info* prev_block, block_info* current_top, uintptr_t* first_free, uint8_t first_block);
extern void free(block_info* block, block_info* next_block, block_info* current_top, uintptr_t* first_free, uint64_t block_max_size);

void init_vmheap()
{
    vmheap_start = (void*)KERNEL_VIRTUAL_START;
    first_free = vmheap_start;
    vmheap_current_top = vmheap_start + 0x1000;
    void* alloc = pmm_calloc();
    vmm_set_page(0, vmheap_start, alloc, PAGE_PRESENT | PAGE_WRITE);
    vmheap_size += 0x1000;

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
    block_info* start_block = first_free;
    block_info* current_block = first_free;

//    if(first_free == vmheap_current_top) {
//        kernel_debug_output(KDB_LVL_VERBOSE, "first free = vmheap current top = 0%p", first_free);
//        block_info* blk = vmheap_start;
//        if(blk->_is_mmapped) {
//            blk = blk->next_free;
//        }
////        while(blk->previous_chunk && blk->previous_chunk >= KERNEL_VIRTUAL_START && blk->_present == 0) {
////            kernel_debug_output_no_ln(KDB_LVL_INFO, "blk = 0%p", blk);
////            blk = blk->previous_chunk;
////        }
//        first_free = blk;
//        current_block = blk;
//        kernel_debug_output(KDB_LVL_VERBOSE, "new first free 0%p -> 0%p", current_block, first_free);
//    }

//    block_info* free = first_free;
    kernel_debug_output(KDB_LVL_VERBOSE, "vm heap c %d start 0%p -> 0%p", alloc_count, start_block, first_free);
    kernel_debug_output_no_ln(KDB_LVL_VERBOSE, "vmalloc first free block = 0%p | size : %d ", current_block, size);

    block_info* prev_block = current_block;
    uint8_t first_block = 1;

    while(1)
    {
        /*  if the next block is smaller than the current top
            then try to allocate the new block
        */
        if(current_block < (block_info*) vmheap_current_top && current_block >= KERNEL_VIRTUAL_START)
        {
            void* ret = alloc(size, current_block, prev_block, vmheap_current_top, (uintptr_t*)&first_free, first_block);
            kernel_debug_output(KDB_LVL_VERBOSE, "RET block = 0%p, first free = 0%p", ret, first_free);
            if(ret != 0)
            {
                vmheap_current_size += size;

                kernel_debug_output(KDB_LVL_VERBOSE, "vmheap first free after alloc block = 0%p", first_free);
                alloc_count++;
                kernel_debug_output(KDB_LVL_INFO, "vmalloc size = %d/%d KiB added : %d to 0%p | %d", vmheap_current_size, BYTE_TO_KiB(vmheap_size), BYTE_TO_KiB(size), ret, alloc_count);
                return ret;
            }
            // current block = next block
            kernel_debug_output(KDB_LVL_VERBOSE, "curr block = 0%p | next block = 0%p", current_block, current_block->next_free);
            prev_block = current_block;
            current_block = current_block->next_free;
        } else
        {
            kernel_debug_output(KDB_LVL_VERBOSE, "invalid block 0%p top = 0%p", current_block, vmheap_current_top);
            // if the current virtual heap top is equal to KERNEL_VIRTUAL_TOP
            // then we don't have any space left in memory.
            // We also don't allow to allocate a block larger than 4072 bytes atm.
            if(vmheap_current_top == (void*)KERNEL_VIRTUAL_TOP)
            {
                //Kernel heap full
                kernel_debug_output(KDB_LVL_CRITICAL, "vmalloc() failed, no more space left in virtual heap");
                return 0;
            }

            // get a new page
            void* alloc = pmm_calloc();
            // set the new page to the vmheap_current_top address
            vmm_set_page(0, vmheap_current_top, alloc, PAGE_PRESENT | PAGE_WRITE);
            vmheap_size += 0x1000;
            kernel_debug_output(KDB_LVL_VERBOSE, "new vmheap size = %d", vmheap_size);

            // set the first block address to vmheap_current_top
            block_info* first_block = vmheap_current_top;

            // set the new first block data.
            first_block->previous_chunk = prev_block;
            first_block->_is_mmapped = 0;
            first_block->_non_arena = 0;
            first_block->_present = 1;
            first_block->_size = 0x1000 - sizeof(block_info);
            first_block->next_free = vmheap_current_top + 0x1000;
            if(prev_block->next_free != vmheap_current_top) {
                kernel_debug_output(KDB_LVL_VERBOSE, "current block next free = 0%p", prev_block->next_free);
                prev_block->next_free = first_block;
                current_block = first_block;
//                while (1) {}
            }

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
    size_t size = block->_size;

    kernel_debug_output(KDB_LVL_VERBOSE, "vm free c %d start 0%p -> 0%p", free_count, block, first_free);
    free(block, next_block, vmheap_current_top, (uintptr_t*)&first_free, vmheap_size);
    vmheap_current_size -= size;
    ++free_count;
    kernel_debug_output(KDB_LVL_INFO, "vmfree size = %d/%d KiB freed : %d from 0%p | %d", vmheap_current_size, BYTE_TO_KiB(vmheap_size), BYTE_TO_KiB(size), ptr, free_count);
    kernel_debug_output(KDB_LVL_VERBOSE, "vmfree first free current block = 0%p", first_free);
}