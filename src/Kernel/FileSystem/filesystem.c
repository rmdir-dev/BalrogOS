#include "BalrogOS/FileSystem/filesystem.h"
#include "BalrogOS/FileSystem/ext2/ext2.h"
#include "BalrogOS/FileSystem/fs_cache.h"
#include "BalrogOS/Debug/debug_output.h"
#include "BalrogOS/Drivers/disk/ata.h"
#include "BalrogOS/Tasking/elf/elf.h"
#include "BalrogOS/Memory/memory.h"
#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Memory/vmm.h"
#include "BalrogOS/Memory/pmm.h"
#include <string.h>
#include "klib/IO/kprint.h"

fs_device dev;

int fs_get_file(const char* name, fs_file* file)
{
    fs_fd fd;
    dev.fs->open(&dev, name, &fd);
    fs_file* tmp = fs_cache_get_file(fd.ftable_idx);
    *file = *tmp;
    return 0;
}

int fs_open(const char* name, fs_fd* fd)
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

void init_file_system()
{
    init_ata();
    kmutex_init(&dev.lock);
    kmutex_lock(&dev.lock);
    if(ata_get_boot_device(&dev) != 0)
    {
        KERNEL_LOG_FAIL("file system : No suitable drive found!");
        while(1){}
    }
    ext2_probe(&dev);
    kmutex_unlock(&dev.lock);
}