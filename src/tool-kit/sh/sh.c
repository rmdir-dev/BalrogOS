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
        char test_arg1[8] = "/bin/ls";
        char test_arg2[9] = "/boot/";
        unsigned long argv[5] = { &test_arg1, &test_arg2, 0, 0, 0 };
        execv(argv[0], &argv);
    }

    printf("Both\n");
    exit(0);
}