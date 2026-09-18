#pragma once

#include "balrog_os/file_system/filesystem.h"
#include <stdint.h>


/**
 * @brief set the boot device.
 * 
 * @param device 
 */
int ata_get_boot_device(fs_device_t* device);

/**
 * @brief initialize ATA drives
 * 
 */
int init_ata();