#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <fcntl.h>
#include <string.h>
#include <balrog/input.h>
#include <balrog/fs/fs_struct.h>
#include <errno.h>
static char* cwd[100] = {};
#include "toolkit/tool_read_keyboard.h"

static char** historic = 0;
static int hist_index = 0;
static char buffer[255] = {};
static int buf_idx = 0;
static char* arguments[32] = {};
static int count_idx = 0;


int sh_exec_cmd(char** args)
{    
    int fd = open(args[0], 0);

    if(fd == -1)
    {
        printf("command '%s' does not exist! \n", args[0]);
        return -1;
    }

    close(fd);
    pid_t id = fork();

    if(id != 0)
    {
        waitpid(id, 0, 0);
    } else
    {
        execv(args[0], args);
    }

    return 0;
}

void change_directory(char* dir) {
    // TODO move this into the kernel sys_chdir
    char* tmp_cwd = malloc(100);
    memcpy(tmp_cwd, cwd, 100);

    if(strcmp(dir, "..") == 0) {
        int len = strlen(tmp_cwd);
        int i = len - 1;
        for(; i >= 0; i--) {
            if(tmp_cwd[i] == '/') {
                if(i == 0) {
                    tmp_cwd[i + 1] = 0;
                    break;
                }
                tmp_cwd[i] = 0;
                break;
            }
        }
    } else if(strcmp(dir, "/") == 0) {
        memcpy(tmp_cwd, "/", 2);
        tmp_cwd[1] = 0;
    } else if(strcmp(dir, "~") == 0) {
//        int len = strlen(user_info.home_dir);
//        memcpy(tmp_cwd, user_info.home_dir, len);
//        tmp_cwd[len] = 0;
    } else {
        int dir_len = strlen(dir);
        if(dir[dir_len - 1] == '/') {
            dir[dir_len - 1] = 0;
        }

        int len = strlen(tmp_cwd);
        char* start_copy = &tmp_cwd[len];

        if(tmp_cwd[len - 1] != '/') {
            tmp_cwd[len] = '/';
            start_copy = &tmp_cwd[++len];
        }

        if(dir[0] == '/') {
            start_copy = &tmp_cwd[0];
            len = 0;
        }
        memcpy(start_copy, dir, strlen(dir));
        tmp_cwd[len + strlen(dir)] = 0;
    }

//    printf("cd to %s\n", tmp_cwd);
    int fd = chdir(tmp_cwd);

    if(fd == -1) {
        switch (errno) {
            case ENOENT:
                printf("cd: '%s' : No such file or directory\n", dir);
                break;
            case EACCES:
                printf("cd: '%s' : Permission denied\n", dir);
                break;
            default:
                break;
        }
    } else {
//        close(fd);

        memcpy(cwd, tmp_cwd, 100);
    }

    free(tmp_cwd);
}

void disconect_user() {
    char* args[2] = {"/bin/clear", 0 };
    sh_exec_cmd(args);

    for (int i = 0; i < 100000000; ++i)
    {}

    exit(0);
}

void sh_parse_cmd()
{
    count_idx = 0;
    for(int i = 0; i < 32; i++)
    {
        arguments[i] = 0;
    }
    char* cmd = strtok(buffer, ' ');
    size_t len = strlen(cmd);
    char* buf = malloc(5 + len);
    memcpy(buf, "/bin/", 5);
    memcpy(&buf[5], cmd, len);
    buf[5 + len] = 0;
    arguments[count_idx] = &buf[0];

    while(arguments[count_idx++])
    {
        arguments[count_idx] = strtok(NULL, ' ');
    }

    if(count_idx == 0) {
        arguments[1] = 0;
    }

    count_idx++;
}

int manage_ctrl(uint16_t special, uint16_t code, uint8_t * keys)
{
    if(special == KEY_RIGHTCTRL || special == KEY_LEFTCTRL)
    {
        if(code == KEY_L)
        {
            buf_idx = 6;
            memset(buffer, 0, 255);
            memcpy(buffer, "clear", 6);
        }

        if(code == KEY_D) {
            disconect_user();
        }
    }
    
    return -1;
}

int manage_alt(uint16_t code, uint8_t* keys) {
    return -1;
}

void sh_read_input()
{
    struct input_event input = {};

    buf_idx = 0;
    memset(buffer, 0, 255);
    if(arguments[0] != 0)
    {
        free(arguments[0]);
    }
    
    printf("\e[0;96mBalrog\e[0m:/$ ");
    while(1)
    {
        read(STDIN_FILENO, &input, sizeof(struct input_event));
        if(process_input(&input, buffer, &buf_idx, 1, &manage_ctrl) != 0 && buf_idx != 0)
        {
            //keys[KEY_ENTER] = 1;
            break;
        }
    }
    printf("\n");
}

void main(int argc, char** argv)
{
    historic = malloc(sizeof(char*) * 10);
    
    for(int i = 0; i < 10; i++)
    {
        historic[i] = malloc(256);
    }

    while(1)
    {
        sh_read_input();
        sh_parse_cmd();

        if(strcmp(arguments[0], "/bin/exit") == 0)
        {
            disconect_user();
        } if(strcmp(arguments[0], "/bin/cd") == 0)
        {
            if(count_idx == 1)
            {
                change_directory("/");
            } else {
                change_directory(arguments[1]);
            }
        }
        else {
            sh_exec_cmd(&arguments);
        }


        // TODO some sleep(nanosec) 
        for(int i = 0; i < 1000000; i++)
        {}
    }
    exit(0);
}