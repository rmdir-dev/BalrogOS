#include "balrog_os/syscall/syscall.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/tasking/tasking.h"
#include "balrog_os/debug/debug_output.h"

int sys_execve(interrupt_regs* stack_frame)
{
    exec_process(stack_frame->rdi, stack_frame->rsi, stack_frame->rdx);
    kernel_debug_output(KDB_LVL_CRITICAL, "error execve! SHOULD NOT PRINT!\n");
    return -1;
}