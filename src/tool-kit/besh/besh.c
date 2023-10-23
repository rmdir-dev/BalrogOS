#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <stat.h>
#include <errno.h>
#include "balrog/input.h"
#include "balrog/fs/fs_struct.h"
#include "balrog/process/processing.h"
#include "balrog/terminal/term.h"
#include "toolkit/tool_read_keyboard.h"
#include "toolkit/list.h"

static int hist_len = 5;
static int hist_key_count = 0;
static list_t historic;
static list_node_t* hist_last_removed = NULL;
static list_node_t* hist_index = NULL;
static char buffer[255] = {};
static int buf_idx = 0;
static char* arguments[32] = {};
static int argc_count = 0;
static char* cwd[100] = {};
static char* hostname = NULL;
static user_info_t user_info = {};
static pid_t child_pid = 0;

void register_new_history_entry(char* entry) {
    if(hist_key_count >= hist_len)
    {
        hist_last_removed = list_remove_last(&historic);
//        printf("new hist count %d\n", hist_key_count);
//                hist_key_count = hist_last_removed->key;
//        printf("removed %s\n", hist_last_removed->value);
//        printf("new hist count %d\n", hist_key_count);
        free(hist_last_removed->value);
    }

    list_node_t* node = list_insert(&historic, hist_key_count++, hist_last_removed);
    node->value = strdup(buffer);
}

int process_child_process_ctrl_key(uint16_t special_key, uint16_t code, uint8_t* keys) {
    if(special_key == KEY_RIGHTCTRL || special_key == KEY_LEFTCTRL)
    {
        static uint8_t killed = 0;
        if(code == KEY_C && !killed)
        {
            kill(child_pid, SIGKILL);
            killed = 1;
        }
    }

    return 0;
}

void manage_child_input(int pid) {
//    printf("pre kill %d\n", pid);
    sleep(5);
    if(kill(pid, 0) == -1) {
        return;
    }
//    printf("post kill %d\n", pid);

    child_pid = pid;
    pid_t id = fork();

    if(id != 0)
    {
        waitpid(child_pid, 0, 0);
        kill(id, SIGKILL);
    } else
    {
        struct input_event input = {};
        while(1)
        {
            read(STDIN_FILENO, &input, sizeof(struct input_event));
            process_input(&input, NULL, NULL, 0, &process_child_process_ctrl_key);
        }
    }
}

int sh_exec_cmd(char** args, uint8_t add_to_history)
{
    int fd = open(args[0], 0);

    if(fd == -1)
    {
        if(errno == ENOENT)
        {
            printf("command not found : %s\n", args[0]);
        } else if(errno == EACCES)
        {
            printf("permission denied : %s\n", args[0]);
        }
        return -1;
    }

    close(fd);
    pid_t id = fork();

    if(id != 0)
    {
        manage_child_input(id);

        if(add_to_history == 1) {
            register_new_history_entry(buffer);
        }
        hist_index = NULL;
    } else
    {
        execv(args[0], args);
    }

    return 0;
}

void manage_cd_prev_dir(char* tmp_cwd) {
    int len = strlen(tmp_cwd);
    int i = len - 1;
    for(; i >= 0; i--) {
        if(tmp_cwd[i] == '/') {
            // if is root directory.
            if(i == 0) {
                tmp_cwd[i + 1] = 0;
                break;
            }
            tmp_cwd[i] = 0;
            break;
        }
    }
}

