#include "BalrogOS/CPU/Interrupts/irq.h"
#include "BalrogOS/CPU/Interrupts/interrupt.h"
#include "BalrogOS/CPU/Ports/ports.h"
#include <stdio.h>

/*
static interrupt_regs* temporary_irq_0_handler(interrupt_regs* stack_frame)
{
    irq_end(INT_IRQ_0);
    printf("irq 0 \n");
    return stack_frame;
}

void init_irq()
{
    register_interrupt_handler(INT_IRQ_0, temporary_irq_0_handler);
}
*/

void irq_end(uint8_t id)
{
    if(id >= INT_IRQ_8)
    {
        out_byte(0xA0, 0x20);
    }
    out_byte(0x20, 0x20);
}

void irq_pic_toggle_mask_bit(uint8_t irq_id)
{
    uint8_t irq_bit;
    uint16_t port;
    if(irq_id < INT_IRQ_8)
    {
        irq_bit = irq_id - INT_IRQ_0;
        port = 0x21;
    } else if(irq_id < INT_IRQ_16)
    {
        irq_bit = irq_id - INT_IRQ_8;
        port = 0xa1;
    } else 
    {
        return;
    }
    uint8_t PIC_Master_Mask = in_byte(port);
    PIC_Master_Mask ^= (1 << irq_bit);
    out_byte(port, PIC_Master_Mask);
}