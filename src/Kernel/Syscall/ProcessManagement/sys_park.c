#include "BalrogOS/Syscall/syscall.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/Tasking/process.h"

#include "klib/IO/kprint.h"
uint64_t park_loop = 0;

extern process* current_running;

static uint64_t about_to_park;

void sys_park(interrupt_regs* stack_frame)
{
    if(stack_frame->rdi)
    {
        about_to_park = 0;
        proc_transfert_to_ready(stack_frame->rdi);
    } else if(about_to_park == current_running->pid)
    {
        proc_transfert_to_waiting(current_running->pid);
    }
}

void sys_setpark(interrupt_regs* stack_frame)
{
    about_to_park = current_running->pid;
}