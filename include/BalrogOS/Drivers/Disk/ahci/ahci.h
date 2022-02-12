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

#define	SATA_SIG_ATA	        0x00000101	// SATA drive
#define	SATA_SIG_ATAPI	        0xEB140101	// SATAPI drive
#define	SATA_SIG_SEMB	        0xC33C0101	// Enclosure management bridge
#define	SATA_SIG_PM	            0x96690101	// Port multiplier
 
#define HBA_PORT_CMD_ST         0x0001
#define HBA_PORT_CMD_FRE        0x0010
#define HBA_PORT_CMD_FR         0x4000
#define HBA_PORT_CMD_CR         0x8000

#define GHC_AHCI_ENABLED        (1 << 31)
#define GHC_INTERRUPT_ENABLED   (1 << 1)
#define GHC_HBA_RESET           (1)

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