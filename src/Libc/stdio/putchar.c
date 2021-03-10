#include <stdio.h>
#include <stdlib.h>

int putchar(int c)
{
    char out_c =  (char) c;
    
    write(0, &out_c, 1);

    return 1;
}