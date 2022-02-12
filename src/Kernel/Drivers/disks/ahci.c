#include "BalrogOS/Drivers/Disk/ahci/ahci.h"
#include "BalrogOS/Drivers/Disk/ahci/ahci_structures.h"
#include "BalrogOS/Drivers/Bus/pci_class.h"
#include "BalrogOS/Drivers/Bus/pci.h"
#include "BalrogOS/Debug/debug_output.h"
#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Memory/vmm.h"
#include "BalrogOS/Memory/pmm.h"
#include "BalrogOS/Memory/memory.h"

static list_t ahci_devices;

static void __ahci_cmd_start(hba_port_t *port)
{
	// Wait until CR (bit15) is cleared
	while (port->cmd & HBA_PORT_CMD_CR)
	{}
 
	// Set FRE (bit4) and ST (bit0)
	port->cmd |= HBA_PORT_CMD_FRE;
	port->cmd |= HBA_PORT_CMD_ST; 
}
 
static void __ahci_cmd_stop(hba_port_t *port)
{
	// Clear ST (bit0)
	port->cmd &= ~HBA_PORT_CMD_ST;
 
	// Clear FRE (bit4)
	port->cmd &= ~HBA_PORT_CMD_FRE;
 
	// Wait until FR (bit14), CR (bit15) are cleared
	while((port->cmd & HBA_PORT_CMD_FR) || (port->cmd & HBA_PORT_CMD_CR))
	{}
}

static int __ahci_port_rebase(hba_port_t *port, uint32_t port_no)
{
    __ahci_cmd_stop(port);
    return 0;
}

static int __ahci_get_port_type(hba_port_t* port)
{
    uint32_t ssts = port->ssts;
    uint8_t ipm = (ssts >> 8) & 0x0F;
    uint8_t det = ssts & 0x0F;

    if(det != HBA_PORT_DET_PRESENT || ipm != HBA_PORT_IPM_ACTIVE)
    {
        return AHCI_DEVICE_NULL;
    }
    
    switch (port->sig)
    {
        case SATA_SIG_ATA:
            return AHCI_DEVICE_SATA;
            break;
        case SATA_SIG_ATAPI:
            return AHCI_DEVICE_SATAPI;
            break;
        case SATA_SIG_SEMB:
            return AHCI_DEVICE_SEMB;
            break;
        case SATA_SIG_PM:
            return AHCI_DEVICE_PM;
            break;
        
        default:
            return AHCI_DEVICE_NULL;
            break;
    }
}

static int __ahci_probe_ports(hba_mem_t* hba)
{
    uint32_t pi = hba->pi;
    uint32_t i = 0;
    while(i < 32)
    {
        if(pi & 1)
        {
            int device_type = __ahci_get_port_type(&hba->ports[i]);

            switch (device_type)
            {
                case AHCI_DEVICE_SATA:
                    KERNEL_LOG_INFO("AHCI device found : type SATA");
                    break;

                case AHCI_DEVICE_SATAPI:
                    KERNEL_LOG_INFO("AHCI device found : type SATAPI");
                    break;

                case AHCI_DEVICE_SEMB:
                    KERNEL_LOG_INFO("AHCI device found : type enclosure management bridge");
                    break;

                case AHCI_DEVICE_PM:
                    KERNEL_LOG_INFO("AHCI device found : type port multiplier");
                    break;
                
                default:
                    break;
            }
        }
        pi >>= 1;
        i++;
    }
    return 0;
}

static int __ahci_check_device(hba_mem_t* hba)
{
    // Check if device is of AHCI type
    if(hba->ghc & GHC_AHCI_ENABLED)
    {
        // Make sure that interrupts are enabled
        hba->ghc |= GHC_INTERRUPT_ENABLED;
        KERNEL_LOG_INFO("HBA host capability : 0%x", hba->cap);
        KERNEL_LOG_INFO("HBA global host control : 0%x", hba->ghc);
        KERNEL_LOG_INFO("HBA port implemented : 0%b", hba->pi);
        KERNEL_LOG_INFO("HBA version : 0%x", hba->vs);

        return __ahci_probe_ports(hba);
    }
    return -1;
}

static int __ahci_probe_device(pci_device_t* dev)
{
    // Check if device is of subclass ATA CONTROLLER.
    if(dev->subclass == PCI_SUBCLASS_STRG_SERIAL_ATA_CONTROLLER)
    {
        // check if device contain AHCI address
        // The BAR[5] point to AHCI base memory called ABAR (AHCI Base Memory Register).
        if(dev->bar[5])
        {
            KERNEL_LOG_INFO("AHCI suitable PCI device found");
            KERNEL_LOG_INFO("PCI device id : 0%x", dev->device_id);
            KERNEL_LOG_INFO("PCI vendor id : 0%x", dev->vendor_id);
            KERNEL_LOG_INFO("PCI cmd register : 0%b", dev->command);
            KERNEL_LOG_INFO("PCI interrupt pin : 0%x", dev->interrupt_pin);
            KERNEL_LOG_INFO("PCI interrupt line : 0%x", dev->interrupt_line);
            KERNEL_LOG_INFO("PCI AHCI ABAR addr : 0%p", dev->bar[5]);

            ahci_device_t* device = kmalloc(sizeof(ahci_device_t));

            device->abar = (void*)P2V(dev->bar[5]);
            device->key = dev->key;

            vmm_set_page(0, device->abar, dev->bar[5], PAGE_PRESENT | PAGE_WRITE);

            hba_mem_t* hba = device->abar;
            if(__ahci_check_device(hba) == 0)
            {
                list_insert(&ahci_devices, device->key);
            }
        }
    }
    return 0;
}

void init_ahci()
{
    KERNEL_LOG_INFO("Looking for AHCI devices");
    
    list_init(&ahci_devices);

    // Get all pci MASS STORAGES devices
    list_t* list = pci_get_devices(PCI_CLASS_MASS_STORAGE_CONTROLLER);
    list_node_t* node = list->head;
    while(node)
    {
        pci_device_t* dev = node->value;
        __ahci_probe_device(dev);
        node = node->next;
    }
    while(1){}
}

int get_ahci_boot_device()
{
    for(list_node_t* node = ahci_devices.head; node; node = node->next)
    {
        
    }
    return -1;
}