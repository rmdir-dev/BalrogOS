#include "BalrogOS/FileSystem/filesystem.h"
#include "BalrogOS/FileSystem/fs_cache.h"
#include "BalrogOS/Drivers/disk/ata.h"
#include "BalrogOS/FileSystem/ext2/ext2.h"
#include "BalrogOS/Memory/memory.h"
#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Memory/vmm.h"
#include "BalrogOS/Memory/pmm.h"
#include <string.h>
#include <stdio.h>

extern uintptr_t pmm_top_addr;

void init_file_system()
{
    init_ata();
    fs_device dev;
    ata_get_boot_device(&dev);
    ext2_probe(&dev);
    char* buffer = fs_cache_get_new_buffer(PAGE_SIZE * 14);
    dev.fs->list(&dev, "/", buffer);
    dev.fs->list(&dev, "/boot", buffer);
    fs_fd fd;
    dev.fs->open(&dev, "/filesys.c", &fd);
    buffer[19] = 0;
    dev.fs->read(&dev, buffer, 19, &fd);
    printf("%s \n", buffer);
    dev.fs->close(&dev, &fd);
}