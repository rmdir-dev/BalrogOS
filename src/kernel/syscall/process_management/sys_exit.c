#include "balrog_os/syscall/syscall.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/tasking/process.h"
#include "balrog_os/debug/debug_output.h"

extern process* current_running;
extern void schedule(size_t tick, uint16_t ms);

void sys_exit(interrupt_regs* stack_frame)
{
    // TODO Manage error
    /*
        RDI contain the error number
    */
    proc_kill_process(current_running->pid);
}