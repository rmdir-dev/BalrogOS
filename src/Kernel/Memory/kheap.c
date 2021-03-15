#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Memory/memory.h"
#include "BalrogOS/Memory/pmm.h"
#include "BalrogOS/Debug/debug_output.h"

uintptr_t kheap_start = 0;
uintptr_t kheap_end = 0;

void init_kheap()
{
    /* Declared in linker script */
    extern uintptr_t* kernel_end;
    kheap_start = &kernel_end;
    kheap_end = PHYSICAL_TO_VIRTUAL(0x9fc00);

    block_info* first_block = kheap_start;
    first_block->_size = (kheap_end - kheap_start) - sizeof(block_info);
    first_block->next_free = kheap_end;
    first_block->previous_chunk = 0;
    first_block->_is_mmapped = 0;
    first_block->_present = 0;
    first_block->_non_arena = 0;

    KERNEL_LOG_INFO("kernel logical heap size = %dKiB starts at : 0%p", BYTE_TO_KiB(first_block->_size), kheap_start);
}

void* kmalloc(size_t size)
{
    return 0x0;
}

void kfree(void* ptr)
{

}
