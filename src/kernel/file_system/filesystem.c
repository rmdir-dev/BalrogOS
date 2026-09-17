#include "balrog_os/file_system/ext2/ext2_cache/ext2_cache.h"
#include "balrog_os/file_system/filesystem.h"
#include "balrog_os/file_system/ext2/ext2.h"
#include "balrog_os/file_system/fs_cache.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/drivers/disk/ahci/ahci.h"
#include "balrog_os/drivers/disk/ata/ata.h"
#include "balrog_os/drivers/disk/ramfs/ramfs.h"
#include "balrog_os/tasking/elf/elf.h"
#include "balrog_os/memory/memory.h"
#include "balrog_os/memory/kheap.h"
#include "balrog_os/memory/vmm.h"
#include "balrog_os/memory/pmm.h"
#include <string.h>
#include "klib/io/kprint.h"

fs_device_t boot_dev;
fs_device_t ramfs_dev;

static const size_t boot_lookout_len = 3;

extern int ata_get_boot_device(fs_device_t* device);
extern int ahci_get_boot_device(fs_device_t* device);

// ramdisk first as we will use it to load the real root disk later on
static int (*fs_boot_lookout[])(fs_device_t*) =
{
    [0] &ramdisk_get_boot_device,
    [1] &ata_get_boot_device,
    [2] &ahci_get_boot_device,
};

static const char* boot_lookout_name[] = {
    "ramdisk",
    "ata",
    "ahci"
};

int fs_get_file(const char* name, fs_file* file, fs_fd* fd)
{
    size_t len = strlen(name);
    char* fname = vmalloc(len + 1);
    memcpy(fname, name, len);
    fname[len] = 0;

    int ret = boot_dev.fs->open(&boot_dev, fname, fd);

    vmfree(fname);

    if(ret != 0)
    {
        return -1;
    }

    fs_file* tmp = fs_cache_get_file(fd->ftable_idx);

    if(!tmp)
    {
        return -1;
    }

    *file = *tmp;

    return 0;
}

int fs_open(char* name, fs_fd* fd)
{
    kmutex_lock(&boot_dev.lock);
    int ret = boot_dev.fs->open(&boot_dev, name, fd);
    kmutex_unlock(&boot_dev.lock);
    return ret;
}

int fs_read(uint8_t* buffer, uint64_t len, fs_fd* fd)
{
    kmutex_lock(&boot_dev.lock);
    int ret = boot_dev.fs->read(&boot_dev, buffer, len, fd);
    kmutex_unlock(&boot_dev.lock);
    return ret;
}

int fs_write(uint8_t* buffer, uint64_t len, fs_fd* fd)
{
    kmutex_lock(&boot_dev.lock);
    int ret = boot_dev.fs->write(&boot_dev, buffer, len, fd);
    kmutex_unlock(&boot_dev.lock);
    return ret;
}

int fs_touch(char* filename)
{
    return fs_create(filename, 0);
}

int fs_create(char* filename, uint64_t size)
{
    kmutex_lock(&boot_dev.lock);
    int ret = boot_dev.fs->create(&boot_dev, filename, size);
    kmutex_unlock(&boot_dev.lock);
    return ret;
}

int fs_mkdir(char* dirname)
{
    kmutex_lock(&boot_dev.lock);
    int ret = boot_dev.fs->mkdir(&boot_dev, dirname);
    kmutex_unlock(&boot_dev.lock);
    return ret;
}

int fs_unlink(char* filename)
{
    kmutex_lock(&boot_dev.lock);
    int ret = boot_dev.fs->unlink(&boot_dev, filename);
    kmutex_unlock(&boot_dev.lock);
    return ret;
}

int fs_rmdir(char* dirname)
{
    kmutex_lock(&boot_dev.lock);
    int ret = boot_dev.fs->rmdir(&boot_dev, dirname);
    kmutex_unlock(&boot_dev.lock);
    return ret;
}

int fs_close(fs_fd* fd)
{
    kmutex_lock(&boot_dev.lock);
    int ret = boot_dev.fs->close(&boot_dev, fd);
    kmutex_unlock(&boot_dev.lock);
    return ret;
}

int fs_fstat(fs_fd* fd, fs_file_stat* stat)
{
    kmutex_lock(&boot_dev.lock);
    int ret = boot_dev.fs->stat(&boot_dev, fd, stat);
    kmutex_unlock(&boot_dev.lock);
    return ret;
}

static int __root_device_lookout()
{
    for(size_t i = 0; i < boot_lookout_len; i++)
    {
        kernel_debug_output(KDB_LVL_INFO, "file system : asking %s for a boot device",
                i < 3 ? boot_lookout_name[i] : "?");

        if(fs_boot_lookout[i](&boot_dev) == 0)
        {
            kernel_debug_output(KDB_LVL_INFO, "file system : %s answered, unique id %d",
                    i < 3 ? boot_lookout_name[i] : "?", boot_dev.unique_id);
            return 0;
        }
    }

    kernel_debug_output(KDB_LVL_ERROR, "file system : none of the %d drivers found a boot device", boot_lookout_len);
    return -1;
}

int init_file_system()
{
    init_ata();
    init_ahci();
    ramdisk_init((void*)P2V(RAMFS_PHYS), RAMFS_SIZE);

    kmutex_init(&boot_dev.lock);
    kmutex_lock(&boot_dev.lock);

    if(__root_device_lookout(&boot_dev) != 0)
    {
        kmutex_unlock(&boot_dev.lock);
        KERNEL_LOG_FAIL("file system : No suitable drive found!");
        while(1){}
    }

    /*
    Initialize ext2 cache datastructures.
    */
    ext2_cache_init();
    ext2_probe(&boot_dev);
    kmutex_unlock(&boot_dev.lock);

    return 0;
}