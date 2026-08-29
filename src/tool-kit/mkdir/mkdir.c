#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        printf("mkdir: missing operand\n");
        return -1;
    }

    for(size_t i = 1; i < argc; i++)
    {
        if(mkdir(argv[i], 0770) != 0)
        {
            printf("mkdir: cannot create directory '%s'\n", argv[i]);
        }
    }

    return 0;
}
