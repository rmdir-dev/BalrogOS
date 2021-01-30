#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Memory/memory.h"
#include "BalrogOS/Memory/pmm.h"
#include "BalrogOS/Debug/debug_output.h"

uintptr_t kheap_start = 0;
uintptr_t kheap_end = 0;

void init_kheap()
{
    /* Declared in linker script */
    extern uintptr_t* kernel_start;
    kheap_start = &kernel_start;
    kheap_end = PHYSICAL_TO_VIRTUAL(0x9fc00);

    KERNEL_LOG_INFO("kernel heap size = %dKiB", BYTE_TO_KiB((uint64_t)(kheap_end - kheap_start)));
}

void* kmalloc(size_t size)
{
    return 0x0;
}
