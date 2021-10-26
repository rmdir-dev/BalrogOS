#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <balrog/input.h>
#include <balrog/fs/fs_struct.h>

#define USERNAME_MODE   1
#define PASSWORD_MODE   0

char file_buf[4096 * 100];
static char buffer[255] = {};
static int buf_idx = 0;
static uint8_t keys[255] = {};
static char* arguments[32] = {};
static int count_idx = 0;
static char* _qwertyuiop = "qwertyuiop[]QWERTYUIOP{}";
static char* _asdfghjkl = "asdfghjkl;'\\ASDFGHJKL:\"|";
static char* _zxcvbnm = "zxcvbnm,./ZXCVBNM<>?";
static char* _num = "1234567890-=!@#$%^&*()_+";
static int shift = 0;

static int process_input(struct input_event input, int mode)
{
    if(input.type == EV_KEY)
    {
        if(input.value && keys[input.code] != input.value)
        {
            shift = keys[KEY_RIGHTSHIFT] || keys[KEY_LEFTSHIFT];

            uint8_t bckspace = 0;
            if(input.code == KEY_ENTER)
            {
                buffer[buf_idx] = 0;
                return -1;
            } else
            if(input.code == KEY_SPACE)
            {
                buffer[buf_idx] = ' ';
            } else
            if(input.code == KEY_BACKSPACE)
            {
                bckspace = 1;
                if(buf_idx > 0)
                {
                    buffer[buf_idx - 1] = 0;
                    buffer[buf_idx] = '\r';
                } else 
                {
                    buffer[buf_idx] = 0;
                }
            } else
            if(input.code >= KEY_1 && input.code <= KEY_0 + 2)
            {
                buffer[buf_idx] = _num[(input.code - KEY_1) + (shift * 12)];
            } else 
            if (input.code >= KEY_Q && input.code <= KEY_P + 2)
            {
                buffer[buf_idx] = _qwertyuiop[(input.code - KEY_Q) + (shift * 12)];
            } else
            if (input.code >= KEY_A && input.code <= KEY_L + 3)
            {
                buffer[buf_idx] = _asdfghjkl[(input.code - KEY_A) + (shift * 12)];
            } else 
            if (input.code >= KEY_Z && input.code <= KEY_M + 3)
            {
                buffer[buf_idx] = _zxcvbnm[(input.code - KEY_Z) + (shift * 10)];
            }

            if(buffer[buf_idx] != 0)
            {
                if(mode)
                {
                    putchar(buffer[buf_idx]);
                }
                if(bckspace == 0)
                {
                    buf_idx++;
                } else 
                {
                    buf_idx--;
                }
            }
        }
        keys[input.code] = input.value;
    }
    return 0;
}

void log_user()
{
    pid_t id = fork();

    if(id != 0)
    {
        waitpid(id, 0, 0);
        execv("/bin/sh", NULL);
    } else 
    {
        execv("/bin/clear", NULL);
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
        if(process_input(input, mode) != 0 && buf_idx != 0)
        {
            keys[KEY_ENTER] = 1;
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
    char* password = NULL;
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

    char* cmd = strtok(file_buf, '\n');
    while(cmd)
    {
        int len = strlen(cmd);
        int pwd_start = strcspn(cmd, ':');
        if(pwd_start != len)
        {
            cmd[pwd_start] = 0;
            int usrn_len = strlen(username);
            if(compare_entries(cmd, username) == 0)
            {
                len = strlen(&cmd[pwd_start + 1]);
                password = malloc(len + 1);
                memcpy(password, &cmd[pwd_start + 1], len);
                password[len] = 0;
                return password;
            }
        }
        cmd = strtok(NULL, '\n');
    }
    return NULL;
}

void login();

void try_log_user()
{
    char* username = malloc(buf_idx + 1);
    memcpy(username, buffer, buf_idx);
    
    char* password = parse_shadow(username);

    if(check_password(password) != 0)
    {
        printf("\nlogin incorrect!\n\n");
        login();
    }
    free(username);
    free(password);

    log_user();
}

void login()
{
    printf("\nbalrog os login: ");
    manage_input(USERNAME_MODE);
    
    try_log_user();
}

void main(int argc, char** argv)
{
    printf("BalrogOS apha\n");
    login();
    exit(0);
}