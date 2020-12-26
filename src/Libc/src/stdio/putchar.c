#include "stdio.h"
#include "Kernel/IO/Terminal/terminal_io.h"

int putchar(int c)
{
    char out_c =  (char) c;
    
    terminal_write(out_c, 1);
}