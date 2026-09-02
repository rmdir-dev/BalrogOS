#include "balrog_os/file_system/ext2/ext2_cache/ext2_cache.h"
#include "balrog_os/file_system/filesystem.h"
#include "balrog_os/file_system/ext2/ext2.h"
#include "balrog_os/file_system/fs_cache.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/drivers/disk/ahci/ahci.h"
#include "balrog_os/drivers/disk/ata/ata.h"
#include "balrog_os/tasking/elf/elf.h"
#include "balrog_os/memory/memory.h"
#include "balrog_os/memory/kheap.h"
#include "balrog_os/memory/vmm.h"
#include "balrog_os/memory/pmm.h"
#include <string.h>
#include "klib/io/kprint.h"

fs_device_t dev;

static const size_t boot_lookout_len = 2;

extern int ata_get_boot_device(fs_device_t* device);
extern int ahci_get_boot_device(fs_device_t* device);

static int (*fs_boot_lookout[])(fs_device_t*) = 
{
    [0] &ata_get_boot_device,
    [1] &ahci_get_boot_device,
};

int fs_get_file(const char* name, fs_file* file, fs_fd* fd)
{
    size_t len = strlen(name);
    char* fname = vmalloc(len + 1);
    memcpy(fname, name, len);
    fname[len] = 0;
    dev.fs->open(&dev, fname, fd);
    fs_file* tmp = fs_cache_get_file(fd->ftable_idx);
    *file = *tmp;
    vmfree(fname);
    return 0;
}

int fs_open(char* name, fs_fd* fd)
{
    kmutex_lock(&dev.lock);
    int ret = dev.fs->open(&dev, name, fd);
    kmutex_unlock(&dev.lock);
    return ret;
}

int fs_read(uint8_t* buffer, uint64_t len, fs_fd* fd)
{
    kmutex_lock(&dev.lock);
    int ret = dev.fs->read(&dev, buffer, len, fd);
    kmutex_unlock(&dev.lock);
    return ret;
}

int fs_write(uint8_t* buffer, uint64_t len, fs_fd* fd)
{
    kmutex_lock(&dev.lock);
    int ret = dev.fs->write(&dev, buffer, len, fd);
    kmutex_unlock(&dev.lock);
    return ret;
}

int fs_touch(char* filename)
{
    kmutex_lock(&dev.lock);
    int ret = dev.fs->touch(&dev, filename);
    kmutex_unlock(&dev.lock);
    return ret;
}

int fs_mkdir(char* dirname)
{
    kmutex_lock(&dev.lock);
    int ret = dev.fs->mkdir(&dev, dirname);
    kmutex_unlock(&dev.lock);
    return ret;
}

int fs_unlink(char* filename)
{
    kmutex_lock(&dev.lock);
    int ret = dev.fs->unlink(&dev, filename);
    kmutex_unlock(&dev.lock);
    return ret;
}

int fs_rmdir(char* dirname)
{
    kmutex_lock(&dev.lock);
    int ret = dev.fs->rmdir(&dev, dirname);
    kmutex_unlock(&dev.lock);
    return ret;
}

int fs_close(fs_fd* fd)
{
    kmutex_lock(&dev.lock);
    int ret = dev.fs->close(&dev, fd);
    kmutex_unlock(&dev.lock);
    return ret;
}

int fs_fstat(fs_fd* fd, fs_file_stat* stat)
{
    kmutex_lock(&dev.lock);
    int ret = dev.fs->stat(&dev, fd, stat);
    kmutex_unlock(&dev.lock);
    return ret;
}

static int __root_device_lookout()
{
    for(size_t i = 0; i < boot_lookout_len; i++)
    {
        if(fs_boot_lookout[i](&dev) == 0)
        {
            return 0;
        }    
    }

    return -1;
}

int init_file_system()
{
    init_ata();
    init_ahci();
    // while(1)
    // {}
    kmutex_init(&dev.lock);
    kmutex_lock(&dev.lock);
    
    if(__root_device_lookout(&dev) != 0)
    {
        kmutex_unlock(&dev.lock);
        KERNEL_LOG_FAIL("file system : No suitable drive found!");
        while(1){}
    }

    /*
    Initialize ext2 cache datastructures.
    */
    ext2_cache_init();
    ext2_probe(&dev);
    kmutex_unlock(&dev.lock);

    return 0;
}