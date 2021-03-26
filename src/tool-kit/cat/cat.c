#include <stdio.h>
#include <fcntl.h>
#include <stat.h>
#include <stdint.h>
#include <unistd.h>
#include <balrog/fs/fs_struct.h>

char buf[4096 * 100];
char name[255];

void main(int argc, char** argv)
{
    if(argc > 1)
    {
        // TODO check the agrs properly
        int fd = open(argv[1], 0);
        
        if(fd == -1)
        {
            printf("file '%s' does not exist.\n", argv[1]);
            exit(0);
        }

        fs_file_stat stat = {};
        fstat(fd, &stat);
        if(stat.size >= (4096 * 100))
        {
            printf("file is currently too large!\n");
            exit(-1);
        }
        read(fd, &buf[0], stat.size);
        buf[stat.size] = 0;
        printf(buf);
        close(fd);
    }
    exit(0);
}