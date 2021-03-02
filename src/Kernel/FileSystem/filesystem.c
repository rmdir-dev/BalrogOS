#include "BalrogOS/FileSystem/filesystem.h"
#include "BalrogOS/Drivers/disk/ata.h"
#include "BalrogOS/FileSystem/ext2/ext2.h"
#include "BalrogOS/Memory/memory.h"
#include "BalrogOS/Memory/vmm.h"
#include "BalrogOS/Memory/pmm.h"
#include <string.h>

extern uintptr_t pmm_top_addr;

void init_file_system()
{
    init_ata();
    fs_device dev;
    ata_get_boot_device(&dev);
    ext2_probe(&dev);
    char* buffer = PHYSICAL_TO_VIRTUAL(pmm_top_addr) + 4096;
    vmm_set_page(0, PHYSICAL_TO_VIRTUAL(pmm_top_addr) + 4096, pmm_calloc(), PAGE_PRESENT | PAGE_WRITE);
    vmm_set_page(0, PHYSICAL_TO_VIRTUAL(pmm_top_addr) + (4096 * 2), pmm_calloc(), PAGE_PRESENT | PAGE_WRITE);
    printf("buffer : 0%p\n", buffer);
    dev.fs->list(&dev, "/", buffer);
    dev.fs->list(&dev, "/boot", buffer);
    printf("dir listed\n");
}