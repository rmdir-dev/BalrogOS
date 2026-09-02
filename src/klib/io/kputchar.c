#include "klib/io/kprint.h"
#include "balrog_os/drivers/screen/vga_driver.h"

int kputchar(int c)
{
    char out_c =  (char) c;
    
    vga_write(&out_c, 1);

    return 1;
}