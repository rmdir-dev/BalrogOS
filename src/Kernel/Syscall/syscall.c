#include "BalrogOS/Syscall/syscall.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"

#include <stdio.h>

static interrupt_regs* syscall_handler(interrupt_regs* stack_frame)
{
    printf("syscall! \n");
    return stack_frame;
}

void init_syscalls()
{
    register_interrupt_handler(INT_SYSCALL, syscall_handler);
}