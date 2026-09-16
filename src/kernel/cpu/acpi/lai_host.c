#include <lai/host.h>
#include <lai/core.h>

#include "balrog_os/cpu/pit/pit.h"
#include "balrog_os/cpu/tsc/tsc.h"
#include "balrog_os/cpu/ports/ports.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/drivers/bus/pci.h"
#include "balrog_os/memory/kheap.h"
#include "balrog_os/memory/memory.h"

#include <stddef.h>
#include <string.h>

#include "balrog_os/memory/vmm.h"

/*
The functions lai asks us to provide.

References :
LAI : https://wiki.osdev.org/LAI
the list of functions : lai/include/lai/host.h
*/

extern acpi_xsdp_t* rsdp;
extern acpi_xsdt_t* xsdt;
extern acpi_rsdt_t* rsdt;

extern uint8_t interrupt_enabled;


extern int __acpi_is_mapped(uintptr_t phys);
extern int __acpi_checksum(acpi_header_t *header);

/*
MEMORY
*/

void* laihost_malloc(size_t size)
{
    return vmalloc(size);
}

void* laihost_realloc(void* ptr, size_t newsize, size_t oldsize)
{
    if(!ptr)
    {
        return vmalloc(newsize);
    }

    if(newsize == 0)
    {
        vmfree(ptr);
        return 0;
    }

    void* copy = vmalloc(newsize);

    if(!copy)
    {
        return 0;
    }

    memcpy(copy, ptr, newsize < oldsize ? newsize : oldsize);
    vmfree(ptr);

    return copy;
}

void laihost_free(void* ptr, size_t size)
{
    vmfree(ptr);
}

/*
SCAN
*/


/*
source : https://wiki.osdev.org/RSDT and https://wiki.osdev.org/XSDT
*/
static size_t __acpi_entry_count()
{
    if(xsdt)
    {
        return (xsdt->header.length - sizeof(acpi_header_t)) / 8;
    }

    return (rsdt->header.length - sizeof(acpi_header_t)) / 4;
}

static uintptr_t __acpi_entry(size_t i)
{
    if(xsdt)
    {
        return (uintptr_t) xsdt->tables[i];
    }

    return (uintptr_t) rsdt->tables[i];
}

void* laihost_scan(const char* signature, size_t index)
{
    if(memcmp(signature, "DSDT", 4) == 0)
    {
        acpi_fadt_t* fadt = laihost_scan("FACP", 0);

        if(!fadt)
        {
            return 0;
        }

        uint64_t dsdt = fadt->dsdt;

        if(fadt->header.length >= offsetof(acpi_fadt_t, x_dsdt) + sizeof(fadt->x_dsdt)
           && fadt->x_dsdt)
        {
            dsdt = fadt->x_dsdt;
        }

        return (void*) P2V(dsdt);
    }

    size_t found = 0;

    if(!xsdt && !rsdt)
    {
        return 0;
    }

    size_t count = __acpi_entry_count();

    for(size_t i = 0; i < count; i++)
    {
        uintptr_t entry = __acpi_entry(i);

        if(!__acpi_is_mapped(entry))
        {
            continue;
        }

        acpi_header_t* h = (acpi_header_t*) P2V(entry);

        if(memcmp(h->signature, signature, 4) != 0)
        {
            continue;
        }

        //  a table that does not checksum is not a table
        if(!__acpi_checksum(h))
        {
            continue;
        }

        if(found == index)
        {
            return h;
        }

        found++;
    }

    return 0;
}


/*
DIAGNOSTIC
*/

void laihost_log(int level, const char* msg)
{
    kernel_debug_output(level == LAI_WARN_LOG ? KDB_LVL_ERROR : KDB_LVL_INFO, "lai : %s", msg);
}

void laihost_panic(const char* msg)
{
    kernel_debug_output(KDB_LVL_CRITICAL, "lai panic : %s", msg);

    while(1)
    {
    }
}

/*
PORT IO
*/

void laihost_outb(uint16_t port, uint8_t value)
{
    out_byte(port, value);
}

void laihost_outw(uint16_t port, uint16_t value)
{
    out_word(port, value);
}

void laihost_outd(uint16_t port, uint32_t value)
{
    out_dword(port, value);
}

uint8_t laihost_inb(uint16_t port)
{
    return in_byte(port);
}

uint16_t laihost_inw(uint16_t port)
{
    return in_word(port);
}

uint32_t laihost_ind(uint16_t port)
{
    return in_dword(port);
}


/*
PCI CONFIGURATION SPACE
*/

static pci_t __lai_pci(uint8_t bus, uint8_t slot, uint8_t fun)
{
    pci_t pci = { .bus = bus, .slot = slot, .func = fun };

    return pci;
}

uint8_t laihost_pci_readb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset)
{
    return pci_read_byte(__lai_pci(bus, slot, fun), offset);
}

uint16_t laihost_pci_readw(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset)
{
    return pci_read_word(__lai_pci(bus, slot, fun), offset);
}

uint32_t laihost_pci_readd(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset)
{
    return pci_read_dword(__lai_pci(bus, slot, fun), offset);
}

void laihost_pci_writed(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset,
                        uint32_t value)
{
    pci_write_dword(__lai_pci(bus, slot, fun), value, offset);
}

void laihost_pci_writew(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset,
                        uint16_t value)
{
    pci_t pci = __lai_pci(bus, slot, fun);

    uint16_t aligned = offset & ~3;
    uint32_t shift = (offset & 2) * 8;
    uint32_t dword = pci_read_dword(pci, aligned);

    dword = (dword & ~(0xFFFFu << shift)) | ((uint32_t) value << shift);

    pci_write_dword(pci, dword, aligned);
}

void laihost_pci_writeb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset,
                        uint8_t value)
{
    pci_t pci = __lai_pci(bus, slot, fun);

    uint16_t aligned = offset & ~3;
    uint32_t shift = (offset & 3) * 8;
    uint32_t dword = pci_read_dword(pci, aligned);

    dword = (dword & ~(0xFFu << shift)) | ((uint32_t) value << shift);

    pci_write_dword(pci, dword, aligned);
}

/*
MEMORY MAPPED IO
*/

void* laihost_map(size_t address, size_t count)
{
    uintptr_t start = address & ~(uintptr_t)(PAGE_SIZE - 1);
    uintptr_t end = (address + count + PAGE_SIZE - 1) & ~(uintptr_t)(PAGE_SIZE - 1);

    for(uintptr_t p = start; p < end; p += PAGE_SIZE)
    {
        if(vmm_get_page(0, (void*) P2V(p)))
        {
            continue;
        }

        /*  PAGE_NOCACHE because what is behind is a device and not memory.
            Reading a cached copy of a status register does not give us what
            the device would have answered.  */
        vmm_set_page(0, (void*) P2V(p), (void*) p,
            PAGE_PRESENT | PAGE_WRITE | PAGE_NOCACHE);
    }

    return (void*) P2V(address);
}

void laihost_unmap(void* pointer, size_t count)
{

}


/*
source : https://github.com/managarm/lai, exec.c on the Timer opcode
*/
uint64_t laihost_timer(void)
{
    return tsc_read() / tsc_per_100ns();
}

void laihost_sleep(uint64_t ms)
{
    /*  a millisecond is ten thousand times 100ns  */
    tsc_wait_100ns(ms * 10000);
}
