#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        printf("touch: missing file operand\n");
        return -1;
    }

    for(size_t i = 1; i < argc; i++)
    {
        if(creat(argv[i], 0664) != 0)
        {
            printf("touch: cannot create '%s'\n", argv[i]);
        }
    }

    return 0;
}
