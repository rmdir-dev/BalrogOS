#include "balrog_os/tasking/process.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include <stdint.h>

#include "balrog_os/cpu/state/cpu_state.h"

int sys_getpid(interrupt_regs* stack_frame)
{
    process* current_running = get_current_process();
    return current_running->pid;
}
