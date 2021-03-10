#include "BalrogOS/Drivers/Screen/vga_driver.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/FileSystem/filesystem.h"
#include "BalrogOS/Tasking/process.h"
#include <stdint.h>

extern process* current_running;

int sys_open(interrupt_regs* stack_frame)
{
    if(current_running)
    {
        fs_fd* fd = &current_running->fd_table[0];
        fs_open(stack_frame->rdi, fd);
        
        return 0;
    }
    return -1;
}

void sys_close(interrupt_regs* stack_frame)
{
    if(current_running)
    {
        fs_fd* fd = &current_running->fd_table[stack_frame->rdi];
        fs_close(fd);
    }
}

void sys_read(interrupt_regs* stack_frame)
{
    if(current_running)
    {
        fs_fd* fd = &current_running->fd_table[stack_frame->rdi];
        fs_read(stack_frame->rsi, stack_frame->rdx, fd);
    }
}

void sys_write(interrupt_regs* stack_frame)
{
    unsigned fd = stack_frame->rdi;
    const char* str = stack_frame->rsi;
    size_t count = stack_frame->rdx;
    vga_write(str, count);
}