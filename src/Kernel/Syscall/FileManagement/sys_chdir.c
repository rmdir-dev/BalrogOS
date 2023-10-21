#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/Syscall/syscall.h"
#include "BalrogOS/Tasking/tasking.h"
#include "BalrogOS/Memory/kheap.h"
#include "klib/IO/kprint.h"
#include <errno.h>
#include <string.h>

extern process* current_running;
extern int __check_file_permission(fs_fd* fd, uint16_t mode);

int sys_chdir(interrupt_regs* stack_frame)
{
    fs_fd fd;

    size_t len = strlen((char*) stack_frame->rdi);
    char* path = vmalloc(len + 1);
    memcpy(path, (char*) stack_frame->rdi, len + 1);
    path[len] = 0;
    if(fs_open(path, &fd) != 0)
    {
        *current_running->error_no = ENOENT;
        return -1;
    }

    if(__check_file_permission(&fd, 01) != 0)
    {
        fs_close(&fd);
        return -1;
    }

    fs_close(&fd);
    vmfree(path);
    vmfree(current_running->cwd);
    len = strlen((char*) stack_frame->rdi);
    current_running->cwd = (char*) vmalloc(len + 1);
    strcpy(current_running->cwd, (char*) stack_frame->rdi);
    current_running->cwd[len] = 0;

    return 0;
}