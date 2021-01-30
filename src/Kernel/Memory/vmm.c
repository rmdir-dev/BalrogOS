#include "BalrogOS/Memory/vmm.h"
#include "memory.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/Debug/debug_output.h"
#include "BalrogOS/Memory/pmm.h"

#define STRIP_FLAGS(addr)       (((uintptr_t)addr) & ~PAGE_FLAG_MASK)
#define ADD_FLAGS(addr, flags)   (((uintptr_t)addr) | (flags & PAGE_FLAG_MASK))

/*
Contain the physical address of the kernel PML4T
*/
page_table* KernelPML4T;

static interrupt_regs* vmm_page_fault_handler(interrupt_regs* regs)
{
    KERNEL_LOG_FAIL("Page fault");

    if(regs->error_code & 0x02)
    {
        KERNEL_LOG_FAIL("read from");
    } else if(regs->error_code & 0x0e)
    {
        KERNEL_LOG_FAIL("execute code from");
    } else 
    {
        KERNEL_LOG_FAIL("write to");
    }

    if(regs->error_code & 0x08)
    {
        KERNEL_LOG_FAIL("user mode");
    } else
    {
        KERNEL_LOG_FAIL("kernel mode");
    }

    if(regs->error_code & 0x01)
    {
        KERNEL_LOG_FAIL("PROTECTION FAULT");
    } else 
    {
        KERNEL_LOG_FAIL("PAGE MISS");
    } 
    
    while(1)
    {}
    
    return regs;
}

void init_vmm()
{
    register_interrupt_handler(INT_PAGE_FAULT, vmm_page_fault_handler);

    KernelPML4T = 0x1000;
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
    
    PML4T = PHYSICAL_TO_VIRTUAL(PML4T);

    PDPT = PML4T[PML4T_OFFSET(virt_addr)];
    if(!PDPT)
    {
        if(!create)
        {
            return 0;
        }
        uintptr_t p = pmm_calloc();
        PML4T[PML4T_OFFSET(virt_addr)] = PDPT = ADD_FLAGS(p, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    }

    PDPT = PHYSICAL_TO_VIRTUAL(STRIP_FLAGS(PDPT));
    PDT = PDPT[PDPT_OFFSET(virt_addr)];
        
    if(!PDT)
    {
        if(!create)
        {
            return 0;
        }

        uintptr_t p = pmm_calloc();
        PDPT[PDPT_OFFSET(virt_addr)] = PDT = ADD_FLAGS(p, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    }
    
    PDT = PHYSICAL_TO_VIRTUAL(STRIP_FLAGS(PDT));
    PT = PDT[PDT_OFFSET(virt_addr)];
    
    if(!PT)
    {
        if(!create)
        {
            return 0;
        }
        uintptr_t p = pmm_calloc();
        PDT[PDT_OFFSET(virt_addr)] = PT = ADD_FLAGS(p, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    }
    
    return PHYSICAL_TO_VIRTUAL(STRIP_FLAGS(PT));
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
    return PT[PT_OFFSET(virt_addr)];
}

void vmm_set_page(page_table* PML4T, uintptr_t virt_addr, uintptr_t phys_addr, uint32_t flags)
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
    
    return PT[PT_OFFSET(virt_addr)];
}