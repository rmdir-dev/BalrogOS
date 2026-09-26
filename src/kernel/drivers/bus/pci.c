#include "balrog_os/drivers/bus/pci.h"
#include "balrog_os/drivers/bus/pci_command.h"
#include "balrog_os/cpu/ports/ports.h"
#include "klib/io/kprint.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/memory/kheap.h"
#include "klib/data_structure/rbt.h"
#include "klib/data_structure/hash_table.h"

static list_t pci_devices[PCI_MAX_CLASS];

static void __pci_probe_bus(pci_t bus);

static void __pci_check_function(pci_t bus)
{
    pci_device_t* device = vmalloc(sizeof(pci_device_t));
    device->bus = bus;
    // common header.
    device->vendor_id = pci_read_word(bus, PCI_W_VENDOR_ID);
    device->device_id = pci_read_word(bus, PCI_W_DEVICE_ID);
    device->revision_id = pci_read_byte(bus, PCI_B_REVISION_ID);
    device->prog_if = pci_read_byte(bus, PCI_B_PROG_IF);
    device->subclass = pci_read_byte(bus, PCI_B_SUBCLASS);
    device->class = pci_read_byte(bus, PCI_B_CLASS_CODE);
    device->cache_line_size = pci_read_byte(bus, PCI_B_CACHE_LINE_SIZE);
    device->latency_timer = pci_read_byte(bus, PCI_B_LATENCY_TIMER);
    device->header_type = pci_read_byte(bus, PCI_B_HEADER_TYPE);
    device->bist = pci_read_byte(bus, PCI_B_BIST);
    // TODO manage different header type.
    //Base Address
    device->bar[0] = pci_read_dword(bus, PCI_D_BASE_ADDRESS_0);
    device->bar[1] = pci_read_dword(bus, PCI_D_BASE_ADDRESS_1);
    device->bar[2] = pci_read_dword(bus, PCI_D_BASE_ADDRESS_2);
    device->bar[3] = pci_read_dword(bus, PCI_D_BASE_ADDRESS_3);
    device->bar[4] = pci_read_dword(bus, PCI_D_BASE_ADDRESS_4);
    device->bar[5] = pci_read_dword(bus, PCI_D_BASE_ADDRESS_5);

    device->command = pci_read_word(bus, PCI_W_COMMAND);
    device->status = pci_read_word(bus, PCI_W_STATUS);

    device->interrupt_pin = pci_read_byte(bus, PCI_B_INTERRUPT_PIN);
    device->interrupt_line = pci_read_byte(bus, PCI_B_INTERRUPT_LINE);

    device->key = (bus.bus << 16) | (bus.slot << 8) | (bus.func);
    // get the class index, PCI_CLASS_CO_PROCESSOR = 0x40
    // So it'll be at index 20 to keep the array shorter.
    uint8_t class_hash_index = device->class < PCI_MAX_CLASS ? device->class : 20;
    list_insert(&pci_devices[class_hash_index], device->key, device);

    KERNEL_LOG_OK("PCI device: %x vendor: %x class: %x subclass: %x progif: %x",
        device->device_id, device->vendor_id, device->class, device->subclass, device->prog_if);

    /*  the bars and the irq line are what a driver needs to find its
        registers. the day a controller sits somewhere unexpected,
        this is the line that says so.  */
    kernel_debug_output(KDB_LVL_VERBOSE, "pci %d:%d.%d bar 0%x 0%x 0%x 0%x 0%x 0%x",
        device->bus.bus, device->bus.slot, device->bus.func,
        device->bar[0], device->bar[1], device->bar[2],
        device->bar[3], device->bar[4], device->bar[5]);
    kernel_debug_output(KDB_LVL_VERBOSE, "pci %d:%d.%d irq pin %d line %d, command 0%x status 0%x, header 0%x",
        device->bus.bus, device->bus.slot, device->bus.func,
        device->interrupt_pin, device->interrupt_line,
        device->command, device->status, device->header_type);
    // if class is a PCI to PCI bridge
    if((device->class == PCI_CLASS_BRIDGE) && (device->subclass == PCI_SUBCLASS_PCI_TO_PCI_BRIDGE))
    {
        pci_t secondary_bus = { 0, 0, 0 };
        secondary_bus.bus = pci_read_byte(bus, PCI_B_SECONDAY_BUS_NUMBER);
        __pci_probe_bus(secondary_bus);
    }
}

