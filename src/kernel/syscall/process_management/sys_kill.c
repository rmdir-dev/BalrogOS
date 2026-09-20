#include "balrog_os/syscall/syscall.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/tasking/process.h"
#include "balrog/process/processing.h"
#include "balrog_os/debug/debug_output.h"
#include <errno.h>

#include "balrog_os/cpu/state/cpu_state.h"

int sys_kill(interrupt_regs* stack_frame)
{
    process* proc = proc_get_process(stack_frame->rdi);

    if(proc)
    {
        switch(stack_frame->rsi)
        {
        case SIGKILL:
            proc_kill(proc, 0);
            break;
        
        default:
            break;
        }
        return 0;
    } else
    {
        process* current_running = get_current_process();
        *current_running->error_no = ESRCH;
    }
    return -1;
}
