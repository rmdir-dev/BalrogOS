#include "balrog_os/syscall/syscall.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/tasking/tasking.h"

extern process* current_running;

int sys_wait(interrupt_regs* stack_frame)
{
    return wait_process(stack_frame->rdi);
}