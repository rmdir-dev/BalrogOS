#include <stdio.h>
#include <unistd.h>

static char cwd[100] = {};

int main(int argc, char** argv)
{
    getcwd(cwd, 100);
    printf("%s\n", cwd);
    return 0;
}