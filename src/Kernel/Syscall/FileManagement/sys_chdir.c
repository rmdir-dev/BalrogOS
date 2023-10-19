#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/Syscall/syscall.h"
#include "BalrogOS/Tasking/tasking.h"
#include "BalrogOS/Memory/kheap.h"
#include "klib/IO/kprint.h"
#include <string.h>

extern process* current_running;

int sys_chdir(interrupt_regs* stack_frame)
{
    vmfree(current_running->cwd);
    uint32_t len = strlen((char*) stack_frame->rdi);
    current_running->cwd = (char*) vmalloc(len + 1);
    strcpy(current_running->cwd, (char*) stack_frame->rdi);
    current_running->cwd[len] = 0;

    return 0;
}