#include "balrog_os/memory/memory.h"
#include "balrog_os/memory/kstack.h"
#include "balrog_os/memory/pmm.h"
#include "balrog_os/memory/vmm.h"
#include "klib/io/kprint.h"
#include "balrog_os/debug/debug_output.h"
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

    kernel_debug_output(KDB_LVL_ERROR, "kstack : the 128 pdpt entries above 384 are all taken");
    return 0;
}

void* kstack_alloc()
{
    uintptr_t vaddr = 0;
    page_table* PT = _kstack_find_free_pt(&vaddr);

    /*  a kernel stack that was never allocated shows up as a double fault
        with nothing to say where it came from.  */
    if(!PT)
    {
        kernel_debug_output(KDB_LVL_ERROR, "kstack : no page table free, no kernel stack");
        return 0;
    }

    for(size_t i = 511; i > 505; i--)
    {
        PT[i] = (uintptr_t)pmm_calloc();
        PT[i] |= PAGE_PRESENT | PAGE_WRITE | PAGE_GLOBAL;
    }

    void* top = (void*)((KERNEL_OFFSET | vaddr) + (4096 * 511));
    kernel_debug_output(KDB_LVL_VERBOSE, "kstack : 6 pages, top at 0%p", top);

    return top;
}

void kstack_free(uintptr_t* addr)
{
    page_table* PML4T = (void*)P2V(KernelPML4T);
    page_table* PDPT = (void*)P2V(STRIP_FLAGS(PML4T[PML4T_OFFSET(addr)]));
    page_table* PDT = (void*)P2V(STRIP_FLAGS(PDPT[PDPT_OFFSET(addr)]));
    page_table* PT = (void*)P2V(STRIP_FLAGS(PDT[PDT_OFFSET(addr)]));

    // the PT covers the 2MiB this address falls in, and we walk all of it
    uintptr_t base = (uintptr_t) addr & ~0x1FFFFFUL;

    for(size_t i = 0; i < 512; i++)
    {
        if(PT[i] != 0)
        {
            pmm_free((void*)STRIP_FLAGS(PT[i]));
            PT[i] = 0;
            vmm_invalidate((void*) (base + (i * 0x1000)));
        }
    }

    if(PDT != 0)
    {
        pmm_free((void*)STRIP_FLAGS(PDT[PDT_OFFSET(addr)]));
        PDT[PDT_OFFSET(addr)] = 0;
    }
}