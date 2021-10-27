#include "BalrogOS/Drivers/Bus/pci.h"
#include "BalrogOS/CPU/Ports/ports.h"
#include "klib/IO/kprint.h"
#include "BalrogOS/Debug/debug_output.h"
#include "BalrogOS/Memory/kheap.h"
#include "klib/DataStructure/rbt.h"
#include "klib/DataStructure/hash_table.h"

list_t pci_devices[PCI_MAX_CLASS];

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

    device->key = (bus.bus << 16) | (bus.slot << 8) | (bus.func);
    // get the class index, PCI_CLASS_CO_PROCESSOR = 0x40
    // So it'll be at index 20 to keep the array shorter.
    uint8_t class_hash_index = device->class < PCI_MAX_CLASS ? device->class : 20;
    list_node_t* node = list_insert(&pci_devices[class_hash_index], device->key);
    node->value = device;
    KERNEL_LOG_OK("PCI device: %x vendor: %x class: %x subclass: %x progif: %x", 
        device->device_id, device->vendor_id, device->class, device->subclass, device->prog_if);
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

void init_pci()
{
    // Check if the PCI bus does exist.
    out_dword(PCI_CONFIG_ADDRESS, 0x80000000);
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
            __pci_probe_bus(pci);

        } else
        {
            /* Multiple PCI host controllers */
            for(uint8_t function = 0; function < 8; function++)
            {
                pci.func = function;
                if(pci_read_word(pci, PCI_W_VENDOR_ID) != 0xffff)
                {
                    break;
                }
                __pci_probe_bus(pci);
            }
        }
    }
}