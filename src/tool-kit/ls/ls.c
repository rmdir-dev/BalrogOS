#include <stdio.h>
#include <fcntl.h>
#include <stat.h>
#include <stdint.h>
#include <balrog/fs/fs_struct.h>

char buf[4096 * 10];
char name[255];

void _test_print_dir(uint8_t* entires)
{
    fs_dir_entry* entry = entires;

    while(entry->inbr)
    {
        memcpy(name, &entry->name, entry->name_len);
        name[entry->name_len] = 0;
        printf("%s \n", name);
        entires += entry->entry_size;
        entry = entires;
    }
}

void main(int argc, char** argv)
{
    printf("elf file :D \n");
    int fd = open("/boot", 0);
    fs_file_stat stat = {};
    fstat(fd, &stat);
    read(fd, buf, stat.size);
    buf[stat.size] = 0;
    _test_print_dir(buf);
    close(fd);
    exit(0);
}