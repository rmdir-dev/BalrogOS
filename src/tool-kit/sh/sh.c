#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

void main(int argc, char** argv)
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
    }

    printf("Both\n");
    exit(0);
}