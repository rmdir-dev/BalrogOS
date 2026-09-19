#include "balrog_os/memory/kheap.h"
#include "balrog_os/memory/memory.h"
#include "balrog_os/memory/pmm.h"
#include "balrog_os/memory/vmm.h"
#include "balrog_os/debug/debug_output.h"

/*
Kernel heap start at KERNEL_OFFSET + 0x4000000000 (256GiB)
Total kernel heap = 128GiB
*/

#define KERNEL_VIRTUAL_START    0xffffffc000000000
#define KERNEL_VIRTUAL_TOP      0xffffffe000000000

#define VMHEAP_MAX_BLOCK_SIZE (KERNEL_VIRTUAL_TOP - KERNEL_VIRTUAL_START)

void* vmheap_start;
void* vmheap_current_top;
void* first_free;
void* vmheap_end;
size_t vmheap_size = 0;
size_t vmheap_current_size = 0;
size_t alloc_count = 0;
size_t free_count = 0;

int init_vmheap()
{
    vmheap_start = (void*)KERNEL_VIRTUAL_START;
    first_free = vmheap_start;

    // allocate first 1 MiB of the virtual heap
    for(size_t i = 0; i < 256; i++) {
        void* alloc = pmm_calloc();
        if (!alloc)
        {
            // if alloc == 0 then alloc = kernel physical address
            kernel_debug_output(KDB_LVL_CRITICAL, "init_vmheap() : fail to allocate page at boot !", alloc_count);
            return 1;
        }
        vmm_set_page(0, vmheap_start + vmheap_size, alloc, PAGE_PRESENT | PAGE_WRITE);
        vmheap_size += 0x1000;
    }

    vmheap_current_top = vmheap_start + vmheap_size;

    block_info block;
    block.previous_chunk = 0;
    block._is_mmapped = 0;
    block._non_arena = 0;
    block._present = 0;
    block._size = vmheap_size - sizeof(block_info);
    block.next_free = vmheap_current_top;
    block_info* first_block = vmheap_start;
    *first_block = block;

    return 0;
}

