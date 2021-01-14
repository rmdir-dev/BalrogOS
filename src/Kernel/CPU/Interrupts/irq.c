#include "irq.h"
#include "interrupt.h"
#include "Kernel/CPU/ports/ports.h"

static interrupt_regs* temporary_irq_0_handler(interrupt_regs* stack_frame)
{
    irq_end(INT_IRQ_0);
    return stack_frame;
}

void init_irq()
{
    register_interrupt_handler(INT_IRQ_0, temporary_irq_0_handler);
}

void irq_end(uint8_t id)
{
    if(id >= INT_IRQ_8)
    {
        out_byte(0xA0, 0x20);
    }
    out_byte(0x20, 0x20);
}