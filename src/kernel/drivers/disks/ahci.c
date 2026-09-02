#include "balrog_os/drivers/disk/ahci/ahci.h"
#include "balrog_os/drivers/disk/ahci/ahci_command.h"
#include "balrog_os/drivers/disk/ahci/ahci_structures.h"
#include "balrog_os/drivers/disk/ata/ata_device.h"
#include "balrog_os/drivers/bus/pci_class.h"
#include "balrog_os/drivers/bus/pci.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/memory/kheap.h"
#include "balrog_os/memory/vmm.h"
#include "balrog_os/memory/pmm.h"
#include "balrog_os/memory/memory.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include <string.h>

static list_t ahci_devices;

/*
    INTERRUPT HANDLING
*/

interrupt_regs* ahci_handler(interrupt_regs* stack_frame)
{
    /*  the driver polls, the per port interrupt enable registers are still
        left to 0. we acknowledge every device anyway so a shared interrupt
        line can't storm. the port status has to be cleared before the
        global one, and both are write 1 to clear.
    */
    for(list_node_t* node = ahci_devices.head; node; node = node->next)
    {
        ahci_device_t* device = node->value;
        hba_mem_t* hba = device->abar;

        device->port->is = device->port->is;
        hba->is = 1 << device->port_no;
    }

    return stack_frame;
}

