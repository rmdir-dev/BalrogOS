#include <stdio.h>
#include <unistd.h>

void main(int argc, char** argv)
{
    if(argc > 1)
    {
        for(size_t i = 1; i < argc; i++)
        {
            printf("%s ", argv[i]);
        }
    }
    printf("\n");
    exit(0);
}