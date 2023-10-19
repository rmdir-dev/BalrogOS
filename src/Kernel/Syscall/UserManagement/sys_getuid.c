#include "BalrogOS/Syscall/syscall.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/Tasking/tasking.h"
#include "klib/IO/kprint.h"

extern process* current_running;

int sys_getuid(interrupt_regs* stack_frame)
{
    return (int) current_running->uid;
}