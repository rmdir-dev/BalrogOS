#include "BalrogOS/Drivers/Screen/vga_driver.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include <stdint.h>

void sys_write(interrupt_regs* stack_frame)
{
    unsigned fd = stack_frame->rdi;
    const char* str = stack_frame->rsi;
    size_t count = stack_frame->rdx;
    vga_write(str, count);
}