#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/syscall/syscall.h"
#include "balrog_os/syscall/syscall_guard.h"
#include "balrog_os/tasking/tasking.h"
#include "balrog_os/memory/kheap.h"
#include "klib/io/kprint.h"
#include "balrog_os/debug/debug_output.h"
#include <errno.h>
#include <string.h>

#include "balrog_os/memory/memory.h"

extern process* current_running;
extern int __check_file_permission(fs_fd* fd, uint16_t mode);

int sys_chdir(interrupt_regs* stack_frame)
{
    fs_fd fd;

    const char* user_path = (const char*) stack_frame->rdi;

    if (!user_ptr_ok((uintptr_t) user_path))
    {
        kernel_debug_output(KDB_LVL_ERROR, "chdir : 0%p is not a user pointer, pid %d",
                user_path, current_running->pid);
        *current_running->error_no = EFAULT;
        return -1;
    }

    size_t len = strlen((char*) stack_frame->rdi);
    char* path = vmalloc(len + 1);

    if (!path)
    {
        kernel_debug_output(KDB_LVL_ERROR, "chdir : no memory for a path of %d bytes", len + 1);
        *current_running->error_no = ENOMEM;
        return -1;
    }

    memcpy(path, (char*) stack_frame->rdi, len + 1);
    path[len] = 0;

    if(fs_open(path, &fd) != 0)
    {
        kernel_debug_output(KDB_LVL_ERROR, "chdir : %s does not open, pid %d", path, current_running->pid);
        vmfree(path);
        *current_running->error_no = ENOENT;
        return -1;
    }

    if(__check_file_permission(&fd, 01) != 0)
    {
        kernel_debug_output(KDB_LVL_ERROR, "chdir : %s is not executable by pid %d", path, current_running->pid);
        fs_close(&fd);
        vmfree(path);
        return -1;
    }

    fs_close(&fd);
    vmfree(current_running->cwd);
    // TODO : path should always be absolute path.
    current_running->cwd = (char*) vmalloc(len + 1);
    memcpy(current_running->cwd, path, len + 1);

    kernel_debug_output(KDB_LVL_VERBOSE, "chdir : pid %d cwd is now %s", current_running->pid, current_running->cwd);

    vmfree(path);

    return 0;
}