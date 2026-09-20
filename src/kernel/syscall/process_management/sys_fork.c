#include "balrog_os/syscall/syscall.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/cpu/state/cpu_state.h"
#include "balrog_os/tasking/tasking.h"

int sys_fork(interrupt_regs* stack_frame)
{
    process* current_running = get_current_process();
    return fork_process(current_running, stack_frame);
}