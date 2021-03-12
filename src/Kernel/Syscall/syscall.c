#include "BalrogOS/Syscall/syscall.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"

#include "lib/IO/kprint.h"

extern void sys_read(interrupt_regs* stack_frame);
extern void sys_write(interrupt_regs* stack_frame);
extern int sys_open(interrupt_regs* stack_frame);
extern void sys_close(interrupt_regs* stack_frame);
extern void sys_fstat(interrupt_regs* stack_frame);
extern uint64_t sys_getpid(interrupt_regs* stack_frame);
extern void sys_exit(interrupt_regs* stack_frame);
extern void sys_park(interrupt_regs* stack_frame);
extern void sys_setpark(interrupt_regs* stack_frame);

static uint64_t (*syscall[])(interrupt_regs*) = 
{
    [SYS_READ] sys_read,
    [SYS_WRITE] sys_write,
    [SYS_OPEN] sys_open,
    [SYS_CLOSE] sys_close,
    [SYS_FSTAT] sys_fstat,
    [SYS_GETPID] sys_getpid,
    [SYS_EXIT] sys_exit,
    [SYS_PARK] sys_park,
    [SYS_SETPARK] sys_setpark,
};

static interrupt_regs* syscall_handler(interrupt_regs* stack_frame)
{
    /*
        Syscall dispatcher
    */
    if(syscall[stack_frame->rax])
    {
        stack_frame->rax = syscall[stack_frame->rax](stack_frame);
    }
    
    return stack_frame;
}

void init_syscalls()
{
    register_interrupt_handler(INT_SYSCALL, syscall_handler);
    set_interrupt_flag(INT_SYSCALL, IDT_PRESENT | IDT_INTERRUPT | IDT_DPL_3);
}