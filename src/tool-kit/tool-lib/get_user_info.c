//
// Created by rmdir on 10/20/23.
//

#include <stdlib.h>
#include <unistd.h>
#include <stat.h>
#include <stdio.h>
#include <stddef.h>
#include <fcntl.h>
#include <string.h>
#include <balrog/input.h>
#include <balrog/fs/fs_struct.h>
#include "toolkit/tool_read_keyboard.h"

void get_user_info(user_info_t* info) {
    int uid = getuid();

    int fd = open("/etc/passwd", 0);

    if(fd == -1) {
        printf("Error: cannot get user details !\n\n");
        exit(-1);
        return;
    }

    fs_file_stat stat = {};
    fstat(fd, &stat);

    char* file_data = malloc(stat.size);
    read(fd, file_data, stat.size);
    close(fd);

    char* line = strtok(file_data, '\n');
    int current_pos = 0;

    while(line != NULL) {
        int len = strlen(line);
        current_pos += len + 1;
        if(line[0] == '#') {
            line = strtok(NULL, '\n');
            continue;
        }

        char* user = strtok(line, ':');
        char* pass = strtok(NULL, ':');
        char* uid_str = strtok(NULL, ':');
        char* gid = strtok(NULL, ':');
        char* home = strtok(NULL, ':');
        char* shell = strtok(NULL, ':');

        int id = atoi(uid_str);
        if(id == uid) {
            info->username = malloc(strlen(user) + 1);
            memcpy(info->username, user, strlen(user) + 1);
            info->home_dir = malloc(strlen(home) + 1);
            memcpy(info->home_dir, home, strlen(home) + 1);
            info->shell = malloc(strlen(shell) + 1);
            memcpy(info->shell, shell, strlen(shell) + 1);
            info->uid = id;
            info->gid = atoi(gid);
            return;
        }

        line = strtok(&file_data[current_pos], '\n');
    }
    free(file_data);
}