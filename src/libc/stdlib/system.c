#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <fcntl.h>

static void _manage_errors() {

}

static int _parse_command_line(const char* command, char** arguments) {
    int argc_count = 0;
    char* buffer = strdup(command);

    for(int i = 0; i < 32; i++)
    {
        arguments[i] = 0;
    }

    char* tmp_buf = strdup(buffer);
    char* cmd = strtok(tmp_buf, ' ');
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

    free(tmp_buf);
    free(buffer);
    return ++argc_count;
}

void system(const char* command)
{
    char* arguments[32];
    int argc_count = _parse_command_line(command, arguments);

    int fd = open(arguments[0], O_RDONLY);

    if(fd == -1)
    {
        _manage_errors();
        return;
    }

    close(fd);
    pid_t id = fork();

    if(id == 0)
    {
        execv(arguments[0], arguments);
    }
}