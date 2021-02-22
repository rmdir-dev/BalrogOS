#include "BalrogOS/Drivers/disk/ata.h"
#include "BalrogOS/Drivers/disk/ata_command.h"

static int _ata_wait_400ns()
{
    return 0;
}

static int _ata_send_command()
{
    return 0;
}

void init_ata_drive()
{

}

static inline void _ata_read_sector(uint8_t* buffer, uint64_t lba, ata_drive* device)
{

}

void ata_read(uint8_t* buffer, uint64_t lba, uint64_t len, ata_drive* device)
{

}

static inline void _ata_write_sector(uint8_t* buffer, uint64_t lba, ata_drive* device)
{

}

void ata_write(uint8_t* buffer, uint64_t lba, uint64_t len, ata_drive* device)
{

}
