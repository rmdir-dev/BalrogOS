#include "stdio.h"
#include "Kernel/IO/tty/tty_io.h"

int putchar(int c)
{
    char out_c =  (char) c;
    
    terminal_write(&out_c, 1);

    return 1;
}