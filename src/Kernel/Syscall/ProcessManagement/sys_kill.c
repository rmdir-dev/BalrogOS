#include "BalrogOS/Syscall/syscall.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/Tasking/process.h"
#include "balrog/process/processing.h"

int sys_kill(interrupt_regs* stack_frame)
{
    process* proc = proc_get_process(stack_frame->rdi);
    if(proc)
    {
        switch (stack_frame->rsi)
        {
        case SIGKILL:
            proc_kill(proc);
            break;
        
        default:
            break;
        }
    }
    return 0;
}