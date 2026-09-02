#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    if(argc != 2)
    {
        printf("Usage: sleep <milliseconds>\n", argv[0]);
        return 1;
    }

    sleep(atoi(argv[1]));

    return 0;
}