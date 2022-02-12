#include "BalrogOS/Drivers/Disk/ahci/ahci.h"
#include "BalrogOS/Drivers/Disk/ahci/ahci_command.h"
#include "BalrogOS/Drivers/Disk/ahci/ahci_structures.h"
#include "BalrogOS/Drivers/Disk/ata/ata_device.h"
#include "BalrogOS/Drivers/Bus/pci_class.h"
#include "BalrogOS/Drivers/Bus/pci.h"
#include "BalrogOS/Debug/debug_output.h"
#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Memory/vmm.h"
#include "BalrogOS/Memory/pmm.h"
#include "BalrogOS/Memory/memory.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include <string.h>

static list_t ahci_devices;

/*
    INTERRUPT HANDLING
*/

interrupt_regs* ahci_handler(interrupt_regs* stack_frame)
{
    return stack_frame;
}

/*
    AHCI 
*/

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

static void __ahci_sata_ident(ahci_device_t* dev)
{
    uint8_t* buff = kmalloc(ATA_SECTOR_SIZE);

    ahci_cmd_header_t* header = &dev->cmd_list->cmd[0];
    header->cflen = sizeof(fis_device_reg_t) / sizeof(uint32_t);
    header->prdtlen = 1;

    ahci_cmd_table_t* table = dev->cmd_table;
    uintptr_t* addr = &table->prdt_entry[0].dba;
    *addr = buff;
    table->prdt_entry[0].dbc = ATA_SECTOR_SIZE - 1;

    fis_device_reg_t* fis = &table->cfis[0];
    fis->fis_type = FIS_TYPE_REG_H2D;
    fis->is_cmd = 1;
    fis->command = ATA_CMD_IDENT_DEV;
}

static int __ahci_port_rebase(ahci_device_t* dev, uint32_t port_no)
{
    dev->port_no = port_no;

    __ahci_cmd_stop(dev->port);
    
    ahci_cmd_list_t* cmd_list = kmalloc(PAGE_SIZE);
    memset(cmd_list, 0, PAGE_SIZE);
    dev->cmd_list = cmd_list;

    uintptr_t* clb = &dev->port->clb;
    *clb = V2P(cmd_list);


    uintptr_t* fis = kmalloc(PAGE_SIZE);
    memset(fis, 0, PAGE_SIZE);
    dev->fis = fis;

    uintptr_t* fb = &dev->port->fb;
    *fb = V2P(fis);

    ahci_cmd_table_t* cmd_table = kmalloc(PAGE_SIZE * 2);
    memset(cmd_table, 0, PAGE_SIZE * 2);
    dev->cmd_table = cmd_table;

    for(size_t i = 0; i < 32; i++)
    {
        cmd_list->cmd[i].prdtlen = 8;
        uintptr_t* c_table = &cmd_list->cmd[i].ctba;
        *c_table = &cmd_table[i];
    }

    __ahci_cmd_start(dev->port);

    KERNEL_LOG_INFO("port : 0%x initialized.", dev->key);

    __ahci_sata_ident(dev);

    while(1){}
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

static ahci_device_t* __ahci_create_device(pci_device_t* dev, hba_mem_t* hba, hba_port_t* port)
{
    if(!(hba->cap & AHCI_64_BIT_CAP))
    {
        return 0;
    }

    ahci_device_t* device = kmalloc(sizeof(ahci_device_t));

    device->abar = hba;
    device->port = port;
    device->key = dev->key;
    device->pci = dev;
    device->cap_64_bit = (hba->cap & 1 << 31 ? 1 : 0);

    list_insert(&ahci_devices, device->key);

    return device;
}

static int __ahci_probe_ports(pci_device_t* dev, hba_mem_t* hba)
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
                    ahci_device_t* device = __ahci_create_device(dev, hba, &hba->ports[i]);

                    return __ahci_port_rebase(device, i);
                    break;

                case AHCI_DEVICE_SATAPI:
                    KERNEL_LOG_INFO("AHCI device found : type SATAPI (unsupported)");
                    break;

                case AHCI_DEVICE_SEMB:
                    KERNEL_LOG_INFO("AHCI device found : type enclosure management bridge (unsupported)");
                    break;

                case AHCI_DEVICE_PM:
                    KERNEL_LOG_INFO("AHCI device found : type port multiplier (unsupported)");
                    break;
                
                default:
                    break;
            }
        }
        pi >>= 1;
        i++;
    }
    return -1;
}

static int __ahci_check_device(pci_device_t* dev, hba_mem_t* hba)
{
    // Check if device is of AHCI type
    if(hba->ghc & GHC_AHCI_ENABLED)
    {
        // Make sure that interrupts are enabled
        hba->ghc |= GHC_INTERRUPT_ENABLED;
        KERNEL_LOG_INFO("==================== HBA INFO ====================");
        KERNEL_LOG_INFO("HBA host capability       : 0%x", hba->cap);
        KERNEL_LOG_INFO("HBA host 64bit capability : %s", (hba->cap & 1 << 31 ? "true" : "false"));
        KERNEL_LOG_INFO("HBA global host control   : 0%x", hba->ghc);
        KERNEL_LOG_INFO("HBA port implemented      : 0%b", hba->pi);
        KERNEL_LOG_INFO("HBA version               : 0%x", hba->vs);
        KERNEL_LOG_INFO("==================== END INFO ====================");

        return __ahci_probe_ports(dev, hba);
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
            KERNEL_LOG_INFO("PCI device id       : 0%x", dev->device_id);
            KERNEL_LOG_INFO("PCI vendor id       : 0%x", dev->vendor_id);
            KERNEL_LOG_INFO("PCI cmd register    : 0%b", dev->command);
            KERNEL_LOG_INFO("PCI interrupt pin   : 0%x", dev->interrupt_pin);
            KERNEL_LOG_INFO("PCI interrupt line  : 0%x", dev->interrupt_line);
            KERNEL_LOG_INFO("PCI AHCI ABAR addr  : 0%p", dev->bar[5]);

            hba_mem_t* hba = (void*)P2V(dev->bar[5]);

            vmm_set_page(0, hba, dev->bar[5], PAGE_PRESENT | PAGE_WRITE);

            if(__ahci_check_device(dev, hba) == 0)
            {
                register_interrupt_handler(dev->interrupt_line, ahci_handler);
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