#include "BalrogOS/Syscall/syscall.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/Tasking/tasking.h"

extern process* current_running;

int sys_execve(interrupt_regs* stack_frame)
{
    exec_process(stack_frame->rdi, stack_frame->rsi, 1);
    kprint("error execve! SHOULD NOT PRINT!\n");
    return 0;
}