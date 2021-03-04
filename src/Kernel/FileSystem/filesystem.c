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
    vmm_set_page(0, PHYSICAL_TO_VIRTUAL(pmm_top_addr) + (4096 * 3), pmm_calloc(), PAGE_PRESENT | PAGE_WRITE);
    vmm_set_page(0, PHYSICAL_TO_VIRTUAL(pmm_top_addr) + (4096 * 4), pmm_calloc(), PAGE_PRESENT | PAGE_WRITE);
    vmm_set_page(0, PHYSICAL_TO_VIRTUAL(pmm_top_addr) + (4096 * 5), pmm_calloc(), PAGE_PRESENT | PAGE_WRITE);
    vmm_set_page(0, PHYSICAL_TO_VIRTUAL(pmm_top_addr) + (4096 * 6), pmm_calloc(), PAGE_PRESENT | PAGE_WRITE);
    vmm_set_page(0, PHYSICAL_TO_VIRTUAL(pmm_top_addr) + (4096 * 7), pmm_calloc(), PAGE_PRESENT | PAGE_WRITE);
    vmm_set_page(0, PHYSICAL_TO_VIRTUAL(pmm_top_addr) + (4096 * 8), pmm_calloc(), PAGE_PRESENT | PAGE_WRITE);
    vmm_set_page(0, PHYSICAL_TO_VIRTUAL(pmm_top_addr) + (4096 * 9), pmm_calloc(), PAGE_PRESENT | PAGE_WRITE);
    vmm_set_page(0, PHYSICAL_TO_VIRTUAL(pmm_top_addr) + (4096 * 10), pmm_calloc(), PAGE_PRESENT | PAGE_WRITE);
    vmm_set_page(0, PHYSICAL_TO_VIRTUAL(pmm_top_addr) + (4096 * 11), pmm_calloc(), PAGE_PRESENT | PAGE_WRITE);
    vmm_set_page(0, PHYSICAL_TO_VIRTUAL(pmm_top_addr) + (4096 * 12), pmm_calloc(), PAGE_PRESENT | PAGE_WRITE);
    vmm_set_page(0, PHYSICAL_TO_VIRTUAL(pmm_top_addr) + (4096 * 13), pmm_calloc(), PAGE_PRESENT | PAGE_WRITE);
    vmm_set_page(0, PHYSICAL_TO_VIRTUAL(pmm_top_addr) + (4096 * 14), pmm_calloc(), PAGE_PRESENT | PAGE_WRITE);
    dev.fs->list(&dev, "/", buffer);
    dev.fs->list(&dev, "/boot", buffer);
    //dev.fs->open(&dev, "/filesys.c", buffer);
    printf("dir listed\n");
}