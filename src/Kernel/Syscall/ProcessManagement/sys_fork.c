#include "BalrogOS/Syscall/syscall.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/Tasking/tasking.h"

extern process* current_running;

int sys_fork(interrupt_regs* stack_frame)
{
    return fork_process(current_running, stack_frame);
}