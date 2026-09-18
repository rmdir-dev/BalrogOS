#include "balrog_os/drivers/disk/ata/ata.h"
#include "balrog_os/drivers/disk/ata/ata_device.h"
#include "balrog_os/drivers/disk/ata/ata_command.h"
#include "balrog_os/file_system/fs_devices.h"
#include "balrog_os/cpu/ports/ports.h"
#include "balrog_os/memory/kheap.h"

#include "balrog_os/debug/debug_output.h"
#include "balrog_os/file_system/gpt/gpt.h"

char ata_disk_id = 'a';
ata_drive drives[4];

static inline void __ata_read_buffer(uint16_t io_bus, uint16_t* buf)
{
    for(size_t i = 0; i < 256; i++)
    {
        buf[i] = in_word(ATA_REG_R_DATA(io_bus));
    }
}

static inline void __ata_write_buffer(uint16_t io_bus, uint16_t* buf)
{
    for(size_t i = 0; i < 256; i++)
    {
        out_word(ATA_REG_W_DATA(io_bus), buf[i]);
    }
}

static int __ata_wait_400ns(uint16_t bus)
{
    in_byte(ATA_REG_R_STATUS(bus));
    in_byte(ATA_REG_R_STATUS(bus));
    in_byte(ATA_REG_R_STATUS(bus));
    in_byte(ATA_REG_R_STATUS(bus));
    return in_byte(ATA_REG_R_STATUS(bus));
}

/*
Wait for the busy bit to drop, and say whether it actually did.
*/
static int __ata_wait_busy(uint16_t bus)
{
    for(uint32_t i = 0; i < ATA_WAIT_LIMIT; i++)
    {
        uint8_t status = in_byte(ATA_REG_R_STATUS(bus));

        if(status == ATA_FLOATING_BUS)
        {
            return 0;
        }

        if(!(status & ATA_STATUS_BSY))
        {
            return 1;
        }
    }

    return 0;
}

static int __ata_flush_cache(ata_drive* drive)
{
    out_byte(ATA_REG_W_CMD(drive->io_bus), 0xe0 | drive->master);
    __ata_wait_400ns(drive->io_bus);
    out_byte(ATA_REG_W_CMD(drive->io_bus), ATA_CMD_CACHE_FLUSH);

    return __ata_wait_busy(drive->io_bus) ? 0 : -1;
}

static uint8_t __ata_send_command(ata_cmd* command)
{
    // select the device
    out_byte(ATA_REG_W_DEV(command->bus), command->device | (command->lba_sep[3] &0xf));
    __ata_wait_400ns(command->bus);

    out_byte(ATA_REG_W_S_COUNT(command->bus), command->count);
    out_byte(ATA_REG_W_LBA_L(command->bus), command->lba_sep[0]);
    out_byte(ATA_REG_W_LBA_M(command->bus), command->lba_sep[1]);
    out_byte(ATA_REG_W_LBA_H(command->bus), command->lba_sep[2]);

    if(!__ata_wait_busy(command->bus))
    {
        return 0;
    }

    out_byte(ATA_REG_W_CMD(command->bus), command->command);

    if(!in_byte(ATA_REG_R_STATUS(command->bus)))
    {
        return 0;
    }

    command->status = __ata_wait_400ns(command->bus);

    if(!__ata_wait_busy(command->bus))
    {
        return 0;
    }

    command->status = in_byte(ATA_REG_R_STATUS(command->bus));

    if(command->wait_status)
    {
        uint32_t spins = 0;

        while(!(command->status = in_byte(ATA_REG_R_STATUS(command->bus)) & (command->wait_status | ATA_STATUS_ERR)))
        {
            if(++spins >= ATA_WAIT_LIMIT)
            {
                return 0;
            }
        }
    }

    command->error = in_byte(ATA_REG_R_ERROR(command->bus));
    command->count = in_byte(ATA_REG_R_S_COUNT(command->bus));
    command->device = in_byte(ATA_REG_R_DEV(command->bus));
    command->lba_sep[0] = in_byte(ATA_REG_R_LBA_L(command->bus));
    command->lba_sep[1] = in_byte(ATA_REG_R_LBA_M(command->bus));
    command->lba_sep[2] = in_byte(ATA_REG_R_LBA_H(command->bus));
    command->lba_sep[3] = 0;

    return command->status;
}