/*  the HBA takes a 64 bit address as two consecutive 32 bit registers,
    the low part first then the high part.
*/
static void __ahci_set_addr(volatile uint32_t* reg, uintptr_t addr)
{
    reg[0] = (uint32_t)addr;
    reg[1] = (uint32_t)(addr >> 32);
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

static int __find_free_cmd_slot(ahci_device_t* dev)
{
    uint32_t slots = (dev->port->sact | dev->port->ci);

    for(size_t i = 0; i < AHCI_CMD_SLOTS; i++)
    {
        if((slots & 1) == 0)
        {
            return i;
        }
        slots >>= 1;
    }
    return -1;
}

/*  the command tables are spread over AHCI_CMD_TABLE_PAGES pages, none of
    them crossing a page boundary so each one stays physically contiguous.
*/
static ahci_cmd_table_t* __ahci_get_cmd_table(ahci_device_t* dev, uint32_t slot)
{
    uint8_t* page = dev->cmd_table_page[slot / AHCI_CMD_TABLE_PER_PAGE];
    return (void*)(page + (slot % AHCI_CMD_TABLE_PER_PAGE) * sizeof(ahci_cmd_table_t));
}

/*  build the command in a free slot, issue it and wait for the HBA to be
    done with it. command->buffer is a physical address.
*/
static int __ahci_send_command(ahci_device_t* dev, ahci_cmd_t* command)
{
    int slot = __find_free_cmd_slot(dev);

    if(slot < 0)
    {
        KERNEL_LOG_FAIL("no command slot available.");
        return -1;
    }

    ahci_cmd_header_t* header = &dev->cmd_list->cmd[slot];
    memset(header, 0, sizeof(ahci_cmd_header_t));
    header->cflen = sizeof(fis_device_reg_t) / sizeof(uint32_t);
    header->w = command->write;
    header->prdtlen = 1;

    ahci_cmd_table_t* table = __ahci_get_cmd_table(dev, slot);
    __ahci_set_addr(&header->ctba, V2P(table));

    memset(&table->prdt_entry[0], 0, sizeof(ahci_prdt_entry_t));
    __ahci_set_addr(&table->prdt_entry[0].dba, command->buffer);
    table->prdt_entry[0].dbc = command->size - 1;
    table->prdt_entry[0].i = 0;

    fis_device_reg_t* fis = (void*)&table->cfis[0];
    memset(fis, 0, sizeof(fis_device_reg_t));
    fis->fis_type = FIS_TYPE_REG_H2D;
    fis->is_cmd = 1;
    fis->command = command->command;
    fis->device = command->device;

    fis->lba0 = command->lba & 0xff;
    fis->lba1 = (command->lba >> 8) & 0xff;
    fis->lba2 = (command->lba >> 16) & 0xff;
    fis->lba3 = (command->lba >> 24) & 0xff;
    fis->lba4 = (command->lba >> 32) & 0xff;
    fis->lba5 = (command->lba >> 40) & 0xff;

    fis->count_low = command->count & 0xff;
    fis->count_high = (command->count >> 8) & 0xff;

    // reset interrupt and error flags. both are write 1 to clear.
    dev->port->is = dev->port->is;
    dev->port->serr = dev->port->serr;

    int to_counter = 0;
    while((dev->port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && to_counter < AHCI_TIMEOUT)
    {
        to_counter++;
    }

    if(to_counter == AHCI_TIMEOUT)
    {
        KERNEL_LOG_FAIL("port is hung.");
        return -1;
    }

    dev->port->ci = 1 << slot;

    to_counter = 0;
    while(dev->port->ci & (1 << slot))
    {
        if(dev->port->is & HBA_PxIS_TFES || to_counter++ == AHCI_TIMEOUT)
        {
            KERNEL_LOG_FAIL("command 0%x failed.", command->command);
            return -1;
        }
    }

    if(dev->port->is & HBA_PxIS_TFES)
    {
        KERNEL_LOG_FAIL("command 0%x ended on a task file error.", command->command);
        return -1;
    }

    return 0;
}

/*  the buffers handed over by the file system sit on the kernel stack or in
    the virtual heap, so they are neither aligned nor physically contiguous.
    every transfer bounces through the device DMA page, AHCI_DMA_SECTORS
    sectors at a time.
*/
static int __ahci_read_sata(ahci_device_t* dev, uint8_t* buffer, uint64_t lba, uint64_t len)
{
    while(len)
    {
        uint64_t count = len > AHCI_DMA_SECTORS ? AHCI_DMA_SECTORS : len;

        ahci_cmd_t command = {
            .command = ATA_CMD_READ_DMA_EXT,
            .device = AHCI_DEV_LBA_MODE,
            .write = 0,
            .count = count,
            .lba = lba,
            .buffer = (uintptr_t)dev->dma_buffer,
            .size = count * ATA_SECTOR_SIZE
        };

        if(__ahci_send_command(dev, &command))
        {
            return -1;
        }

        memcpy(buffer, (void*)P2V(dev->dma_buffer), count * ATA_SECTOR_SIZE);

        buffer += count * ATA_SECTOR_SIZE;
        lba += count;
        len -= count;
    }

    return 0;
}

static int __ahci_write_sata(ahci_device_t* dev, uint8_t* buffer, uint64_t lba, uint64_t len)
{
    while(len)
    {
        uint64_t count = len > AHCI_DMA_SECTORS ? AHCI_DMA_SECTORS : len;

        memcpy((void*)P2V(dev->dma_buffer), buffer, count * ATA_SECTOR_SIZE);

        ahci_cmd_t command = {
            .command = ATA_CMD_WRITE_DMA_EXT,
            .device = AHCI_DEV_LBA_MODE,
            .write = 1,
            .count = count,
            .lba = lba,
            .buffer = (uintptr_t)dev->dma_buffer,
            .size = count * ATA_SECTOR_SIZE
        };

        if(__ahci_send_command(dev, &command))
        {
            return -1;
        }

        buffer += count * ATA_SECTOR_SIZE;
        lba += count;
        len -= count;
    }

    return 0;
}

static int __ahci_sata_ident(ahci_device_t* dev)
{
    KERNEL_LOG_INFO("AHCI IDENT SATA");

    ahci_cmd_t command = {
        .command = ATA_CMD_IDENT_DEV,
        .device = 0,
        .write = 0,
        .count = 0,
        .lba = 0,
        .buffer = (uintptr_t)dev->dma_buffer,
        .size = ATA_SECTOR_SIZE
    };

    if(__ahci_send_command(dev, &command))
    {
        KERNEL_LOG_FAIL("could not initialize device");
        return -1;
    }

    ata_id* ident = (void*)P2V(dev->dma_buffer);
    KERNEL_LOG_INFO("AHCI sata device : %s %dMiB", &ident->model[0], (ident->capacity_lba48 / 1024) / 2);

    dev->initialized = 1;

    return 0;
}

static int __ahci_port_rebase(ahci_device_t* dev, uint32_t port_no)
{
    dev->port_no = port_no;

    __ahci_cmd_stop(dev->port);

    /*  the HBA wants the command list 1KiB aligned, the received FIS 256
        bytes aligned and every command table 128 bytes aligned. kmalloc
        aligns nothing, so everything the controller reads comes from the
        pmm, which hands out page aligned pages.
    */
    void* cmd_list = pmm_calloc();
    void* fis = pmm_calloc();
    void* dma_buffer = pmm_calloc();

    if(!cmd_list || !fis || !dma_buffer)
    {
        KERNEL_LOG_FAIL("could not allocate the port structures.");
        return -1;
    }

    dev->dma_buffer = dma_buffer;

    dev->cmd_list = (void*)P2V(cmd_list);
    __ahci_set_addr(&dev->port->clb, (uintptr_t)cmd_list);

    dev->fis = (void*)P2V(fis);
    __ahci_set_addr(&dev->port->fb, (uintptr_t)fis);

    for(size_t i = 0; i < AHCI_CMD_TABLE_PAGES; i++)
    {
        void* page = pmm_calloc();

        if(!page)
        {
            KERNEL_LOG_FAIL("could not allocate the command tables.");
            return -1;
        }
        dev->cmd_table_page[i] = (void*)P2V(page);
    }

    for(size_t i = 0; i < AHCI_CMD_SLOTS; i++)
    {
        dev->cmd_list->cmd[i].prdtlen = 8;
        __ahci_set_addr(&dev->cmd_list->cmd[i].ctba, V2P(__ahci_get_cmd_table(dev, i)));
    }

    // clear the error register before the port is started again.
    dev->port->serr = dev->port->serr;

    __ahci_cmd_start(dev->port);

    return __ahci_sata_ident(dev);
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
    device->initialized = 0;

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
                    if(!device)
                    {
                        KERNEL_LOG_FAIL("AHCI device is not 64 bit capable");
                        return -1;
                    }

                    // __ahci_port_rebase() returns 0 when the device answered.
                    if(__ahci_port_rebase(device, i) == 0)
                    {
                        list_node_t* node = list_insert(&ahci_devices, device->key);
                        node->value = device;
                        return 0;
                    }
                    return -1;
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
                    // KERNEL_LOG_FAIL("Device is null.");
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

            /*  the ABAR holds the generic host control then up to 32 port
                register sets, 0x100 + 32 * 0x80 bytes, so it doesn't fit in
                a single page. it is memory mapped io, it must not be cached.
            */
            for(size_t i = 0; i < AHCI_ABAR_PAGES; i++)
            {
                vmm_set_page(0, (void*)hba + i * PAGE_SIZE, (void*)(uintptr_t)(dev->bar[5] + i * PAGE_SIZE), PAGE_PRESENT | PAGE_WRITE | PAGE_NOCACHE);
            }

            if(__ahci_check_device(dev, hba) == 0)
            {
                register_interrupt_handler(dev->interrupt_line, ahci_handler);
            }
        }
    }
    return 0;
}

void ahci_read(fs_device_t* device, uint8_t* buffer, uint64_t lba, uint64_t len)
{
    __ahci_read_sata(device->drive, buffer, lba, len);
}

void ahci_write(fs_device_t* device, uint8_t* buffer, uint64_t lba, uint64_t len)
{
    __ahci_write_sata(device->drive, buffer, lba, len);
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
}

int ahci_get_boot_device(fs_device_t* device)
{
    uint16_t* buffer = vmalloc(ATA_SECTOR_SIZE);

    for(list_node_t* node = ahci_devices.head; node; node = node->next)
    {
        ahci_device_t* drive = node->value;

        if(!drive->initialized)
        {
            continue;
        }

        KERNEL_LOG_INFO("searching boot device. port %d", drive->port_no);

        if(!__ahci_read_sata(drive, (void*)buffer, 0, 1))
        {
            // the boot sector ends with the MBR signature.
            if(buffer[255] == 0xaa55)
            {
                KERNEL_LOG_INFO("boot device found!");
                device->unique_id = drive->key;
                device->read = ahci_read;
                device->write = ahci_write;
                device->drive = drive;
                vmfree(buffer);
                return 0;
            }
        }
    }
    vmfree(buffer);
    return -1;
}