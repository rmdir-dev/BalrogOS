#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/Tasking/tasking.h"
#include "BalrogOS/Tasking/proc_sleep.h"
#include "balrog/time/time.h"
#include "BalrogOS/CPU/PIT/pit.h"

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