static void __pci_probe_device(pci_t bus)
{
    uint16_t vendor = pci_read_word(bus, PCI_W_VENDOR_ID);

    if(vendor != 0xffff)
    {
        __pci_check_function(bus);
        uint8_t header = pci_read_byte(bus, PCI_B_HEADER_TYPE);

        if((header & PCI_MULTIFUNCTIONAL_DEVICE) != 0)
        {
            for(uint8_t function = 1; function < 8; function++)
            {
                bus.func = function;
                uint16_t fct_vendor = pci_read_word(bus, PCI_W_VENDOR_ID);
                if(fct_vendor != 0xffff)
                {
                    __pci_check_function(bus);
                }
            }
        }
    }
}

static void __pci_probe_bus(pci_t bus)
{
    for(uint8_t device_id = 0; device_id < 32; device_id++)
    {
        bus.slot = device_id;
        __pci_probe_device(bus);
    }
}

uint32_t pci_read_dword(pci_t pci, uint16_t offset)
{
    uint32_t addr = (uint32_t)((pci.bus << 16) |
                    (pci.slot << 11) |
                    (pci.func << 8) |
                    (offset & 0xfc) |
                    ((uint32_t) 0x80000000));

    out_dword(PCI_CONFIG_ADDRESS, addr);
    return in_dword(PCI_CONFIG_DATA);
}

uint16_t pci_read_word(pci_t pci, uint16_t offset)
{
    return (uint16_t) (pci_read_dword(pci, offset & ~0x03) >> ((offset & 0x02) * 8));
}

uint8_t pci_read_byte(pci_t pci, uint16_t offset)
{
    return (uint8_t) (pci_read_word(pci, offset & ~0x01) >> ((offset & 0x01) * 8));
}

void pci_write_dword(pci_t pci, uint32_t value, uint16_t offset)
{
    uint32_t addr = (uint32_t)((pci.bus << 16) |
                    (pci.slot << 11) |
                    (pci.func << 8) |
                    (offset & 0xfc) |
                    ((uint32_t) 0x80000000));

    out_dword(PCI_CONFIG_ADDRESS, addr);
    out_dword(PCI_CONFIG_DATA, value);
}

list_t* pci_get_devices(uint8_t device_type)
{
    uint8_t class_hash_index = device_type < PCI_MAX_CLASS ? device_type : 20;
    if(class_hash_index < PCI_MAX_CLASS)
    {
        return &pci_devices[class_hash_index];
    }
    return NULL;
}

void pci_set_command_register(pci_device_t* device, uint16_t cmd)
{
    uint16_t status = 0x00;
    uint32_t cmdstat = (status << 16) | cmd;
    pci_write_dword(device->bus, cmdstat, PCI_W_COMMAND);
}

int init_pci()
{
    // Check if the PCI bus does exist.
    out_dword(PCI_CONFIG_ADDRESS, 0x80000000);
    if(in_dword(PCI_CONFIG_ADDRESS) != 0x80000000)
    {
        kernel_debug_output(KDB_LVL_ERROR, "pci : no configuration space at 0%x, no bus to walk", PCI_CONFIG_ADDRESS);
    }

    if(in_dword(PCI_CONFIG_ADDRESS) == 0x80000000)
    {
        for(int i = 0; i < PCI_MAX_CLASS; i++)
        {
            list_init(&pci_devices[i]);
        }
        //KERNEL_LOG_INFO("Initialize PCI bus");
        pci_t pci = { 0, 0, 0 };
        if((pci_read_byte(pci, PCI_B_HEADER_TYPE) & PCI_MULTIFUNCTIONAL_DEVICE) == 0)
        {
            /* Single PCI host controller */
            kernel_debug_output(KDB_LVL_INFO, "pci : single host controller");
            __pci_probe_bus(pci);

        } else
        {
            /* Multiple PCI host controllers */
            kernel_debug_output(KDB_LVL_INFO, "pci : multiple host controllers");
            for(uint8_t function = 0; function < 8; function++)
            {
                pci_t host = { 0, 0, function };

                if(pci_read_word(host, PCI_W_VENDOR_ID) == 0xffff)
                {
                    continue;
                }

                pci_t bus = { function, 0, 0 };
                __pci_probe_bus(bus);
            }
        }
    }

    return 0;
}