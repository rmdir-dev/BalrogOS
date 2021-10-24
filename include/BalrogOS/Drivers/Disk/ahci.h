#pragma once

#include <stdint.h>

#include "BalrogOS/FileSystem/filesystem.h"

/*
Advance Host Controller Interface

Documentation :
    AHCI : https://wiki.osdev.org/AHCI
*/

/**
 * @brief set the boot device.
 * 
 * @param device 
 */
int ahci_get_boot_device(fs_device_t* device);

/**
 * @brief initialize AHCI drives if they exist.
 * 
 */
void init_ahci();