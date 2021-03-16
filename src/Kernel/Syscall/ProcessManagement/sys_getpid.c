#include "BalrogOS/Tasking/process.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include <stdint.h>

extern process* current_running;

int sys_getpid(interrupt_regs* stack_frame)
{
    return current_running->pid;
}