#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stddef.h>
#include <string.h>
#include <balrog/input.h>
#include <balrog/fs/fs_struct.h>
#include <fcntl.h>
#include <stat.h>
#include <errno.h>
#include "toolkit/tool_read_keyboard.h"

static char** historic = 0;
static int hist_len = 5;
static int hist_index = 0;
static char buffer[255] = {};
static int buf_idx = 0;
static char* arguments[32] = {};
static int argc_count = 0;
static char* cwd[100] = {};
static char* hostname = NULL;
static int uid;
static user_info_t user_info = {};

int sh_exec_cmd(char** args)
{
    int fd = open(args[0], 0);

    if(fd == -1)
    {
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
        int len = strlen(user_info.home_dir);
        memcpy(tmp_cwd, user_info.home_dir, len);
        tmp_cwd[len] = 0;
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

    int fd = open(tmp_cwd, 0);

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
        chdir(tmp_cwd);
        close(fd);

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
    argc_count = 0;
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
    arguments[argc_count] = &buf[0];

    cmd = strtok(NULL, ' ');
    while(cmd)
    {
        arguments[++argc_count] = cmd;
        cmd = strtok(NULL, ' ');
    }

    if(argc_count == 0) {
        arguments[1] = 0;
    }

    argc_count++;
}

int manage_ctrl(uint16_t code, uint8_t * keys)
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

    return -1;
}

int manage_alt(uint16_t code, uint8_t * keys) {
    return -1;
}

char* get_cwd_value() {
    if(strcmp(cwd, user_info.home_dir) == 0) {
        return "~";
    }

    return cwd;
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

    printf("\e[0;94m%s\e[0;92m@\e[0;96m%s\e[0m:%s%s ", user_info.username, hostname, get_cwd_value(), uid == 0 ? "#" : "$");

    while(1)
    {
        read(STDIN_FILENO, &input, sizeof(struct input_event));

        if(process_input(&input, buffer, &buf_idx, 1, &manage_ctrl, &manage_alt) != 0 && buf_idx != 0)
        {
            //keys[KEY_ENTER] = 1;
            break;
        }
    }
    printf("\n");
}

void get_hostname() {
    int fd = open("/etc/hostname", 0);

    if(fd != -1) {
        char* buf = malloc(32);
        read(fd, buf, 32);
        char* first_line = strtok(buf, '\n');
        int len = strlen(first_line);
        hostname = malloc(len);
        memcpy(hostname, first_line, len);
        hostname[len] = 0;
        free(buf);
        close(fd);
    } else {
        hostname = "localhost";
    }
}

void process_config() {
    int home_dir_len = strlen(user_info.home_dir);
    char* config_file = malloc(home_dir_len + 12);
    memcpy(config_file, user_info.home_dir, home_dir_len);
    memcpy(&config_file[home_dir_len],
           user_info.home_dir[home_dir_len - 1] != '/' ? "/.spade" : ".spade", 9);
    int fd = open(config_file, 0);

    if(fd == -1) {
        printf("No config file found at : %s ", config_file);
        return; // default config
    }

    fs_file_stat stat = {};
    fstat(fd, &stat);

    char* buf = malloc(stat.size);
    read(fd, buf, stat.size);
    buf[stat.size] = 0;

    char* line = strtok(buf, '\n');
    char* cursor_char = NULL;
    uint8_t show_cursor = 0;
    int index = 0;
    while(line != NULL) {
        index += strlen(line) + 1;
        if(line[0] == '#') {
            line = strtok(NULL, '\n');
            continue;
        }

        char* key = strtok(line, '=');

        if(key)
        {
            char* value = strtok(NULL, '\n');
            if(strcmp(key, "CURSOR") == 0)
            {
                cursor_char = strdup(value);
                show_cursor = 1;
            }

            if(strcmp(key, "CURSOR_COLOR") == 0)
            {
                cursor_color = strdup(value);
                show_cursor = 1;
            }

            if(strcmp(key, "HISTORIC_LEN") == 0)
            {
                hist_len = atoi(value);
            }
            key = NULL;
        }

        if(index < stat.size) {
            line = strtok(&buf[index], '\n');
        } else {
            line = NULL;
        }
    }

    if(show_cursor)
    {
        // cursor must be equal to \e[0; + cursor_color + m + cursor_char + \e[0m
        int cursor_char_len = 1;
        int cursor_len = 9 + strlen(cursor_color) + cursor_char_len;
        cursor = malloc(cursor_len);
        memcpy(cursor, "\e[0;", 4);
        memcpy(&cursor[4], cursor_color, strlen(cursor_color));
        memcpy(&cursor[4 + strlen(cursor_color)], "m", 1);
        if(cursor_char) {
            memcpy(&cursor[5 + strlen(cursor_color)], cursor_char, cursor_char_len);
        } else {
            memcpy(&cursor[5 + strlen(cursor_color)], " ", 1);
        }
        memcpy(&cursor[5 + strlen(cursor_color) + cursor_char_len], "\e[0m", 4);
        cursor[cursor_len] = 0;
    }

    close(fd);
    free(buf);
    free(config_file);
    if(cursor_char){
        free(cursor_char);
    }
}

void main(int argc, char** argv)
{
    get_user_info(&user_info);
    get_hostname();
    process_config();
    getcwd(&cwd[0], 100);

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
            if(argc_count == 1)
            {
                change_directory(user_info.home_dir);
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