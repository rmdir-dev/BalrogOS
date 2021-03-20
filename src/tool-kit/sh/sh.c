#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <balrog/input.h>

static char buffer[255] = {};
static int buf_idx = 0;
static uint8_t keys[255] = {};
static char* arguments[32] = {};
static int count_idx = 0;
static char* _qwertyuiop = "qwertyuiop";
static char* _asdfghjkl = "asdfghjkl";
static char* _zxcvbnm = "zxcvbnm";
static char* _num = "123456789";

int sh_exec_cmd(char** args)
{
    pid_t id = fork();

    printf("pid : %d\n", id);

    if(id != 0)
    {
        printf("Parent\n");
        waitpid(id, 0, 0);
    } else 
    {
        printf("Child\n");
        execv(args[0], args);
    }

    printf("Both\n");
    return 0;
}

void sh_parse_cmd()
{
    count_idx = 0;
    for(int i = 0; i < 32; i++)
    {
        arguments[i] = 0;
    }
    arguments[count_idx] = strtok(buffer, ' ');

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
                buffer[buf_idx - 1] = 0;
                buffer[buf_idx] = '\r';
            } else
            if(input.code >= KEY_1 && input.code <= KEY_0)
            {
                buffer[buf_idx] = _num[input.code - KEY_1];
            } else 
            if (input.code >= KEY_Q && input.code <= KEY_P)
            {
                buffer[buf_idx] = _qwertyuiop[input.code - KEY_Q];
            } else
            if (input.code >= KEY_A && input.code <= KEY_L)
            {
                buffer[buf_idx] = _asdfghjkl[input.code - KEY_A];
            } else 
            if (input.code >= KEY_Z && input.code <= KEY_M)
            {
                buffer[buf_idx] = _zxcvbnm[input.code - KEY_Z];
            } else 
            if (input.code == KEY_SLASH)
            {
                buffer[buf_idx] = '/';
            }

            putchar(buffer[buf_idx]);
            if(buffer[buf_idx] != '\r')
            {
                buf_idx++;
            } else 
            {
                buf_idx -= 2;
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

    printf("Balrog:/$ ");
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
    while(1)
    {
        sh_read_input();
        sh_parse_cmd();

        sh_exec_cmd(&arguments);

        // TODO some sleep(nanosec) 
        for(int i = 0; i < 1000000; i++)
        {

        }
    }
    exit(0);
}