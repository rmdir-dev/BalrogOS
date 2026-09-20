#include "balrog_os/syscall/syscall.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/cpu/state/cpu_state.h"
#include "balrog_os/tasking/process.h"
#include "balrog_os/debug/debug_output.h"

extern void schedule(size_t tick, uint16_t ms);

void sys_exit(interrupt_regs* stack_frame)
{
    // TODO Manage error
    /*
        RDI contain the error number
    */
    process* current_running = get_current_process();
    proc_kill_process(current_running->pid);
}