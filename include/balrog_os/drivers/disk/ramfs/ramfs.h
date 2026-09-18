#pragma once
#include <stdint.h>
#include "balrog_os/file_system/filesystem.h"

/*
Where stage 2 dropped the ramfs image, and how much of it there is.

Keep in sync with src/bootloader/common/layout.inc : RAMFS_PHYS and RAMFS_SECTORS.
*/
#define RAMFS_PHYS  0x10000000              // 256 MiB
#define RAMFS_SIZE  (16384 * 512)           // RAMFS_SECTORS * 512, so 8 MiB

typedef struct __ramdisk_t
{
    uint8_t* base; // start of the image in memory
    uint64_t size;
} __attribute__((__packed__)) ramdisk_t;

/**
 * @brief
 *
 * @param base
 * @param size
 *
 * @return
 */
int ramdisk_init(void* base, uint64_t size);

/**
 * @brief fill a device with the ramdisk, the way ata and ahci do
 *
 * @param device
 *
 * @return
 */
int ramdisk_get_boot_device(fs_device_t* device);