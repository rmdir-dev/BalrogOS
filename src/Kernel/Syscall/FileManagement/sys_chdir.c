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

    if(fs_open((char*) stack_frame->rdi, &fd) != 0)
    {
        *current_running->error_no = ENOENT;
        return -1;
    }

    if(__check_file_permission(&fd, 01) != 0)
    {
        return -1;
    }

    vmfree(current_running->cwd);
    uint32_t len = strlen((char*) stack_frame->rdi);
    current_running->cwd = (char*) vmalloc(len + 1);
    strcpy(current_running->cwd, (char*) stack_frame->rdi);
    current_running->cwd[len] = 0;

    return 0;
}