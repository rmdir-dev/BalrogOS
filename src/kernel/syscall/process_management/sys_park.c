#include "balrog_os/syscall/syscall.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/cpu/state/cpu_state.h"
#include "balrog_os/tasking/process.h"

#include "klib/io/kprint.h"
uint64_t park_loop = 0;

static uint64_t about_to_park;

void sys_park(interrupt_regs* stack_frame)
{
    process* current_running = get_current_process();

    if(stack_frame->rdi)
    {
        about_to_park = 0;
        proc_transfert_to_ready(stack_frame->rdi, PROCESS_STATE_WAITING);
    } else if(about_to_park == current_running->pid)
    {
        proc_to_sleep(current_running->pid, PROCESS_STATE_WAITING);
    }
}

void sys_setpark(interrupt_regs* stack_frame)
{
    process* current_running = get_current_process();
    about_to_park = current_running->pid;
}