#include "balrog_os/file_system/pstore/pstore.h"

#include "balrog_os/cpu/cr/control_register.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/memory/pmm.h"
#include "balrog_os/memory/vmm.h"

// The top of the 32bit address map declared in pmm.c
extern void* top_32_addr;

uintptr_t pstore_start_addr;
uintptr_t pstore_end_addr;
pstore_info_t* pstore_info;

pstore_info_t* get_pstore_info()
{
    return pstore_info;
}

int init_pstore()
{
     // page aligned top 32bit addr
    pstore_start_addr = PAGE_ALIGN_DOWN((uintptr_t) top_32_addr - PSTORE_RAM_SIZE);
    pstore_end_addr = pstore_start_addr + PSTORE_RAM_SIZE;
    pmm_reserve((void*) pstore_start_addr, PSTORE_RAM_SIZE);

    for (uintptr_t p = pstore_start_addr; p < pstore_end_addr; p += PAGE_SIZE)
    {
        vmm_set_page(0, (void*) P2V(p), (void*) p, PAGE_PRESENT | PAGE_WRITE | PAGE_NOCACHE);
    }

    // refresh the tlb to take the PAGE_NOCACHE into account
    write_cr3(read_cr3());

    pstore_info = (pstore_info_t*) P2V(pstore_start_addr);

    KERNEL_LOG_INFO("pstore : %p to %p, held 0%x", (void*) pstore_start_addr,
            (void*) (pstore_end_addr), pstore_info->sig);

    // set PSTORE_SURVIVE to make a persistent store after reboot on balrog,
    // at the moment it is not planned as we should reboot on linux to read the logs
#ifdef PSTORE_SURVIVE
    if (pstore_info->sig == PSTORE_RAM_SIG)
    {
        KERNEL_LOG_INFO("pstore : reusing previous zone");
    } else
    {
        KERNEL_LOG_INFO("pstore : cold zone initialized");
        pstore_info->start = 0;
        pstore_info->size = 0;
    }
#else
    pstore_info->start = 0;
    pstore_info->size = 0;
#endif
    pstore_info->sig = PSTORE_RAM_SIG;

    return 0;
}
