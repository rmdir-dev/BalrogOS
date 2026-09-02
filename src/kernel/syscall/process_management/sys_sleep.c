#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/tasking/tasking.h"
#include "balrog_os/tasking/proc_sleep.h"
#include "balrog/time/time.h"
#include "balrog_os/cpu/pit/pit.h"

extern process* current_running;

int sys_sleep(interrupt_regs* stack_frame)
{
    timespec* rqtp = (timespec*)stack_frame->rdi;
    timespec* rmtp = (timespec*)stack_frame->rsi;
    timespec relative_time = {0, 0};
    get_relative_time(rqtp, &relative_time);

    sleep(&relative_time, current_running);

    return 0;
}