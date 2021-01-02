#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "Kernel/Drivers/Screen/vga_driver.h"

void terminal_initialize()
{
	vga_init();
}

void terminal_write(const char* data, size_t size) 
{
	vga_write(data, size);
}
 
void terminal_writestring(const char* data) 
{
	terminal_write(data, strlen(data));
}