void* vmalloc(size_t size)
{
    if (size > VMHEAP_MAX_BLOCK_SIZE - sizeof(block_info) * 3)
    {
        kernel_debug_output(KDB_LVL_CRITICAL, "vmalloc() : %d is above the heap ceiling", size);
        return 0;
    }

    block_info* start_block = first_free;
    block_info* current_block = first_free;
    size += sizeof(block_info) * 3; // add 72 bytes to the size to protect against heap corruption

    //kernel_debug_output(KDB_LVL_VERBOSE, "vm heap c %d start 0%p -> 0%p", alloc_count, start_block, first_free);
    //kernel_debug_output(KDB_LVL_VERBOSE, "vmalloc first free block = 0%p | size : %d ", current_block, size);

    block_info* prev_block = current_block;
    uint8_t first_block = 1;

    while(1)
    {
        /*  if the next block is smaller than the current top
            then try to allocate the new block
        */
        if(current_block < (block_info*) vmheap_current_top && current_block >= (block_info*) KERNEL_VIRTUAL_START)
        {
            void* ret = heap_alloc(size, current_block, prev_block, vmheap_current_top, (uintptr_t*)&first_free, first_block);
            //kernel_debug_output(KDB_LVL_VERBOSE, "RET block = 0%p, first free = 0%p", ret, first_free);
            if(ret != 0)
            {
                vmheap_current_size += ((block_info*)(ret - sizeof(block_info)))->_size;

                //kernel_debug_output(KDB_LVL_VERBOSE, "vmheap first free after alloc block = 0%p", first_free);
                alloc_count++;
                // kernel_debug_output(KDB_LVL_VERBOSE, "vmalloc size = %d/%d KiB added : %d to 0%p | %d | first free 0%p", vmheap_current_size, BYTE_TO_KiB(vmheap_size), size, ret, alloc_count, first_free);

                return ret;
            }
            // current block = next block
            //kernel_debug_output(KDB_LVL_VERBOSE, "curr block = 0%p | next block = 0%p", current_block, current_block->next_free);
            prev_block = current_block;
            current_block = current_block->next_free;
        } else
        {
            uint8_t empty_list = (prev_block >= (block_info*)vmheap_current_top) || (prev_block <  (block_info*)KERNEL_VIRTUAL_START);
            block_info* tail = prev_block;
            uint8_t extend = !empty_list && ((uint8_t*)tail + sizeof(block_info) + tail->_size == (uint8_t*)vmheap_current_top);

            size_t needed = extend ? (size + sizeof(block_info) - tail->_size) : (size + sizeof(block_info) * 2);
            size_t pages  = (needed / 0x1000) + 1;

            if(vmheap_current_top + (pages * 0x1000) > (void*)KERNEL_VIRTUAL_TOP)
            {
                kernel_debug_output(KDB_LVL_CRITICAL, "vmalloc() failed, no more space left in virtual heap");
                return 0;
            }

            for(size_t i = 0; i < pages; i++)
            {
                void* alloc = pmm_calloc();
                if(!alloc)
                {
                    kernel_debug_output(KDB_LVL_CRITICAL, "vmalloc() failed, no physical page left");
                    for(size_t j = 0; j < i; j++)
                    {
                        void* mapped = vmm_get_page(0, vmheap_current_top + (j * 0x1000));
                        vmm_free_page(0, vmheap_current_top + (j * 0x1000));
                        pmm_free(mapped);
                    }
                    return 0;
                }
                vmm_set_page(0, vmheap_current_top + (i * 0x1000), alloc, PAGE_PRESENT | PAGE_WRITE);
            }

            if(extend)                                                /* A */
            {
                /*  already in the free list, already the right previous_chunk.
                    only its size and the sentinel it points at change.
                */
                tail->_size += pages * 0x1000;
                tail->next_free = vmheap_current_top + (pages * 0x1000);
                current_block = tail;
            } else
            {
                // set the first block address to vmheap_current_top
                block_info* first_block = vmheap_current_top;

                // set the new first block data.
                // The block previous block/chunk is now allocated by default so not = prev_block anymore
                first_block->previous_chunk = 0;
                first_block->_is_mmapped = 0;
                first_block->_non_arena = 0;
                // Switch the logic, now the block bellow is always allocated so not = 1 anymore
                first_block->_present = 0;
                first_block->_size = (pages * 0x1000) - sizeof(block_info);
                first_block->next_free = vmheap_current_top + (pages * 0x1000);

                if(!empty_list && prev_block->next_free != vmheap_current_top)
                {
                    prev_block->next_free = first_block;
                }

                if(empty_list || first_free >= vmheap_current_top)
                {
                    first_free = first_block;
                }

                current_block = first_block;
            }

            vmheap_size += pages * 0x1000;
            vmheap_current_top += pages * 0x1000;
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

    if(!block->_is_mmapped)
    {
        kernel_debug_output(KDB_LVL_ERROR, "double vmfree() 0%p", block);
        while(1)
        {
        }
    }

    //kernel_debug_output(KDB_LVL_VERBOSE, "vm free c %d start 0%p -> 0%p", free_count, block, first_free);
    size_t max_block = vmheap_size < VMHEAP_MAX_BLOCK_SIZE ? vmheap_size : VMHEAP_MAX_BLOCK_SIZE;
    heap_free(block, next_block, vmheap_current_top, (uintptr_t*)&first_free, max_block);
    vmheap_current_size -= size;
    ++free_count;
    alloc_count--;
    // kernel_debug_output(KDB_LVL_VERBOSE, "vmfree size = %d/%d KiB freed : %d from 0%p | %d | first free 0%p", vmheap_current_size, BYTE_TO_KiB(vmheap_size), BYTE_TO_KiB(size), ptr, free_count, first_free);
    //kernel_debug_output(KDB_LVL_VERBOSE, "vmfree first free current block = 0%p", first_free);
}