#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        printf("rmdir: missing operand\n");
        return -1;
    }

    for(size_t i = 1; i < argc; i++)
    {
        if(rmdir(argv[i]) != 0)
        {
            printf("rmdir: failed to remove '%s'\n", argv[i]);
        }
    }

    return 0;
}
