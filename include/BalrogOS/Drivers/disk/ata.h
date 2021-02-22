#pragma once

#include "BalrogOS/Drivers/disk/ata_device.h"

#include <stdint.h>

void ata_read(uint8_t* buffer, uint64_t lba, uint64_t len, ata_drive* device);

void ata_write(uint8_t* buffer, uint64_t lba, uint64_t len, ata_drive* device);

void init_ata_drive();