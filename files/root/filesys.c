#include "ext2/ext2.h"
#include "ata/ata.h"
#include "filesys.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv)
{
    fs_device dev;
    dev.name = "filesys.dd";
    dev.read = ata_read;
    dev.write = ata_write;
    ext2_probe(&dev);

    char* buffer = malloc(4096 * 1024);

    dev.fs->list(&dev, "/", buffer);
    dev.fs->mkdir(&dev, "/boot");
    dev.fs->list(&dev, "/", buffer);
    dev.fs->list(&dev, "/boot/", buffer);
    dev.fs->touch(&dev, "/boot/os-image");
    dev.fs->list(&dev, "/boot/", buffer);
    char* buffer2 = malloc(4096 * 1024);
    buffer2[0] = 'B';
    buffer2[1] = 'A';
    buffer2[2] = 'L';
    buffer2[3] = 'R';
    buffer2[4] = 'O';
    buffer2[5] = 'G';
    buffer2[6] = 'O';
    buffer2[7] = 'S';
    buffer2[8] = '_';
    dev.fs->write(&dev, "boot/os-image", buffer2, 0, 4194304);
    dev.fs->open(&dev, "/filesys.c", buffer);

    return 0;
}