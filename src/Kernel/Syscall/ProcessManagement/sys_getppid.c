#include "BalrogOS/Tasking/process.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include <stdint.h>

extern process* current_running;

int sys_getppid(interrupt_regs* stack_frame)
{
    if(!current_running->parent)
    {
        return 0;
    }

    return current_running->parent->pid;
}