#pragma once

#include "BalrogOS/Drivers/disk/ata_device.h"

#include <stdint.h>

/**
 * @brief read data from the ata drive
 * 
 * @param buffer the buffer that will contain the newly read datas
 * @param lba logical block address
 * @param len length
 * @param device the ata device to use
 */
void ata_read(uint8_t* buffer, uint64_t lba, uint64_t len, ata_drive* device);

/**
 * @brief write data to the ata drive
 * 
 * @param buffer the buffer containing the datas to write.
 * @param lba logical block address
 * @param len length
 * @param device the ata device to use
 */
void ata_write(uint8_t* buffer, uint64_t lba, uint64_t len, ata_drive* device);

/**
 * @brief initialize ATA drives
 * 
 */
void init_ata();