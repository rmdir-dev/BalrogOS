#include "BalrogOS/Memory/pmm.h"
#include "BalrogOS/Debug/debug_output.h"
#include <string.h>
#include "BalrogOS/Memory/vmm.h"

uint64_t total_memory = 0;
uint64_t total_memory_used = 0;
uint64_t total_usable_memory = 0;

/*
Pointer to last address allocated | increment this at each new alloc (if no memory was a freed one)
Pointer to last address freed -> contain the last address freed before
*/
uintptr_t next_addr = 0;
uintptr_t top_32_addr = 0;
uintptr_t top_addr = 0;
uintptr_t last_free_addr = 0x0;

void pmm_free(uintptr_t* addr)
{
    if(next_addr < addr)
    {
        return;
    }
    addr = PHYSICAL_TO_VIRTUAL(addr);
    *addr = last_free_addr;
    last_free_addr = addr;
    total_memory_used -= 0x1000;
}

uintptr_t* pmm_alloc()
{
    uintptr_t* p = 0x00;
    if(last_free_addr)
    {
        p = last_free_addr;
        last_free_addr = *p;
        p = VIRTUAL_TO_PHYSICAL(p);

    } else 
    {
        if(next_addr == top_addr)
        {
            return 0x0;
        }

        if(next_addr >= top_32_addr - 0x1000 && next_addr < 0x100000000)
        {
            next_addr = 0x100000000;
        }

        p = next_addr;

        next_addr += PAGE_SIZE;
    }
    total_memory_used += 0x1000;
    return p;
}

uintptr_t* pmm_calloc()
{
    uintptr_t* p = pmm_alloc();

    memset(PHYSICAL_TO_VIRTUAL(p), 0, PAGE_SIZE);
    
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
            } else 
            {
                top_addr = end;
            }
        }

        for(uintptr_t p = start; (p + PAGE_SIZE) < end; p += PAGE_SIZE)
        {
            vmm_set_page(0, PHYSICAL_TO_VIRTUAL(p), p, PAGE_PRESENT | PAGE_WRITE);
            
            if(SMAPinfo[i].Type == USABLE_MEMORY && p > 0x100000)
            {
                pmm_free(p);
            }
        }
    }
    
    KERNEL_LOG_INFO("Total system memory : %dMiB", BYTE_TO_MiB(total_memory));
    KERNEL_LOG_INFO("Total usable system memory : %dMiB", BYTE_TO_MiB(total_usable_memory));
}