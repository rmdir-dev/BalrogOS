#include "balrog_os/tasking/process.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include <stdint.h>

#include "balrog_os/cpu/state/cpu_state.h"

int sys_getppid(interrupt_regs* stack_frame)
{
    process* current_running = get_current_process();

    if(!current_running->parent)
    {
        return 0;
    }

    return current_running->parent->pid;
}
