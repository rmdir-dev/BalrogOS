#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        printf("rm: missing operand\n");
        return -1;
    }

    for(size_t i = 1; i < argc; i++)
    {
        if(unlink(argv[i]) != 0)
        {
            printf("rm: cannot remove '%s'\n", argv[i]);
        }
    }

    return 0;
}
