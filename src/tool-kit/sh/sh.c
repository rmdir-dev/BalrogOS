#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <balrog/input.h>

static char** historic = 0;
static int hist_index = 0;
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

int sh_exec_cmd(char** args)
{
    if(buf_idx == 0)
    {
        return 0;
    }
    
    int fd = open(args[0], 0);

    if(fd == -1)
    {
        printf("command '%s' does not exist! \n", args[0]);
        return -1;
    }

    pid_t id = fork();

    if(id != 0)
    {
        waitpid(id, 0, 0);
    } else 
    {
        execv(args[0], args);
    }

    close(fd);
    return 0;
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
}

char sh_process_input(struct input_event input)
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
                putchar(buffer[buf_idx]);
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
        if(sh_process_input(input) != 0)
        {
            keys[KEY_ENTER] = 1;
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

        sh_exec_cmd(&arguments);

        // TODO some sleep(nanosec) 
        for(int i = 0; i < 1000000; i++)
        {}
    }
    exit(0);
}