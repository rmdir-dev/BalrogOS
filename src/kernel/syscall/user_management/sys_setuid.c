#include "balrog_os/syscall/syscall.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/tasking/tasking.h"
#include "balrog_os/memory/kheap.h"
#include "klib/io/kprint.h"

extern process* current_running;

void sys_setuid(interrupt_regs* stack_frame)
{
    if(current_running->uid != 0)
    {
        return;
    }
    current_running->uid = stack_frame->rdi;
}