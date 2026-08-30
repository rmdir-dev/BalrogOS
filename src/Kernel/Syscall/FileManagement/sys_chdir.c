#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/Syscall/syscall.h"
#include "BalrogOS/Syscall/syscall_guard.h"
#include "BalrogOS/Tasking/tasking.h"
#include "BalrogOS/Memory/kheap.h"
#include "klib/IO/kprint.h"
#include <errno.h>
#include <string.h>

#include "BalrogOS/Memory/memory.h"

extern process* current_running;
extern int __check_file_permission(fs_fd* fd, uint16_t mode);

int sys_chdir(interrupt_regs* stack_frame)
{
    fs_fd fd;

    const char* user_path = (const char*) stack_frame->rdi;

    if (!user_ptr_ok((uintptr_t) user_path))
    {
        *current_running->error_no = EFAULT;
        return -1;
    }

    size_t len = strlen((char*) stack_frame->rdi);
    char* path = vmalloc(len + 1);

    if (!path)
    {
        *current_running->error_no = ENOMEM;
        return -1;
    }

    memcpy(path, (char*) stack_frame->rdi, len + 1);
    path[len] = 0;

    if(fs_open(path, &fd) != 0)
    {
        vmfree(path);
        *current_running->error_no = ENOENT;
        return -1;
    }

    if(__check_file_permission(&fd, 01) != 0)
    {
        fs_close(&fd);
        vmfree(path);
        return -1;
    }

    fs_close(&fd);
    vmfree(current_running->cwd);
    current_running->cwd = (char*) vmalloc(len + 1);
    memcpy(current_running->cwd, path, len + 1);
    vmfree(path);

    return 0;
}