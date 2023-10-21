#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stat.h>
#include <balrog/input.h>
#include <balrog/fs/fs_struct.h>
#include <balrog/terminal/term.h>
#include "toolkit/tool_read_keyboard.h"

#define USERNAME_MODE   1
#define PASSWORD_MODE   0

char file_buf[4096 * 100];
static char buffer[255] = {};
static int buf_idx = 0;
static uint8_t keys[255] = {};
static char* arguments[32] = {};
static int count_idx = 0;
static int uid = -1;
static char* username = NULL;
static char* password = NULL;
static char* shell = NULL;

void start_login();

void log_user()
{
    printf(TERM_CLEAR);
    pid_t id = fork();

    if(id != 0)
    {
        waitpid(id, 0, 0);
        free(username);
        free(password);
        free(shell);
        start_login();
    } else
    {
        setuid(uid);
        if(shell) {
            execv(shell, NULL);
        } else {
            execv("/bin/sh", NULL);
        }
    }
}

void manage_input(int mode)
{
    memset(buffer, 0, 255);
    buf_idx = 0;
    struct input_event input = {};
    
    while(1)
    {
        read(STDIN_FILENO, &input, sizeof(struct input_event));
        if(process_input(&input, buffer, &buf_idx, mode, NULL) != 0 && buf_idx != 0)
        {
            break;
        }
    }
    printf("\n");
}

int compare_entries(const char* entry1, const char* entry2)
{
    int e1_len = strlen(entry1);
    int e2_len = strlen(entry2);

    if(e1_len == e2_len)
    {
        return memcmp(entry1, entry2, e1_len);
    }
    return -1;
}

int check_password(const char* password)
{

    printf("password: ");
    manage_input(PASSWORD_MODE);

    if(password && compare_entries(password, buffer) == 0)
    {
        return 0;
    }
    return -1;
}

char* parse_shadow(const char* username)
{
    int fd = open("/etc/shadow", 0);

    if(fd == -1)
    {
        printf("/etc/shadow not found!");
        while(1);
    }

    fs_file_stat stat = {};
    fstat(fd, &stat);
    if(stat.size >= (4096 * 100))
    {
        printf("file is currently too large!\n");
        exit(-1);
    }

    read(fd, &file_buf[0], stat.size);
    file_buf[stat.size] = 0;

    char* user_line = NULL;
    char* cmd = strtok(file_buf, '\n');
    while(cmd)
    {
        if(cmd[0] == '#')
        {
            cmd = strtok(NULL, '\n');
            continue;
        }

        int len = strlen(cmd);
        int pwd_start = strcspn(cmd, ':');
        if(pwd_start != len)
        {
            cmd[pwd_start] = 0;
            int usrn_len = strlen(username);
            if(compare_entries(cmd, username) == 0)
            {
                user_line = &cmd[pwd_start + 1];
                break;
            }
        }
        cmd = strtok(NULL, '\n');
    }

    if(user_line == NULL)
    {
        password = NULL;
        return NULL;
    }

    char* pwd = strtok(user_line, ':');
    password = malloc(strlen(pwd) + 1);
    password[strlen(pwd)] = 0;
    memcpy(password, pwd, strlen(pwd));

    uid = atoi(strtok(NULL, ':'));
    char* gid = strtok(NULL, ':');
    char* home = strtok(NULL, ':');
    char* sh = strtok(NULL, ':');
    if(sh) {
        shell = malloc(strlen(sh) + 1);
        shell[strlen(sh)] = 0;
        memcpy(shell, sh, strlen(sh));
    }

    close(fd);
    return NULL;
}

void login();

void try_log_user()
{
    username = malloc(buf_idx + 1);
    memcpy(username, buffer, buf_idx);
    parse_shadow(username);

    if(check_password(password) != 0)
    {
        printf("\nlogin incorrect!\n\n");
        login();
    }

    log_user();
}

void login()
{
    printf("\nbalrog os login: ");
    manage_input(USERNAME_MODE);
    
    try_log_user();
}

void start_login() {
    printf("BalrogOS apha\n");
    login();
}

int main(int argc, char** argv)
{
    cursor = "\e[0;97m_\e[0m";

    start_login();
    return 0;
}