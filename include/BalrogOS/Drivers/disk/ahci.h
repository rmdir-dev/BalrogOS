#pragma once

#include "BalrogOS/FileSystem/filesystem.h"

#include <stdint.h>

/**
 * @brief set the boot device.
 * 
 * @param device 
 */
int ahci_get_boot_device(fs_device* device);

/**
 * @brief initialize AHCI drives if they exist.
 * 
 */
void init_ahci();