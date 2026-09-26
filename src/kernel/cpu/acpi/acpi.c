#include "balrog_os/cpu/acpi/acpi.h"

#include <lai/core.h>
#include <lai/helpers/pm.h>
#include <lai/helpers/sci.h>

#include "balrog_os/cpu/ports/ports.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/memory/memory.h"
#include "balrog_os/memory/vmm.h"

static acpi_xsdp_t* rsdp = 0;
acpi_xsdt_t* xsdt = 0;
acpi_rsdt_t* rsdt = 0;

static int acpi_is_up = 0;

#define ACPI_8042_WAIT      100000


// source : https://wiki.osdev.org/RSDT#Checksum
int __acpi_rsdp_checksum(acpi_xsdp_t *candidate)
{
    uint8_t sum = 0;
    const uint8_t* p = (const uint8_t*) candidate;

    for(size_t i = 0; i < sizeof(acpi_rsdp_t); i++)
    {
      sum += p[i];
    }

    if(sum != 0)
    {
      return 0;
    }

    if(candidate->revision < 2)
    {
      return 1;
    }

    sum = 0;

    for(size_t i = 0; i < candidate->length; i++)
    {
      sum += p[i];
    }

    // When you add all the values in the table it should be equal to 0
    // source : https://wiki.osdev.org/RSDP#Validating
    return sum == 0;
}

int __acpi_checksum(acpi_header_t *header)
{
    unsigned char sum = 0;

    for (int i = 0; i < header->length; i++)
    {
        sum += ((char *) header)[i];
    }

    return sum == 0;
}

int __acpi_is_mapped(uintptr_t phys)
{
    return vmm_get_page(0, (void*) P2V(phys)) != 0;
}

// Check if the acpi was mapped in the page table.
// Should be map by the kernel_entry.asm, but if changed this will catch and log a critical error.
static int __acpi_range_is_mapped(uintptr_t start, uintptr_t end, const char* what)
{
    for(uintptr_t addr = start & ~(PAGE_SIZE - 1); addr < end; addr += PAGE_SIZE)
    {
        if(!__acpi_is_mapped(addr))
        {
            kernel_debug_output(KDB_LVL_CRITICAL, "acpi : %s is not mapped at 0%p, we stop before the page fault", what, addr);
            return 0;
        }
    }

    return 1;
}

static acpi_xsdp_t* __acpi_scan_range(uintptr_t start, uintptr_t end)
{
    for(uintptr_t addr = start; addr < end; addr += 16)
    {
        acpi_xsdp_t* candidate = (acpi_xsdp_t*) P2V(addr);

        if(memcmp(candidate->signature, "RSD PTR ", 8) != 0)
        {
            continue;
        }

        if(!__acpi_rsdp_checksum(candidate))
        {
            continue;
        }

        return candidate;
    }

    return 0;
}

static int __acpi_find_rsdp()
{
    /*  the word at 0x40E is a segment, we multiply it by 16 to get the
        address of the ebda.  */
    uint16_t ebda_segment = *(uint16_t*) P2V(0x40E);
    uintptr_t ebda = (uintptr_t) ebda_segment * 16;

    if(!__acpi_range_is_mapped(ebda, ebda + 1024, "the ebda"))
    {
        return -1;
    }

    rsdp = __acpi_scan_range(ebda, ebda + 1024);

    if(!rsdp)
    {
        if(!__acpi_range_is_mapped(0x000E0000, 0x00100000, "bios ROM aera"))
        {
            return -1;
        }
        rsdp = __acpi_scan_range(0x000E0000, 0x00100000);
    }

    if(!rsdp)
    {
        return -1;
    }

    if(rsdp->revision >= 2 && rsdp->xsdt)
    {
        if(!__acpi_range_is_mapped(rsdp->xsdt, rsdp->xsdt + sizeof(acpi_header_t), "the xsdt"))
        {
            return -1;
        }
        xsdt = (acpi_xsdt_t*) P2V(rsdp->xsdt);
    }
    else
    {
        if(!__acpi_range_is_mapped(rsdp->rsdt, rsdp->rsdt + sizeof(acpi_header_t), "the rsdt"))
        {
            return -1;
        }
        rsdt = (acpi_rsdt_t*) P2V(rsdp->rsdt);
    }

    return 0;
}

void acpi_power_off()
{
    if(!acpi_is_up)
    {
        kernel_debug_output(KDB_LVL_CRITICAL, "acpi : no namespace, we cannot power off");
        return;
    }

    if(!acpi_is_up)
    {
        kernel_debug_output(KDB_LVL_CRITICAL, "acpi : no namespace, we cannot power off");
        return;
    }

    // Send shutdown command _S5_
    lai_enter_sleep(5);
}

// source : https://wiki.osdev.org/Reboot
void acpi_reboot()
{
    /*  the 8042 line below works without acpi, so no namespace only costs us
        the clean reset and not the reboot itself.  */
    if(acpi_is_up)
    {
        // reset fadt
        lai_acpi_reset();
    }

    // wait
    for(uint32_t i = 0; i < ACPI_8042_WAIT && (in_byte(0x64) & 0x02); i++)
    {
    }

    // send impulse 8042on reset line
    out_byte(0x64, 0xFE);
}

int init_acpi()
{
    if(__acpi_find_rsdp() != 0)
    {
        return -1;
    }

    lai_set_acpi_revision(rsdp->revision);
    lai_create_namespace();
    lai_enable_acpi(1);

    acpi_is_up = 1;

    acpi_is_up = 1;

    return 0;
}