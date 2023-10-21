#include <stdio.h>
#include <fcntl.h>
#include <stat.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <balrog/fs/fs_struct.h>

char buf[4096 * 100];
char name[255];
char cwd[100] = {};

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
            if(cwd_len > 1) {
                tmp[cwd_len] = '/';
                start_index = cwd_len + 1;
            }
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
                    printf("cat: %s: No such file or directory\n", argv[1]);
                    break;
                case EACCES:
                    printf("cat: %s: Permission denied\n", argv[1]);
                    break;
                default:
                    printf("cat: %s: Unknown error\n", argv[1]);
                    break;
            }
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
    printf("\n");
    exit(0);
}