#pragma once

#include <stdint.h>
#include <stddef.h>

#define COM1 0x3f8
#define SERIAL_BUF 256 // Serial buffer size

#define IER_RX_AVAILABLE  0x01 // received data available
#define IER_TX_EMPTY      0x02 // transmitter holding register empty
#define IER_LINE_STATUS   0x04 // overrun, parity, framing, break
#define IER_MODEM_STATUS  0x08

/**
 * @brief Initialize the serial
 *
 * @return status 0 ok, 1 when there is no chip answering on COM1.
 */
int serial_init();

/**
 * @brief REQUIRE SERIAL INIT & INTERRUPT INIT !!!
 * @return
 */
void serial_irq_init();

/**
 * @brief Log what COM1 answered.
 */
void serial_log_info();

/**
 * @brief Read a single char from serial
 *
 * @return read char
 */
char serial_read_char();

/**
 * @brief send a single byte to serial
 *
 * @param c
 */
void serial_put_char(char c);

/**
 * @brief read bytes from serial.
 *
 * @param s
 */
void serial_write(const char *str, size_t size);