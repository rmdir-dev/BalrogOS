#include "BalrogOS/Syscall/syscall.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/Tasking/tasking.h"

extern process* current_running;

int sys_wait(interrupt_regs* stack_frame)
{
    return wait_process(stack_frame->rdi);
}