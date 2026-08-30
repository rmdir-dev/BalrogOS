#include "BalrogOS/Drivers/Keyboard/keyboard.h"
#include "BalrogOS/Drivers/Screen/vga_driver.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/FileSystem/filesystem.h"
#include "BalrogOS/Tasking/process.h"
#include "BalrogOS/Syscall/syscall_guard.h"
#include "BalrogOS/Memory/kheap.h"
#include "BalrogOS/Debug/debug_output.h"
#include "klib/IO/kprint.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>

extern process* current_running;

static int __copy_user_path(uintptr_t user, char* dst, size_t dst_size)
{
  if(!user_ptr_ok(user))
  {
      *current_running->error_no = EFAULT;
      return -1;
  }

  size_t len = strlen((const char*)user);

  if(len >= dst_size)
  {
      *current_running->error_no = ENAMETOOLONG;
      return -1;
  }

  memcpy(dst, (const char*)user, len);
  dst[len] = 0;
  return 0;
}

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

        if (__copy_user_path(stack_frame->rdi, name, sizeof(name)) != 0)
        {
            return -1;
        }
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

int sys_creat(interrupt_regs* stack_frame)
{
    if(current_running)
    {
        char name[256] = {};
        if (__copy_user_path(stack_frame->rdi, name, sizeof(name)) != 0)
        {
            return -1;
        }
        return fs_touch(name);
    }
    return -1;
}

int sys_mkdir(interrupt_regs* stack_frame)
{
    if(current_running)
    {
        char name[256] = {};
        if (__copy_user_path(stack_frame->rdi, name, sizeof(name)) != 0)
        {
            return -1;
        }
        return fs_mkdir(name);
    }
    return -1;
}

int sys_unlink(interrupt_regs* stack_frame)
{
    if(current_running)
    {
        char name[256] = {};
        if (__copy_user_path(stack_frame->rdi, name, sizeof(name)) != 0)
        {
            return -1;
        }
        return fs_unlink(name);
    }
    return -1;
}

int sys_rmdir(interrupt_regs* stack_frame)
{
    if(current_running)
    {
        char name[256] = {};
        if (__copy_user_path(stack_frame->rdi, name, sizeof(name)) != 0)
        {
            return -1;
        }
        return fs_rmdir(name);
    }
    return -1;
}

void sys_write(interrupt_regs* stack_frame)
{
    unsigned fd_id = stack_frame->rdi;
    const char* str = stack_frame->rsi;
    size_t count = stack_frame->rdx;

    /*  0, 1 and 2 are the console, every descriptor above has been handed
        out by sys_open() and goes through the file system.
    */
    if(fd_id > 2 && current_running)
    {
        fs_fd* fd = &current_running->fd_table[fd_id];

        if(__check_file_permission(fd, 02) != 0)
        {
            return;
        }

        fs_write((void*) str, count, fd);
        return;
    }

    vga_write(str, count);
    KERNEL_DEBUG_EXEC({
        serial_write(str, count);
    });
}
