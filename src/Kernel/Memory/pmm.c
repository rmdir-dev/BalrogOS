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
uintptr_t next_addr = 0;
/*
Pointer to the top most 32 bit address.
*/
uintptr_t top_32_addr = 0;
/*
Pointer to the top most address.
*/
uintptr_t pmm_top_addr = 0;
/*
Pointer to last address freed -> contain the last address freed before
*/
uintptr_t last_free_addr = 0x0;

// freed address queue.
queue_t last_free_q;

void pmm_free(uintptr_t* addr)
{
    // if addr is greater than next_addr
    // then it should not be freed because next_addr is not
    // at addr yet.
    if((uintptr_t*) next_addr < addr)
    {
        return;
    }

    addr = P2V(addr);
    queue_enqueue(&last_free_q, addr);

    // decrease total memory used by one page
    total_memory_used -= PAGE_SIZE;
}

uintptr_t* pmm_alloc()
{
    uintptr_t* p = 0x00;
    if(last_free_q.head)
    {
        uintptr_t addr;
        queue_dequeue(&last_free_q, &addr);
        p = V2P(addr);

    } else 
    {
        // if the next address is equal to the top address then return 0
        if(next_addr >= pmm_top_addr)
        {
            // kprint("next : %p | top : %p ", next_addr, pmm_top_addr);
            return 0x0;
        }
        
        // if next address exceed 32bit addressing then switch to the 64bit address.
        if(next_addr >= top_32_addr - 0x1000 && next_addr < 0x100000000)
        {
            next_addr = 0x100000000;
        }

        p = next_addr;

        // increase next_addr to point to the next page.
        next_addr += PAGE_SIZE;
    }
    // increase total memory used by a page.
    total_memory_used += PAGE_SIZE;
    return p;
}

uintptr_t* pmm_calloc()
{
    uintptr_t* p = pmm_alloc();

    // set the bits inside the page to 0.
    memset(P2V(p), 0, PAGE_SIZE);
    
    return p;
}

void init_pmm(SMAP_entry* SMAPinfo, uint16_t* SMAPsize)
{
    for(uint16_t i = 0; i < *SMAPsize; i++)
    {
        total_memory += SMAPinfo[i].Length;
        uint64_t start = SMAPinfo[i].BaseAddress;
        uint64_t end = SMAPinfo[i].BaseAddress + SMAPinfo[i].Length;

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

        for(uintptr_t p = start; (p + PAGE_SIZE) < end; p += PAGE_SIZE)
        {
            vmm_set_page(0, P2V(p), p, PAGE_PRESENT | PAGE_WRITE);
            
            if(SMAPinfo[i].Type == USABLE_MEMORY && p > 0x100000)
            {
                pmm_free(p);
            }
        }
    }
    
    queue_init(&last_free_q);
    KERNEL_LOG_INFO("Total system memory : %dMiB", BYTE_TO_MiB(total_memory));
    KERNEL_LOG_INFO("Total usable system memory : %dMiB", BYTE_TO_MiB(total_usable_memory));
}