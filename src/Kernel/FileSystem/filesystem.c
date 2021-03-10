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

void test_print_dir(uint8_t* entires)
{
    fs_dir_entry* entry = entires;
    char* name = vmalloc(255);
    while(entry->inbr)
    {
        memcpy(name, &entry->name, entry->name_len);
        name[entry->name_len] = 0;
        kprint("%s \n", name);
        entires += entry->entry_size;
        entry = entires;
    }
}

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

void init_file_system()
{
    init_ata();
    ata_get_boot_device(&dev);
    ext2_probe(&dev);
    char* buffer = fs_cache_get_new_buffer(PAGE_SIZE * 14);
    //dev.fs->list(&dev, "/", buffer);
    //dev.fs->list(&dev, "/boot", buffer);
    fs_fd fd;
    dev.fs->open(&dev, "/", &fd);
    memset(buffer, 0, PAGE_SIZE * 14);
    dev.fs->read(&dev, buffer, 4096, &fd);
    test_print_dir(buffer);
    dev.fs->close(&dev, &fd);
}