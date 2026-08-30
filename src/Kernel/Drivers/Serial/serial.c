#include "string.h"
#include "BalrogOS/CPU/Ports/ports.h"

/*
Serial port driver, 16550 UART on COM1
Documentation : 
    Serial Ports : https://wiki.osdev.org/Serial_Ports
*/

#define COM1 0x3f8

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
    if(in_byte(COM1 + 0) != 0xAE) {
        ret = 1;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    out_byte(COM1 + 4, 0x0F);
    return ret;
}

static int __serial_receive_byte()
{
    return in_byte(COM1 + 5) & 1;
}

char serial_read_char()
{
    while(__serial_receive_byte() == 0)
    {}

    return in_byte(COM1);
}

void serial_read_buffer(char* buffer, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        buffer[i] = serial_read_char();
    }
}

void serial_put_char(char c)
{
    if (__lsr_wait(0x20) == 0)
    {
        return;
    }

    out_byte(COM1, c);
}

void serial_write(const char *s)
{
    while (*s)
    {
        serial_put_char(*s);
        s++;
    }
}