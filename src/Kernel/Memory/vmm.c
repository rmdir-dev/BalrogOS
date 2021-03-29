#include "BalrogOS/Memory/vmm.h"
#include "BalrogOS/Memory/memory.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/Debug/debug_output.h"
#include "BalrogOS/Memory/pmm.h"
#include <string.h>

/*
Contain the physical address of the kernel PML4T
*/
page_table* KernelPML4T;

void init_vmm()
{
    KernelPML4T = (void*)0x1000;
}

/**
 * @brief Find an existing page table
 * 
 * @param PML4T the Page Map Level 4 Table
 * @param virt_addr the virtual address we want to use
 * @param create if we want to create pages that do not exist.
 * @return uintptr_t return the Level 1 Page Table.
 */
static uintptr_t* vmm_find_page(page_table* PML4T, uintptr_t virt_addr, uint8_t create)
{
    virt_addr = STRIP_FLAGS(virt_addr);

    page_table* PDPT;
    page_table* PDT;
    page_table* PT;
    
    PML4T = (void*)P2V(PML4T);
    PDPT = (void*)PML4T[PML4T_OFFSET(virt_addr)];

    if(!PDPT)
    {
        if(!create)
        {
            return 0;
        }
        uintptr_t p = (uintptr_t)pmm_calloc();
        PDPT = (void*)ADD_FLAGS(p, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
        PML4T[PML4T_OFFSET(virt_addr)] = (page_table)PDPT;
    }

    PDPT = (void*)P2V(STRIP_FLAGS(PDPT));
    PDT = (void*)PDPT[PDPT_OFFSET(virt_addr)];
        
    if(!PDT)
    {
        if(!create)
        {
            return 0;
        }

        uintptr_t p = (uintptr_t)pmm_calloc();
        PDT = (void*)ADD_FLAGS(p, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
        PDPT[PDPT_OFFSET(virt_addr)] = (page_table)PDT;
    }
    
    PDT = (void*)P2V(STRIP_FLAGS(PDT));
    PT = (void*)PDT[PDT_OFFSET(virt_addr)];
    
    if(!PT)
    {
        if(!create)
        {
            return 0;
        }
        uintptr_t p = (uintptr_t)pmm_calloc();
        PT = (void*)ADD_FLAGS(p, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
        PDT[PDT_OFFSET(virt_addr)] = (page_table)PT;
    }
    
    return (void*)P2V(STRIP_FLAGS(PT));
}

uintptr_t vmm_get_page(page_table* PML4T, uintptr_t virt_addr)
{
    if(!PML4T)
    {
        PML4T = KernelPML4T;
    }
    
    page_table* PT = vmm_find_page(PML4T, virt_addr, 0);
    
    if(!PT)
    {
        return 0;
    }
    return STRIP_FLAGS(PT[PT_OFFSET(virt_addr)]);
}

void* vmm_set_page(page_table* PML4T, uintptr_t virt_addr, uintptr_t phys_addr, uint32_t flags)
{
    if(!PML4T)
    {
        PML4T = KernelPML4T;
    }
    
    page_table* PT = vmm_find_page(PML4T, virt_addr, 1);

    if(!PT)
    {
        return 0x00;
    }

    PT[PT_OFFSET(virt_addr)] = ADD_FLAGS(phys_addr, flags);
    
    return (void*)PT[PT_OFFSET(virt_addr)];
}

void vmm_free_page(page_table* PML4T, uintptr_t virt_addr)
{
    if(!PML4T)
    {
        PML4T = KernelPML4T;
    }

    page_table* PT = vmm_find_page(PML4T, virt_addr, 1);

    if(!PT)
    {
        return;
    }

    if(PT[PT_OFFSET(virt_addr)])
    {
        pmm_free((uintptr_t*)STRIP_FLAGS(PT[PT_OFFSET(virt_addr)]));
        PT[PT_OFFSET(virt_addr)] = 0;
    }
}

static int _vmm_clean(page_table* table, uint8_t level)
{
    page_table* tab = (void*)P2V(STRIP_FLAGS(table));
    
    for(int i = 0; i < 512; i++)
    {
        // if tab[i] has an address 
        // and tab[i] is not the kernel address
        // the kernel is at address 511 of the PML4T
        if(tab[i] && (i < 511 || level < 4))
        {
            // if the page table is a PML4T, PDPT, PDT
            // then clean the level below before cleaning it.
            if(level > 1)
            {
                _vmm_clean((void*)tab[i], level - 1);
            }
            // free the page.
            pmm_free((void*)STRIP_FLAGS(tab[i]));
            tab[i] = 0;
        }
    }
    return 0;
}

int vmm_clean_page_table(page_table* PML4T)
{
    if(!PML4T)
    {
        return -1;
    }
    
    return _vmm_clean(PML4T, 4);
}