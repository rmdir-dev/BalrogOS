#include "klib/IO/kprint.h"
#include "BalrogOS/Drivers/Screen/vga_driver.h"

void kclear()
{
    vga_clear();
}