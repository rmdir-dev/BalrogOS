#include "BalrogOS/Memory/pmm.h"
#include "BalrogOS/Debug/debug_output.h"
#include "BalrogOS/Memory/vmm.h"

#include "klib/DataStructure/queue.h"
#include "klib/IO/kprint.h"
#include <string.h>

uint64_t total_memory = 0;
uint64_t total_memory_used = 0;
uint64_t total_usable_memory = 0;

/*
Pointer to last address allocated | increment this at each new alloc (if no memory was a freed)
*/
void* next_addr = 0;
/*
Pointer to the top most 32 bit address.
*/
void* top_32_addr = 0;
/*
Pointer to the top most address.
*/
void* pmm_top_addr = 0;
/*
Pointer to last address freed -> contain the last address freed before
*/
void* last_free_addr = 0x0;

// freed address queue.
queue_t last_free_q;

void pmm_free(void* addr)
{
    // if addr is greater than next_addr
    // then it should not be freed because next_addr is not
    // at addr yet.
    if(next_addr < addr)
    {
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

    // set the bits inside the page to 0.
    memset((void*)P2V(p), 0, PAGE_SIZE);
    
    return p;
}

void init_pmm(SMAP_entry* SMAPinfo, uint16_t* SMAPsize)
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
                top_32_addr = end;
                pmm_top_addr = end;
            } else 
            {
                pmm_top_addr = end;
            }
        }

        if(i != *SMAPsize - 1 || SMAPinfo[i].Type == 1) {
            for (void *p = start; (p + PAGE_SIZE) < end; p += PAGE_SIZE) {
                vmm_set_page(0, (void *) P2V(p), p, PAGE_PRESENT | PAGE_WRITE);

                if (SMAPinfo[i].Type == USABLE_MEMORY && p > (void *) 0x100000) {
                    pmm_free(p);
                }
            }
        }
    }
    
    KERNEL_LOG_INFO("Total system memory : %dMiB", BYTE_TO_MiB(total_memory));
    KERNEL_LOG_INFO("Total usable system memory : %dMiB", BYTE_TO_MiB(total_usable_memory));
}