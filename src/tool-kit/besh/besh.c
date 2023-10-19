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

static char** historic = 0;
static int hist_index = 0;
static char buffer[255] = {};
static int buf_idx = 0;
static uint8_t keys[255] = {};
static char* arguments[32] = {};
static int argc_count = 0;
static char* _qwertyuiop = "qwertyuiop[]QWERTYUIOP{}";
static char* _asdfghjkl = "asdfghjkl;'\\ASDFGHJKL:\"|";
static char* _zxcvbnm = "zxcvbnm,./ZXCVBNM<>?";
static char* _num = "1234567890-=!@#$%^&*()_+";
static int shift = 0;
static int ctrl = 0;
static char* username = NULL;
static char* hostname = NULL;
static char* user_home = NULL;
static char* cwd[100] = {};
static int uid;

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
        memcpy(tmp_cwd, user_home, strlen(user_home));
        tmp_cwd[strlen(user_home)] = 0;
    } else {
        int dir_len = strlen(dir);
        if(dir[dir_len - 1] == '/') {
            dir[dir_len - 1] = 0;
        }

        int len = strlen(tmp_cwd);
        if(tmp_cwd[len - 1] != '/') {
            tmp_cwd[len] = '/';
        }
        char* start_copy = &tmp_cwd[len + 1];
        if(dir[0] == '/') {
            start_copy = &tmp_cwd[0];
            len = 0;
        }
        memcpy(start_copy, dir, strlen(dir));
        tmp_cwd[len + strlen(dir)] = 0;
    }

    int fd = open(tmp_cwd, 0);

    if(fd == -1) {
        printf("cd: '%s' : No such file or directory\n", dir);
    } else {
        close(fd);
        chdir(tmp_cwd);
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

char manage_ctrl(uint16_t code)
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

char sh_process_input(struct input_event input)
{
    if(input.type == EV_KEY)
    {
        if(input.value && keys[input.code] != input.value)
        {
            shift = keys[KEY_RIGHTSHIFT] || keys[KEY_LEFTSHIFT];

            if((keys[KEY_LEFTCTRL] || keys[KEY_RIGHTCTRL]))
            {
                return manage_ctrl(input.code);
            }
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

char* get_cwd_value() {
    if(strcmp(cwd, user_home) == 0) {
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

    printf("\e[0;94m%s\e[0;92m@\e[0;96m%s\e[0m:%s%s ", username, hostname, get_cwd_value(), uid == 0 ? "#" : "$");
    while(1)
    {
        read(STDIN_FILENO, &input, sizeof(struct input_event));
        if(sh_process_input(input) != 0 && buf_idx != 0)
        {
            //keys[KEY_ENTER] = 1;
            break;
        }
    }
    printf("\n");
}

void get_user_details() {
    uid = getuid();

    int fd = open("/etc/passwd", 0);

    if(fd == -1) {
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

        if(atoi(uid_str) == uid) {
            username = malloc(strlen(user) + 1);
            memcpy(username, user, strlen(user) + 1);
            user_home = malloc(strlen(home) + 1);
            memcpy(user_home, home, strlen(home) + 1);
            return;
        }

        line = strtok(&file_data[current_pos], '\n');
    }
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

void main(int argc, char** argv)
{
    get_user_details();
    get_hostname();
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
                change_directory(user_home);
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