#include "BalrogOS/FileSystem/filesystem.h"
#include "BalrogOS/Drivers/disk/ata.h"

void init_file_system()
{
    init_ata();
}