#include "balrog_os/drivers/serial/serial.h"
#include "balrog_os/cpu/ports/ports.h"
#include "balrog_os/cpu/interrupts/interrupt.h"
#include "balrog_os/cpu/interrupts/irq.h"
#include "balrog_os/debug/debug_output.h"

/*
Serial port driver, 16550 UART on COM1
Documentation : 
    Serial Ports : https://wiki.osdev.org/Serial_Ports
*/

static char rx_buf[SERIAL_BUF];
static volatile uint32_t rx_head;
static volatile uint32_t rx_tail;

static int serial_present = 0;

static uint8_t serial_loopback = 0;

static int __lsr_wait(uint8_t mask)
{
    for (int i = 0; i < 100000; i++)
    {
        if (in_byte(COM1 + 5) & mask)
        {
            return 1;
        }
    }

    return 0;
}

int serial_init()
{
    out_byte(COM1 + 1, 0x00); // Disable all interrupts
    out_byte(COM1 + 3, 0x80); // Enable DLAB (set baud rate divisor)
    out_byte(COM1 + 0, 0x01); // Set divisor to 3 (lo byte) 38400 baud
    out_byte(COM1 + 1, 0x00); //                  (hi byte)
    out_byte(COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
    out_byte(COM1 + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    out_byte(COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set

    // TEST
    out_byte(COM1 + 4, 0x1E);    // Set in loopback mode, test the serial chip
    out_byte(COM1 + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)
    int ret = 0;

    // wait.
    __lsr_wait(0x01);

    // Check if serial is faulty (i.e: not same byte as sent)
    serial_loopback = in_byte(COM1 + 0);
    if(serial_loopback != 0xAE) {
        ret = 1;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    out_byte(COM1 + 4, 0x0F);

    serial_present = (ret == 0);

    return ret;
}

void serial_log_info()
{
    if(!serial_present)
    {
        kernel_debug_output(KDB_LVL_INFO, "serial : nothing on COM1 0%x, loopback read back 0%x",
            COM1, serial_loopback);
        return;
    }

    kernel_debug_output(KDB_LVL_INFO, "serial : COM1 0%x, 38400 baud, 8n1, fifo on", COM1);
}

char serial_read_char()
{
    if(!serial_present)
    {
        return 0;
    }

    while(rx_head == rx_tail)
    {}

    char c = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) % SERIAL_BUF;

    return c;
}

void serial_put_char(char c)
{
    if(!serial_present)
    {
        return;
    }

    if (c == '\n')
    {
        serial_put_char('\r');
    }

    if (__lsr_wait(0x20) == 0)
    {
        return;
    }

    out_byte(COM1, c);
}

void serial_write(const char *str, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        serial_put_char(str[i]);
    }
}

static interrupt_regs* __serial_int_handler(interrupt_regs *stack_frame)
{
    while (in_byte(COM1 + 5) & 0x01)
    {
        char c = in_byte(COM1);
        uint32_t next = (rx_head + 1) % SERIAL_BUF;

        if (next != rx_tail)
        {
            rx_buf[rx_head] = c;
            rx_head = next;
        }
    }

    irq_end(INT_IRQ_4);

    return stack_frame;
}

void serial_irq_init()
{
    // no need to initialize irq4 if serial is not present.
    if(!serial_present)
    {
        return;
    }

    kernel_debug_output(KDB_LVL_INFO, "serial : irq %d unmasked, rx interrupts on", INT_IRQ_4);
    register_interrupt_handler(INT_IRQ_4, &__serial_int_handler);
    irq_pic_toggle_mask_bit(INT_IRQ_4);
    out_byte(COM1 + 1, IER_RX_AVAILABLE); // Enable interrupts
}