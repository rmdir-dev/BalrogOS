#include "BalrogOS/Syscall/syscall.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/Tasking/tasking.h"
#include "BalrogOS/Memory/kheap.h"
#include "klib/IO/kprint.h"

extern process* current_running;

void sys_setuid(interrupt_regs* stack_frame)
{
    if(current_running->uid != 0)
    {
        return;
    }
    current_running->uid = stack_frame->rdi;
}