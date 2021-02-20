#include "BalrogOS/Syscall/syscall.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/Tasking/process.h"

#include <stdio.h>
uint64_t park_loop = 0;

extern process* current_running;

void sys_park(interrupt_regs* stack_frame)
{
    //printf("park rdi : %d | currently running pid : %d \n", stack_frame->rdi, current_running->pid);

    if(stack_frame->rdi)
    {
        proc_transfert_to_ready(stack_frame->rdi);
    } else 
    {
        proc_transfert_to_waiting(current_running->pid);
    }
}