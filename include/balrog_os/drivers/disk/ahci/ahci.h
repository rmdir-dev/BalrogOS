#pragma once

#include <stdint.h>

#include "ahci_structures.h"
#include "balrog_os/drivers/disk/ata/ata_device.h"
#include "balrog_os/file_system/filesystem.h"

/*
Advance Host Controller Interface

Documentation :
    AHCI : 
        https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/serial-ata-ahci-spec-rev1-3-1.pdf
        https://wiki.osdev.org/AHCI
        http://kurtqiao.github.io/uefi/2014/12/24/AHCI-mode.html
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

#define AHCI_64_BIT_CAP         (1 << 31)

#define AHCI_ABAR_PAGES         2           // 0x100 + 32 * 0x80 bytes of registers
#define AHCI_TIMEOUT            1000000     // polling loop bail out
#define AHCI_DEV_LBA_MODE       (1 << 6)    // FIS device register, LBA mode
#define AHCI_DMA_SECTORS        (PAGE_SIZE / ATA_SECTOR_SIZE)   // sectors carried by one command

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

/**
 * @brief read sectors from an AHCI drive.
 * 
 * @param device 
 * @param buffer where the sectors are copied
 * @param lba first sector to read
 * @param len number of sectors
 */
void ahci_read(fs_device_t* device, uint8_t* buffer, uint64_t lba, uint64_t len);

/**
 * @brief write sectors to an AHCI drive.
 * 
 * @param device 
 * @param buffer the sectors to write
 * @param lba first sector to write
 * @param len number of sectors
 */
void ahci_write(fs_device_t* device, uint8_t* buffer, uint64_t lba, uint64_t len);