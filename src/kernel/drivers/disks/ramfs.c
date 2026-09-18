#include "balrog_os/drivers/disk/ramfs/ramfs.h"

#include "balrog_os/debug/debug_output.h"
#include "balrog_os/file_system/ext2/ext2.h"
#include "balrog_os/memory/memory.h"
#include "balrog_os/memory/pmm.h"

static ramdisk_t ramfs;

int __ramdisk_read(fs_device_t* dev, uint8_t* buffer, uint64_t lba, uint64_t len)
{
    ramdisk_t* ramdisk = dev->drive;
    uint64_t offset = lba * 512;

    if (offset + len * 512 > ramdisk->size)
    {
        kernel_debug_output(KDB_LVL_CRITICAL, "ramdisk : read past the end, lba 0%x len 0%x", lba, len);
        return -1;
    }

    memcpy(buffer, ramdisk->base + offset, len * 512);

    return 0;
}

int __ramdisk_write(fs_device_t* dev, uint8_t* buffer, uint64_t lba, uint64_t len)
{
    ramdisk_t* ramdisk = dev->drive;
    uint64_t offset = lba * 512;

    if (offset + len * 512 > ramdisk->size)
    {
        kernel_debug_output(KDB_LVL_CRITICAL, "ramdisk : write past the end, lba 0%x len 0%x", lba, len);
        return -1;
    }

    memcpy(ramdisk->base + offset, buffer, len * 512);

    return 0;
}

int ramdisk_init(void* base, uint64_t size)
{
    ramfs.base = base;
    ramfs.size = size;

    /*
    E820 calls this plain usable memory, so we must claim it before
    the physical allocator hands it out.
    */
    pmm_reserve((void*)V2P(base), size);

    return 0;
}

int ramdisk_get_boot_device(fs_device_t* device)
{
    if(!ramfs.base || !ramfs.size)
    {
        return -1;
    }

    /* TODO: make ext2_probe return an error if not found atm we check it here (botch) */
    ext2_superblock* sb = (ext2_superblock*)(ramfs.base + 1024);

    if(sb->ext2_signature != EXT2_SIGNATURE)
    {
        KERNEL_LOG_INFO("ramdisk : nothing ext2 at %p, falling through", ramfs.base);
        return -1;
    }

    device->name = "ramfs";
    device->read = __ramdisk_read;
    device->write = __ramdisk_write;
    device->type = FS_DEVICE_TYPE_RAMFS;
    device->drive = &ramfs;
    device->gpt_header = NULL;
    device->gpt_partition = NULL;
    fs_add_device(device);

    KERNEL_LOG_OK("ramdisk : ext2 image at %p, %d MiB", ramfs.base, ramfs.size / (1024 * 1024));

    return 0;
}
