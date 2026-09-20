#include "balrog_os/syscall/syscall.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog/debug/debug.h"
#include "balrog_os/cpu/state/cpu_state.h"
#include "balrog_os/debug/debug_output.h"
#include "balrog_os/tasking/tasking.h"
#include "balrog_os/tasking/process.h"

extern int sys_read(interrupt_regs* stack_frame);
extern void sys_write(interrupt_regs* stack_frame);
extern int sys_open(interrupt_regs* stack_frame);
extern void sys_close(interrupt_regs* stack_frame);
extern void sys_fstat(interrupt_regs* stack_frame);
extern int sys_brk(interrupt_regs* stack_frame);
extern int sys_sleep(interrupt_regs* stack_frame);
extern int sys_getpid(interrupt_regs* stack_frame);
extern int sys_fork(interrupt_regs* stack_frame);
extern int sys_execve(interrupt_regs* stack_frame);
extern void sys_exit(interrupt_regs* stack_frame);
extern int sys_wait(interrupt_regs* stack_frame);
extern int sys_kill(interrupt_regs* stack_frame);
extern int sys_creat(interrupt_regs* stack_frame);
extern int sys_mkdir(interrupt_regs* stack_frame);
extern int sys_unlink(interrupt_regs* stack_frame);
extern int sys_rmdir(interrupt_regs* stack_frame);
extern void* sys_getcwd(interrupt_regs* stack_frame);
extern int sys_chdir(interrupt_regs* stack_frame);
extern int sys_getuid(interrupt_regs* stack_frame);
extern void sys_setuid(interrupt_regs* stack_frame);
extern int sys_getppid(interrupt_regs* stack_frame);
extern int sys_mount(interrupt_regs* stack_frame);
extern int sys_umount(interrupt_regs* stack_frame);
extern void sys_reboot(interrupt_regs* stack_frame);
extern void sys_park(interrupt_regs* stack_frame);
extern void sys_setpark(interrupt_regs* stack_frame);
extern void sys_debug(interrupt_regs* stack_frame);

static int (*syscall[SYSCALL_MAX])(interrupt_regs*) =
{
    [SYS_READ] &sys_read,
    [SYS_WRITE] &sys_write,
    [SYS_OPEN] &sys_open,
    [SYS_CLOSE] &sys_close,
    [SYS_FSTAT] &sys_fstat,
    [SYS_BRK] &sys_brk,
    [SYS_NANOSLEEP] &sys_sleep,
    [SYS_GETPID] &sys_getpid,
    [SYS_FORK] &sys_fork,
    [SYS_EXECVE] &sys_execve,
    [SYS_EXIT] &sys_exit,
    [SYS_WAIT] &sys_wait,
    [SYS_KILL] &sys_kill,
    [SYS_CREAT] &sys_creat,
    [SYS_MKDIR] &sys_mkdir,
    [SYS_UNLINK] &sys_unlink,
    [SYS_RMDIR] &sys_rmdir,
    [SYS_GETCWD] &sys_getcwd,
    [SYS_CHDIR] &sys_chdir,
    [SYS_GETUID] &sys_getuid,
    [SYS_SETUID] &sys_setuid,
    [SYS_GETPPID] &sys_getppid,
    [SYS_MOUNT] &sys_mount,
    [SYS_UMOUNT] &sys_umount,
    [SYS_REBOOT] &sys_reboot,
    [SYS_PARK] &sys_park,
    [SYS_SETPARK] &sys_setpark,
    [SYS_DEBUG] &sys_debug,
};

static const char* syscall_name[SYSCALL_MAX] =
{
    [SYS_READ] = "read",        [SYS_WRITE] = "write",      [SYS_OPEN] = "open",
    [SYS_CLOSE] = "close",      [SYS_FSTAT] = "fstat",      [SYS_BRK] = "brk",
    [SYS_NANOSLEEP] = "nanosleep", [SYS_GETPID] = "getpid", [SYS_FORK] = "fork",
    [SYS_EXECVE] = "execve",    [SYS_EXIT] = "exit",        [SYS_WAIT] = "wait",
    [SYS_KILL] = "kill",        [SYS_CREAT] = "creat",      [SYS_MKDIR] = "mkdir",
    [SYS_UNLINK] = "unlink",    [SYS_RMDIR] = "rmdir",      [SYS_GETCWD] = "getcwd",
    [SYS_CHDIR] = "chdir",      [SYS_GETUID] = "getuid",    [SYS_SETUID] = "setuid",
    [SYS_GETPPID] = "getppid",  [SYS_REBOOT] = "reboot",    [SYS_PARK] = "park",
    [SYS_SETPARK] = "setpark",  [SYS_DEBUG] = "debug",
};

static interrupt_regs* syscall_handler(interrupt_regs* stack_frame)
{
    process* current_running = get_current_process();
    if(stack_frame->rax >= SYSCALL_MAX)
    {
        kernel_debug_output(KDB_LVL_CRITICAL, "syscall %d is out of the table, pid %d",
                stack_frame->rax, current_running->pid);

        proc_kill_process(current_running->pid);
        return stack_frame;
    }

    if(!syscall[stack_frame->rax])
    {
        kernel_debug_output(KDB_LVL_CRITICAL, "Unknown syscall %d", stack_frame->rax);

        proc_kill_process(current_running->pid);
        return stack_frame;
    }

    static uint64_t last_call = SYSCALL_MAX;
    static int last_pid = -1;
    static size_t repeated = 0;

    if(stack_frame->rax == last_call && current_running->pid == last_pid)
    {
        repeated++;
    } else
    {
        if(repeated)
        {
            kernel_debug_output(KDB_LVL_VERBOSE, "syscall %s pid %d, %d more of the same",
                    syscall_name[last_call] ? syscall_name[last_call] : "?",
                    last_pid, repeated);
            repeated = 0;
        }

        last_call = stack_frame->rax;
        last_pid = current_running->pid;

        kernel_debug_output(KDB_LVL_VERBOSE, "syscall %d %s pid %d, rdi 0%x rsi 0%x rdx 0%x",
                stack_frame->rax,
                syscall_name[stack_frame->rax] ? syscall_name[stack_frame->rax] : "?",
                current_running->pid,
                stack_frame->rdi, stack_frame->rsi, stack_frame->rdx);
    }

    /*  rax holds the return value on the way out, so the number has to be
        kept if we still want to name the call afterwards.  */
    uint64_t called = stack_frame->rax;

    /*
        Syscall dispatcher
    */
    if(syscall[stack_frame->rax])
    {
        stack_frame->rax = syscall[stack_frame->rax](stack_frame);
    }

    if((long) stack_frame->rax < 0)
    {
        kernel_debug_output(KDB_LVL_INFO, "syscall %s returned %d to pid %d",
                syscall_name[called] ? syscall_name[called] : "?",
                (long) stack_frame->rax, current_running->pid);
    }

    return stack_frame;
}

int init_syscalls()
{
    register_interrupt_handler(INT_SYSCALL, syscall_handler);
    set_interrupt_flag(INT_SYSCALL, IDT_PRESENT | IDT_INTERRUPT | IDT_DPL_3);

    return 0;
}