#include "BalrogOS/Syscall/syscall.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"

#include <stdio.h>

extern void sys_write(unsigned fd, const char* str, size_t count);

static interrupt_regs* syscall_handler(interrupt_regs* stack_frame)
{
    /*
        Syscall dispatcher
    */
    switch (stack_frame->rax)
    {
    case 1:
        sys_write(stack_frame->rdi, stack_frame->rsi, stack_frame->rdx);
        break;
    
    default:
        break;
    }
    
    return stack_frame;
}

void init_syscalls()
{
    register_interrupt_handler(INT_SYSCALL, syscall_handler);
    set_interrupt_flag(INT_SYSCALL, IDT_PRESENT | IDT_INTERRUPT | IDT_DPL_3);
}