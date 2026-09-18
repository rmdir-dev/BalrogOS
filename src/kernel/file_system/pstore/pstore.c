#include "balrog_os/file_system/pstore/pstore.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/memory/pmm.h"
#include "balrog_os/memory/vmm.h"

int init_pstore()
{
    pmm_reserve(PSTORE_RAM_PHYS, PSTORE_RAM_SIZE);
    // vmm_set_page()
    volatile uint32_t* mark = (volatile uint32_t*) P2V(PSTORE_RAM_PHYS);

    KERNEL_LOG_INFO("pstore ram : %p held 0%x", (void*) PSTORE_RAM_PHYS, *mark);

    // TODO : check if pstore is possible on dell
    if (*mark == 0x42414C52)
    {
        while (1) {}
    }

    *mark = 0x42414C52;         // BALR, and any value would do

    return 0;
}
