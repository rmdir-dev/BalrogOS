#include "balrog_os/memory/pmm.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/memory/vmm.h"

#include "klib/data_structure/queue.h"
#include "klib/io/kprint.h"
#include <string.h>

static uint64_t total_memory = 0;
static uint64_t total_memory_used = 0;
static uint64_t total_usable_memory = 0;

/*
Pointer to last address allocated | increment this at each new alloc (if no memory was a freed)
*/
static void* next_addr = 0;
/*
Pointer to the top most 32 bit address.
*/
void* top_32_addr = 0;
/*
Pointer to the top most address.
*/
static void* pmm_top_addr = 0;
/*
Pointer to last address freed -> contain the last address freed before
*/
static void* last_free_addr = 0x0;

// freed address queue.
static queue_t last_free_q;

/*
Physical range the bootloader already filled, that pmm_alloc must never hand
out. Zero length means nothing is reserved.
*/
/* maximum allowed reserved memory */
#define PMM_MAX_RESERVED    8

static void* reserved_start[PMM_MAX_RESERVED] = {};
static void* reserved_end[PMM_MAX_RESERVED] = {};
static size_t reserved_count = 0;
static uint8_t disable_pmm_calloc_debuging = 0;

void pmm_disable_alloc_logs()
{
    disable_pmm_calloc_debuging = 1;
}

void pmm_enable_alloc_logs()
{
    disable_pmm_calloc_debuging = 0;
}

void pmm_reserve(void* start, uint64_t size)
{
    if(reserved_count == PMM_MAX_RESERVED)
    {
        kernel_debug_output(KDB_LVL_CRITICAL, "pmm : no slot left, 0%p is NOT reserved", start);
        return;
    }

    reserved_start[reserved_count] = (void*)((uintptr_t)start & ~(PAGE_SIZE - 1));
    reserved_end[reserved_count] = (void*)(((uintptr_t)start + size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));

    KERNEL_LOG_INFO("pmm : reserved %p to %p, %d KiB", reserved_start[reserved_count],
        reserved_end[reserved_count],
        BYTE_TO_KiB(((uintptr_t)reserved_end[reserved_count] - (uintptr_t)reserved_start[reserved_count])));

    reserved_count++;
}

void pmm_free(void* addr)
{
    // if addr is greater than next_addr
    // then it should not be freed because next_addr is not
    // at addr yet.
    if(next_addr < addr)
    {
        static int said = 0;
        static size_t refused = 0;

        refused++;

        if((refused & (refused - 1)) == 0)
        {
            kernel_debug_output(KDB_LVL_ERROR, "pmm : 0%p is above the allocation front 0%p, not freed, %d so far",
                    addr, next_addr, refused);
        }
        return;
    }
    addr = (void*)P2V(addr);
    queue_enqueue(&last_free_q, (uintptr_t)addr);

    // decrease total memory used by one page
    total_memory_used -= PAGE_SIZE;
}

void* pmm_alloc()
{
    void* p = 0x00;
    if(!queue_empty(&last_free_q))
    {
        uintptr_t addr;
        queue_dequeue(&last_free_q, &addr);
        p = (void*)V2P(addr);

    } else 
    {
        // if the next address is equal to the top address then return 0
        if(next_addr >= pmm_top_addr)
        {
            return 0x0;
        }
        
        // if next address exceed 32bit addressing then switch to the 64bit address.
        if(next_addr >= top_32_addr - 0x1000 && next_addr < (void*) 0x100000000)
        {
            next_addr = (void*) 0x100000000;

            // Check if there is any usable memory above 4GiB
            if(next_addr >= pmm_top_addr)
            {
                return 0x0;
            }
        }

        /* step over the reserved ranges. */
        uint8_t moved = 1;

        while(moved)
        {
            moved = 0;

            for(size_t i = 0; i < reserved_count; i++)
            {
                if(next_addr >= reserved_start[i] && next_addr < reserved_end[i])
                {
                    next_addr = reserved_end[i];
                    moved = 1;
                }
            }
        }

        if(next_addr >= pmm_top_addr)
        {
            return 0x0;
        }

        p = (void*)next_addr;

        // increase next_addr to point to the next page.
        next_addr += PAGE_SIZE;
    }
    // increase total memory used by a page.
    total_memory_used += PAGE_SIZE;
    return p;
}

void* pmm_calloc()
{
    void* p = pmm_alloc();

    if (!p)
    {
        // P2V(0) os the kernel code and stack, never memset!
        return 0x0;
    }

    // if (disable_pmm_calloc_debuging == 0)
    // {
    //     kernel_debug_output(KDB_LVL_VERBOSE, "pmm alloc 0%p", p);
    // }

    // set the bits inside the page to 0.
    memset((void*)P2V(p), 0, PAGE_SIZE);
    
    return p;
}

int init_pmm(SMAP_entry* SMAPinfo, uint16_t* SMAPsize)
{
    queue_init(&last_free_q);

    for(uint16_t i = 0; i < *SMAPsize; i++)
    {
        total_memory += SMAPinfo[i].Length;
        void* start = (void*)SMAPinfo[i].BaseAddress;
        void* end = (void*)(SMAPinfo[i].BaseAddress + SMAPinfo[i].Length);

        if(SMAPinfo[i].Length > ONE_MiB)
        {
            KERNEL_LOG_INFO("Base %x | end %p | Length %d MiB | %d type", start, end, BYTE_TO_MiB(SMAPinfo[i].Length), SMAPinfo[i].Type);
        } else 
        {
            KERNEL_LOG_INFO("Base %x | end %p | Length %d kiB | %d Type", start, end, BYTE_TO_KiB(SMAPinfo[i].Length), SMAPinfo[i].Type);
        }

        if(SMAPinfo[i].BaseAddress >= 0x100000 && SMAPinfo[i].Type == USABLE_MEMORY)
        {
            total_usable_memory += SMAPinfo[i].Length;

            if(!next_addr)
            {
                next_addr = start;
            }

            /*
             * top_32_addr is the top of the highest usable region under 4GiB.
             */
            if(end <= (void*) 0x100000000)
            {
                top_32_addr = end;
            }

            pmm_top_addr = end;
        }

        /*
         * Skip the last entry except if it is type 1 (usable).
         * source : https://wiki.osdev.org/Memory_Map_(x86)
         *
         * The types we get here, and what they mean :
         * 1 usable, 2 reserved, 3 acpi reclaimable, 4 acpi nvs, 5 bad memory.
         * source : https://wiki.osdev.org/Detecting_Memory_(x86)
         */
        if(i != *SMAPsize - 1 || SMAPinfo[i].Type == 1)
        {
            for(void* p = start; p < end; p += PAGE_SIZE)
            {
                vmm_set_page(0, (void*)P2V(p), p, PAGE_PRESENT | PAGE_WRITE);

                if(SMAPinfo[i].Type == USABLE_MEMORY && p > (void*)0x100000)
                {
                    pmm_free(p);
                }
            }
        }
    }
    
    KERNEL_LOG_INFO("Total system memory : %dMiB", BYTE_TO_MiB(total_memory));
    KERNEL_LOG_INFO("Total usable system memory : %dMiB", BYTE_TO_MiB(total_usable_memory));

    return 0;
}