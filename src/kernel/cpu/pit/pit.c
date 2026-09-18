#include "balrog_os/cpu/pit/pit.h"
#include "balrog_os/cpu/ports/ports.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/cpu/interrupts/irq.h"
#include "balrog_os/cpu/apic/apic.h"
#include "balrog_os/tasking/proc_sleep.h"
#include "balrog_os/debug/debug_output.h"
#include <stddef.h>


#define FREQUENCY 100
static unsigned long timer_ticks = 0;
static unsigned char timer_ms = 0;
static pit_event scheduler_event = NULL;
extern void wake_up(size_t tick, uint16_t ms);

void _init_clock_frequency(uint32_t frequency) {
    // calculate the divisor to get the correct frequency
    // 1193180 is the tick rate
    uint32_t divisor = 1193180 / frequency;

    /* Send mode command :
            - 0b00110110 = 0x36
            0b
            00  CHANNEL (using chanel 0)
            11  ACCESS MODE (send low byte then hight byte)
            011 OPERATING MODE (MODE 3 square wave generator)
            0   BINARY MODE (16 bit binary)
    */
    out_byte(0x43, 0x36);

    // Set the low and high byte to be sent to the PIT
    uint8_t low = (uint8_t)(divisor & 0xff);
    uint8_t high = (uint8_t)((divisor >> 8) & 0xff);

    // Send the low then the high byte to the PIT
    out_byte(0x40, low);
    out_byte(0x40, high);

    kernel_debug_output(KDB_LVL_INFO, "pit : channel 0 at %d Hz, divisor %d, mode 3", frequency, divisor);
}

static void __on_tick()
{
    if(++timer_ms == FREQUENCY) {
        timer_ms = 0;
        timer_ticks++;
    }

    wake_up(timer_ticks, timer_ms);
    if(scheduler_event != NULL) {
        scheduler_event(timer_ticks, timer_ms);
    }
}

void timer_set_event(pit_event event)
{
    scheduler_event = event;
}

interrupt_regs* timer_lvt_handler(interrupt_regs* stack_frame)
{
    lapic_eoi();
    __on_tick();

    return stack_frame;
}

interrupt_regs* irq0_handler(interrupt_regs* stack_frame) {
    irq_end(INT_IRQ_0);
    __on_tick();

    return stack_frame;
}

void get_relative_time(timespec* time, timespec* relative_time)
{
    relative_time->sec = timer_ticks + time->sec;
    relative_time->msec = timer_ms + time->msec;
    if(relative_time->msec > FREQUENCY) {
        relative_time->sec += 1;
        relative_time->msec = relative_time->msec % FREQUENCY;
    }
}

int pit_compare(timespec* time) {
    // check that time is still in the future
    return timer_ticks > time->sec || (timer_ticks == time->sec && timer_ms > time->msec);
}

void init_pit(pit_event scheduler)
{
    kernel_debug_output(KDB_LVL_INFO, "pit : driving the scheduler on irq %d", INT_IRQ_0);
    irq_pic_unmask(INT_IRQ_0);
    register_interrupt_handler(INT_IRQ_0, &irq0_handler);
    timer_set_event(scheduler);

    _init_clock_frequency(FREQUENCY);
}