static void __ata_init_drive(ata_drive* drive)
{
    uint16_t io_bus = drive->io_bus;

    /*  nothing decodes this port, so there is no controller on this bus. */
    if(in_byte(ATA_REG_R_STATUS(io_bus)) == ATA_FLOATING_BUS)
    {
        return;
    }

    /* select the drive */
    out_byte(ATA_REG_W_DEV(io_bus), 0xa0 | drive->master);

    out_byte(ATA_REG_W_S_COUNT(io_bus), 0);
    out_byte(ATA_REG_W_LBA_L(io_bus), 0);
    out_byte(ATA_REG_W_LBA_M(io_bus), 0);
    out_byte(ATA_REG_W_LBA_H(io_bus), 0);

    // do a software rest
    out_byte(ATA_REG_W_CTR(drive->ctr_bus), ATA_CTR_SRST);
    // clear the reset bit
    out_byte(ATA_REG_W_CTR(drive->ctr_bus), 0);
    
    // get logical block address
    uint64_t lba = (in_byte(ATA_REG_R_LBA_H(io_bus)) << 16) | (in_byte(ATA_REG_R_LBA_H(io_bus)) << 8) | in_byte(ATA_REG_R_LBA_L(io_bus));
    
    /* prepare the first command to be sent, 
    the first command will wake up the sleeping drives */
    ata_cmd command = {
        .bus = io_bus,
        .device = 0xa0 | drive->master,
        .command = ATA_CMD_IDENT_DEV,
        .wait_status = ATA_STATUS_DRQ
    };
    //command.bus = io_bus;
    //command.device = 0xa0 | drive->master;
    //command.command = ATA_CMD_IDENT_DEV;
    //command.wait_status = ATA_STATUS_DRQ;

    /* check if the drive is ATAPI */
    kernel_debug_output(KDB_LVL_VERBOSE, "ata : bus 0%x answered lba 0%x after the reset", io_bus, lba);

    if(lba == ATAPI_LBA_MAGIC)
    {
        kernel_debug_output(KDB_LVL_INFO, "ata : bus 0%x holds an atapi drive", io_bus);
        drive->atapi = 1;
        command.command = ATA_CMD_IDENT_PCK_DEV;
    }

    // send the command if the device is asleep
    // the first command will wake it up.
    if(!__ata_send_command(&command))
    {
        // Command and status are a union in ata_cmd, so the structure
        // has nothing left to say once the call gave up so we read
        // the registers instead.
        kernel_debug_output(KDB_LVL_ERROR, "ata : bus 0%x did not answer IDENTIFY, status 0%x error 0%x",
                io_bus, in_byte(ATA_REG_R_STATUS(io_bus)), in_byte(ATA_REG_R_ERROR(io_bus)));
        return;
    }

    // fill the buffer with the content asked by the previous command.
    __ata_read_buffer(io_bus, (void*) &drive->id);

    KERNEL_LOG_OK("drive 0%x %s load successfully.", io_bus, drive->master == 0x10 ? "slave" : "master");

    drive->exist = 1;
}

static inline int __ata_read_sector(ata_drive* device, uint16_t* buffer, uint64_t lba)
{
    // number of tries.
    int retries = 5;
    while(retries--)
    {
        // create a command
        ata_cmd command = {
            .bus = device->io_bus,
            .count = 1,
            .lba = lba & 0xffffff,
            .device = 0xe0 | device->master | ((lba >> 24) & 0xf),
            .command = ATA_CMD_READ_SEC_RETRY
        };

        // send the command
        int status = __ata_send_command(&command);

        // if the command failed to read then retry 
        if(status & (ATA_STATUS_DF | ATA_STATUS_ERR) || !(status & ATA_STATUS_DRQ))
        {
            continue;
        }
        
        // fill the buffer
        __ata_read_buffer(device->io_bus, buffer);
        return 0;
    }
    return -1;
}

int __ata_read(fs_device_t* device, uint8_t* buffer, uint64_t lba, uint64_t len)
{
    ata_drive* drive = &drives[device->unique_id];
    for(size_t i = 0; i < len; i++)
    {
        if(__ata_read_sector(drive, (uint16_t*)buffer, get_first_lba(device) + lba + i) != 0)
        {
            return -1;
        }
        buffer += 512;
    }
    return 0;
}

