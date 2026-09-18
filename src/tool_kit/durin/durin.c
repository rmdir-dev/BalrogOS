#include <stdio.h>
#include <fcntl.h>
#include <stat.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <balrog/fs/fs_struct.h>

char name[255];
char cwd[100] = {};

int open_file(const char* file_name, int flags)
{
    int fd = open(file_name, 0);

    if(fd == -1)
    {
        switch(errno)
        {
        case ENOENT:
            printf("durin: %s No such file or directory\n", file_name);
            break;
        case EACCES:
            printf("durin: %s Permission denied\n", file_name);
            break;
        default:
            printf("durin: %s Unknown error\n", file_name);
            break;
        }
    }

    return fd;
}

void process_durinfs(const char* data)
{
    char* file_data = (char*) malloc(strlen(data) + 1);
    memcpy(file_data, data, strlen(data) + 1);
    size_t len = strlen(file_data);
    file_data[len] = 0;
    size_t next_pos = 0;

    char* line = strtok(file_data, '\n');
    while(line != NULL)
    {
        next_pos = (size_t) (line - file_data) + strlen(line) + 1;

        while (*line == ' ' || *line == '\t')
        {
            line++;
        }

        if (*line != '#')
        {
            const char* uuid = strtok(line, ';');
            const char* mount_point = (uuid != NULL) ? strtok(NULL, ';') : NULL;

            if (uuid == NULL || mount_point == NULL)
            {
                printf("durin: malformed line, expected uuid;mount_point\n");
            }
            else if (strlen(mount_point) != 1 || mount_point[0] != '/')
            {
                int mount_ret = mount(uuid, mount_point);

                if (mount_ret != 0)
                {
                    printf("mount failed with error %d\n", mount_ret);
                    printf("device : %s\n", uuid);
                    printf("mount point : %s\n", mount_point);
                }
            }
        }

        line = (next_pos < len) ? strtok(&file_data[next_pos], '\n') : NULL;
    }

    free(file_data);
}

void process_durin(const char* data)
{
    char* file_data = (char*) malloc(strlen(data) + 1);
    memcpy(file_data, data, strlen(data) + 1);
    size_t len = strlen(file_data);
    file_data[len] = 0;
    size_t next_pos = 0;

    char* line = strtok(file_data, '\n');
    while(line != NULL)
    {
        next_pos = (size_t) (line - file_data) + strlen(line) + 1;

        while (*line == ' ' || *line == '\t')
        {
            line++;
        }

        if (*line != '#' && *line != 0)
        {
            int fd = open_file(line, 0);

            if (fd != -1)
            {
                close(fd);
                pid_t id = fork();
                char** args = malloc(sizeof(char*) * 2);
                args[0] = line;
                // TODO process args
                args[1] = NULL;

                if(id != 0)
                {
                    waitpid(id, 0, 0);
                } else
                {
                    execv(args[0], args);
                }
            } else
            {
                printf("durin: cannot execute file : %s\n", line);
            }
        }

        line = (next_pos < len) ? strtok(&file_data[next_pos], '\n') : NULL;
    }

    free(file_data);
}

int main()
{
    getcwd(cwd, 100);

    int fd_init = open_file("/etc/durin", 0);
    int fd_fs = open_file("/etc/durinfs", 0);

    if(fd_fs != -1)
    {
        printf("durin : mount devices\n");
        fs_file_stat stat;
        fstat(fd_fs, &stat);
        char* file_buffer = malloc(stat.size + 1);
        read(fd_fs, file_buffer, stat.size);
        file_buffer[stat.size] = 0;
        process_durinfs(file_buffer);
    } else
    {
        printf("durin : no /etc/durinfs\n");
    }

    printf("durin : start init\n");
    fs_file_stat stat;
    fstat(fd_init, &stat);
    char* init_data = malloc(stat.size + 1);
    read(fd_init, init_data, stat.size);
    init_data[stat.size] = 0;
    process_durin(init_data);

    return 0;
}