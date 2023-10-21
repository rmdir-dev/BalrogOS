#include "BalrogOS/Syscall/syscall.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/Tasking/process.h"
#include "balrog/process/processing.h"
#include "BalrogOS/Debug/debug_output.h"
#include <errno.h>

extern process* current_running;

int sys_kill(interrupt_regs* stack_frame)
{
    process* proc = proc_get_process(stack_frame->rdi);

    if(proc)
    {
        switch (stack_frame->rsi)
        {
        case SIGKILL:
            proc_kill(proc, 0);
            break;
        
        default:
            break;
        }
        return 0;
    } else {
        *current_running->error_no = ESRCH;
    }
    return -1;
}