#include "BalrogOS/CPU/Scheduler/Scheduler.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/CPU/Interrupts/irq.h"
#include <stdio.h>

/*
Will be use as temporary test scheduler.
*/
static interrupt_regs* round_robin_shedule(interrupt_regs* stack_frame)
{
    //printf("Schedule work!\n");
    irq_end(INT_IRQ_0);
    return stack_frame;
}

void init_scheduler()
{
    register_interrupt_handler(INT_IRQ_0, round_robin_shedule);
    irq_pic_toggle_mask_bit(INT_IRQ_0);
}