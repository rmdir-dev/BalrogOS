#include "BalrogOS/Syscall/syscall.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/Tasking/process.h"

#include "klib/IO/kprint.h"

extern process* current_running;
extern void schedule(size_t tick, uint16_t ms);

void sys_exit(interrupt_regs* stack_frame)
{
    // TODO Manage error 
    /*
        RDI contain the error number
    */
    proc_kill_process(current_running->pid);
}