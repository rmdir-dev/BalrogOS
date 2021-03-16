#include "klib/IO/kprint.h"
#include "BalrogOS/Drivers/Screen/vga_driver.h"

int kputchar(int c)
{
    char out_c =  (char) c;
    
    vga_write(&out_c, 1);

    return 1;
}