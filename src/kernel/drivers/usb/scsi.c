#include "balrog_os/drivers/usb/usb_spec.h"
#include "balrog_os/drivers/usb/usb_storage.h"
#include "balrog_os/debug/debug_output.h"
#include <endian.h>
#include <string.h>

extern int __usb_bot_command(usb_disk_t* disk, uint8_t* cdb, uint8_t cdb_len, void* buffer, uint32_t len, uint8_t dir);

int __scsi_rw10(usb_disk_t* disk, uint64_t lba, uint16_t blocks,
        void* buffer, uint8_t write)
{
    uint8_t cdb[10] = {};

    cdb[0] = write ? SCSI_WRITE_10 : SCSI_READ_10;

    uint32_t be_lba = htobe32((uint32_t) lba);
    uint16_t be_blocks = htobe16(blocks);

    memcpy(&cdb[2], &be_lba, sizeof(be_lba));
    memcpy(&cdb[7], &be_blocks, sizeof(be_blocks));

    int ret = __usb_bot_command(disk, cdb, sizeof(cdb), buffer,
            blocks * disk->block_size,
            write ? USB_CBW_DIR_OUT : USB_CBW_DIR_IN);

    if(ret != 0)
    {
        kernel_debug_output(KDB_LVL_ERROR, "usb : %s of %d blocks at lba %d failed",
                write ? "write" : "read", blocks, (uint32_t) lba);
    }

    return ret;
}

int __scsi_read_capacity(usb_disk_t* disk)
{
    uint8_t cdb[10] = { SCSI_READ_CAPACITY_10 };
    uint8_t answer[8] = {};

    if(__usb_bot_command(disk, cdb, sizeof(cdb), answer, sizeof(answer), USB_CBW_DIR_IN) != 0)
    {
        return -1;
    }

    uint32_t last_block = 0;
    uint32_t block_size = 0;

    memcpy(&last_block, &answer[0], sizeof(last_block));
    memcpy(&block_size, &answer[4], sizeof(block_size));

    disk->last_block = be32toh(last_block);
    disk->block_size = be32toh(block_size);

    kernel_debug_output(KDB_LVL_INFO, "usb : %d blocks of %d bytes, %d MiB",
            disk->last_block + 1, disk->block_size,
            ((disk->last_block + 1) / 1024) * disk->block_size / 1024);

    if(!disk->block_size)
    {
        return -1;
    }

    return 0;
}

