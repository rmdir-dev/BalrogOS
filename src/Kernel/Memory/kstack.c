#include "BalrogOS/Memory/memory.h"
#include "BalrogOS/Memory/kstack.h"
#include "BalrogOS/Memory/pmm.h"
#include "BalrogOS/Memory/vmm.h"
#include "klib/IO/kprint.h"
#include <stddef.h>

extern page_table* KernelPML4T;

void* _kstack_find_free_pt(uintptr_t* virt_addr)
{
    page_table* PML4T = (void*)P2V(KernelPML4T);

    page_table* PDPT = (void*)P2V(STRIP_FLAGS(PML4T[511]));
    for(size_t i = 384; i < 512; i++)
    {
        if(PDPT[i] == 0)
        {
            PDPT[i] = (uintptr_t)pmm_calloc();
            PDPT[i] |= PAGE_PRESENT | PAGE_WRITE;
        }
        page_table* PDT = (void*)P2V(STRIP_FLAGS(PDPT[i]));
        for(size_t j = 0; j < 512; j++)
        {
            if(PDT[j] == 0)
            {
                PDT[j] = (uintptr_t)pmm_calloc();
                PDT[j] |= PAGE_PRESENT | PAGE_WRITE;
                *virt_addr |= PML4T_TO_VIRT(511);
                *virt_addr |= PDPT_TO_VIRT(i);
                *virt_addr |= PDT_TO_VIRT(j);
                return (void*)P2V(STRIP_FLAGS(PDT[j]));
            }
        }
    }

    return 0;
}

void* kstack_alloc()
{
    uintptr_t vaddr = 0;
    page_table* PT = _kstack_find_free_pt(&vaddr);

    for(size_t i = 511; i > 505; i--)
    {
        PT[i] = (uintptr_t)pmm_calloc();
        PT[i] |= PAGE_PRESENT | PAGE_WRITE;
    }

    return (void*)((KERNEL_OFFSET | vaddr) + (4096 * 511));
}

void kstack_free(uintptr_t* addr)
{
    page_table* PML4T = (void*)P2V(KernelPML4T);
    page_table* PDPT = (void*)P2V(STRIP_FLAGS(PML4T[PML4T_OFFSET(addr)]));
    page_table* PDT = (void*)P2V(STRIP_FLAGS(PDPT[PDPT_OFFSET(addr)]));
    page_table* PT = (void*)P2V(STRIP_FLAGS(PDT[PDT_OFFSET(addr)]));

    for(size_t i = 0; i < 512; i++)
    {
        if(PT[i] != 0)
        {
            pmm_free((void*)STRIP_FLAGS(PT[i]));
            PT[i] = 0;
        }
    }

    if(PDT != 0)
    {
        pmm_free((void*)STRIP_FLAGS(PDT[PDT_OFFSET(addr)]));
        PDT[PDT_OFFSET(addr)] = 0;
    }
}