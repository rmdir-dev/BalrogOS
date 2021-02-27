#include "BalrogOS/FileSystem/filesystem.h"
#include "BalrogOS/Drivers/disk/ata.h"

void init_file_system()
{
    init_ata();
    fs_device dev;
    ata_get_boot_device(&dev);
}