static inline int __ata_write_sector(ata_drive* device, uint8_t* buffer, uint64_t lba)
{
    // number of tries.
    int retries = 5;

    while(retries--)
    {
        // create a command
        ata_cmd command = {
            .bus = device->io_bus,
            .count = 1,
            .lba = lba & 0xffffff,
            .device = 0xe0 | device->master | ((lba >> 24) & 0xf),
            .command = ATA_CMD_WRITE_SEC_RETRY,
        };

        // send the command
        int status = __ata_send_command(&command);

        // if the command failed to read then retry
        if(status & (ATA_STATUS_DF | ATA_STATUS_ERR) || !(status & ATA_STATUS_DRQ))
        {
            continue;
        }

        // fill the buffer
        __ata_write_buffer(device->io_bus, buffer);

        if (!__ata_wait_busy(device->io_bus))
        {
            continue;
        }

        if(in_byte(ATA_REG_R_STATUS(device->io_bus)) & (ATA_STATUS_DF | ATA_STATUS_ERR))
        {
            continue;
        }

        return 0;
    }
    return -1;
}

int __ata_write(fs_device_t* device, uint8_t* buffer, uint64_t lba, uint64_t len)
{
    ata_drive* drive = &drives[device->unique_id];

    for(size_t i = 0; i < len; i++)
    {
        if(__ata_write_sector(drive, (uint16_t*) buffer, get_first_lba(device) + lba + i) != 0)
        {
            kernel_debug_output(KDB_LVL_ERROR, "ata : write failed at lba %d",
                    get_first_lba(device) + lba + i);
            return -1;
        }

        buffer += 512;
    }

    __ata_flush_cache(drive);

    return 0;
}


int ata_scan_devices()
{
    int boot_found = -1;
    uint16_t* buffer = vmalloc(512);
    for(size_t i = 0; i < 4; i++)
    {
        KERNEL_LOG_INFO("searching boot device. %d", i);

        if(!drives[i].exist)
        {
            kernel_debug_output(KDB_LVL_VERBOSE, "ata : drive %d does not exist, skipped", i);
            continue;
        }
        if(!__ata_read_sector(&drives[i], buffer, 0))
        {
            fs_device_t* device = vmalloc(sizeof(fs_device_t));

            fs_device_init(device);
            device->name = vmalloc(4 + 1); // sda + NULL byte + 1 buffer byte
            memcpy(device->name, "sd", 2);
            device->type = FS_DEVICE_TYPE_ATA;
            device->name[2] = ata_disk_id++;
            device->name[3] = 0; // nullbyte
            device->first_lba = 0;
            device->unique_id = i;
            device->read = __ata_read;
            device->write = __ata_write;
            device->drive = &drives[i];
            fs_add_device(device);
        }
    }

    vmfree(buffer);
    return boot_found;
}

int init_ata()
{
    KERNEL_LOG_INFO("Looking for ATA devices.");
    drives[0].io_bus = ATA_DEV_IO_PREMARY;
    drives[0].ctr_bus = ATA_DEV_CTR_PRIMARY;
    drives[0].exist = 0;
    drives[0].master = ATA_MASTER;
    __ata_init_drive(&drives[0]);

    drives[1].io_bus = ATA_DEV_IO_PREMARY;
    drives[1].ctr_bus = ATA_DEV_CTR_PRIMARY;
    drives[1].exist = 0;
    drives[1].master = ATA_SLAVE;
    __ata_init_drive(&drives[1]);

    drives[2].io_bus = ATA_DEV_IO_SECONDARY;
    drives[2].ctr_bus = ATA_DEV_CTR_SECONDARY;
    drives[2].exist = 0;
    drives[2].master = ATA_MASTER;
    __ata_init_drive(&drives[2]);

    drives[3].io_bus = ATA_DEV_IO_SECONDARY;
    drives[3].ctr_bus = ATA_DEV_CTR_SECONDARY;
    drives[3].exist = 0;
    drives[3].master = ATA_SLAVE;
    __ata_init_drive(&drives[3]);

    return 0;
}