void change_directory(char* dir) {
    // TODO move this into the kernel sys_chdir
    char* tmp_cwd = malloc(100);
    memcpy(tmp_cwd, cwd, 100);

    if(strcmp(dir, "..") == 0) {
        manage_cd_prev_dir(tmp_cwd);
    } else if(strcmp(dir, "/") == 0) {
        memcpy(tmp_cwd, "/", 2);
        tmp_cwd[1] = 0;
    } else if(strcmp(dir, "~") == 0) {
        int len = strlen(user_info.home_dir);
        memcpy(tmp_cwd, user_info.home_dir, len);
        tmp_cwd[len] = 0;
    } else {
        int dir_len = strlen(dir);
        // remove trailing slash
        if(dir[dir_len - 1] == '/') {
            dir[dir_len - 1] = 0;
        }

        int len = strlen(tmp_cwd);
        char* start_copy = &tmp_cwd[len];

        // if chdir starts with a slash
        if(dir[0] == '/') {
            start_copy = &tmp_cwd[0];
            len = 0;
        } else {
            if(dir[0] == '.' && dir[1] == '/') {
                dir += 2;
            }

            while(dir[0] == '.' && dir[1] == '.') {
                if(dir[2] != '/' && dir[2] != 0) {
                    printf("cd : invalid path\n");
                    return;
                }

                manage_cd_prev_dir(tmp_cwd);
                if(dir[2] == '/') {
                    dir += 3;
                } else if(dir[2] == 0) {
                    dir += 2;
                }
            }
            // reset the length if there was a ../
            len = strlen(tmp_cwd);

            // if not root directory and cwd does not end with a slash
            if(tmp_cwd[len - 1] != '/') {
                tmp_cwd[len] = '/';
                start_copy = &tmp_cwd[++len];
            }
        }

        // check if dir contains other .. and remove them
        char* tmp_dir = strdup(dir);
        char* tmp_dir_start = tmp_dir;
        char* tmp_dir_end = tmp_dir;
        while(tmp_dir_end != NULL) {
            tmp_dir_end = strstr(tmp_dir_start, "../");
            if(tmp_dir_end != NULL) {
                free(tmp_dir);
                printf("cd : invalid path\n");
                return;
            }
        }

        // check if dir contains other . and remove them
        tmp_dir_start = tmp_dir;
        tmp_dir_end = tmp_dir;
        while(tmp_dir_end != NULL) {
            tmp_dir_end = strstr(tmp_dir_start, "./");
            if(tmp_dir_end != NULL) {
                free(tmp_dir);
                printf("cd : invalid path\n");
                return;
            }
        }

        free(tmp_dir);
        dir_len = strlen(dir);

        if(dir_len != 0) {
            memcpy(start_copy, dir, dir_len);
            tmp_cwd[len + strlen(dir)] = 0;
        }
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
        int success = chdir(tmp_cwd);
        if(success == -1) {
            printf("failed to change directory\n");
            switch (errno) {
                case ENOENT:
                    printf("cd: '%s' : No such file or directory\n", tmp_cwd);
                    break;
                case EACCES:
                    printf("cd: '%s' : Permission denied\n", tmp_cwd);
                    break;
                default:
                    break;
            }
        }
        register_new_history_entry(buffer);
        close(fd);

        memcpy(cwd, tmp_cwd, 100);
    }

    free(tmp_cwd);
}

void disconect_user() {
    char* args[2] = {"/bin/clear", 0 };
    sh_exec_cmd(args, 0);

    sleep(500);

    exit(0);
}

void sh_parse_cmd()
{
    argc_count = 0;
    for(int i = 0; i < 32; i++)
    {
        arguments[i] = 0;
    }
    char* tmp_buf = strdup(buffer);
    char* cmd = strtok(tmp_buf, ' ');
    size_t len = strlen(cmd);
    char* buf = malloc(5 + len);
    size_t start_write = 0;
    if(cmd[0] != '/')
    {
        memcpy(buf, "/bin/", 5);
        start_write = 5;
    }
    memcpy(&buf[start_write], cmd, len);
    buf[start_write + len] = 0;
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

    free(tmp_buf);
    argc_count++;
}

void replace_buffer(char* str) {
    for(int i = buf_idx - 1; i >= 0; i--) {
        puts("\b");
    }
    memset(buffer, 0, 255);

    memcpy(buffer, str, strlen(str));
    buf_idx = strlen(str);
    puts(buffer);
}

static uint8_t already_cleared = 0;
int manage_ctrl(uint16_t special, uint16_t code, uint8_t * keys)
{
    if(special == KEY_RIGHTCTRL || special == KEY_LEFTCTRL)
    {
        if(code == KEY_L && !already_cleared)
        {
            buf_idx = 6;
            memset(buffer, 0, 255);
            memcpy(buffer, "clear", 6);
            already_cleared = 1;
        }

        if(code == KEY_D) {
            disconect_user();
        }

        return -1;
    }

    // key arrow up
    if(special == 72)
    {
        if(hist_index == NULL)
        {
            hist_index = historic.head;
        } else
        {
            if(hist_index->next != NULL)
            {
                hist_index = hist_index->next;
            } else
            {
//                hist_index = NULL;
            }
        }

        if(hist_index != NULL)
        {
            replace_buffer(hist_index->value);
        }
    // key arrow down
    } else if(special == 80)
    {
        if(hist_index != NULL)
        {
            if(hist_index->prev)
            {
                hist_index = hist_index->prev;
            } else
            {
//                hist_index = NULL;
            }
        }

        if(hist_index != NULL)
        {
            replace_buffer(hist_index->value);
        } else
        {
            if(buf_idx == 0 || hist_index)
            {
                replace_buffer("");
            }
        }
    }

    return 0;
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
    if (arguments[0] != 0)
    {
        free(arguments[0]);
    }

    printf("\e[0;94m%s\e[0;92m@\e[0;96m%s\e[0m:%s%s ", user_info.username, hostname, get_cwd_value(), user_info.uid == 0 ? "#" : "$");

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
                cursor_char[1] = 0; // only one char
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

int main(int argc, char** argv)
{
    list_init(&historic);
    get_user_info(&user_info);
    get_hostname();
    process_config();
    getcwd(&cwd[0], 100);

    while(1)
    {
        sh_read_input();
        sh_parse_cmd();

        uint8_t is_clear_cmd = strcmp(arguments[0], "/bin/clear") == 0;
        if(!is_clear_cmd)
        {
            already_cleared = 0;
        }

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
            sh_exec_cmd(&arguments, is_clear_cmd ? 0 : 1);
        }
    }

    return 0;
}