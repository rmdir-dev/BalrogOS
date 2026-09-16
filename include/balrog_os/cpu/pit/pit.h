#pragma once
#include <stdint.h>
#include "balrog/time/time.h"
#include "balrog_os/cpu/interrupts/interrupt.h"

typedef void (*pit_event)(size_t tick, uint16_t ms);

/**
 * @brief initialize the programmable interval timer
 *
 * @param frequency the timer frequency
 */
void init_pit(pit_event scheduler);

void get_relative_time(timespec* time, timespec* relative_time);

/**
 * @brief compare if the time specified is before the current time
 *
 * @param time
 * @return
 */
int pit_compare(timespec* time);

/**
 * @brief Set what gets called on every tick, whichever timer is driving us.
 *        init_pit does it on its own, the local apic timer calls this.
 *
 * @param event the scheduler entry point
 */
void timer_set_event(pit_event event);

/**
 * @brief The handler for the local apic timer.
 *        Same bookkeeping as the pit one. The counters are shared, so sleeping
 *        and get_relative_time keep working whichever timer runs. The end of
 *        interrupt goes to the local apic and not to the 8259.
 *
 * @param stack_frame
 * @return interrupt_regs*
 */
interrupt_regs* timer_lvt_handler(interrupt_regs* stack_frame);
