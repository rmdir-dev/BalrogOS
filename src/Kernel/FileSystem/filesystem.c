#include "BalrogOS/FileSystem/filesystem.h"
#include "BalrogOS/FileSystem/fs_cache.h"
#include "BalrogOS/Drivers/disk/ata.h"
#include "BalrogOS/FileSystem/ext2/ext2.h"
#include "BalrogOS/Memory/memory.h"
#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Memory/vmm.h"
#include "BalrogOS/Memory/pmm.h"
#include <string.h>
#include "lib/IO/kprint.h"

fs_device dev;

int fs_open(const char* name, fs_fd* fd)
{
    return dev.fs->open(&dev, name, fd);
}

int fs_read(uint8_t* buffer, uint64_t len, fs_fd* fd)
{
    return dev.fs->read(&dev, buffer, len, fd);
}

int fs_close(fs_fd* fd)
{
    return dev.fs->close(&dev, fd);
}

int fs_fstat(fs_fd* fd, fs_file_stat* stat)
{
    return dev.fs->stat(&dev, fd, stat);
}

void init_file_system()
{
    init_ata();
    ata_get_boot_device(&dev);
    ext2_probe(&dev);
}