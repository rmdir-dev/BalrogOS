#include "BalrogOS/Syscall/syscall.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"

#include "klib/IO/kprint.h"

extern void sys_read(interrupt_regs* stack_frame);
extern void sys_write(interrupt_regs* stack_frame);
extern int sys_open(interrupt_regs* stack_frame);
extern void sys_close(interrupt_regs* stack_frame);
extern void sys_fstat(interrupt_regs* stack_frame);
extern int sys_brk(interrupt_regs* stack_frame);
extern int sys_getpid(interrupt_regs* stack_frame);
extern int sys_fork(interrupt_regs* stack_frame);
extern int sys_execve(interrupt_regs* stack_frame);
extern void sys_exit(interrupt_regs* stack_frame);
extern int sys_wait(interrupt_regs* stack_frame);
extern int sys_kill(interrupt_regs* stack_frame);
extern void sys_park(interrupt_regs* stack_frame);
extern void sys_setpark(interrupt_regs* stack_frame);

static int (*syscall[])(interrupt_regs*) = 
{
    [SYS_READ] sys_read,
    [SYS_WRITE] sys_write,
    [SYS_OPEN] sys_open,
    [SYS_CLOSE] sys_close,
    [SYS_FSTAT] sys_fstat,
    [SYS_BRK] sys_brk,
    [SYS_GETPID] sys_getpid,
    [SYS_FORK] sys_fork,
    [SYS_EXECVE] sys_execve,
    [SYS_EXIT] sys_exit,
    [SYS_WAIT] sys_wait,
    [SYS_KILL] sys_kill,
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