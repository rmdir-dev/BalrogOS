#include "BalrogOS/Syscall/syscall.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/Tasking/tasking.h"
#include "klib/IO/kprint.h"
#include "libc/string.h"

extern process* current_running;

void* sys_getcwd(interrupt_regs* stack_frame)
{
    char* buf = (char*) stack_frame->rdi;
    size_t size = (size_t) stack_frame->rsi;

    if(size < strlen(current_running->cwd))
    {
        return NULL;
    }

    strcpy(buf, current_running->cwd);

    return buf;
}