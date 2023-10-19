#include "BalrogOS/Drivers/Disk/ahci/ahci.h"
#include "BalrogOS/Drivers/Bus/pci_class.h"
#include "BalrogOS/Drivers/Bus/pci.h"
#include "BalrogOS/Debug/debug_output.h"

static list_t devices;

void init_ahci()
{
    list_init(&devices);

    list_t* list = pci_get_devices(PCI_CLASS_MASS_STORAGE_CONTROLLER);
    list_node_t* node = list->head;
    while(node)
    {
        pci_device_t* dev = node->value;
        if(dev->subclass == PCI_SUBCLASS_STRG_SERIAL_ATA_CONTROLLER)
        {
            KERNEL_LOG_INFO("AHCI suitable PCI device found");
            KERNEL_LOG_INFO("AHCI device : 0%x", dev->device_id);
            KERNEL_LOG_INFO("AHCI vendor : 0%x", dev->vendor_id);
            KERNEL_LOG_INFO("AHCI addr : 0%p", dev->bar[5]);
            list_insert(dev, dev->key);
        }
        node = node->next;
    }
}

int get_ahci_boot_device()
{
    for(list_node_t* node = devices.head; node; node = node->next)
    {

    }
    return -1;
}