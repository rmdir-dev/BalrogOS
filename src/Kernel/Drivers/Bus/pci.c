#include "BalrogOS/Drivers/Bus/pci.h"
#include "BalrogOS/CPU/Ports/ports.h"
#include "klib/IO/kprint.h"

static void __pci_probe_bus(uint8_t bus);

static void __pci_check_function(pci_t device)
{
    uint8_t class = pci_read_byte(device, PCI_B_CLASS_CODE);
    uint8_t subclass = pci_read_byte(device, PCI_B_SUBCLASS);

    // if class is a PCI to PCI bridge
    if((class == PCI_CLASS_BRIDGE) && (subclass == PCI_SUBCLASS_PCI_TO_PCI_BRIDGE))
    {
        uint8_t secondary_bus = pci_read_byte(device, PCI_B_SECONDAY_BUS_NUMBER);
        __pci_probe_bus(secondary_bus);
    }
}

static void __pci_probe_device(uint8_t bus, uint8_t device)
{
    pci_t pci_device = { bus, device, 0 };
    uint16_t vendor = pci_read_word(pci_device, PCI_W_VENDOR_ID);

    if(vendor != 0xffff)
    {
        uint8_t header = pci_read_byte(pci_device, PCI_B_HEADER_TYPE);

        if((header & PCI_MULTIFUNCTIONAL_DEVICE) != 0)
        {
            for(uint8_t function = 1; function < 8; function++)
            {
                pci_t fct_dev = { bus, device, function };
                uint16_t fct_vendor = pci_read_word(fct_dev, PCI_W_VENDOR_ID);
                if(fct_vendor != 0xffff)
                {
                    __pci_check_function(fct_dev);
                }
            }
        }
    }
}

static void __pci_probe_bus(uint8_t bus)
{
    for(uint8_t device_id; device_id < 32; device_id++)
    {
        __pci_probe_device(bus, device_id);
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

void init_pci()
{
    // Check if the PCI bus does exist.
    out_dword(PCI_CONFIG_ADDRESS, 0x80000000);
    if(in_dword(PCI_CONFIG_ADDRESS) == 0x80000000)
    {       
        pci_t pci = { 0, 0, 0 };
        if((pci_read_byte(pci, PCI_B_HEADER_TYPE) & PCI_MULTIFUNCTIONAL_DEVICE) == 0)
        {
            /* Single PCI host controller */
            __pci_probe_bus(pci.bus);

        } else
        {
            /* Multiple PCI host controllers */
            for(uint8_t function = 0; function < 8; function++)
            {
                pci_t fct_pci = { pci.bus, pci.slot, function };
                if(pci_read_word(fct_pci, PCI_W_VENDOR_ID) != 0xffff)
                {
                    break;
                }
                __pci_probe_bus(function);
            }
        }
        while(1){}
    }
}