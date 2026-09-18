#include "balrog_os/cpu/interrupts/irq.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/cpu/ports/ports.h"
#include "klib/io/kprint.h"

void irq_end(uint8_t id)
{
    if(id >= INT_IRQ_8)
    {
        /*
            if IRQ > IRQ_8 then it require to reset the state 
            of the slave PIC
        */
        out_byte(0xA0, 0x20);
    }
    /*
        reset the state of the master PIC
    */
    out_byte(0x20, 0x20);
}

static int __irq_pic_bit(uint8_t irq_id, uint8_t* irq_bit, uint16_t* port)
{
    if(irq_id < INT_IRQ_0)
    {
        return -1;
    }

    if(irq_id < INT_IRQ_8)
    {
        *irq_bit = irq_id - INT_IRQ_0;
        *port = 0x21;   // set master PIC mask
        return 0;
    }

    if(irq_id < INT_IRQ_16)
    {
        *irq_bit = irq_id - INT_IRQ_8;
        *port = 0xa1;   // set slave PIC mask
        return 0;
    }

    return -1;
}

void irq_pic_mask(uint8_t irq_id)
{
    uint8_t irq_bit;
    uint16_t port;

    if(__irq_pic_bit(irq_id, &irq_bit, &port) != 0)
    {
        return;
    }

    out_byte(port, in_byte(port) | (1 << irq_bit));
}

void irq_pic_unmask(uint8_t irq_id)
{
    uint8_t irq_bit;
    uint16_t port;

    if(__irq_pic_bit(irq_id, &irq_bit, &port) != 0)
    {
        return;
    }

    out_byte(port, in_byte(port) & ~(1 << irq_bit));
}