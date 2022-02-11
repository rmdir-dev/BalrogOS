#pragma once

#include <stdint.h>

#include "ahci_structures.h"
#include "BalrogOS/FileSystem/filesystem.h"

/*
Advance Host Controller Interface

Documentation :
    AHCI : https://wiki.osdev.org/AHCI
*/

#define AHCI_DEVICE_NULL        0x00    // No Drive
#define AHCI_DEVICE_SATA        0x01    // SATA drive
#define AHCI_DEVICE_SEMB        0x02    // Enclosure management bridge
#define AHCI_DEVICE_PM          0x03    // Port multiplier
#define AHCI_DEVICE_SATAPI      0x04    // SATAPI drive

#define HBA_PORT_IPM_ACTIVE     0x01
#define HBA_PORT_DET_PRESENT    0x03

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