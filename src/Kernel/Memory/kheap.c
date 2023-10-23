#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Memory/memory.h"
#include "BalrogOS/Memory/pmm.h"
#include "BalrogOS/Debug/debug_output.h"

void* kheap_start = 0;
void* kheap_end = 0;
void* kfirst_free = 0;
size_t kheap_max_size = 0;
size_t kheap_size = 0;

extern void* alloc(size_t size, block_info* current_block, block_info* prev_block, block_info* current_top, uintptr_t* first_free, uint8_t first_block);
extern void free(block_info* block, block_info* next_block, block_info* current_top, uintptr_t* first_free, uint64_t block_max_size);

void init_kheap()
{
    /* Declared in linker script */
    extern uintptr_t* kernel_end;
    kheap_start = &kernel_end;
    kheap_end = (void*)P2V(0x9fc00);
    kheap_max_size = kheap_end - kheap_start;

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
    size += sizeof(block_info) * 2; // add 40 bytes to the size to protect against heap corruption

    kernel_debug_output(KDB_LVL_VERBOSE, "kheap current block = 0%p", current_block);
    while(1)
    {
        if(current_block < (block_info*)kheap_end)
        {
            void* ret = alloc(size, current_block, prev_block, kheap_end, (uintptr_t*)&kfirst_free, first_block);
            kernel_debug_output(KDB_LVL_VERBOSE, "found block = 0%p, first free = 0%p", ret, kfirst_free);
            if(ret != 0)
            {
                kheap_size += size;
                kernel_debug_output(KDB_LVL_VERBOSE, "kalloc size = %d/%d KiB added : %d", kheap_size, BYTE_TO_KiB(kheap_max_size), size);
                return ret;
            }
            // current block = next block
            prev_block = current_block;
            current_block = current_block->next_free;
        } else 
        {
            kernel_debug_output(KDB_LVL_CRITICAL, "kheap no more memory available");
            return 0x00;
        }
    }
    return 0x0;
}

void kfree(void* ptr)
{
    block_info* block = ptr - sizeof(block_info);
    block_info* next_block =  ptr + block->_size;
    size_t size = block->_size;

    free(block, next_block, kheap_end, (uintptr_t*)&kfirst_free, kheap_end - kheap_start);
    kheap_size -= size;
    kernel_debug_output(KDB_LVL_VERBOSE, "kfree size = %d/%d KiB freed : %d from 0%p", BYTE_TO_KiB(kheap_size), BYTE_TO_KiB(kheap_max_size), size, ptr);
}
