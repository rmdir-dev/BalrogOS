#include <stdio.h>
#include <fcntl.h>
#include <stat.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <balrog/fs/fs_struct.h>

char buf[4096 * 10];
char name[255];
char cwd[100] = {};

void _print_dir(uint8_t* entires)
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
    getcwd(cwd, 100);
    if(argc > 1)
    {
        char tmp[100] = {};
        int start_index = 0;
        int total_len = strlen(argv[1]);
        int argv_len = strlen(argv[1]);
        if(argv[1][0] != '/') {
            int cwd_len = strlen(cwd);
            memcpy(tmp, cwd, cwd_len);
            tmp[cwd_len] = '/';
            start_index = cwd_len + 1;
            total_len = cwd_len + argv_len;
        }
        memcpy(&tmp[start_index], argv[1], argv_len);
        tmp[total_len + 1] = 0;
        // TODO check the agrs properly
        int fd = open(tmp, 0);

        if(fd == -1)
        {
            switch (errno) {
                case ENOENT:
                    printf("ls: cannot access '%s': No such file or directory\n", argv[1]);
                    break;
                case EACCES:
                    printf("ls: cannot access '%s': Permission denied\n", argv[1]);
                    break;
                default:
                    break;
            }
            exit(0);
        }
        
        fs_file_stat stat = {};
        fstat(fd, &stat);
        read(fd, &buf[0], stat.size);
        buf[stat.size] = 0;
        _print_dir(buf);
        close(fd);
    } else {
        int fd = open(cwd, 0);
        fs_file_stat stat = {};
        fstat(fd, &stat);
        read(fd, &buf[0], stat.size);
        buf[stat.size] = 0;
        _print_dir(buf);
        close(fd);
    }
//    printf("before exit\n");
    exit(0);
}