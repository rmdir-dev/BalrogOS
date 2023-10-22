#include "BalrogOS/Drivers/Keyboard/keyboard.h"
#include "BalrogOS/Drivers/Screen/vga_driver.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/FileSystem/filesystem.h"
#include "BalrogOS/Tasking/process.h"
#include "BalrogOS/Memory/kheap.h"
#include "klib/IO/kprint.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>

extern process* current_running;


int __check_file_permission(fs_fd* fd, uint16_t mode) {
    fs_file_stat current_file_stat;
    fs_fstat(fd, &current_file_stat);

    // if the user is not the owner of the file or root
    if(current_file_stat.uid != current_running->uid && current_running->uid != 0)
    {
        if(!(current_file_stat.mode & mode)) {
            if(current_file_stat.gid != current_running->gid || !(current_file_stat.mode & (mode << 3)))
            {
                *current_running->error_no = EACCES;
                return -1;
            }
        }
    }

    return 0;
}

int sys_open(interrupt_regs* stack_frame)
{
    if(current_running)
    {
        fs_fd* fd = &current_running->fd_table[3];
        char name[256] = {};
        uint8_t len = strlen(stack_frame->rdi);
        // TODO manage error if len > 255
        memcpy(name, stack_frame->rdi, len);
        name[len] = 0;
        if(fs_open(name, fd) != 0)
        {
            *current_running->error_no = ENOENT;
            return -1;
        }

        if(__check_file_permission(fd, 04) != 0)
        {
            return -1;
        }
        return 3;
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

void sys_fstat(interrupt_regs* stack_frame)
{
    if(current_running)
    {
        fs_fd* fd = &current_running->fd_table[stack_frame->rdi];

        if(__check_file_permission(fd, 04) != 0)
        {
            return;
        }

        fs_fstat(fd, stack_frame->rsi);
    }
}

void sys_read(interrupt_regs* stack_frame)
{
    if(current_running)
    {
        switch (stack_frame->rdi)
        {
        case 0:
            keyboard_read(stack_frame->rsi);
            break;
        
        default:
            {
                fs_fd* fd = &current_running->fd_table[stack_frame->rdi];

                if(__check_file_permission(fd, 04) != 0)
                {
                    return;
                }

                fs_read(stack_frame->rsi, stack_frame->rdx, fd);
                break;
            }
        }
    }
}

void sys_write(interrupt_regs* stack_frame)
{
    unsigned fd = stack_frame->rdi;
    const char* str = stack_frame->rsi;
    size_t count = stack_frame->rdx;
    vga_write(str, count);
}
