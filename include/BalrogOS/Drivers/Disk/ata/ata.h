#pragma once

#include "BalrogOS/FileSystem/filesystem.h"

#include <stdint.h>

/**
 * @brief read data from the ata drive
 * 
 * @param device the ata device to use
 * @param buffer the buffer that will contain the newly read datas
 * @param lba logical block address
 * @param len length
 */
void ata_read(fs_device_t* device, uint8_t* buffer, uint64_t lba, uint64_t len);

/**
 * @brief write data to the ata drive
 * 
 * @param device the ata device to use
 * @param buffer the buffer containing the datas to write.
 * @param lba logical block address
 * @param len length
 */
void ata_write(fs_device_t* device, uint8_t* buffer, uint64_t lba, uint64_t len);

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
void init_ata();