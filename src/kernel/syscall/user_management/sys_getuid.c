#include "balrog_os/syscall/syscall.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/tasking/tasking.h"
#include "klib/io/kprint.h"

extern process* current_running;

int sys_getuid(interrupt_regs* stack_frame)
{
    return (int) current_running->uid;
}