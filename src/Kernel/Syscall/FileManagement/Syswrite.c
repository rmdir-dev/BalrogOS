#include "BalrogOS/Drivers/Screen/vga_driver.h"
#include <stdint.h>

void sys_write(unsigned fd, const char* str, size_t count)
{
    vga_write(str, count);
}