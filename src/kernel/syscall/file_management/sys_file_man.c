#include "balrog_os/drivers/keyboard/keyboard.h"
#include "balrog_os/drivers/screen/vga_driver.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/file_system/filesystem.h"
#include "balrog_os/tasking/process.h"
#include "balrog_os/syscall/syscall_guard.h"
#include "balrog_os/memory/kheap.h"
#include "balrog_os/debug/debug_output.h"
#include "klib/data_structure/rbt.h"
#include "klib/io/kprint.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>

extern process* current_running;

static int __copy_user_path(uintptr_t user, char* dst, size_t dst_size)
{
  if(!user_ptr_ok(user))
  {
      kernel_debug_output(KDB_LVL_ERROR, "path 0%p is not a user pointer, pid %d",
              user, current_running->pid);
      *current_running->error_no = EFAULT;
      return -1;
  }

  size_t len = strlen((const char*)user);

  if(len >= dst_size)
  {
      kernel_debug_output(KDB_LVL_ERROR, "path is %d long, the buffer holds %d, pid %d",
              len, dst_size, current_running->pid);
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
                kernel_debug_output(KDB_LVL_ERROR, "permission denied : mode 0%b asked on a 0%b file, uid %d gid %d against %d/%d",
                        mode, current_file_stat.mode,
                        current_running->uid, current_running->gid,
                        current_file_stat.uid, current_file_stat.gid);
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
        size_t index = current_running->fd_size++;
        fs_fd* fd = &current_running->fd_table[index];
        char name[256] = {};

        if (__copy_user_path(stack_frame->rdi, name, sizeof(name)) != 0)
        {
            return -1;
        }
        if(fs_open(name, fd) != 0)
        {
            kernel_debug_output(KDB_LVL_ERROR, "open : %s not found, pid %d", name, current_running->pid);
            *current_running->error_no = ENOENT;
            return -1;
        }

        kernel_debug_output(KDB_LVL_VERBOSE, "open : %s on fd %d, pid %d", name, index, current_running->pid);

        if(__check_file_permission(fd, 04) != 0)
        {
            return -1;
        }
        // shift + 3 as 0 = stdin 1 = stdout 2 = stderr
        return index + 3;
    }

    kernel_debug_output(KDB_LVL_ERROR, "open called with no running process");
    return -1;
}

void sys_close(interrupt_regs* stack_frame)
{
    if(current_running)
    {
        fs_fd* fd = &current_running->fd_table[stack_frame->rdi - 3];
        fs_close(fd);
    }
}

void sys_fstat(interrupt_regs* stack_frame)
{
    if(current_running)
    {
        fs_fd* fd = &current_running->fd_table[stack_frame->rdi - 3];

        if(__check_file_permission(fd, 04) != 0)
        {
            return;
        }

        fs_fstat(fd, stack_frame->rsi);
    }
}

int sys_read(interrupt_regs* stack_frame)
{
    if(current_running)
    {
        switch (stack_frame->rdi)
        {
        case 0:
            if (!user_ptr_ok(stack_frame->rsi))
            {
                *current_running->error_no = EFAULT;
                return -1;
            }
            return keyboard_read((struct input_event*) stack_frame->rsi);

        case 1:
        case 2:
            break;

        default:
            {
                fs_fd* fd = &current_running->fd_table[stack_frame->rdi - 3];

                if(__check_file_permission(fd, 04) != 0)
                {
                    *current_running->error_no = EACCES;
                    return -1;
                }

                fs_read(stack_frame->rsi, stack_frame->rdx, fd);
                break;
            }
        }
    }

    return 0;
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
        kernel_debug_output(KDB_LVL_VERBOSE, "creat : %s", name);
        return fs_touch(name);
    }

    kernel_debug_output(KDB_LVL_ERROR, "creat called with no running process");
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
        kernel_debug_output(KDB_LVL_VERBOSE, "mkdir : %s", name);
        return fs_mkdir(name);
    }

    kernel_debug_output(KDB_LVL_ERROR, "mkdir called with no running process");
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
        kernel_debug_output(KDB_LVL_VERBOSE, "unlink : %s", name);
        return fs_unlink(name);
    }

    kernel_debug_output(KDB_LVL_ERROR, "unlink called with no running process");
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
        kernel_debug_output(KDB_LVL_VERBOSE, "rmdir : %s", name);
        return fs_rmdir(name);
    }

    kernel_debug_output(KDB_LVL_ERROR, "rmdir called with no running process");
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
        fs_fd* fd = &current_running->fd_table[fd_id - 3];

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

// %rax     System call             %rdi                %rsi                        %rdx                %r10                    %r8             %r9
// 165	    sys_mount	            char *dev_name	    char *dir_name	        char *type	        unsigned long flags	void *data
int sys_mount(interrupt_regs* stack_frame)
{
    if(current_running)
    {
        char dev_name[256] = {};
        char dir_name[256] = {};
        if(__copy_user_path(stack_frame->rdi, dev_name, sizeof(dev_name)) != 0)
        {
            return -1;
        }

        if(__copy_user_path(stack_frame->rsi, dir_name, sizeof(dir_name)) != 0)
        {
            return -1;
        }

        kernel_debug_output(KDB_LVL_VERBOSE, "mount : %s on %s", dev_name, dir_name);
        return fs_mount(dir_name, dev_name);
    }

    kernel_debug_output(KDB_LVL_ERROR, "mount called with no running process");
    return -1;
}


int sys_umount(interrupt_regs* stack_frame)
{
    return 0;
}
